#include "StringScanner.h"
#include <iostream>
#include <bitset>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileUtil.h>

using namespace std;

extern string sStartTag;
extern string sEndTag;
extern bool bBackup;


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

    cout << sAppError;
    cout << sOSError;
}

uint32_t StringScanner::ShiftBits(uint32_t nValue)
{
    // 10000000 = 0x80
    // 01111111 = 0x7F

    // Preserving the high order bit 0x80
    // Shift all of the other bits over
    uint8_t nLowBit = nValue&0x01;
    uint32_t nNewValue = 0x80 | (nValue&0x7F)>>1 | (nLowBit<<6);

//    cout << "before:" << std::bitset<8>(nValue) << "after:" << std::bitset<8>(nNewValue) << "\n";

    return nNewValue;
}

void StringScanner::Obfuscate(uint8_t* pBuf, uint32_t nBytes)
{
    if (nBytes < 24)
    {
        wcout << "Not enough chars to obfuscate!";
        return;
    }

    uint8_t nXOR = pBuf[0];      // first and last 12 bytes are randomized characters

    // modify our xor value by 23 possible values based on the length of the string.
    // This makes the obfuscation xor different for every string.
    //    uint8_t nXOR = 0xed ^ (nBytes%23);      


    for (uint32_t i = 12; i < nBytes-12; i++)       // no need to obfuscate tags
    {
        if (pBuf[i] != 0)               // leave nulls alone
            pBuf[i] = pBuf[i] ^ nXOR;

        nXOR = ShiftBits(nXOR);
    }
}

// The following function takes a file to change and an offset and number of bytes to obfuscate
void StringScanner::ObfuscateStringInFile(const std::wstring& sFilename, uint64_t nOffset, uint32_t nBytes)
{
    wcout << "Obfuscating string in file " << sFilename << " at offset " << nOffset << "\n";

    if (nBytes < sStartTag.length() + sEndTag.length())
    {
        wcout << "Can't obfuscate string that is not at least surrounded with start/end tags.\n";
        return;
    }


    EA::IO::FileStream hScanFile(sFilename.c_str());
    if (hScanFile.Open(EA::IO::kAccessFlagReadWrite, EA::IO::kCDOpenExisting, EA::IO::FileStream::kShareNone) != true)
    {
        FillError(L"Couldn't open file!");
        return;
    }

    if (hScanFile.SetPosition(nOffset) != true || hScanFile.GetPosition() != nOffset)
    {
        FillError(L"Couldn't set file pointer for reading existing string!");
        hScanFile.Close();
        return;
    }

    uint8_t* pStringBuf = new uint8_t[nBytes];

    hScanFile.Read((void*)pStringBuf, nBytes);

    cout << "Input string: \"";
    for (uint32_t i = 0; i < nBytes; i++)
        cout << (char)pStringBuf[i];
    cout << "\"\n";

    // obliterate start and end tags
    for (uint32_t i = 0; i < sStartTag.length(); i++)
        pStringBuf[i] = 0x80 | rand() % 127;
    for (uint32_t i = nBytes - sEndTag.length(); i < nBytes; i++)
        pStringBuf[i] = 0x80 | rand() % 127;


    Obfuscate(pStringBuf, nBytes);

    cout << "Output string: \"";
    for (uint32_t i = 0; i < nBytes; i++)
        cout << (char)pStringBuf[i];
    cout << "\"\n";





    if (hScanFile.SetPosition(nOffset) != true || hScanFile.GetPosition() != nOffset)
    {
        FillError(L"Couldn't set file pointer for writing obfuscated string!");
        hScanFile.Close();
        return;
    }

    if (hScanFile.Write(pStringBuf, nBytes) != true)
    {
        FillError(L"Couldn't write new string!");
    }

    hScanFile.Flush();
    hScanFile.Close();

    wcout << L"Obfuscation Complete - New string written.\n";
}

bool StringScanner::Scan(const std::wstring& filePath)
{
    string sStartTagLocal;
    string sEndTagLocal;
    bool bBackupDone = false;       // if any strings are find to obfuscate and the user requested a backup, make one

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
    }

    EA::IO::FileStream hScanFile(filePath.c_str());
    if (hScanFile.Open(EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, EA::IO::FileStream::kShareRead) != true)
    {
        FillError(L"Couldn't open file!");
		return false;
	}

	size_t nTotalBytes = hScanFile.GetSize();
    mnTotalStringsObfuscated = 0;

    uint32_t nStartTagLength = sStartTagLocal.length();
    uint32_t nEndTagLength = sEndTagLocal.length();

    // Initial implementation is going to be stupid and read the whole file in.  If we're ever obfuscating strings in large files turn this into an iterative buffer
	char* pBuf = new char[nTotalBytes];
    if (hScanFile.Read(pBuf, nTotalBytes) != nTotalBytes)
    {
        FillError(L"Couldn't read file!");
        return false;
    }

    hScanFile.Close();

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

                    if (bBackup && !bBackupDone)
                    {
                        // No backups made yet and we're about to modify the original file.....make one now.
                        std::wstring sBackupFilename = filePath + L"_backup";
                        EA::IO::File::Copy(filePath.c_str(), sBackupFilename.c_str());
                        bBackupDone = true;
                    }

                    ObfuscateStringInFile(filePath, i, j - i);
                    mnTotalStringsObfuscated++;

                    i = j;  // advance i past the end tag
                    break;
                }
            }            
        }
    }

	delete[] pBuf;
	pBuf = NULL;

    return true;
}
