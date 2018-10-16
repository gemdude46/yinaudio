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

void AudioNodeALSAInput::tick() {
	if (!enabled) return;

	int err;
	
	if (!handle) {
		if ((err = open_device(GLOBAL_RATE))) {
			std::cerr << "Unable to open audio device " << device << ": Error " << err << ": " << snd_strerror(err) << std::endl;
			enabled = false;
			output.channels = 1;
			output.zero();
			return;
		}
		
		if (!handle) {
			std::cerr << "Unable to open audio device " << device << ": Unknown error. Retrying." << std::endl;
			return;
		}
	}

	if (input_bitwidth == 32) {
		err = -snd_pcm_readi(handle, output.data, AUDIOFRAME_SIZE);
	} else {
		char buf[2 * AUDIOFRAME_SIZE * MAX_CHANNELS];
		err = -snd_pcm_readi(handle, buf, AUDIOFRAME_SIZE);

		if (input_bitwidth == 16) {
			for (int i = 0; i < AUDIOFRAME_SIZE * input_channels; i++) {
				output.data[i] = ((int16_t*) buf)[i] << 16;
			}
		} else if (input_bitwidth == 8) {
			for (int i = 0; i < AUDIOFRAME_SIZE * input_channels; i++) {
				output.data[i] = ((int8_t*) buf)[i] << 24;
			}
		}
	}

	if (err == EPIPE) {
		std::cerr << "Overrun occurred on device " << device << std::endl;
	} else if (err > 0) {
		std::cerr << "Error reading frames from device " << device << ": Error " << err << ": " << snd_strerror(-err) << std::endl;
	}

	output.channels = input_channels;
}

int AudioNodeALSAInput::open_device(int rate) {
	int err;
	
	if ((err = snd_pcm_open(&handle, device.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) return err;

	snd_pcm_hw_params_alloca(&params);
	if ((err = snd_pcm_hw_params_any(handle, params)) < 0) return err;
	if ((err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) return err;
	
	input_bitwidth = 32;
	if (snd_pcm_hw_params_set_format(handle, params, ALSA_FMT) < 0) {
		input_bitwidth = 16;
		if (snd_pcm_hw_params_set_format(handle, params, ALSA_FMT_FALLBACK) < 0) {
			input_bitwidth = 8;
			if ((err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S8)) < 0) return err;
		}
	}

	for (int i = MAX_CHANNELS;; i--) {
		if ((err = snd_pcm_hw_params_set_channels(handle, params, i)) >= 0) {
			input_channels = i;
			break;
		}

		if (i == 1) return err;
	}

	unsigned int actual_rate = rate;
	int subunit_direction;
	snd_pcm_hw_params_set_rate_near(handle, params, &actual_rate, &subunit_direction);
	
	snd_pcm_uframes_t ps = AUDIOFRAME_SIZE / input_channels;
	snd_pcm_hw_params_set_period_size_near(handle, params, &ps, &subunit_direction);
	snd_pcm_hw_params_set_buffer_size(handle, params, AUDIOFRAME_SIZE * 2);

	if ((err = snd_pcm_hw_params(handle, params)) < 0) return err;

	return 0;
}

void AudioNodeALSAInput::serialize(std::ostream &dest) {
	dest << "[ALSA Input Device]\ndevice=" << device << '\n';
}

int AudioNodeALSAInput::get_output(int id, AudioFrame** buf) {
	if (id != 0) {
		return E_INVALID_AUDIONODE_OUTPUT;
	}

	*buf = &output;

	return 0;
}
