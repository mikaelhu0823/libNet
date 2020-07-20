#pragma once
//========================================================================
//[File Name]:Server.h
//[Description]:a implemetation of read-writer handler to process socket stream.
//[Author]:Nico Hu
//[Date]:2020-07-20
//[Other]:
//========================================================================

#include <array>
#include <functional>
#include <iostream>
#include "asio/asio.hpp"
#include <thread>
#include "Message.h"

using namespace asio;
using namespace asio::ip;


class RWHandler : public std::enable_shared_from_this<RWHandler> {
public:
	explicit RWHandler(io_context& io_c) : socket_(io_c) {}
	~RWHandler() = default;

	int HandleRead() {
		readMsg_.clear();
		std::error_code ec;
		auto size = asio::read(socket_, asio::buffer(readMsg_.data(), Message::header_len), transfer_exactly(Message::header_len), ec);
		if (ec == asio::stream_errc::eof) {
			HandleErr(ec); // Connection closed cleanly by peer.
			return -1;
		}
		else if (ec) {
			HandleErr(ec); // Some other error.
			return -2;
		}

		if (!readMsg_.decodeHeader()) {
			std::cout << "[RWHandler] read data with wrong header!" << std::endl;
			return 0;
		}

		int ret = ReadBody();
		if (ret > 0) {
			std::cout << "[RWHandler] read data : " << readMsg_.body() << std::endl;
		}
		return ret;
	}
	int ReadBody() {
		std::error_code ec;
		auto size = asio::read(socket_, asio::buffer(readMsg_.body(), readMsg_.bodyLen()), transfer_exactly(readMsg_.bodyLen()), ec);
		if (ec == asio::stream_errc::eof) {
			HandleErr(ec); // Connection closed cleanly by peer.
			return -1;
		}
		else if (ec) {
			HandleErr(ec); // Some other error.
			return -2;
		}

		return size;
	}
	void HandleWrite(char* data, int len) {
		asio::error_code ec;
		write(socket_, asio::buffer(data, len), ec);
		if (ec)
			HandleErr(ec);
	}
	void HandleAsyncWrite(char* data, int len) {
		char* t = new char[len];
		memcpy_s(t, sizeof(char) * len, data, sizeof(char) * len);
		this->sendBuff_.emplace_back(std::make_tuple(std::shared_ptr<char>(t), len));
		
		AsyncWrite();
	}
	void AsyncWrite() {
		auto msg = this->sendBuff_.front();
		async_write(socket_, asio::buffer(std::get<0>(msg).get(), std::get<1>(msg)), [this](asio::error_code ec, size_t size) {
				if (ec) {
					HandleErr(ec);
					if (!this->sendBuff_.empty())
						this->sendBuff_.clear();
				}
				else {
					std::cout << "[RWHandler] write data size : " << size << std::endl;
					this->sendBuff_.pop_front();
				}
			});
		this->sendBuff_.pop_front();
	}

	tcp::socket& GetSocket() {
		return socket_;
	}
	void CloseSocket() {
		asio::error_code ec;
		socket_.shutdown(tcp::socket::shutdown_send, ec);
		socket_.close();
	}
	void SetConnId(int conn_id) {
		connId_ = conn_id;
	}
	int GetConnId() const {
		return connId_;
	}
	template<typename F>
	void SetCallBackError(F f) {
		cbErr_ = f;
	}

protected:
	void HandleErr(const asio::error_code& ec) {
		CloseSocket();
		std::cout << "[RWHandler] Err, reason : " << ec.message() << std::endl;
		if (cbErr_)
			cbErr_(connId_);
	}

protected:
	tcp::socket socket_;
	std::list<std::tuple<std::shared_ptr<char>, int>> sendBuff_;
	int connId_{-1};
	std::function<void(int)> cbErr_;
	Message readMsg_;
};