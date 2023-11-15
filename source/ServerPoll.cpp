#include <fcntl.h>
#include <stdexcept>
#include <cstdlib>
#include <signal.h>
#include <netdb.h>

#include "../include/Server.hpp"

Server::Server(std::string port, std::string password) : opName(""), opPassword(""), op(NULL) {
	char* pointer;
	long strictingPort;
	char hostnameBuf[1024];
	struct hostent* hostStruct;

	strictingPort = std::strtol(port.c_str(), &pointer, 10);
	if (*pointer != 0 || strictingPort < 1024 || strictingPort > 65535)
		throw std::runtime_error("Error : port is wrong");
	for (size_t i = 0; i < password.size(); i++) {
		if (password[i] == 0 || password[i] == '\r'
			|| password[i] == '\n' || password[i] == ':')
			throw std::runtime_error("Error : password is wrong");
	}
	this->password = password;
	this->port = static_cast<int>(strictingPort);
	if (gethostname(hostnameBuf, sizeof(hostnameBuf)) == -1)
		this->host = "127.0.0.1";
	else {
		if (!(hostStruct = gethostbyname(hostnameBuf)))
			this->host = "127.0.0.1";
		else
			this->host = inet_ntoa(*((struct in_addr*)hostStruct->h_addr_list[0]));
	}
}

Server::~Server() {
	for (cltmap::iterator it = clientList.begin(); it != clientList.end(); it++)
		delete it->second;
	for (chlmap::iterator it = channelList.begin(); it != channelList.end(); it++)
		delete it->second;
	for (pollvec::iterator it = connectingFds.begin(); it < connectingFds.end(); it++)
		close(it->fd);
	delete handler;
}

void Server::init() {
	this->servSock = socket(PF_INET, SOCK_STREAM, 0);

	if (servSock == -1)
		throw std::runtime_error("Error : server socket is wrong");

	memset(&this->servAddr, 0, sizeof(this->servAddr));

	this->servAddr.sin_family = AF_INET;
	this->servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	this->servAddr.sin_port = htons(this->port);
	struct pollfd tmp = {this->servSock, POLL_IN, 0};
	this->connectingFds.push_back(tmp);

	// signal(SIGINT, SIG_IGN);

	int yes = 1;
	setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if (bind(servSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
		throw std::runtime_error("Error : bind");
	if (listen(servSock, CONNECT) == -1)
		throw std::runtime_error("Error : listen");
	fcntl(servSock, F_SETFL, O_NONBLOCK);
	this->handler = new CommandHandle(*this);
	this->running = true;
	this->startTime = getCurTime();
}

void Server::loop() {
	std::cout << "[" + getStringTime(getCurTime()) + "] server start!" << std::endl;

	while (running) {
		poll(this->connectingFds.begin().base(), (nfds_t)this->connectingFds.size(), 3000);
		for (pollvec::iterator it = this->connectingFds.begin(); it < this->connectingFds.end(); it++) {
			if (it->revents & POLL_HUP) {
				if (it->fd == servSock) {
					running = false;
					break;
				}
				else {
					delClient(it);
				}
			} else if (it->revents & POLL_IN) {
				if (it->fd == servSock) {
					addClient(it);
				}
				else {
					readMessage(it);
				}
			} else if (it->revents & POLL_OUT) {
				sendMessage(it->fd);
			}
		}
		pingLoop();
	}
}

void Server::addClient(pollvec::iterator& it) {
	int clnt_sock;
	struct sockaddr_in clnt_adr;
	socklen_t clnt_sz;
	pollfd tmp;
	pollvec::iterator::difference_type it_diff;

	memset(&tmp, 0, sizeof(tmp));
	it_diff = std::distance(this->connectingFds.begin(), it);
	clnt_sz = sizeof(clnt_adr);
	if ((clnt_sock = accept(it->fd, (struct sockaddr*)&clnt_adr, &clnt_sz)) == -1)
		throw std::runtime_error("Error : accept!()");
	tmp = (struct pollfd){clnt_sock, POLL_IN, 0};
	this->connectingFds.push_back(tmp);
	it = this->connectingFds.begin() + it_diff;
	this->clientList.insert(std::make_pair(clnt_sock, new Client(clnt_sock, clnt_adr.sin_addr)));
	this->savingBufForRead.insert(std::make_pair(clnt_sock, ""));
	this->savingBufForSend.insert(std::make_pair(clnt_sock, ""));
	fcntl(clnt_sock, F_SETFL, O_NONBLOCK);
}

void Server::delClient(pollvec::iterator& it) {
	if (this->op == this->clientList[it->fd])
		this->op = NULL;
	delete this->clientList[it->fd];
	this->savingBufForRead.erase(it->fd);
	this->savingBufForSend.erase(it->fd);
	close(it->fd);
	it = this->connectingFds.erase(it);
	this->clientList.erase(it->fd);
	// Channel에서 제거하는 부분 추가로 필요
}

void Server::addChannel(std::string& ch_name) {

}

void Server::delChannel(std::string& ch_name) {

}

void Server::pingLoop() {

}

void Server::readMessage(pollvec::iterator& it) {
	char buf[BUF_SIZE + 1];
	std::string tmp;
	std::string message = "";
	int byte = 0;
	size_t size = 0;
	bool first = true;

	memset(buf, 0, sizeof(buf));
	if ((byte = recv(it->fd, buf, BUF_SIZE, 0)) == -1)
		return ;
	if (byte == 0)
		return delClient(it);
	tmp = buf;
	while ((size = tmp.find("\r\n")) != std::string::npos) {
		if (first) {
			message = this->savingBufForRead[it->fd];
			this->savingBufForRead[it->fd] = "";
			first = false;
		}
		message += tmp.substr(0, size + 2);
		tmp = tmp.substr(size + 2, tmp.size());
		switch (this->handler->parsMessage(message)) {
			// 각 case에 대한 CommandHandle 멤버 함수 연계
			case IS_PASS:
				handler->pass(*this->clientList[it->fd], this->password);
				break;
			case IS_NICK:
				handler->nick(*this->clientList[it->fd], this->clientList);
				break;
			case IS_USER:
				handler->user(*this->clientList[it->fd]);
				break;
			case IS_NOT_ORDER:
				error::ERR_UNKNOWNCOMMAND(this->getHost(), message);
				break;
		};
		message = "";
	}
	this->savingBufForRead[it->fd] += tmp;
}

void Server::sendMessage(int fd) {
	if (savingBufForSend[fd] != "") {
		size_t size = 0;
		std::string mes = 0;

		mes = savingBufForSend[fd];
		size = send(fd, &mes[0], mes.size(), 0);
		if (size == -1)
			return ;
		savingBufForSend[fd] = "";
		if (size < mes.size()) {
			savingBufForSend[fd] = mes.substr(size, mes.size());
		}
	}
}

void Server::sendMessage(int fd, std::string message) {
	savingBufForSend[fd] += message;
	if (savingBufForSend[fd] != "") {
		size_t size = 0;
		std::string mes;

		mes = savingBufForSend[fd];
		savingBufForSend[fd] = "";
		size = send(fd, &mes[0], mes.size(), 0);
		if (size == -1)
			return ;
		if (size < mes.size()) {
			savingBufForSend[fd] = mes.substr(size, mes.size());
		}
	}
}

int const& Server::getServerSocket() const {
	return this->servSock;
}

std::string const& Server::getHost() const {
	return this->host;
}

struct sockaddr_in const& Server::getServAddr() const {
	return this->servAddr;
}

int const& Server::getPort() const {
	return this->port;
}

std::string const& Server::getPassword() const {
	return this->password;
}

Client& Server::getOp() const {
	return *this->op;
}

time_t const& Server::getServStartTime() const {
	return this->startTime;
}
