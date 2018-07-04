#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <unistd.h>

#include <alloca.h>
#include <alsa/asoundlib.h>

int main(int argc, char **argv)
{
    snd_pcm_t *alsa;

    if (snd_pcm_open(&alsa, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)
        abort();

    if (snd_pcm_nonblock(alsa, 0) < 0)
        abort();

    snd_pcm_hw_params_t *alsa_hwparams;
    snd_pcm_sw_params_t *alsa_swparams;

    snd_pcm_hw_params_alloca(&alsa_hwparams);
    snd_pcm_sw_params_alloca(&alsa_swparams);

    if (snd_pcm_hw_params_any(alsa, alsa_hwparams) < 0)
        abort();
    if (snd_pcm_hw_params_set_format(alsa, alsa_hwparams, SND_PCM_FORMAT_S16) < 0)
        abort();
    if (snd_pcm_hw_params_set_access(alsa, alsa_hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
        abort();

    int channels = 2;
    if (snd_pcm_hw_params_set_channels_near(alsa, alsa_hwparams, &channels) < 0 || channels != 2)
        abort();

    if (snd_pcm_hw_params_set_rate_resample(alsa, alsa_hwparams, 0) < 0)
        abort();

    int rate = 8000;
    if (snd_pcm_hw_params_set_rate_near(alsa, alsa_hwparams, &rate, NULL) < 0 || rate != 8000)
        abort();

    if (snd_pcm_hw_params_set_buffer_time_near(alsa, alsa_hwparams, &(unsigned int){250000}, NULL) < 0)
        abort();

    if (snd_pcm_hw_params_set_periods_near(alsa, alsa_hwparams, &(unsigned int){16}, NULL) < 0)
        abort();

    if (snd_pcm_hw_params(alsa, alsa_hwparams) < 0)
        abort();

    if (snd_pcm_sw_params_current(alsa, alsa_swparams) < 0)
        abort();

    snd_pcm_uframes_t boundary;
    if (snd_pcm_sw_params_get_boundary(alsa_swparams, &boundary) < 0)
        abort();

    snd_pcm_uframes_t chunk_size;
    if (snd_pcm_hw_params_get_period_size(alsa_hwparams, &chunk_size, NULL) < 0)
        abort();

    if (snd_pcm_sw_params_set_start_threshold(alsa, alsa_swparams, chunk_size) < 0)
        abort();

    if (snd_pcm_sw_params_set_stop_threshold(alsa, alsa_swparams, boundary) < 0)
        abort();

    if (snd_pcm_sw_params_set_silence_size(alsa, alsa_swparams, boundary) < 0)
        abort();

    if (snd_pcm_sw_params(alsa, alsa_swparams) < 0)
        abort();

    int samples = 32768;
    char *stuff = calloc(samples, 4);
    if (!stuff)
        abort();

    fprintf(stderr, "start playing\n");


    snd_pcm_prepare(alsa);
    int r = snd_pcm_writei(alsa, stuff, samples);
    fprintf(stderr, "r=%d\n", r);

    usleep(300000);

    fprintf(stderr, "start drain\n");
    snd_pcm_drain(alsa);
    fprintf(stderr, "end drain\n");
    snd_pcm_close(alsa);
    free(stuff);
    return 0;
}
