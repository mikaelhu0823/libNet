#pragma once
//========================================================================
//[File Name]:Server.h
//[Description]:a implemetation of tcp client based on asio.
//[Author]:Nico Hu
//[Date]:2020-07-20
//[Other]:
//========================================================================

#include <array>
#include <functional>
#include <iostream>
#include <thread>

#include "asio/asio.hpp"
#include "RWHandler.h"

using namespace asio;
using namespace asio::ip;

class Client {
public:
	explicit Client(io_context& io_c, const std::string str_ip, short port) : ioCtx_(io_c),
		serverAddr_(tcp::endpoint(address::from_string(str_ip), port))
	{
		CreateRWHandler(io_c);
	}
	~Client() {}

	bool Start() {
		bExit_ = false;
		rwHandler_->GetSocket().async_connect(serverAddr_, [&](const asio::error_code& ec) {
			if (ec) {
				HandleConnErr(ec);
				return;
			}
			std::cout << "[Client] Connect ok!" << std::endl;
			isConnected_ = true;
			
			while (!bExit_) {
				auto ret = rwHandler_->HandleRead();
				if (ret < 0)
					break;
			}
			});

		std::this_thread::sleep_for(std::chrono::seconds(5));
		return isConnected_;
	}
	void Stop() {
		bExit_ = true;
		if (chkThread_ && chkThread_->joinable()) {
			chkThread_->join();
			chkThread_ = nullptr;
		}

		isConnected_ = false;
		rwHandler_->CloseSocket();
		std::cout << "[Client] Disconnect ok!" << std::endl;
	}
	bool IsConnected() const {
		return isConnected_;
	}
	void Send(char* data, int len) {
		if (!isConnected_)
			return;
		rwHandler_->HandleWrite(data, len);
	}
	void AsyncSend(char* data, int len) {
		if (!isConnected_)
			return;
		rwHandler_->HandleAsyncWrite(data, len);
	}

protected:
	void CreateRWHandler(io_context& io_c) {
		rwHandler_ = std::make_shared<RWHandler>(io_c);
		rwHandler_->SetCallBackError([this](int conn_id) { HandleRWErr(conn_id); });
	}
	void CheckConnect() {
		if (chkThread_ != nullptr)
			return;

		chkThread_ = std::make_shared<std::thread>([&] {
			while (!bExit_) {
				if (!IsConnected())
					Start();

				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			});
	}
	void HandleConnErr(const asio::error_code& ec) {
		isConnected_ = false;
		std::cout << "[Client] Err, reasion : " << ec.message() << std::endl;
		CheckConnect();
	}
	void HandleRWErr(int conn_id) {
		std::cout << "[Client] RW err, connect id : " << conn_id << std::endl;
		isConnected_ = false;
		CheckConnect();
	}

protected:
	io_context& ioCtx_;
	tcp::endpoint serverAddr_;
	std::shared_ptr<RWHandler> rwHandler_{nullptr};
	bool isConnected_{false};
	bool bExit_{false};
	std::shared_ptr<std::thread> chkThread_{nullptr};
};