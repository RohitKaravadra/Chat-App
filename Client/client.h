#pragma once
#include "Networking.h"

using Network::SocketBase;

class Client :public SocketBase
{
public:
	Client() = default;

	bool connect(std::string host, const unsigned int& port)
	{
		return SocketBase::connectServer(host, port);
	}

	bool send(std::string msg)
	{
		return Network::sendMessage(socketID, msg);
	}

	bool recv(const unsigned int& bufferSize, std::string& out)
	{
		return Network::recvMessage(socketID, bufferSize, out);
	}
};