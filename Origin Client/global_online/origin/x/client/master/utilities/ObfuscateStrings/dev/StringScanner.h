//////////////////////////////////////////////////////////////////////////////////////////////////
// StringScanner
// Copyright 2014 Electronic Arts
// Written by Alex Zvenigorodsky
//
// 
#include <string>
#include <stdint.h>

#ifdef WIN32
#include <Windows.h>
#include <tchar.h>
#endif

#ifndef _TCHAR
typedef wchar_t _TCHAR;
#endif

class StringScanner
{
public:
	StringScanner();
	~StringScanner();

	bool			Scan(const std::wstring& filePath);			// Starts a scanning thread

    int32_t         mnTotalStringsObfuscated;   // Set by Scanner

private:
    void				    FillError(const _TCHAR* sAppError);
    uint32_t                ShiftBits(uint32_t nValue);

    void                    ObfuscateStringInFile(const std::wstring& sFilename, uint64_t nOffset, uint32_t nBytes);
    void                    Obfuscate(uint8_t* pBuf, uint32_t nBytes);
};