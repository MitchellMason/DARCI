#pragma once
class NetClient
{
public:
	NetClient();
	~NetClient();
	void start();
private:
	void run();

	int port;
	char *addr;
	bool running = false;
};

