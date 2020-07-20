#pragma once
#include "Server.h"

void test_server() {
	io_context io_c;
	
	asio::io_context::work worker(io_c);
	std::thread t([&io_c]() {io_c.run(); });

	Server server(io_c, 9900);
	server.Start();

	io_c.stop();
	t.join();
}

int main(int argc, char** argv) {
	test_server();

	std::this_thread::sleep_for(std::chrono::seconds(1000));

	return 0;
}