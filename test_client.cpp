#include "Client.h"

int main(int argc, char** argv) {
	asio::io_context io_c;
	asio::io_context::work worker(io_c);
	std::thread t([&io_c]() {io_c.run(); });

	Client clt(io_c, "127.0.0.1", 9900);
	clt.Start();

	if (!clt.IsConnected()) {
		std::cout << "Client connect to Server failed!" << std::endl;
		return -1;
	}

	std::string str;
	while (std::cin >> str) {
		Message msg;
		msg.encodeData(str.data(), str.length() + 1);
		clt.Send(msg.data(), msg.length());
	}

	io_c.stop();
	t.join();

	return 0;
}