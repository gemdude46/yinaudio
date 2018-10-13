#ifndef _AUDIONODE_H
#define _AUDIONODE_H

#define ALSA_PCM_NEW_HW_PARAMS_API

#include <stdint.h>
#include <vector>
#include <string>
#include <iostream>
#include <alsa/asoundlib.h>
#include "audioframe.h"

#define E_INVALID_AUDIONODE_INPUT  1
#define E_INVALID_AUDIONODE_OUTPUT 2

class AudioNode {
public:
	std::string name;

	virtual void tick() = 0;

	virtual int set_input(int id, AudioFrame* buf) = 0;
	virtual int get_output(int id, AudioFrame** buf) = 0;
};


class AudioNodeStereoMerge : public AudioNode {
private:
	AudioFrame left, right, output;

public:
	AudioNodeStereoMerge() {}

	void tick();

	int set_input(int id, AudioFrame* buf);
	int get_output(int id, AudioFrame** buf);
};

class AudioNodeSine : public AudioNode {
private:
	AudioFrame output;
	double x;

public:
	double freq;
	double amp;

	AudioNodeSine(double f, double a) : freq(f), amp(a) {
		x = 0;
	}

	void tick();

	int set_input(int id, AudioFrame* buf);
	int get_output(int id, AudioFrame** buf);
};

class AudioNodeALSAOutput : public AudioNode {
private:
	bool enabled;

	AudioFrame input;

	std::string device;

	snd_pcm_t* handle;
	snd_pcm_hw_params_t* params;

	int output_channels;
	int output_bitwidth;

	int open_device(int rate, int channels);

public:
	AudioNodeALSAOutput(std::string d) : device(d) {
		enabled = true;
		handle = NULL;
	}

	void tick();

	int set_input(int id, AudioFrame* buf);
	int get_output(int id, AudioFrame** buf);
};

#endif
