///////////////////////////////////////////////////////////////////////////////
// main.cpp
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#pragma warning(disable: 4917) // GUID
#pragma warning(disable: 4548) // comma

#include <windows.h>
#include <string>
#include <psapi.h>
#include <memory>
#include <ShlObj.h>
#include <fstream>
#include <AclAPI.h>
#include <Sddl.h>
#include <vector>
#include <map>
#include "version/version.h"

#include <Softpub.h>	//for exe signing verification
#include <wincrypt.h>	//for exe signing verification 
#include <wintrust.h>	//for exe signing verification

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)
#define ORIGIN_CODESIGNING_SUBJECT_NAME L"Electronic Arts, Inc."

#pragma comment(lib, "wintrust.lib")	//for exe signing verification
#pragma comment(lib, "crypt32.lib")		//for cert info verification
#pragma comment(lib, "version.lib")

bool isValidEACertificate(LPCWSTR szBinaryFileName);

#include "Win32CppHelpers.h"

#define ARGS_BUFFER_SIZE 1024
#define FULL_CONTROL_MASK 0x001F01FF
#define ORIGIN_UPDATETOOL_MUTEX L"OriginUpdateTool"

using namespace std;

namespace
{
    const int MAX_ATTEMPTS = 200;
    const int SLEEP_TIME = 50;
    const size_t PROCESS_IDS_LEN = 1024;
}

wstring logFilePath()
{
    // If the file can't be opened the function will return an invalid path. The file wont be opened. 
    // The logging will silently fail but no other issues will appear i.e. the UpdateTool will continue.
    const wstring sLogFilename(L"\\Logs\\UpdateTool_Log.txt");
    std::wstring strLogFilePath;
    TCHAR szPath[MAX_PATH] = L"";
    
    if (SUCCEEDED( SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath))) 
    {
        strLogFilePath = szPath;
        strLogFilePath.append (L"\\Origin");
        strLogFilePath.append (sLogFilename);
    }

    return strLogFilePath;
}

class UTLogger
{
public:
    void logEntry(const std::wstring& data, int numberOfTimes, bool isShowError = false)
    {
        string tmp(data.begin(), data.end());
        logEntry(tmp, numberOfTimes, isShowError);
    }

    void logEntry(const std::wstring& data, bool isShowError = false)
    {
        string tmp(data.begin(), data.end());
        logEntry(tmp, isShowError);
    }

    void logEntry(const std::string& data, bool isShowError = false)
    {
        if (mFile.is_open())
        {
            timeStamp();
            mFile  << data.c_str();
            if (isShowError)
            {
                mFile  << ". Last error: [" << GetLastError() << "]";
                SetLastError(ERROR_SUCCESS);
            }
            mFile  << endl;

            printf("%s\n", data.c_str());
        }
    }

    void logEntry(const std::string& data, int numberOfTimes, bool isShowError = false)
    {
        if (mFile.is_open())
        {
            timeStamp();
            mFile  << data.c_str() << ". Number of times: [" << numberOfTimes << "]";
            if (isShowError)
            {
                mFile  << ". Last error: [" << GetLastError() << "]";
                SetLastError(ERROR_SUCCESS);
            }
            mFile  << endl;

            printf("%s\n", data.c_str());
        }
    }

    /// \fn instance
    /// \brief If necessary, creates and then returns a pointer to the required OriginClient.
     static UTLogger* instance()
     {
         if(!mMe.get()) 
         { 
             mMe.reset(new UTLogger); 
         } 
         return mMe.get(); 
     }
    ~UTLogger() 
    {
        logEntry("***** Ending");
        mFile.close();
    };
private:

    UTLogger()
    {
        wstring myFileName = logFilePath();
        mFile.open (myFileName.c_str(),std::ios::app);
        if(!mFile.is_open())
        {
            mFile.clear();
            mFile.open(myFileName.c_str(), std::ios::out); //Create file.
            mFile.close();
            mFile.open(myFileName.c_str());
        }
        std::string version = EALS_VERSION_P_DELIMITED;

        logEntry(std::string("Starting ; Update System 2 ; UpdateTool v") + version);
    }

    void timeStamp() 
    {
         GetSystemTime(&mSystemTime);
         mFile << "[" << mSystemTime.wYear << "/"<< mSystemTime.wMonth << "/" << mSystemTime.wDay 
             << " " << mSystemTime.wHour << ":" << mSystemTime.wMinute << ":" << mSystemTime.wSecond << "." << mSystemTime.wMilliseconds << "]\t" ;
    }

    /// \brief My singleton; private.
    static std::unique_ptr<UTLogger> mMe; 

    fstream mFile;
    SYSTEMTIME mSystemTime;

// disable these
    UTLogger(const UTLogger&);
    UTLogger& operator=(const UTLogger&);

};

/// \brief Definition of my singleton.
std::unique_ptr<UTLogger> UTLogger::mMe;

// Prototypes
BOOL deleteFile( const wchar_t * const fileToDelete );
BOOL moveFile(const wchar_t * const fileFrom, const wchar_t * const fileTo );
BOOL copyFile(const wchar_t * const fileFrom, const wchar_t * const fileTo );
bool isPlatformXP();
BOOL fileExists(const wchar_t * const file);
BOOL createFolderIfNotExist( const wchar_t * const folder);
bool setProgramDataPermissions();
void installDependencies(bool removeInstalledFiles);
void updateShortcuts();
void execProcess(std::wstring file, std::wstring args, bool runAs = false, bool blocking = false);

EXPLICIT_ACCESS eaForWellKnownGroup(PSID groupSID, bool fullControl = true)
{
    EXPLICIT_ACCESS ea={0};
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfAccessPermissions = (fullControl) ? GENERIC_ALL : GENERIC_READ;
    ea.grfInheritance = CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE;
    ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;  	
    ea.Trustee.ptstrName = (LPTSTR)groupSID;

    return ea;
}

bool setProgramDataPermissions()
{
    TCHAR szPath[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath))) 
        return false;

    std::wstring strProgramDataOrigin (szPath);
    strProgramDataOrigin.append (L"\\Origin");

    HANDLE hDir = CreateFile(strProgramDataOrigin.c_str(), READ_CONTROL|WRITE_DAC,0,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
    if (hDir == INVALID_HANDLE_VALUE) 
    {
        CloseHandle(hDir);	
        return false;
    }

    ACL* pOldDACL=NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    GetSecurityInfo(hDir,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,&pOldDACL,NULL,&pSD);

    PSID usersSID;
    PSID everyoneSID;

    static const LPWSTR UsersGroup = L"S-1-5-32-545";
    static const LPWSTR EveryoneGroup = L"S-1-1-0";

    if ((ConvertStringSidToSid(UsersGroup, &usersSID)) &&
        ConvertStringSidToSid(EveryoneGroup, &everyoneSID))
    {
        EXPLICIT_ACCESS newAccess[2];

        newAccess[0] = eaForWellKnownGroup(usersSID);
        newAccess[1] = eaForWellKnownGroup(everyoneSID);

        ACL* pNewDACL = NULL;

        if (SetEntriesInAcl(2, newAccess,pOldDACL,&pNewDACL) == ERROR_SUCCESS)
        {
            SetSecurityInfo(hDir,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,pNewDACL,NULL);
        }
        else
        {
            UTLogger::instance()->logEntry("SetEntriesInAcl failed", true);
        }

        LocalFree(pNewDACL);
    }
    else
    {
        UTLogger::instance()->logEntry("ConvertStringSidtoSid failed", true);
        LocalFree(pSD);
        CloseHandle(hDir);
        return false;
    }

    LocalFree(pSD);
    CloseHandle(hDir);

    return true;
}

std::wstring getSelfUpdateFolderPath()
{
    wchar_t szProgramDataPath[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szProgramDataPath))) 
        return std::wstring();

    std::wstring strProgramDataOrigin (szProgramDataPath);
    strProgramDataOrigin.append (L"\\Origin\\SelfUpdate\\");
    wstring updateLocation = strProgramDataOrigin;

    return updateLocation;
}

bool createProgramDataSelfUpdateFolder()
{
    wstring updateLocation = getSelfUpdateFolderPath();
    if (updateLocation.empty())
        return false;

    // Ensure our SelfUpdate working folder exists
    int dirResult = SHCreateDirectoryEx(NULL, updateLocation.c_str(), NULL);
    if (dirResult != ERROR_SUCCESS && dirResult != ERROR_ALREADY_EXISTS)
    {
        UTLogger::instance()->logEntry(std::wstring(L"DownloadUpdate: failed to create selfupdate folder: ") + updateLocation, true);
        return false;
    }
    return true;
}

bool setEmptyInheritableDacl(std::wstring fileFolder)
{
    UTLogger::instance()->logEntry(std::wstring(L"Set empty ACL for: ") + fileFolder);

    // Create a new empty ACL and apply it using SetNamedSecurityInfo (which propagates inherited ACEs automatically) and
    // we will specify the UNPROTECTED_DACL_SECURITY_INFORMATION flag, which indicates that the DACL inherits ACEs from its parent.

    DWORD newACLSize = sizeof(ACL);
    AutoLocalHeap<PACL> pNewACL;
    pNewACL = (PACL)LocalAlloc(LPTR, newACLSize);

    // Initialize the new empty ACL
    if (!InitializeAcl(pNewACL, newACLSize, ACL_REVISION))
    {
        UTLogger::instance()->logEntry(std::wstring(L"InitializeAcl failed."), true);
        return false;
    }

    // Get PSID for built-in Administrators well-known group
    AutoLocalHeap<PSID> pBuiltInAdministratorsGroupSid;
    static const LPWSTR BuiltInAdministratorsGroup = L"S-1-5-32-544";
    if (ConvertStringSidToSid(BuiltInAdministratorsGroup, &pBuiltInAdministratorsGroupSid) != TRUE)
    {
        UTLogger::instance()->logEntry(std::wstring(L"ConvertStringSidToSid failed for Built-In Administrators Group."), true);
        return false;
    }

    // Specify UNPROTECTED_DACL_SECURITY_INFORMATION to ensure the inheritance bits are set
    if (SetNamedSecurityInfo((LPWSTR)fileFolder.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION, pBuiltInAdministratorsGroupSid, NULL, pNewACL, NULL) != ERROR_SUCCESS)
    {
        UTLogger::instance()->logEntry(std::wstring(L"SetNamedSecurityInfo failed."), true);
        return false;
    }

    return true;
}

bool setFileFolderPermissions(std::wstring fileFolder, bool restrict, bool verbose = true)
{
    // Don't do this on XP
    if (isPlatformXP())
        return true;

    if (restrict)
        UTLogger::instance()->logEntry(std::wstring(L"Restrict permissions for: ") + fileFolder);
    else
        UTLogger::instance()->logEntry(std::wstring(L"Open permissions for: ") + fileFolder);

    if (restrict)
    {
        return setEmptyInheritableDacl(fileFolder);
    }

    // Get PSID for well-known group
    AutoLocalHeap<PSID> pAuthenticatedUsersSid;
    static const LPWSTR AuthenticatedUsersGroup = L"S-1-5-11";
    if (ConvertStringSidToSid(AuthenticatedUsersGroup, &pAuthenticatedUsersSid) != TRUE)
    {
        UTLogger::instance()->logEntry(std::wstring(L"ConvertStringSidToSid failed for Authenticated Users Group."), true);
        return false;
    }

    // Allocate memory for a new ACE for Authenticated Users/Full Control
    DWORD dwSidSize = GetLengthSid(pAuthenticatedUsersSid);
    DWORD dwAceSize = sizeof(ACCESS_ALLOWED_ACE) + dwSidSize - sizeof(DWORD);
    AutoBuffer<ACCESS_ALLOWED_ACE> pNewAce(dwAceSize);
    memset(pNewAce.data(), 0, dwAceSize);
    pNewAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    pNewAce->Header.AceSize = (WORD)dwAceSize;
    pNewAce->Header.AceFlags = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    pNewAce->Mask = FULL_CONTROL_MASK;
    if (CopySid(
                dwSidSize,
                (PSID)&pNewAce->SidStart,
                pAuthenticatedUsersSid
                ) == FALSE)
    {
        UTLogger::instance()->logEntry(std::wstring(L"CopySid failed."), true);
        return false;
    }

    // Allocate a new ACL with space for one ACE
    DWORD newACLSize = sizeof(ACL) + dwAceSize;
    newACLSize = newACLSize + (sizeof(DWORD) - 1) & 0xfffffffc; // DWORD align (according to MSDN)
    AutoLocalHeap<PACL> pNewACL;
    pNewACL = (PACL)LocalAlloc(LPTR, newACLSize);

    // Initialize the new empty ACL
    if (!InitializeAcl(pNewACL, newACLSize, ACL_REVISION))
    {
        UTLogger::instance()->logEntry(std::wstring(L"InitializeAcl failed."), true);
        return false;
    }

    // Insert the modified ACE into the DACL.
    if (AddAce(
                pNewACL,                  // Pointer to the DACL
                ACL_REVISION,             // For ACCESS_ALLOWED_ACE_TYPE
                0,                        // Index of the ACE (at the beginning)
                pNewAce,                  // Pointer to the new ACE
                dwAceSize                 // Length of the new ACE
                ) == FALSE)
    {
        UTLogger::instance()->logEntry(std::wstring(L"AddAce failed."), true);
        return false;
    }

#ifdef _DEBUG
    if (verbose)
        wprintf(L"Added Authenticated Users/Full Control ACE\n");
#endif

    // Allow inheritance with UNPROTECTED_DACL_SECURITY_INFORMATION
    if (SetNamedSecurityInfo((LPWSTR)fileFolder.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, pNewACL, NULL) != ERROR_SUCCESS)
    {
        UTLogger::instance()->logEntry(std::wstring(L"SetNamedSecurityInfo failed."), true);
        return false;
    }

    if (verbose)
        UTLogger::instance()->logEntry(std::wstring(L"Permissions modified successfully."));

    return true;
}

bool setFolderTreePermissions(const wchar_t* searchPath)
{
    bool success = true;
    wchar_t searchPattern[MAX_PATH] = {0};
    wsprintfW(searchPattern, L"%s*.*", searchPath);
    WIN32_FIND_DATAW findFileData;
    ZeroMemory(&findFileData, sizeof(findFileData));
    HANDLE handle = FindFirstFileW(searchPattern, &findFileData);
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            // Ignore the ./.. directories
            if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
                continue;

            wchar_t childPath[MAX_PATH] = {0};
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                wsprintfW(childPath, L"%s%s\\", searchPath, findFileData.cFileName);

                // Call the actual function on this directory first
                if (!setEmptyInheritableDacl(childPath))
                {
                    success = false;
                    break;
                }

                // Recurse
                if (!setFolderTreePermissions(childPath))
                {
                    success = false;
                    break;
                }
            }
            else
            {
                wsprintfW(childPath, L"%s%s", searchPath, findFileData.cFileName);

                // Call the actual function
                if (!setEmptyInheritableDacl(childPath))
                {
                    success = false;
                    break;
                }
            }
        }
		while (FindNextFileW(handle, &findFileData));

        FindClose(handle);
    }

    return success;
}

bool setOriginFolderPermissions(std::wstring originFolder, bool restrict)
{
    // Don't do this on XP
    if (isPlatformXP())
        return true;

    UTLogger::instance()->logEntry("Set origin folder permissions");

    // If a specific Origin wasn't specified, just assume we're running from the Origin directory
    if (originFolder.length() == 0)
    {
        // Get Origin install folder
        wchar_t path[MAX_PATH] = { 0 };
        GetModuleFileName(NULL, path, MAX_PATH);
        originFolder = path;
        // Remove executable
        int lastSlash = originFolder.find_last_of(L"\\");
        originFolder = originFolder.substr(0, lastSlash + 1);
    }

    if (!setFileFolderPermissions(originFolder, restrict))
        return false;

    // Recurse to child objects (only for restricting, because removing ACEs can leave orphaned non-inherited ACEs on child objects)
    // We will set an empty inheritable ACL on every child object
    if (restrict)
    {
        return setFolderTreePermissions(originFolder.c_str());
    }

    return true;
}

bool findOriginInstallFromRegistry(std::wstring &originLocation)
{
    HKEY key = NULL;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Origin", 0, KEY_READ, &key) != ERROR_SUCCESS)
    {
        UTLogger::instance()->logEntry(std::wstring(L"Unable to open registry key: HKLM\\Software\\Origin"), true);
        return false;
    }

    wchar_t clientPathKey[1024] = {0};
    DWORD cbClientPathKey = sizeof(clientPathKey)-1;
    if (RegQueryValueEx(key, L"ClientPath", NULL, NULL, (LPBYTE)clientPathKey, &cbClientPathKey) != ERROR_SUCCESS)
    {
        UTLogger::instance()->logEntry(std::wstring(L"Unable to read registry key: HKLM\\Software\\Origin\\ClientPath"), true);
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

    UTLogger::instance()->logEntry(std::wstring(L"Unable to find Origin.exe"));
    return false;
}

void handleBetaArg(wchar_t* strMatch)
{
    strMatch += wcslen(L"/Beta:");
	std::wstring isBeta(strMatch);
	int end = isBeta.find_first_of(L"\"");
	isBeta = isBeta.substr(0, end);
	HKEY key = NULL;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Origin", 0, KEY_SET_VALUE , &key) == ERROR_SUCCESS)
	{
		RegSetValueEx(key, L"IsBeta", 0, REG_SZ, (BYTE*)isBeta.c_str(), wcslen(isBeta.c_str()) * sizeof(wchar_t)); 
	}
}

void handleVersionArg(wchar_t* strMatch)
{
    strMatch += wcslen(L"/Version:");
	std::wstring version(strMatch);
	int end = version.find_first_of(L"\"");
	version = version.substr(0, end);
	HKEY key = NULL;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Origin", 0, KEY_SET_VALUE , &key) == ERROR_SUCCESS)
	{
		RegSetValueEx(key, L"ClientVersion", 0, REG_SZ, (BYTE*)version.c_str(), wcslen(version.c_str()) * sizeof(wchar_t)); 
	}
}

bool findUpdateFiles(const wchar_t* searchPath, bool root, std::wstring prefix, std::vector<std::wstring> &foundFolders, std::vector<std::wstring> &foundFiles)
{
    bool foundValidUpdate = false;

    wchar_t updateFiles[MAX_PATH] = {0};
    wsprintfW(updateFiles, L"%s*.*", searchPath);
    WIN32_FIND_DATAW findUpdateData;
    ZeroMemory(&findUpdateData, sizeof(findUpdateData));
    HANDLE handle = FindFirstFileW(updateFiles, &findUpdateData);
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            // Ignore the ./.. directories
            if (wcscmp(findUpdateData.cFileName, L".") == 0 || wcscmp(findUpdateData.cFileName, L"..") == 0)
                continue;

            if (root && wcscmp(findUpdateData.cFileName, L"OriginTMP.exe") == 0)
            {
                foundValidUpdate = true;
            }

            wchar_t childPath[MAX_PATH] = {0};
            wchar_t prefixPath[MAX_PATH] = {0};
            if (findUpdateData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                wsprintfW(childPath, L"%s%s\\", searchPath, findUpdateData.cFileName);
                wsprintfW(prefixPath, L"%s%s\\", prefix.c_str(), findUpdateData.cFileName);

#ifdef _DEBUG
                wprintf(L"Found update folder: %s\n", childPath);
#endif

                foundFolders.push_back(std::wstring(prefixPath));

                // Recurse
                foundValidUpdate |= findUpdateFiles(childPath, false, std::wstring(prefixPath), foundFolders, foundFiles);
            }
            else
            {
                // Use the format: Some\\Prefix\\Dirs\\Filename.dat
                wsprintfW(prefixPath, L"%s%s", prefix.c_str(), findUpdateData.cFileName);

#ifdef _DEBUG
                wprintf(L"Found update file: %s\n", childPath);
#endif

                foundFiles.push_back(std::wstring(prefixPath));
            }
        }
		while ( FindNextFileW(handle, &findUpdateData));

        FindClose(handle);
    }

    return foundValidUpdate;
}

bool findRunningOriginFolder(std::wstring &originLocation)
{
    bool originFound = false;
    wchar_t originFilename[MAX_PATH] = {0};
    DWORD cProcesses = 0;
    DWORD processIds[PROCESS_IDS_LEN] = {0};
	if (EnumProcesses(processIds, PROCESS_IDS_LEN*sizeof(DWORD), &cProcesses))
	{
		cProcesses /= sizeof(DWORD);
		for (DWORD i = 0; i < cProcesses; i++)
		{
			HANDLE  hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processIds[i]);
			if (hProcess != NULL)
			{
                wchar_t baseName[MAX_PATH] = {0};
				if (GetModuleBaseName(hProcess, NULL, baseName, MAX_PATH) > 0)
				{
					if (wcscmp(baseName, L"Origin.exe") == 0)
					{
                        if (GetModuleFileNameEx(hProcess, NULL, originFilename, sizeof(originFilename)))
                        {
                            originFound = true;

                            UTLogger::instance()->logEntry(std::wstring(L"Origin running at: ") + std::wstring(originFilename));
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
        UTLogger::instance()->logEntry(std::wstring(L"No running Origin process found, searching the registry instead..."));
        return findOriginInstallFromRegistry(originLocation);
    }

    originLocation = originFilename;
    
    // Remove executable
    int lastSlash = originLocation.find_last_of(L"\\");
    originLocation = originLocation.substr(0, lastSlash + 1);

    return true;
}

bool renameRunningOrigin(std::wstring originFolder)
{
    wchar_t originLocation[MAX_PATH] = {0};
    wchar_t originTmpLocation[MAX_PATH] = {0};
    wchar_t originOldLocation[MAX_PATH] = {0};
    wsprintfW(originLocation, L"%sOrigin.exe", originFolder.c_str());
    wsprintfW(originTmpLocation, L"%sOriginTMP.exe", originFolder.c_str());
    wsprintfW(originOldLocation, L"%sOriginOLD.exe", originFolder.c_str());

    if (fileExists(originLocation))
    {
        UTLogger::instance()->logEntry(wstring(originLocation) + L" exists.");
    }
    else
    {
        UTLogger::instance()->logEntry(wstring(originLocation) + L" does not exist.", true);
        return false;
    }

    if (fileExists(originTmpLocation))
    {
        UTLogger::instance()->logEntry(wstring(originTmpLocation) + L" exists.");
    }
    else
    {
        UTLogger::instance()->logEntry(wstring(originTmpLocation) + L" does not exist.", true);
        return false;
    }

    UTLogger::instance()->logEntry(wstring(L"Processing ") + originLocation + wstring(L" and ") + originTmpLocation);
    UTLogger::instance()->logEntry("Setting up new Origin");

    if (fileExists(originOldLocation) && !deleteFile(originOldLocation))
    {
        UTLogger::instance()->logEntry("Unable to remove OriginOLD.exe before swap");
        return false;
    }
    
    if (!moveFile(originLocation, originOldLocation))
    {
        UTLogger::instance()->logEntry("Unable to move Origin.exe to OriginOLD.exe");
        return false;
    }

    // Copy instead of move here so that OriginTMP.exe will always exist (the bootstrapper has a possible race condition otherwise)
    if (!copyFile(originTmpLocation, originLocation))
    {
        UTLogger::instance()->logEntry("Unable to move OriginTMP.exe to Origin.exe");
        return false;
    }

    if (!moveFile(originOldLocation, originTmpLocation))
    {
        UTLogger::instance()->logEntry("Unable to move OriginOLD.exe to OriginTMP.exe");
        return false;
    }

    if (fileExists(originOldLocation))
    {
        UTLogger::instance()->logEntry(wstring(L"[PROBLEM]:This file shouldn't be here anymore!] Deleting : ") + originOldLocation);
        deleteFile(originOldLocation);
    }

    if (!setFileFolderPermissions(originTmpLocation, false))
    {
        UTLogger::instance()->logEntry("Unable to relax permissions for OriginTMP.exe");
        return false;
    }

    return true;
}

bool renameRunningUpdateTool(std::wstring originFolder)
{
    wchar_t updateToolLocation[MAX_PATH] = {0};
    wchar_t updateToolOldLocation[MAX_PATH] = {0};
    wsprintfW(updateToolLocation, L"%sUpdateTool.exe", originFolder.c_str());
    wsprintfW(updateToolOldLocation, L"%sUpdateToolOLD.exe", originFolder.c_str());

    if (fileExists(updateToolLocation))
    {
        UTLogger::instance()->logEntry(wstring(updateToolLocation) + L" exists.");
    }
    else
    {
        UTLogger::instance()->logEntry(wstring(updateToolLocation) + L" does not exist.", true);
        return false;
    }

    if (fileExists(updateToolOldLocation))
    {
        UTLogger::instance()->logEntry(wstring(updateToolOldLocation) + L" exists.");
        if (!deleteFile(updateToolOldLocation))
        {
            UTLogger::instance()->logEntry("Unable to delete UpdateToolOLD.exe");
            return false;
        }
    }
    
    UTLogger::instance()->logEntry("Moving running UpdateTool...");
    
    if (!moveFile(updateToolLocation, updateToolOldLocation))
    {
        UTLogger::instance()->logEntry("Unable to move UpdateTool.exe to UpdateToolOLD.exe");
        return false;
    }

    return true;
}

bool terminateProcessList(std::vector<std::wstring> processPaths)
{
    UTLogger::instance()->logEntry(L"Attempting to terminate processes.");

    std::map<size_t, std::wstring> processPathsNativeFormat;
    std::map<size_t, std::wstring> processPathsShortFormat;

    for (size_t c = 0; c < processPaths.size(); ++c)
    {
        std::wstring processPath = processPaths[c];

        // Get the short format (8.3)
        wchar_t shortFullFilePath[MAX_PATH+1] = {0};
        if (GetShortPathName(processPath.c_str(), shortFullFilePath, MAX_PATH) == 0)
        {
            UTLogger::instance()->logEntry(processPath + std::wstring(L" could not be killed.  GetShortPathName failed."), true);
            continue;
        }

        // Save it to the map
        processPathsShortFormat[c] = shortFullFilePath;

        // Truncated path (for if we have to fall back to native NT paths, which are like \Device\HarddiskVolume3\Windows\System32\etc)
        size_t driveSep = processPath.find_first_of(L":");
        if (driveSep == std::wstring::npos)
        {
            UTLogger::instance()->logEntry(processPath + std::wstring(L" could not be killed.  Path format incorrect."));
            continue;
        }
        std::wstring truncatedPath = processPath.substr(driveSep+1, truncatedPath.length() - (driveSep+1));

        // DOS/Win32 Drive letter (e.g. 'C:')
        std::wstring drive = processPath.substr(0, driveSep+1);

        // Get native NT device name for the drive letter (e.g. '\Device\HarddiskVolume3')
        wchar_t nativeDeviceName[512] = {0};
        if (QueryDosDevice(drive.c_str(), nativeDeviceName, sizeof(nativeDeviceName)-1) == 0)
        {
            UTLogger::instance()->logEntry(processPath + std::wstring(L" could not be killed.  QueryDosDevice failed."), true);
            continue;
        }

        // Create the full native NT path
        wchar_t nativeFullFilePath[1024] = {0};
        wsprintfW(nativeFullFilePath, L"%s%s", nativeDeviceName, truncatedPath.c_str());

        // Save it to our map
        processPathsNativeFormat[c] = std::wstring(nativeFullFilePath);
    }

    bool killed = false;

    DWORD cProcesses = 0;
    DWORD processIds[PROCESS_IDS_LEN] = {0};
	if (EnumProcesses(processIds, PROCESS_IDS_LEN*sizeof(DWORD), &cProcesses))
	{
		cProcesses /= sizeof(DWORD);
		for (DWORD i = 0; i < cProcesses; i++)
		{
			Auto_HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processIds[i]);
			if (hProcess != NULL && hProcess != INVALID_HANDLE_VALUE)
			{
                bool nativeFormat = false;

                wchar_t processFullPath[512] = {0};
				if (GetModuleFileNameEx(hProcess, NULL, processFullPath, sizeof(processFullPath)) > 0)
				{
                    nativeFormat = false;
				}
                // If this error occurs, it is because the process is a 64-bit process and we are 32-bit
                else if (GetLastError() == ERROR_PARTIAL_COPY)
                {
                    // This API returns native NT paths (e.g. \Volume\HarddiskVolume3\windows\etc)
                    if (GetProcessImageFileName(hProcess, processFullPath, sizeof(processFullPath)) > 0)
                    {
                        nativeFormat = true;
                    }
                    else if (GetLastError() == ERROR_INVALID_HANDLE)
                    {
                        continue;
                    }
                    else
                    {
                        UTLogger::instance()->logEntry(L"Warning: GetProcessImageFileName failed.", true);
                        continue;
                    }
                }
                else if (GetLastError() == ERROR_INVALID_HANDLE)
                {
                    continue;
                }
                else
                {
                    UTLogger::instance()->logEntry(L"Warning: GetModuleFileNameEx failed.", true);
                    continue;
                }

                // Get the short format (8.3) for the process we're comparing
                // This seems redundant because we already pre-computed the short pathnames for the target files, however
                // the Windows API sometimes returns hybrid short/long path names, depending on the environment when a process
                // was started.  (e.g. The current directory was 'C:\PROGRA~2\ORIGIN' at the time)
                // Anyway, this will result in a fully short-ified path which we can reliably compare to our previous computed
                // fully short-ified path.
                wchar_t shortFullFilePath[MAX_PATH+1] = {0};
                if (!nativeFormat)
                {
                    if (GetShortPathName(processFullPath, shortFullFilePath, MAX_PATH) == 0)
                    {
                        UTLogger::instance()->logEntry(std::wstring(L"GetShortPathName failed for: ") + std::wstring(processFullPath), true);
                    }
                }

                // Loop through our list of target processes
                bool match = false;
                std::wstring targetProcessMatch;
                for (size_t c = 0; c < processPaths.size(); ++c)
                {
                    std::wstring targetProcessPath = processPaths[c];
                    std::wstring targetProcessNativePath = processPathsNativeFormat[c];
                    std::wstring targetProcessShortPath = processPathsShortFormat[c];

                    if (nativeFormat)
                    {
                        // If it matches the process we want to kill
					    if (_wcsicmp(processFullPath, targetProcessNativePath.c_str()) == 0)
					    {
                            targetProcessMatch = targetProcessPath;
                            match = true;
                            break;
                        }
                    }
                    else
                    {
                        // If it matches the process we want to kill (by long name or short name)
					    if (_wcsicmp(processFullPath, targetProcessPath.c_str()) == 0 || _wcsicmp(shortFullFilePath, targetProcessShortPath.c_str()) == 0)
					    {
                            targetProcessMatch = targetProcessPath;
                            match = true;
                            break;
					    }
                    }
                }
                
                if (match)
                {
                    UTLogger::instance()->logEntry(targetProcessMatch + std::wstring(L" is running. Attempting to kill process."));
					
                    if (TerminateProcess(hProcess,0))
				    {
					    // Process terminated
                        UTLogger::instance()->logEntry(targetProcessMatch + std::wstring(L" killed successfully."));

                        killed = true;
				    }
				    else
				    {
					    // Unable to terminate process
					    UTLogger::instance()->logEntry(targetProcessMatch + std::wstring(L" could not be killed."), true);
				    }
                }
			}
		}
	}

    if (!killed)
    {
        UTLogger::instance()->logEntry(L"No target processes running.");
    }
    return killed;
}

bool terminateOriginProcesses(std::wstring originLocation, std::vector<std::wstring> foundFiles)
{
    // Find all the .exe files in the Origin folder
    std::vector<std::wstring> targetProcesses;
    for (size_t c = 0; c < foundFiles.size(); ++c)
    {
        std::wstring targetFile = originLocation + foundFiles[c];

        // Don't kill Origin or UpdateTool itself, if they somehow made it into this list
        if ((_wcsicmp(foundFiles[c].c_str(), L"Origin.exe") == 0) || (_wcsicmp(foundFiles[c].c_str(), L"UpdateTool.exe") == 0))
            continue;

        // If it is not an executable file, skip it
        if (_wcsicmp(targetFile.substr(targetFile.length() - 4, 4).c_str(), L".exe") != 0)
            continue;
        
        // If it doesn't exist on disk, skip it
        if (!fileExists(targetFile.c_str()))
            continue;

        // Add it to the kill list
        targetProcesses.push_back(targetFile);
    }

    if (targetProcesses.size() == 0)
        return true;
    else
        return terminateProcessList(targetProcesses);
}

bool copyUpdateFiles(std::wstring originLocation, std::wstring updateLocation, std::vector<std::wstring> foundFolders, std::vector<std::wstring> foundFiles)
{
    bool isXP = isPlatformXP();

    // Lock all the files for deletion
    std::map<std::wstring, HANDLE> fileHandleCache;
    bool lockSuccess = true;
    for (size_t c = 0; c < foundFiles.size(); ++c)
    {
        std::wstring sourceFile = updateLocation + foundFiles[c];
        std::wstring targetFile = originLocation + foundFiles[c];
        
        // On Windows XP, WinHTTP.dll will be in use by the bootstrapper, so we don't want to try and delete it, but we should try to delete the temp file location it will go to
        if (isXP && _wcsicmp(foundFiles[c].c_str(), L"winhttp.dll") == 0)
        {
            targetFile = originLocation + std::wstring(L"winhttpTMP.dll");
        }

        // Only interested in already-existing files
        if (!fileExists(targetFile.c_str()))
            continue;

        ::SetFileAttributes(targetFile.c_str(), FILE_ATTRIBUTE_NORMAL);       // ensure file is not read-only just in case.

        const int kMaxRetriesForDelete = 40;
        const int kRetrieSleepTimeForLockForDelete = 250;
        int nRetriesLeft = kMaxRetriesForDelete;         // maximum times to retry locking a file for delete before erroring out
        bool bLocked = FALSE;
        bool killAttempted = false;
        while (bLocked == FALSE && nRetriesLeft-- > 0)
        {
            HANDLE hFile = ::CreateFile(targetFile.c_str(), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                DWORD nLastError = ::GetLastError();

                if (nLastError != ERROR_FILE_NOT_FOUND)
                {
                    UTLogger::instance()->logEntry(std::wstring(L"File couldn't be locked for deletion: ") + targetFile, true);
                }

                switch (nLastError)
                {
                case ERROR_FILE_NOT_FOUND:	// file isn't there so it doesn't need to be deleted.
                    bLocked = true;
                    nRetriesLeft = 0;
                    break;
                case ERROR_SHARING_VIOLATION:
                    {
                        std::wstring fileType = targetFile.substr(targetFile.length() - 4, 4);
                        // If it is an executable file or DLL
                        if (!killAttempted && (_wcsicmp(fileType.c_str(), L".exe") == 0 || _wcsicmp(fileType.c_str(), L".dll") == 0))
                        {
                            // Try to terminate all the Origin processes
                            terminateOriginProcesses(originLocation, foundFiles);

                            killAttempted = true;
                        }
                    }
                case ERROR_ACCESS_DENIED:
                case ERROR_BUSY:
                case ERROR_LOCK_VIOLATION:
                case ERROR_NETWORK_BUSY:
                    // wait and try again
#ifdef _DEBUG
                    wprintf(L"Waiting and trying again. Attempt #%d\n", (kMaxRetriesForDelete - nRetriesLeft));
#endif
                    Sleep(kRetrieSleepTimeForLockForDelete); // delay before trying again
                    break;

                default:
                    nRetriesLeft = 0;	// auto-retries are not going to work for these cases so just fail out and toss up the update error message
                    break;
                }
            }
            else
            {
                fileHandleCache[targetFile] = hFile;
                bLocked = true;
            }
        }

        if (!bLocked)
        {
            // We can't lock all files, so bail and don't waste time on the rest
            lockSuccess = false;
            break;
        }
    }

    if (!lockSuccess)
    {
        UTLogger::instance()->logEntry(std::wstring(L"Unable to lock all updated files for deletion."));
    }

    // Regardless of success or failure, we have to release the handles
    bool deleteSuccess = true;
    for (std::map<std::wstring, HANDLE>::iterator iter = fileHandleCache.begin(); iter != fileHandleCache.end(); ++iter)
    {
        std::wstring targetFile = (*iter).first;
        HANDLE hFile = (*iter).second;

        // If we did get all the locks, delete the corresponding files
        if (lockSuccess && deleteSuccess)
        {
            if (!deleteFile(targetFile.c_str()))
            {
                deleteSuccess = false;
            }
        }

        CloseHandle(hFile);
    }

    if (!lockSuccess || !deleteSuccess)
        return false;

    // Clear the cache
    fileHandleCache.clear();

    // Create the folders
    for (size_t c = 0; c < foundFolders.size(); ++c)
    {
        std::wstring targetFolder = originLocation + foundFolders[c];
        if (!createFolderIfNotExist(targetFolder.c_str()))
        {
            return false;
        }
    }

    for (size_t c = 0; c < foundFiles.size(); ++c)
    {
        std::wstring sourceFile = updateLocation + foundFiles[c];
        std::wstring targetFile = originLocation + foundFiles[c];

        // We don't want to copy WinHttp.dll
        if (!isXP && _wcsicmp(foundFiles[c].c_str(), L"winhttp.dll") == 0)
        {
            deleteFile(sourceFile.c_str());
            continue;
        }

        // On Windows XP, WinHTTP.dll will be in use by the bootstrapper, so we need to unpack to a temporary filename
        if (isXP && _wcsicmp(foundFiles[c].c_str(), L"winhttp.dll") == 0)
        {
            targetFile = originLocation + std::wstring(L"winhttpTMP.dll");
        }

        // Lock the file while we copy it, so we can verify the signatures
        Auto_FileLock fileLock(sourceFile.c_str());

#if defined(CHECK_SIGNATURES)
        if (!fileLock.acquired())
        {
            UTLogger::instance()->logEntry(std::wstring(L"Unable to lock source file for signature checking: ") + sourceFile, true);
            return false;
        }

        // If it is one of our signed files, check it before we copy
        if (_wcsicmp(foundFiles[c].c_str(), L"OriginClientService.exe") == 0 || _wcsicmp(foundFiles[c].c_str(), L"OriginTMP.exe") == 0)
        {
            if (!isValidEACertificate(sourceFile.c_str()))
                return false;
        }
#endif

        if (!copyFile(sourceFile.c_str(), targetFile.c_str()))
        {
            return false;
        }

        // Release the lock
        fileLock.release();

        // Delete the update files as we copy them, except ourselves
        if (_wcsicmp(foundFiles[c].c_str(), L"UpdateTool.exe") != 0)
        {
            deleteFile(sourceFile.c_str());
        }
    }

    // Delete the empty selfupdate folders (go in reverse to get subfolders first)
    for (int c = foundFolders.size()-1; c >= 0; --c)
    {
        std::wstring sourceFolder = updateLocation + foundFolders[(size_t)c];
        
        RemoveDirectory(sourceFolder.c_str());
    }

    return true;
}

bool removeOldStagedUpdates(std::wstring originLocation)
{
    bool success = true;

    // Look for staged update zip files in the ProgramFiles (Origin) location and move them to ProgramData
    wchar_t oldStagedFileLocation[MAX_PATH] = {0};
    wsprintfW(oldStagedFileLocation, L"%sOriginUpdate*.zip", originLocation.c_str());
    WIN32_FIND_DATAW oldStagedDownloadFileData;
    ZeroMemory(&oldStagedDownloadFileData, sizeof(oldStagedDownloadFileData));
    HANDLE handle = FindFirstFileW(oldStagedFileLocation, &oldStagedDownloadFileData);

    if (handle != INVALID_HANDLE_VALUE)
    {
		// loop and find all update files present
        do
        {
            std::wstring oldStagedDataFullPath = originLocation + oldStagedDownloadFileData.cFileName;
            
            if (!deleteFile(oldStagedDataFullPath.c_str()))
            {
                success = false;
            }
        }
		while ( FindNextFileW(handle, &oldStagedDownloadFileData));
    }
    FindClose(handle);

    return success;
}

void uninstallResponderServiceIfInstalled(std::wstring originLocation)
{
    std::wstring responderServiceLocation = originLocation + std::wstring(L"OriginWebHelperService.exe");
    if (fileExists(responderServiceLocation.c_str()))
    {
        UTLogger::instance()->logEntry("Uninstalling Web Helper Service.");
        execProcess(responderServiceLocation, L"/nsisuninstall", false, true);
    }
}

bool doSelfUpdate(int argc, wchar_t** argv)
{
    for (int i = 0; i < argc; i++)
	{
		wchar_t* strMatch = NULL;

		strMatch = wcsstr(argv[i], L"/Beta:");
		if (strMatch != 0)
		{
			handleBetaArg(strMatch);
		}
		strMatch = wcsstr(argv[i], L"/Version:");
		if (strMatch != 0)
		{
			handleVersionArg(strMatch);
		}
	}

    if (setProgramDataPermissions())
        UTLogger::instance()->logEntry("Setting ProgramData permissions succeeded");
    else
        UTLogger::instance()->logEntry("Setting ProgramData permissions failed");

    std::wstring originLocation;

    if (!findRunningOriginFolder(originLocation))
        return false;

    UTLogger::instance()->logEntry(std::wstring(L"Found Origin folder: ") + originLocation);

    wchar_t szProgramDataPath[MAX_PATH] = {0};
    if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szProgramDataPath))) 
    {
        UTLogger::instance()->logEntry(std::wstring(L"Unable to get ProgramData path."), true);
        return false;
    }

    std::wstring strProgramDataOrigin (szProgramDataPath);
    strProgramDataOrigin.append (L"\\Origin\\SelfUpdate\\StagedUpdate\\");
    wstring updateLocation = strProgramDataOrigin;

    UTLogger::instance()->logEntry(std::wstring(L"SelfUpdate data location: ") + updateLocation);

    // Copy the update files over the existing files
    std::vector<std::wstring> foundFolders;
    std::vector<std::wstring> foundFiles;
    bool foundValidUpdate = findUpdateFiles(updateLocation.c_str(), true, std::wstring(L""), foundFolders, foundFiles);

    if (foundValidUpdate)
    {
        UTLogger::instance()->logEntry(std::wstring(L"Found valid update data."));
    }
    else
    {
        UTLogger::instance()->logEntry(std::wstring(L"No valid update data found."));
        return false;
    }

    // Uninstall the Web Helper Service
    uninstallResponderServiceIfInstalled(originLocation);

    // Refresh the Origin folder permissions (unrestrict)  (if this fails, just continue anyway)
    setOriginFolderPermissions(originLocation, false);

    // If this fails, it's not critical
    removeOldStagedUpdates(originLocation.c_str());

    if (!copyUpdateFiles(originLocation, updateLocation, foundFolders, foundFiles))
        return false;

    if (!renameRunningOrigin(originLocation))
        return false;
         
    // Update our working DIR in case we were called from RTP
    _wchdir(originLocation.c_str());

    // Install dependencies (e.g. Origin Client Service)
    installDependencies(false);

    updateShortcuts();  // needs to be done here because we need to be elevated

    // Refresh the Origin folder permissions (if this fails, just continue anyway)
    setOriginFolderPermissions(originLocation, true);

    // If no EACore.ini file exists, create an empty one (client will ignore)
    std::wstring eaCoreIniLocation = originLocation + std::wstring(L"EACore.ini");
    if (!fileExists(eaCoreIniLocation.c_str()))
    {
        // Create an empty file
        Auto_HANDLE eaCoreIniFile = CreateFile(eaCoreIniLocation.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    // Unrestrict it
    setFileFolderPermissions(eaCoreIniLocation.c_str(), false);

    // Unrestrict the OriginTMP.exe file so it can be deleted
    std::wstring originTmpExeLocation = originLocation + std::wstring(L"OriginTMP.exe");
    setFileFolderPermissions(originTmpExeLocation.c_str(), false);

    UTLogger::instance()->logEntry(std::wstring(L"Self-update successful."));
    return true;
}

bool doDowngrade()
{
    UTLogger::instance()->logEntry(std::wstring(L"Performing downgrade operation."));

    std::wstring originLocation;

    if (!findRunningOriginFolder(originLocation))
        return false;

    UTLogger::instance()->logEntry(std::wstring(L"Found Origin folder: ") + originLocation);

    wchar_t szProgramDataPath[MAX_PATH] = {0};
    if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szProgramDataPath))) 
    {
        UTLogger::instance()->logEntry(std::wstring(L"Unable to get ProgramData path."), true);
        return false;
    }

    std::wstring strProgramDataOrigin (szProgramDataPath);
    strProgramDataOrigin.append (L"\\Origin\\SelfUpdate\\StagedUpdate\\");
    wstring updateLocation = strProgramDataOrigin;

    UTLogger::instance()->logEntry(std::wstring(L"SelfUpdate data location: ") + updateLocation);

    // Copy the update files over the existing files
    std::vector<std::wstring> foundFolders;
    std::vector<std::wstring> foundFiles;
    bool foundValidUpdate = findUpdateFiles(updateLocation.c_str(), true, std::wstring(L""), foundFolders, foundFiles);

    if (foundValidUpdate)
    {
        UTLogger::instance()->logEntry(std::wstring(L"Found valid update data."));
    }
    else
    {
        UTLogger::instance()->logEntry(std::wstring(L"No valid update data found."));
        return false;
    }

    // Refresh the Origin folder permissions (unrestrict)  (if this fails, just continue anyway)
    setOriginFolderPermissions(originLocation, false);

    // If an EACore.ini file exists, see if it is empty.  If so, we need to remove it, because previous clients don't ignore empty EACore.ini files
    std::wstring eaCoreIniLocation = originLocation + std::wstring(L"EACore.ini");
    if (fileExists(eaCoreIniLocation.c_str()))
    {
        // Open the file
        Auto_HANDLE eaCoreIniFile = CreateFile(eaCoreIniLocation.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if ((HANDLE)eaCoreIniFile != INVALID_HANDLE_VALUE)
        {
            // We only care about the low-order DWORD
            DWORD fileSizeLow = GetFileSize(eaCoreIniFile, NULL);
            if (fileSizeLow == 0)
            {
                // The file is empty
                deleteFile(eaCoreIniLocation.c_str());
            }
        }
    }

    // Move ourselves out of the way first
    if (!renameRunningUpdateTool(originLocation))
        return false;

    // Copy the files back to the Origin folder
    if (!copyUpdateFiles(originLocation, updateLocation, foundFolders, foundFiles))
        return false;

    UTLogger::instance()->logEntry(std::wstring(L"Downgrade successful."));
    return true;
}

bool doPrepareDowngrade(int argc, wchar_t** argv)
{
    int updateSystemVersion = -1;

    for (int i = 0; i < argc; i++)
    {
        wchar_t* strMatch;
        strMatch = wcsstr(argv[i], L"UpdateSystem");
        if (strMatch != 0)
        {
            strMatch = wcsstr(strMatch, L"=");

            // If there is no = sign, nothing to do
            if (strMatch == 0)
            {
                UTLogger::instance()->logEntry(std::wstring(L"Malformed UpdateSystem parameter: ") + std::wstring(argv[i]));
                return false;
            }

            // Make sure there is more than one character
            if (wcslen(strMatch) <= 1)
            {
                UTLogger::instance()->logEntry(std::wstring(L"Malformed UpdateSystem parameter: ") + std::wstring(argv[i]));
                return false;
            }

            // Move one character over because we don't care about the = symbol
            strMatch++;

            // Found the update system arg, convert it to an integer
            wchar_t* endPtr = NULL;
            updateSystemVersion = (int)wcstol(strMatch, &endPtr, 10);

            // If no digit could be retrieved, nothing to do
            if (strMatch == endPtr)
            {
                UTLogger::instance()->logEntry(std::wstring(L"Malformed UpdateSystem parameter.  Couldn't parse integer: ") + std::wstring(argv[i]));
                return false;
            }
        }
    }

    if (updateSystemVersion < 0)
    {
        UTLogger::instance()->logEntry(std::wstring(L"Missing UpdateSystem parameter."));
        return false;
    }

    UTLogger::instance()->logEntry(std::wstring(L"Preparing for downgrade."));

    std::wstring originLocation;

    if (!findRunningOriginFolder(originLocation))
        return false;

    // If we are downgrading to Update System Version 2
    if (updateSystemVersion == 2)
    {
        UTLogger::instance()->logEntry(std::wstring(L"Target Update System version: 2"));

        // We need to uninstall the Web Helper Service
        uninstallResponderServiceIfInstalled(originLocation);
    }

    return true;
}

bool isLegacySecondUpdateStep(int argc, wchar_t** argv)
{
    // We want to determine here if this is the second (non-elevated) invocation of the UpdateTool.exe
	for (int i = 0; i < argc; i++)
	{
		wchar_t* strMatch;
        strMatch = wcsstr(argv[i],L"/SelfUpdate");
        if (strMatch != 0)
        {
            // This is the new 1st mode that the bootstrapper calls if it is 'read-only Origin folder-aware'
            return false;
        }
        strMatch = wcsstr(argv[i],L"/FinalizeSelfUpdate");
        if (strMatch != 0)
        {
            // This is the new 2nd mode that the bootstrapper calls if it is 'read-only Origin folder-aware'
            return false;
        }
        strMatch = wcsstr(argv[i],L"/Downgrade");
        if (strMatch != 0)
        {
            return false;
        }
        strMatch = wcsstr(argv[i], L"/PrepareDowngrade");
        if (strMatch != 0)
        {
            return false;
        }
		strMatch = wcsstr(argv[i], L"/Beta:");
		if (strMatch != 0)
		{
			return false;
		}
		strMatch = wcsstr(argv[i], L"/Version:");
		if (strMatch != 0)
		{
			return false;
		}
    }

    return true;
}

bool isSameVersion(const wchar_t* fileName)
{
    // Convert our version to a wide char string
    wchar_t curVersion[128] = {0};
    wsprintfW(curVersion, L"%S", EALS_VERSION_P_DELIMITED);

    DWORD unused = 0;
    DWORD size = GetFileVersionInfoSize(fileName, &unused);

    // If we can't get version info then assume we didn't find OriginClient.dll
    if (size == 0)
    {
        UTLogger::instance()->logEntry(std::wstring(L"Unable to get file version info for: ") + std::wstring(fileName));
        return false;
    }

    AutoBuffer<BYTE> versionInfo(size);
    if (GetFileVersionInfo(fileName, unused, size, versionInfo))
    {
        VS_FIXEDFILEINFO* vsfi = NULL;
        UINT len = 0;
        VerQueryValue(versionInfo, L"\\", (void**)&vsfi, &len);
        WORD first = HIWORD(vsfi->dwFileVersionMS);
        WORD second = LOWORD(vsfi->dwFileVersionMS);
        WORD third = HIWORD(vsfi->dwFileVersionLS);
        WORD fourth = LOWORD(vsfi->dwFileVersionLS);

        wchar_t wVersionInfo[32] = {0};
        wsprintfW(wVersionInfo, L"%hu.%hu.%hu.%hu", first, second, third, fourth);
        
        UTLogger::instance()->logEntry(std::wstring(L"File: ") + std::wstring(fileName) + std::wstring(L" version: ") + std::wstring(wVersionInfo));
        if (wcscmp(wVersionInfo, curVersion) == 0)
        {
            UTLogger::instance()->logEntry(std::wstring(L"Version matches current version!"));
            return true;
        }
    }
    return false;
}

bool ensureProperSerialization(Auto_MUTEX& mutex, int argc, wchar_t** argv)
{
    if (!mutex.valid())
    {
        return false;
    }

    // If we're on the second update step, we need to ensure the first has already completed (don't log here since we use the same log file)
    if (isLegacySecondUpdateStep(argc, argv))
    {
        // Sleep two seconds to avoid the race condition where the second step acquires the mutex first
        Sleep(2000);
    }

    // Acquire the synchronization mutex
    mutex.acquire(INFINITE);

    UTLogger::instance()->logEntry(std::wstring(L"Mutex acquired."));

    return true;
}

int wmain(int argc, wchar_t** argv)
{
#ifndef _DEBUG
	ShowWindow( GetConsoleWindow(), SW_HIDE );
#endif

    Auto_MUTEX mutex(ORIGIN_UPDATETOOL_MUTEX);
    if (!ensureProperSerialization(mutex, argc, argv))
    {
        // We should never see this!
        ::MessageBox( NULL, L"Something went wrong during the installation. Please reinstall Origin", L"Error installing Origin", MB_OK | MB_ICONEXCLAMATION);

        exit(0);
    }    

    UTLogger::instance()->logEntry("Registry operations");
    bool updateFromOldBootstrap = true;
	bool registered = false;
	for (int i = 0; i < argc; i++)
	{
		wchar_t* strMatch;
        strMatch = wcsstr(argv[i],L"/SelfUpdate");
        if (strMatch != 0)
        {
            // This is the new mode that the bootstrapper calls if it is 'read-only Origin folder-aware'
            // In this mode, we assume that you need elevation in order to rename Origin's executable, etc.
            // Therefore, the UpdateTool process needs to work a bit differently.
            if (!doSelfUpdate(argc, argv))
            {
                // We should never see this!
                ::MessageBox( NULL, L"Something went wrong during the installation. Please reinstall Origin", L"Error installing Origin", MB_OK | MB_ICONEXCLAMATION);
            }

            exit(0);
        }
        strMatch = wcsstr(argv[i],L"/Downgrade");
        if (strMatch != 0)
        {
            // This is a new mode that the bootstrapper calls if it is 'read-only Origin folder-aware' and we are downgrading to a version that is NOT.
            // We need to open the folder permissions and then copy all the files over.
            if (!doDowngrade())
            {
                // We should never see this!
                ::MessageBox( NULL, L"Something went wrong during the installation. Please reinstall Origin", L"Error installing Origin", MB_OK | MB_ICONEXCLAMATION);
            }

            exit(0);
        }
        strMatch = wcsstr(argv[i], L"/PrepareDowngrade");
        if (strMatch != 0)
        {
            // This is a new mode that the bootstrapper calls if it is downgrading from a newer UpdateSystem version to an older one.
            if (!doPrepareDowngrade(argc, argv))
            {
                // We should never see this!
                ::MessageBox(NULL, L"Something went wrong during the installation. Please reinstall Origin", L"Error installing Origin", MB_OK | MB_ICONEXCLAMATION);
            }

            exit(0);
        }
		strMatch = wcsstr(argv[i], L"/Beta:");
		if (strMatch != 0)
		{
			handleBetaArg(strMatch);
			registered = true;
		}
		strMatch = wcsstr(argv[i], L"/Version:");
		if (strMatch != 0)
		{
			handleVersionArg(strMatch);
			registered = true;
		}
        strMatch = wcsstr(argv[i],L"/FinalizeSelfUpdate");
        if (strMatch != 0)
        {
            // This is the new 2nd mode that the bootstrapper calls if it is 'read-only Origin folder-aware'
            UTLogger::instance()->logEntry("Finalizing self update.");

            updateFromOldBootstrap = false;
        }
        strMatch = wcsstr(argv[i],L"/Restrict");
        if (strMatch != 0)
        {
            // Refresh the Origin folder permissions
            setOriginFolderPermissions(std::wstring(), true);

            // If no EACore.ini file exists, create an empty one (client will ignore)
            std::wstring eaCoreIniLocation = std::wstring(L"EACore.ini");
            if (!fileExists(eaCoreIniLocation.c_str()))
            {
                // Create an empty file
                Auto_HANDLE eaCoreIniFile = CreateFile(eaCoreIniLocation.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            }

            // Unrestrict it
            setFileFolderPermissions(eaCoreIniLocation.c_str(), false);

            exit(0);
        }
        strMatch = wcsstr(argv[i],L"/Open");
        if (strMatch != 0)
        {
            // Refresh the Origin folder permissions
            setOriginFolderPermissions(std::wstring(), false);

            exit(0);
        }
	}

    if (setProgramDataPermissions())
        UTLogger::instance()->logEntry("Setting ProgramData permissions succeeded");
    else
        UTLogger::instance()->logEntry("Setting ProgramData permissions failed");

	if (registered)
    {
        //////////////////////////////////////
        // We are running with elevation here
        //////////////////////////////////////

        UTLogger::instance()->logEntry(std::wstring(L"Performing upgrade operation."));

        std::wstring originFolder;
        if (!findRunningOriginFolder(originFolder))
        {
            UTLogger::instance()->logEntry("Unable to find Origin folder.");

            // We should never see this!
            ::MessageBox( NULL, L"Something went wrong during the installation. Please reinstall Origin", L"Error installing Origin", MB_OK | MB_ICONEXCLAMATION);

            exit(1);
        }

        std::wstring originDllLocation = originFolder + std::wstring(L"originClient.dll");
        if (!fileExists(originDllLocation.c_str()))
        {
            UTLogger::instance()->logEntry("Unable to find originClient.dll.  Aborting upgrade.");

            // Exit quietly
            exit(0);
        }

        // Need to remove WinHTTP.dll. It should only be here it we are running XP
	    if (!isPlatformXP() && fileExists(L"WinHttp.dll"))
	    {
            UTLogger::instance()->logEntry("We are *NOT* in XP.");
            UTLogger::instance()->logEntry("Deleting WinHttp.dll.");
            deleteFile(L"WinHttp.dll");
	    }
             
        // Install dependencies (e.g. Origin Client Service)
        installDependencies(false);

        updateShortcuts();  // needs to be done here because we need to be elevated

        // Make sure the OriginTMP.exe we are about to swap in is the same version as us!
        std::wstring originTmpExeLocation = originFolder + std::wstring(L"OriginTMP.exe");
        bool sameVersion = isSameVersion(originTmpExeLocation.c_str());
        if (!sameVersion)
        {
            UTLogger::instance()->logEntry(L"OriginTMP was NOT same version, a previous update step may have failed.");
        }

        // Handle the old-bootstrapper mode use-case
        // Since we need elevation to move files around, we need to do it now while we're elevated
        if (!sameVersion || !renameRunningOrigin(originFolder))
        {
            UTLogger::instance()->logEntry("Unable to rename Origin executables.");

            // We should never see this!
            ::MessageBox( NULL, L"Something went wrong during the installation. Please reinstall Origin", L"Error installing Origin", MB_OK | MB_ICONEXCLAMATION);

            exit(1);
        }

        // Refresh the Origin folder permissions
        setOriginFolderPermissions(originFolder, true);

        // If no EACore.ini file exists, create an empty one (client will ignore)
        std::wstring eaCoreIniLocation = originFolder + std::wstring(L"EACore.ini");
        if (!fileExists(eaCoreIniLocation.c_str()))
        {
            // Create an empty file
            Auto_HANDLE eaCoreIniFile = CreateFile(eaCoreIniLocation.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        }

        // Unrestrict it
        setFileFolderPermissions(eaCoreIniLocation.c_str(), false);

        // Unrestrict the OriginTMP.exe file so it can be deleted
        setFileFolderPermissions(originTmpExeLocation.c_str(), false);

		exit(0);
    }

	// This tool is called by the boot when we know that both of these files exist
	// Make sure Origin isn't still in use though in case it was slow to close
	
	DWORD cProcesses = 0;

    DWORD processIds[PROCESS_IDS_LEN] = {0};
	
	if (EnumProcesses(processIds, PROCESS_IDS_LEN*sizeof(DWORD), &cProcesses))
	{

		cProcesses /= sizeof(DWORD);
		for (DWORD i = 0; i < cProcesses; i++)
		{
			HANDLE  hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processIds[i]);
			if (hProcess != NULL)
			{
				wchar_t baseName[MAX_PATH] = L"";
				if (GetModuleBaseName(hProcess, NULL, baseName, MAX_PATH) > 0)
				{
					if (wcscmp(baseName, L"Origin.exe") == 0)
					{
                        UTLogger::instance()->logEntry("Origin.exe is running. Waiting for it to close.");
						WaitForSingleObject(hProcess, INFINITE);
						CloseHandle(hProcess);
						break;
					}
				}
				CloseHandle(hProcess);
			}
		}
	}

    wchar_t path[MAX_PATH] = L"";
    GetModuleFileName(NULL, path, MAX_PATH);
    std::wstring location(path);
    // Remove executable
    int lastSlash = location.find_last_of(L"\\");
    location = location.substr(0, lastSlash + 1);
    wchar_t originTmpLocation[MAX_PATH];
    wsprintfW(originTmpLocation, L"%sOriginTMP.exe", location.c_str());

//#define DEBUG_THIS // uncomment this line if you want to debug the UpdateTool.
#ifdef DEBUG_THIS
    __asm { int 3 }
#endif

    // Delete the OriginTMP.exe file if it exists
    BOOL success = TRUE;
    if (fileExists(originTmpLocation))
    {
        UTLogger::instance()->logEntry(wstring(originTmpLocation) + L" exists.");

        wchar_t originExeLocation[MAX_PATH + 1] = { 0 };
        wsprintfW(originExeLocation, L"%sOrigin.exe", location.c_str());

        wchar_t originDllLocation[MAX_PATH + 1] = { 0 };
        wsprintfW(originDllLocation, L"%soriginClient.dll", location.c_str());

        // If OriginTMP.exe IS our current version, Origin.exe IS NOT our current version, and originClient.dll IS our current version
        // This happens in a pre-9.5.6 bootstrapper where the user has canceled UAC, so we have a partially applied update
        // The only way to FORCE the bootstrap to re-download the full update is to delete originClient.dll
        if (updateFromOldBootstrap && isSameVersion(originTmpLocation) && !isSameVersion(originExeLocation) && isSameVersion(originDllLocation))
        {
            UTLogger::instance()->logEntry("Detected an incomplete update.  Removing originClient.dll to force bootstrapper to restart the update process.");
            success = deleteFile(originDllLocation);
        }
        else
        {
            success = deleteFile(originTmpLocation);
        }
    }

    // On Windows XP, we need to perform a special winhttp.dll renaming operation
    if (isPlatformXP() && success)
    {
        wchar_t winhttpLocation[MAX_PATH] = {0};
        wchar_t winhttpTMPLocation[MAX_PATH] = {0};
        
        wsprintfW(winhttpLocation, L"%swinhttp.dll", location.c_str());
        wsprintfW(winhttpTMPLocation, L"%swinhttpTMP.dll", location.c_str());

        if (fileExists(winhttpTMPLocation))
        {
            UTLogger::instance()->logEntry(wstring(winhttpTMPLocation) + L" exists.");

            // Delete the old one
            if (fileExists(winhttpLocation))
            {
                success = deleteFile(winhttpLocation);
            }

            if (success)
            {
                // Move the new one into the place of the old one
                success = moveFile(winhttpTMPLocation, winhttpLocation);
            }
            
            if (success)
            {
                UTLogger::instance()->logEntry(L"Replaced winhttp.dll at " + wstring(winhttpLocation));
            }
        }
    }

    if (!success)
    {
        ::MessageBox( NULL, L"Something went wrong during the installation. Please reinstall Origin"
            , L"Error installing Origin", MB_OK | MB_ICONEXCLAMATION);
    }

	// Start Origin back up
	// argv[1] contains the full commandline of the original launch of Origin.
	STARTUPINFO startupInfo;
	memset(&startupInfo,0,sizeof(startupInfo));
	PROCESS_INFORMATION procInfo;
	memset(&procInfo,0,sizeof(procInfo));
    UTLogger::instance()->logEntry("Creating backup process.");

    CreateProcess(NULL, argv[1], NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &procInfo);

}

/*
 * Returns true if this platform is XP
 **/
bool isPlatformXP()
{
    bool isXP = false;
    OSVERSIONINFO	osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osvi))
    {
        if (osvi.dwMajorVersion == 5 && (osvi.dwMinorVersion == 1 || osvi.dwMinorVersion == 2))
        {
            isXP = true;
        }
    }
    return isXP;
}

/*
 * executes a file via the shell to allow UAC prompts!
 **/
void execProcess(std::wstring file, std::wstring args, bool runAs, bool blocking)
{
    if (!fileExists(file.c_str()))
    {
        UTLogger::instance()->logEntry(wstring(L"execProcess failed - file not found ") + file + L" " + args);
        return;
    }

    // Need to check OS version. If Vista or higher we want to use runas to invoke UAC.
    bool isVistaOrHigher = false;
    OSVERSIONINFO	osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osvi))
    {
        if (osvi.dwMajorVersion >= 6)
        {
            isVistaOrHigher = true;
        }
    }

    if (runAs)
    {
        SHELLEXECUTEINFO  shExInfo = { 0 };
        shExInfo.cbSize = sizeof(shExInfo);
        shExInfo.lpVerb = isVistaOrHigher ? L"runas" : L"";
        shExInfo.lpFile = file.c_str();
        shExInfo.lpParameters = args.c_str();
        shExInfo.nShow = SW_SHOWNORMAL;
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;

        UTLogger::instance()->logEntry(wstring(L"execShell: ") + file + L" " + args);

        if (ShellExecuteEx(&shExInfo))
        {
            if (blocking)
            {
                WaitForSingleObject(shExInfo.hProcess, INFINITE);
                CloseHandle(shExInfo.hProcess);
            }
            UTLogger::instance()->logEntry(wstring(L"execShell: ") + file + L" " + args);

        }
        else
        {
            int err = GetLastError();
            if (err == ERROR_CANCELLED)
            {
                UTLogger::instance()->logEntry(wstring(L"execShell failed - user aborted.") + file + L" " + args);
            }
            else
            {
                UTLogger::instance()->logEntry(wstring(L"execShell failed"), true);
            }
        }
    }
    else
    {
        UTLogger::instance()->logEntry(wstring(L"createProcess: ") + file + L" " + args);

        // Pack the arguments into command line form (e.g. ""Executable.exe" /arg1 /arg2")
        wchar_t argsBuf[ARGS_BUFFER_SIZE];
        ZeroMemory(argsBuf, sizeof(argsBuf));
        std::wstring fullArgs = std::wstring(L"\"") + file + std::wstring(L"\" ") + args;
        wcsncpy_s(argsBuf, ARGS_BUFFER_SIZE-1, fullArgs.c_str(), fullArgs.length());

        PROCESS_INFORMATION processInformation;
        STARTUPINFO startupInfo;
        memset(&processInformation, 0, sizeof(PROCESS_INFORMATION));
        memset(&startupInfo, 0, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);
        
        if (!::CreateProcessW(NULL, argsBuf, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInformation))
        {
            UTLogger::instance()->logEntry("CreateProcess Failed.", true);
        }
        else if (blocking)
        {
            WaitForSingleObject(processInformation.hProcess,INFINITE);
        }
    }
}

//Example from https://msdn.microsoft.com/en-us/library/ms684139(v=vs.85).aspx 
//Not sure if there's a cleaner way to do this.
typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;
BOOL IsWow64()
{
    BOOL bIsWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if (NULL != fnIsWow64Process)
    {
        fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
    }
    return bIsWow64;
}


bool VerifyRegistryValue(std::wstring location, std::wstring keyToCheck, std::wstring expectedValue)
{
    HKEY key = NULL;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, location.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
    {
        UTLogger::instance()->logEntry(std::wstring(L"Unable to open registry key: "+location), true);
        return false;
    }

    wchar_t actualValue[1024] = { 0 };
    DWORD actualValueLength = sizeof(actualValue) - 1;
    if (RegQueryValueEx(key, keyToCheck.c_str(), NULL, NULL, (LPBYTE)actualValue, &actualValueLength) != ERROR_SUCCESS)
    {
        UTLogger::instance()->logEntry(std::wstring(L"Unable to read registry key: "+location+L" "+keyToCheck), true);
        RegCloseKey(key);
        return false;
    }

    // Clean up
    RegCloseKey(key);

    return expectedValue.compare(actualValue) == 0;
}


/*
 * Install dependencies - patches, KB's, Visual Studio runtime
 * IMPORTANT: match them with \originClient\dev\installer\EbisuInstaller.nsi dependency installation!!!
 **/
void installDependencies(bool removeInstalledFiles)
{
    if (removeInstalledFiles)
        UTLogger::instance()->logEntry("Starting dependency installation with cleanup.");
    else
        UTLogger::instance()->logEntry("Starting dependency installation without cleanup.");

    bool runAs = false;

    // For Vista and newer, we only want to RunAs when not running elevated
    if (!isPlatformXP())
    {
        // Determine if we are running with elevation
        HANDLE hToken;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            TOKEN_ELEVATION_TYPE elevationType;
            DWORD dwSize;
            if (GetTokenInformation(hToken, TokenElevationType, &elevationType, sizeof(elevationType), &dwSize))
            {
                if (elevationType == TokenElevationTypeLimited)
                {
                    UTLogger::instance()->logEntry("Not running elevated, installing dependencies using runas.");
                    runAs = true;
                }
            }
            CloseHandle(hToken);
        }
    }
    
    //Install VS2013 Runtimes - only install x64 on 64 bit
    if (IsWow64() && !VerifyRegistryValue(L"SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\12.0\\VC\\Runtimes\\x64", L"Installed", L"\x1"))
    {
        UTLogger::instance()->logEntry("Installing VS2013 64 bit runtimes.");
        execProcess(L"vcredist_x64.exe", L"/q", runAs, true);
    }
    //install x86 on 64 and 32 bit
    if (!VerifyRegistryValue(L"SOFTWARE\\Microsoft\\VisualStudio\\12.0\\VC\\Runtimes\\x86", L"Installed", L"\x1"))
    {
        UTLogger::instance()->logEntry("Installing VS2013 32 bit runtimes.");
        execProcess(L"vcredist_x86.exe", L"/q", runAs, true);
    }
    // install client service - Newer than WinXP only
    if (!isPlatformXP())
    {
        UTLogger::instance()->logEntry("Installing Client Service.");
        execProcess(L"OriginClientService.exe", L"/nsisinstall", runAs, true);
    }

    UTLogger::instance()->logEntry("Installing Web Helper Service.");
    execProcess(L"OriginWebHelperService.exe", L"/nsisinstall", runAs, true);
}

void logFileAttributes(const DWORD attributes);
BOOL setFileAttribute( const wchar_t * const fileToDelete, const DWORD fileAttribToSet );

/*
 * deletes file
 */
BOOL deleteFile( const wchar_t * const fileToDelete )
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
                    UTLogger::instance()->logEntry(wstring(L"File not found: ") + fileToDelete);
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
        UTLogger::instance()->logEntry(wstring(L"Attempting deletion operation: ") + fileToDelete, attempts, true);
        Sleep(SLEEP_TIME);
    } while ((attempts != MAX_ATTEMPTS) && !success);

    if (success)
    {
        UTLogger::instance()->logEntry(wstring(L"SUCCESFULLY Deleted: ") + fileToDelete, attempts);
    }
    else
    {
        UTLogger::instance()->logEntry(wstring(L"Deletion operation FAILED for : ") + fileToDelete, attempts, true);
    }
    return success;
}

/*
 * tries to move file
 */
BOOL moveFile(const wchar_t * const fileFrom, const wchar_t * const fileTo )
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
        UTLogger::instance()->logEntry(wstring(L"Attempting moving operation from ") + fileFrom + wstring(L" to ") + fileTo, attempts, true);
        Sleep(SLEEP_TIME);
    } while ((attempts != MAX_ATTEMPTS) && !success);

    if (success)
    {
        UTLogger::instance()->logEntry(wstring(L"SUCCESFULLY moved from: ") + fileFrom + wstring(L" to ") + fileTo, attempts);
    }
    else
    {
        UTLogger::instance()->logEntry(wstring(L"FAILED Moving operation from ") + fileFrom + wstring(L" to ") + fileTo, attempts, true);
    }
    return success;
}

BOOL copyFile(const wchar_t * const fileFrom, const wchar_t * const fileTo )
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
            setFileAttribute(fileTo, FILE_ATTRIBUTE_NORMAL);
        }
        attempts++;
        UTLogger::instance()->logEntry(wstring(L"Retrying copy operation from ") + fileFrom + wstring(L" to ") + fileTo, attempts, true);
        Sleep(SLEEP_TIME);
    } while ((attempts != MAX_ATTEMPTS) && !success);

    if (success)
    {
        UTLogger::instance()->logEntry(wstring(L"SUCCESFULLY copied from: ") + fileFrom + wstring(L" to ") + fileTo, attempts);
    }
    else
    {
        UTLogger::instance()->logEntry(wstring(L"FAILED Copy operation from ") + fileFrom + wstring(L" to ") + fileTo, attempts, true);
    }
    return success;
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
    UTLogger::instance()->logEntry(data, true);
}
/*
 * gets and set file attribute to the passed parameter
 */
BOOL setFileAttribute( const wchar_t * const fileToProcess, const DWORD fileAttribToSet )
{
    const DWORD attributes = GetFileAttributes(fileToProcess); 
    if (INVALID_FILE_ATTRIBUTES==attributes) 
    {
        UTLogger::instance()->logEntry(wstring(L"INVALID_FILE_ATTRIBUTES: ") + fileToProcess, true);
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
        UTLogger::instance()->logEntry("SetFileAttributes failed.", true);
    }
    return res; 
}

/*
 * Returns true if the file exists
 **/
BOOL fileExists(const wchar_t * const file)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE handle = FindFirstFile(file, &FindFileData) ;
    BOOL found = handle != INVALID_HANDLE_VALUE;
    if(found) 
    {
        FindClose(handle);
    }
    return found;
}

BOOL createFolderIfNotExist( const wchar_t * const folder)
{
    BOOL success = false;
    int attempts = 0;
    do 
    {
        UTLogger::instance()->logEntry(wstring(L"Creating folder (if not existing): ") + std::wstring(folder));

        // Ensure our SelfUpdate working folder exists
        int dirResult = SHCreateDirectoryEx(NULL, folder, NULL);
        if (dirResult == ERROR_SUCCESS)
        {
            success = true;
            break;
        }
        if (dirResult == ERROR_ALREADY_EXISTS)
        {
            return TRUE;
        }

        attempts++;
        Sleep(SLEEP_TIME);
    } while ((attempts != MAX_ATTEMPTS) && !success);

    if (success)
    {
        UTLogger::instance()->logEntry(wstring(L"SUCCESSFULLY created folder: ") + std::wstring(folder), true);
    }
    else
    {
        UTLogger::instance()->logEntry(wstring(L"FAILED to create folder: ") + std::wstring(folder), true);
    }
    return success;
}

HRESULT createLink(LPCWSTR lpszPathObj, LPCWSTR lpszLinkName, LPCWSTR lpszWorkingPath, LPCWSTR lpszDesc) 
{ 
    HRESULT hres; 
    IShellLink* psl; 

    hres = ::CoInitialize(NULL);
    if (!SUCCEEDED(hres))
        return hres;

    // find Start Menu\Programs\Origin path
    // common program groups that appear on the Start menu for all users.
    LPITEMIDLIST pidl;
   
    // Get a pointer to an item ID list that represents the path of a special folder.
    HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidl);

    // Convert the item ID list's binary representation into a file system path.
    wchar_t specialFolderPath[_MAX_PATH-1];

    // SpecialFolderPath is now Start Menu -> Programs path.
    BOOL f = SHGetPathFromIDList(pidl, specialFolderPath);

    // Allocate a pointer to an IMalloc interface
    LPMALLOC pMalloc;

    // Get the address of our task allocator's IMalloc interface
    hr = SHGetMalloc(&pMalloc);

    // Free the item ID list allocated by SHGetSpecialFolderLocation
    pMalloc->Free(pidl);

    // Free our task allocator
    pMalloc->Release();

    if (f == false)
    {
         UTLogger::instance()->logEntry("createLink failed.  Could not obtain CSIDL_COMMON_PROGRAMS");
         return E_FAIL;
    }

    wchar_t linkPath[_MAX_PATH];

    wsprintfW(linkPath, L"%s\\Origin\\%s.lnk", specialFolderPath, lpszLinkName);

    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
    // has already been called.
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 

        UTLogger::instance()->logEntry(wstring(L"working path:") + lpszWorkingPath);

        // Set the path to the shortcut target and add the description. 
        psl->SetPath(lpszPathObj); 
        psl->SetWorkingDirectory(lpszWorkingPath); 
        psl->SetDescription(lpszDesc); 

        // Query IShellLink for the IPersistFile interface, used for saving the 
        // shortcut in persistent storage. 
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf); 

        if (SUCCEEDED(hres)) 
        {
            // Save the link by calling IPersistFile::Save. 
            hres = ppf->Save(linkPath, TRUE); 
            ppf->Release(); 
        } 
        psl->Release(); 
    } 

    ::CoUninitialize();

    return hres; 
}


void updateShortcuts()
{
    wchar_t path[MAX_PATH] = L"";
    GetModuleFileName(NULL, path, MAX_PATH);
    std::wstring location(path);
    // Remove executable
    int lastSlash = location.find_last_of(L"\\");
    location = location.substr(0, lastSlash + 1);
    wchar_t originERLocation[MAX_PATH];
    wsprintfW(originERLocation, L"%sOriginER.exe", location.c_str());

    // create a shortcut to the Origin Error Reporter
    if (fileExists(originERLocation))
    {
        HRESULT hres;
        wchar_t originERWorkingDir[MAX_PATH];
        wsprintfW(originERWorkingDir, L"%s", location.c_str());

        hres = createLink(originERLocation, L"Origin Error Reporter", originERWorkingDir, L"OriginER");

        if (SUCCEEDED(hres))
            UTLogger::instance()->logEntry(wstring(L"Created shortcut to Origin Error Reporter : ") + originERLocation);
        else
        {
            wchar_t error_str[1024];
            wsprintfW(error_str, L"[ERROR] Error (%d) creating shortcut to Origin Error Reporter", hres);
            UTLogger::instance()->logEntry(error_str + wstring(L" : ") + originERLocation);
        }
    }
}

// VerifyEmbeddedSignature
//
// Takes the path of a file and checks to see if it has a valid signature
//
// http://msdn.microsoft.com/en-us/library/ff554705%28VS.85%29.aspx
// http://msdn.microsoft.com/en-us/library/aa382384(v=VS.85).aspx
bool verifyEmbeddedSignature(LPCWSTR pwszSourceFile)
{
    LONG lStatus;
    DWORD dwLastError;
    bool verified = false;
    // Initialize the WINTRUST_FILE_INFO structure.

    WINTRUST_FILE_INFO FileData;
    memset(&FileData, 0, sizeof(FileData));
    FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
    FileData.pcwszFilePath = pwszSourceFile;
    FileData.hFile = NULL;
    FileData.pgKnownSubject = NULL;

    /*
    WVTPolicyGUID specifies the policy to apply on the file
    WINTRUST_ACTION_GENERIC_VERIFY_V2 policy checks:

    1) The certificate used to sign the file chains up to a root 
    certificate located in the trusted root certificate store. This 
    implies that the identity of the publisher has been verified by 
    a certification authority.

    2) In cases where user interface is displayed (which this example
    does not do), WinVerifyTrust will check for whether the  
    end entity certificate is stored in the trusted publisher store,  
    implying that the user trusts content from this publisher.

    3) The end entity certificate has sufficient permission to sign 
    code, as indicated by the presence of a code signing EKU or no 
    EKU.
    */

    GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA WinTrustData;

    // Initialize the WinVerifyTrust input data structure.

    // Default all fields to 0.
    memset(&WinTrustData, 0, sizeof(WinTrustData));

    WinTrustData.cbStruct = sizeof(WinTrustData);

    // Use default code signing EKU.
    WinTrustData.pPolicyCallbackData = NULL;

    // No data to pass to SIP.
    WinTrustData.pSIPClientData = NULL;

    // Disable WVT UI.
    WinTrustData.dwUIChoice = WTD_UI_NONE;

    // No revocation checking.
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE; 

    // Verify an embedded signature on a file.
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;

    // Default verification.
    WinTrustData.dwStateAction = 0;

    // Not applicable for default verification of embedded signature.
    WinTrustData.hWVTStateData = NULL;

    // Not used.
    WinTrustData.pwszURLReference = NULL;

    // Only verify that the signer matches. This allows builds with dev certificates to be
    // tested by QA outside the EA domain.
    WinTrustData.dwProvFlags = WTD_HASH_ONLY_FLAG;

    // This is not applicable if there is no UI because it changes 
    // the UI to accommodate running applications instead of 
    // installing applications.
    WinTrustData.dwUIContext = 0;

    // Set pFile.
    WinTrustData.pFile = &FileData;

    // WinVerifyTrust verifies signatures as specified by the GUID 
    // and Wintrust_Data.
    lStatus = WinVerifyTrust(
        NULL,
        &WVTPolicyGUID,
        &WinTrustData);

    switch (lStatus) 
    {
    case ERROR_SUCCESS:
        /*
        Signed file:
        - Hash that represents the subject is trusted.

        - Trusted publisher without any verification errors.

        - UI was disabled in dwUIChoice. No publisher or 
        time stamp chain errors.

        - UI was enabled in dwUIChoice and the user clicked 
        "Yes" when asked to install and run the signed 
        subject.
        */

        UTLogger::instance()->logEntry(wstring(L"Successfully verified that file [") + wstring(pwszSourceFile) + wstring(L"] was signed and signature is verified."));

        verified = true;
        break;

    case TRUST_E_NOSIGNATURE:
        // The file was not signed or had a signature 
        // that was not valid.

        // Get the reason for no signature.
        dwLastError = GetLastError();
        if (TRUST_E_NOSIGNATURE == dwLastError ||
            TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError ||
            TRUST_E_PROVIDER_UNKNOWN == dwLastError) 
        {
            // The file was not signed.
            UTLogger::instance()->logEntry(wstring(L"Failed to verify embedded signature for file [") + wstring(pwszSourceFile) + wstring(L"]: File was not signed."), true);
        } 
        else 
        {
            // The signature was not valid or there was an error 
            // opening the file.
            UTLogger::instance()->logEntry(wstring(L"Failed to verify embedded signature for file [") + wstring(pwszSourceFile) + wstring(L"]: Unknown error occurred attempting to verify."), true);
        }

        break;

    case TRUST_E_EXPLICIT_DISTRUST:
        // The hash that represents the subject or the publisher 
        // is not allowed by the admin or user.
        UTLogger::instance()->logEntry(wstring(L"Failed to verify embedded signature for file [") + wstring(pwszSourceFile) + wstring(L"]: Signature is present, but specifically disallowed."), true);

        break;

    case TRUST_E_SUBJECT_NOT_TRUSTED:
        // The user clicked "No" when asked to install and run.
        UTLogger::instance()->logEntry(wstring(L"Failed to verify embedded signature for file [") + wstring(pwszSourceFile) + wstring(L"]: Signature is present, but not trusted."), true);

        break;

    case CRYPT_E_SECURITY_SETTINGS:
        /*
        The hash that represents the subject or the publisher 
        was not explicitly trusted by the admin and the 
        admin policy has disabled user trust. No signature, 
        publisher or time stamp errors.
        */
        UTLogger::instance()->logEntry(wstring(L"Failed to verify embedded signature for file [") + wstring(pwszSourceFile) + wstring(L"]: The hash ") +
                                       wstring(L"representing the subject or the publisher wasn't ") + 
                                       wstring(L"explicitly trusted by the admin and admin policy ") + 
                                       wstring(L"has disabled user trust. No signature, publisher ") + 
                                       wstring(L"or timestamp errors."), true);

        break;

    default:
        // The UI was disabled in dwUIChoice or the admin policy 
        // has disabled user trust. lStatus contains the 
        // publisher or time stamp chain error.
        {
            wchar_t errorMsg[128] = {0};
            wsprintfW(errorMsg, L"Error is: 0x%lx.", lStatus);
            UTLogger::instance()->logEntry(wstring(L"Failed to verify embedded signature for file [") + wstring(pwszSourceFile) + wstring(L"]: ") + wstring(errorMsg), true);
        }
        break;
    }

    return verified;
}

// GetCertificateContext
//
// Given an path and exe grab the certificate info
void getCertificateContext(LPCWSTR szFileName,  PCCERT_CONTEXT &pCertContext)
{
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL; 

    BOOL fResult;   
    DWORD dwEncoding, dwContentType, dwFormatType;
    PCMSG_SIGNER_INFO pSignerInfo = NULL;
    DWORD dwSignerInfo;    
    CERT_INFO CertInfo;

    // Get message handle and store handle from the signed file.
    fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
        szFileName,
        CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
        CERT_QUERY_FORMAT_FLAG_BINARY,
        0,
        &dwEncoding,
        &dwContentType,
        &dwFormatType,
        &hStore,
        &hMsg,
        NULL);
    if (fResult == FALSE)
        goto cert_cleanup;

    // Get signer information size.
    fResult = CryptMsgGetParam(hMsg, 
        CMSG_SIGNER_INFO_PARAM, 
        0, 
        NULL, 
        &dwSignerInfo);
    if (fResult == FALSE)
        goto cert_cleanup;

    // Allocate memory for signer information.
    pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
    if (!pSignerInfo)
        goto cert_cleanup;

    // Get Signer Information.
    fResult = CryptMsgGetParam(hMsg, 
        CMSG_SIGNER_INFO_PARAM, 
        0, 
        (PVOID)pSignerInfo, 
        &dwSignerInfo);
    if (fResult == FALSE)
        goto cert_cleanup;

    // Search for the signer certificate in the temporary certificate store.
    CertInfo.Issuer = pSignerInfo->Issuer;
    CertInfo.SerialNumber = pSignerInfo->SerialNumber;

    pCertContext = CertFindCertificateInStore(hStore,
        ENCODING,
        0,
        CERT_FIND_SUBJECT_CERT,
        (PVOID)&CertInfo,
        NULL);

cert_cleanup:
    //the calling function is responsible for cleaning up pCertContext

    //clean up
    if (pSignerInfo != NULL) 
        LocalFree(pSignerInfo);

    if (hStore != NULL) 
        CertCloseStore(hStore, 0);

    if (hMsg != NULL) 
        CryptMsgClose(hMsg);

}

// isEACertificate
// 
// Compares specified binary certificate to see if it is an EA certificate
bool isValidEACertificate(LPCWSTR szBinaryFileName)
{
    // To begin with, the embedded signature must be valid
    if (!verifyEmbeddedSignature(szBinaryFileName))
    {
        return false;
    }

    // Next, get the certificate context and see if it is actually an EA certificate
    bool validated = false;
    
    PCCERT_CONTEXT binaryCert = NULL;

    // Get the certificate context for specified binary
    // This will allocate memory for the returned context so we must free it below
    getCertificateContext(szBinaryFileName, binaryCert);

    if (!binaryCert)
    {
        UTLogger::instance()->logEntry(wstring(L"Failed to verify signing certificate for file [") + wstring(szBinaryFileName) + wstring(L"]: Unable to read certificate."), true);
        return false;
    }

    wchar_t szSubjectNameString[256] = {0};
    if(CertGetNameString(binaryCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, szSubjectNameString, sizeof(szSubjectNameString)-1))
    {
        // Make sure it is an EA certificate
        if (_wcsicmp(szSubjectNameString, ORIGIN_CODESIGNING_SUBJECT_NAME) == 0)
        {
            validated = true;
        }
    }
    else
    {
        UTLogger::instance()->logEntry(wstring(L"Unable to read certificate subject name from file: ") + wstring(szBinaryFileName), true);
    }

    if (validated)
    {
        UTLogger::instance()->logEntry(wstring(L"The file has a valid EA certificate: ") + wstring(szBinaryFileName));
    }
    else
    {
        UTLogger::instance()->logEntry(wstring(L"Certificate was valid but incorrect subject name (i.e. wrong certificate) for file: ") + wstring(szBinaryFileName));
    }

    // Free the context
    CertFreeCertificateContext(binaryCert);

    return validated;
}