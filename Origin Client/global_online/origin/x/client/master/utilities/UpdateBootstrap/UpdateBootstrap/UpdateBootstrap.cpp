//////////////////////////////////////////////////////////////////////////////////////////////////
// UpdateBootstrap
// Copyright 2015 Electronic Arts
// Written by Alex Zvenigorodsky
//

#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <locale>
#include <string>
#include "Win32CppHelpers.h"
#include <psapi.h>

using namespace std;

//#pragma comment(lib, "version.lib")

const int MAX_ATTEMPTS = 200;
const int SLEEP_TIME = 50;
const size_t PROCESS_IDS_LEN = 1024;

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
    //ParseCommand(L"-file:", argc, argv, sFilename);
}

BOOL fileExists(std::wstring file)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE handle = FindFirstFile(file.c_str(), &FindFileData);
    BOOL found = handle != INVALID_HANDLE_VALUE;
    if (found)
    {
        FindClose(handle);
    }
    return found;
}

/*
* Logs file attributes
*/
void logFileAttributes(const wchar_t * const fileToProcess, const DWORD attributes)
{
    wstring data;

    if (attributes & FILE_ATTRIBUTE_ARCHIVE)
        data = L"FILE_ATTRIBUTE_ARCHIVE: ";

    if (attributes & FILE_ATTRIBUTE_COMPRESSED)
        data = L"FILE_ATTRIBUTE_COMPRESSED: ";

    if (attributes & FILE_ATTRIBUTE_DEVICE)
        data = L"FILE_ATTRIBUTE_DEVICE: ";

    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        data = L"FILE_ATTRIBUTE_DIRECTORY: ";

    if (attributes & FILE_ATTRIBUTE_ENCRYPTED)
        data = L"FILE_ATTRIBUTE_ENCRYPTED: ";

    if (attributes & FILE_ATTRIBUTE_HIDDEN)
        data = L"FILE_ATTRIBUTE_HIDDEN: ";

    if (attributes & FILE_ATTRIBUTE_NORMAL)
        data = L"FILE_ATTRIBUTE_NORMAL: ";

    if (attributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
        data = L"FILE_ATTRIBUTE_NOT_CONTENT_INDEXED: ";

    if (attributes & FILE_ATTRIBUTE_OFFLINE)
        data = L"FILE_ATTRIBUTE_OFFLINE: ";

    if (attributes & FILE_ATTRIBUTE_READONLY)
        data = L"FILE_ATTRIBUTE_READONLY: ";

    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
        data = L"FILE_ATTRIBUTE_REPARSE_POINT: ";

    if (attributes & FILE_ATTRIBUTE_SPARSE_FILE)
        data = L"FILE_ATTRIBUTE_SPARSE_FILE: ";

    if (attributes & FILE_ATTRIBUTE_SYSTEM)
        data = L"FILE_ATTRIBUTE_SYSTEM: ";

    if (attributes & FILE_ATTRIBUTE_TEMPORARY)
        data = L"FILE_ATTRIBUTE_TEMPORARY: ";

    if (attributes & FILE_ATTRIBUTE_VIRTUAL)
        data = L"FILE_ATTRIBUTE_VIRTUAL: ";

    data = data + fileToProcess;
    wcout << data;
}

/*
* gets and set file attribute to the passed parameter
*/
BOOL setFileAttribute(const wchar_t * const fileToProcess, const DWORD fileAttribToSet)
{
    const DWORD attributes = GetFileAttributes(fileToProcess);
    if (INVALID_FILE_ATTRIBUTES == attributes)
    {
        wcout << L"INVALID_FILE_ATTRIBUTES: " << fileToProcess;
        return FALSE;
    }

    logFileAttributes(fileToProcess, attributes);

    BOOL res = FALSE;
    if (attributes & FILE_ATTRIBUTE_READONLY)
    {
        // This file is read-only for some reason, try to set it to something more manageable
        res = SetFileAttributes(fileToProcess, fileAttribToSet);
    }

    if (!res)
    {
        wcout << L"SetFileAttributes failed.";
    }
    return res;
}


/*
* deletes file
*/
BOOL deleteFile(const wchar_t * const fileToDelete)
{
    int attempts = 0;
    // It's possible that a virus scanner or other indexing tool will open a handle to Origin
    // This will keep us from deleting it. Attempt to delete it a few times before giving up
    BOOL success = false;
    do
    {
        success = DeleteFile(fileToDelete);
        if (!success)
        {
            switch (GetLastError())
            {
            case ERROR_FILE_NOT_FOUND:
            {
                // file does not exist. Get out
                wcout << L"File not found: " << fileToDelete;
                success = true;
                break;
            }
            break;
            default:
            {
                setFileAttribute(fileToDelete, FILE_ATTRIBUTE_NORMAL);
            }
            break;
            }
        }
        else
        {
            break;
        }

        attempts++;
        wcout << L"Attempting deletion operation: " << fileToDelete << attempts << L"\n";
        Sleep(SLEEP_TIME);
    } while ((attempts != MAX_ATTEMPTS) && !success);

    if (success)
    {
        wcout << L"SUCCESFULLY Deleted: " << fileToDelete << attempts << L"\n";
    }
    else
    {
        wcout << L"Deletion operation FAILED for : " << fileToDelete << attempts << L"\n";
    }
    return success;
}

/*
* tries to move file
*/
BOOL moveFile(const wchar_t * const fileFrom, const wchar_t * const fileTo)
{
    BOOL success = false;
    int attempts = 0;
    do
    {
        success = MoveFileEx(fileFrom, fileTo, MOVEFILE_REPLACE_EXISTING);
        if (!success)
        {
            setFileAttribute(fileFrom, FILE_ATTRIBUTE_NORMAL);
            setFileAttribute(fileTo, FILE_ATTRIBUTE_NORMAL);
        }
        else
        {
            break;
        }
        attempts++;
        wcout << L"Attempting moving operation from " << fileFrom << L" to " << fileTo << attempts << L"\n";
        Sleep(SLEEP_TIME);
    } while ((attempts != MAX_ATTEMPTS) && !success);

    if (success)
    {
        wcout << L"SUCCESFULLY moved from: " <<  fileFrom  << L" to " << fileTo <<  attempts << L"\n";
    }
    else
    {
       wcout << L"FAILED Moving operation from " << fileFrom << L" to " <<  fileTo << attempts << L"\n";
    }
    return success;
}

BOOL copyFile(const wchar_t * const fileFrom, const wchar_t * const fileTo)
{
    BOOL success = false;
    int attempts = 1;
    do
    {
        success = CopyFile(fileFrom, fileTo, FALSE);
        if (success)
        {
            break;
        }
        else
        {
            DWORD nError = GetLastError();
            wcout << "Error copying file.  Error code " << nError << "\n";
            setFileAttribute(fileTo, FILE_ATTRIBUTE_NORMAL);
            wcout << L"Retrying copy operation from " << fileFrom << L" to " << fileTo << attempts << "\n";
            attempts++;
            Sleep(SLEEP_TIME);
        }
    } while ((attempts != MAX_ATTEMPTS) && !success);

    if (success)
    {
        wcout << L"SUCCESFULLY copied from: " << fileFrom << L" to " <<  fileTo << attempts << L"\n";
    }
    else
    {
        wcout << L"FAILED Copy operation from " <<  fileFrom << L" to " << fileTo << attempts << L"\n";
    }
    return success;
}


bool isBootstrapOlderVersion(std::wstring originFolder)
{
    std::wstring originPath = originFolder + L"Origin.exe";

    DWORD hnd;
    DWORD size = GetFileVersionInfoSize(originPath.c_str(), &hnd);

    // If we can't get version info then assume we didn't find OriginClient.dll
    if (size == 0)
    {
        wcout << "Couldn't find a valid Origin.exe to use";
        return false;
    }
    else
    {
        BYTE* versionInfo = new BYTE[size];
        if (GetFileVersionInfo(originPath.c_str(), hnd, size, versionInfo))
        {
            VS_FIXEDFILEINFO*   vsfi = NULL;
            UINT len = 0;
            VerQueryValue(versionInfo, L"\\", (void**)&vsfi, &len);
            WORD first = HIWORD(vsfi->dwFileVersionMS);
            WORD second = LOWORD(vsfi->dwFileVersionMS);
            WORD third = HIWORD(vsfi->dwFileVersionLS);
            WORD fourth = LOWORD(vsfi->dwFileVersionLS);

            // Check whether this version is 9.5.6 or older
            if (first < 9)
                return true;
            if (first > 9)
                return false;

            if (second < 5)
                return true;
            if (second > 5)
                return false;

            if (third < 6)
                return true;
            if (third > 6)
                return false;
        }
    }

    return false;
}


bool getBinaryVersionTag(std::wstring sFileName, std::wstring sTagName, std::wstring& sTagValue)
{
    DWORD versionSize = GetFileVersionInfoSize(sFileName.c_str(), NULL);
    if (versionSize == 0)
    {
#ifdef _DEBUG
        wprintf(L"Couldn't load binary from: %s\n.", sFileName.c_str());
#endif  
        return false;
    }

    AutoLocalHeap<LPBYTE> versionInfoBuffer((LPBYTE)LocalAlloc(LPTR, versionSize));
    if (GetFileVersionInfo(sFileName.c_str(), NULL, versionSize, (LPVOID)versionInfoBuffer.data()) == FALSE)
    {
#ifdef _DEBUG
        wprintf(L"Couldn't load verion data from: %s\n.", sFileName.c_str());
#endif
        return false;
    }

    // Structure used to store enumerated languages and code pages
    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;
    UINT cbTranslate = 0;

    // Read the list of languages and code pages
    VerQueryValue((LPVOID)versionInfoBuffer.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate);

    // Read the file description for each language and code page
    for (size_t c = 0; c < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); ++c)
    {
        // Construct the sub-block 'address'
        wchar_t subBlock[256] = { 0 };
        wsprintfW(subBlock, L"\\StringFileInfo\\%04x%04x\\%s", lpTranslate[c].wLanguage, lpTranslate[c].wCodePage, sTagName.c_str());

        // Retrieve the sub-block value 
        UINT cbBuffer = 0;
        LPBYTE lpbuffer = NULL;
        if (VerQueryValue((LPVOID)versionInfoBuffer.data(), subBlock, (LPVOID*)&lpbuffer, &cbBuffer) != FALSE)
        {
#ifdef _DEBUG
            wprintf(L"Found value %s = %s\n", sTagName.c_str(), (LPCWSTR)lpbuffer);
#endif
            sTagValue = (LPCWSTR)lpbuffer;
            return true;
        }
    }

    return false;
}

bool findOriginInstallFromRegistry(std::wstring &originLocation)
{
    HKEY key = NULL;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Origin", 0, KEY_READ, &key) != ERROR_SUCCESS)
    {
        wcout << L"Unable to open registry key: HKLM\\Software\\Origin";
        return false;
    }

    wchar_t clientPathKey[1024] = { 0 };
    DWORD cbClientPathKey = sizeof(clientPathKey) - 1;
    if (RegQueryValueEx(key, L"ClientPath", NULL, NULL, (LPBYTE)clientPathKey, &cbClientPathKey) != ERROR_SUCCESS)
    {
        wcout <<  L"Unable to read registry key: HKLM\\Software\\Origin\\ClientPath";
        RegCloseKey(key);
        return false;
    }

    // Clean up
    RegCloseKey(key);

    originLocation = clientPathKey;

#ifdef _DEBUG
    wprintf(L"Found client path reg key: %s\n", originLocation.c_str());
#endif

    // Remove executable
    int lastSlash = originLocation.find_last_of(L"\\");
    originLocation = originLocation.substr(0, lastSlash + 1);

    // If the file the registry points to is there
    if (fileExists(clientPathKey))
        return true;

    wcout << L"Unable to find Origin.exe";
    return false;
}

bool writeStagedSuccessRegistryKey(std::wstring originFolder)
{
    HKEY key = NULL;
    wchar_t clientPathKey[1024] = { 0 };
    DWORD cbClientPathKey = sizeof(clientPathKey) - 1;

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Origin", 0, NULL, 0, KEY_WRITE, NULL, &key, NULL) != ERROR_SUCCESS)
    {
        wcout << L"Unable to create registry key: HKLM\\Software\\Origin";
        return false;
    }

    if (RegSetValueEx(key, L"staged", 0, REG_SZ, (LPBYTE) originFolder.c_str(), originFolder.length()*sizeof(wchar_t)) != ERROR_SUCCESS)
    {
        wcout << L"Unable to write new staged key";
        return false;
    }

    RegCloseKey(key);
    return true;
}

bool findRunningOriginFolder(std::wstring &originLocation)
{
    bool originFound = false;
    wchar_t originFilename[MAX_PATH] = { 0 };
    DWORD cProcesses = 0;
    DWORD processIds[PROCESS_IDS_LEN] = { 0 };
    if (EnumProcesses(processIds, PROCESS_IDS_LEN*sizeof(DWORD), &cProcesses))
    {
        cProcesses /= sizeof(DWORD);
        for (DWORD i = 0; i < cProcesses; i++)
        {
            HANDLE  hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processIds[i]);
            if (hProcess != NULL)
            {
                wchar_t baseName[MAX_PATH] = { 0 };
                if (GetModuleBaseName(hProcess, NULL, baseName, MAX_PATH) > 0)
                {
                    if (wcscmp(baseName, L"Origin.exe") == 0)
                    {
                        if (GetModuleFileNameEx(hProcess, NULL, originFilename, sizeof(originFilename)))
                        {
                            originFound = true;

                            wcout << L"Origin running at: " << originFilename;
                            break;
                        }
                    }
                }
                CloseHandle(hProcess);
            }
        }
    }

    // We can't continue if Origin isn't running
    if (!originFound)
    {
        wcout << L"No running Origin process found, searching the registry instead...";
        return findOriginInstallFromRegistry(originLocation);
    }

    originLocation = originFilename;

    // Remove executable
    int lastSlash = originLocation.find_last_of(L"\\");
    originLocation = originLocation.substr(0, lastSlash + 1);

    return true;
}

bool findNewOriginPath(std::wstring originFolder, std::wstring& newOriginPath)
{
    newOriginPath = originFolder + L"staged/Origin.exe";
    if (!fileExists(newOriginPath))
    {
        wcout << L"Unable to find staged Origin " << newOriginPath.c_str();
        return false;
    }

    return true;
}


bool renameRunningOrigin(std::wstring originFolder)
{
    wchar_t originLocation[MAX_PATH] = { 0 };
    wchar_t originOldLocation[MAX_PATH] = { 0 };
    wsprintfW(originLocation, L"%sOrigin.exe", originFolder.c_str());
    wsprintfW(originOldLocation, L"%sOriginOLD.exe", originFolder.c_str());

    if (fileExists(originLocation))
    {
        wcout << originLocation << L" exists.";
    }
    else
    {
        wcout << originLocation << L" does not exist.";
        return false;
    }

    wcout << L"Processing " << originLocation;
    wcout << "Setting up new Origin";

    if (fileExists(originOldLocation) && !deleteFile(originOldLocation))
    {
        wcout << L"Unable to remove OriginOLD.exe before swap";
        return false;
    }

    if (!moveFile(originLocation, originOldLocation))
    {
        wcout << L"Unable to move Origin.exe to OriginOLD.exe";
        return false;
    }


    return true;
}

int _tmain(int argc, _TCHAR* argv[])
{
	DWORD nStartTime = ::GetTickCount();

	ParseCommands(argc, argv);

	wcout << "***************************\n";

/*    if (argc < 2 || sFilename.empty())
	{
		wcout << "Usage: UpdateBootstrap -file:FILENAME [-backup]\n";
        wcout << "\nIf -backup is specified, the original file will be copied to a _backup appended filename ONLY if any strings were found to obfuscate.\n";
        wcout << "\n The strings to be obfuscated MUST be surrounded by the exact tags: \"**OB_START**\" and \"***OB_END***\"\n\n";
		wcout << "***************************\n";
		return -1;
	}

	wcout << "file:         " << sFilename.c_str() << "\n";
	wcout << "\n";*/

    std::wstring originFolder;
    if (!findRunningOriginFolder(originFolder))
        return -1;

    std::wstring newOriginSourcePath;
    if (!findNewOriginPath(originFolder, newOriginSourcePath))
        return -2;

    if (isBootstrapOlderVersion(originFolder))
    {
        wcout << L"Replacing Bootstrap.\n";
        if (!renameRunningOrigin(originFolder))
            return -3;

        std::wstring newOriginDestPath = originFolder + L"Origin.exe";
        if (!copyFile(newOriginSourcePath.c_str(), newOriginDestPath.c_str()))
            return -4;
    }
    else
    {
        wcout << L"Bootstrap doesn't need replacing.\n";
    }

    // Registry key will be written whether it's because the EXE was replaced or because the bootstrap is already newer
    writeStagedSuccessRegistryKey(originFolder);

	return 0;
}









