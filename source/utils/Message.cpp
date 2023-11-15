#include "../../include/utils/Message.hpp"
#include "../../include/utils/utils.hpp"
#include "../../include/utils/Print.hpp"
#include <sstream>

mesvec Message::comMes;

Message::Message() {}

Message::~Message() {}

void Message::parsMessage(std::string& origin) {
	Message::comMes.clear();

	bool first = true;
	std::istringstream str;
	std::string tmp = "";

	if (origin[origin.size() - 1] == '\r'
		|| (origin[origin.size() - 2] != '\r' && origin[origin.size() - 1] == '\n'))
		str.str(origin.substr(0, origin.size() - 1));
	else
		str.str(origin.substr(0, origin.size() - 2));

	while (std::getline(str, tmp, ' ')) {
		if (first) {
			if (tmp.empty() || chkForbiddenChar(tmp, "\r\n\0"))
				break ;
			else
				comMes.push_back(tmp);
			first = false;
		}
		else {
			if (tmp.empty())
				continue ;
			else if (tmp[0] == ':') {
				comMes.push_back(tmp.substr(1, tmp.size()));
				break;
			}
			else {
				if (!chkForbiddenChar(tmp, ":\r\n\0"))
					comMes.push_back(tmp);
				else
					break;
			}
		}
	}

	tmp = "";
	str >> tmp;
	if (tmp != "")
		comMes[comMes.size() - 1] += " " + tmp;
}

mesvec const& Message::getMessage() {
	return comMes;
}
