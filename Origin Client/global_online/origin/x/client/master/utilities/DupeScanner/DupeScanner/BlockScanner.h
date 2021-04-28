//////////////////////////////////////////////////////////////////////////////////////////////////
// BlockScanner
// Written by Alex Zvenigorodsky
//
#ifndef BLOCKSCANNER_H
#define BLOCKSCANNER_H

#include <string>
#include <stdint.h>
#include <Windows.h>
#include <set>
#include <map>
#include <hash_set>
#include <openssl/md5.h>
#include "MatcherThread.h"

using namespace std;

typedef set<wstring> tStringSet;

class MD5Val
{
public:
    MD5Val(uint8_t* pVal = NULL);
    MD5Val(const MD5Val& rhs);

    bool operator < (const MD5Val& rhs) const
    {
        return memcmp(&mVal[0], &rhs.mVal[0], MD5_DIGEST_LENGTH) < 0;
    }

    bool operator > (const MD5Val& rhs) const
    {
        return memcmp(&mVal[0], &rhs.mVal[0], MD5_DIGEST_LENGTH) > 0;
    }

    bool operator == (const MD5Val& rhs) const
    {
        return memcmp(&mVal[0], &rhs.mVal[0], MD5_DIGEST_LENGTH) == 0;
    }

    bool operator != (const MD5Val& rhs) const
    {
        return memcmp(&mVal[0], &rhs.mVal[0], MD5_DIGEST_LENGTH) != 0;
    }

    uint8_t& operator[](std::size_t index)
    {
        return mVal[index];
    }

    friend wostream& operator << (wostream& out, const MD5Val& rhs);

private:
    uint8_t mVal[MD5_DIGEST_LENGTH];
};

class BlockLocation
{
public:
    BlockLocation() : mpPath(NULL), mnOffset(-1) {}
    BlockLocation(const wchar_t* pPath, int64_t nOffset) : mpPath(pPath), mnOffset(nOffset) {};


    bool operator < (const BlockLocation& rhs) const
    {
        int nResult = wcscmp(mpPath, rhs.mpPath);
        if (nResult == 0)
        {
            if (mnOffset < rhs.mnOffset)
                return true;
            else if (mnOffset > rhs.mnOffset)
                return false;
        }
        
        return nResult < 0;
    }

    const wchar_t* mpPath;
    int64_t mnOffset;
};

typedef std::set<BlockLocation> tBlockLocationSet;
typedef std::map<MD5Val, tBlockLocationSet> tMd5ToBlockLocationSet;

class BlockDescription
{
public:
    BlockDescription();

    uint32_t        mRollingChecksum;   // extra, can be removed since this should be a mapping of rolling checksum to set of blocks matching it
    //uint8_t         mMD5[MD5_DIGEST_LENGTH];    
    MD5Val          mMD5;
    const wchar_t*  mpPath;
    int64_t         mnOffset;           // offset within the file where the block is found

    bool operator < (const BlockDescription& rhs) const
    {
        if (mRollingChecksum < rhs.mRollingChecksum)
            return true;
        else if (mRollingChecksum > rhs.mRollingChecksum)
            return false;

        if (mMD5 < rhs.mMD5)
            return true;
        else if (mMD5 > rhs.mMD5)
            return false;

        if (mnOffset < rhs.mnOffset)
            return true;
        else if (mnOffset > rhs.mnOffset)
            return false;
        
        return wcscmp(mpPath, rhs.mpPath) < 0;
    }

    bool operator == (const BlockDescription& rhs) const
    {
        // true if all of pieces equate
        return  mnOffset == rhs.mnOffset &&                     // same offset in the file
                wcscmp(mpPath, rhs.mpPath) == 0;                 // same file                
    }

    friend wostream& operator << (wostream& out, const BlockDescription& rhs);
};


//typedef std::set<BlockDescription> tBlockSet;
typedef std::map<MD5Val, BlockDescription> tMd5ToBlockMap;
//typedef std::hash_multiset<uint32_t, tMd5ToBlockMap> tChecksumToMD5BlockMap;
typedef std::map<uint32_t, tMd5ToBlockMap>   tChecksumToMD5BlockMap;
//typedef std::vector<tMd5ToBlockMap>     tChecksumToMD5BlockMap;

const int32_t kMaxMatcherThreads = 12;


class BlockScanner
{
public:

    friend class MatcherThread;

    enum eState
    {
        kNone       = 0,
        kScanning   = 1,
        kMatching   = 2,
        kFinished   = 3,
        kCancelled  = 4,
        kError      = 5
    };

    BlockScanner();
    ~BlockScanner();

    void			        Scan(wstring rootPath, int32_t nBlockSize, wostream* pOutStream);			// Starts a scanning thread
    void			        Cancel();								// Signals the thread to terminate and returns when thread has terminated

    const wchar_t*          UniquePath(const wstring& sPath);
    int32_t                 NumUniquePaths() { return (int32_t) mAllPaths.size(); }

    void                    DumpReport();
    void                    DumpChecksums(wostream& outStream);

    int32_t			        mnStatus;				// Set by Scanner
    std::wstring	        msError;				// Set by Scanner

    uint64_t                mnNumBlocks;
    uint64_t                mnTotalFiles;
    uint64_t                mnTotalFolders;
    uint64_t                mnTotalBytesToProcess;
    uint64_t                mnTotalOffsetsCompared;

    uint64_t                mnUniqueBlocks;

    DWORD                   mStartScanTimestamp;
    DWORD                   mEndScanTimestamp;



    // checksum calc
    static uint32_t         GetRollingChecksum(const uint8_t* pData, size_t dataLength);
    static uint32_t         UpdateRollingChecksum(uint32_t prevChecksum, uint8_t dropByteValue, uint8_t addByteValue, uint32_t nBlockSizeLog2);

private:
    tChecksumToMD5BlockMap  mChecksumToMD5BlockMap;        
    tStringSet              mAllPaths;


    void                    StartMatchThread(const wstring& sFilename, int64_t nStartOffset, int64_t nBytesToScan);

    // For collecting matches in a multithreaded way
    void                    AddMatch(const MD5Val& md5Val, const BlockLocation& blockLocation);        // maps a found match
    tMd5ToBlockLocationSet  mMatches;       
    CRITICAL_SECTION        mMatchCriticalSection;


    void                    ScanFile(const wstring& sFilename);

    static DWORD WINAPI		ScanProc(LPVOID lpParameter);
    static void				FillError(BlockScanner* pScanner);

    wostream*                mpOutStream;

    std::wstring            msRootPath;				                // File to be scanned
    int32_t                 mnBlockSize;
    int32_t                 mnBlockSizeLog2;
    bool		            mbCancel;

    // Thread related
    HANDLE			        mhThread;	
    DWORD			        mThreadID;

    MatcherThread*          mMatcher[kMaxMatcherThreads];
};

#endif // BLOCKSCANNER_H