#include "audioframebuffer.h"

#include "audioframe.h"

int AudioFrameBuffer::read_frame(AudioFrame* frame) {
	if (readptr == writeptr) {
		return E_AUDIOFRAMEBUFFER_EMPTY;
	}

	*frame = buf[readptr];

	readptr = (readptr + 1) % AUDIOFRAMEBUFFER_SIZE;

	return 0;
}

int AudioFrameBuffer::write_frame(AudioFrame &frame) {
	if ((writeptr + 1) % AUDIOFRAMEBUFFER_SIZE == readptr) {
		return E_AUDIOFRAMEBUFFER_FULL;
	}

	buf[writeptr] = frame;

	writeptr = (writeptr + 1) % AUDIOFRAMEBUFFER_SIZE;

	return 0;
}
