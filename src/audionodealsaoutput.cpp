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

/*
inline snd_pcm_format_t get_alsa_format(int format) {
	if (format == F_S8)  return SND_PCM_FORMAT_S8;
	if (format == F_U8)  return SND_PCM_FORMAT_U8;
	if (format == F_S16) return ENDIANIFY(SND_PCM_FORMAT_S16);
	if (format == F_U16) return ENDIANIFY(SND_PCM_FORMAT_U16);
	if (format == F_S32) return ENDIANIFY(SND_PCM_FORMAT_S32);
	if (format == F_U32) return ENDIANIFY(SND_PCM_FORMAT_U32);
	
	std::cerr << "Not a valid frame format: " << format << std::endl;
	return SND_PCM_FORMAT_UNKNOWN;
}
*/

void AudioNodeALSAOutput::tick() {
	if (!enabled) return;

	int err;
	AudioFrame frame;

	while (0 == (err = fbuf.read_frame(&frame))) {
		if (!handle) {
			if ((err = open_device(GLOBAL_RATE, frame.channels))) {
				std::cerr << "Unable to open audio device " << device << ": Error " << err << ": " << snd_strerror(err) << std::endl;
				enabled = false;
				return;
			}

			if (!handle) {
				std::cerr << "Unable to open audio device " << device << ": Unknown error. Retrying." << std::endl;
				return;
			}
		}

		if (output_bitwidth != 32 || output_channels > frame.channels) {
			if (output_channels > frame.channels) {
				for (int i = 0; i < AUDIOFRAME_SIZE; i++) {
					for (int j = 0; j < output_channels / frame.channels; j++) {
						rescale[i * output_channels / frame.channels + j] = frame.data[i];
					}
				}

				if (output_bitwidth == 16) {
					for (int i = 0; i < 6 * AUDIOFRAME_SIZE; i++) {
						((int16_t*) rescale)[i] = static_cast<int16_t>(rescale[i] >> 16);
					}
				}

				else if (output_bitwidth == 8) {
					for (int i = 0; i < 6 * AUDIOFRAME_SIZE; i++) {
						((int8_t*) rescale)[i] = static_cast<int8_t>(rescale[i] >> 24);
					}
				}

				err = -snd_pcm_writei(handle, rescale, AUDIOFRAME_SIZE / frame.channels);
			} else {
				if (output_bitwidth == 16) {
					for (int i = 0; i < AUDIOFRAME_SIZE; i++) {
						((int16_t*) frame.data)[i] = static_cast<int16_t>(frame.data[i] >> 16);
					}
				}

				else if (output_bitwidth == 8) {
					for (int i = 0; i < AUDIOFRAME_SIZE; i++) {
						((int8_t*) frame.data)[i] = static_cast<int8_t>(frame.data[i] >> 24);
					}
				}
				
				err = -snd_pcm_writei(handle, frame.data, AUDIOFRAME_SIZE / frame.channels);
			}
		} else {
			err = -snd_pcm_writei(handle, frame.data, AUDIOFRAME_SIZE / frame.channels);
		}
		
		if (err == EPIPE) {
			std::cerr << "Underrun occurred on device " << device << std::endl;
		} else if (err > 0) {
			std::cerr << "Error writing frames to device " << device << ": Error " << err << ": " << snd_strerror(-err) << std::endl;
		}
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

int AudioNodeALSAOutput::add_output(AudioFrameBuffer* /*buf*/, int /*output_id*/) {
	return E_INVALID_AUDIONODE_OUTPUT;
}
