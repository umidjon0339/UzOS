#ifndef PACKET_H
#define PACKET_H

/*
 * packet.h - Packet buffers: one frame, and room to add headers in front.
 *
 * (Linux calls this an "sk_buff".) Sending goes DOWN the layers, and
 * each layer puts its header IN FRONT of what the layer above made:
 *
 *   alloc            [ headroom ...................... |          ]
 *   put payload      [ headroom ...................... | payload  ]
 *   push TCP header  [ headroom .............. | TCP   | payload  ]
 *   push IP header   [ headroom ....... | IP   | TCP   | payload  ]
 *   push Ethernet    [ headroom | Eth   | IP   | TCP   | payload  ]
 *                               ^ data, len = everything from here
 *
 * Receiving goes UP: each layer reads its header and PULLs it off, so
 * 'data' points at what the next layer up should read.
 *
 * Buffers come from a fixed pool (PACKET_POOL); when it is empty new
 * frames are dropped, which is exactly what a flood does to a real OS.
 */

#include <stdint.h>

#define PACKET_HEADROOM 128     // Ethernet 14 + IP up to 60 + TCP up to 60
#define PACKET_DATA_MAX 1600    // one Ethernet frame and some spare
#define PACKET_POOL     128     // buffers in all

struct netdev;

typedef struct packet {
    struct packet *next;        // next in a queue
    struct netdev *dev;         // came in on / goes out on
    uint8_t *data;              // the first byte that counts
    uint16_t len;               // how many bytes count
    uint8_t *mac_header;        // set on the way up, for later layers
    uint8_t *net_header;        //   (ARP / IP reply to the sender's MAC,
    uint8_t *transport_header;  //    TCP needs the IP addresses, ...)
    uint8_t buf[PACKET_HEADROOM + PACKET_DATA_MAX];
} packet_t;

// A first-in first-out list of packets.
typedef struct {
    packet_t *head, *tail;
    uint32_t count;             // packets in it
    uint32_t bytes;             // their len, added up
} packet_queue_t;

// An empty packet (data at the end of the headroom, len 0), or NULL if
// the pool is used up.
packet_t *packet_alloc(void);
void packet_free(packet_t *p);

// A packet holding a copy of 'len' bytes (NULL if no buffer or too big).
packet_t *packet_from(const void *bytes, uint16_t len);

// Grow at the end by n bytes (the payload); returns where they start.
uint8_t *packet_put(packet_t *p, uint16_t n);

// Grow at the front by n bytes (a header); returns the new start.
uint8_t *packet_push(packet_t *p, uint16_t n);

// Remove n bytes from the front (a header that was read). NULL if the
// packet is shorter than n: a truncated packet.
uint8_t *packet_pull(packet_t *p, uint16_t n);

// Cut the packet to 'len' bytes (drop padding after the real data).
void packet_trim(packet_t *p, uint16_t len);

int packet_free_count(void);
uint32_t packet_alloc_failures(void);   // times the pool was empty

void packet_queue_push(packet_queue_t *q, packet_t *p);
packet_t *packet_queue_pop(packet_queue_t *q);  // NULL if empty
packet_t *packet_queue_peek(packet_queue_t *q); // the first, left in the queue
void packet_queue_flush(packet_queue_t *q);     // free them all

#endif
