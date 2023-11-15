#ifndef _UTILS_HPP_
# define _UTILS_HPP_

# include <ctime>
# include <string>
# include <vector>
# include <map>

class Client;
class Channel;

typedef std::vector<std::string> mesvec;
typedef std::map<int, std::string> fdmap;
typedef std::vector<struct kevent> kquvec;
typedef std::map<int, Client*> cltmap;
typedef std::map<std::string, Channel*> chlmap;
typedef std::map<int, Client*> cltmap;
typedef std::map<std::string, Channel*> chlmap;

// 서버에서 명령어를 확인하는 부분
# define IS_PASS 1 << 0
# define IS_NICK 1 << 1
# define IS_USER 1 << 2
# define IS_LOGIN 7
# define IS_PING 1 << 3
# define IS_PONG 1 << 4
# define IS_MODE 1 << 5
# define IS_JOIN 1 << 6
# define IS_QUIT 1 << 7
# define IS_NOT_ORDER 421

// error와 reply의 숫자, 채널과 클라이언트 쪽에서 사용
# define TOOMANYCHANNELS 405
# define CHANNELISFULL 471
# define INVITEONLYCHAN 473
# define BADCHANNELKEY 475

# define IS_SUCCESS 10000

// 채널 모드 확인
# define USER_LIMIT_PER_CHANNEL 1 << 0
# define INVITE_CHANNEL 1 << 1
# define KEY_CHANNEL 1 << 2
# define SAFE_TOPIC 1 << 3
# define SET_CHANOP 1 << 4

// 전반적으로 적용하는 define 매크로
# define CONNECT 1000 // 서버 접속자 수 상한
# define MAX_CHANNEL 30 // 서버가 최대로 보유할 수 있는 채널 상한
# define USERNICK_LEN 9 // 사용자 별칭의 최대 길이(RFC 1459)
# define CHANNELNAME_LEN 200 // 채널 이름 최대 길이(RFC 1459)
# define CHANNEL_LIMIT_PER_USER 10 // 클라이언트 당 참가할 수 있는 채널 상항

// system call 실패에 대한 상수
# define SYS_FAILURE -1
# define CRLF "\r\n"
# define CR '\r'
# define LF '\n'

/*
	유틸이 할 일
	0. define 매크로 저장
	1. 시간 계산 함수
	2. message에서 내용물 걸러주는 함수
*/

enum Color {
	RESET = 0,
	RED = 31,
	GREEN = 32,
	YELLOW = 33,
	BLUE = 34,
	MAGENTA = 35,
	CYAN = 36,
	WHITE = 37
};

// 시간 계산 함수
time_t getCurTime();
std::string getStringTime(time_t const& time);

// 메세지에 금지된 문자가 있는 지 확인
bool chkForbiddenChar(std::string const& str, std::string const& forbidden_set);
#endif
