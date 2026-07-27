#include "ring_buffer.h"

void WLESS_RB_init(WLESS_RingBuffer *rb)
{
    rb->head = 0U;
    rb->tail = 0U;
}

bool WLESS_RB_write(WLESS_RingBuffer *rb, uint16_t data)
{
    uint16_t next = (uint16_t)((rb->head + 1U) % WLESS_RB_SIZE);

    if(next == rb->tail)
    {
        return false;
    }

    rb->buffer[rb->head] = data;
    rb->head = next;
    return true;
}

bool WLESS_RB_read(WLESS_RingBuffer *rb, uint16_t *data)
{
    if(rb->head == rb->tail)
    {
        return false;
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (uint16_t)((rb->tail + 1U) % WLESS_RB_SIZE);
    return true;
}
