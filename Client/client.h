#pragma once
#include "Networking.h"
#include "MessageQueue.h"

class Client :public SocketBase
{
	MsgQueue sendQueue;

	std::thread* sendThread;
	std::thread* recvThread;

	std::mutex recvMtx;

	std::vector<User> users;
	std::vector<std::vector<std::string>> userChats;

	int myId;
public:
	std::string name;

	Client() = default;

	bool connect(std::string host, const unsigned int& port)
	{
		if (SocketBase::connectServer(host, port))
		{
			connected = true;

			if (setClientData())
			{
				sendThread = new std::thread(&Client::handleSendMessageThread, this);
				return true;
			}
		}
		return false;
	}

	bool setClientData()
	{
		if (sendMessage(socketID, name))
		{
			std::string msg;
			if (recvMessage(socketID, msg))
			{
				myId = std::stoi(msg);
				return true;
			}
		}
		return false;
	}

	bool send(std::string msg, int to = 0)
	{
		Message data(MessageType::message, myId, to, msg);
		encodeMessage(data, msg);
		std::cout << msg << std::endl;
		sendQueue.enqueue(msg);
		return true;
	}

	bool recv(std::string& out)
	{
		return connected = recvMessage(socketID, out);
	}

	void OnMessageRecvd(std::string msg)
	{
		std::cout << msg << std::endl;

		Message data;
		if (!decodeMessage(msg, data))
			return;

		std::lock_guard<std::mutex> lock(recvMtx);
		if (data.type == MessageType::message)
		{
			for (int i = 0; i < users.size(); i++)
				if (users[i].id == data.from)
					userChats[i].emplace_back(users[i].name + " : " + data.data);
		}
		else
		{

		}
	}

	void handleSendMessageThread()
	{
		std::string msg;
		while (connected)
		{
			if (sendQueue.dequeue(msg))
				connected = sendMessage(socketID, msg);
		}

		std::cout << "Send Thread Closed" << std::endl;
	}

	void handleRecvMessageThread()
	{
		while (connected)
		{
			std::string msg;
			if (recv(msg))
				OnMessageRecvd(msg);
		}

		std::cout << "Recieve Thread Closed" << std::endl;
	}

	std::vector<std::string> getChat(int i) {
		std::lock_guard<std::mutex> lock(recvMtx);
		return userChats[i];
	}

	User getUser(int i) {
		std::lock_guard<std::mutex> lock(recvMtx);
		return users[i];
	}

	int getTotalUsers() {
		std::lock_guard<std::mutex> lock(recvMtx);
		return users.size();
	}

	bool disconnect()
	{
		if (connected)
		{
			sendMessage(socketID, NETWORK_EXIT);
			connected = false;
			return true;
		}

		return false;
	}

	bool isConnected() {
		return connected;
	}

	~Client()
	{
		if (sendThread != nullptr)
			sendThread->join();

		if (recvThread != nullptr)
			recvThread->join();

		delete sendThread, recvThread;
		std::cout << "Client Destroyed" << std::endl;
	}
};