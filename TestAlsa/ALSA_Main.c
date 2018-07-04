/*
 *  This small demo sends a simple sinusoidal wave to your speakers.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include "../include/asoundlib.h"
#include <sys/time.h>
#include <math.h>
static char *device = "plughw:0,0";                     /* playback device */
static snd_pcm_format_t format = SND_PCM_FORMAT_S16;    /* sample format */
static unsigned int rate = 44100;                       /* stream rate */
static unsigned int channels = 1;                       /* count of channels */
static unsigned int buffer_time = 500000;               /* ring buffer length in us */
static unsigned int period_time = 100000;               /* period time in us */
static int verbose = 0;                                 /* verbose flag */
static int resample = 1;                                /* enable alsa-lib resampling */
static int period_event = 0;                            /* produce poll event after each period */
static snd_pcm_sframes_t buffer_size;
static snd_pcm_sframes_t period_size;
static snd_output_t *output = NULL;


static struct transfer_method transfer_methods[] = {
        { "write", SND_PCM_ACCESS_RW_INTERLEAVED, write_loop },
        { "write_and_poll", SND_PCM_ACCESS_RW_INTERLEAVED, write_and_poll_loop },
        { "async", SND_PCM_ACCESS_RW_INTERLEAVED, async_loop },
        { "async_direct", SND_PCM_ACCESS_MMAP_INTERLEAVED, async_direct_loop },
        { "direct_interleaved", SND_PCM_ACCESS_MMAP_INTERLEAVED, direct_loop },
        { "direct_noninterleaved", SND_PCM_ACCESS_MMAP_NONINTERLEAVED, direct_loop },
        { "direct_write", SND_PCM_ACCESS_MMAP_INTERLEAVED, direct_write_loop },
        { NULL, SND_PCM_ACCESS_RW_INTERLEAVED, NULL }
};

int main(int argc, char *argv[])
{
        snd_pcm_t *handle;
        int err;
        snd_pcm_hw_params_t *hwparams;
        snd_pcm_sw_params_t *swparams;
        int method = 0;
        signed short *samples;
        unsigned int chn;
        snd_pcm_channel_area_t *areas;
        snd_pcm_hw_params_alloca(&hwparams);
        snd_pcm_sw_params_alloca(&swparams);
        if ((err = snd_output_stdio_attach(&output, stdout, 0)) < 0) {
                printf("Output failed: %s\n", snd_strerror(err));
                return 0;
        }
        printf("Playback device is %s\n", device);
        printf("Stream parameters are %iHz, %s, %i channels\n", rate, snd_pcm_format_name(format), channels);
        printf("Using transfer method: %s\n", transfer_methods[method].name);
        if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
                printf("Playback open error: %s\n", snd_strerror(err));
                return 0;
        }

        if ((err = set_hwparams(handle, hwparams, transfer_methods[method].access)) < 0) {
                printf("Setting of hwparams failed: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        if ((err = set_swparams(handle, swparams)) < 0) {
                printf("Setting of swparams failed: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        if (verbose > 0)
                snd_pcm_dump(handle, output);
        samples = malloc((period_size * channels * snd_pcm_format_physical_width(format)) / 8);
        if (samples == NULL) {
                printf("No enough memory\n");
                exit(EXIT_FAILURE);
        }

        areas = calloc(channels, sizeof(snd_pcm_channel_area_t));
        if (areas == NULL) {
                printf("No enough memory\n");
                exit(EXIT_FAILURE);
        }
        for (chn = 0; chn < channels; chn++) {
                areas[chn].addr = samples;
                areas[chn].first = chn * snd_pcm_format_physical_width(format);
                areas[chn].step = channels * snd_pcm_format_physical_width(format);
        }
        err = transfer_methods[method].transfer_loop(handle, samples, areas);
        if (err < 0)
                printf("Transfer failed: %s\n", snd_strerror(err));
        free(areas);
        free(samples);
        snd_pcm_close(handle);
        return 0;
}
