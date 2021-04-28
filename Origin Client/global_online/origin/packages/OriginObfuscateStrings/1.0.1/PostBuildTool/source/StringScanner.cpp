#include "StringScanner.h"
#include "ObfuscateStrings/shiftbits.h"

#include <iostream>
#include <bitset>
#include <vector>
#include <utility>
#include <fstream>

#include <time.h>

static std::string sStartTag = "**OB_START**";
static std::string sEndTag = "***OB_END***" ;
static std::wstring sWStartTag = L"**OB_START**";
static std::wstring sWEndTag = L"***OB_END***";

StringScanner::StringScanner()
{
    mnTotalStringsObfuscated = -1;
}

StringScanner::~StringScanner()
{
}

void StringScanner::FillError(const _TCHAR* sAppError)
{
    std::string sOSError("Unknown error");

#ifdef WIN32
    char* pError   = NULL;
    DWORD  dwResult = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&pError, 0, NULL);
    if(dwResult)
    {
        sOSError.assign(pError);
        LocalFree(pError);
    }
#else
    sOSError = std::string(strerror(errno));
#endif

    std::wcout << sAppError;
    std::wcout << sOSError.c_str();
}


void StringScanner::Obfuscate(uint8_t* pBuf, uint32_t chars)
{
    if (chars < 24)
    {
        std::wcout << "Not enough chars to obfuscate!";
        return;
    }

    uint8_t nXOR = pBuf[0];      // first and last 12 bytes are randomized characters
    // modify our xor value by 23 possible values based on the length of the string.
    // This makes the obfuscation xor different for every string.
    //    uint8_t nXOR = 0xed ^ (nBytes%23);      

    for (uint32_t i = 12; i < chars-12; i++)       // no need to obfuscate tags
    {
        if (pBuf[i] != 0)               // leave nulls alone
            pBuf[i] = pBuf[i] ^ nXOR;

        nXOR = Origin::ObfuscateStrings::ShiftBits(nXOR);
    }
}

void StringScanner::Obfuscate(uint_wchar* pBuf, uint32_t chars)
{
    if (chars < 24)
    {
        std::wcout << "Not enough chars to obfuscate!";
        return;
    }

    uint_wchar nXOR = pBuf[0];      // first and last 12 bytes are randomized characters

    // modify our xor value by 23 possible values based on the length of the string.
    // This makes the obfuscation xor different for every string.
    //    uint8_t nXOR = 0xed ^ (nBytes%23);      


    for (uint32_t i = 12; i < chars - 12; i++)       // no need to obfuscate tags
    {
        if (pBuf[i] != 0)               // leave nulls alone
            pBuf[i] = pBuf[i] ^ nXOR;

        nXOR = Origin::ObfuscateStrings::ShiftBits(nXOR);
    }
}

// The following function takes a file to change and an offset and number of bytes to obfuscate
void StringScanner::ObfuscateStringInBuffer(char* fileBuffer, std::vector<std::pair<int, int>>& offsets)
{
	for (auto const& x : offsets)
	{
		const auto nOffset = x.first;
		const auto nBytes = x.second;

		uint8_t* pStringBuf = reinterpret_cast<uint8_t*>(fileBuffer + nOffset);

#ifdef _DEBUG
		//Uncomment for debugging purposes
		std::wcout << "Input string: \"";
		for (uint32_t i = 0; i < nBytes; i++)
			std::wcout << (char)pStringBuf[i];
		std::wcout << "\"\n";
#endif // _DEBUG


		// obliterate start and end tags
		for (uint32_t i = 0; i < sStartTag.length(); i++)
			pStringBuf[i] = 0x80 | rand() % 127;
		for (uint32_t i = nBytes - sEndTag.length(); i < nBytes; i++)
			pStringBuf[i] = 0x80 | rand() % 127;


		Obfuscate(pStringBuf, nBytes);
#ifdef _DEBUG
		//Uncomment for debugging purposes
		std::wcout << "Output string: \"";
		for (uint32_t i = 0; i < nBytes; i++)
		    std::wcout << (char)pStringBuf[i];
		std::wcout << "\"\n";

		std::wcout << L"Obfuscation Complete - New string written.\n";  
#endif // 

	}
}

void StringScanner::ObfuscateWStringInBuffer(char* fileBuffer, std::vector<std::pair<int, int>> offsets)
{
	for (auto const& x:offsets)
	{
		auto nOffset = x.first;
		auto nBytes = x.second;

		//wcout << "Obfuscating wstring in file " << sFilename << " at offset " << nOffset << "\n";
		uint32_t chars = nBytes / 2;
		if (chars < sWStartTag.length() + sWEndTag.length())
		{
			std::wcout << "Can't obfuscate string that is not at least surrounded with start/end tags.\n";
			continue;
		}

		uint_wchar* pStringBuf = reinterpret_cast<uint_wchar *>(fileBuffer + nOffset);

#ifdef _DEBUG
		//Uncomment for debugging purposes. Visual Studio can treat this output as an error depending on what's being obfuscated.
		//Go home VS you're drunk.
		std::wcout << "Input string:  \"";
		for (uint32_t i = 0; i < chars; i++)
		    std::wcout << (char)pStringBuf[i];
		std::wcout << "\"\n";  
#endif // _DEBUG


		// obliterate start and end tags
		for (uint32_t i = 0; i < sWStartTag.length(); i++)
			pStringBuf[i] = 0x8000 | rand() % 16383;
		for (uint32_t i = chars - sWEndTag.length(); i < chars; i++)
			pStringBuf[i] = 0x8000 | rand() % 16383;


		Obfuscate(pStringBuf, chars);

#ifdef _DEBUG
		//Casting a char to wchar will just wrap around at the 256 mark.
		//This is handy cause at least printable characters are showing,
		//Most of the time it's in a range with no characters if we use wchar
		//Uncomment for debugging purposes
		std::wcout << "Output string: \"";
		for (uint32_t i = 0; i < chars; i++)
		    std::wcout << (char)pStringBuf[i];
		std::wcout << "\"\n";

		std::wcout << L"Obfuscation Complete - New string written.\n";  
#endif // DEBUG

	}
}

bool StringScanner::Scan(const arguments& args)
{
    std::string sStartTagLocal;
    std::string sEndTagLocal;
    std::wstring sWStartTagLocal;
    std::wstring sWEndTagLocal;

    srand(clock());

/*    // test bit shifts
    uint8_t nTest = 0x80 | 0x01;
    for (int i = 0; i < 16; i++)
    {
        nTest = ShiftBits(nTest);
    }*/


/*    if (bUnobfuscate)
    {
        // if the command is to unobfuscate then we must search for obfuscated strings

        // obfuscate the tags so we know what to look for
        int nLength = sStartTag.length();
        uint8_t* pTagBuf = new uint8_t[nLength];        // assuming that start and end tags are the same length

        // Obfuscate start tag
        memcpy((void*)pTagBuf, sStartTag.data(), nLength);
        Obfuscate(pTagBuf, nLength);
        sStartTagLocal.assign((const char*) pTagBuf, nLength);

        // Obfuscate end tag
        memcpy((void*)pTagBuf, sEndTag.data(), nLength);
        Obfuscate(pTagBuf, nLength);
        sEndTagLocal.assign((const char*) pTagBuf, nLength);

        delete[] pTagBuf;
    }
    else*/
    {
        sStartTagLocal = sStartTag;
        sEndTagLocal = sEndTag;
        sWStartTagLocal = sWStartTag;
        sWEndTagLocal = sWEndTag;
    }

    std::fstream hScanFile(args.fileName, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
    if (hScanFile.is_open() == false)
    {
        FillError(L"Couldn't open file!");
		return false;
	}

	const size_t nTotalBytes = hScanFile.tellg();
	hScanFile.seekg(0, std::ios::beg);
    mnTotalStringsObfuscated = 0;

    const uint32_t nStartTagLength = sStartTagLocal.length();
    const uint32_t nEndTagLength = sEndTagLocal.length();
    const uint32_t nWStartTagLength = sWStartTagLocal.length()*sizeof(wchar_t);
    const uint32_t nWEndTagLength = sWEndTagLocal.length()*sizeof(wchar_t);

    // Initial implementation is going to be stupid and read the whole file in.  If we're ever obfuscating strings in large files turn this into an iterative buffer
	char* pBuf = new char[nTotalBytes];
    if (! hScanFile.read(pBuf, nTotalBytes))
    {
        FillError(L"Couldn't read file!");
        return false;
    }

	if (args.backUpFile)
	{
		std::wcout << "Making backup file...\n";
#ifdef WIN32
		std::wstring sBackupFilename = args.fileName + L"_backup";
#else
		std::string sBackupFilename = args.fileName + "_backup";
#endif
        std::fstream backupFile(sBackupFilename, std::ios::out | std::ios::binary | std::ios::trunc);
        backupFile.write(pBuf, nTotalBytes);
		backupFile.close();
	}

	std::vector<std::pair<int, int>> offsets;
	std::vector<std::pair<int, int>> woffsets;
    for (uint32_t i = 0; i < nTotalBytes - nStartTagLength - nEndTagLength; i++)
    {
        if (memcmp((void*)(pBuf + i), sStartTagLocal.data(), nStartTagLength) == 0)      // found a start tag
        {
            // find the end tag
            for (uint32_t j = i; j < nTotalBytes; j++)
            {
                if (memcmp((void*) (pBuf + j), sEndTagLocal.data(), nEndTagLength) == 0)
                {
                    j += nEndTagLength;     // advance j past the end tag

                    // Obfuscate everything from the beginning of the start tag to the end of the end tag
					offsets.push_back({ i, j - i });

                    mnTotalStringsObfuscated++;

                    i = j;  // advance i past the end tag
                    break;
                }
            }            
        }	
        
		if (memcmp((void*)(pBuf + i), sWStartTagLocal.data(), nWStartTagLength) == 0)      // found a start tag
        {
            // find the end tag
            for (uint32_t j = i; j < nTotalBytes; j++)
            {
				if (memcmp((void*)(pBuf + j), sWEndTagLocal.data(), nWEndTagLength) == 0)
				{
					j += nWEndTagLength;     // advance j past the end tag

					// Obfuscate everything from the beginning of the start tag to the end of the end tag
					woffsets.push_back({ i, j - i });
					mnTotalStringsObfuscated++;

					i = j;  // advance i past the end tag
					break;
				}
            }
        }
    }

	ObfuscateStringInBuffer(pBuf, offsets);
	ObfuscateWStringInBuffer(pBuf, woffsets);

	hScanFile.seekg(0, std::ios::beg);
	if (!hScanFile.write(pBuf, nTotalBytes))
	{
		std::wcout << "Cant write output file - Strings are not going to be obfuscated\n";
		return false;
	}

	delete[] pBuf;
	pBuf = NULL;

    return true;
}
