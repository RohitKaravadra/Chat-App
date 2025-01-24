
#include "server.h"
#include <thread>

using namespace Network;

constexpr int BUFFER_SIZE = 1024;

void handleClient(SOCKET client_socket)
{
	bool running = true;
	while (running)
	{
		std::string msg;

		if (Network::recvMessage(client_socket, BUFFER_SIZE, msg))
		{
			if (msg == "!bye")
			{
				running = false;
				continue;
			}

			std::reverse(msg.begin(), msg.end());

			Network::sendMessage(client_socket, msg);
		}
	}

	// Step 7: Clean up
	closesocket(client_socket);
}

int main()
{
	if (InitWinSock())
	{
		Server server;
		if (server.create())
		{
			if (server.bind(65432))
			{
				if (server.start())
				{
					while (true)
					{
						SOCKET client;
						std::string ip;

						if (server.accept(client, ip))
							std::thread* clientT = new std::thread(handleClient, client);
					}
				}
			}

			server.clear();
		}
	}

	CleanWinSock();

	return 0;
}