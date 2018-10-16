#include "audionode.h"

#include <vector>
#include <string>
#include <stdint.h>
#include <math.h>
#include "config.h"
#include "audioframe.h"

#define TAU 6.28318530718

void AudioNodeSine::tick() {
	output.channels = 1;

	for (int i = 0; i < AUDIOFRAME_SIZE; i++) {
		output.data[i] = static_cast<int32_t>(sin(x) * amp * 2000000000);
		x += TAU * freq / GLOBAL_RATE;
		while (x > TAU) x -= TAU;
	}
}

void AudioNodeSine::serialize(std::ostream &dest) {
	dest << "[Sine Wave]\nfrequency=" << freq << "\namplitude=" << amp << '\n';
}

int AudioNodeSine::get_output(int id, AudioFrame** buf) {
	if (id != 0) {
		return E_INVALID_AUDIONODE_OUTPUT;
	}

	*buf = &output;

	return 0;
}

int AudioNodeSine::update_attribute(std::string key, std::string value) {
	if (key == "frequency") {
		try {
			freq = stod(value);
			return 0;
		} catch (const std::invalid_argument &err) {
			return E_INVALID_AUDIONODE_ATTRIBUTE_VALUE;
		}
	} else if (key == "amplitude") {
		try {
			double na = stod(value);
			if (na >= 0 && na <= 1) {
				amp = na;
				return 0;
			} else {
				return E_INVALID_AUDIONODE_ATTRIBUTE_VALUE;
			}
		} catch (const std::invalid_argument &err) {
			return E_INVALID_AUDIONODE_ATTRIBUTE_VALUE;
		}
	} else {
		return E_INVALID_AUDIONODE_ATTRIBUTE_KEY;
	}
}
