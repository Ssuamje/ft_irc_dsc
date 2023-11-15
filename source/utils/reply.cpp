#include "../../include/utils/reply.hpp"

std::string const suffix = "\r\n";

std::string const reply::RPL_WELCOME(std::string const& server_host, std::string const& nick, std::string const& user, std::string const& host) {
	return ":" + server_host + " 001 " + nick + " :Welcome to the Internet Relay Network " + nick + "!" + user + "@" + host + suffix;
}

std::string const reply::RPL_YOURHOST(std::string const& server_host, std::string const& nick, std::string const& version) {
	return ":" + server_host + " 002 " + nick + " :Your host is " + server_host + ", running version " + version + suffix;
}

std::string const reply::RPL_CREATED(std::string const& server_host, std::string const& nick, std::string const& date) {
	return ":" + server_host + " 003 " + nick + " :This server was created " + date + suffix;
}

std::string const reply::RPL_MYINFO(std::string const& server_host, std::string const& nick, std::string const& version, std::string const& usermode, std::string const& chanmode) {
	return ":" + server_host + " 004 " + nick + " :" + server_host + " " + version + " " + usermode + " " + chanmode + suffix;
}

std::string const reply::RPL_ISUPPORT(std::string const& server_host, std::string const& nick) {
	return ":" + server_host + " 005 " + nick + " :CASEMAPPING=rfc1459 CHANMODES=i,t,k,o,l CHANTYPES=&# CHARSET=ascii MASCHANNELS=10 MAXNICKLEN=9" + suffix; 
}

std::string const reply::RPL_MOTDSTART(std::string const& server_host, std::string const& nick) {
	return ":" + server_host + " 375 " + nick + " :- " + server_host + " Message of the day - " + suffix;
}

std::string const reply::RPL_MOTD(std::string const& server_host, std::string const& nick, std::string const& line) {
	return ":" + server_host + " 372 " + nick + " :" + line + suffix;
}

std::string const reply::RPL_ENDOFMOTD(std::string const& server_host, std::string const& nick) {
	return ":" + server_host + " 376 " + nick + " :End of /MOTD command." + suffix;
}

std::string const reply::RPL_CHANNELMODEIS(std::string const& serverHost, std::string const& nick, std::string const& chName, std::string const& mode, std::string const& argument) {
	return ":" + serverHost + " 324 " + nick + " " + chName + " " + mode + " " + argument + suffix;
}

std::string const reply::RPL_CREATIONTIME(std::string const& serverHost, std::string const& nick, std::string const& chName, std::string const& time) {
	return ":" + serverHost + " 329 " + nick + " " + chName + " :" + time;
}

std::string const reply::RPL_SUCCESSMODE(std::string const& nick, std::string const& user, std::string const& host, std::string const& chName, std::string const& mode, std::string const& argument) {
	return ":" + nick + "!" + user + "@" + host + " MODE " + chName + " " + mode + " " + argument + suffix;
}

std::string const reply::RPL_SUCCESSJOIN(std::string const& nick, std::string const& user, std::string const& host, std::string const& chName) {
	return ":" + nick + "!" + user + "@" + host + " JOIN " + chName + " :" + chName + suffix;
}

std::string const reply::RPL_TOPIC(std::string const& serverHost, std::string const& nick, std::string const& chName, std::string const& topic) {
	return ":" + serverHost + " 332 " + nick + " " + chName + " :" + topic + suffix;
}

std::string const reply::RPL_NAMREPLY(std::string const& serverHost, std::string const& nick, std::string const& chName, std::string const& userList) {
	return ":" + serverHost + " 353 " + nick + " = " + chName + " :" + userList + suffix;
}

std::string const reply::RPL_ENDOFNAMES(std::string const& serverHost, std::string const& nick, std::string const& chName) {
	return ":" + serverHost + " 366 " + nick + " " + chName + " :End of /NAMES list." + suffix;
}

std::string const reply::RPL_SUCCESSQUIT(std::string const& nick, std::string const& user, std::string const& host, std::string const& reason) {
	return ":" + nick + "!" + user + "@" + host + " QUIT :Quit: " + reason + suffix;
}
