#include "client.h"

#define DEFAULT_BUFFER_SIZE 1024

using namespace Network;

int main() {
	const char* host = "127.0.0.1"; // Server IP address
	unsigned int port = 65432;
	std::string sentence = "Hello, server!";

	if (InitWinSock())
	{

		Client client;

		if (client.create())
		{
			if (client.connect(host, port))
			{
				bool running = true;
				while (running)
				{
					std::getline(std::cin, sentence);

					client.send(sentence);

					if (sentence == "!bye")
					{
						running = false;
						continue;
					}

					std::string msg;
					client.recv(DEFAULT_BUFFER_SIZE, msg);
					std::cout << "Received : " << msg << std::endl;
				}
			}

			client.clear();
		}
	}

	CleanWinSock();

	return 0;
}
