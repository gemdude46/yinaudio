#include "audioframe.h"

#include <stdint.h>
#include <string.h>

void AudioFrame::zero() {
	memset(data, 0, 4 * channels * AUDIOFRAME_SIZE);
}

void AudioFrame::increase_channels(int new_channel_count) {
	int32_t new_data[MAX_CHANNELS * AUDIOFRAME_SIZE];
	for (int i = 0; i < AUDIOFRAME_SIZE; i++) {
		for (int j = 0; j < new_channel_count; j++) {
			new_data[i * new_channel_count + j] = data[i * channels + j % channels];
		}
	}

	memcpy(data, new_data, 4 * AUDIOFRAME_SIZE * new_channel_count);

	channels = new_channel_count;
}
