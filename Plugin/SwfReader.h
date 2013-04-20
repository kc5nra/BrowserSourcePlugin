#include <zlib.h>
#include <Windows.h>
#define BUFFER_SIZE 4096

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
	unsigned char* inflateBuffer(unsigned char* data, size_t size);
	int extractDimensions(const unsigned char* data);
		
public:
	SwfReader(unsigned char* buffer, size_t bufferSize = BUFFER_SIZE);
		
public:
	int getWidth();
	int getHeight();
	int getVersion();
	int getFileSize();
	bool hasError();
		
protected:
	__inline int le4CharToInt(const unsigned char* _in) {
		return 0 + (_in[3] << 8*3) + (_in[2] << 8*2) + (_in[1] << 8) + _in[0];
	}
};
