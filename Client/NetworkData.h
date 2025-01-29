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

struct NetInfo
{
	NetInfoType type;
	std::string data;

	NetInfo() :type(NetInfoType::Null), data("") {}
	NetInfo(NetInfoType _type, std::string _data) :type(_type), data(_data) {}

	std::string encode() {
		return std::to_string(type) + DELIMITER + data;
	}

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
	int from;
	int to;
	std::string data;

	Message() :from(0), to(0), data("") {};
	Message(int _from, int _to, std::string _data) : from(_from), to(_to), data(_data) {}

	// encode message to a string
	// msg : messsage to be encoded
	// out : encoded result in string
	std::string encode() {

		std::string out = "";
		out += std::to_string(from) + DELIMITER;
		out += std::to_string(to) + DELIMITER;
		out += data;
		return out;
	};

	// decode message from string
	// data : message in string format
	// msg : decoded message
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
	unsigned int id;
	std::string name;

	User() :id(-1), name("") {};
	User(unsigned int _id, std::string _name) :id(_id), name(_name) {}

	// encode into string
	std::string encode() {
		return std::to_string(id) + DELIMITER + name;
	}

	// decode from string
	bool decode(std::string _data) {
		int pos = _data.find(DELIMITER);
		if (pos != _data.npos)
		{
			id = std::stoi(_data.substr(0, pos));
			name = _data.substr(pos + 1, _data.size());
			return true;
		}
		return false;
	}
};

struct ServerContext
{
	int myId;
	std::vector<User> clientList;

	ServerContext() :myId(-1), clientList(NULL) {}
	ServerContext(const int& _id, const std::vector<User>& _users) :myId(_id), clientList(_users) {}

	// encode into string
	std::string encode() {
		std::string info = "";

		info += std::to_string(myId);
		for (auto& c : clientList)
			info += DELIMITER + c.encode();

		return info;
	}

	// decode from string
	bool decode(std::string _data) {
		std::stringstream st(_data);

		std::string in;
		if (std::getline(st, in, DELIMITER))
		{
			myId = std::stoi(in);

			while (std::getline(st, in, DELIMITER)) // read till end of the string
			{
				int id = std::stoi(in);		// decode id
				std::string name;
				std::getline(st, name, DELIMITER);

				clientList.emplace_back(User(id, name));	// add user
			}
			return true;
		}
		return false;
	}
};

struct ClientContext
{
	std::string name;

	ClientContext() :name("") {};
	ClientContext(std::string _name) :name(_name) {};

	std::string encode() { return name; }
	bool decode(std::string _data) { name = _data; return true; }
};