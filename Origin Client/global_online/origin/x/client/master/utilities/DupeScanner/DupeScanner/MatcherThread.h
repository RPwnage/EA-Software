//////////////////////////////////////////////////////////////////////////////////////////////////
// MatcherThread
// Written by Alex Zvenigorodsky
//
#ifndef MATCHERTHREAD_H
#define MATCHERTHREAD_H

#include <string>
#include <stdint.h>
#include <Windows.h>

using namespace std;
class BlockScanner;
class BufferedFileReader;

class MatcherThread
{
public:

    enum eState
    {
        kNone       = 0,
        kMatching   = 2,
        kFinished   = 3,
        kCancelled  = 4,
        kError      = 5
    };

    MatcherThread(BlockScanner* pScanner, int32_t nBlockSize);
    ~MatcherThread();

    void                    FindMatches(const wchar_t* pPath, int64_t nStartOffset, int64_t nBytesToScan, wostream* pOutStream);
    void			        Cancel();								// Signals the thread to terminate and returns when thread has terminated

    int32_t			        mnStatus;				// Set by Scanner
    std::wstring	        msError;				// Set by Scanner

private:

    static DWORD WINAPI		MatchProc(LPVOID lpParameter);
    static void				FillError(BlockScanner* pScanner);

    BlockScanner*           mpBlockScanner;     // for reporting matches.

    wostream*               mpOutStream;

    const wchar_t*          mpPath;
    int64_t                 mnStartOffset;
    int64_t                 mnBytesToScan;

    int32_t                 mnBlockSize;
    uint8_t*                mpCircularBuff;
    int32_t                 mnCircularIndex;

    // Thread related
    HANDLE			        mhThread;	
    DWORD			        mThreadID;

    BufferedFileReader*     mpReader;
};

#endif // MATCHERTHREAD_H