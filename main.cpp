#include <iostream>
#include "ServerKqueue.hpp"

/**
 * 코딩 컨벤션
 * 1. 웬만해서 전부 다 카멜 케이스
 * 2. 클래스는 대문자로 시작
 * 3. 함수, 프로퍼티는 소문자로 시작
 * 4. 중괄호는 함수나 클래스의 시작하는 라인부터
 * 5. 변수 선언, 유효성 검사는 맨 위에
 * 6. 코드 블럭의 동작이 길고, 구분될 수 있다면 개행으로 분리
 * 7. 가능하다면 주석을 달 것
 */

int main(int ac, char* av[]) {
	// 변수 선언

	// 유효성 검사
	if (ac != 3) {
		std::cerr << "Error : ./ircserv [port] [password]" << std::endl;
		return 1;
	}

	// 로직
	try {
		Server ircServ(av[1], av[2]);
		ircServ.init();
		ircServ.loop();
		std::cout << "hello world!" << std::endl;
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}
