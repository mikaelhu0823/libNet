#pragma once
//========================================================================
//[File Name]:Server.h
//[Description]:a implemetation of tcp server based on asio.
//[Author]:Nico Hu
//[Date]:2020-07-20
//[Other]:
//========================================================================

#include <unordered_map>
#include <numeric>
#include "RWHandler.h"
#include "asio/asio/buffer.hpp"

using namespace asio;
using namespace asio::ip;

constexpr int MaxConnNum = 65536;
constexpr int MaxRecvSize = 65536;

class Server {
public:
	explicit Server(io_context& io_c, short port):ioCtx_(io_c), acceptor_(io_c, \
	tcp::endpoint(tcp::v4(), port)), cnnIdPool_(MaxConnNum) {
		cnnIdPool_.resize(MaxConnNum);
		std::iota(cnnIdPool_.begin(), cnnIdPool_.end(), 1);
	}
	~Server() = default;

	void Start() {
		std::cout << "[Server] Start listening!" << std::endl;
		bExit_ = false;
		while (!bExit_) {
			std::shared_ptr<RWHandler> handler = CreateRWHandler();
			acceptor_.accept(handler->GetSocket());
			std::shared_ptr<std::thread> work_thread = std::make_shared<std::thread>([&, handler] {
				umRWHandlers_.insert(std::make_pair(handler->GetConnId(), handler));
				std::cout << "[Server] Current connect count:" << umRWHandlers_.size() << std::endl;

				while (!bExit_) {
					auto ret = handler->HandleRead();
					if (ret < 0)
						break;
				}
				});

			wkThds_.emplace_back(work_thread);
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}
	void Stop() {
		bExit_ = true;
		for (auto it : wkThds_) {
			if (it->joinable()) {
				it->join();
				it = nullptr;
			}
		}

		StopAccept();
		for (auto it : umRWHandlers_)
			RecycleCnnId(it.first);
		std::cout << "[Server] Stop!" << std::endl;
	}

protected:
	void HandleAcpErr(std::shared_ptr<RWHandler>& evt_handler, const asio::error_code& ec) {
		std::cout << "[Server] Err, reason:" << ec.value() << ec.message() << std::endl;
		evt_handler->CloseSocket();
		StopAccept();
	}

	void StopAccept() {
		asio::error_code ec;
		acceptor_.cancel(ec);
		acceptor_.close(ec);
		ioCtx_.stop();
	}
	std::shared_ptr<RWHandler> CreateRWHandler() {
		int conn_id = cnnIdPool_.front();
		cnnIdPool_.pop_front();

		std::shared_ptr<RWHandler> handler = std::make_shared<RWHandler>(ioCtx_);

		handler->SetConnId(conn_id);
		handler->SetCallBackError([&](int conn_id) {
			RecycleCnnId(conn_id);
			});
		return handler;
	}
	void RecycleCnnId(int conn_id) {
		auto it = umRWHandlers_.find(conn_id);
		if (it != umRWHandlers_.end()) {
			it->second->CloseSocket();
			umRWHandlers_.erase(it);
		}

		std::cout << "[Server] Current connect count : " << umRWHandlers_.size() << std::endl;
		cnnIdPool_.emplace_back(conn_id);
	}

protected:
	bool bExit_{false};
	io_context& ioCtx_;
	tcp::acceptor acceptor_;
	std::unordered_map<int, std::shared_ptr<RWHandler>> umRWHandlers_;
	std::list<int> cnnIdPool_;
	std::list<std::shared_ptr<std::thread>> wkThds_;
};
