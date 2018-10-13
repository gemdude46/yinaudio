#include "audionode.h"

#include "audioframe.h"

void AudioNodeStereoMerge::tick() {
	output.channels = 2;

	if (left.channels != 1 || right.channels != 1) {
		std::cerr << "Mono to Stereo input must be mono" << std::endl;
		output.zero();
		return;
	}

	for (int i = 0; i < AUDIOFRAME_SIZE; i++) {
		output.data[2 * i] = left.data[i];
		output.data[2 * i + 1] = right.data[i];
	}
}

int AudioNodeStereoMerge::set_input(int id, AudioFrame* buf) {
	if (id == 0) {
		left = *buf;
	} else if (id == 1) {
		right = *buf;
	} else {
		return E_INVALID_AUDIONODE_INPUT;
	}

	return 0;
}

int AudioNodeStereoMerge::get_output(int id, AudioFrame** buf) {
	if (id != 0) {
		return E_INVALID_AUDIONODE_OUTPUT;
	}

	*buf = &output;

	return 0;
}
