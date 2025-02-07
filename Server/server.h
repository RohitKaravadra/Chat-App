#pragma once

#include "../Client/Networking.h"
#include "../Client/MessageQueue.h"
#include "../Client/NetworkData.h"

#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>

static unsigned int USER_ID = 1; // 0 is reserved for all chat

static unsigned int GenereateID() { return USER_ID++; }

class Server :public SocketBase
{
	MsgQueue<std::pair<SOCKET, std::string>> sendQueue;

	std::thread* sendThread;
	std::vector<std::thread*> clientThreads;

	std::unordered_map<SOCKET, User> clients;

	std::atomic<bool> running;
	std::mutex mtx;
public:
	Server() = default;

	bool bind(const unsigned int& port) {
		return SocketBase::bindServer(port);
	}

	bool start() {
		running = SocketBase::startServer();
		if (running)
			sendThread = new std::thread(&Server::sendMessageThread, this);
		return running;
	}

	std::vector<User> getUsers()
	{
		std::vector<User> users;
		for (auto& c : clients)
			users.emplace_back(c.second);
		return users;
	}

	void handleClient(SOCKET socketID)
	{
		//context exchange
		int id = GenereateID();

		// send server context

		mtx.lock();
		ServerContext sc(id, getUsers());
		mtx.unlock();

		if (!sendInfo(socketID, sc.encode()))
		{
			std::cout << "Server context not send" << std::endl;
			return;
		}

		// get client context
		std::string info;
		if (!recvInfo(socketID, info))
		{
			std::cout << "Client context not received" << std::endl;
			return;
		}

		//update clients
		ClientContext cc;
		if (!cc.decode(info))
		{
			std::cout << "Client context is corrupted" << std::endl;
			return;
		}

		mtx.lock();
		clients.insert({ socketID,User(id, cc.username) });
		sendQueue.enqueue(std::make_pair(0, NetInfo(NetInfoType::clientJoined, clients[socketID].encode()).encode()));
		mtx.unlock();

		std::cout << cc.username << " Joined " << std::endl;

		//message loop
		NetInfo netInfo;
		while (running)
		{
			if (recvInfo(socketID, info))
			{
				if (info == NETWORK_EXIT)
					break;

				if (!netInfo.decode(info))
				{
					std::cout << "Corrupted info received from " << clients[socketID].username << std::endl;
					continue;
				}

				std::cout << "Received -> " << netInfo.data << std::endl;
				if (netInfo.type == NetInfoType::message)
				{
					Message msg;
					if (!msg.decode(netInfo.data))
					{
						std::cout << "Corrupted message received from " << clients[socketID].username << std::endl;
						continue;
					}

					sendQueue.enqueue(std::make_pair(msg.to, info));
				}
				std::cout << "Received " << toString(netInfo.type) << " -> " << info << std::endl;
			}
			else
				break;
		}

		std::cout << clients[socketID].username << " left." << std::endl;
		closesocket(socketID);

		mtx.lock();
		sendQueue.enqueue(std::make_pair(0, NetInfo(NetInfoType::clientLeft, clients[socketID].encode()).encode()));
		clients.erase(socketID);
		mtx.unlock();
	}

	void sendMessageThread()
	{
		std::pair<SOCKET, std::string> data;
		while (running)
		{
			if (sendQueue.dequeue(data))
			{
				mtx.lock();
				if (data.first == 0)	forwardToAll(data.second);
				else					forward(data.first, data.second);
				mtx.unlock();
			}
		}
	}

	bool accept()
	{
		SOCKET sock;
		std::string ip;
		User user;

		if (SocketBase::acceptClient(sock, ip))
		{
			clientThreads.emplace_back(new std::thread(&Server::handleClient, this, sock));
			return true;
		}

		return false;
	}

	void forward(const int& _id, std::string _data)
	{
		std::cout << "Forwarding : " << _data << std::endl;
		for (auto c = clients.begin(); c != clients.end(); c++)
			if (c->second.id == _id)
				sendInfo(c->first, _data);
	}

	void forwardToAll(std::string _data)
	{
		std::cout << "Forwarding to all : " << _data << std::endl;

		for (auto c = clients.begin(); c != clients.end(); c++)
			sendInfo(c->first, _data);
	}

	~Server()
	{
		running = false;

		for (int i = 0; i < clientThreads.size(); i++)
		{
			clientThreads[i]->join();
			delete clientThreads[i];
		}

		sendThread->join();

		std::cout << "Server Cleaned" << std::endl;
	}


};
