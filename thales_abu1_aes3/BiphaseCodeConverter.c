#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define L_TIME  1
#define S_TIME  0

#define START_OF_CYCLE 0
#define MID_CYCLE 1

// LETS ASSUME ITS CALIBRATED AND IT STARTS FROM THE BEGINNING
// THIS ONE USES INTERRUPTS ON BOTH EDGES...IT'S SIMPLER
// THIS CODE WAS TESTED ON https://www.onlinegdb.com/online_c_compiler
// AND IT WORKED

int main ()
{
    static int previous_state = START_OF_CYCLE;
    static int time_measured[10] = {L_TIME, S_TIME, S_TIME, L_TIME, S_TIME, S_TIME, L_TIME, L_TIME, S_TIME, S_TIME};
    static int previous_time = L_TIME;
    int value = 0;
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
}
