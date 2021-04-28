//////////////////////////////////////////////////////////////////////////////////////////////////
// TweakCRC
// Copyright 2014 Electronic Arts
// Written by Alex Zvenigorodsky
//
// CRCMap class used to calculate 32 bit CRC values from arbitrary source data
//
// CRCScanner will scan a file for the 32 bit CRC value and optionally searches for a signature 
// Scanning is done on another thread
//
// 
#include <string>
#include <stdint.h>
#include <Windows.h>

class CRCMap
{
public:
	CRCMap() : nCRC(0xffffffff) {}

	void update(const uint8_t* data, int nLength);
	operator uint32_t() {return ~nCRC;}

	uint32_t nCRC;
};

class CRCScanner
{
public:

	enum eState
	{
		kNone = 0,
		kScanning = 1,
		kFinished = 2,
		kCancelled = 3,
		kError = 4
	};

	CRCScanner();
	~CRCScanner();

	void			Scan(std::wstring filePath, std::string sSignature = "");			// Starts a scanning thread
	void			Cancel();															// Signals the thread to terminate and returns when thread has terminated

	static void		ReverseCRC(uint32_t& nCRC, int64_t nIterations);			// Utility function needed to be able to calculate tweak value for forcing a file to have a specific CRC

	int32_t			mnStatus;				// Set by Scanner
	std::wstring	msError;				// Set by Scanner

	int64_t			mnTotalBytes;			// Set by Scanner
	int64_t			mnTotalBytesScanned;	// Set by Scanner
	uint32_t		mnCRC;					// Set by Scanner
	int64_t			mnOffsetOfTag;			// Set by Scanner if tag is found

private:
	static DWORD WINAPI			ScanProc(LPVOID lpParameter);
	static void					FillError(CRCScanner* pScanner);
	static inline const char*	FindSubString(const char* pSubString, int nSubStringLength, const char* pBuffer, int nBufferLength);

	std::wstring	msFilePath;				// File to be scanned
	std::string		msSignature;			// Signature to look for.
	bool			mbScanForSignature;		// Only scan if a signature was passed in
	bool			mbCancel;

	// Thread related
	HANDLE			mhThread;	
	DWORD			mThreadID;
};