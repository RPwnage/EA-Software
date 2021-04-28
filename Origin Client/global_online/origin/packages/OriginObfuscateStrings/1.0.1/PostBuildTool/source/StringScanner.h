//////////////////////////////////////////////////////////////////////////////////////////////////
// StringScanner
// Copyright 2014 Electronic Arts
// Written by Alex Zvenigorodsky
//
// 
#ifndef _STRING_SCANNER_H_
#define _STRING_SCANNER_H_
#include "EABase/eabase.h"
#include <string>
#include <stdint.h>
#include <vector>

#ifdef WIN32
#include <Windows.h>
#include <tchar.h>
#endif

#ifndef _TCHAR
typedef wchar_t _TCHAR;
#endif

#ifdef __MACH__
typedef uint32_t uint_wchar;
#endif
#ifdef WIN32
typedef uint16_t uint_wchar;
#endif

struct arguments
{
	#ifdef WIN32
	std::wstring fileName;
	#else
	std::string fileName;
	#endif
	bool backUpFile = false;
};

class StringScanner
{
public:
	StringScanner();
	~StringScanner();

	bool			Scan(const arguments& args);			// Starts a scanning thread

    int32_t         mnTotalStringsObfuscated;   // Set by Scanner

private:
    void				    FillError(const _TCHAR* sAppError);

	void					ObfuscateStringInBuffer(char* fileBuffer, std::vector<std::pair<int, int>>& offsets);
	void					ObfuscateWStringInBuffer(char* fileBuffer, std::vector<std::pair<int, int>> offsets);
    void                    Obfuscate(uint8_t* pBuf, uint32_t chars);
    void                    Obfuscate(uint_wchar* pBuf, uint32_t chars);

};
#endif //_STRING_SCANNER_H_