#ifndef _CHANNEL_HPP_
# define _CHANNEL_HPP_

/*
	Channel이 하는 일
	1. 단일 채널에 필요한 변수 보유
		a. 채널 운영자 client 포인터
		b. 채널 소속 인원 목록
		c. ban 목록... 근데 루프백 IP면 이게 소용이 있나?
		d. 채널 모드 플래그
	2. 위 내용물을 볼 수 있는 getter 함수
	3. 채널 내 클라이언트 제거, 추가 함수
*/

class Client;

# include "./utils/utils.hpp"
# include "./Client.hpp"

class Channel {
private:
	// 채널 이름
	std::string chName;

	// 채널 운영자
	Client* chanOp;

	// 채널 가입자 명단
	cltmap userList;

	// 가입자수 상한
	int userLimit;

	// 채널 모드(비트 마스킹)
	int mode;

	// 채널 주제
	std::string topic;

	// 채널 비밀번호
	std::string password;

	// 채널 생성 시간
	time_t creationTime;

	// 채널 암호
	std::string key;

	// 초대자 명단
	cltmap inviteList;
public:
	Channel(std::string chName, Client* client);
	~Channel();

	// setter
	void setChName(std::string& name);
	void setChanOp(Client* client);
	void setUserLimit(int userLimit);
	void setTopic(std::string topic);
	void setPassword(std::string pw);
	void setMode(int mode, bool flag);
	void setKey(std::string key);

	// add
	void addInviteList(Client* client);
	void addClientList(Client* client);

	// del
	void delInviteList(Client* client);
	void deleteClientList(Client* client);

	// getter
	Client const& getChanOp() const;
	cltmap const& getUserList() const;
	int const getUserLimit() const;
	std::string const getChName() const;
	std::string const getTopic() const;
	int const getMode() const;
	time_t const getTime() const;
	std::string const getKey() const;
	std::string const getStrUserList() const;

	// chker
	bool isClientInvite(Client* client);
};

#endif
