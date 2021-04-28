///////////////////////////////////////////////////////////////////////////////
// HELPERS_H.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Helpers.h"

#ifdef ORIGIN_PC

#include "IGOLogger.h"
#include "../IGOProxy/md5.h"
#include "madCHook.h"
#include <Tlhelp32.h>

namespace OriginIGO
{

extern HINSTANCE gInstDLL; 


bool DirectoryExists(LPCWSTR path)
{
	DWORD dwAttrib = GetFileAttributes(path);
	bool exists = dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);

	return exists;
}

bool GetCommonDataPath(WCHAR buffer[MAX_PATH])
{
	if (!buffer)
		return false;

	ZeroMemory(buffer, sizeof(buffer));

	if (!GetEnvironmentVariableW(L"IGOLogPath", buffer, MAX_PATH))
	{
        if (!GetEnvironmentVariableW(L"ProgramData", buffer, MAX_PATH))
            return false;
	}

	return buffer[0] != '\0';
}

bool ReadFileContent(LPCWSTR fileName, char** buffer, size_t* bufferSize)
{
	if (!buffer || !bufferSize)
		return false;

	bool retVal = false;
	*buffer = NULL;
	*bufferSize = 0;
	HANDLE fd = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fd != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER fileSizeInfo;
		if (GetFileSizeEx(fd, &fileSizeInfo))
		{
            DWORD fileSize = static_cast<DWORD>(fileSizeInfo.QuadPart);
			char* data = new char[fileSize];
			ZeroMemory(data, fileSize);

			DWORD readSize = 0;
			BOOL success = ReadFile(fd, data, static_cast<DWORD>(fileSize), &readSize, NULL);
			if (success && readSize == fileSize)
			{
				retVal = true;
				*buffer = data;
				*bufferSize = readSize;
			}

			else
            {
                IGOLogWarn("File '%S' doesn't have expected size (%d/%d)", fileName, readSize, fileSize);
				delete[] data;
            }
		}

		CloseHandle(fd);
	}
    else
        IGOLogWarn("Unable to open file '%S'", fileName);

	return retVal;
}

bool ComputeFileMd5(HMODULE module, char md5[33])
{
	bool retVal = false;

	WCHAR fileName[MAX_PATH];
	DWORD fileNameLen = GetModuleFileName(module, fileName, MAX_PATH);
	if (fileNameLen == 0 || fileNameLen == MAX_PATH)
		return false;

	char* data = NULL;
	size_t dataSize = 0;
	if (ReadFileContent(fileName, &data, &dataSize))
	{
		calculate_md5(data, static_cast<unsigned int>(dataSize), md5);
		delete[] data;

		retVal = true;
	}

	return retVal;
}

void* ValidateMethod(HMODULE module, uint64_t offset, OriginIGO::TelemetryRenderer renderer)
{
	IMAGE_NT_HEADERS* imageHeader = GetImageNtHeaders(module);
	ULONG imageSize = GetSizeOfImage(imageHeader);

	if (offset > imageSize)
    {
        IGOLogWarn("Method offset larger than image size! (%ull/%d)", offset, imageSize);
		return NULL;
    }

	if (offset < 2)
    {
        IGOLogWarn("Method offset too small! (%ull)", offset);
		return NULL;
    }

  	uint64_t addr = reinterpret_cast<uint64_t>(module)+offset;
    IGOLogWarn("Analyzing addr=0x%016llx (0x%016llx)", addr, offset); 

    unsigned char* data = reinterpret_cast<unsigned char*>(addr);
    char opcodes[128] = {0};
    StringCchPrintfA(opcodes, _countof(opcodes), "Opcodes: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
    IGOLogWarn(opcodes);

    // Do we see the "usual" method startup code? (doesn't mean much, but collecting in case it could help direct the debugging of some problem in the future)
    if (Is64bitOS())
    {
        // Check that we can find the mandatory padding of 'int 3' opcodes (0xcc) before the method.
	    const unsigned char paddingByteCode = 0xcc;
	    unsigned char* prefix = reinterpret_cast<unsigned char*>(addr - 2);
	    if (prefix[0] != paddingByteCode || prefix[1] != paddingByteCode)
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_INFO, OriginIGO::TelemetryContext_RESOURCE, renderer, "Missing int3 prefix (0x%02x 0x%02x) - preamble:'%s'", prefix[0], prefix[1], opcodes));
    }

    else
    {
        // Do we see the "usual" method startup code? (doesn't mean much, but collecting in case it could help direct the debugging of some problem in the future)
        // 8B FF    - mov edi, edi (for hot patching)
        // 55       - push ebp
        // 8B EC    - mov ebp, esp
        // 81 EC ...- sub esp, ...
        if (data[0] != 0x8B || data[1] != 0xFF || data[2] != 0x55 || data[3] != 0x8B || data[4] != 0xEC || data[5] != 0x81 || data[6] != 0xEC)
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_INFO, OriginIGO::TelemetryContext_RESOURCE, renderer, "Unexpected pre-computed function preamble:'%s'", opcodes));
    }

    // Not sure what useful check I could do at this point... 
	return data;
}

void SaveRawRGBAToTGA(LPCWSTR fileName, size_t width, size_t height, const unsigned char* data, bool overwrite)
{
	if (!overwrite)
	{
		DWORD dwAttrib = GetFileAttributes(fileName);
		if (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0)
			return;
	}

	HANDLE fd = CreateFile(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fd != INVALID_HANDLE_VALUE)
	{
		unsigned char defaultValueChar = 0;
		short int defaultValueInt = 0;

		WriteFile(fd, &defaultValueChar, sizeof(unsigned char), NULL, NULL);     // idlength: no identification string
		WriteFile(fd, &defaultValueChar, sizeof(unsigned char), NULL, NULL);     // colourmaptype: no color map

		unsigned char imageType = 2;
		WriteFile(fd, &imageType, sizeof(unsigned char), NULL, NULL);            // datatypecode: 2 = uncompressed RGB

		WriteFile(fd, &defaultValueInt, sizeof(short int), NULL, NULL);          // colourmaporigin
		WriteFile(fd, &defaultValueInt, sizeof(short int), NULL, NULL);          // colourmaplength
		WriteFile(fd, &defaultValueChar, sizeof(unsigned char), NULL, NULL);     // colourmapdepth
		WriteFile(fd, &defaultValueInt, sizeof(short int), NULL, NULL);          // x_origin
		WriteFile(fd, &defaultValueInt, sizeof(short int), NULL, NULL);          // y_origin

		WriteFile(fd, &width, sizeof(short int), NULL, NULL);                    // width
		WriteFile(fd, &height, sizeof(short int), NULL, NULL);                   // height

		unsigned char bits = 32;
		WriteFile(fd, &bits, sizeof(unsigned char), NULL, NULL);                 // bitsperpixel

		unsigned char descriptor = 0x28;
		WriteFile(fd, &descriptor, sizeof(unsigned char), NULL, NULL);           // imagedescriptor: Targa 32 (Bits 3-0) = 8
		// + Origin in upper left-hand corner (Bit5)

		int colorMode = 4;
		DWORD Size = static_cast<DWORD>(width * height * colorMode);
		WriteFile(fd, data, sizeof(unsigned char) * Size, NULL, NULL);

		CloseHandle(fd);
	}
}

const NotXPSupportedAPIs* GetNotXPSupportedAPIs()
{
    static bool initialized = false;
    static NotXPSupportedAPIs apis;

    if (!initialized)
    {
        initialized = true;
        apis.createEventEx = (NotXPSupportedAPIs::CreateEventExFn)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "CreateEventExW");
        if (!apis.createEventEx)
            TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_RESOURCE, TelemetryRenderer_DX12, "No CreateEventExW API found");
    }

    return &apis;
}

void* FindModuleAPI(LPCWSTR filter, LPCSTR apiName)
{
    if (!filter || !apiName)
        return NULL;


    HANDLE hModuleSnap;
    MODULEENTRY32W me32 = { 0 };

    // Take a snapshot of all modules in the specified process.
    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    if (hModuleSnap == INVALID_HANDLE_VALUE)
        return NULL;

    // Set the size of the structure before using it.
    me32.dwSize = sizeof(MODULEENTRY32W);

    // Retrieve information about the first module, and exit if unsuccessful
    if (!Module32FirstW(hModuleSnap, &me32))
    {
        CloseHandle(hModuleSnap);
        return NULL;
    }

    // Now walk the module list of the process, and use the filter to limit the number of modules to check for the API
    void* apiAddr = NULL;
    size_t filterCount = wcslen(filter);
    do
    {
        if (_wcsnicmp(me32.szModule, filter, filterCount) == 0)
            apiAddr = GetProcAddress(me32.hModule, apiName);

    } while (!apiAddr && Module32NextW(hModuleSnap, &me32));

    CloseHandle(hModuleSnap);
    return apiAddr;
}

bool LoadEmbeddedResource(int resourceID, const char** content, DWORD* contentSize)
{
    if (!content || !contentSize)
        return false;

    HRSRC handle = FindResource(gInstDLL, MAKEINTRESOURCE(resourceID), RT_RCDATA);
    if (handle)
    {
        HGLOBAL resource = LoadResource(gInstDLL, handle);
        if (resource)
        {
            *content = (const char *)LockResource(resource);
            *contentSize = SizeofResource(gInstDLL, handle);

            if (*content && (*contentSize) != 0)
                return true;

            else
            {
                IGOLogWarn("Found invalid resource=%d", resourceID);
                IGO_ASSERT(0);
            }
        }

        else
        {
            IGOLogWarn("Failed to load resource=%d (%d)", resourceID, GetLastError());
            IGO_ASSERT(0);
        }
    }

    else
    {
        IGOLogWarn("Missing resource=%d (%d)", resourceID, GetLastError());
        IGO_ASSERT(0);
    }

    return false;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

CodePatternFinder::CodePatternFinder(const PatternSignature* const patterns, size_t patternsCount)
	: mPatterns(patterns), mPatternsCount(patternsCount)
{
}

// CAREFUL: This is only implemented for 64-bit!
CodePatternFinder::FindInstanceResult CodePatternFinder::GetMethod(HMODULE module, void** method)
{
	if (!method)
		return FindInstanceResult_NONE;

	*method = NULL;

	// Grab its unique ID = its header
	IMAGE_NT_HEADERS* imageHeader = GetImageNtHeaders(module);
	IMAGE_FILE_HEADER moduleID = imageHeader->FileHeader;

	IGOLogInfo("CodePatternFinder - Lookup for moduleID %d, %d, %d, %d, %d, %d, %d", moduleID.Machine, moduleID.NumberOfSections, moduleID.TimeDateStamp, moduleID.PointerToSymbolTable, moduleID.NumberOfSymbols, moduleID.SizeOfOptionalHeader, moduleID.Characteristics);

	// Search for a matching module ID
	size_t patternIdx = (size_t)-1;
	for (size_t idx = 0; idx < mPatternsCount; ++idx)
	{
		if (mPatterns[idx].moduleID.Machine == moduleID.Machine
			&& mPatterns[idx].moduleID.NumberOfSections == moduleID.NumberOfSections
			&& mPatterns[idx].moduleID.TimeDateStamp == moduleID.TimeDateStamp
			&& mPatterns[idx].moduleID.PointerToSymbolTable == moduleID.PointerToSymbolTable
			&& mPatterns[idx].moduleID.NumberOfSymbols == moduleID.NumberOfSymbols
			&& mPatterns[idx].moduleID.SizeOfOptionalHeader == moduleID.SizeOfOptionalHeader
			&& mPatterns[idx].moduleID.Characteristics == moduleID.Characteristics)
		{
			if (patternIdx != static_cast<size_t>(-1))
			{
				IGOLogWarn("CodePatternFinder - Found multiple lookup entries that match the module ID");
				return FindInstanceResult_NONE;
			}

			patternIdx = idx;
		}
	}

	bool defaultEntry = false;
	if (patternIdx == static_cast<size_t>(-1))
	{
		// Check whether there is a default entry to use?
		for (size_t idx = 0; idx < mPatternsCount; ++idx)
		{
			if (mPatterns[idx].moduleID.Machine == moduleID.Machine
				&& (mPatterns[idx].moduleID.NumberOfSections| mPatterns[idx].moduleID.TimeDateStamp | mPatterns[idx].moduleID.PointerToSymbolTable
					| mPatterns[idx].moduleID.NumberOfSymbols | mPatterns[idx].moduleID.SizeOfOptionalHeader | mPatterns[idx].moduleID.Characteristics) == 0)
			{
				defaultEntry = true;
				patternIdx = idx;
				break;
			}
		}

		if (patternIdx == static_cast<size_t>(-1))
		{
			IGOLogWarn("CodePatternFinder - No lookup entry matches the module ID");
			return FindInstanceResult_NONE;
		}
	}

	if (defaultEntry)
		IGOLogInfo("CodePatternFinder - Using default lookup entry %d", patternIdx);

	else
		IGOLogInfo("CodePatternFinder - Using lookup entry %d", patternIdx);

	ULONG imageSize = GetSizeOfImage(imageHeader);

	// TODO: improve this to only search for code areas!
	unsigned char* curAddr = reinterpret_cast<unsigned char*>(module);
	unsigned char* endAddr = curAddr + imageSize - 3; // (because of our 3 bytes search for the next valid method)
	IGOLogDebug("CodePatternFinder - Search from %p to %p", curAddr, endAddr);

	size_t patternSize = mPatterns[patternIdx].patternSize;
	const unsigned char* pattern = mPatterns[patternIdx].patternBytes;
	const unsigned char* patternMask = mPatterns[patternIdx].maskBytes;

	// Are we to use the mask?
	unsigned char useMask = 0;
	for (size_t idx = 0; idx < patternSize; ++idx)
		useMask |= patternMask[idx];


	while (curAddr < endAddr)
	{
		// Look for the beginning of a new method (ie search for auto prefix padding)
		// 1) Search for start of prefix
		const unsigned char paddingByteCode = 0xcc; // int 3
		while ((curAddr < endAddr) && (curAddr[0] != paddingByteCode || curAddr[1] != paddingByteCode || curAddr[2] == paddingByteCode))
			++curAddr;

		if (curAddr < endAddr)
		{
			// New method to look into
			curAddr += 2;
			unsigned char* methodBaseAddr = curAddr;
#ifdef _DEBUG
			IGOLogDebug("CodePatternFinder - Found new method: %p", methodBaseAddr);
#endif

			// Search for our pattern until we get to the end of the method
			while ((curAddr < endAddr - patternSize) && (curAddr[0] != paddingByteCode || curAddr[1] != paddingByteCode))
			{
				size_t checkIdx = 0;
				if (useMask)
				{
					for (; checkIdx < patternSize; ++checkIdx)
					{
						unsigned char mask = patternMask[checkIdx];
						unsigned char byte = curAddr[checkIdx] & mask;
						if (byte != pattern[checkIdx])
							break;
					}
				}

				else
				{
					for (; checkIdx < patternSize; ++checkIdx)
					{
						if (curAddr[checkIdx] != pattern[checkIdx])
							break;
					}
				}

				if (checkIdx == patternSize)
				{
					// We found it!
					if (*method)
					{
						IGOLogWarn("CodePatternFinder - Found another instance of the pattern in %p - need to change the lookup pattern", methodBaseAddr);
						return FindInstanceResult_MULTIPLE;
					}

					IGOLogInfo("CodePatternFinder - Found pattern in method at %p", methodBaseAddr);
					*method = methodBaseAddr;
					break;
				}

				curAddr += checkIdx + 1;
			}

#ifdef _DEBUG
			// In debug, we keep on going to check that the pattern is unique to the DLL
			continue;
#else
			if (*method)
				break;
#endif
		}
	}

	if (*method)
		return FindInstanceResult_ONE;

	// Couldn't find the patter
	IGOLogWarn("CodePatternFinder - Couldn't pattern in the module");
	return FindInstanceResult_NONE;
}

} // OriginIGO

#else // ORIGIN_MAC

#include <CoreServices/CoreServices.h>

#endif // ORIGIN_PC

namespace OriginIGO
{

    // Helper to keep track of deltaT
    IntervalKeeper::IntervalKeeper()
        : mRefTime(0)
    {
#if defined(ORIGIN_MAC)
        mRefTime = UnsignedWideToUInt64(AbsoluteToNanoseconds(UpTime()));
#elif defined(ORIGIN_PC)
        LARGE_INTEGER ts;
        if (QueryPerformanceCounter(&ts))
            mRefTime = ts.QuadPart;

        LARGE_INTEGER freq;
        if (QueryPerformanceFrequency(&freq))
            mRefFrequency = (double)freq.QuadPart;
        else
            mRefFrequency = 1.0;
#endif
    }

    IntervalKeeper::~IntervalKeeper()
    {
    }

    void IntervalKeeper::Update()
    {
#if defined(ORIGIN_MAC)
        UInt64 time = UnsignedWideToUInt64(AbsoluteToNanoseconds(UpTime()));
        mDeltaInMS = (float)((time - mRefTime) / (UInt64)1000000);
        mRefTime = time;
#elif defined(ORIGIN_PC)
        LARGE_INTEGER ts;
        if (QueryPerformanceCounter(&ts))
        {
            mDeltaInMS = (float)((ts.QuadPart - mRefTime) * 1000.0 / mRefFrequency);
            mRefTime = ts.QuadPart;
        }

        else
            mDeltaInMS = 16; // Still return something - should never get there, but if we do at least the animation won't get stuck!
#endif
    }

} // OriginIGO

