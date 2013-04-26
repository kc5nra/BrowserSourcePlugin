#include "swfreader.h"

SwfReader::SwfReader(unsigned char* buffer, size_t bufferSize)
{
    this->buffer = buffer;
    this->bufferSize = bufferSize;

    error = false;
    width = 0;
    height = 0;
    fileSize = 0;
    version = 0;

    f1 = buffer[0];
    f2 = buffer[1];
    f3 = buffer[2];
    version = buffer[3];

    fileSize = LE4CHAR_TO_INT(buffer+4);

    if(f1 == 'C') {
        unsigned char* data = InflateBuffer(buffer+8, bufferSize-8);
        if (!error) {
            ExtractDimensions(data);
        }
        free(data);
    } else {
        ExtractDimensions(buffer+8);
    }
}

unsigned char* SwfReader::InflateBuffer(unsigned char* data, size_t size)
{
    unsigned char* result = (unsigned char*) malloc(sizeof(unsigned char*) * fileSize);

    int ret;
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
    {
        error = true;
        return result;
    }

    strm.next_in = data;
    strm.avail_in = size;
    if(strm.avail_in == 0)
    {
        error = true;
        return result;
    }

    strm.next_out = result;
    strm.avail_out = fileSize;
    ret = inflate(&strm, Z_NO_FLUSH);
    switch (ret)
    {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR: 
        {
            (void)inflateEnd(&strm);
            error = true;
            return result;
        }
    }

    (void)inflateEnd(&strm);

    return result;
}

int SwfReader::ExtractDimensions(const unsigned char* data)
{
    unsigned char nbits = data[0];
    unsigned int size = nbits >> 3;

    unsigned long* dims = (unsigned long*) malloc(4 * sizeof(unsigned long));
    unsigned long neg_root = 1 << (size-1);

    unsigned int biOffset = (size % 8) ? (8 - (size % 8)) : 0;
    unsigned int byOffset = (size + biOffset) / 8;
    unsigned int ioffset;
    unsigned long ibuf = (unsigned long) (nbits % 8);

    for(unsigned int i = 0; i<4; i++)
    {
        ioffset = byOffset * i;

        for(unsigned int j = 0; j < byOffset; j++)
        {
            ibuf <<= 8;
            ibuf += data[1+ioffset+j];
        }
        dims[i] = (ibuf >> (3 + byOffset + (i * biOffset))) / 20;
        if(dims[i] >= neg_root)
        {
            dims[i] = (-1) * (neg_root - (dims[i] - neg_root));
        }
        int expn = 3 + biOffset + (i * biOffset);
        ibuf = ibuf % (1 << (expn-1));
    }

    width = dims[1] - dims[0];
    height = dims[3] - dims[2];

    free(dims);

    return 0;
}

int SwfReader::GetWidth()
{
    return width;
}

int SwfReader::GetHeight()
{
    return height;
}

int SwfReader::GetVersion()
{
    return version;
}

int SwfReader::GetFileSize()
{
    return fileSize;
}

bool SwfReader::HasError() {
    return error;
}


