/*
 *   AES3 Buffer written for Thales Proof of Concept regarding the conversion of
 *   the AES3 data to comprehensible audio.
 *   Author: Emmanouil Nikolakakis
 */

/*
 *  Look into splitting the buffer and the pcm functionality into two different
 *  modules and use a single int main() between them
 */

#include "AES3Buffer.h"

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <fstream.h>

#define preamble_X_1 11100010 // 0xE2
#define preamble_X_2 00011101 // 0x1D
#define preamble_Y_1 11100100 // 0xE3
#define preamble_Y_2 00011011 // 0x1B
#define preamble_Z_1 11101000 // 0xE8
#define preamble_Z_2 00010111 // 0x17

#define CIRCULAR_BUFFER_LENGTH 384
#define PCM_DEVICE "default"

static char sample[CIRCULAR_BUFFER_LENGTH];

static byte test[4];
memset(test,0x00,4);
test[]={0xb4,0xaf,0x98,0x1a};

static bool calibrated = 0;

// a lot of the ring_buffer functionality copied from
// https://embeddedartistry.com/blog/2017/4/6/circular-buffers-in-cc

typedef struct {
    uint8_t * buffer;
    size_t head;
    size_t tail;
    size_t size; //of the buffer
} circular_buf_t

/*
example:
circular_buf_t cbuf;
cbuf.size = RING_BUFFER_LENGTH;
cbuf.buffer = malloc(cbuf.size);
circular_buf_reset(&cbuf)
*/

int circular_buf_reset(circular_buf_t * cbuf)
{
    int r = -1;
    if(cbuf)
    {
        cbuf->head = 0;
        cbuf->tail = 0;
        r = 0;
    }
    return r;
}

bool circular_buf_empty(circular_buf_t cbuf)
{
    // We define empty as head == tail
    return (cbuf.head == cbuf.tail);
}

bool circular_buf_full(circular_buf_t cbuf)
{
    // We determine "full" case by head being one position behind the tail
    // Note that this means we are wasting one space in the buffer!
    // Instead, you could have an "empty" flag and determine buffer full that way
    return ((cbuf.head + 1) % cbuf.size) == cbuf.tail;
}

int circular_buf_put(circular_buf_t * cbuf, uint8_t data)
{
    int r = -1;
    if(cbuf)
    {
        cbuf->buffer[cbuf->head] = data;
        cbuf->head = (cbuf->head + 1) % cbuf->size;
        if(cbuf->head == cbuf->tail)
        {
            cbuf->tail = (cbuf->tail + 1) % cbuf->size;
        }
        r = 0;
    }
    return r;
}

int circular_buf_get(circular_buf_t * cbuf, uint32_t * data)
{
    int r = -1;
    if(cbuf && data && !circular_buf_empty(*cbuf))
    {
        *data = cbuf->buffer[cbuf->tail];
        cbuf->tail = (cbuf->tail + 1) % cbuf->size;
        r = 0;
    }
    return r;
}

void calibrate_circular_buffer(){
  // to be created
}
void preamble_detection(){
  // to be created
}
void aline_circular_buffer(){
  // to be created
}
void check_calibration(){
  // to be created
}
