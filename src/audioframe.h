#ifndef _AUDIOFRAME_H
#define _AUDIOFRAME_H

#include <stdint.h>

#define AUDIOFRAME_SIZE 256
#define MAX_CHANNELS 6

class AudioFrame {
public:
	int channels;
	int32_t data[MAX_CHANNELS * AUDIOFRAME_SIZE];

	AudioFrame() : channels(1) {}
	AudioFrame(int c) : channels(c) {}

	void zero();
	void increase_channels(int new_channel_count);
};

#endif
