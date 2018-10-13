#include "audionode.h"

#include "audioframe.h"
#include "audioframebuffer.h"

void AudioNodeStereoMerge::tick() {
	while (left.poll() && right.poll()) {
		AudioFrame lf, rf, o1, o2;
		left.read_frame(&lf);
		right.read_frame(&rf);

		if (lf.channels != 1 || rf.channels != 1) {
			std::cerr << "Mono to Stereo input must be mono" << std::endl;
			continue;
		}

		o1.channels = o2.channels = 2;

		for (int i = 0; i < AUDIOFRAME_SIZE / 2; i++) {
			o1.data[2 * i] = lf.data[i];
			o1.data[2 * i + 1] = rf.data[i];
			o2.data[2 * i] = lf.data[i + AUDIOFRAME_SIZE / 2];
			o2.data[2 * i + 1] = rf.data[i + AUDIOFRAME_SIZE / 2];
		}
	
		for (auto const &output: outputs) {
			output->write_frame(o1);
			output->write_frame(o2);
		}
	}
}

int AudioNodeStereoMerge::add_output(AudioFrameBuffer* buf, int output_id) {
	if (output_id != 0) {
		return E_INVALID_AUDIONODE_OUTPUT;
	}

	outputs.push_back(buf);

	return 0;
}
