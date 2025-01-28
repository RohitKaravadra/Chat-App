#pragma once
#include "../Client/Networking.h"
#include <vector>
#include <thread>
#include <mutex>

static unsigned int USER_ID = 1; // 0 is reserved for all chat

struct Client {
	User user;
	std::string ip;
	SOCKET socketID;

	bool operator==(const Client& c) {
		return c.socketID == socketID;
	}
};

static unsigned int GenereateID() { return USER_ID++; }

class Server :public SocketBase
{
	std::vector<std::thread*> clientThreads;
	std::vector<Client> clients;

	std::atomic<bool> running;
	std::mutex mtx;
public:
	Server() = default;

	bool bind(const unsigned int& port) {
		return SocketBase::bindServer(port);
	}

	bool start() {
		return running = SocketBase::startServer();
	}

	bool OnUserJoinded(Client& client)
	{
		// receive user name
		if (recvMessage(client.socketID, client.user.name))
		{
			client.user.id = GenereateID();			// generate unique userid

			// send generated user id
			if (sendMessage(client.socketID, std::to_string(client.user.id)))
			{
				std::lock_guard<std::mutex> lock(mtx);

				clients.emplace_back(client);
				std::cout << client.user.name << " joined." << std::endl;

				return true;
			}
		}

		return false;
	}

	void handleClient(Client client)
	{
		while (running)
		{
			std::string msg;
			if (recvMessage(client.socketID, msg))
			{
				if (msg == NETWORK_EXIT)
					break;

				Message data;
				if (decodeMessage(msg, data))
				{
					// peocess message
					// forward message
					std::cout << msg << std::endl;
				}
			}
			else
				break;
		}

		std::cout << client.user.name << " left." << std::endl;
		closesocket(client.socketID);

		std::lock_guard<std::mutex> lock(mtx);
		clients.erase(std::find(clients.begin(), clients.end(), client)); // remove client
	}

	Client accept()
	{
		Client client;

		if (SocketBase::acceptClient(client.socketID, client.ip))
			if (OnUserJoinded(client))
				clientThreads.emplace_back(new std::thread(&Server::handleClient, this, client));

		return client;
	}

	bool forwardMsg(const Client& c, std::string msg)
	{
		return sendMessage(c.socketID, msg);
	}

	bool forwardMsgAll(std::string msg)
	{
		std::lock_guard<std::mutex> lock(mtx);

		for (int i = 0; i < clients.size(); i++)
			sendMessage(clients[i].socketID, msg);
		return true;
	}

	~Server()
	{
		running = false;
		for (int i = 0; i < clientThreads.size(); i++)
		{
			clientThreads[i]->join();
			delete clientThreads[i];
		}

		std::cout << "Server Cleaned" << std::endl;
	}


};
