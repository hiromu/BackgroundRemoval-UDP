#pragma once

#include <vector>
#include <Windows.h>
#include <Windef.h>
#include <WinNT.h>

#include "ip/UdpSocket.h"
#include "png.h"

struct image
{
	char *buffer;
	size_t size;
};

class NetworkManager
{
	static const int        PORT = 51228;
public:
	NetworkManager(std::vector<std::string> args);
	~NetworkManager(void);

	void updateBuffer(int w, int h, int b, const BYTE *data);
	void Run(void);
	static DWORD WINAPI NetworkManager::ThreadFunc(LPVOID lpParameter);
	void NetworkManager::Terminate(void);

private:
	bool                             update, awake;
	int	                             width, height, bufferSize;
	BYTE                             *buffer;
	char                             *packet;
	std::vector<UdpTransmitSocket *> transmitSocket;
	
	void send(void);
};

