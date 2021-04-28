#include "BlockScanner.h"
#include <list>
#include <iostream>
#include <string>
#include <openssl/md5.h>
#include "BufferedFileReader.h"
#include <cmath>

using namespace std;

MD5Val::MD5Val(uint8_t* pVal)
{
    if (pVal)
    {
        memcpy(&mVal[0], pVal, MD5_DIGEST_LENGTH);
        return;
    }

    memset(&mVal[0], 0, MD5_DIGEST_LENGTH);
}

MD5Val::MD5Val(const MD5Val& rhs)
{
    memcpy(&mVal[0], &rhs.mVal[0], MD5_DIGEST_LENGTH);
}


BlockDescription::BlockDescription() : mRollingChecksum(0), mpPath(NULL), mnOffset(-1)
{
}

wostream& operator << (wostream& out, const BlockDescription& rhs)
{
    out << "file:" << rhs.mpPath << " ";
    out << "offset:" << rhs.mnOffset << " ";
    out << "checksum:" << rhs.mRollingChecksum << " ";
    out << rhs.mMD5;

    return out;
}

wostream& operator << (wostream& out, const MD5Val& rhs)
{
    out << "[";
    for (int i = 0; i < MD5_DIGEST_LENGTH-1; i++)
        out << /*std::hex <<*/ rhs.mVal[i] << ",";
    out << rhs.mVal[MD5_DIGEST_LENGTH-1] << "]";

    return out;
}


BlockScanner::BlockScanner()
{
    mnStatus = kNone;
    mbCancel = false;

    mhThread = NULL;
    mThreadID = 0;

    mnNumBlocks = 0;
    mnTotalFiles = 0;
    mnTotalFolders = 0;
    mnTotalBytesToProcess = 0;
    mnTotalOffsetsCompared = 0;
    mnUniqueBlocks = 0;
    mStartScanTimestamp = 0;
    mEndScanTimestamp = 0;
    mpOutStream = NULL;

    InitializeCriticalSection(&mMatchCriticalSection);

    // instantiate the matcher threads
    for (int32_t i = 0; i < kMaxMatcherThreads; i++)
        mMatcher[i] = NULL;
}

BlockScanner::~BlockScanner()
{
    for (int32_t i = 0; i < kMaxMatcherThreads; i++)
    {
        while (mMatcher[i] && mMatcher[i]->mnStatus == kMatching)
        {
            Sleep(0);
        };

        delete mMatcher[i];
        mMatcher[i] = NULL;
    }
    DeleteCriticalSection(&mMatchCriticalSection);
}

void CalcMD5FromCircularBuffer(uint8_t* pCircularBuff, int32_t nCircularBufferSize, int32_t nCircularIndex, MD5Val& val)
{
    MD5_CTX context;
    MD5_Init(&context);
    int32_t nBytesAfterIndex = nCircularBufferSize-nCircularIndex;
    int32_t nBytesBeforeIndex = nCircularIndex;

    if (nBytesAfterIndex > 0)
        MD5_Update(&context, pCircularBuff+nCircularIndex, nBytesAfterIndex);
    if (nBytesBeforeIndex > 0)
        MD5_Update(&context, pCircularBuff, nBytesBeforeIndex);

    MD5_Final(&(val[0]), &context);
}


void BlockScanner::Scan(wstring rootPath, int32_t nBlockSize, wostream* outStream)
{
    msRootPath = rootPath;
    mnStatus = BlockScanner::kScanning;
    mnBlockSize = nBlockSize;
    mnBlockSizeLog2 = (int32_t) (log((double) mnBlockSize)/log((double) 2));
    mpOutStream = outStream;

    // instantiate the matcher threads
    for (int32_t i = 0; i < kMaxMatcherThreads; i++)
        mMatcher[i] = new MatcherThread(this, mnBlockSize);

    mhThread = CreateThread(NULL, 0, BlockScanner::ScanProc, this, 0, &mThreadID);
}


void BlockScanner::FillError(BlockScanner* pScanner)
{
    WCHAR* pError   = NULL;
    DWORD  dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (WCHAR*)&pError, 0, NULL);
    if(dwResult)
    {
        pScanner->msError  = pError;
        LocalFree(pError);
    }
    pScanner->mnStatus = BlockScanner::kError;
}

void BlockScanner::Cancel()
{
    mbCancel = true;
    while (mnStatus == BlockScanner::kScanning)
    {
        Sleep(50);
    }
    mnStatus = kCancelled;
}

void BlockScanner::ScanFile(const wstring& sFilename)
{
    BufferedFileReader reader(16*1024);

    *mpOutStream << "Scanning: " << sFilename << "\n";

    if (!reader.Open(sFilename))
    {
        *mpOutStream << "Can't open file for scanning: " << sFilename.c_str() << "!\n";
        return;
    }

    bool bDone = false;
    uint64_t nOffset = 0;
    uint8_t* pBuff = new uint8_t[mnBlockSize]();
    do 
    {
        int32_t nNumRead = 0;
        if (!reader.Read(pBuff, mnBlockSize, nNumRead))
        {
            bDone = true;
        }
        else
        {
            if (nNumRead > 0)
            {
                BlockDescription block;
            
                mnTotalBytesToProcess += nNumRead;
                block.mRollingChecksum = GetRollingChecksum(pBuff, nNumRead);

                MD5(pBuff, nNumRead, &block.mMD5[0]);
                block.mpPath = UniquePath(sFilename);
                block.mnOffset = nOffset;

                if (nOffset == 6520832 || nOffset == 5832704)
                {
                    int x = 5;
                }

                tMd5ToBlockMap& md5ToBlockMap = mChecksumToMD5BlockMap[ block.mRollingChecksum ];
                if (md5ToBlockMap.find(block.mMD5) == md5ToBlockMap.end())
                    md5ToBlockMap[block.mMD5] = block;

                mnNumBlocks++;

                nOffset += nNumRead;
            }
            else
                bDone = true;
        }
    } while (!bDone);

    mnTotalFiles++;
	delete[] pBuff;
    reader.Close();
}

void BlockScanner::StartMatchThread(const wstring& sFilename, int64_t nStartOffset, int64_t nBytesToScan)
{
    DWORD nStart = ::GetTickCount();
    bool bDone = false;
    int32_t nThreadIndex = 0;
    while (!bDone)
    {
        nThreadIndex++;
        
        if (nThreadIndex >= kMaxMatcherThreads)
        {
            nThreadIndex = 0;
            Sleep(0); // went around whole index, wait a bit before trying again.
        }

        int32_t nStatus = mMatcher[nThreadIndex]->mnStatus;
        if (nStatus == MatcherThread::kNone || nStatus == MatcherThread::kFinished || nStatus == MatcherThread::kError)
        {
            mMatcher[nThreadIndex]->FindMatches(UniquePath(sFilename), nStartOffset, nBytesToScan, mpOutStream);

#ifdef _DEBUG
            wcout << "Time spent waiting to start: " << GetTickCount() - nStart << "\n";
#endif
            return;
        }
    }
}


uint32_t BlockScanner::UpdateRollingChecksum(uint32_t prevChecksum, uint8_t dropByteValue, uint8_t addByteValue, uint32_t nBlockSizeLog2)
{
    uint16_t low16  = (uint16_t)prevChecksum;
    uint16_t high16 = (uint16_t)(prevChecksum >> 16);

    low16  += (addByteValue - dropByteValue);
    high16 += low16 - (dropByteValue << nBlockSizeLog2);

    return (uint32_t)((high16 << 16) | low16);
}

uint32_t BlockScanner::GetRollingChecksum(const uint8_t* pData, size_t dataLength)
{
   uint16_t low16  = 0;
   uint16_t high16 = 0;

    while(dataLength)
    {
        const uint8_t c = *pData++;
        low16  += (uint16_t)(c);
        high16 += (uint16_t)(c * dataLength);
        dataLength--;
    }

    return (uint32_t)((high16 << 16) | low16);
}

DWORD WINAPI BlockScanner::ScanProc(LPVOID lpParameter)
{
    BlockScanner* pScanner = (BlockScanner*) lpParameter;

    pScanner->mStartScanTimestamp = GetTickCount();

    *pScanner->mpOutStream << "BlockScanner - Scanning path: " << pScanner->msRootPath << "\n";

    list<wstring> foldersToScan;
    foldersToScan.push_back(pScanner->msRootPath);
    pScanner->mnTotalFolders++;

    while (!pScanner->mbCancel && !foldersToScan.empty())
    {
        wstring sPath = foldersToScan.back();
        wstring sPattern = sPath;
        sPattern.append(L"*");
        foldersToScan.pop_back();

        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile(sPattern.c_str(), &findData);
        bool bContinue = true;
        while (!pScanner->mbCancel && hFind != INVALID_HANDLE_VALUE && bContinue)
        {
            if (wcscmp( findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0)
            {

                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    wstring sFolder(sPath);
                    sFolder.append(L"/");
                    sFolder.append(findData.cFileName);
                    sFolder.append(L"/");
                    foldersToScan.push_back(sFolder);
                    pScanner->mnTotalFolders++;
                }
                else
                {
                    wstring sFilePath(sPath);
                    sFilePath.append(findData.cFileName);
                    pScanner->ScanFile(sFilePath);
                }
            }

            if (FindNextFile(hFind, &findData) == FALSE)
            {
                bContinue = false;
            }
        }
        FindClose(hFind);
    }

    *pScanner->mpOutStream << "BlockScanner - Matching: " << pScanner->msRootPath << "\n";

    pScanner->DumpChecksums(wcout);

    pScanner->mnStatus = BlockScanner::kMatching;

    foldersToScan.push_back(pScanner->msRootPath);

    while (!pScanner->mbCancel && !foldersToScan.empty())
    {
        wstring sPath = foldersToScan.back();
        wstring sPattern = sPath;
        sPattern.append(L"*");
        foldersToScan.pop_back();

        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile(sPattern.c_str(), &findData);
        bool bContinue = true;
        while (!pScanner->mbCancel && hFind != INVALID_HANDLE_VALUE && bContinue)
        {
            if (wcscmp( findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0)
            {

                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    wstring sFolder(sPath);
                    sFolder.append(L"/");
                    sFolder.append(findData.cFileName);
                    sFolder.append(L"/");
                    foldersToScan.push_back(sFolder);
                }
                else
                {
                    wstring sFilePath(sPath);
                    sFilePath.append(findData.cFileName);

                    int64_t nFileSize = (((int64_t) findData.nFileSizeHigh) << 32)|(int64_t) findData.nFileSizeLow;

                    const int64_t kMatchThreadScanSize = 1024*1024;
                    for (int64_t nStartOffset = 0; nStartOffset < nFileSize; nStartOffset += kMatchThreadScanSize)
                    {
                        // If we're at the end of the file, pass the last set of bytes
                        int64_t nBytesToScan = kMatchThreadScanSize + pScanner->mnBlockSize;      // include an extra block for the rolling window to check all offsets
                        if (nStartOffset + nBytesToScan > nFileSize)
                            nBytesToScan = nFileSize-nStartOffset;

                        pScanner->StartMatchThread(sFilePath, nStartOffset, nBytesToScan);       // this will only return once a thread has been assigned to do the scanning
                    }
                }
            }

            if (FindNextFile(hFind, &findData) == FALSE)
            {
                bContinue = false;
            }
        }
        FindClose(hFind);
    }

    // Wait for all threads to finish
    bool bDone = false;
    while (!bDone)
    {
#ifdef _DEBUG
        wcout << "Matcher Statuses: ";
#endif
        bDone = true;
        for (int32_t i = 0; i < kMaxMatcherThreads; i++)
        {
#ifdef _DEBUG
            wcout << "[" << i << ":" << pScanner->mMatcher[i]->mnStatus << "] ";
#endif
            if (pScanner->mMatcher[i]->mnStatus == kMatching)
            {
                bDone = false;
                Sleep(50);
                continue;
            }
        }
        wcout << "\n";
    }

    pScanner->mEndScanTimestamp = GetTickCount();
    pScanner->mnStatus = BlockScanner::kFinished;

    return 0;
}

const wchar_t* BlockScanner::UniquePath(const wstring& sPath)
{
    tStringSet::iterator it = mAllPaths.insert(sPath).first;
   return (*it).c_str();
}

void BlockScanner::DumpReport()
{
    *mpOutStream << "************************************\n";
    *mpOutStream << "BlockScanner Report\n";

    *mpOutStream << "Total Folders Scanned: " << mnTotalFolders << "\n";
    *mpOutStream << "Total Files Scanned: " << mnTotalFiles << "\n";

    *mpOutStream << "Block Size: " << mnBlockSize << "\n";

    uint64_t nBlockCount = 0;
    for (tChecksumToMD5BlockMap::iterator it = mChecksumToMD5BlockMap.begin(); it != mChecksumToMD5BlockMap.end(); it++)
    {
        uint32_t nChecksum = (*it).first;
        tMd5ToBlockMap& blockMap = (*it).second;

        uint32_t nChecksumBlockCount = blockMap.size();
        nBlockCount += nChecksumBlockCount;
    }

    *mpOutStream << "Number of Matcher Threads: " << kMaxMatcherThreads << "\n";
    *mpOutStream << "Number of Unique Blocks: " << nBlockCount << "\n";
    *mpOutStream << "Number of Unique Hashes: " << mChecksumToMD5BlockMap.size() << "\n";


    *mpOutStream << "Total Bytes In Files: " << mnTotalBytesToProcess << "\n";
    *mpOutStream << "Total Offsets Compared: " << mnTotalOffsetsCompared << "\n";
    *mpOutStream << "Time spent (in seconds): " << (mEndScanTimestamp - mStartScanTimestamp)/1000 << "\n";
    *mpOutStream << "Blocks checked with no matches: " << mnUniqueBlocks << "\n";


    *mpOutStream << "Blocks with Matches: " << mMatches.size() << "\n";

    uint32_t nDuplicateBlocks = 0;
    for (tMd5ToBlockLocationSet::iterator it = mMatches.begin(); it != mMatches.end(); it++)
    {
        MD5Val val = (*it).first;

        tBlockLocationSet& locationSet = (*it).second;

        nDuplicateBlocks += locationSet.size();     // count the duplicates

        *mpOutStream << "MD5:" << val << " matches: " << locationSet.size() << " locations:\n";
        for (tBlockLocationSet::iterator locationIt = locationSet.begin(); locationIt != locationSet.end(); locationIt++)
        {
            const BlockLocation& location = *locationIt;
            *mpOutStream << location.mpPath << " offset:" << location.mnOffset << "\n";
        }
    }

    *mpOutStream << "Total Duplicate Blocks: " << nDuplicateBlocks << "\n";

}

void BlockScanner::DumpChecksums(wostream& outStream)
{
    outStream << "************************************\n";
    outStream << "Total Checksums Mapped:" << mChecksumToMD5BlockMap.size() << "\n";
    int32_t nUniqueChecksumCount = 0;
    for (tChecksumToMD5BlockMap::iterator it = mChecksumToMD5BlockMap.begin(); it != mChecksumToMD5BlockMap.end(); it++)
    {
        uint32_t nChecksum = (*it).first;
        tMd5ToBlockMap& blockMap = (*it).second;

        uint32_t nChecksumBlockCount = blockMap.size();

        if (nChecksumBlockCount > 1)
            outStream << "Checksum: " << nChecksum << " count: " << nChecksumBlockCount << "\n";
        else
            nUniqueChecksumCount++;
    }

    outStream << "Unique Checksums: " << nUniqueChecksumCount;
}


// Matches are rare, but must be inserted in a thread safe manner if we're to scan data with multiple threads
void BlockScanner::AddMatch(const MD5Val& md5Val, const BlockLocation& blockLocation)
{
    EnterCriticalSection(&mMatchCriticalSection);

    mMatches[md5Val].insert(blockLocation);

    LeaveCriticalSection(&mMatchCriticalSection);
}

