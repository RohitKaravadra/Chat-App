#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

const std::string NETWORK_EXIT = "!##!##!";

class SocketBase
{
protected:

	SOCKET socketID;							// int id of socket
	std::string ip;								// ip address of this socket

	bool valid;									// check if created
	std::atomic<bool> connected;				// check if connected or not (set manually in derived class)

	// default constructor
	SocketBase() = default;

	// bind server to a port
	// - returns true if bind successful
	bool bindServer(const unsigned int& port)
	{
		if (!valid)
			return false;

		sockaddr_in server_address = {};
		server_address.sin_family = AF_INET;			// Address family (IPv4 / IPv6)
		server_address.sin_port = htons(port);			// Server port
		server_address.sin_addr.s_addr = INADDR_ANY;	// Accept connections on any IP address

		// bind server with port
		if (bind(socketID, (sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
			std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
			return false;
		}

		return true;
	}

	// start listening to clients if bind is already done
	// - returns true if server is started
	bool startServer()
	{
		if (listen(socketID, SOMAXCONN) == SOCKET_ERROR) {
			std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
			return false;
		}

		std::cout << "Server is listening on port 65432..." << std::endl;
		return true;
	}

	// connect to a server
	// - host : ip address of server
	// - port : port number of server
	bool connectServer(std::string host, const unsigned int& port)
	{
		// check if socket is created or not
		if (!valid)
		{
			std::cout << "Client is not created." << std::endl;
			return false;
		}

		// Resolve the server address and port
		sockaddr_in server_address = {};
		server_address.sin_family = AF_INET;	//server_address.sin_port = htons(std::stoi(port));
		server_address.sin_port = htons(port);

		if (inet_pton(AF_INET, host.c_str(), &server_address.sin_addr) <= 0) {
			std::cerr << "Invalid address/ Address not supported" << std::endl;
			return false;
		}

		// Connect to the server
		if (connect(socketID, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) == SOCKET_ERROR) {
			std::cerr << "Connection failed with error: " << WSAGetLastError() << std::endl;
			return false;
		}

		std::cout << "Connected to the server." << std::endl;
		return true;
	}

	// accept client connection
	//- clientOut : SOCKET of the client connected
	//- ipOut : ip address of client connected
	// returns true if client connected successfully
	bool acceptClient(SOCKET& clientOut, std::string& ipOut)
	{
		// accept connection
		sockaddr_in client_address = {};
		int client_address_len = sizeof(client_address);
		SOCKET client_socket = accept(socketID, (sockaddr*)&client_address, &client_address_len);

		// check if connection accepted
		if (client_socket == INVALID_SOCKET) {
			std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
			return false;
		}

		// convert client ip to string
		char client_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);

		// set return data
		clientOut = client_socket;
		ipOut = std::string(client_ip);

		return true;
	}

public:

	// create Socket
	bool create()
	{
		// generate socket 
		socketID = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// validate socket
		if (socketID == INVALID_SOCKET) {
			std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
			return false;
		}

		std::cout << "Socket Created ( Socket Id : " << socketID << " )" << std::endl;

		// set valid and connected
		valid = true;
		connected = false;

		return true;
	}

	// destroy socket
	void destroy()
	{
		//check if created
		if (valid)
			std::cout << "Socket " << socketID << " Closed with code " << closesocket(socketID) << std::endl;

		// reset calid and connected
		valid = false;
		connected = false;
	}

	// destructor
	virtual ~SocketBase()
	{
		destroy();
	}
};

// Initialize WinSock
static bool InitWinSock()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}

// Clean WinSock
static void CleanWinSock()
{
	WSACleanup();
}

// send data from given socket
static bool sendData(SOCKET socketID, const char* info, const unsigned int& size)
{
	// Send the sentence to the server
	if (send(socketID, info, size, 0) == SOCKET_ERROR) {
		std::cerr << "Send failed with error: " << WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

// recieve data for given socket
static bool recvData(SOCKET socketID, const unsigned int& bufferSize, std::string& out)
{
	// Receive the reversed sentence from the server
	char* buffer = new char[bufferSize];
	int bytes_received = recv(socketID, buffer, bufferSize - 1, 0);

	if (bytes_received > 0)
	{
		buffer[bytes_received] = '\0'; // Null-terminate the received data

		out = std::string(buffer);
		delete[] buffer;

		return true;
	}

	if (bytes_received == 0) {
		std::cout << "Connection closed by server." << std::endl;
	}
	else {
		std::cerr << "Receive failed with error: " << WSAGetLastError() << std::endl;
	}

	delete[] buffer;
	return false;
}

// receive information for the socket
// - SOCKET : socket id of the socket
// - out : received information
static bool recvInfo(SOCKET _socketID, std::string& _out)
{
	std::string header;
	if (recvData(_socketID, 100, header))
	{
		unsigned int size = std::stoi(header);
		std::string info;
		if (recvData(_socketID, size + 1, info))
		{
			_out = info;
			return true;
		}
	}

	return false;
}

//send information for socker
// - SOCKET : socket id of socket
// - msg : information to be send
static bool sendInfo(SOCKET _socketID, std::string _msg)
{
	std::string size = std::to_string(_msg.size());
	if (sendData(_socketID, size.c_str(), size.size()))
		if (sendData(_socketID, _msg.c_str(), _msg.size()))
			return true;

	return false;
}