#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include "../include/asoundlib.h"
#include <sys/time.h>
#include <math.h>


static int set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *params, snd_pcm_access_t access);
static int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams);

struct transfer_method {
        const char *name;
        snd_pcm_access_t access;
        int (*transfer_loop)(snd_pcm_t *handle,
                             signed short *samples,
                             snd_pcm_channel_area_t *areas);
};
