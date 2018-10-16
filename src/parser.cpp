#include "parser.h"

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include "audionode.h"
#include "nodelink.h"

std::string trim(std::string x, std::string ws = " \t\r\n\v") {
	std::size_t fc = x.find_first_not_of(ws);

	if (fc == std::string::npos) return "";

	std::size_t len = x.find_last_not_of(ws) - fc + 1;

	return x.substr(fc, len);
}

class PreNodeLink {
public:
	std::string dest;
	int dest_id;

	std::string src;
	int src_id;

	PreNodeLink() {}
	PreNodeLink(std::string d, int did, std::string s, int sid) : dest(d), dest_id(did), src(s), src_id(sid) {}
	PreNodeLink(std::string so, std::string di) {
		std::size_t slash = di.find_first_of('/');

		if (slash == std::string::npos) throw std::invalid_argument("No /");

		dest = di.substr(0, slash);
		dest_id = stoi(di.substr(slash + 1));

		slash = so.find_first_of('/');

		if (slash == std::string::npos) throw std::invalid_argument("No /");

		src = so.substr(0, slash);
		src_id = stoi(so.substr(slash + 1));
	}
};

int add_node(std::string &current_type, std::unordered_map<std::string,std::string> &current_data, std::vector<AudioNode*> &nodes) {
	AudioNode* new_node;
	
	if (current_type == "[2 Monos to Stereo]") {
		new_node = new AudioNodeStereoMerge();
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
	
	else if (current_type == "[ALSA Input Device]") {
		std::string did;
		
		try {
			did = current_data.at("device");
		} catch (const std::out_of_range &err) {
			std::cerr << "[ALSA Input Device] node missing required property \"device\"" << std::endl;
			return -1;
		}

		new_node = new AudioNodeALSAInput(did);
	}

	else if (current_type == "[ALSA Output Device]") {
		std::string did;
		
		try {
			did = current_data.at("device");
		} catch (const std::out_of_range &err) {
			std::cerr << "[ALSA Output Device] node missing required property \"device\"" << std::endl;
			return -1;
		}
		
		new_node = new AudioNodeALSAOutput(did);
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

int parse_code(std::string code, std::vector<AudioNode*> &nodes, std::vector<NodeLink> &links) {
	int err;
	std::vector<PreNodeLink> prelinks;

	std::string current_type;
	std::unordered_map<std::string,std::string> current_data;

	std::size_t strind = 0;
	while (strind != std::string::npos) {
		std::size_t next = code.find_first_of('\n', strind);
		std::string line = trim(code.substr(strind, next == std::string::npos ? next : next - strind));

		if (!line.empty()) {
			std::size_t equals;

			if (line[0] == '[') {
				if (!current_type.empty()) {
					if ((err = add_node(current_type, current_data, nodes))) return err;
				}

				current_type = line;
				current_data = std::unordered_map<std::string,std::string>();
			} else if ((equals = line.find_first_of('=')) != std::string::npos) {
				if (line[equals + 1] == '>') {
					prelinks.emplace_back(trim(line.substr(0, equals)), trim(line.substr(equals + 2)));
				} else {
					current_data[trim(line.substr(0, equals))] = trim(line.substr(equals + 1));
				}
			} else {
				std::cerr << '"' << line << "\" is not a valid section header or property assignment";
				return -1;
			}

		}

		strind = next == std::string::npos ? next : next + 1;
	}
	
	if (!current_type.empty() && (err = add_node(current_type, current_data, nodes))) return err;

	
	for (auto const &prelink: prelinks) {
		AudioNode* src  = NULL;
		AudioNode* dest = NULL;

		for (auto const &node: nodes) {
			if (node->name == prelink.src)
				src = node;

			if (node->name == prelink.dest)
				dest = node;

			if (src && dest)
				break;
		}

		if (!src) {
			std::cerr << "Name " << prelink.src << " is not defined" << std::endl;
			return -1;
		}

		if (!dest) {
			std::cerr << "Name " << prelink.dest << " is not defined" << std::endl;
			return -1;
		}

		NodeLink link;
		link.src = src;
		link.src_id = prelink.src_id;
		link.dest = dest;
		link.dest_id = prelink.dest_id;
		links.push_back(link);
	}

	return 0;
}
