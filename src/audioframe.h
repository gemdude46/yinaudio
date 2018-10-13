#ifndef _AUDIOFRAME_H
#define _AUDIOFRAME_H

#include <stdint.h>

#define AUDIOFRAME_SIZE 1024

class AudioFrame {
public:
	int channels;
	int32_t data[AUDIOFRAME_SIZE];

	AudioFrame() : channels(0) {}
	AudioFrame(int c) : channels(c) {}
};

/*
inline int get_bytewidth(int format) {
	if (format == F_S8  || format == F_U8)  return 1;
	if (format == F_S16 || format == F_U16) return 2;
	if (format == F_S32 || format == F_U32) return 4;

	std::cerr << "Not a valid frame format: " << format << std::endl;
	return 0;
}

inline int get_bitwidth(int format) {
	if (format == F_S8  || format == F_U8)  return 8;
	if (format == F_S16 || format == F_U16) return 16;
	if (format == F_S32 || format == F_U32) return 32;

	std::cerr << "Not a valid frame format: " << format << std::endl;
	return 0;
}
*/

#endif
