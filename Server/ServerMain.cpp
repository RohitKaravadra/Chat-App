
#include "server.h"
#include <thread>

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
						server.accept();
				}
			}

			server.destroy();
		}
	}

	CleanWinSock();

	return 0;
}