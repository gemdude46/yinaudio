#include "audionode.h"

#define ALSA_PCM_NEW_HW_PARAMS_API

#include <iostream>
#include <alsa/asoundlib.h>
#include "audioframe.h"
#include "config.h"

void AudioNodeALSAInput::tick() {

}

int AudioNodeALSAInput::add_output(AudioFrameBuffer* buf, int output_id) {
	if (output_id != 0) {
		return E_INVALID_AUDIONODE_OUTPUT;
	}

	outputs.push_back(buf);

	return 0;
}
