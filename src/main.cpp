#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <iterator>
#include <chrono>
#include <thread>
#include "audionode.h"
#include "audioframe.h"
#include "nodelink.h"
#include "parser.h"
#include "socketcontroller.h"
#include "config.h"

int main(int argc, char** argv) {

	bool allow_flags = true;

	std::vector<std::string> files;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		
		if (arg[0] == '-' && allow_flags) {
			if (arg == "--") {
				allow_flags = false;
				continue;
			}

			else if (arg == "-") {
				files.push_back("/dev/stdin");
				continue;
			}

			else if (arg == "-h" || arg == "--help") {
				if (i == 1 && argc == 2) {
					std::cout << "Usage: yinaudio [OPTIONS]... [FILE]..." << std::endl
					          << std::endl
					          << "YinAudio is an audio framework for linux designed to allow the easy creation of" << std::endl
					          << "advanced audio control systems. It has features such as filters and limiters," << std::endl
					          << "and can be configured via a simple and intuative GUI or TUI." << std::endl
					          << std::endl
					          << "When FILE is -, it reads standard input." << std::endl
					          << std::endl
					          << "  -h, --help          display this help and exit" << std::endl
					          << "  -v, --version       display version and license information and exit" << std::endl;
					
					return 0;
				} else {
					std::cerr << arg << " cannot be used with other flags." << std::endl;
					return -1;
				}
			}

			else if (arg == "-v" || arg == "--version") {
				if (i == 1 && argc == 2) {
					std::cout << "YinAudio version 1.0.0" << std::endl
					          << "Copyright (C) 2018 Marley Adair." << std::endl
					          << "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>." << std::endl
					          << "This is free software: you are free to change and redistribute it." << std::endl
					          << "There is NO WARRANTY, to the extent permitted by law." << std::endl;

					return 0;
				} else {
					std::cerr << arg << " cannot be used with other flags." << std::endl;
					return -1;
				}
			}

			else {
				std::cerr << "Unrecognized option '" << arg << "'" << std::endl;
				return -1;
			}
		} else {
			files.push_back(arg);
		}
	}

	if (files.empty()) {
		std::cerr << "Warning: no input files" << std::endl;
	}

	std::vector<AudioNode*> nodes;
	std::vector<NodeLink> links;

	for (auto const &file: files) {
		std::string code;
		if (file == "/dev/stdin") {
			std::istreambuf_iterator<char> begin(std::cin), end;
			code = std::string(begin, end);
		} else {
			std::ifstream fs;
			fs.open(file);
			if (!fs.good()) {
				std::cerr << "Error: failed to open " << file << std::endl;
				return -1;
			}
			std::stringstream buf;
			buf << fs.rdbuf();
			fs.close();
			code = buf.str();
		}

		int err;
		if ((err = parse_code(code, nodes, links))) {
			return err;
		}
	}

	SocketController sctrl("./yinaudio-sock", &nodes, &links);

	int err;
	if ((err = sctrl.open())) {
		std::cerr << "Unable to open socket at " << sctrl.path << ": Errno " << err << std::endl;
		return -1;
	}

	std::chrono::time_point<std::chrono::system_clock> tp;

	int loopc = 0;

	while (true) {
		if (loopc++ % 256 == 0) {
			tp = std::chrono::system_clock::now();
		}

		for (auto const &link: links) {
			AudioFrame* buf;
			if ((err = link.src->get_output(link.src_id, &buf))) {
				std::cerr << "Unable to get output " << link.src_id << " of " << (link.src->name.empty() ? "UNNAMED" : link.src->name.c_str()) << ": Errno: " << err << std::endl;
				return -1;
			}
			if (link.dest->set_input(link.dest_id, buf)) {
				std::cerr << "Unable to set input " << link.dest_id << " of " << (link.dest->name.empty() ? "UNNAMED" : link.dest->name.c_str()) << std::endl;
				return -1;
			}
		}

		for (auto const &node: nodes) {
			node->tick();
		}

		sctrl.tick();

		tp += std::chrono::milliseconds(1000 * AUDIOFRAME_SIZE / GLOBAL_RATE);
		std::this_thread::sleep_until(tp);
	}

	return 0;
}
