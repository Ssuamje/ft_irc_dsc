#ifndef _ERROR_HPP_
# define _ERROR_HPP_

# include <string>

// 숫적 응답(numeric reply) 중, 에러 담당
namespace error {
	std::string const ERR_INPUTTOOLONG(std::string const& serverHost);
	std::string const ERR_NEEDMOREPARAMS(std::string const& serverHost, std::string const& command);
	std::string const ERR_ALREADYREGISTERED(std::string const& serverHost);
	std::string const ERR_PASSWDMISMATCH(std::string const& serverHost);
	std::string const ERR_NONICKNAMEGIVEN(std::string const& serverHost);
	std::string const ERR_NICKNAMEINUSE(std::string const& serverHost, std::string const& nick);
	std::string const ERR_ERRONEUSNICKNAME(std::string const& serverHost, std::string const& nick);
	std::string const ERR_NOTREGISTERED(std::string const& serverHost, std::string const& reason);
	std::string const ERR_UNKNOWNCOMMAND(std::string const& serverHost, std::string const& command);
	std::string const ERR_NOORIGIN(std::string const& serverHost, std::string const& nick);
	std::string const ERR_INVALIDMODEPARAM(std::string const& serverHost, std::string const& nick, std::string const& chName, char const& mode, std::string const& reason);
	std::string const ERR_UNKNOWNMODE(std::string const& serverHost, std::string const& nick, char const& mode);
	std::string const ERR_NOSUCHCHANNEL(std::string const& serverHost, std::string const& nick, std::string const& chName);
	std::string const ERR_CHANOPRIVSNEEDED(std::string const& serverHost, std::string const& nick, std::string const& chName);
	std::string const ERR_BADCHANMASK(std::string const& serverHost, std::string const& nick, std::string const& chName);
	std::string const ERR_TOOMANYCHANNELS(std::string const& serverHost, std::string const& nick, std::string const& chName);
	std::string const ERR_CHANNELISFULL(std::string const& serverHost, std::string const& nick, std::string const& chName);
	std::string const ERR_INVITEONLYCHAN(std::string const& serverHost, std::string const& nick, std::string const& chName);
	std::string const ERR_BADCHANNELKEY(std::string const& serverHost, std::string const& nick, std::string const& chName);
}

#endif
