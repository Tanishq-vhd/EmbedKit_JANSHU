/*
 * ringbuf.c
 * A circular (ring) buffer for uint8_t data, capacity 8 bytes.
 * Models a UART RX buffer: ISR (producer) writes bytes, main loop
 * (consumer) reads them.
 *
 * Build: gcc -Wall -std=c99 ringbuf.c -o ringbuf
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 8U   

/* Bonus: use (BUFFER_SIZE - 1) with & instead of % BUFFER_SIZE.
 * % compiles to a division, which is slow on MCUs without a hardware
 * divider; & is a single-cycle instruction. This only works because
 * BUFFER_SIZE is a power of 2: BUFFER_SIZE - 1 is all 1-bits in the
 * low bits (e.g. 8-1 = 0111b), so "index & (N-1)" == "index % N"
 * only when N is a power of 2. */
#define BUFFER_MASK (BUFFER_SIZE - 1U)

#define RINGBUF_OK         0
#define RINGBUF_ERR_FULL  -1
#define RINGBUF_ERR_EMPTY -2

typedef struct
{
    uint8_t  data[BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} ring_buffer_t;

void     ringbuf_init(ring_buffer_t *rb);
int      ringbuf_write(ring_buffer_t *rb, uint8_t byte);
int      ringbuf_read(ring_buffer_t *rb, uint8_t *byte);
uint16_t ringbuf_get_count(const ring_buffer_t *rb);
int      ringbuf_is_full(const ring_buffer_t *rb);
int      ringbuf_is_empty(const ring_buffer_t *rb);

void ringbuf_init(ring_buffer_t *rb)
{
    rb->head  = 0U;
    rb->tail  = 0U;
    rb->count = 0U;
    memset(rb->data, 0, sizeof(rb->data));
}

int ringbuf_write(ring_buffer_t *rb, uint8_t byte)
{
    if (ringbuf_is_full(rb))
    {
        return RINGBUF_ERR_FULL;
    }

    rb->data[rb->head] = byte;
    rb->head = (uint16_t)((rb->head + 1U) & BUFFER_MASK);
    rb->count++;

    return RINGBUF_OK;
}

int ringbuf_read(ring_buffer_t *rb, uint8_t *byte)
{
    if (ringbuf_is_empty(rb))
    {
        return RINGBUF_ERR_EMPTY;
    }

    *byte = rb->data[rb->tail];
    rb->tail = (uint16_t)((rb->tail + 1U) & BUFFER_MASK);
    rb->count--;

    return RINGBUF_OK;
}

uint16_t ringbuf_get_count(const ring_buffer_t *rb)
{
    return rb->count;
}

int ringbuf_is_full(const ring_buffer_t *rb)
{
    return (rb->count == BUFFER_SIZE) ? 1 : 0;
}

int ringbuf_is_empty(const ring_buffer_t *rb)
{
    return (rb->count == 0U) ? 1 : 0;
}

int main(void)
{
    ring_buffer_t rb;
    ringbuf_init(&rb);

    uint8_t initial_bytes[BUFFER_SIZE] =
        { 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48 };

    
    for (uint16_t i = 0U; i < BUFFER_SIZE; i++)
    {
        int result = ringbuf_write(&rb, initial_bytes[i]);
        if (result == RINGBUF_OK)
        {
            if (ringbuf_is_full(&rb))
            {
                printf("[WRITE] 0x%02X -> OK (count=%u) FULL\n",
                       initial_bytes[i], ringbuf_get_count(&rb));
            }
            else
            {
                printf("[WRITE] 0x%02X -> OK (count=%u)\n",
                       initial_bytes[i], ringbuf_get_count(&rb));
            }
        }
        else
        {
            printf("[WRITE] 0x%02X -> FAIL (buffer full)\n", initial_bytes[i]);
        }
    }

    if (ringbuf_is_full(&rb) && ringbuf_get_count(&rb) == BUFFER_SIZE)
    {
        printf("[CHECK] Buffer is FULL, count = %u\n", ringbuf_get_count(&rb));
    }

    
    {
        uint8_t extra_byte = 0x99;
        int result = ringbuf_write(&rb, extra_byte);
        if (result == RINGBUF_ERR_FULL)
        {
            printf("[WRITE] 0x%02X -> FAIL (buffer full)\n", extra_byte);
        }
        else
        {
            printf("[WRITE] 0x%02X -> OK (count=%u)\n",
                   extra_byte, ringbuf_get_count(&rb));
        }
    }

    
    for (int i = 0; i < 3; i++)
    {
        uint8_t value;
        int result = ringbuf_read(&rb, &value);
        if (result == RINGBUF_OK)
        {
            printf("[READ] -> 0x%02X (count=%u)\n", value, ringbuf_get_count(&rb));
        }
        else
        {
            printf("[READ] (empty) -> FAIL (buffer empty)\n");
        }
    }

    if (ringbuf_get_count(&rb) == 5U)
    {
        printf("[CHECK] count = %u (expected 5)\n", ringbuf_get_count(&rb));
    }

    
    uint8_t new_bytes[3] = { 0x49, 0x4A, 0x4B };
    for (int i = 0; i < 3; i++)
    {
        int result = ringbuf_write(&rb, new_bytes[i]);
        if (result == RINGBUF_OK)
        {
            printf("[WRITE] 0x%02X -> OK (count=%u)\n",
                   new_bytes[i], ringbuf_get_count(&rb));
        }
        else
        {
            printf("[WRITE] 0x%02X -> FAIL (buffer full)\n", new_bytes[i]);
        }
    }

    if (ringbuf_get_count(&rb) == 8U)
    {
        printf("[CHECK] count = %u (expected 8)\n", ringbuf_get_count(&rb));
    }

    
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        uint8_t value;
        int result = ringbuf_read(&rb, &value);
        if (result == RINGBUF_OK)
        {
            printf("[READ] -> 0x%02X (count=%u)\n", value, ringbuf_get_count(&rb));
        }
        else
        {
            printf("[READ] (empty) -> FAIL (buffer empty)\n");
        }
    }

    if (ringbuf_is_empty(&rb))
    {
        printf("[CHECK] Buffer is EMPTY, count = %u\n", ringbuf_get_count(&rb));
    }

    
    {
        uint8_t value;
        int result = ringbuf_read(&rb, &value);
        if (result == RINGBUF_ERR_EMPTY)
        {
            printf("[READ] (empty) -> FAIL (buffer empty)\n");
        }
        else
        {
            printf("[READ] -> 0x%02X (count=%u)\n", value, ringbuf_get_count(&rb));
        }
    }

    return 0;
}