#include "../../include/Channel.hpp"

Channel::Channel(std::string chName, Client* client) : chName(chName), chanOp(client), userLimit(0), mode(0), password(""), creationTime(time(NULL)), key("") {
}

Channel::~Channel() {
}

void Channel::setChName(std::string& name) {
	this->chName = name;
}

void Channel::setChanOp(Client* client) {
	this->chanOp = client;
}

void Channel::setUserLimit(int userLimit) {
	this->userLimit = userLimit;
}

void Channel::setTopic(std::string topic) {
	this->topic = topic;
}

void Channel::setPassword(std::string pw) {
	this->password = pw;
}

void Channel::setMode(int mode, bool flag) {
	if (flag)
		this->mode |= mode;
	else
		this->mode &= mode;
}

void Channel::setKey(std::string key) {
	this->key = key;
}

void Channel::addInviteList(Client* client) {
	if (this->inviteList.find(client->getClientFd()) == this->inviteList.end())
		this->inviteList.insert(std::make_pair(client->getClientFd(), client));
}

void Channel::delInviteList(Client* client) {
	if (this->inviteList.find(client->getClientFd()) != this->inviteList.end())
		this->inviteList.erase(client->getClientFd());
}

bool Channel::isClientInvite(Client* client) {
	if (this->inviteList.find(client->getClientFd()) != this->inviteList.end())
		return true;
	return false;
}

void Channel::addClientList(Client* client) {
	if (this->userList.find(client->getClientFd()) == this->userList.end())
		this->userList.insert(std::make_pair(client->getClientFd(), client));
}

void Channel::deleteClientList(Client* client) {
	if (this->userList.find(client->getClientFd()) != this->userList.end())
		this->userList.erase(client->getClientFd());
}

Client const& Channel::getChanOp() const {
	return *this->chanOp;
}

cltmap const& Channel::getUserList() const {
	return this->userList;
}

int const Channel::getUserLimit() const {
	return this->userLimit;
}

std::string const Channel::getChName() const {
	return this->chName;
}

std::string const Channel::getTopic() const {
	return this->topic;
}

time_t const Channel::getTime() const {
	return this->creationTime;
}

int const Channel::getMode() const {
	return this->mode;
}

std::string const Channel::getKey() const {
	return this->key;
}

std::string const Channel::getStrUserList() const {
	size_t size = this->userList.size();
	std::string list = "";

	if (this->chanOp) {
		list = "@" + this->chanOp->getNick() + " ";
	}
	for (cltmap::const_iterator it = this->userList.begin(); it != this->userList.end(); it++)
		if (it->second != this->chanOp)
			list += it->second->getNick() + " ";
	if (list[list.size() - 1] == ' ')
		list = list.substr(0, list.size() - 1);
	return list;
}
