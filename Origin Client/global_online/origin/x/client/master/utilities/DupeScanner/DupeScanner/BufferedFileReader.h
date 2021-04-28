#ifndef BUFFEREDFILEREADER_H
#define BUFFEREDFILEREADER_H

#include <Windows.h>
#include <stdint.h>
#include <string>

using namespace std;

const int kDefaultBufferSize = 32*1024;
class BufferedFileReader
{
public:
    BufferedFileReader(uint32_t nBufferSize = kDefaultBufferSize);
    ~BufferedFileReader();

    bool Open(const wstring& sFilename);
    bool Read(uint8_t* pDestination, int32_t nReadSize, int32_t& nNumRead);
    bool Seek(int64_t nOffsetFromBeginning);
    bool Close();

private:
    uint8_t* mpBuffer;
    uint8_t* mpCurrentReadPosition;
    HANDLE   mhFile;
    uint32_t mnTotalBufferSize;
    int32_t mnBufferDataAvailable;
};

#endif