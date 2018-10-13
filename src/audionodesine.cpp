#include "audionode.h"

#include <vector>
#include <stdint.h>
#include <math.h>
#include "config.h"
#include "audioframe.h"
#include "audioframebuffer.h"

#define TAU 6.28318530718

void AudioNodeSine::tick() {
	AudioFrame frame(1);

	for (int i = 0; i < AUDIOFRAME_SIZE; i++) {
		frame.data[i] = static_cast<int32_t>(sin(x) * amp * 2000000000);
		x += TAU * freq / GLOBAL_RATE;
		while (x > TAU) x -= TAU;
	}

	for (auto const &output: outputs) {
		output->write_frame(frame);
	}
}

int AudioNodeSine::add_output(AudioFrameBuffer* buf, int output_id) {
	if (output_id != 0) {
		return E_INVALID_AUDIONODE_OUTPUT;
	}

	outputs.push_back(buf);

	return 0;
}
