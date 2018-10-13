#include "audionode.h"

#define ALSA_PCM_NEW_HW_PARAMS_API

#include <iostream>
#include <alsa/asoundlib.h>
#include "audioframe.h"
#include "config.h"

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
    defined(__BIG_ENDIAN__) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)

#define ALSA_FMT SND_PCM_FORMAT_S32_BE
#define ALSA_FMT_FALLBACK SND_PCM_FORMAT_S16_BE
#else
#define ALSA_FMT SND_PCM_FORMAT_S32_LE
#define ALSA_FMT_FALLBACK SND_PCM_FORMAT_S16_LE
#endif

void AudioNodeALSAOutput::tick() {
	if (!enabled) return;

	int err;

	if (!handle) {
		if ((err = open_device(GLOBAL_RATE, input.channels))) {
			std::cerr << "Unable to open audio device " << device << ": Error " << err << ": " << snd_strerror(err) << std::endl;
			enabled = false;
			return;
		}

		if (!handle) {
			std::cerr << "Unable to open audio device " << device << ": Unknown error. Retrying." << std::endl;
			return;
		}
	}

	if (output_channels > input.channels) {
		input.increase_channels(output_channels);
	}

	if (output_bitwidth == 16) {
		for (int i = 0; i < AUDIOFRAME_SIZE * input.channels; i++) {
			((int16_t*) input.data)[i] = static_cast<int16_t>(input.data[i] >> 16);
		}
	}

	else if (output_bitwidth == 8) {
		for (int i = 0; i < AUDIOFRAME_SIZE * input.channels; i++) {
			((int8_t*) input.data)[i] = static_cast<int8_t>(input.data[i] >> 24);
		}
	}
		
	err = -snd_pcm_writei(handle, input.data, AUDIOFRAME_SIZE);
	
	if (err == EPIPE) {
		std::cerr << "Underrun occurred on device " << device << std::endl;
	} else if (err > 0) {
		std::cerr << "Error writing frames to device " << device << ": Error " << err << ": " << snd_strerror(-err) << std::endl;
	}
}

int AudioNodeALSAOutput::open_device(int rate, int channels) {
	output_channels = channels;
	output_bitwidth = 32;

	int err;

	if ((err = snd_pcm_open(&handle, device.c_str(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) return err;

	snd_pcm_hw_params_alloca(&params);
	if ((err = snd_pcm_hw_params_any(handle, params)) < 0) return err;
	if ((err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) return err;
	if ((err = snd_pcm_hw_params_set_format(handle, params, ALSA_FMT)) < 0) {
		output_bitwidth = 16;
		if ((err = snd_pcm_hw_params_set_format(handle, params, ALSA_FMT_FALLBACK)) < 0) {
			output_bitwidth = 8;
			if ((err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S8)) < 0) return err;
		}
	}
	
	if ((err = snd_pcm_hw_params_set_channels(handle, params, channels)) < 0) {
		int chcount = 0;
		
		for (; chcount < 8; chcount++) {
			if (snd_pcm_hw_params_set_channels(handle, params, chcount) >= 0) break;

			if (chcount > 6) return err;
		}

		if (chcount < channels) return err;

		output_channels = chcount;

		std::cerr << "Warning: device " << device << " does not support using " << channels << " channel(s), so using " << chcount << " instead." << std::endl;
	}

	unsigned int actual_rate = rate;
	int subunit_direction;
	snd_pcm_hw_params_set_rate_near(handle, params, &actual_rate, &subunit_direction);

	snd_pcm_uframes_t ps = AUDIOFRAME_SIZE / channels;
	snd_pcm_hw_params_set_period_size_near(handle, params, &ps, &subunit_direction);

	if ((err = snd_pcm_hw_params(handle, params)) < 0) return err;

	return 0;
}

int AudioNodeALSAOutput::set_input(int id, AudioFrame* buf) {
	if (id != 0) {
		return E_INVALID_AUDIONODE_INPUT;
	}

	input = *buf;

	return 0;
}

int AudioNodeALSAOutput::get_output(int, AudioFrame**) {
	return E_INVALID_AUDIONODE_OUTPUT;
}
