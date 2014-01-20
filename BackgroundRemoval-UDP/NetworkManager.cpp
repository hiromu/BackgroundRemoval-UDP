#include "NetworkManager.h"

#include <WinCrypt.h>

void png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	struct image* p=(struct image*)png_get_io_ptr(png_ptr);
	size_t nsize = p->size + length;

	p->buffer = (char *)realloc(p->buffer, nsize);
	if(!p->buffer)
		png_error(png_ptr, "Write Error");

	memcpy(p->buffer + p->size, data, length);
	p->size += length;
}

NetworkManager::NetworkManager(std::vector<std::string> args)
{
	bufferSize = 0;
	buffer = NULL;
	packet = NULL;
	awake = true;
	update = false;

	for(unsigned int i = 0; i < args.size(); i++)
		transmitSocket.push_back(new UdpTransmitSocket(IpEndpointName(args[i].c_str(), PORT)));
}


NetworkManager::~NetworkManager(void)
{
	while(awake || update)
		continue;
	free(buffer);
	free(packet);

	for(unsigned int i = 0; i < transmitSocket.size(); i++)
		delete transmitSocket[i];
}

void NetworkManager::updateBuffer(int w, int h, int b, const BYTE *data)
{
	if(b != 4)
		return;

	width = w, height = h;
	int dataLength = width * height * 4;

	if(bufferSize < dataLength) {
		buffer = (BYTE *)realloc(buffer, sizeof(BYTE) * dataLength);
		packet = (char *)realloc(packet, sizeof(char) * dataLength * 2);
	}
	bufferSize = dataLength;

	memcpy(buffer, data, sizeof(BYTE) * bufferSize);

	update = true;
}

void NetworkManager::send(void)
{
	if(!update)
		return;

	struct image state;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytepp rows;

	state.buffer = NULL;
	state.size = 0;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);

	rows = (png_bytepp)png_malloc(png_ptr, sizeof(png_bytep) * height);
	for(int i = 0; i < height; i++) {
		rows[i] = (png_bytep)png_malloc(png_ptr, sizeof(png_byte) * width);
		for(int j = 0; j < width; j++)
			rows[i][j] = buffer[i * width * 4 + j * 4 + 3];
	}

	update = false;

	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_set_write_fn(png_ptr, &state, png_write_data, NULL);
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, rows);
	png_write_end(png_ptr, info_ptr);

	for(int i = 0; i < height; i++)
		png_free(png_ptr, rows[i]);
	png_free(png_ptr, rows);
	png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

	char *packet = (char *)malloc(state.size + sizeof(char) * 2);
	packet[0] = state.size % 256;
	packet[1] = state.size / 256;

	memcpy(packet + 2, state.buffer, state.size);
	for(unsigned int i = 0; i < transmitSocket.size(); i++)
		transmitSocket[i]->Send(packet, state.size + sizeof(char) * 2);

	free(state.buffer);
	free(packet);
}

void NetworkManager::Run(void)
{
	while(awake)
		send();
	update = false;
}

DWORD WINAPI NetworkManager::ThreadFunc(LPVOID lpParameter) 
{
	((NetworkManager *)lpParameter)->Run();
	return 0;
}

void NetworkManager::Terminate(void)
{
	awake = false;
}