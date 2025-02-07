#pragma once
#include <string>
#include <sstream>
#include <vector>

constexpr auto DELIMITER = '^';;

// enum for seperating messages and information in data
enum NetInfoType
{
	Null,
	clientLeft,
	clientJoined,
	clientList,
	message
};

// convert enum NetInfoType to string
static std::string toString(NetInfoType _type) {
	switch (_type)
	{
	case Null: 			 return "Null";
	case clientLeft:	 return "Client Left";
	case clientJoined:	 return "Client Joined";
	case clientList:	 return "Client List";
	case message:		 return "Message";
	default:			 return "ERROR";
	}
}

// holds data for network information
struct NetInfo
{
	NetInfoType type;	// type of net work information
	std::string data;	// network information data

	NetInfo() :type(NetInfoType::Null), data("") {}
	NetInfo(NetInfoType _type, std::string _data) :type(_type), data(_data) {}

	// encode network information into a string
	// returns a string containing encoded data
	std::string encode() {
		return std::to_string(type) + DELIMITER + data;
	}
	// decode network information from string and store in this object
	// _data : encoded information in string
	bool decode(std::string _data) {
		int pos = _data.find(DELIMITER);
		if (pos != _data.npos)
		{
			type = (NetInfoType)std::stoi(_data.substr(0, pos));
			if (pos < _data.size())
				data = _data.substr(pos + 1, _data.size());
			else
				data = "";
			return true;
		}
		return false;
	}
};

// struct to store message and its header
struct Message
{
	int from;			// id of the sender
	int to;				// id of receiver
	std::string data;	// information

	Message() :from(0), to(0), data("") {};
	Message(int _from, int _to, std::string _data) : from(_from), to(_to), data(_data) {}

	// encode message to a string
	// returns encoded message in string format
	std::string encode() {

		std::string out = "";
		out += std::to_string(from) + DELIMITER;
		out += std::to_string(to) + DELIMITER;
		out += data;
		return out;
	};

	// decode message from string and store in this object
	// _data : message in string format
	// returns true if decoding is successful
	bool decode(const std::string _data) {

		std::stringstream st(_data);
		std::string in;
		if (std::getline(st, in, DELIMITER))
		{
			from = std::stoi(in);
			if (std::getline(st, in, DELIMITER))
			{
				to = std::stoi(in);
				getline(st, in);
				data = in;
				return true;
			}
		}
		return false;
	}
};

// struct to store user data
struct User {
	unsigned int id;	// unique user id
	std::string username;	// name of the user

	User() :id(-1), username("") {};
	User(unsigned int _id, std::string _name) :id(_id), username(_name) {}

	// encode into string
	// returns encoded data in string format
	std::string encode() {
		return std::to_string(id) + DELIMITER + username;
	}

	// decode from string and store in this object
	// _data : encoded data in string format
	bool decode(std::string _data) {
		int pos = _data.find(DELIMITER);
		if (pos != _data.npos)
		{
			id = std::stoi(_data.substr(0, pos));
			username = _data.substr(pos + 1, _data.size());
			return true;
		}
		return false;
	}
};

// data containing server context
struct ServerContext
{
	int myId;						// id of the user to be send to
	std::vector<User> clientList;	// list of existing users on the server

	ServerContext() :myId(-1), clientList(NULL) {}
	ServerContext(const int& _id, const std::vector<User>& _users) :myId(_id), clientList(_users) {}

	// encode into string
	// returns encoded data as string
	std::string encode() {
		std::string info = "";

		info += std::to_string(myId);
		for (auto& c : clientList)
			info += DELIMITER + c.encode();

		return info;
	}

	// decode from string and store in this object
	// _data : encoded data in string format
	bool decode(std::string _data) {
		std::stringstream st(_data);

		std::string in;
		if (std::getline(st, in, DELIMITER))
		{
			myId = std::stoi(in);							// decode myId

			while (std::getline(st, in, DELIMITER))			// read till end of the string
			{
				int id = std::stoi(in);						// decode id
				std::string username;
				std::getline(st, username, DELIMITER);			// decode name

				clientList.emplace_back(User(id, username));	// create and add user
			}
			return true;
		}
		return false;
	}
};

// stores client context data (just name for now)
struct ClientContext
{
	std::string username;	// name of the client

	ClientContext() :username("") {};
	ClientContext(std::string _name) :username(_name) {};

	std::string encode() { return username; }
	bool decode(std::string _data) { username = _data; return true; }
};