#include "Server.h"

int main() {
	Server server;
	server.run("127.0.0.1"); // это локальный IP, чтобы узнать свой надо в коммандной строке ввести ipconfig

	return 0;
}
