#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "AES3Buffer.h"
#include "ALSAHandler.h"

#define BUFFER_LENGTH 384
static char sample[BUFFER_LENGTH]

int main(){

    int ret, fd;
    printf("Starting the AES3 software\n");
    fd = open("/dev/AES3conv", O_RDWR)
    if (fd < 0){
        perror("Failed to open the device...")
        return errno;
    }

    while(1){
        //procedure
        ret = read(fd, sample, BUFFER_LENGTH);
        if (ret < 0){
            perror("Failed to read the message from the device.");
            return errno;
        }

        circular_buf_t sample_circular_buffer;
        sample_circular_buffer.size = CIRCULAR_BUFFER_LENGTH;
        sample_circular_buffer.buffer = malloc(cbuf.size);
        sample_circular_buffer_full = 0;
        sample_circular_buffer_empty = 0;
        circular_buf_reset(&sample_circular_buffer)

        while(!sample_circular_buffer_full){
            circular_buf_put()
        }
    }

}
