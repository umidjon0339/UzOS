/*
 * inet.c - IPv4 and MAC addresses to and from text; the Internet checksum.
 *
 * Uses: format (lib/format).
 */

#include "modules/network_management/core/inet/inet.h"   // this file
#include "lib/format/format.h"                            // format

int inet_parse(const char *text, uint32_t *ip) {
    uint32_t result = 0;
    for (int part = 0; part < 4; part++) {
        if (*text < '0' || *text > '9')
            return 0;                       // every part needs a digit
        uint32_t value = 0;
        for (int digits = 0; *text >= '0' && *text <= '9'; digits++, text++) {
            value = value * 10 + (uint32_t)(*text - '0');
            if (value > 255 || digits >= 3)
                return 0;
        }
        result = (result << 8) | value;
        if (part < 3 && *text++ != '.')
            return 0;
    }
    if (*text)
        return 0;                           // something after the 4th part
    *ip = result;
    return 1;
}

void inet_format(uint32_t ip, char out[INET_ADDR_TEXT]) {
    format(out, INET_ADDR_TEXT, "%u.%u.%u.%u",
           ip >> 24, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

uint32_t inet_mask(int prefix) {
    if (prefix <= 0)
        return 0;
    if (prefix >= 32)
        return 0xFFFFFFFF;
    return 0xFFFFFFFF << (32 - prefix);
}

int inet_prefix(uint32_t mask) {
    int bits = 0;
    while (mask & 0x80000000) {
        bits++;
        mask <<= 1;
    }
    return bits;
}

void mac_format(const uint8_t mac[6], char out[MAC_TEXT]) {
    format(out, MAC_TEXT, "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/* ---------- Checksum ---------- */

// Add 'len' bytes to a running 32-bit sum, as 16-bit words.
static uint32_t sum_words(uint32_t sum, const void *data, uint32_t len) {
    const uint8_t *bytes = data;
    while (len > 1) {
        sum += (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8);   // the word as x86 reads it
        bytes += 2;
        len -= 2;
    }
    if (len)
        sum += bytes[0];                    // an odd last byte: padded with a zero
    return sum;
}

// Carry the overflow back in until it fits 16 bits, then invert.
static uint16_t fold(uint32_t sum) {
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

uint16_t inet_checksum(const void *data, uint32_t len) {
    return fold(sum_words(0, data, len));
}

uint16_t inet_checksum_pseudo(uint32_t src, uint32_t dst, uint8_t protocol,
                              const void *data, uint32_t len) {
    struct {
        uint32_t src, dst;
        uint8_t zero, protocol;
        uint16_t length;
    } __attribute__((packed)) pseudo = { htonl(src), htonl(dst), 0, protocol, htons((uint16_t)len) };
    return fold(sum_words(sum_words(0, &pseudo, sizeof(pseudo)), data, len));
}
