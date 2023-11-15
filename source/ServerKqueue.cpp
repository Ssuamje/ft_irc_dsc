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
	if (gethostname(hostnameBuf, sizeof(hostnameBuf)) == SYS_FAILURE)
		throw std::runtime_error("Error : Failed to run gethostname system call!");

	/**
	 * hostname을 통해 hostent 구조체를 가져온다.
	 */
	if (!(hostStruct = gethostbyname(hostnameBuf)))
		throw std::runtime_error("Error : Failed to run gethostbyname with buffer!");

	// internet_networkToAddress를 통해 hostStruct에 있는 주소를 가져온다. = IPv4 주소
	// 이 경우, 현재 컴퓨터의 IPv4 주소로 호스트가 지정된다.
	this->host = inet_ntoa(*((struct in_addr*)hostStruct->h_addr_list[0]));

	// kqueue를 열어보고 안되면 에러처리
	if ((this->kq = kqueue()) == SYS_FAILURE)
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

	// 서버의 소켓을 연다. PF_INET는 IPv4,
	// SOCK_STREAM은 TCP 프로토콜을 사용하는 연결 지향형 소켓
	// socket() 함수는 PF_INET에 맞는 기본 프로토콜
	if ((this->serverSocket = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		throw std::runtime_error("Error : server socket is wrong");

	// 서버의 주소 값을 초기화, socket_internet의 family, address, port를 지정
	memset(&this->servAddr, 0, sizeof(this->servAddr));
	this->servAddr.sin_family = AF_INET;
	this->servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	this->servAddr.sin_port = htons(this->port);

	/*
	요약:
	서버의 메인 소켓(this->servSock)에 대한 읽기 이벤트(EVFILT_READ)를 this->eventListToRegister 벡터에 추가한다.
	(내부적으로 EV_SET 매크로를 사용해 kevent 구조체를 초기화하고, 이를 this->eventListToRegister 벡터에 추가)
	이 함수 호출은 kqueue 이벤트 모니터링 시스템을 설정하는 데 사용된다.

	1) 함수 호출 분석
	this->eventListToRegister: 이벤트를 추가할 kqueue 이벤트 벡터. 이 벡터는 struct kevent 객체들을 저장하며, kqueue에 등록될 각종 이벤트를 관리한다.
	(struct kevent 구조체는 다양한 유형의 이벤트(예: 파일 디스크립터의 읽기/쓰기 가능, 신호, 타이머 등)를 모니터링하는 데 필요. 이 구조체는 kqueue 시스템을 통해 특정 이벤트의 발생을 감시하고, 그에 따라 적절한 조치를 취할 수 있도록 정보를 제공함.)
	this->servSock: 이벤트를 감지할 대상인 서버의 메인 소켓 파일 디스크립터.
	EVFILT_READ: 읽기 이벤트 필터. 새로운 클라이언트 연결이 들어오는 것을 감지하는 데 사용된다.
	EV_ADD | EV_ENABLE: 이벤트 플래그. EV_ADD는 새 이벤트를 추가하라는 지시이며, EV_ENABLE는 이벤트를 활성화하라는 의미다.
	0, 0, NULL: kevent 함수의 추가 인자들로, 이 경우 특별한 기능을 수행하지 않는다.
	
	2) 코드의 목적
	이 호출은 서버가 클라이언트로부터의 새 연결 요청을 감지하기 위해 필요하다. EVFILT_READ 이벤트는 this->servSock 소켓에 데이터(새 연결 요청)가 도착했는지를 kqueue 시스템이 감시하게 한다.
	클라이언트로부터 연결 요청이 들어오면, 이 이벤트가 발생하며 서버는 이를 처리할 수 있다.
	이 방식은 효율적인 네트워크 이벤트 처리를 위해 사용되며, 서버가 동시에 여러 연결을 관리할 수 있게 해준다.
	*/
	pushEventToList(this->eventListToRegister, this->serverSocket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);

	// 서버의 소켓 옵션을 설정한다. default 세팅이라고 생각하면 된다.
	// SOL_SOCKET: 옵션의 레벨(level)을 지정합니다. SOL_SOCKET은 일반적인 소켓 옵션을 설정하는 데 사용
	// SO_REUSEADDR: 설정하려는 옵션의 이름입니다. SO_REUSEADDR은 주로 TCP 소켓에서 사용되며, 이 옵션을 설정함으로써 이전에 사용된 주소와 포트를 즉시 재사용
	// &yes: 옵션의 값을 설정하는 매개변수입니다. 여기서 yes는 int형 변수로, SO_REUSEADDR 옵션을 사용할 것이므로 일반적으로 1로 설정됩니다. 이는 해당 소켓이 이전에 사용된 주소를 재사용할 수 있도록 허용
	// 옵션 값의 크기
	bool isReuseAddr = true;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &isReuseAddr, sizeof(int));

	// bind() 함수는 소켓에 주소를 할당하는 함수
	// 서버 소켓에 주소를 할당한다.
	if (bind(serverSocket, (struct sockaddr*)&servAddr, sizeof(servAddr)) == SYS_FAILURE)
		throw std::runtime_error("Error : bind");

	// listen() 함수는 소켓을 연결 대기 상태로 만드는 함수
	// 서버 소켓을 연결 대기 상태로 만든다.
	if (listen(serverSocket, CONNECT) == SYS_FAILURE)
		throw std::runtime_error("Error : listen");

	// fcntl() 함수는 파일 디스크립터의 플래그를 변경하는 함수
	// 서브젝트 자체에서 해당 소켓을 논블로킹으로 설정하도록 얘기했음(MacOS의 경우).
	fcntl(serverSocket, F_SETFL, O_NONBLOCK);

	// 서버의 가동 상태를 의미하는 플래그
	this->running = true;

	// 서버 시작 시간 설정
	this->startTime = getCurTime();
}

// 서버 루프(사실상 run)
void Server::loop() {
	int cntNewEvents;
	struct kevent newEvents[CNT_EVENT_POOL];

	Print::PrintLineWithColor("[" + getStringTime(getCurTime()) + "] server start!", BLUE);

	// 루프로 계속 kqueue에 이벤트가 있는지 확인한다.
	while (this->running) {

		// kq는 서버에서 관리하는 kqueue fd고, eventListToRegister는 이벤트를 확인하기 위한 kevent 구조체의 리스트
		// this->eventListToRegister.size()는 목록에 포함된 이벤트의 수를 나타낸다.

		/**
		 * kq는 운영체제 자체에서 관리하는 kqueue에 대한 식별자이다.
		 * 이 상황에서, eventListToRegister는 서버 어플리케이션 자체에서 kqueue에 등록하고자 하는 이벤트들의 리스트다.
		 * kevent를 호출하면, 커널에서는 kqueue에 등록된 kevent 구조체들에 대해서, 이벤트가 발생한 경우 eventList에 새 이벤트들을 담아준다.
		 *
		 * 동작 흐름
		 * 첫 이벤트는 server socket에 대한 read 이벤트를 등록해놓는다. 이 이벤트를 등록하면, 클라이언트가 새롭게 연결을 요청할 때에,
		 * 서버의 소켓 자체에 write를 하게 되고, 이 경우에 kqueue에서 첫번째로 등록되어 있는 identifier가 server socket이고, read인 이벤트를 발생시킨다.
		 * 그 경우에, addClient에서 새로운 클라이언트를 등록하고, 이벤트를 추가한다.
		 */
		cntNewEvents = kevent(this->kq, &this->eventListToRegister[0], this->eventListToRegister.size(), newEvents, CNT_EVENT_POOL, NULL);

		this->eventListToRegister.clear();

		for (int i = 0; i < cntNewEvents; i++) {
			struct kevent cur = newEvents[i];
			if (cur.flags & EV_ERROR) {
				if (isServerEvent(cur.ident)) {
					running = false;
					break ;
				}
				else {
					deleteClient(cur.ident);
				}
			}
			if (cur.flags & EVFILT_READ) {
				if (isServerEvent(cur.ident)) {
					addClient(cur.ident);
				}
				if (this->containsCurrentEvent(cur.ident)) {
					handleReadEvent(cur.ident, cur.data);
				}
			}
			if (cur.ident & EVFILT_WRITE) {
				if (this->containsCurrentEvent(cur.ident))
					handleWriteEvent(cur.ident);
			}
		}
		// 새 이벤트에 대한 처리가 끝난 이후에 다음 루프를 돌기 전에, 클라이언트와의 연결 상태를 확인한다.
		handleDisconnectedClients();
	}
}

bool Server::containsCurrentEvent(uintptr_t ident) {
	return (this->clientList.find(ident) != this->clientList.end());
}

bool Server::isServerEvent(uintptr_t ident) {
	return (ident == this->serverSocket);
}

/*
요약:
kqueue 이벤트 모델을 사용하여 이벤트를 설정하고, 이를 관리할 목록인 kquvec 벡터에 추가하는 역할을 한다.

작동 방식:
1) 이벤트 구성 (EV_SET 매크로 사용):

EV_SET 매크로는 struct kevent 구조체를 초기화하는 데 사용된다.
이 구조체는 kevent 함수에 의해 모니터링될 이벤트의 세부 사항을 정의한다.

ident: 이벤트가 연결될 파일 디스크립터 또는 식별자입니다.
filter: 이벤트의 유형을 지정합니다 (예: EVFILT_READ는 읽기 이벤트).
flags: 이벤트에 대한 작업을 지정합니다 (예: EV_ADD는 이벤트 추가, EV_ENABLE은 이벤트 활성화).
fflags: 필터별 플래그, 필터에 따라 특정 작동을 정의합니다.
data: 필터에 의해 사용될 추가 데이터.
udata: 사용자 정의 데이터, 필터에 의해 사용될 수 있습니다.

2) 이벤트 목록에 추가:
초기화된 struct kevent 객체 tmp를 list (이벤트 목록을 나타내는 kquvec 벡터)에 추가합니다.
이렇게 하면 해당 이벤트가 kqueue 시스템에 의해 추후 모니터링될 수 있도록 준비됩니다.
*/
void Server::pushEventToList(kquvec& list, uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void* udata) {
	struct kevent toPut;

	// ident: eventListRegister, filter: EVFILT_READ, flags: EV_ADD | EV_ENABLE, fflags: 0, data: 0, udata: NULL
	EV_SET(&toPut, ident, filter, flags, fflags, data, udata);
	list.push_back(toPut);
}

void Server::addClient(int fd) {
	int clientSocket;
	struct sockaddr_in clntAdr;
	socklen_t clntSz;

	clntSz = sizeof(clntAdr);
	if ((clientSocket = accept(fd, (struct sockaddr*)&clntAdr, &clntSz)) == -1)
		throw std::runtime_error("Error : accept!()");
	pushEventToList(this->eventListToRegister, clientSocket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	pushEventToList(this->eventListToRegister, clientSocket, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
	this->clientList.insert(std::make_pair(clientSocket, new Client(clientSocket, clntAdr.sin_addr)));
	Buffer::resetReadBuf(clientSocket);
	Buffer::resetSendBuf(clientSocket);
	fcntl(clientSocket, F_SETFL, O_NONBLOCK);

	Print::PrintComplexLineWithColor("[" + getStringTime(time(NULL)) + "] " + "Connected Client : ", clientSocket, GREEN);
}

void Server::deleteClient(int fd) {
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

void Server::handleDisconnectedClients() {
	time_t curTime = time(NULL);

	for (cltmap::iterator it = this->clientList.begin(); it != clientList.end(); it++) {
		if ((it->second->getPassConnect() & IS_LOGIN) && (curTime - it->second->getTime()) > 120)
			deleteClient(it->first);
	}
}

void Server::handleReadEvent(int fd, intptr_t data) {
	std::string buffer;
	std::string message;
	int byte = 0;
	size_t size = 0;
	int suffixFlag = 0;
	int cut;

	this->clientList[fd]->setFinalTime();
	byte = Buffer::readMessage(fd, data);

	if (byte == -1)
		return ;
	if (byte == 0)
		return deleteClient(fd);

	buffer = Buffer::getReadBuf(fd);
	Buffer::resetReadBuf(fd);
	while (true) {
		if ((size = buffer.find(CRLF)) != std::string::npos) {
			suffixFlag = 0;
		} else if ((size = buffer.find(CR)) != std::string::npos) {
			suffixFlag = 1;
		} else if ((size = buffer.find(LF)) != std::string::npos) {
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

void Server::handleWriteEvent(int fd) {
	this->clientList[fd]->setFinalTime();
	Buffer::sendMessage(fd);
}

void Server::runCommand(int fd) {
	switch (CommandExecute::getCommand()) {
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
			this->clientList[fd]->setFinalTime();
			break;
		case IS_MODE:
			CommandExecute::mode(*this->clientList[fd], this->channelList, this->host);
			break;
		case IS_JOIN:
			CommandExecute::join(*this->clientList[fd], this->channelList, this->host);
			break;
		case IS_QUIT:
			this->deleteClient(fd);
			break;
		case IS_NOT_ORDER:
			Buffer::sendMessage(fd, error::ERR_UNKNOWNCOMMAND(this->host, (Message::getMessage())[0]));
			break;
	};
}

int const& Server::getServerSocket() const {
	return this->serverSocket;
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

time_t const& Server::getStartTime() const {
	return this->startTime;
}
