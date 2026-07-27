#ifndef WLESS_RING_BUFFER_H_
#define WLESS_RING_BUFFER_H_

#include <stdbool.h>
#include <stdint.h>

#define WLESS_RB_SIZE 64U

typedef struct
{
    uint16_t buffer[WLESS_RB_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} WLESS_RingBuffer;

void WLESS_RB_init(WLESS_RingBuffer *rb);
bool WLESS_RB_write(WLESS_RingBuffer *rb, uint16_t data);
bool WLESS_RB_read(WLESS_RingBuffer *rb, uint16_t *data);

#endif
