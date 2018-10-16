#ifndef _SOCKETCONTROLLER_H
#define _SOCKETCONTROLLER_H

#include <string>
#include <fcntl.h>
#include "audionode.h"
#include "nodelink.h"

struct UnknownConnection {
	int fd;
	int read;
	char data[4];

	UnknownConnection(int fd) : fd(fd), read(0) {}
};

class ControlConnection {
private:
	std::string buf;
	
	void get_data(bool blocking);
public:
	int fd;

	ControlConnection(int fd) : fd(fd) {
		fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
	}
	
	int read_char(bool blocking = true);
	std::string read_line();

	void success();
	void error(std::string msg);
};

class SocketController {
private:
	int server_fd;
	bool opened;

	std::vector<UnknownConnection> unknown_connections;
	std::vector<ControlConnection> control_connections;

	std::vector<AudioNode*>* nodes;
	std::vector<NodeLink>* links;

public:
	std::string path;
	
	SocketController(std::string path, std::vector<AudioNode*>* nodes, std::vector<NodeLink>* links) : opened(false), nodes(nodes), links(links), path(path) {}
	~SocketController();

	int open();
	void tick();
};

#endif
