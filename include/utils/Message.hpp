#ifndef _MESSAGE_HPP_
# define _MESSAGE_HPP_

/*
	메세지 파싱 전용 정적 클래스

	날 것 그대로의 메세지를 파싱하여 돌려준다
*/

#include "utils.hpp"

class Message {
private:
	Message();
	static mesvec comMes;
public:
	~Message();
	static void parsMessage(std::string& origin);
	static mesvec const& getMessage();
};

#endif
