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

	/**
	 * 매개변수로 받은 포트를 long으로 변환, pointer를 이용해서 오류가 있는지 확인한다.
	 * well-known port 이상인 1023부터 65535까지만 허용한다.
	 * 이후 서버의 포트를 지정한다.
	 */
	strictPort = std::strtol(port.c_str(), &pointer, 10);
	if (*pointer != 0 || strictPort <= 1023 || strictPort > 65535)
		throw std::runtime_error("Error : port is wrong");
	this->port = static_cast<int>(strictPort);

	/**
	 * 매개변수로 받은 패스워드를 검증하고, 서버의 패스워드를 지정한다.
	 */
	for (size_t i = 0; i < password.size(); i++) {
		if (password[i] == 0 || password[i] == '\r'
			|| password[i] == '\n' || password[i] == ':')
			throw std::runtime_error("Error : password is wrong");
	}
	this->password = password;


	/**
	 *	호스트의 이름(Domain Name)을 가져온다.
	 *	예시 : c4r6s5.42seoul.kr
	 */
	if (gethostname(hostnameBuf, sizeof(hostnameBuf)) == -1)
		throw std::runtime_error("Error : Failed to run gethostname system call!");
	std::cout << hostnameBuf << std::endl;

	/**
	 * hostname을 통해 hostent 구조체를 가져온다.
	 */
	if (!(hostStruct = gethostbyname(hostnameBuf)))
		throw std::runtime_error("Error : Failed to run gethostbyname with buffer!");

	// internet_networkToAddress를 통해 hostStruct에 있는 주소를 가져온다. = IPv4 주소
	// 이 경우, 현재 컴퓨터의 IPv4 주소로 호스트가 지정된다.
	this->host = inet_ntoa(*((struct in_addr*)hostStruct->h_addr_list[0]));

	// kqueue를 열어보고 안되면 에러처리
	if ((this->kq = kqueue()) == -1)
		throw std::runtime_error("kqueue error!");
}

/**
 * 클라이언트 맵을 지우고, 채팅 채널을 지운 후, kq를 닫는다.
 */
Server::~Server() {
	for (cltmap::iterator it = clientList.begin(); it != clientList.end(); it++)
		delete it->second;
	for (chlmap::iterator it = channelList.begin(); it != channelList.end(); it++)
		delete it->second;
	close(kq);
}

// 서버 초기화
void Server::init() {
	int yes = 1; // ?

	// 서버의 소켓을 연다. PF_INET는 IPv4,
	// SOCK_STREAM은 TCP 프로토콜을 사용하는 연결 지향형 소켓
	// socket() 함수는 PF_INET에 맞는 기본 프로토콜
	if ((this->servSock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		throw std::runtime_error("Error : server socket is wrong");

	// 서버의 주소 값을 초기화, socket_internet의 family, address, port를 지정
	memset(&this->servAddr, 0, sizeof(this->servAddr));
	this->servAddr.sin_family = AF_INET;
	this->servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	this->servAddr.sin_port = htons(this->port);


	// event를 kqueue에 추가한다.
	// connectingFds는 현재 서버와 연결된 fd들에 대한 vector다.
	// EVFILT_READ는 파일 디스크립터에서 읽기 가능한 데이터가 있는지를 검사하는 필터
	// 이벤트를 추가, 활성화 한다.
	pushEvents(this->connectingFds, this->servSock, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);

	// 서버의 소켓 옵션을 설정한다. default 세팅이라고 생각하면 된다.
	// SOL_SOCKET: 옵션의 레벨(level)을 지정합니다. SOL_SOCKET은 일반적인 소켓 옵션을 설정하는 데 사용
	// SO_REUSEADDR: 설정하려는 옵션의 이름입니다. SO_REUSEADDR은 주로 TCP 소켓에서 사용되며, 이 옵션을 설정함으로써 이전에 사용된 주소와 포트를 즉시 재사용
	// &yes: 옵션의 값을 설정하는 매개변수입니다. 여기서 yes는 int형 변수로, SO_REUSEADDR 옵션을 사용할 것이므로 일반적으로 1로 설정됩니다. 이는 해당 소켓이 이전에 사용된 주소를 재사용할 수 있도록 허용
	// 옵션 값의 크기.
	setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	// bind() 함수는 소켓에 주소를 할당하는 함수
	// 서버 소켓에 주소를 할당한다.
	if (bind(servSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
		throw std::runtime_error("Error : bind");

	// listen() 함수는 소켓을 연결 대기 상태로 만드는 함수
	// 서버 소켓을 연결 대기 상태로 만든다.
	if (listen(servSock, CONNECT) == -1)
		throw std::runtime_error("Error : listen");

	// fcntl() 함수는 파일 디스크립터의 플래그를 변경하는 함수
	// 서브젝트 자체에서 해당 소켓을 논블로킹으로 설정하도록 얘기했음(MacOS의 경우).
	fcntl(servSock, F_SETFL, O_NONBLOCK);

	// 서버의 가동 상태를 의미하는 플래그
	this->running = true;

	// 서버 시작 시간 설정
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
