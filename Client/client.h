#pragma once
#include "Networking.h"
#include "NetworkData.h"
#include "MessageQueue.h"
#include <map>

struct UserData
{
	std::string username;
	std::string chat;
	int newMsgs;

	UserData() :username(""), chat(""), newMsgs(0) {}
	UserData(std::string _name, std::string _chat, bool _hasNewMsg = 0) :
		username(_name), chat(_chat), newMsgs(_hasNewMsg) {
	}
};

class Client :public SocketBase
{
	MsgQueue<std::string> sendQueue;

	std::thread* sendThread;		// sending thread
	std::thread* recvThread;		// recving thread

	std::map<int, UserData> userData;	// user data contains {userId, (username, userchat)}
	std::mutex mtx;						// mutex to protect userData

	int myId;						// this client id on server
public:
	std::string username;			// this client name

	Client() = default;

	// connect to server
	// - host : ip address of server
	// - port : port number of server
	// returns if connection is successfull or not
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

		std::cout << "Server Context Received" << std::endl;

		// decode server context
		ServerContext sc;
		if (!sc.decode(ctx))
		{
			std::cout << "Corrupted Server Context received" << std::endl;
			disconnect();
			return false;
		}

		// set data from server context
		myId = sc.myId;
		populateUsers(sc.clientList);

		// send client sontext
		ClientContext cc(username);				// prepare client context
		if (!sendInfo(socketID, cc.encode()))	// encode and send client context
		{
			std::cout << "Client context Not sent" << std::endl;
			disconnect();
			return false;
		}

		sendThread = new std::thread(&Client::sendInfoThread, this);	// start sending thread for this client
		recvThread = new std::thread(&Client::recvInfoThread, this);	// start receiving thread for this client

		return true;
	}

	// populate users received from server
	// _users : list of users
	void populateUsers(std::vector<User>& _users)
	{
		std::lock_guard<std::mutex> lock(mtx);							// critical section

		userData.insert({ 0, UserData("General", "") });
		for (auto& u : _users)
		{
			userData.insert({ u.id, UserData(u.username, "") });
			userData[0].chat += "\n\n" + u.username + " Joined :)\n";	// add join message to general chat
		}
	}

	// recieve info from server
	// _out : recieved info
	// return true if received successfully
	bool recv(std::string& _out) {
		return connected = recvInfo(socketID, _out);
	}

	// send message to server
	// msg : message to be send
	// to : id of user to send message
	// return true if send is successful
	bool sendMessage(std::string _msg, int to = 0)
	{
		// check if user exist or not
		if (to >= userData.size())
		{
			std::cout << "User Not Found. No message send to " << to << std::endl;
			return false;
		}

		mtx.lock();												// critical section begin

		// traverse to users to get current user
		auto sendTo = userData.begin();
		std::advance(sendTo, to);

		sendTo->second.chat += "\nYou  : " + _msg;				// add own message to chat
		Message data(myId, sendTo->first, _msg);				// prepare message

		mtx.unlock();											// critical section end

		NetInfo newInfo(NetInfoType::message, data.encode());	// encode message
		sendQueue.enqueue(newInfo.encode());					// add message to sending queue

		std::cout << "Added to Send Queue - " << _msg << std::endl;

		return true;
	}
	// callback to handle new user 
	// _info : network info of new user
	void onUserJoined(const NetInfo& _info)
	{
		User newUser;											// create user
		if (!newUser.decode(_info.data))						// decode user data
		{
			std::cout << "Client join information is corrupted" << std::endl;
			return;
		}

		if (newUser.id == myId)									// check if new user is me
			return;

		mtx.lock();												//critical section begin

		userData.insert({ newUser.id,UserData(newUser.username, "") });		// add new user to the user list

		userData[0].chat += "\n\n" +
			newUser.username + " Joined :)\n";					// add join message to general chat

		mtx.unlock();											// critical section end
	}

	// callback to handle user exit
	// _info : network info of user left
	void onUserExit(const NetInfo& _info)
	{
		User newUser;											// create user
		if (!newUser.decode(_info.data))						// decode user data
		{
			std::cout << "Client exit information is corrupted" << std::endl;
			return;
		}

		if (newUser.id == myId)
			return;

		mtx.lock();												// critical section begin

		userData.erase(newUser.id);								// remove user from userlist

		userData[0].chat += "\n\n" +
			newUser.username + " Left :(\n";					// add left message in general chat

		mtx.unlock();											// critical section end
	}

	// callback to handle message
	// _info : network information of the message received
	void onMsgRecvd(const NetInfo& _info)
	{
		Message msg;											// create message
		if (!msg.decode(_info.data))							// decode message
		{
			std::cout << "Message is corrupted" << std::endl;
			return;
		}

		if (msg.from == myId)									// check if i am the sender
			return;

		int chat = msg.to == 0 ? 0 : msg.from;					// find chat index in user list (if to is 0 that means its for general chat)
		userData[chat].chat += "\n" +
			userData[msg.from].username + "  : " + msg.data;	// add message to the chat 

		userData[chat].newMsgs += 1;							// add new message notification counter
	}

	// callback to handle received information
	// process information according to type
	// _info : received information
	void onInfoRecvd(std::string _info)
	{
		std::cout << _info << std::endl;
		NetInfo newInfo;										// create information
		if (!newInfo.decode(_info))								// decode information
		{
			std::cout << "Information received is corrupted" << std::endl;
			return;
		}

		std::cout << "Received -> " << toString(newInfo.type) << std::endl;

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

	// handles sending thread of this client
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

	// handles receiving thread of this client
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

	// returns chat for user index
	// i : index of the user
	std::string getChat(int i) {
		std::lock_guard<std::mutex> lock(mtx);					// critical section

		if (i < userData.size())
		{
			auto p = userData.begin();
			std::advance(p, i);
			p->second.newMsgs = 0;								// set new message notification counter 0
			return p->second.chat;
		}
		else
			return "";
	}

	// returns username for user index
	// i : index of the user
	std::string getUsername(int i) {
		std::lock_guard<std::mutex> lock(mtx);					// critical section

		if (i < userData.size())
		{
			auto p = userData.begin();
			std::advance(p, i);
			return p->second.username;
		}
		else
			return "Unknown " + std::to_string(i);
	}

	// returns username for user index
	// i : index of the user
	int getNewMsgCount(int i) {
		std::lock_guard<std::mutex> lock(mtx);					// critical section

		if (i < userData.size())
		{
			auto p = userData.begin();
			std::advance(p, i);
			return p->second.newMsgs;
		}
		else
			return 0;
	}

	// returns  total users connected to server
	int getTotalUsers() {
		std::lock_guard<std::mutex> lock(mtx);					// critical section
		return userData.size();
	}

	// disconnect from server
	// returns true if disconnected 
	bool disconnect()
	{
		if (connected)
		{
			sendInfo(socketID, NETWORK_EXIT);					// send disconnection message to server
			connected = false;
			return true;
		}

		return false;
	}

	// checks of client is connected or not
	bool isConnected() {
		return connected;
	}

	// destructor
	~Client()
	{
		if (sendThread != nullptr)
			sendThread->join();

		if (recvThread != nullptr)
			recvThread->join();

		delete sendThread, recvThread;
		std::cout << "\nClient Destroyed.." << std::endl;
	}
};