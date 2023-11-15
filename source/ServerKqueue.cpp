#include "../include/ServerKqueue.hpp"
#include <fcntl.h>
#include <stdexcept>
#include <cstdlib>
#include <signal.h>
#include <netdb.h>

Server::Server(std::string port, std::string password) : opName(""), opPassword(""), op(NULL) {
	char* pointer;
	long strictPort;
	char hostnameBuf[1024];
	struct hostent* hostStruct;

	strictPort = std::strtol(port.c_str(), &pointer, 10);
	if (*pointer != 0 || strictPort <= 1000 || strictPort > 65535)
		throw std::runtime_error("Error : port is wrong");
	for (size_t i = 0; i < password.size(); i++) {
		if (password[i] == 0 || password[i] == '\r'
			|| password[i] == '\n' || password[i] == ':')
			throw std::runtime_error("Error : password is wrong");
	}
	this->password = password;
	this->port = static_cast<int>(strictPort);

	if (gethostname(hostnameBuf, sizeof(hostnameBuf)) == -1)
		this->host = "127.0.0.1";
	else {
		if (!(hostStruct = gethostbyname(hostnameBuf)))
			this->host = "127.0.0.1";
		else
			this->host = inet_ntoa(*((struct in_addr*)hostStruct->h_addr_list[0]));
	}

	if ((this->kq = kqueue()) == -1)
		throw std::runtime_error("kqueue error!");
}

Server::~Server() {
	for (cltmap::iterator it = clientList.begin(); it != clientList.end(); it++)
		delete it->second;
	for (chlmap::iterator it = channelList.begin(); it != channelList.end(); it++)
		delete it->second;
	close(kq);
}

void Server::init() {
	int yes = 1;

	this->servSock = socket(PF_INET, SOCK_STREAM, 0);

	if (servSock == -1)
		throw std::runtime_error("Error : server socket is wrong");

	memset(&this->servAddr, 0, sizeof(this->servAddr));
	this->servAddr.sin_family = AF_INET;
	this->servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	this->servAddr.sin_port = htons(this->port);

	pushEvents(this->connectingFds, this->servSock, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);

	// signal(SIGINT, SIG_IGN);

	setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	if (bind(servSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
		throw std::runtime_error("Error : bind");
	if (listen(servSock, CONNECT) == -1)
		throw std::runtime_error("Error : listen");
	
	fcntl(servSock, F_SETFL, O_NONBLOCK);

	this->running = true;
	this->startTime = getCurTime();
}

void Server::loop() {
	int new_events;
	struct kevent eventList[15];

	Print::PrintLineWithColor("[" + getStringTime(getCurTime()) + "] server start!", BLUE);

	while (running) {
		new_events = kevent(this->kq, &this->connectingFds[0], this->connectingFds.size(), eventList, 15, NULL);

		this->connectingFds.clear();
	
		for (int i = 0; i < new_events; i++) {
			if (eventList[i].flags & EV_ERROR) {
				if (eventList[i].ident == this->servSock) {
					running = false;
					break;
				}
				else {
					delClient(eventList[i].ident);
				}
			} else if (eventList[i].flags & EVFILT_READ) {
				if (eventList[i].ident == this->servSock) {
					addClient(eventList[i].ident);
				}
				else if (clientList.find(eventList[i].ident) != clientList.end()) {
					chkReadMessage(eventList[i].ident, eventList[i].data);
				}
			} else if (eventList[i].ident & EVFILT_WRITE) {
				if (clientList.find(eventList[i].ident) != clientList.end())
					chkSendMessage(eventList[i].ident);
			}
		}
		monitoring();
	}
}

void Server::pushEvents(kquvec& list, uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void* udata) {
	struct kevent tmp;

	EV_SET(&tmp, ident, filter, flags, fflags, data, udata);
	list.push_back(tmp);
}

void Server::addClient(int fd) {
	int clntSock;
	struct sockaddr_in clntAdr;
	socklen_t clntSz;

	clntSz = sizeof(clntAdr);
	if ((clntSock = accept(fd, (struct sockaddr*)&clntAdr, &clntSz)) == -1)
		throw std::runtime_error("Error : accept!()");
	pushEvents(this->connectingFds, clntSock, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	pushEvents(this->connectingFds, clntSock, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
	this->clientList.insert(std::make_pair(clntSock, new Client(clntSock, clntAdr.sin_addr)));
	Buffer::resetReadBuf(clntSock);
	Buffer::resetSendBuf(clntSock);
	fcntl(clntSock, F_SETFL, O_NONBLOCK);

	Print::PrintComplexLineWithColor("[" + getStringTime(time(NULL)) + "] " + "Connected Client : ", clntSock, GREEN);
}

void Server::delClient(int fd) {
	if (this->op == this->clientList[fd])
		this->op = NULL;
	delete this->clientList[fd];
	Buffer::eraseReadBuf(fd);
	Buffer::eraseSendBuf(fd);
	this->clientList.erase(fd);
	Print::PrintComplexLineWithColor("[" + getStringTime(time(NULL)) + "] " + "Disconnected Client : ", fd, RED);
}

void Server::addChannel(std::string& chName, Client* client) {
	Channel* channel = new Channel(chName, client);

	this->channelList.insert(std::make_pair(chName, channel));
}

void Server::delChannel(std::string& chName) {
	if (this->channelList.find(chName) != this->channelList.end()) {
		delete channelList[chName];
		channelList.erase(chName);
	}
}

void Server::monitoring() {
	time_t curTime = time(NULL);

	for (cltmap::iterator it = this->clientList.begin(); it != clientList.end(); it++) {
		if ((it->second->getPassConnect() & IS_LOGIN) && (curTime - it->second->getTime()) > 120)
			delClient(it->first);
	}
}

void Server::chkReadMessage(int fd, intptr_t data) {
	std::string buffer;
	std::string message;
	int byte = 0;
	size_t size = 0;
	int suffixFlag = 0;
	int cut;

	this->clientList[fd]->setTime();
	byte = Buffer::readMessage(fd, data);

	if (byte == -1)
		return ;
	if (byte == 0)
		return delClient(fd);

	buffer = Buffer::getReadBuf(fd);
	Buffer::resetReadBuf(fd);
	while (1) {
		if ((size = buffer.find("\r\n")) != std::string::npos) {
			suffixFlag = 0;
		} else if ((size = buffer.find("\r")) != std::string::npos) {
			suffixFlag = 1;
		} else if ((size = buffer.find("\n")) != std::string::npos) {
			suffixFlag = 2;
		} else {
			break;
		}
		if (suffixFlag == 0)
			cut = size + 2;
		else
			cut = size + 1;

		message = "";
		message += buffer.substr(0, cut);
		buffer = buffer.substr(cut, buffer.size());
		if (message.size() > 512) {
			Buffer::sendMessage(fd, error::ERR_INPUTTOOLONG(this->getHost()));
			continue;
		}
		Message::parsMessage(message);
		runCommand(fd);
	}
	Buffer::setReadBuf(std::make_pair(fd, buffer));
}

void Server::chkSendMessage(int fd) {
	this->clientList[fd]->setTime();
	Buffer::sendMessage(fd);
}

void Server::runCommand(int fd) {
	switch (CommandExecute::chkCommand()) {
		// 각 case에 대한 CommandHandle 멤버 함수 연계
		case IS_PASS:
			CommandExecute::pass(*this->clientList[fd], this->password, this->host);
			break;
		case IS_NICK:
			CommandExecute::nick(*this->clientList[fd], this->clientList, this->host);
			break;
		case IS_USER:
			CommandExecute::user(*this->clientList[fd], this->host, this->startTime);
			break;
		case IS_PING:
			CommandExecute::ping(*this->clientList[fd], this->host);
			break;
		case IS_PONG:
			this->clientList[fd]->setTime();
			break;
		case IS_MODE:
			CommandExecute::mode(*this->clientList[fd], this->channelList, this->host);
			break;
		case IS_JOIN:
			CommandExecute::join(*this->clientList[fd], this->channelList, this->host);
			break;
		case IS_QUIT:
			this->delClient(fd);
			break;
		case IS_NOT_ORDER:
			Buffer::sendMessage(fd, error::ERR_UNKNOWNCOMMAND(this->host, (Message::getMessage())[0]));
			break;
	};
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
