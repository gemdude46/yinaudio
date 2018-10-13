#ifndef _AUDIONODE_H
#define _AUDIONODE_H

#define ALSA_PCM_NEW_HW_PARAMS_API

#include <stdint.h>
#include <vector>
#include <string>
#include <iostream>
#include <alsa/asoundlib.h>

#include "audioframebuffer.h"

#define E_INVALID_AUDIONODE_OUTPUT 1

class AudioNode {
public:
	std::string name;

	virtual void tick() = 0;
	virtual int add_output(AudioFrameBuffer* buf, int output_id) = 0;
};


class AudioNodeStereoMerge : public AudioNode {
private:
	AudioFrameBuffer left, right;
	std::vector<AudioFrameBuffer*> outputs;

public:
	AudioNodeStereoMerge() {}

	AudioFrameBuffer* get_input_left() {
		return &left;
	}

	AudioFrameBuffer* get_input_right() {
		return &right;
	}

	void tick();
	int add_output(AudioFrameBuffer* buf, int output_id);
};

class AudioNodeSine : public AudioNode {
private:
	std::vector<AudioFrameBuffer*> outputs;
	double x;

public:
	double freq;
	double amp;

	AudioNodeSine(double f, double a) : freq(f), amp(a) {
		x = 0;
	}

	void tick();
	int add_output(AudioFrameBuffer* buf, int output_id);
};

class AudioNodeALSAOutput : public AudioNode {
private:
	bool enabled;

	std::string device;
	AudioFrameBuffer fbuf;

	snd_pcm_t* handle;
	snd_pcm_hw_params_t* params;

	int output_channels;
	int output_bitwidth;
	int32_t rescale[6 * AUDIOFRAME_SIZE];

	int open_device(int rate, int channels);

public:
	AudioNodeALSAOutput(std::string d) : device(d) {
		enabled = true;
		handle = NULL;
	}

	AudioFrameBuffer* get_input() {
		return &fbuf;
	}
	
	void tick();
	int add_output(AudioFrameBuffer* buf, int output_id);
};

#endif
