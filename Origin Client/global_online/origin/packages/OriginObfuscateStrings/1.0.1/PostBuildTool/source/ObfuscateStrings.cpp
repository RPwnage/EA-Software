//////////////////////////////////////////////////////////////////////////////////////////////////
// ObfuscateStrings
// Copyright 2015 Electronic Arts
// Written by Alex Zvenigorodsky
//
// Utility to search for tagged strings in a file and lightly obfuscate them
// 
#include "StringScanner.h"
#include <stdio.h>
#include <iostream>
#include <locale>
#include <string>

#ifdef WIN32
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
	// Any event, stop the scanner
        std::wcout << "Canceling\n";

	return TRUE;
}
#endif

#ifdef WIN32
void parseWinCommand(arguments& args)
{
        LPWSTR commandLine = ::GetCommandLineW();
        int argc;
        LPWSTR* argv = ::CommandLineToArgvW(commandLine, &argc);
        if(argv != NULL)
        {
                for(int i = 1; i < argc; ++i)
                {
                        if(0 == ::wcsncmp(L"-backup", argv[i], 7))
                        {
                                args.backUpFile = true;
                        }
                        else if(0 == ::wcsncmp(L"-file:", argv[i], 6))
                        {
                                args.fileName = argv[i] + 6;
                        }
                }
        }

}
#else
void parseUnixCommand(int argc, char** argv, arguments& args)
{
    for(int i = 1; i < argc; ++i)
    {
        if(0 == ::strncmp("-backup", argv[i], 7))
        {
            args.backUpFile = true;
        }
        else if(0 == ::strncmp("-file:", argv[i], 6))
        {
            args.fileName = argv[i] + 6;
        }
    }
}
#endif

int main(int argc, char* argv[])
{
    arguments args;
#ifdef WIN32
	SetConsoleCtrlHandler(ConsoleCtrlHandler, true);		// capture any control events like CTRL-C, or shutdown or logoff, etc.  This is for canceling scanning
    parseWinCommand(args);
#else
    parseUnixCommand(argc, argv, args);
#endif


	std::cout << "***************************\n";

    if (argc < 2 || args.fileName.empty())
	{
		std::cout << "Usage: ObfuscateStrings -file:FILENAME [-backup]\n";
        std::cout << "\nIf -backup is specified, the original file will be copied to a _backup appended filename ONLY if any strings were found to obfuscate.\n";
        std::cout << "\n The strings to be obfuscated MUST be surrounded by the exact tags: \"**OB_START**\" and \"***OB_END***\"\n\n";
		std::cout << "***************************\n";
		return -1;
	}

	std::wcout << "file:         " << args.fileName.c_str() << "\n";

    StringScanner scanner;
    if (scanner.Scan(args))
    {
        std::cout << "Total Strings Found and Obfuscated: " << scanner.mnTotalStringsObfuscated << "\n";
    }

	return 0;
}






