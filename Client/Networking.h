#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

namespace Network
{
	class SocketBase
	{
	public:

		SOCKET socketID;
		bool valid;

		SocketBase() = default;

		bool bindServer(const unsigned int& port)
		{
			if (!valid)
				return false;

			// Step 3: Bind the socket
			sockaddr_in server_address = {};
			server_address.sin_family = AF_INET;
			server_address.sin_port = htons(port);  // Server port
			server_address.sin_addr.s_addr = INADDR_ANY; // Accept connections on any IP address

			if (bind(socketID, (sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
				std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
				return false;
			}

			return true;
		}

		bool startServer()
		{
			// Step 4: Listen for incoming connections
			if (listen(socketID, SOMAXCONN) == SOCKET_ERROR) {
				std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
				return false;
			}

			std::cout << "Server is listening on port 65432..." << std::endl;
			return true;
		}

		bool connectServer(std::string host, const unsigned int& port)
		{
			if (!valid)
			{
				std::cout << "Client is not created." << std::endl;
				return false;
			}

			// Resolve the server address and port
			sockaddr_in server_address = {};
			server_address.sin_family = AF_INET;
			//server_address.sin_port = htons(std::stoi(port));
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

		bool acceptClient(SOCKET& clientOut, std::string& ipOut)
		{
			sockaddr_in client_address = {};
			int client_address_len = sizeof(client_address);
			SOCKET client_socket = accept(socketID, (sockaddr*)&client_address, &client_address_len);
			if (client_socket == INVALID_SOCKET) {
				std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
				return false;
			}

			char client_ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
			std::cout << "Accepted connection from " << client_ip << ":" << ntohs(client_address.sin_port) << std::endl;

			clientOut = client_socket;
			ipOut = std::string(client_ip);
			return true;
		}

		bool create()
		{
			socketID = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (socketID == INVALID_SOCKET) {
				std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
				return false;
			}

			std::cout << "Socket Created ( Socket Id : " << socketID << " )" << std::endl;
			valid = true;
			return true;
		}

		void clear()
		{
			if (valid)
				std::cout << "Socket " << socketID << " Closed with code " << closesocket(socketID) << std::endl;
			valid = false;
		}

		virtual ~SocketBase()
		{
			clear();
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

	// send message from given socket
	static bool sendMessage(SOCKET socketID, std::string msg)
	{
		// Send the sentence to the server
		if (send(socketID, msg.c_str(), static_cast<int>(msg.size()), 0) == SOCKET_ERROR) {
			std::cerr << "Send failed with error: " << WSAGetLastError() << std::endl;
			return false;
		}

		std::cout << "Sent: " << msg << std::endl;
		return true;
	}

	// recieve message for given socket
	static bool recvMessage(SOCKET socketID, const unsigned int& bufferSize, std::string& out)
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
}