// DupeScanner.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include <tchar.h>
#include <locale>
#include <string>
#include "BlockScanner.h"
#include "BufferedFileReader.h"

using namespace std;

uint32_t kDefaultBlockSize = 16*1024;  

std::wstring sPathToScan;
std::wstring sOutputLog;
uint32_t    nBlockSize = kDefaultBlockSize;
BlockScanner scanner;






#include <math.h>

const   long N = 624;		// Replaced the #defines for these with const values - DHL
const   long M = 397;
const   long K = 0x9908B0DFU;
const   long DEFAULT_SEED = 4357;

#define hiBit(u)       ((u) & 0x80000000U)   // mask all but highest   bit of u
#define loBit(u)       ((u) & 0x00000001U)   // mask all but lowest    bit of u
#define loBits(u)      ((u) & 0x7FFFFFFFU)   // mask     the highest   bit of u
#define mixBits(u, v)  (hiBit(u)|loBits(v))  // move hi bit of u to hi bit of v

class CRandomMT
{
	long	state[N+1];     // state vector + 1 extra to not violate ANSI C
	long  *next;			// next random value is computed from here
	long			left;			// can *next++ this many times before reloading
	long	seedValue;		// Added so that setting a seed actually maintains 
	// that seed when a ReloadMT takes place.

public:
	CRandomMT();
	CRandomMT(long _seed);
	~CRandomMT(){};
	void		SeedMT(long seed);


	inline long RandomMT(void)
	{
		long y;

		if(--left < 0)
			return(ReloadMT());

		y  = *next++;
		y ^= (y >> 11);
		y ^= (y <<  7) & 0x9D2C5680U;
		y ^= (y << 15) & 0xEFC60000U;
		return(y ^ (y >> 18));
	}


	long operator()(long limit) { return RandomMax(limit); }

	inline long	RandomMax(long max)
	{
		return(((long)RandomMT()%max));
	}


	inline long RandomRange(long lo, long hi)
	{
		// Changed thanks to mgearman's message
		return (RandomMT() % (hi - lo + 1) + lo);
	}


private:
	long	ReloadMT(void);

};


BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
    // Any event, stop the scanner
    wcout << "Canceling\n";
    scanner.Cancel();

    return TRUE;
}


bool ParseCommand(const std::wstring& sFlag, int argc, _TCHAR* argv[], std::wstring& sCommand)
{
    int nLength = sFlag.length();
    for (int i = 0; i < argc; i++)
    {
        if ( memcmp(argv[i], sFlag.data(), nLength*sizeof(wchar_t)) == 0)
        {
            sCommand.assign(argv[i]+nLength);
            return true;
        }
    }

    return false;
}

void ParseCommands(int argc, _TCHAR* argv[])
{
    // look for input
    ParseCommand(L"-path:", argc, argv, sPathToScan);

    wstring sBlockSize;
    if (ParseCommand(L"-blocksize:", argc, argv, sBlockSize))
    {
        nBlockSize = _wtol(sBlockSize.c_str());
    }

    ParseCommand(L"-output:", argc, argv, sOutputLog);
}

//#define TEST_MD5_CIRCULAR

#ifdef  TEST_MD5_CIRCULAR

uint32_t UpdateRollingChecksum(uint32_t prevChecksum, uint8_t dropByteValue, uint8_t addByteValue)
{

    uint32_t nBlockSizeLog2 = (int32_t) (log((double) 8)/log((double) 2));

    uint16_t low16  = (uint16_t)prevChecksum;
    uint16_t high16 = (uint16_t)(prevChecksum >> 16);

    low16  += (addByteValue - dropByteValue);
    high16 += low16 - (dropByteValue << nBlockSizeLog2);

    return (uint32_t)((high16 << 16) | low16);
}

uint32_t GetRollingChecksum(const uint8_t* pData, size_t dataLength)
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
#endif


int _tmain(int argc, _TCHAR* argv[])
{


#ifdef TEST_MD5_CIRCULAR
    uint8_t md51[16];
    uint8_t md52[16];

    char* pBuf1[] = {"00000000"};
    char* pBuf2[] = {"11111111"};

    MD5((const unsigned char*) pBuf1, 8, &md51[0]);
    MD5((const unsigned char*) pBuf2, 8, &md52[0]);



    char pBuf3[] = {"0123456789ABCDEF"};
    char pBuf4[] = {"123456789ABCDEFG"};

    uint32_t nCheck1 = GetRollingChecksum((uint8_t*) pBuf4, 8);
    uint32_t nCheck2 = GetRollingChecksum((uint8_t*) pBuf3, 8);
    nCheck2 = UpdateRollingChecksum(nCheck2, '0', '8');

    if (nCheck1 != nCheck2)
    {
        int x = 5;
    }

#endif // TEST_MD5_CIRCULAR


//#define TEST_BUFFERED_READER
#ifdef  TEST_BUFFERED_READER
    BufferedFileReader reader(16*1024);

    reader.Open(L"D:/temp/cas_01.cas_bad_from_akamai");

    HANDLE mhFile = CreateFile(L"d:/temp/cas_02.cas_bad_from_akamai", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);

    const int blockSize = 100*1024;
    uint8_t* pBuf = new uint8_t[blockSize];
    int32_t nNumRead = 0;
    reader.Read(pBuf, blockSize, nNumRead);
    while (nNumRead > 0)
    {
        DWORD nNumWritten = 0;
        WriteFile(mhFile, pBuf, nNumRead, &nNumWritten, NULL);

        reader.Read(pBuf, 1+rand()%32768, nNumRead);
    }
    CloseHandle(mhFile);

#endif //  TEST_BUFFERED_READER

//#define TEST_BUFFERED_READER_SPEED
#ifdef TEST_BUFFERED_READER_SPEED
    // Test reading files
    
    DWORD nTime = ::GetTickCount();

    BufferedFileReader reader(16*1024);

    reader.Open(L"D:/temp/cas_01.cas_bad_from_akamai");


    const int blockSize = 100*1024;
    uint8_t* pBuf = new uint8_t[blockSize];
    int32_t nNumRead = 0;
    reader.Read(pBuf, blockSize, nNumRead);

    uint32_t nTotalRead = nNumRead;
    uint32_t nRollingChecksum = 0;
    while (nNumRead > 0)
    {
//        memcpy(pBuf, pBuf+1, blockSize-1);      // shift over all data by 1 byte
        uint8_t nOldVal = *pBuf;
        reader.Read(pBuf, 1, nNumRead);
        scanner.UpdateRollingChecksum(nRollingChecksum, nOldVal, *pBuf);
        nTotalRead += nNumRead;
    }

    reader.Close();

    wcout << "end time: " << ::GetTickCount() - nTime << " total read:" << nTotalRead << " checksum:" << nRollingChecksum << "\n";


    DWORD nTime2 = ::GetTickCount();

    HANDLE mhFile = CreateFile(L"d:/temp/cas_01.cas_bad_from_akamai", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    DWORD nNumRead2 = 1;
    uint8_t nChar;
    while (::ReadFile(mhFile, &nChar, 1, &nNumRead2, NULL) == TRUE && nNumRead2 > 0);

    CloseHandle(mhFile);
    wcout << "end of read time: " << ::GetTickCount() - nTime2 << "\n";

#endif // TEST_BUFFERED_READER_SPEED

//#define GENERATE_FILE
#ifdef GENERATE_FILE

    // Random number generator...... because rand() sucks.
    CRandomMT rd(DEFAULT_SEED);

    CRandomMT::CRandomMT() : left(-1)
    {
	    SeedMT(DEFAULT_SEED);
    }

    CRandomMT::CRandomMT(long _seed) : left(-1), seedValue(_seed)
    {
	    SeedMT(seedValue);
    }

    void CRandomMT::SeedMT(long seed)
    {
	    register long x = (seed | 1U) & 0xFFFFFFFFU, *s = state;
	    register long    j;

	    for(left=0, *s++=x, j=N; --j;
		    *s++ = (x*=69069U) & 0xFFFFFFFFU);
		    seedValue = seed;	// Save the seed value used - DHL
    }


    long CRandomMT::ReloadMT(void)
    {
	    register long *p0=state, *p2=state+2, *pM=state+M, s0, s1;
	    register long    j;

	    if(left < -1)
		    SeedMT(seedValue);	// Changed to make use of our seed value - DHL

	    left=N-1, next=state+1;

	    for(s0=state[0], s1=state[1], j=N-M+1; --j; s0=s1, s1=*p2++)
		    *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

	    for(pM=state, j=M; --j; s0=s1, s1=*p2++)
		    *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

	    s1=state[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
	    s1 ^= (s1 >> 11);
	    s1 ^= (s1 <<  7) & 0x9D2C5680U;
	    s1 ^= (s1 << 15) & 0xEFC60000U;
	    return(s1 ^ (s1 >> 18));
    }





    const int32_t nSize = 1*1024*1024;
    const int32_t nDupeSize = 16*1024;
    int32_t nDupeCount = 1;

//    srand(GetTickCount());
    rd.SeedMT(GetTickCount());
    uint8_t* pBuf = new uint8_t[nSize];
    for (int i = 0; i < nSize; i++)
    {
        *(pBuf+i) = rd.RandomMax(256);
    }


    while (nDupeCount-- > 0)
    {
//        int32_t nSourceIndex = rd.RandomMax(nSize-nDupeSize);
        int32_t nSourceIndex = 1;

//        int32_t nDestIndex = rd.RandomMax(nSize-nDupeSize);
        int32_t nDestIndex = 20*1024;

        memcpy(pBuf+nDestIndex, pBuf+nSourceIndex, nDupeSize);
    }

    HANDLE hFile = ::CreateFile(L"d:/temp/test_dupe_generated/file.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD nNumWritten;
        WriteFile(hFile, pBuf, nSize, &nNumWritten, NULL);
        CloseHandle(hFile);

        delete[] pBuf;
        exit(-1);
    }
    else
    {
		delete[] pBuf;
        wcout << "Failed to open file!\n";
        uint8_t* pNull = NULL;
        *pNull = 1;       // crash here

        return -1;
    }

#endif


    ParseCommands(argc, argv);

    if (sPathToScan.empty())
    {
        wcout << "usage: DupeScanner -path:PATH [-blocksize:BYTES] [-output:FILENAME]\n";
        system("pause");
        return -1;
    }

    wostream* pOutStream = &wcout;
    wofstream outputFile;
    if (!sOutputLog.empty())
    {
        outputFile.open(sOutputLog, ios::out|ios::trunc);
        pOutStream = &outputFile;
    }

    // 
    scanner.Scan(sPathToScan, nBlockSize, pOutStream);

    DWORD nReportTime = ::GetTickCount();
    DWORD nStartTime = nReportTime;
    const DWORD kReportPeriod = 1000;

    bool bDone = false;
    while (!bDone)
    {
        DWORD nCurrentTime = ::GetTickCount();

        if (scanner.mnStatus == BlockScanner::kError)
        {
            wcout << L"BlockScanner - Error - \"" << scanner.msError.c_str() << "\"";

            bDone = true;
            return 0;
        }

        if (scanner.mnStatus == BlockScanner::kFinished)
        {
            wcout << "Finished.....Dumping Report.\n";
            scanner.DumpReport();
            wcout << L"done.\n";
            return 0;
        }

        if (scanner.mnStatus == BlockScanner::kCancelled)
        {
            wcout << L"cancelled.";
            return 0;
        }

        if (scanner.mnStatus == BlockScanner::kScanning && nCurrentTime - nReportTime > kReportPeriod)
        {
            nReportTime = nCurrentTime;
            wcout << L"Scanning... files:" << scanner.mnTotalFiles << " blocks:" << scanner.mnNumBlocks << " time elapsed:" << (::GetTickCount()-nStartTime)/1000 << " \n";
        }

        if (scanner.mnStatus == BlockScanner::kMatching && nCurrentTime - nReportTime > kReportPeriod)
        {
            nReportTime = nCurrentTime;
            float fPercentComplete = 0.0f;
            if (scanner.mnTotalBytesToProcess > 0)
                fPercentComplete = 100.0f * ((float) scanner.mnTotalOffsetsCompared/ (float) scanner.mnTotalBytesToProcess);
            wcout << L"Matching... Total Bytes:" << scanner.mnTotalBytesToProcess << " Offsets Compared:" << scanner.mnTotalOffsetsCompared << " Percent Complete:" << fPercentComplete << " time elapsed:" << (::GetTickCount()-nStartTime)/1000 << " \n";
        }


        Sleep(100);
    }

	return 0;
}

