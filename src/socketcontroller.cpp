#include "socketcontroller.h"

#include <string.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include "audionode.h"
#include "parser.h"
#include "nodeserializer.h"

void ControlConnection::get_data(bool blocking) {
	char data[256];
	int rc = recv(fd, data, 256, blocking ? 0 : MSG_DONTWAIT);

	if (rc > 0) {
		buf += std::string(data, rc);
	} else if (blocking || rc >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
		throw rc;
	}
}

int ControlConnection::read_char(bool blocking) {
	if (buf.empty())
		get_data(blocking);
	
	if (!blocking && buf.empty())
		return -1;

	char c = buf[0];

	buf = buf.substr(1);

	return c;
}

std::string ControlConnection::read_line() {
	std::size_t nl;

	while ((nl = buf.find_first_of('\n')) == std::string::npos)
		get_data(true);
	
	std::string l = buf.substr(0, nl);

	buf = buf.substr(nl + 1);

	return l;
}

void ControlConnection::success() {
	send(fd, "S", 1, 0);
}

void ControlConnection::error(std::string msg) {
	send(fd, "E", 1, 0);
	send(fd, msg.c_str(), msg.size(), 0);
	send(fd, "\n", 1, 0);
}

int SocketController::open() {
    struct sockaddr_un server_addr;

	memset(&server_addr, 0, sizeof server_addr);

	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, path.c_str(), (sizeof server_addr.sun_path) - 1);

	if ((server_fd = socket(PF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) <= 0) {
		return errno;
	}

	if (bind(server_fd, (struct sockaddr*) (&server_addr), sizeof server_addr) < 0) {
		return errno;
	}

	opened = true;

	if (listen(server_fd, SOMAXCONN) < 0) {
		return errno;
	}

	return 0;
}

void SocketController::tick() {
	int new_conn;

	while ((new_conn = accept(server_fd, NULL, NULL)) > 0) {
		fcntl(new_conn, F_SETFL, fcntl(new_conn, F_GETFL) | O_NONBLOCK);
		unknown_connections.emplace_back(new_conn);
	}

	for (int i = 0; i < (int) unknown_connections.size(); i++) {
		UnknownConnection conn = unknown_connections[i];
		int rc = recv(conn.fd, conn.data + conn.read, 4 - conn.read, 0);
		if (rc < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				close(conn.fd);
				unknown_connections.erase(unknown_connections.begin() + i);
				i--;
			}
		} else if (rc == 0) {
			shutdown(conn.fd, SHUT_RDWR);
			close(conn.fd);
			unknown_connections.erase(unknown_connections.begin() + i);
			i--;
		} else {
			conn.read += rc;
			if (conn.read == 4) {
				if (!memcmp(conn.data, "\xe9\x9f\xb3\x43", 4)) {
					control_connections.emplace_back(conn.fd);
				} else {
					shutdown(conn.fd, SHUT_RDWR);
					close(conn.fd);
				}

				unknown_connections.erase(unknown_connections.begin() + i);
				i--;
			}
		}
	}

	for (int i = 0; i < (int) control_connections.size(); i++) {
		ControlConnection conn = control_connections[i];

		try {
			int cmd;
			
			while ((cmd = conn.read_char(false)) > 0) {
				if (cmd == 'q') {
					conn.success();
					throw 0;
				}
				
				else if (cmd == 'x') {
					std::string lc_str = conn.read_line();
					unsigned long lc;
					try {
						lc = stoul(lc_str);
					} catch (const std::invalid_argument &err) {
						conn.error("INVALID LINE COUNT");
						continue;
					} catch (const std::out_of_range &err) {
						conn.error("INVALID LINE COUNT");
						continue;
					}
					
					std::string code;

					for (unsigned long i = 0; i < lc; i++) {
						code += conn.read_line() + '\n';
					}

					if (parse_code(code, *nodes, *links)) {
						conn.error("INVALID CODE");
					} else {
						conn.success();
					}
				}

				else if (cmd == 'w') {
					std::string dest = conn.read_line();
					try {
						std::ofstream df;
						df.open(dest, std::ios::out | std::ios::trunc);
						serialize(df, *nodes, *links);
						df.close();
						conn.success();
					} catch (const std::ofstream::failure &err) {
						conn.error("IO ERROR");
					}
				}
				
				else if (cmd == 'k') {
					conn.success();
					this->~SocketController();
					exit(0);
				}

				else {
					conn.error("UNKNOWN COMMAND");
				}
			}
		} catch (int err) {
			shutdown(conn.fd, SHUT_RDWR);
			close(conn.fd);
			control_connections.erase(control_connections.begin() + i);
			i--;

			if (err < 0) {
				std::cerr << "Errno " << errno << " while reading control socket." << std::endl;
			}
		}
	}
}

SocketController::~SocketController() {
	if (opened) {
		for (auto const &conn: unknown_connections) {
			shutdown(conn.fd, SHUT_RDWR);
			close(conn.fd);
		}
		
		for (auto const &conn: control_connections) {
			shutdown(conn.fd, SHUT_RDWR);
			close(conn.fd);
		}

		shutdown(server_fd, SHUT_RDWR);
		close(server_fd);
		unlink(path.c_str());
	}
}
