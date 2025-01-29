#pragma once
#include "Networking.h"
#include "NetworkData.h"
#include "MessageQueue.h"
#include <unordered_map>

class Client :public SocketBase
{
	MsgQueue<std::string> sendQueue;

	std::thread* sendThread;	// sending thread
	std::thread* recvThread;	// recving thread

	std::unordered_map<int,
		std::pair<std::string, std::string>> userData;	// user data contains <userId, <username, userchat>>
	std::mutex mtx;										// mutex to protect userData

	int myId;					// this client id on server
public:
	std::string name;			// this client name

	Client() = default;

	// connect to server
	// - host : ip address of server
	// - port : port number of server
	bool connect(std::string host, const unsigned int& port)
	{
		if (!SocketBase::connectServer(host, port))
			return false;

		connected = true;

		std::string ctx;

		// receive server context
		if (!recvInfo(socketID, ctx))
		{
			std::cout << "Server Context failed" << std::endl;
			disconnect();
			return false;
		}

		std::cout << "Server Context :- " << ctx << std::endl;

		// decode server context
		ServerContext sc;
		if (!sc.decode(ctx))
		{
			std::cout << "Corrupted Server Context received" << std::endl;
			disconnect();
			return false;
		}

		myId = sc.myId;
		populateUsers(sc.clientList);

		// send client sontext
		ClientContext cc(name);
		if (!sendInfo(socketID, cc.encode()))
		{
			std::cout << "Client context Not sent" << std::endl;
			disconnect();
			return false;
		}

		sendThread = new std::thread(&Client::sendInfoThread, this);
		recvThread = new std::thread(&Client::recvInfoThread, this);

		return true;
	}

	void populateUsers(std::vector<User>& users)
	{
		mtx.lock();

		userData.insert({ 0, std::make_pair("General", "") });
		for (auto& u : users)
		{
			std::cout << u.name << std::endl;
			userData.insert({ u.id, std::make_pair(u.name, "") });
		}

		mtx.unlock();
	}

	// recieve info from server
	// out : recieved info
	// return true if received successfully
	bool recv(std::string& _out) {
		return connected = recvInfo(socketID, _out);
	}

	// send message to server
	// msg : message to be send
	// to : id of user to send message
	// return true if send is successful
	bool sendMessage(std::string msg, int to = 0)
	{
		if (to >= userData.size())
		{
			std::cout << "User Not Found. No message send to " << to << std::endl;
			return false;
		}

		mtx.lock();

		auto sendTo = userData.begin();
		std::advance(sendTo, to);
		sendTo->second.second += "\nYou : " + msg; // add own message to chat
		Message data(myId, sendTo->first, msg);

		mtx.unlock();

		NetInfo newInfo(NetInfoType::message, data.encode());
		sendQueue.enqueue(newInfo.encode());

		std::cout << "Added to Send Queue - " << newInfo.data << std::endl;

		return true;
	}

	void onUserJoined(const NetInfo& _info)
	{
		User newUser;
		if (!newUser.decode(_info.data))
		{
			std::cout << "Client join information is corrupted" << std::endl;
			return;
		}

		if (newUser.id == myId)
			return;

		mtx.lock();
		userData.insert({ newUser.id, std::make_pair(newUser.name, "") });
		userData[0].second += "\n" + newUser.name + " Joined :)";
		mtx.unlock();
	}

	void onUserExit(const NetInfo& _info)
	{
		User newUser;
		if (!newUser.decode(_info.data))
		{
			std::cout << "Client exit information is corrupted" << std::endl;
			return;
		}

		if (newUser.id == myId)
			return;

		mtx.lock();
		userData.erase(newUser.id);
		userData[0].second += "\n" + newUser.name + " Left :(";
		mtx.unlock();
	}

	void onMsgRecvd(const NetInfo& _info)
	{
		Message msg;
		if (!msg.decode(_info.data))
		{
			std::cout << "Message is corrupted" << std::endl;
			return;
		}

		if (msg.from == myId)
			return;

		int chat = msg.to == 0 ? 0 : msg.from;
		userData[chat].second += "\n" + userData[msg.from].first + " : " + msg.data;
	}

	// callback when a information is received
	// process information according to type
	// info : received information
	void onInfoRecvd(std::string _info)
	{
		std::cout << _info << std::endl;
		NetInfo newInfo;
		if (!newInfo.decode(_info))
		{
			std::cout << "Information received is corrupted" << std::endl;
			return;
		}

		std::cout << "Received -> " << _info << std::endl;

		switch (newInfo.type)
		{
		case NetInfoType::clientJoined: onUserJoined(newInfo);
			break;
		case NetInfoType::clientLeft: onUserExit(newInfo);
			break;
		case NetInfoType::message: onMsgRecvd(newInfo);
			break;
		}
	}

	void sendInfoThread()
	{
		std::string msg;
		while (connected)
		{
			if (sendQueue.dequeue(msg))
				connected = sendInfo(socketID, msg);
		}

		std::cout << "Send Thread Closed" << std::endl;
	}

	void recvInfoThread()
	{
		std::string msg;
		while (connected)
		{
			if (recv(msg))
				onInfoRecvd(msg);
		}

		std::cout << "Recieve Thread Closed" << std::endl;
	}

	std::pair<std::string, std::string> getData(int i) {
		std::lock_guard<std::mutex> lock(mtx);

		if (i < userData.size())
		{
			auto p = userData.begin();
			std::advance(p, i);
			return p->second;
		}
		else
			return std::make_pair("Unknown", "");
	}

	int getTotalUsers() {
		std::lock_guard<std::mutex> lock(mtx);
		return userData.size();
	}

	bool disconnect()
	{
		if (connected)
		{
			sendInfo(socketID, NETWORK_EXIT);
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