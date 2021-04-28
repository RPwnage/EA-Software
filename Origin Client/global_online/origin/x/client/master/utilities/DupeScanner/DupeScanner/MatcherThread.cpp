#include "MatcherThread.h"
#include "BlockScanner.h"
#include "BufferedFileReader.h"

extern void CalcMD5FromCircularBuffer(uint8_t* pCircularBuff, int32_t nCircularBufferSize, int32_t nCircularIndex, MD5Val& val);


const int32_t kReaderBlockSize = 1024*1024;  // 1024k file IO per thread

MatcherThread::MatcherThread(BlockScanner* pScanner, int32_t nBlockSize)
{
    mnStatus = kNone;
    mpBlockScanner = pScanner;
    mnBlockSize = nBlockSize;
    mnStartOffset = -1;
    mnBytesToScan = -1;

    mpReader = new BufferedFileReader(kReaderBlockSize);

    mhThread = INVALID_HANDLE_VALUE;

    mpCircularBuff = new uint8_t[nBlockSize]();
    mnCircularIndex = 0;
}

MatcherThread::~MatcherThread()
{
    if (mhThread != 0)
        CloseHandle(mhThread);

    delete mpReader;
    delete[] mpCircularBuff;
}

void MatcherThread::FindMatches(const wchar_t* pPath, int64_t nStartOffset, int64_t nBytesToScan, wostream* pOutStream)
{
    mnStatus = kMatching;
    mnStartOffset = nStartOffset;
    mnBytesToScan = nBytesToScan;
    mpPath = pPath;
    mpOutStream = pOutStream;

    if (mpReader->Open(std::wstring(mpPath)) == false)
    {
        *(mpOutStream) << "Can't open file for matching: " << mpPath << "!\n";
        mnStatus = kError;
        return;
    }

    if (mpReader->Seek(mnStartOffset) == false)
    {
        *(mpOutStream) << "Can't set read position: " << mpPath << "! position:" << mnStartOffset << "\n";
        mnStatus = kError;
        mpReader->Close();
        return;
    }

    // Read the initial block
    int32_t nNumRead = 0;
    if (mpReader->Read(mpCircularBuff, mnBlockSize, nNumRead) == false)
    {
        mnStatus = kError;
        mpReader->Close();
        return;
    }

    if (nNumRead < mnBlockSize)
    {
        // Don't have at least one block size to chekc
        mnStatus = kFinished;
        mpReader->Close();
        return;
    }

    if (mhThread)
        CloseHandle(mhThread);

    mhThread = CreateThread(NULL, 0, MatcherThread::MatchProc, this, 0, &mThreadID);
}

//#define VALIDATE_EVERY_ROLLING_CHECKSUM
#ifdef VALIDATE_EVERY_ROLLING_CHECKSUM
uint32_t GetChecksumFromFile(const wchar_t* pPath, int64_t nOffset, int32_t nBlockSize)
{
    HANDLE hFile = CreateFile(pPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    DWORD nNumRead = 0;
    if (hFile != INVALID_HANDLE_VALUE)
    {
        uint8_t* pBuf = new uint8_t[nBlockSize];


        LONG nLowBits = (LONG) (nOffset & 0x00000000ffffffff);
        LONG nHighBits = (LONG) (nOffset >> 32);
        ::SetFilePointer(hFile, nLowBits, &nHighBits, FILE_BEGIN);

        ReadFile(hFile, pBuf, nBlockSize, &nNumRead, NULL);

        uint32_t nChecksum = BlockScanner::GetRollingChecksum(pBuf, nBlockSize);

        delete[] pBuf;

        return nChecksum;
    }

    return 0;
}

#endif

DWORD WINAPI MatcherThread::MatchProc(LPVOID lpParameter)
{
    MatcherThread* pMatcher = (MatcherThread*) lpParameter;

    int32_t nBlockSize = pMatcher->mnBlockSize;
    int32_t nBlockSizeLog2 = pMatcher->mpBlockScanner->mnBlockSizeLog2;
    int64_t nBytesToScan = pMatcher->mnBytesToScan;

    BlockDescription matchBlock;
    matchBlock.mnOffset = pMatcher->mnStartOffset;
    matchBlock.mpPath = pMatcher->mpPath;


    bool bDone = false;
    matchBlock.mRollingChecksum = pMatcher->mpBlockScanner->GetRollingChecksum(pMatcher->mpCircularBuff, nBlockSize);

    while (!bDone)
    {
        bool bMatchFound = false;

        tChecksumToMD5BlockMap::iterator MD5blockMapIt = pMatcher->mpBlockScanner->mChecksumToMD5BlockMap.find(matchBlock.mRollingChecksum);
        if (MD5blockMapIt != pMatcher->mpBlockScanner->mChecksumToMD5BlockMap.end())
        {
            // Found a match for the checksum.

            // Now that the checksums match, calculate the MD5
            CalcMD5FromCircularBuffer(pMatcher->mpCircularBuff, nBlockSize, pMatcher->mnCircularIndex, matchBlock.mMD5);

            tMd5ToBlockMap& blockMap = (*MD5blockMapIt).second;

            tMd5ToBlockMap::iterator blockIt = blockMap.find(matchBlock.mMD5);

            if (blockIt != blockMap.end())
            {
                const BlockDescription& block = (*blockIt).second;

                // Exclude if this is the same file and the same offset
                if (!(block == matchBlock))
                {
                    pMatcher->mpBlockScanner->AddMatch(matchBlock.mMD5, BlockLocation(matchBlock.mpPath, matchBlock.mnOffset));
                    pMatcher->mpBlockScanner->AddMatch(block.mMD5, BlockLocation(block.mpPath, block.mnOffset));

                    // Output this only once if any matches were found
                    bMatchFound = true;
#ifdef _DEBUG
                    *(pMatcher->mpOutStream) << "\nblock:" << matchBlock << " First 8 bytes of data: \"";
                    for (int i = pMatcher->mnCircularIndex; i < pMatcher->mnCircularIndex+8; i++)
                        *(pMatcher->mpOutStream) << (wchar_t) pMatcher->mpCircularBuff[(i%nBlockSize)];
                    *(pMatcher->mpOutStream) << "\"\n";

                    *(pMatcher->mpOutStream) << "found in file:" << block.mpPath << " offset:" << block.mnOffset << "\n";
#endif
                }
            }

        }

        if (bMatchFound)
        {
            // Skip ahead by a block size
            pMatcher->mnCircularIndex = 0;
            int32_t nNumRead = 0;
            if (pMatcher->mpReader->Read(pMatcher->mpCircularBuff, nBlockSize, nNumRead) == false)
            {
                // no more to read
                bDone = true;
            }
            else
            {
                matchBlock.mnOffset += nNumRead;
                nBytesToScan -= nNumRead;
                matchBlock.mRollingChecksum = pMatcher->mpBlockScanner->GetRollingChecksum(pMatcher->mpCircularBuff, nNumRead);
            }
        }
        else 
        {
            pMatcher->mpBlockScanner->mnUniqueBlocks++;

            uint8_t nDropByte = *(pMatcher->mpCircularBuff+pMatcher->mnCircularIndex);

            int32_t nNumRead = 0;
            if (pMatcher->mpReader->Read(pMatcher->mpCircularBuff+pMatcher->mnCircularIndex, 1, nNumRead) == false)
            {
                // no more to read
                bDone = true;
            }
            else
            {
                matchBlock.mRollingChecksum = pMatcher->mpBlockScanner->UpdateRollingChecksum(matchBlock.mRollingChecksum, nDropByte, *(pMatcher->mpCircularBuff+pMatcher->mnCircularIndex), nBlockSizeLog2);
                matchBlock.mnOffset++;

                pMatcher->mnCircularIndex = (pMatcher->mnCircularIndex+1)%nBlockSize;        // advance the index

                pMatcher->mpBlockScanner->mnTotalOffsetsCompared++;
                nBytesToScan--;


#ifdef VALIDATE_EVERY_ROLLING_CHECKSUM
//               if (matchBlock.mnOffset > 1000*1024)
               {
                    uint32_t nChecksum = GetChecksumFromFile(pMatcher->mpPath, matchBlock.mnOffset, pMatcher->mnBlockSize);
                    if (nChecksum != matchBlock.mRollingChecksum)
                    {
                        int x = 5;
                    }
               }
#endif
            }
        }

        if (nBytesToScan <= 0)
            bDone = true;

    };

    pMatcher->mpReader->Close();
    pMatcher->mnStatus = kFinished;
    return 0;
}
