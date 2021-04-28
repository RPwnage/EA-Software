#include "BufferedFileReader.h"

BufferedFileReader::BufferedFileReader(uint32_t nBufferSize)
{
    mnTotalBufferSize = nBufferSize;
    mpBuffer = new uint8_t[mnTotalBufferSize];
    mhFile = INVALID_HANDLE_VALUE;
    mnBufferDataAvailable = 0;
    mpCurrentReadPosition = NULL;
}

BufferedFileReader::~BufferedFileReader()
{
    if (mhFile)
        CloseHandle(mhFile);

    delete[] mpBuffer;
    mpBuffer = NULL;
    mnBufferDataAvailable = 0;
    mnTotalBufferSize = 0;
}

bool BufferedFileReader::Open(const wstring& sFilename)
{
    mhFile = CreateFile(sFilename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    mnBufferDataAvailable = 0;
    return mhFile != INVALID_HANDLE_VALUE;
}

bool BufferedFileReader::Read(uint8_t* pDestination, int32_t nReadSize, int32_t& nNumRead)
{
    if (nReadSize <= mnBufferDataAvailable)
    {
        memcpy(pDestination, mpCurrentReadPosition, nReadSize);
        mpCurrentReadPosition += nReadSize;
        mnBufferDataAvailable -= nReadSize;
        nNumRead = nReadSize;

        return true;
    }

    // copy over what we have
    if (mnBufferDataAvailable > 0)
        memcpy(pDestination, mpCurrentReadPosition, mnBufferDataAvailable);

    // calculate how much more we need
    uint32_t nBytesRequestedRemaining = nReadSize - mnBufferDataAvailable;

    // read directly into destination
    DWORD nBytesReadThisTime = 0;
    ::ReadFile(mhFile, pDestination + mnBufferDataAvailable, nBytesRequestedRemaining, &nBytesReadThisTime, NULL);
    
    nNumRead = mnBufferDataAvailable + nBytesReadThisTime;  // returned bytes read 

    // If there's more data in the file
    ::ReadFile(mhFile, mpBuffer, mnTotalBufferSize, &nBytesReadThisTime, NULL);
    mnBufferDataAvailable = nBytesReadThisTime;
    mpCurrentReadPosition = mpBuffer;

    return nNumRead > 0;
}

bool BufferedFileReader::Seek(int64_t nOffsetFromBeginning)
{
    LONG nLowBits = (LONG) (nOffsetFromBeginning & 0x00000000ffffffff);
    LONG nHighBits = (LONG) (nOffsetFromBeginning >> 32);
    mnBufferDataAvailable = 0;
    return ::SetFilePointer(mhFile, nLowBits, &nHighBits, FILE_BEGIN) == nLowBits;
}


bool BufferedFileReader::Close()
{
    CloseHandle(mhFile);
    mhFile = INVALID_HANDLE_VALUE;
    mnBufferDataAvailable = 0;

    return true;
}

