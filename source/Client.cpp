#include "../include/Client.hpp"

Client::Client(int fd, in_addr info) : passConnect(0), passPing(false), op(false), fd(fd), info(info), host(inet_ntoa(info)), serv(""), nick(""), real("") {
}

Client::~Client() {
	chlmap::iterator it = this->joinList.begin();

	for (; it != this->joinList.end(); it++) {
		if (it->second->getChanOp().getClientFd() == this->fd)
			it->second->setChanOp(NULL);
		it->second->deleteClientList(this);
	}
	close(fd);
}

void Client::addJoinList(Channel* channel) {
	if (this->joinList.find(channel->getChName()) == this->joinList.end())
		this->joinList.insert(std::make_pair(channel->getChName(), channel));
}

void Client::delJoinList(Channel* channel) {
	if (this->joinList.find(channel->getChName()) != this->joinList.end())
		this->joinList.erase(channel->getChName());
}

int Client::joinChannel(Channel* channel, std::string const& key) {
	int channelMode = channel->getMode();

	if (this->joinList.size() == CHANNEL_LIMIT_PER_USER)
		return TOOMANYCHANNELS;
	if (channelMode & INVITE_CHANNEL && !channel->isClientInvite(this))
		return INVITEONLYCHAN;
	if (channelMode & USER_LIMIT_PER_CHANNEL && channel->getUserLimit() == channel->getUserList().size())
		return CHANNELISFULL;
	if (channelMode & KEY_CHANNEL && key != channel->getKey())
		return BADCHANNELKEY;
	channel->addClientList(this);
	if (channelMode & INVITE_CHANNEL)
		channel->delInviteList(this);
	this->addJoinList(channel);
	return IS_SUCCESS;
}

void Client::setPassPing(bool flag) {
	this->passPing = flag;
}

void Client::setPassConnect(int flag) {
	this->passConnect |= flag;
}

void Client::setNick(std::string nick) {
	this->nick = nick;
}

void Client::setReal(std::string real) {
	this->real = real;
}

void Client::setHost(std::string host) {
	this->host = host;
}

void Client::setUser(std::string user) {
	this->user = user;
}

void Client::setServ(std::string serv) {
	this->serv = serv;
}

void Client::setTime() {
	this->finalTime = time(NULL);
}

bool Client::getOp() const {
	return this->op;
}

int Client::getPassConnect() const {
	return this->passConnect;
}

int Client::getClientFd() const {
	return this->fd;
}

std::string const& Client::getHost() const {
	return this->host;
}

std::string const& Client::getNick() const {
	return this->nick;
}

std::string const& Client::getReal() const {
	return this->real;
}

std::string const& Client::getUser() const {
	return this->user;
}

std::string const& Client::getServ() const {
	return this->serv;
}

time_t const& Client::getTime() const {
	return this->finalTime;
}
