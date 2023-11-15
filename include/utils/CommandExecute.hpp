#ifndef _COMMANDEXECUTE_HPP_
# define _COMMANDEXECUTE_HPP_

# include "utils.hpp"
# include "../Client.hpp"
# include "../Channel.hpp"

namespace CommandExecute {
	int chkCommand();
	void motd(Client& client, std::string const& serverHost);
	void pass(Client& client, std::string const& password, std::string const& serverHost);
	void nick(Client& client, cltmap& clientList, std::string const& serverHost);
	void user(Client& client, std::string const& serverHost, time_t const& serverStartTime);
	void quit(Client& client, cltmap& clientList);
	void ping(Client& client, std::string const& serverHost);
	void pong(Client& client, std::string const& serverHost);
	void mode(Client& client, chlmap& channel, std::string const& serverHost);
	void privmsg(Client& client, chlmap& chlList);
	void notice();
	void part(Client& client, chlmap& chlList);
	void join(Client& client, chlmap& chlList, std::string const& serverHost);
	void kick(Client& client, cltmap& cltList, Channel* channel);
	void topic(Client& client, Channel* channel);
	void invite(Client& client, cltmap& cltList, Channel* channel);
};

#endif
