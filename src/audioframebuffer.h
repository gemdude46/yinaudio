#ifndef _AUDIOFRAMEBUFFER_H
#define _AUDIOFRAMEBUFFER_H

#include "audioframe.h"

#define AUDIOFRAMEBUFFER_SIZE 8

#define E_AUDIOFRAMEBUFFER_FULL 1
#define E_AUDIOFRAMEBUFFER_EMPTY 2

class AudioFrameBuffer {
private:
	AudioFrame buf[AUDIOFRAMEBUFFER_SIZE];
	int readptr, writeptr;

public:
	AudioFrameBuffer() {
		readptr = 0;
		writeptr = 0;
	}

	bool poll() {
		return readptr != writeptr;
	}

	int read_frame(AudioFrame* frame);
	int write_frame(AudioFrame &frame);
};

#endif
