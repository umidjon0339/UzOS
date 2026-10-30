/*
 * packet.c - The packet pool, and moving 'data' to add or remove headers.
 *
 * The pool is a static array, chained into a free list. Interrupts are
 * off while the list changes: the card's interrupt allocates too.
 *
 * Uses: cpu_irq_save / cpu_irq_restore (arch/x86/cpu), memcpy (lib/string).
 */

#include "modules/network_management/core/packet/packet.h"   // this file
#include "arch/x86/cpu/cpu.h"                                 // cpu_irq_save, cpu_irq_restore
#include "lib/string/string.h"                                // memcpy

static packet_t pool[PACKET_POOL];
static packet_t *free_list;
static int free_count;
static int ready;
static uint32_t failures;

static void init_pool(void) {
    for (int i = 0; i < PACKET_POOL; i++) {
        pool[i].next = free_list;
        free_list = &pool[i];
    }
    free_count = PACKET_POOL;
    ready = 1;
}

packet_t *packet_alloc(void) {
    uint32_t flags = cpu_irq_save();
    if (!ready)
        init_pool();
    packet_t *p = free_list;
    if (p) {
        free_list = p->next;
        free_count--;
    } else {
        failures++;
    }
    cpu_irq_restore(flags);
    if (!p)
        return 0;

    p->next = 0;
    p->dev = 0;
    p->data = p->buf + PACKET_HEADROOM;
    p->len = 0;
    p->mac_header = p->net_header = p->transport_header = 0;
    return p;
}

void packet_free(packet_t *p) {
    if (!p)
        return;
    uint32_t flags = cpu_irq_save();
    p->next = free_list;
    free_list = p;
    free_count++;
    cpu_irq_restore(flags);
}

packet_t *packet_from(const void *bytes, uint16_t len) {
    if (len > PACKET_DATA_MAX)
        return 0;
    packet_t *p = packet_alloc();
    if (p)
        memcpy(packet_put(p, len), bytes, len);
    return p;
}

/* ---------- Moving the edges ---------- */

uint8_t *packet_put(packet_t *p, uint16_t n) {
    uint8_t *end = p->data + p->len;
    if (end + n > p->buf + sizeof(p->buf))
        return 0;                   // no room at the end
    p->len += n;
    return end;
}

uint8_t *packet_push(packet_t *p, uint16_t n) {
    if (p->data - n < p->buf)
        return 0;                   // no headroom left
    p->data -= n;
    p->len += n;
    return p->data;
}

uint8_t *packet_pull(packet_t *p, uint16_t n) {
    if (n > p->len)
        return 0;                   // shorter than its header claims
    uint8_t *header = p->data;
    p->data += n;
    p->len -= n;
    return header;
}

void packet_trim(packet_t *p, uint16_t len) {
    if (len < p->len)
        p->len = len;
}

int packet_free_count(void) {
    return ready ? free_count : PACKET_POOL;
}

uint32_t packet_alloc_failures(void) {
    return failures;
}

/* ---------- Queues ---------- */

void packet_queue_push(packet_queue_t *q, packet_t *p) {
    uint32_t flags = cpu_irq_save();
    p->next = 0;
    if (q->tail)
        q->tail->next = p;
    else
        q->head = p;
    q->tail = p;
    q->count++;
    q->bytes += p->len;
    cpu_irq_restore(flags);
}

packet_t *packet_queue_pop(packet_queue_t *q) {
    uint32_t flags = cpu_irq_save();
    packet_t *p = q->head;
    if (p) {
        q->head = p->next;
        if (!q->head)
            q->tail = 0;
        q->count--;
        q->bytes -= p->len;
        p->next = 0;
    }
    cpu_irq_restore(flags);
    return p;
}

packet_t *packet_queue_peek(packet_queue_t *q) {
    return q->head;
}

void packet_queue_flush(packet_queue_t *q) {
    packet_t *p;
    while ((p = packet_queue_pop(q)))
        packet_free(p);
}
