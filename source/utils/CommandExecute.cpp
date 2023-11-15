#include "../../include/utils/CommandExecute.hpp"
#include "../../include/utils/Buffer.hpp"
#include "../../include/utils/Message.hpp"
#include "../../include/utils/error.hpp"
#include "../../include/utils/reply.hpp"
#include "../../include/utils/Print.hpp"
#include <sstream>

int CommandExecute::getCommand() {
	mesvec const& message = Message::getMessage();

	if (!message.size())
		return IS_NOT_ORDER;
	if (message[0] == "PASS")
		return IS_PASS;
	if (message[0] == "NICK")
		return IS_NICK;
	if (message[0] == "USER")
		return IS_USER;
	if (message[0] == "PING")
		return IS_PING;
	if (message[0] == "PONG")
		return IS_PONG;
	if (message[0] == "MODE")
		return IS_MODE;
	if (message[0] == "JOIN")
		return IS_JOIN;
	if (message[0] == "QUIT")
		return IS_QUIT;
	// if (message[0] == "QUIT")
	// 	return IS_QUIT;
	// if (message[0] == "MODE")
	// 	return IS_MODE;
	return IS_NOT_ORDER;
}

void CommandExecute::motd(Client& client, std::string const& serverHost) {
	Buffer::sendMessage(client.getClientFd(), reply::RPL_MOTDSTART(serverHost, client.getNick()));
	Buffer::sendMessage(client.getClientFd(), reply::RPL_MOTD(serverHost, client.getNick(), "Hello! This is FT_IRC!"));
	Buffer::sendMessage(client.getClientFd(), reply::RPL_ENDOFMOTD(serverHost, client.getNick()));
}

void CommandExecute::pass(Client& client, std::string const& password, std::string const& serverHost) {
	mesvec const& message = Message::getMessage();

	if (message.size() != 2) {
		Buffer::sendMessage(client.getClientFd(), error::ERR_NEEDMOREPARAMS(serverHost, "PASS"));
	} else if (client.getPassConnect() & IS_PASS) {
		Buffer::sendMessage(client.getClientFd(), error::ERR_ALREADYREGISTERED(serverHost));
	} else if (message[1] != password) {
		Buffer::sendMessage(client.getClientFd(), error::ERR_PASSWDMISMATCH(serverHost));
	} else {
		client.setPassConnect(IS_PASS);
	}
}

static bool duplicate_nick(cltmap& clients, std::string const& nick) {
	for (cltmap::iterator it = clients.begin(); it != clients.end(); it++) {
		if (clients[it->first]->getNick() == nick)
			return true;
	}
	return false;
}

void CommandExecute::nick(Client& client, cltmap& clientList, std::string const& serverHost) {
	mesvec const& message = Message::getMessage();

	// Nickname 충돌 오류는 어차피 서버 간 통신은 신경 쓰지 않아도 되기에 구현 안 함
	if (message.size() != 2)
		Buffer::sendMessage(client.getClientFd(), error::ERR_NONICKNAMEGIVEN(serverHost));
	else if (duplicate_nick(clientList, message[1]))
		Buffer::sendMessage(client.getClientFd(), error::ERR_NICKNAMEINUSE(serverHost, message[1]));
	else if (chkForbiddenChar(message[1], "#&:") || std::isdigit(message[1][0]))
		Buffer::sendMessage(client.getClientFd(), error::ERR_ERRONEUSNICKNAME(serverHost, message[1]));
	else {
		client.setPassConnect(IS_NICK);
		if (client.getNick() != "") {
			// 별칭 변경 사항 타 클라이언트에게 알리기
		}
		client.setNick(message[1]);
	}
}

void CommandExecute::user(Client& client, std::string const& serverHost, time_t const& serverStartTime) {
	mesvec const& message = Message::getMessage();

	if (message.size() != 5)
		Buffer::sendMessage(client.getClientFd(), error::ERR_NEEDMOREPARAMS(serverHost, "USER"));
	else if (client.getPassConnect() & IS_USER)
		Buffer::sendMessage(client.getClientFd(), error::ERR_ALREADYREGISTERED(serverHost));
	else if (!(client.getPassConnect() & (IS_PASS | IS_NICK)))
		Buffer::sendMessage(client.getClientFd(), error::ERR_NOTREGISTERED(serverHost, "You input pass, before It enroll User"));
	else {
		client.setPassConnect(IS_USER);
		client.setUser(message[1]);
		client.setHost(message[2]);
		client.setServ(message[3]);
		if (message[4][0] == ':')
			client.setReal(message[4].substr(1, message[4].size()));
		else
			client.setReal(message[4]);
		if (client.getPassConnect() & IS_LOGIN) {
			time_t serv_time = serverStartTime;
			Buffer::sendMessage(client.getClientFd(), reply::RPL_WELCOME(serverHost, client.getNick(), client.getUser(), client.getHost()));
			Buffer::sendMessage(client.getClientFd(), reply::RPL_YOURHOST(serverHost, client.getNick(), "1.0"));
			Buffer::sendMessage(client.getClientFd(), reply::RPL_CREATED(serverHost, client.getNick(), getStringTime(serv_time)));
			Buffer::sendMessage(client.getClientFd(), reply::RPL_MYINFO(serverHost, client.getNick(), "ircserv 1.0", "x", "itkol"));
			Buffer::sendMessage(client.getClientFd(), reply::RPL_ISUPPORT(serverHost, client.getNick()));
			CommandExecute::motd(client, serverHost);
		}
	}
}

void CommandExecute::ping(Client& client, std::string const& serverHost) {
	mesvec const& message = Message::getMessage();

	client.setFinalTime();
	if (message.size() != 2)
		error::ERR_NEEDMOREPARAMS(serverHost, message[0]);
	else if (message[1] != serverHost)
		error::ERR_NOORIGIN(serverHost, client.getNick());
	else
		CommandExecute::pong(client, serverHost);
}

void CommandExecute::pong(Client& client, std::string const& serverHost) {
	Buffer::sendMessage(client.getClientFd(), ":" + serverHost + " PONG " + serverHost + " :" + serverHost);
}

static bool chkNum(std::string const& str) {
	for (int i = 0; i < str.size(); i++)
		if (str[i] < '0' || str[i] > '9')
			return false;
	return true;
}

static void createSetMode(int mode, Channel& chan, std::string& outputMode, std::string& outputValue) {
	std::ostringstream oss;

	outputMode += "+";
	for (int i = 0; i < 5; i++) {
		if (mode & (1 << i)) {
			switch (1 << i) {
				case USER_LIMIT_PER_CHANNEL:
					oss << chan.getUserLimit();
					outputMode += "l";
					outputValue += oss.str() + " ";
					oss.clear();
					break;
				case INVITE_CHANNEL:
					outputMode += "i";
					break;
				case KEY_CHANNEL:
					outputMode += "k";
					outputValue += chan.getKey() + " ";
					break;
				case SAFE_TOPIC:
					outputMode += "t";
					break;
				case SET_CHANOP:
					outputMode += "o";
					outputValue += chan.getChanOp().getNick() + " ";
					break;
			}
		}
	}
	if (outputValue != "" && outputValue[outputValue.size() - 1] == ' ') {
		outputValue = outputValue.substr(0, outputValue.size() - 1);
	}
}

void CommandExecute::mode(Client& client, chlmap& channel, std::string const& serverHost) {
	mesvec const& message = Message::getMessage();
	chlmap::iterator it;
	std::string successMode = "";
	std::string successValue = "";
	std::string supportMode = "itkol";
	std::ostringstream oss;
	bool flag = true;
	int set[5];
	int val = 3;

	if (message.size() == 2 && (it = channel.find(message[1])) != channel.end()) {
		oss << it->second->getTime();
		createSetMode(it->second->getMode(), *it->second, successMode, successValue);
		Buffer::sendMessage(client.getClientFd(), reply::RPL_CHANNELMODEIS(serverHost, client.getNick(), message[1], successMode, successValue));
		Buffer::sendMessage(client.getClientFd(), reply::RPL_CREATIONTIME(serverHost, client.getNick(), message[1], oss.str()));
	}
	else if (message.size() < 3 || message.size() > 4)
		Buffer::sendMessage(client.getClientFd(), error::ERR_NEEDMOREPARAMS(serverHost, "MODE"));
	else if ((it = channel.find(message[1])) == channel.end())
		Buffer::sendMessage(client.getClientFd(), error::ERR_NOSUCHCHANNEL(serverHost, client.getNick(), message[1]));
	else if (channel[message[1]]->getChanOp().getClientFd() != client.getClientFd())
		Buffer::sendMessage(client.getClientFd(), error::ERR_CHANOPRIVSNEEDED(serverHost, client.getNick(), message[1]));
	else {
		for (int i = 0; i < message[2].size(); i++) {
			if (message[2][i] == '+' || message[2][i] == '-') {
				if (message[2][i] == '+') {
					successMode += "+";
					flag = true;
					for (int j = 0; j < 5; j++)
						set[j] = (1 << j);
				} else {
					successMode += "-";
					flag = false;
					for (int k = 0; k < 5; k++)
						set[k] = ~(1 << k);
				}
				continue;
			}
			if (supportMode.find(message[2][i]) != std::string::npos) {
				switch (message[2][i]) {
					case 'l':
						if (val > message.size()
							|| !chkNum(message[val]))
							Buffer::sendMessage(client.getClientFd(),
								error::ERR_INVALIDMODEPARAM(serverHost, client.getNick(), message[1], message[2][i], "You must specify a parameter. Syntax: <limit>"));
						else {
							it->second->setMode(set[0], flag);
							successMode += "l";
							successValue += message[val] + " ";
						}
						val++;
						break;
					case 'i':
						it->second->setMode(set[1], flag);
						successMode += "i";
						break;
					case 'k':
						if (val > message.size()
							|| message[val] == "")
							Buffer::sendMessage(client.getClientFd(),
								error::ERR_INVALIDMODEPARAM(serverHost, client.getNick(), message[1], message[2][i], "You must specify a parameter. Syntax: <key>"));
						else {
							it->second->setMode(set[2], flag);
							successMode += "k";
							successValue += message[val] + " ";
						}
						val++;
						break;
					case 't':
						it->second->setMode(set[3], flag);
						break;
					case 'o':
						if (val > message.size()
							|| message[val] == "")
							Buffer::sendMessage(client.getClientFd(),
								error::ERR_INVALIDMODEPARAM(serverHost, client.getNick(), message[1], message[2][i], "You must specify a parameter. Syntax: <nick>"));
						else {
							it->second->setMode(set[4], flag);
							successMode += "o";
						}
						val++;
						break;
				}
			} else {
				Buffer::sendMessage(client.getClientFd(), error::ERR_UNKNOWNMODE(serverHost, client.getNick(), message[2][i]));
			}
		}
	}
	if (successMode != "") {
		cltmap const& userList = it->second->getUserList();
		if (successValue != "" && successValue[successValue.size() - 1] == ' ')
			successValue = successValue.substr(0, successValue.size() - 1);
		for (cltmap::const_iterator iter = userList.begin(); iter != userList.end(); iter++)
			Buffer::sendMessage(iter->second->getClientFd(), reply::RPL_SUCCESSMODE(client.getNick(), client.getUser(), client.getHost(), message[1], successMode, successValue));
	}
}

static bool isChanName(std::string const& chanName) {
	if (chanName.size() < 2)
		return false;
	if (chanName[0] == '&' || chanName[0] == '#')
		return true;
	return false;
}

static void addChannel(std::string& chName, Client* client, chlmap& chlList) {
	Channel* channel = new Channel(chName, client);

	chlList.insert(std::make_pair(chName, channel));
}

static void delChannel(std::string& chName, chlmap& chlList) {
	if (chlList.find(chName) != chlList.end()) {
		delete chlList[chName];
		chlList.erase(chName);
	}
}

void CommandExecute::join(Client& client, chlmap& chlList, std::string const& serverHost) {
	std::istringstream chan;
	std::istringstream key;
	std::string chanStr = "";
	std::string keyStr = "";
	mesvec const& message = Message::getMessage();
	Channel* channel;

	if (message.size() < 2 || message.size() > 3)
		Buffer::sendMessage(client.getClientFd(), error::ERR_NEEDMOREPARAMS(serverHost, "JOIN"));
	else {
		chan.str(message[1]);
		if (message.size() == 3)
			key.str(message[2]);
		while (std::getline(chan, chanStr, ',')) {
			if (message.size() == 3)
				std::getline(key, keyStr, ',');
			if (!isChanName(chanStr)) {
				Buffer::sendMessage(client.getClientFd(), error::ERR_BADCHANMASK(serverHost, client.getNick(), chanStr));
				continue;
			}
			if (chlList.find(chanStr) == chlList.end())
				addChannel(chanStr, &client, chlList);
			channel = chlList.find(chanStr)->second;
			switch (client.joinChannel(chlList.find(chanStr)->second, keyStr)) {
				case TOOMANYCHANNELS:
					Buffer::sendMessage(client.getClientFd(), error::ERR_TOOMANYCHANNELS(serverHost, client.getNick(), chanStr));
					break;
				case CHANNELISFULL:
					Buffer::sendMessage(client.getClientFd(), error::ERR_CHANNELISFULL(serverHost, client.getNick(), chanStr));
					break;
				case INVITEONLYCHAN:
					Buffer::sendMessage(client.getClientFd(), error::ERR_INVITEONLYCHAN(serverHost, client.getNick(), chanStr));
					break;
				case BADCHANNELKEY:
					Buffer::sendMessage(client.getClientFd(), error::ERR_BADCHANNELKEY(serverHost, client.getNick(), chanStr));
					break;
				case IS_SUCCESS:
					Buffer::sendMessage(client.getClientFd(), reply::RPL_SUCCESSJOIN(client.getNick(), client.getUser(), client.getHost(), chanStr));
					if (channel->getTopic() != "")
						Buffer::sendMessage(client.getClientFd(), reply::RPL_TOPIC(serverHost, client.getNick(), chanStr, channel->getTopic()));
					Buffer::sendMessage(client.getClientFd(), reply::RPL_NAMREPLY(serverHost, client.getNick(), chanStr, channel->getStrUserList()));
					Buffer::sendMessage(client.getClientFd(), reply::RPL_ENDOFNAMES(serverHost, client.getNick(), chanStr));
					for (cltmap::const_iterator iter = channel->getUserList().begin(); iter != channel->getUserList().end(); iter++)
						if (iter->second != &client)
							Buffer::sendMessage(iter->second->getClientFd(), reply::RPL_SUCCESSJOIN(client.getNick(), client.getUser(), client.getHost(), chanStr));
					break;
			}
			chanStr = "";
			keyStr = "";
		}
	}
}

void CommandExecute::quit(Client& client, cltmap& clientList) {
	mesvec const& message = Message::getMessage();
	std::string reason = "";

	for (int i = 1; i < message.size(); i++)
		reason += message[i] + " ";
	if (reason != "" && reason[reason.size() - 1] == ' ')
		reason = reason.substr(0, reason.size() - 1);
	for (cltmap::iterator it = clientList.begin(); it != clientList.end(); it++)
		if (it->second != &client)
			Buffer::sendMessage(client.getClientFd(), reply::RPL_SUCCESSQUIT(client.getNick(), client.getUser(), client.getHost(), reason));
}
