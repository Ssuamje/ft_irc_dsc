#include "../../include/utils/utils.hpp"

time_t getCurTime() {
	return time(NULL);
}

std::string getStringTime(time_t const& time) {
	char buf[50];
	struct tm* tm;

	memset(buf, 0, sizeof(buf));
	tm = localtime(&time);
	strftime(buf, 50, "%c", tm);
	return buf;
}

bool chkForbiddenChar(std::string const& str, std::string const& forbidden_set) {
	for (int i = 0; i < str.size(); i++) {
		for (int j = 0; j < forbidden_set.size(); j++)
			if (str[i] == forbidden_set[j])
				return true;
	}
	return false;
}
