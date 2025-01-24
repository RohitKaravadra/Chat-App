#pragma once
#include "../Client/Networking.h"
#include <vector>

using Network::SocketBase;

class Server :public SocketBase
{
	std::vector<SOCKET> clients;
public:
	Server() = default;

	bool bind(const unsigned int& port)
	{
		return SocketBase::bindServer(port);
	}

	bool start()
	{
		return SocketBase::startServer();
	}

	bool accept(SOCKET& client, std::string& ip)
	{
		if (SocketBase::acceptClient(client, ip))
		{
			clients.emplace_back(client);
			return true;
		}
		return false;
	}
};
