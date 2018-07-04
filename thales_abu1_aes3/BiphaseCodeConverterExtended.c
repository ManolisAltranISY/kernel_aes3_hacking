#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define LEFT_CHANNEL 0
#define SUBFRAME_EMPTY_1 1
#define RIGHT_CHANNEL 2
#define SUBFRAME_EMPTY_2 3

#define L_TIME  1
#define S_TIME  0

#define START_OF_CYCLE 0
#define MID_CYCLE 1

#define preamble_X_1 11100010 // 0xE2
#define preamble_X_2 00011101 // 0x1D
#define preamble_Y_1 11100100 // 0xE3
#define preamble_Y_2 00011011 // 0x1B
#define preamble_Z_1 11101000 // 0xE8
#define preamble_Z_2 00010111 // 0x17

// LETS ASSUME ITS CALIBRATED AND IT STARTS FROM THE BEGINNING
// THIS ONE USES INTERRUPTS ON BOTH EDGES...IT'S SIMPLER
//

int main ()
{

    int preamble_X_1 = 0b11100010; // 0xE2 or 226
    int preamble_X_2 = 0b00011101; // 0x1D or 29
    int preamble_Y_1 = 0b11100100; // 0xE3 or 227
    int preamble_Y_2 = 0b00011011; // 0x1B or 27
    int preamble_Z_1 = 0b11101000; // 0xE8 or 232
    int preamble_Z_2 = 0b00010111; // 0x17 or 23

    static int number_of_frames = 0;

    static int current_stage_of_subframe = LEFT_CHANNEL;
    static int count = 0;

    static int previous_state = START_OF_CYCLE;
    static int time_measured[10] = {L_TIME, S_TIME, S_TIME, L_TIME, S_TIME, S_TIME, L_TIME, L_TIME, S_TIME, S_TIME};
    static int previous_time = L_TIME;
    int value = 0;

    while(1)
    {
      if (current_stage_of_subframe == LEFT_CHANNEL || current_stage_of_subframe ==  RIGHT_CHANNEL)
      {
          for(int i = 0; i<=10; i++)
          {
              if (time_measured[i] == S_TIME && previous_state == START_OF_CYCLE)
              {
                value = 1;
                previous_state = MID_CYCLE;
                printf("The value is %d\n", value);
              }
              else if (time_measured[i] == S_TIME && previous_state == MID_CYCLE)
              {
                previous_state = START_OF_CYCLE;
              }
              else if (time_measured[i] == L_TIME)
              {
                value = 0;
                printf("The value is %d\n", value);
              }
          }
          count++;
          if (count>63)
          {
            count = 0;
            if (current_stage_of_subframe == LEFT_CHANNEL)
            {
              current_stage_of_subframe = RIGHT_CHANNEL;
            }
            else
            {
              current_stage_of_subframe = LEFT_CHANNEL;
            }
          }
      }

      else if (current_stage_of_subframe == SUBFRAME_EMPTY_1)
      {
        count++;
        if (count>191)
        {
          count = 0;
          current_stage_of_subframe = RIGHT_CHANNEL;
        }
      }
      else if (current_stage_of_subframe == SUBFRAME_EMPTY_2)
      {
        count++;
        if (count>447)
        {
          count = 0;
          current_stage_of_subframe = LEFT_CHANNEL;
          number_of_frames++;
          if (number_of_frames > 192) //due to empty frames, in terms of AES3, it's 192*6 = 1152 frames
          {
              number_of_frames = 0;
              // do something that will say, hey, here, take my buffer
          }
        }
      }
    }
}

void int preamble_detection(&time_measured){
  //add code, also we should probably add check if calibration is needed again
  //even after the software is originally calibrated
}
