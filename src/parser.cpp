#include "parser.h"

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include "audionode.h"
#include "audioframebuffer.h"

class NodeLink {
public:
	AudioFrameBuffer* buf;
	std::string src;
	int output;

	NodeLink() {}
	NodeLink(AudioFrameBuffer* b, std::string s, int o) : buf(b), src(s), output(o) {}
	NodeLink(AudioFrameBuffer* b, std::string so) : buf(b) {
		std::size_t slash = so.find_first_of('/');

		if (slash == std::string::npos) throw std::invalid_argument("No /");

		src = so.substr(0, slash);
		output = stoi(so.substr(slash + 1));
	}
};

int add_node(std::string &current_type, std::unordered_map<std::string,std::string> &current_data, std::vector<NodeLink> &links, std::vector<AudioNode*> &nodes) {
	AudioNode* new_node;
	
	if (current_type == "[2 Monos to Stereo]") {
		AudioNodeStereoMerge* node = new AudioNodeStereoMerge();

		try {
			links.emplace_back(node->get_input_left(), current_data.at("left_source"));
		} catch (const std::out_of_range &err) {
			std::cerr << "[2 Monos to Stereo] has no property \"left_source\", and so is not driven" << std::endl;
		} catch (const std::invalid_argument &err) {
			std::cerr << '"' << current_data["left_source"] << "\" is not a valid source format" << std::endl;
			return -1;
		}
		
		try {
			links.emplace_back(node->get_input_right(), current_data.at("right_source"));
		} catch (const std::out_of_range &err) {
			std::cerr << "[2 Monos to Stereo] has no property \"right_source\", and so is not driven" << std::endl;
		} catch (const std::invalid_argument &err) {
			std::cerr << '"' << current_data["right_source"] << "\" is not a valid source format" << std::endl;
			return -1;
		}

		new_node = node;
	}
	
	else if (current_type == "[Sine Wave]") {
		double f, a;
		
		try {
			f = stod(current_data.at("frequency"));
		} catch (const std::out_of_range &err) {
			std::cerr << "[Sine Wave] node missing required property \"frequency\"" << std::endl;
			return -1;
		} catch (const std::invalid_argument &err) {
			std::cerr << "[Sine Wave] property \"frequency\" must be valid floating point number" << std::endl;
			return -1;
		}

		try {
			a = stod(current_data.at("amplitude"));
		} catch (const std::out_of_range &err) {
			a = 1;
		} catch (const std::invalid_argument &err) {
			std::cerr << "[Sine Wave] property \"amplitude\" must be valid floating point number" << std::endl;
			return -1;
		}

		if (a < 0 || a > 1) {
			std::cerr << "[Sine Wave] property \"amplitude\" must be between 0 and 1" << std::endl;
			return -1;
		}

		new_node = new AudioNodeSine(f, a);
	}
	
	else if (current_type == "[ALSA Output Device]") {
		std::string did;
		
		try {
			did = current_data.at("device");
		} catch (const std::out_of_range &err) {
			std::cerr << "[ALSA Output Device] node missing required property \"device\"" << std::endl;
			return -1;
		}

		AudioNodeALSAOutput* node = new AudioNodeALSAOutput(did);

		try {
			links.emplace_back(node->get_input(), current_data.at("source"));
		} catch (const std::out_of_range &err) {
			std::cerr << "[ALSA Output Device] has no property \"source\", and so is not driven" << std::endl;
		} catch (const std::invalid_argument &err) {
			std::cerr << '"' << current_data["source"] << "\" is not a valid source format" << std::endl;
			return -1;
		}

		new_node = node;
	}
	
	else {
		std::cerr << current_type << " is not a known node type" << std::endl;
		return -1;
	}
	
	if (current_data.count("name")) {
		new_node->name = current_data["name"];
	}

	nodes.push_back(new_node);

	return 0;
}

int parse_code(std::string code, std::vector<AudioNode*> &nodes) {
	int err;
	std::vector<NodeLink> links;

	std::string current_type;
	std::unordered_map<std::string,std::string> current_data;

	std::size_t strind = 0;
	while (strind != std::string::npos) {
		std::size_t next = code.find_first_of('\n', strind);
		std::string line = code.substr(strind, next == std::string::npos ? next : next - strind);

		std::size_t last_char = line.find_last_not_of(" \t\r\n\v");

		if (last_char != std::string::npos) {
			line = line.substr(0, last_char + 1);

			std::size_t equals;

			if (line[0] == '[') {
				if (!current_type.empty()) {
					if ((err = add_node(current_type, current_data, links, nodes))) return err;
				}

				current_type = line;
				current_data = std::unordered_map<std::string,std::string>();
			} else if ((equals = line.find_first_of('=')) != std::string::npos) {
				current_data[line.substr(0, equals)] = line.substr(equals + 1);
			} else {
				std::cerr << '"' << line << "\" is not a valid section header or property assignment";
				return -1;
			}

		}

		strind = next == std::string::npos ? next : next + 1;
	}
	
	if (!current_type.empty() && (err = add_node(current_type, current_data, links, nodes))) return err;

	for (auto const &link: links) {
		bool found = false;
		for (auto const &node: nodes) {
			if (node->name == link.src) {
				found = true;
				if ((err = node->add_output(link.buf, link.output))) {
					std::cerr << "Invalid output id" << std::endl;
					return -1;
				}
				break;
			}
		}

		if (!found) {
			std::cerr << "Name " << link.src << " is not defined" << std::endl;
			return -1;
		}
	}

	return 0;
}
