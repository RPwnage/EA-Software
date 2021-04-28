//////////////////////////////////////////////////////////////////////////////////////////////////
// TweakCRC
// Copyright 2014 Electronic Arts
// Written by Alex Zvenigorodsky
//
// Utility to tweak the CRC value in a file.
// Based on algorithm defined here:
// http://www.codeproject.com/Articles/4251/Spoofing-the-Wily-Zip-CRC
//
// Signature
// The file must have 4 bytes that are modifiable anywhere in the file.  
// Typically this should be done by including a fixed signature that is at least 4 bytes long in the file that TweakCRC can scan for. 
// (For example "XXXX_TWEAK THIS_XXXX")
// 
// Make sure the signature large enough so that it is unique so that no other location in the file may match it.
// 
// TweakCRC can be used in a number of ways.
// 1) Scan a file and compute CRC
// TweakCRC.exe -tweak:FILENAME
// 
// 2) Scan a file, compute CRC and find a signature
// TweakCRC.exe -tweak:FILENAME -signature:SIGNATURE
// 
// 3) Tweak the CRC of a file based on a known 32 bit CRC value. (hex or decimal values work)
// TweakCRC.exe -tweak:FILENAME -signature:SIGNATURE -targetcrc:0x1234567
// TweakCRC.exe -tweak:FILENAME -signature:SIGNATURE -targetcrc:5005
// 
// 4) Tweak the CRC of a file based on the CRC of another reference file
// TweakCRC.exe -tweak:FILENAME -signature:SIGNATURE -reference:FILENAME
// 
// Optional flags:
// -scansimultaneously      - Scans both the tweak file and reference file simultaneously. Useful if the files are on different harddrives.
// -verify                  - Rescans the tweaked file to verify the CRC matches the target value.
// -skipconfirm             - Skips confirmation
//

#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <locale>
#include <string>
#include "CRCScanner.h"

using namespace std;

// Global Scanner
CRCScanner tweakFileScanner;
CRCScanner referenceFileScanner;

std::wstring sTweakFilename;
std::wstring sReferenceFilename;
std::wstring sTargetCRC;
std::string sSignature;
bool bScanReference = false;
bool bScanSimultaneously = false;		// Whether to scan both reference and tweak file simultaneously
bool bVerify = false;
bool bSkipConfirm = false;

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
	// Any event, stop the scanner
	wcout << "Canceling\n";
	tweakFileScanner.Cancel();
	referenceFileScanner.Cancel();

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
	ParseCommand(L"-tweak:", argc, argv, sTweakFilename);
	ParseCommand(L"-reference:", argc, argv, sReferenceFilename);

	if (!sReferenceFilename.empty())
		bScanReference = true;

	ParseCommand(L"-targetcrc:", argc, argv, sTargetCRC);

	wstring sParam;
	ParseCommand(L"-signature:", argc, argv, sParam);
	sSignature.assign(sParam.begin(), sParam.end());		// convert to ascii

	bScanSimultaneously = ParseCommand(L"-scansimultaneously", argc, argv, sParam);	
	bVerify = ParseCommand(L"-verify", argc, argv, sParam);
	bSkipConfirm = ParseCommand(L"-skipconfirm", argc, argv, sParam);
}

// The following function takes a file to change and an offset in that file and changes 4 bytes at that offset so that the file will match the target CRC
void TweakCRC(const wstring& sTweakFilename, uint64_t nOffset, uint32_t tweakCRC, uint32_t nTargetCRC)
{
	wcout << "Tweaking CRC in file " << sTweakFilename << " at offset " << nOffset << "\n";

	HANDLE hScanFile = CreateFile( sTweakFilename.c_str(), GENERIC_WRITE|GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hScanFile == INVALID_HANDLE_VALUE)
	{
		WCHAR* pError   = NULL;
		DWORD  dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (WCHAR*)&pError, 0, NULL);
		if(dwResult)
		{
			cout << "Couldn't open file!";
			cout << pError;
			LocalFree(pError);
		}
		return;
	}

	LARGE_INTEGER nFileOffset;
	nFileOffset.QuadPart = nOffset;

	LARGE_INTEGER nNewLocation;

	if (SetFilePointerEx(hScanFile, nFileOffset, &nNewLocation, FILE_BEGIN) != TRUE || nNewLocation.QuadPart != nFileOffset.QuadPart)
	{
		WCHAR* pError   = NULL;
		DWORD  dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (WCHAR*)&pError, 0, NULL);
		if(dwResult)
		{
			cout << "Couldn't set file pointer for reading existing tag!";
			cout << pError;
			LocalFree(pError);
		}
		CloseHandle(hScanFile);
		return;
	}

	LARGE_INTEGER nLarge;
	GetFileSizeEx(hScanFile, &nLarge);

	uint64_t nIterationsToReverseCRC = (nLarge.QuadPart - nOffset)*8;
	uint32_t nFixUpCRC = tweakCRC^nTargetCRC;
	CRCScanner::ReverseCRC(nFixUpCRC, nIterationsToReverseCRC);

	uint32_t nCurrentTagData = 0;
	DWORD nNumBytesRead = 0;
	ReadFile(hScanFile, (void*) &nCurrentTagData, sizeof(uint32_t), &nNumBytesRead, NULL);


	uint32_t nTweakedTag = nCurrentTagData;
	nTweakedTag ^= nFixUpCRC;
	if (SetFilePointerEx(hScanFile, nFileOffset, &nNewLocation, FILE_BEGIN) != TRUE || nNewLocation.QuadPart != nFileOffset.QuadPart)
	{
		WCHAR* pError   = NULL;
		DWORD  dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (WCHAR*)&pError, 0, NULL);
		if(dwResult)
		{
			cout << "Couldn't set file pointer for writing new tag!";
			cout << pError;
			LocalFree(pError);
		}
		CloseHandle(hScanFile);
		return;
	}
	
	DWORD nNumWritten = 0;
	if (WriteFile(hScanFile, &nTweakedTag, sizeof(uint32_t), &nNumWritten, NULL) != TRUE || nNumWritten != sizeof(uint32_t))
	{
		WCHAR* pError   = NULL;
		DWORD  dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (WCHAR*)&pError, 0, NULL);
		if(dwResult)
		{
		cout << "Couldn't write new tag!";
			cout << pError;
			LocalFree(pError);
		}
	}

	CloseHandle(hScanFile);

	wcout << L"Tweak Complete - New tag written.\n";
	if (bVerify)
	{
		wcout << L"Verifying tweaked file CRC.";
		CRCScanner scanner;
		scanner.Scan(sTweakFilename);
		while (scanner.mnStatus != CRCScanner::kFinished)
		{
			Sleep(10);
		}

		wcout << L"Final CRC - " << std::dec << scanner.mnCRC << " (0x" << std::hex << scanner.mnCRC << ") ->";
		if (scanner.mnCRC == nTargetCRC)
			wcout << "Target CRC verified\n";
		else
			wcout << "WARNING TARGET CRC FAILED!\n";
	}
}

uint32_t StringtoCRC(std::wstring& s)
{
	// if the string has a 0x or any a through f, then it's a hex string
	if (s.find_first_of(L"xabcdefXABCDEF") != std::wstring::npos)
		return std::stoul(s, NULL, 16);

	return std::stoul(s);
}

int _tmain(int argc, _TCHAR* argv[])
{
	DWORD nStartTime = ::GetTickCount();

	SetConsoleCtrlHandler(ConsoleCtrlHandler, true);		// capture any control events like CTRL-C, or shutdown or logoff, etc.  This is for canceling scanning

	ParseCommands(argc, argv);

	wcout << "***************************\n";

	if (argc < 2 || sTweakFilename.empty())
	{
		wcout << "Usage: TweakCRC -tweak:FILENAME [-reference:FILENAME] [-targetcrc:CRC] [-signature:SIGNATURE] [-scansimultaneously]\n";
		wcout << "\n";
		wcout << "-tweak:FILENAME      - File to scan, calculating the CRC.  If a signature and reference file or signature and targetCRC are passed in, modifies the signature to make the file's CRC match.\n";
		wcout << "\n";
		wcout << "-reference:FILENAME  - Optional reference file which will be scanned and target CRC calculated.  Not needed if targetcrc is supplied.\n";
		wcout << "\n";
		wcout << "-targetcrc:CRC       - Optional target CRC value.  If supplied reference file will not be scanned.\n";
		wcout << "\n";
		wcout << "-signature:SIGNATURE - Optional signature to look for in the tweak file.  If no signature provided, only CRC will be calculated and reported.\n";
		wcout << "\n";
		wcout << "-scansimultaneously  - Optional flag to have both tweakfile and referencefile scanned simultaneously. Only recommended if they are on different harddrives.\n";
		wcout << "\n";
		wcout << "-verify              - Optional flag test the CRC of the resulting tweaked file.\n";
		wcout << "***************************\n";
		return -1;
	}


//#define TEST_THIS
#ifdef TEST_THIS

	CRCMap test;
	char s[41]={"This is a 0 crc file\x1a" "xxxx" "arbitrary stuff"};
    test.update((uint8_t*) s, sizeof(s));
    cout << "The CRC for this string is " << hex << test.nCRC << endl;

	uint32_t nFixup = test.nCRC ^ 0x01234567;
	ReverseCRC(nFixup, (41-21)*8);
	*(reinterpret_cast<uint32_t *>(s+21)) ^= nFixup;

	CRCMap test2;
	test2.update((uint8_t*) s, sizeof(s));
	cout << "The new CRC for this string is " << hex << test2.nCRC << endl;

	

#endif



	wcout << "tweak file:         " << sTweakFilename.c_str() << "\n";
	wcout << "reference file:     " << sReferenceFilename.c_str() << "\n";
	wcout << "scansimultaneously: " << (int) bScanSimultaneously << "\n";
	wcout << "signature:        \"" << sSignature.c_str() << "\"\n";
	wcout << "targetcrc:        \"" << sTargetCRC.c_str() << "\"\n";
	wcout << "\n";

	if (bScanSimultaneously && sReferenceFilename.empty())
	{
		wcout << "-scansimultaneously specified but no -reference parameter found.\n";
		return -1;
	}

	// If both a targetCRC and reference file are listed, we don't know which one to use.
	if (!sTargetCRC.empty() && !sReferenceFilename.empty())
	{
		wcout << "-targetcrc and -reference parameters both specified. Please only specify one or the other.";
		return -1;
	}

	tweakFileScanner.Scan(sTweakFilename, sSignature);
	if (bScanSimultaneously)
		referenceFileScanner.Scan(sReferenceFilename);

	DWORD nReportTime = ::GetTickCount();
	const DWORD kReportPeriod = 1000;

	bool bDone = false;
	while (!bDone)
	{
		DWORD nCurrentTime = ::GetTickCount();

		if (tweakFileScanner.mnStatus == CRCScanner::kError)
		{
			wcout << L"TweakFile - Error - \"" << tweakFileScanner.msError.c_str() << "\"";
			
			bDone = true;
			return 0;
		}
		if (referenceFileScanner.mnStatus == CRCScanner::kError)
		{
			wcout << L"ReferenceFileScanner - Error - \"" << referenceFileScanner.msError.c_str() << "\"";
			
			bDone = true;
			return 0;
		}

		// If the tweak file has completed scanning and we still need to scan the reference file
		if (!bScanSimultaneously && bScanReference && tweakFileScanner.mnStatus == CRCScanner::kFinished && referenceFileScanner.mnStatus == CRCScanner::kNone)
		{
			referenceFileScanner.Scan(sReferenceFilename);
		}

		if (tweakFileScanner.mnStatus == CRCScanner::kFinished && (referenceFileScanner.mnStatus == CRCScanner::kFinished || !bScanReference))
		{
			wcout << L"TweakFileScanner - Finished - Time Spent " << nCurrentTime - nStartTime << "ms.\n";
			wcout << L"TweakFile CRC - " << std::dec << tweakFileScanner.mnCRC << " (0x" << std::hex << tweakFileScanner.mnCRC << ")\n";

			if (bScanReference)
			{
				wcout << L"ReferenceFileScanner - Finished\n";
				wcout << L"ReferenceFile CRC - " << std::dec << referenceFileScanner.mnCRC << " (0x" << std::hex << referenceFileScanner.mnCRC << ")\n";
			}

			bool bSignatureFound = false;
			if (tweakFileScanner.mnOffsetOfTag >= 0)
			{
				wcout << L"TweakFile Signature \"" << sSignature.c_str() << "\" - Found at offset: " << std::dec << tweakFileScanner.mnOffsetOfTag << "\n";
				bSignatureFound = true;
			}
			else if (sSignature.length() > 0)
			{
				wcout << L"TweakFile Signature \"" << sSignature.c_str() << "\" - Not found.\n";
			}
			else 
			{
				wcout << L"TweakFile Signature - Did not search.  None provided.\n";
			}

			if (bSignatureFound)
			{
				// If a target CRC is available
				if (bScanReference || !sTargetCRC.empty())
				{
					uint32_t nTargetCRC = 0;

					if (bScanReference)
						nTargetCRC = referenceFileScanner.mnCRC;
					else
						nTargetCRC = StringtoCRC(sTargetCRC);


					// Nothing more to do if the CRC already matches
					if (tweakFileScanner.mnCRC == nTargetCRC)
					{
						wcout << "Tweak CRC already matches target.  Not making any changes.\n";
						bDone = true;
						return 0;
					}


					// Check with user before doing destructive operation
					bool bProceed = false;
					if (bSkipConfirm)		// if the "-proceed" was passed in, don't bother asking
					{
						bProceed = true;
					}
					else
					{
						wcout << L"\nReady to tweak value.\n*WARNING* This will change the following file: " << sTweakFilename << "!\n";
						wcout << L"\nType 'y' and press ENTER to proceed.";
						wstring sUserInput;
						wcin >> sUserInput;

						if (sUserInput.substr(0, 1) == L"y" || sUserInput.substr(0, 1) == L"Y")
						{
							bProceed = true;
						}
					}

					if (bProceed)
					{
						// Tweak the CRC 
						TweakCRC(sTweakFilename, tweakFileScanner.mnOffsetOfTag, tweakFileScanner.mnCRC, nTargetCRC);
					}
					else
					{
						wcout << L"Skipped tweaking file.\n";
					}
				}
			}


			bDone = true;
			return 0;
		}

		if (tweakFileScanner.mnStatus == CRCScanner::kCancelled || referenceFileScanner.mnStatus == CRCScanner::kCancelled)
		{
			bDone = true;
			return 0;
		}

		if (nCurrentTime - nReportTime > kReportPeriod)
		{
			nReportTime = nCurrentTime;
			if (tweakFileScanner.mnStatus == CRCScanner::kScanning)
			{
				uint64_t nTweakPercent = (tweakFileScanner.mnTotalBytesScanned*100) / tweakFileScanner.mnTotalBytes;
				wcout << L"Tweak-Scanning:" << sTweakFilename.c_str() << " " << tweakFileScanner.mnTotalBytesScanned/1024 << "MB out of " << tweakFileScanner.mnTotalBytes/1024 << "MB. " << nTweakPercent << "% Complete.  Time Elapsed:" << nCurrentTime - nStartTime << "ms.\n";
			}

			if (referenceFileScanner.mnStatus == CRCScanner::kScanning)
			{
				uint64_t nReferencePercent = (referenceFileScanner.mnTotalBytesScanned*100) / referenceFileScanner.mnTotalBytes;
				wcout << L"Reference-Scanning:" << sReferenceFilename.c_str() << " " << referenceFileScanner.mnTotalBytesScanned/1024 << "MB out of " << referenceFileScanner.mnTotalBytes/1024 << "MB. " << nReferencePercent << "% Complete.  Time Elapsed:" << nCurrentTime - nStartTime << "ms.\n";
			}
		}

		Sleep(100);
	}

	return 0;
}









