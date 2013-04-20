#pragma once

#include <zlib.h>
#include <Windows.h>
#define BUFFER_SIZE 4096

#define LE4CHAR_TO_INT(_in) (0 + ((_in)[3] << 8*3) + ((_in)[2] << 8*2) + ((_in)[1] << 8) + (_in)[0])

class SwfReader
{
	
private:
	int width;
	int height;
	int fileSize;
	int version;

	char f1, f2, f3;
	unsigned char* buffer;
	size_t bufferSize;
	bool error;
	
protected:
	unsigned char* InflateBuffer(unsigned char* data, size_t size);
	int ExtractDimensions(const unsigned char* data);
		
public:
	SwfReader(unsigned char* buffer, size_t bufferSize = BUFFER_SIZE);
		
public:
	int GetWidth();
	int GetHeight();
	int GetVersion();
	int GetFileSize();
	bool HasError();

};
