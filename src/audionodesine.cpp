#include "audionode.h"

#include <vector>
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

int AudioNodeSine::set_input(int, AudioFrame*) {
	return E_INVALID_AUDIONODE_INPUT;
}

int AudioNodeSine::get_output(int id, AudioFrame** buf) {
	if (id != 0) {
		return E_INVALID_AUDIONODE_OUTPUT;
	}

	*buf = &output;

	return 0;
}
