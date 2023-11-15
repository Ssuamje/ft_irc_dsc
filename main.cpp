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

/**
 * Git 컨벤션
 *
 * 커밋 메시지(prefix)
 * FEAT(기능 추가), ex) FEAT: 새로운 기능 추가
 * FIX(고치거나 개선),
 * DOCS(문서 관련),
 * REFACTOR(리팩토링),
 * MOVE(파일 이동이나 이름 변경),
 * REMOVE(파일 삭제)
 */

const std::string USAGE = "Usage : ./ircserv [port] [password]";

int main(int ac, char* av[]) {

	if (ac != 3) {
		std::cerr << USAGE << std::endl;
		return 1;
	}

	try {
		Server ircServ(av[1], av[2]);
		ircServ.init();
		ircServ.loop();
	} catch (std::exception& e) {
		Print::printError(e.what());
	}
}
