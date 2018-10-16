#include "audionode.h"

#include <string>

int AudioNode::set_input(int, AudioFrame*) {
	return E_INVALID_AUDIONODE_INPUT;
}

int AudioNode::get_output(int, AudioFrame**) {
	return E_INVALID_AUDIONODE_OUTPUT;
}

int AudioNode::update_attribute(std::string, std::string) {
	return E_INVALID_AUDIONODE_ATTRIBUTE_KEY;
}
