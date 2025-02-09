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
	MsgQueue<std::pair<int, std::string>> sendQueue;

	std::thread* sendThread;
	std::vector<std::thread*> clientThreads;

	std::unordered_map<SOCKET, User> clients;

	std::atomic<bool> running;
	std::mutex mtx;
public:
	Server() = default;

	// bind server to given port
	// return true if bind successful
	bool bind(const unsigned int& port) {
		return SocketBase::bindServer(port);
	}

	// start server 
	// return true if successful
	bool start() {
		running = SocketBase::startServer();
		if (running)
			sendThread = new std::thread(&Server::sendMessageThread, this);
		return running;
	}

	// returns a list of users for server context
	std::vector<User> getUsers()
	{
		std::vector<User> users;
		for (auto& c : clients)
			users.emplace_back(c.second);
		return users;
	}

	// thread method to handle connected client
	void handleClient(SOCKET socketID)
	{
		// generate new unique id for this client
		int id = GenereateID();

		// create server context
		mtx.lock();													// critical section begin
		ServerContext sc(id, getUsers());
		mtx.unlock();												// critical section end

		// send server context
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
		// critical section begin
		mtx.lock();
		clients.insert({ socketID,User(id, cc.username) });			// add new user to client list
		sendQueue.enqueue(std::make_pair(0, NetInfo(NetInfoType::clientJoined, clients[socketID].encode()).encode()));	// add client joined info to send queue
		mtx.unlock();												// critical section end

		std::cout << cc.username << " Joined " << std::endl;

		//message loop
		NetInfo netInfo;
		while (running)
		{
			if (recvInfo(socketID, info))		// receive info from clients
			{
				if (info == NETWORK_EXIT)		// check for exit message
					break;

				if (!netInfo.decode(info))		// decode info
				{
					std::cout << "Corrupted info received from " << clients[socketID].username << std::endl;
					continue;
				}

				std::cout << "Received -> " << netInfo.data << std::endl;
				if (netInfo.type == NetInfoType::message)	// check if info is a message
				{
					Message msg;
					if (!msg.decode(netInfo.data))			// decode message from info
					{
						std::cout << "Corrupted message received from " << clients[socketID].username << std::endl;
						continue;
					}

					sendQueue.enqueue(std::make_pair(msg.to, info));	// add message to send queue
				}
				std::cout << "Received " << toString(netInfo.type) << " -> " << info << std::endl;
			}
			else
				break;
		}

		std::cout << clients[socketID].username << " left." << std::endl;
		closesocket(socketID);	// close connection

		mtx.lock();					// critical section begin
		sendQueue.enqueue(std::make_pair(0, NetInfo(NetInfoType::clientLeft, clients[socketID].encode()).encode()));	// add client left info to send queue
		clients.erase(socketID);	// remove user from client list
		mtx.unlock();				// critical section end
	}

	// thread method to handle sending of information
	void sendMessageThread()
	{
		std::pair<int, std::string> data;	// tmp data object to get data
		while (running)
		{
			if (sendQueue.dequeue(data))	// get data from queue
			{
				mtx.lock();														// critical section begin
				if (data.first == 0)	forwardToAll(data.second);				// broadcast info
				else					forward(data.first, data.second);		// forward info
				mtx.unlock();													// critical section end
			}
		}
	}

	// accept client connection
	// return true if client connected
	bool accept()
	{
		SOCKET sock;
		std::string ip;
		User user;

		if (SocketBase::acceptClient(sock, ip))
		{
			clientThreads.emplace_back(new std::thread(&Server::handleClient, this, sock));	// strat new client thread
			return true;
		}

		return false;
	}

	// forward information to given connection
	// _id: user id of receiver
	// _data: information to send
	void forward(const int& _id, std::string _data)
	{
		std::cout << "Forwarding : " << _data << std::endl;
		for (auto c = clients.begin(); c != clients.end(); c++)
			if (c->second.id == _id)
				sendInfo(c->first, _data);
	}

	// broadcast information to all connections
	// _data: information to broadcast
	void forwardToAll(std::string _data)
	{
		std::cout << "Forwarding to all : " << _data << std::endl;

		for (auto c = clients.begin(); c != clients.end(); c++)
			sendInfo(c->first, _data);
	}

	~Server()
	{
		running = false;

		for (int i = 0; i < clientThreads.size(); i++) // destroy all client threads
		{
			clientThreads[i]->join();
			delete clientThreads[i];
		}

		sendThread->join();	// waut for send thread to join

		std::cout << "Server Cleaned" << std::endl;
	}
};
