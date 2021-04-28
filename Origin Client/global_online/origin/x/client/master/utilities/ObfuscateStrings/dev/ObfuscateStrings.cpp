//////////////////////////////////////////////////////////////////////////////////////////////////
// ObfuscateStrings
// Copyright 2015 Electronic Arts
// Written by Alex Zvenigorodsky
//
// Utility to search for tagged strings in a file and lightly obfuscate them
// 

#include <stdio.h>
#include <iostream>
#include <locale>
#include <string>
#include <codecvt>
#include "StringScanner.h"
#include "EAPackageDefines.h"

using namespace std;

std::wstring sFilename;
bool bBackup = false;
std::string sStartTag = "**OB_START**";
std::string sEndTag = "***OB_END***" ;

#ifdef WIN32
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
	// Any event, stop the scanner
	wcout << "Canceling\n";

	return TRUE;
}
#endif

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
    ParseCommand(L"-file:", argc, argv, sFilename);

    std::wstring sTemp;
    bBackup = ParseCommand(L"-backup", argc, argv, sTemp);
}


int _tmain(int argc, _TCHAR* argv[])
{
#ifdef WIN32
	SetConsoleCtrlHandler(ConsoleCtrlHandler, true);		// capture any control events like CTRL-C, or shutdown or logoff, etc.  This is for canceling scanning
#endif

	ParseCommands(argc, argv);

	wcout << "***************************\n";

    if (argc < 2 || sFilename.empty())
	{
		wcout << "Usage: ObfuscateStrings -file:FILENAME [-backup]\n";
        wcout << "\nIf -backup is specified, the original file will be copied to a _backup appended filename ONLY if any strings were found to obfuscate.\n";
        wcout << "\n The strings to be obfuscated MUST be surrounded by the exact tags: \"**OB_START**\" and \"***OB_END***\"\n\n";
		wcout << "***************************\n";
		return -1;
	}

	wcout << "file:         " << sFilename.c_str() << "\n";
	wcout << "\n";

    StringScanner scanner;
    if (scanner.Scan(sFilename))
    {
        wcout << L"Total Strings Found and Obfuscated: " << scanner.mnTotalStringsObfuscated << "\n";
    }

	return 0;
}

#ifndef WIN32
int main(int argc, char* argv[])
{
    wchar_t** wargs = new wchar_t*[argc+1];
    wargs[argc] = NULL;
    
    // Xcode doesn't support _tmain or wmain so convert UTF-8 encoded char* args to UTF-16 wchar_t* args
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
    for (int i = 0; i < argc; ++i)
    {
        std::wstring wstr = convert.from_bytes(argv[i]);
        wchar_t* pBuf = new wchar_t[wstr.length() + 1];
        memset(pBuf, 0, wstr.length() + 1);
        wcscpy(pBuf, wstr.data());
        wargs[i] = pBuf;
    }
                                    
    int result = _tmain(argc, wargs);
    
    for (int i = 0; i < argc; ++i)
    {
        delete[] wargs[i];
    }
    delete [] wargs;
        
    return result;
}
#endif






