//  LogServicePlatform_win.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "LogService_win.h"

#include <Windows.h>
#include "Shlwapi.h"
#include "ShlObj.h"

#include <AccCtrl.h>
#include <AclAPI.h>
#include <Sddl.h>

using namespace std;

#define ENABLED 1

bool grantEveryoneAccessToFile(const std::wstring& sFileName);
    
///////////////////////////////////////////////////////////////////////////////
//	LOGSERVICEPLATFORM
///////////////////////////////////////////////////////////////////////////////

// Static mutex member variable definition
static CRITICAL_SECTION sMutex;

void LogServicePlatform::initializeMutex ()
{
#if ENABLED
    InitializeCriticalSection (&sMutex);
#endif
}

void LogServicePlatform::deleteMutex ()
{
#if ENABLED
    DeleteCriticalSection (&sMutex);
#endif
}

void LogServicePlatform::lockMutex ()
{
    EnterCriticalSection (&sMutex);
}

void LogServicePlatform::unlockMutex ()
{
    LeaveCriticalSection (&sMutex);
}

EXPLICIT_ACCESS eaForWellKnownGroup(PSID groupSID)
{
    EXPLICIT_ACCESS ea={0};
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfInheritance = CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE;
    ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;  	
    ea.Trustee.ptstrName = (LPTSTR)groupSID;

    return ea;
}

bool LogServicePlatform::grantEveryoneAccessToFile(const std::wstring& sFileName)
{
    bool ret = false;

    HANDLE hDir = CreateFile(sFileName.c_str(), READ_CONTROL|WRITE_DAC,0,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
    if (hDir == INVALID_HANDLE_VALUE) 
    {
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
            ret = (SetSecurityInfo(hDir,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,pNewDACL,NULL) == ERROR_SUCCESS);
        }

        LocalFree(pNewDACL);
    }

    LocalFree(pSD);

    CloseHandle(hDir);

    return ret;
}

bool LogServicePlatform::pathExists (const wchar_t *path)
{
    return (::PathFileExists(path) ? true : false);
}

void LogServicePlatform::createPath (const wchar_t *path)
{
    SHCreateDirectoryEx( NULL, path, NULL ); // Create (if it doesn't exist) the full path to our log directory
}
