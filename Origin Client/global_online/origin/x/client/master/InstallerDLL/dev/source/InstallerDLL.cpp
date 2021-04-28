// InstallerDLL.cpp : Defines the entry point for the DLL application.
//

// telemetry
#include "TelemetryAPIDLL.h"

#include "stdafx.h"
#include "resource.h"

#include "InstallerDLL.h"
//#include "CmdPortalClient.h"
#include "CoreParameterizer.h"
#include <ShlObj.h>
#include <Shlwapi.h>
#include <Tlhelp32.h>
#include <Psapi.h>
#include <winsafer.h>
#include <Wtsapi32.h>
#include "UninstallHelpers.h"

#include "Aclapi.h"
#include "Sddl.h"

#include <EAIO/PathString.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAIniFile.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EATextUtil.h>

typedef std::basic_string<TCHAR> tstring;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//Helpers
static bool ConvertWideCharToMCBS(LPCWSTR WideString, char *sBuffer, size_t nBuffSize);
static bool LoadCStringFromFile(const CString& sFilename, CString& string);
static bool SaveCStringToFile(const CString& sFilename, CString& string);
static CString ReadRegString( HKEY hRoot, LPCTSTR lpSubKey, LPCTSTR lpValueName );
static CString ReadIniString( LPCTSTR lpIniFile, LPCTSTR lpSection, LPCTSTR lpEntry );
static bool WriteIniString( LPCTSTR lpIniFile, LPCTSTR lpSection, LPCTSTR lpKey, LPCTSTR lpEntry );
static void Output( LPCTSTR szFormat, ... );
static bool GetParentInfo(DWORD pid, PROCESSENTRY32* pPe);
static int FindNoCase(const CString& strString, const CString& strLookFor, int nStartAt = 0);
static int FindNoCase(const tstring& strString, const tstring& strLookFor, int nStartAt = 0);
static CString GetISOLocale(const CString& strLocale);
bool MoveFolder(tstring srcPath, tstring destPath);

static void DebugMessageBox(LPCTSTR szFormat,...)
{
#ifdef _DEBUG
	va_list ap;
	va_start(ap, szFormat);
	CString szText;
	szText.FormatV(szFormat, ap);
	MessageBox(NULL, szText, L"InstallerDLL", MB_OK);
	va_end(ap);
#endif
}

static int GetInstalledVersionMajor()
{
	TCHAR szBuffer[MAX_PATH + 1];
	DWORD dwType = 0;
	DWORD dwSize = sizeof(szBuffer);
	*szBuffer = L'\0';

	if ( SHGetValue(HKEY_LOCAL_MACHINE,
				L"SOFTWARE\\Electronic Arts\\EA Core",
				L"ClientVersion",
				&dwType,
				szBuffer,
				&dwSize) == ERROR_SUCCESS )
	{
		if ( dwType == REG_SZ && _istdigit(*szBuffer) )
		{
			TCHAR digit = *szBuffer;
			return digit - L'0';
		}
	}

	return 0;
}

static CString GetEADM4Preferences(bool bUnencode = true)
{
	CString sData;

	CString sCachePath = ReadRegString(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Electronic Arts\\EA Core"), _T("CachePath"));

	if ( sCachePath.IsEmpty() )
	{
		//Try backup key
		sCachePath = ReadRegString(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Electronic Arts\\EAL"), _T("CachePath"));
	}

	if ( sCachePath.IsEmpty() )
		return sData;


	if ( sCachePath.Right(1) != _T("\\") )
	{
		sCachePath.Append(_T("\\"));
	}

	//Test for EADM/cache components, and append if they are not anywhere in the path
	if ( FindNoCase(sCachePath,_T("\\EADM\\cache")) < 0 )
	{
		sCachePath.Append(_T("EADM\\cache\\"));
	}
	
	sCachePath += _T("Prefs.ead");

	// Read the data
	if ( !LoadCStringFromFile(sCachePath, sData) )
	{
		DebugMessageBox(L"GetLegacyPreferences %s:\n%s", sCachePath, (LPCTSTR) sData);
		return CString();
	}

	// Unencode it
	if( bUnencode && (UrlUnescapeInPlace(sData.GetBuffer(), 0) != S_OK) ) 
	{
		return CString();
	}

	sData.ReleaseBuffer();

	DebugMessageBox(L"GetEADM4Preferences %s:\n%s", (LPCTSTR) sCachePath, (LPCTSTR) sData);
	return sData;
}

static CString GetEADM5Preferences(bool bUnencode = true)
{
	CString sData;
	TCHAR prefsFilePath[MAX_PATH];

	// Use the preferences files in the "Electronic Arts" dir
	// The data is retrieved from CSIDL_COMMON_APPDATA  (D&S/All Users/Application Data)
	SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_DEFAULT, prefsFilePath);
	PathAppend(prefsFilePath, _T("Electronic Arts\\eadm_app.prefs"));

	// Read the data
	if ( !LoadCStringFromFile(prefsFilePath, sData) )
	{
		DebugMessageBox(L"GetLegacyPreferences %s:\n%s", prefsFilePath, (LPCTSTR) sData);
		return CString();
	}

	if ( sData.IsEmpty() )
		return CString();

	CString sCachePath;
	const CString sKey = CString(L"software\\electronic arts\\ea core");
	const CString sCacheMarker = CString (L"[CachePath:");

	Core::Parameterizer params;

	if ( params.Parse(sData) )
	{
		sCachePath = params.GetString(sKey);

		int nPos = sCachePath.Find(sCacheMarker);

		if (nPos >= 0)
		{
			nPos += sCacheMarker.GetLength();
			int nPathPosEnd = sCachePath.Find(L"]", nPos);
			sCachePath = sCachePath.Mid(nPos, nPathPosEnd - nPos);
		}
	}

	if ( !sCachePath.IsEmpty() )
	{
		//Test for EADM/cache components, and append if they are not anywhere in the path
		if ( FindNoCase(sCachePath,_T("\\EADM\\cache")) < 0 )
		{
			sCachePath.Append(_T("EADM\\cache\\"));
		}

		sCachePath += _T("Prefs.ead");

		// Read the data
		if ( !LoadCStringFromFile(sCachePath, sData) )
		{
			DebugMessageBox(L"GetLegacyPreferences %s:\n%s", sCachePath, (LPCTSTR) sData);
			return CString();
		}

		// Unencode it
		if( bUnencode && (UrlUnescapeInPlace(sData.GetBuffer(), 0) != S_OK) ) 
		{
			return CString();
		}

		sData.ReleaseBuffer();
	}

	DebugMessageBox(L"GetEADM5Preferences %s:\n%s", (LPCTSTR) sCachePath, (LPCTSTR) sData);

	return sData;
}

static CString GetCachePathEADM4()
{
	CString sRet;
	CString sData = GetEADM4Preferences(false);
	Core::Parameterizer params;

	if ( params.Parse(sData) )
	{
		sRet = params.GetString(L"base_cache_dir");
	}

	return sRet;
}

static CString GetLocaleEADM4()
{
	CString sRet;
	CString sData = GetEADM4Preferences(false);
	Core::Parameterizer params;

	if ( params.Parse(sData) )
	{
		sRet = params.GetString(L"locale");
	}

	return sRet;
}

static CString GetCachePathEADM5()
{
	CString sRet;
	CString sData = GetEADM5Preferences(false);
	
	Core::Parameterizer params;

	if ( params.Parse(sData) )
	{
		sRet = params.GetString(L"base_cache_dir");
	}

	return sRet;
}

static CString GetEADM6PreferencesFile()
{
	CString sIniPath = ReadRegString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Electronic Arts\\EA Core", L"EADM6InstallDir");

	if ( sIniPath.IsEmpty() )
		return CString();

	sIniPath.Append(L"\\EACore_App.ini");

	return sIniPath;
}

static CString GetEADM6RTPreferencesFile()
{
	CString sIniPath = GetEADM6PreferencesFile();

	if ( sIniPath.IsEmpty() )
		return sIniPath;

	CString sServerId;

	if ( !GetPrivateProfileString(L"EACore", L"SharedServerId", NULL, sServerId.GetBufferSetLength(256), 256, sIniPath ) )
		return CString();

	sServerId.ReleaseBuffer();

	// get local application data directory
	CString sBaseDir; 
	SHGetSpecialFolderPath(NULL, sBaseDir.GetBufferSetLength(MAX_PATH+1), CSIDL_COMMON_APPDATA, false);
	sBaseDir.ReleaseBuffer();

	// Check ending slashes for OS compatibility ?
	if (sBaseDir.Right(1) != _T("\\"))
		sBaseDir.Append(_T("\\"));

	sIniPath.Format(L"%sElectronic Arts\\EA Core\\prefs\\%s.ini", sBaseDir, sServerId);

	return sIniPath;
}

static CString GetEADM6Preference(LPCTSTR lpSection, LPCTSTR lpEntry)
{
	CString sValue;
	CString sIniFile = GetEADM6RTPreferencesFile();

	if ( !sIniFile.IsEmpty() )
	{
		sValue = ReadIniString(sIniFile, lpSection, lpEntry);
	}

	if ( sValue.IsEmpty() )
	{
		sIniFile = GetEADM6PreferencesFile();

		if ( !sIniFile.IsEmpty() )
		{
			sValue = ReadIniString(sIniFile, lpSection, lpEntry);
		}
	}

	return sValue;
}

static CString GetCachePathEADM6()
{
	return GetEADM6Preference(L"EACoreSession", L"full_base_cache_dir");
}

static CString GetLocaleEADM5()
{
	CString sRet;
	CString sData = GetEADM5Preferences(false);
	Core::Parameterizer params;

	if ( params.Parse(sData) )
	{
		sRet = params.GetString(L"locale");
	}

	return sRet;
}

#ifdef _DEBUG
extern "C" __declspec(dllexport) void CALLBACK RunDLLEntryPoint(HWND hwnd,HINSTANCE hInstance,LPTSTR lpCmdLine,int nCmdShow)
{
	CString sCache = GetCachePathEADM6();
	//CString sLocale = GetLocaleEADM5();
	//RelativizeManifestPaths(_T("C:\\pepe\\EADM\\cache"), 
	//						_T("C:\\ProgramData\\Electronic Arts\\EADM\\cache"));
	//TCHAR sBuffer[MAX_PATH];
	//GetCurrentLocale(sBuffer, MAX_PATH);
	return;

	//GetLocaleLegacy();
	//GetLocale();

	//GetCurrentCachePath(sBuffer, MAX_PATH);
}
#endif

extern "C" BOOL PASCAL EXPORT GetCurrentCachePath( wchar_t* sEADMCachePath,	// In/Out: Really, it's just a buffer for the command path
												   size_t nBufferSize )		// In: The number of bytes in the buffer
{
	bool rval = false;
	CString szCachePath;

	if ( nBufferSize == 0 )
		return false;

	*sEADMCachePath = L'\0';

	int iVerMajor = GetInstalledVersionMajor();

	if ( iVerMajor == 4 )
	{
		szCachePath = GetCachePathEADM4();
	}
	else if ( iVerMajor == 5 )
	{
		szCachePath = GetCachePathEADM5();
	}
	else if ( iVerMajor > 5 )
	{
		szCachePath = GetCachePathEADM6();
	}

	if ( !szCachePath.IsEmpty() )
	{
		if ( iVerMajor < 6 )
		{
			//Test for EADM/cache components, and append if they are not anywhere in the path
			if ( FindNoCase(szCachePath,_T("\\EADM\\cache")) < 0 )
			{
				if ( szCachePath.Right(1) != _T("\\") )
				{
					szCachePath.Append(_T("\\"));
				}

				szCachePath.Append(_T("EADM\\cache"));
			}
		}

		if ( szCachePath.GetLength() < (int) nBufferSize )
		{
			lstrcpy(sEADMCachePath, szCachePath);
			rval = true;
		}
		else
		{
			//Convert to the short version
			CString szShortPath;

			if ( GetShortPathName((LPCTSTR) szCachePath, szShortPath.GetBufferSetLength(MAX_PATH), MAX_PATH) <= MAX_PATH )
			{
				szShortPath.ReleaseBuffer();

				if ( szShortPath.GetLength() < (int) nBufferSize )
				{
					lstrcpy(sEADMCachePath, szShortPath);
					rval = true;
				}
			}
		}
	}
	
	return rval;
}

extern "C" BOOL PASCAL EXPORT GetCurrentLocale(wchar_t* szLocale,
											   size_t nBufferSize)
{
	bool rval = false;
	CString sLocale;

	if ( nBufferSize == 0 )
		return false;

	*szLocale = L'\0';

	int iVerMajor = GetInstalledVersionMajor();

	if ( iVerMajor == 4 )
	{
		sLocale = GetLocaleEADM4();
	}
	else if ( iVerMajor == 5 )
	{
		sLocale = GetLocaleEADM5();
	}
	else if ( iVerMajor > 5 )
	{
		//EADM6 uses per client locale
	}

	CString sFullISOLocale = GetISOLocale(sLocale);

	DebugMessageBox(L"GetCurrentLocale %s -> %s", (LPCTSTR) sLocale, (LPCTSTR) sFullISOLocale);
	
	if ( !sFullISOLocale.IsEmpty() )
	{
		if ( sFullISOLocale.GetLength() < (int) nBufferSize )
		{
			lstrcpy(szLocale, sFullISOLocale);
			rval = true;
		}
	}

	return rval;
}
/*
static bool KillEACoreServerProcess()
{
	CString sServerPath = ReadRegString(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Electronic Arts\\EA Core"), _T("ClientPath"));

	if ( !sServerPath.IsEmpty() )
	{
		int idx = sServerPath.ReverseFind( _T('\\') ) + 1;

		if ( idx > 0 )
		{
			sServerPath = sServerPath.Right( sServerPath.GetLength() - idx );
		}
	}
	else
	{
		sServerPath = _T("Core.exe");
	}

	return KillProcess( sServerPath ) ? TRUE : FALSE;
}
*/
// extern "C" BOOL PASCAL EXPORT ShutdownEACore()
// {
// 	Output(_T("ShutdownEACore"));
// 
// 	CString sCmdPortalPath = ReadRegString(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Electronic Arts\\EA Core Server"), _T("ClientAccessDLLPath"));
// 
// 	if ( sCmdPortalPath.IsEmpty() )
// 	{
// 		sCmdPortalPath = _T("CoreCmdPortalClient.dll");
// 		Output(_T("Failed to read ClientAccessDLLPath location in the registry"));
// 	}
// 
// 	HINSTANCE hCmdPortal = ::LoadLibrary( sCmdPortalPath );
// 
// 	if ( hCmdPortal == NULL )
// 	{
// 		Output(_T("Failed to load %s"), sCmdPortalPath);
// 		return false;
// 	}
// 
// 	//Resolve the connect and the shutdown methods
// 	fdDMConnect3 fDM_Connect3 = (fdDMConnect3) ::GetProcAddress(hCmdPortal, "Connect3");
// 	fdDMCoreShutdown fDM_CoreShutdown = (fdDMCoreShutdown) ::GetProcAddress(hCmdPortal, "CoreShutdown");
// 	fdDMDisconnect fDM_Disconnect = (fdDMDisconnect) ::GetProcAddress(hCmdPortal, "Disconnect");
// 
// 	bool bResult = true;
// 
// 	if ( !fDM_Connect3 || !fDM_CoreShutdown )
// 	{
// 		Output(_T("Failed to resolve required entrypoints"));
// 		bResult = false;
// 	}
// 
// 	//Connect
// 	if ( bResult )
// 	{
// 		wchar_t sConnectionStateItemHandle[512];
// 		*sConnectionStateItemHandle = L'\0';
// 
// 		long long llClientGUID = 0LL;
// 
// 		E_CONNECT_COMMAND_RESPONSE eResponse = CCR_ERROR;
// 
// 		//Do not provide a locale to avoid the second request
// 		bResult = fDM_Connect3( eResponse, _T("localhost"), _T(""), _T(""), _T("EACoreInstaller"),
// 			llClientGUID, _T(""), sConnectionStateItemHandle, 
// 			_countof(sConnectionStateItemHandle), NULL );
// 
// 		if ( bResult )
// 		{
// 			bResult = eResponse == CCR_SUCCESS;
// 		}
// 
// 		if ( !bResult )
// 		{
// 			Output(_T("Connect failed. Response: %d"), eResponse);
// 		}
// 	}
// 
// 	//Shutdown
// 	if ( bResult )
// 	{
// 		E_CORE_SHUTDOWN_COMMAND_RESPONSE eResponse = CSCR_UNKNOWN;
// 		bResult = fDM_CoreShutdown( true, 10000, eResponse );
// 
// 		if ( bResult )
// 		{
// 			bResult = (eResponse == CSCR_SUCCESS);
// 		}
// 
// 		if ( !bResult )
// 		{
// 			Output(_T("CoreShutdown failed. Response: %d"), eResponse);
// 		}
// 	}
// 
// 	if ( bResult )
// 	{
// 		//Wait for core termination
// 		HANDLE hGlobalMutex = ::OpenMutex( SYNCHRONIZE, FALSE, _T("Global\\EACoreServer_InstanceMutex") );
// 
// 		if  ( hGlobalMutex )
// 		{
// 			DWORD dRes = ::WaitForSingleObject( hGlobalMutex, 7000 );
// 			::CloseHandle( hGlobalMutex );
// 
// 			if ( dRes != WAIT_OBJECT_0 )
// 			{
// 				Output(_T("Wait for EACore_InstanceMutex failed. Code: %d"), dRes);
// 				KillEACoreServerProcess();
// 			}
// 		}
// 	}
// 
// 	if ( fDM_Disconnect )
// 	{
// 		fDM_Disconnect();
// 	}
// 
// 	::FreeLibrary( hCmdPortal );
// 
// 	if ( !bResult )
// 	{
// 		KillEACoreServerProcess();
// 	}
// 
// 	return bResult ? TRUE : FALSE;
// }

extern "C" BOOL PASCAL EXPORT ShutdownProxyInstaller()
{
	bool result = true;

	HANDLE hExitEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, _T("Global\\ProxyInstallerShutdownEvent"));

	if ( hExitEvent )
	{
		Output(L"Signaling ProxyInstallerShutdownEvent");
		::SetEvent( hExitEvent );
		::CloseHandle( hExitEvent );
		Sleep(250);
	}
	else
	{
		Output(L"ProxyInstallerShutdownEvent not found");
	}

	return result ? TRUE : FALSE;
}

// PSAPI Function Pointers.
typedef BOOL (WINAPI *lpfEnumProcesses)( DWORD *, DWORD cb, DWORD * );
typedef BOOL (WINAPI *lpfEnumProcessModules)( HANDLE, HMODULE *, DWORD, LPDWORD );
typedef DWORD (WINAPI *lpfGetModuleBaseName)( HANDLE, HMODULE, LPTSTR, DWORD );
typedef DWORD (WINAPI *lpfGetModuleFileNameEx)( HANDLE, HMODULE, LPTSTR, DWORD );

extern "C" void PASCAL EXPORT KillProcess(wchar_t *sProcName, bool respectPath)
{
	BOOL bResult;
	DWORD aiPID[4096];//,iNumProc = 0;
	DWORD iCbneeded = 0;
	static TCHAR szLongProcName[32767];
	HINSTANCE hInstLib;

	Output( _T("Called KillProcess for %s"), sProcName );

	// Load library and get the procedures explicitly. We do
	// this so that we don't have to worry about modules using
	// this code failing to load under Windows 9x, because
	// it can't resolve references to the PSAPI.DLL.
	hInstLib = LoadLibrary(_T("PSAPI.DLL"));

	if ( hInstLib == NULL )
	{
		Output(_T("Failed to load PSAPI.DLL. Error: %u"), GetLastError());
		return;
	}

	// Get procedure addresses.
	lpfEnumProcesses pEnumProcesses = (lpfEnumProcesses) GetProcAddress( hInstLib, "EnumProcesses" );
	lpfEnumProcessModules pEnumProcessModules = (lpfEnumProcessModules) GetProcAddress( hInstLib, "EnumProcessModules" );
	lpfGetModuleBaseName pGetModuleBaseName = (lpfGetModuleBaseName) GetProcAddress( hInstLib,
#ifdef UNICODE		
		"GetModuleBaseNameW" ) ;
#else
		"GetModuleBaseNameA" ) ;
#endif

	lpfGetModuleFileNameEx pGetModuleFileNameEx = (lpfGetModuleFileNameEx) GetProcAddress( hInstLib,
#ifdef UNICODE		
		"GetModuleFileNameExW" ) ;
#else
		"GetModuleFileNameExA" ) ;
#endif

	if ( pEnumProcesses == NULL || pEnumProcessModules == NULL || pGetModuleBaseName == NULL  || pGetModuleFileNameEx == NULL )
	{
		Output(_T("Failed to resolve required entrypoints in PSAPI.DLL"));
		FreeLibrary(hInstLib);
		return;
	}

	bResult = pEnumProcesses(aiPID,_countof(aiPID),&iCbneeded);

	if ( !bResult )
	{
		// Unable to get process list, EnumProcesses failed
		Output(_T("EnumProcesses failed. Error: %u"), GetLastError());
		FreeLibrary(hInstLib);
		return;
	}

	// How many processes are there?
	DWORD iNumProc = iCbneeded/sizeof(DWORD);

	// Get and match the name of each process
	for( DWORD i = 0; i < iNumProc; i++ )
	{
		static TCHAR szName[32767];
		HANDLE hProc;
		
		// Get the (module) name for this process
		*szName = _T('\0');

		// First, get a handle to the process
		hProc = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,aiPID[i]);
		
		// Now, get the process name
		if ( hProc )
		{
			HMODULE hMod = NULL;
			if ( pEnumProcessModules(hProc,&hMod,sizeof(hMod),&iCbneeded) )
			{
				if (respectPath)
					pGetModuleFileNameEx(hProc,hMod,szName,_countof(szName));
				else
					pGetModuleBaseName(hProc,hMod,szName,_countof(szName));
			}
			
			CloseHandle(hProc);
		}


		// get the long file name too
		szLongProcName[0] = _T('\0');
		::GetLongPathName(sProcName, szLongProcName, _countof(szLongProcName));

		// We will match regardless of lower or upper case
		if ( *szName && ( (lstrcmpi(szName, sProcName) == 0) || (lstrcmpi(szName, szLongProcName) == 0)) )
		{
			// Process found, now terminate it

			// First open for termination
			hProc = OpenProcess(PROCESS_TERMINATE,FALSE,aiPID[i]);
			
			if ( hProc )
			{
				if ( TerminateProcess(hProc,0) )
				{
					// process terminated
					Output(_T("%s terminated successfully"), sProcName);
				}
				else
				{
					// Unable to terminate process
					Output(_T("TerminateProcess failed. Error: %u"), GetLastError());
				}
				
				CloseHandle(hProc);
				hProc = NULL;
			}
			else
			{
				// Unable to open process for termination
				Output(_T("OpenProcess failed. Error: %u"), GetLastError());
			}
		}
	}

	if ( hInstLib )
	{
		FreeLibrary(hInstLib);
		hInstLib = 0;
	}
}


extern "C" BOOL PASCAL EXPORT FindProcess(wchar_t* sProcName, bool respectPath)
{
	BOOL bResult;
	DWORD aiPID[4096];//,iNumProc = 0;
	DWORD iCbneeded = 0;
	static TCHAR szLongProcName[32767];
	HINSTANCE hInstLib;
	bool bFound = false;

	Output( _T("Called FindProcess for %s"), sProcName );

	// Load library and get the procedures explicitly. We do
	// this so that we don't have to worry about modules using
	// this code failing to load under Windows 9x, because
	// it can't resolve references to the PSAPI.DLL.
	hInstLib = LoadLibrary(_T("PSAPI.DLL"));

	if ( hInstLib == NULL )
	{
		Output(_T("Failed to load PSAPI.DLL. Error: %u"), GetLastError());
		return false;
	}

	// Get procedure addresses.
	lpfEnumProcesses pEnumProcesses = (lpfEnumProcesses) GetProcAddress( hInstLib, "EnumProcesses" );
	lpfEnumProcessModules pEnumProcessModules = (lpfEnumProcessModules) GetProcAddress( hInstLib, "EnumProcessModules" );
	lpfGetModuleBaseName pGetModuleBaseName = (lpfGetModuleBaseName) GetProcAddress( hInstLib,
#ifdef UNICODE		
		"GetModuleBaseNameW" ) ;
#else
		"GetModuleBaseNameA" ) ;
#endif

	lpfGetModuleFileNameEx pGetModuleFileNameEx = (lpfGetModuleFileNameEx) GetProcAddress( hInstLib,
#ifdef UNICODE		
		"GetModuleFileNameExW" ) ;
#else
		"GetModuleFileNameExA" ) ;
#endif

	if ( pEnumProcesses == NULL || pEnumProcessModules == NULL || pGetModuleBaseName == NULL  || pGetModuleFileNameEx == NULL )
	{
		Output(_T("Failed to resolve required entrypoints in PSAPI.DLL"));
		FreeLibrary(hInstLib);
		return false;
	}

	bResult = pEnumProcesses(aiPID,_countof(aiPID),&iCbneeded);

	if ( !bResult )
	{
		// Unable to get process list, EnumProcesses failed
		Output(_T("EnumProcesses failed. Error: %u"), GetLastError());
		FreeLibrary(hInstLib);
		return false;
	}

	// How many processes are there?
	DWORD iNumProc = iCbneeded/sizeof(DWORD);

	// Get and match the name of each process
	for( DWORD i = 0; i < iNumProc; i++ )
	{
		static TCHAR szName[32767];
		HANDLE hProc;
		
		// Get the (module) name for this process
		*szName = _T('\0');

		// First, get a handle to the process
		hProc = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,aiPID[i]);
		
		// Now, get the process name
		if ( hProc )
		{
			HMODULE hMod = NULL;
			if ( pEnumProcessModules(hProc,&hMod,sizeof(hMod),&iCbneeded) )
			{
				if (respectPath)
					pGetModuleFileNameEx(hProc,hMod,szName,_countof(szName));
				else
					pGetModuleBaseName(hProc,hMod,szName,_countof(szName));
			}
			
			CloseHandle(hProc);
		}


		// get the long file name too
		szLongProcName[0] = _T('\0');
		::GetLongPathName(sProcName, szLongProcName, _countof(szLongProcName));

		// We will match regardless of lower or upper case
		if ( *szName && ( (lstrcmpi(szName, sProcName) == 0) || (lstrcmpi(szName, szLongProcName) == 0)) )
		{
			// Process found
			bFound = true;
			break;
		}
	}

	if ( hInstLib )
	{
		FreeLibrary(hInstLib);
		hInstLib = 0;
	}

	return bFound;
}


static bool RelativizeManifestContent(LPCTSTR szFileName, LPCTSTR szBaseDir)
{
	bool result = false;

	CString sContent;
	LoadCStringFromFile(szFileName, sContent);

	Output(_T("Fixing manifest paths for %s"), szFileName);

	if ( !sContent.IsEmpty() )
	{
		Core::Parameterizer p;
		
		if ( p.Parse(sContent) )
		{
			bool bModified = false;
			Core::tKeyToValueMap valueMap;
			p.GetAllParams(valueMap);

			CString sBaseDir(szBaseDir);

			Core::tKeyToValueMap::iterator it;
			for ( it = valueMap.begin(); it != valueMap.end(); it++ )
			{
				//We go trough GetString() so it gets URL unescaped
				CString sValue = p.GetString(it->first);
				
				//EAD 4.0.0.395 seems to add double backslashes so take care of that
				sValue.Replace(_T("\\\\"), _T("\\"));
				
				CString sPart = sValue.Left( sBaseDir.GetLength() );

				if ( sBaseDir.CompareNoCase(sPart) == 0 )
				{
					CString sNewVal = sValue.Mid(sBaseDir.GetLength());
					p.SetString(it->first, sNewVal);
					Output( _T("Changed %s to %s (was %s)"), (LPCTSTR) it->first, sNewVal, (LPCTSTR) it->second );
					bModified = true;
				}
			}

			if ( bModified )
			{
				result = p.GetPackedParameterString(sContent);
				
				if ( result )
				{
					result = SaveCStringToFile(szFileName, sContent);
				}
			}
			else
			{
				result = true;
			}
		}
	}

	return result;
}

static bool RelativizeUserManifestPaths(LPCTSTR szUserCacheDir, LPCTSTR szOriginalPath)
{
	bool result = true;

	CString sUserCacheDir = szUserCacheDir;
	if ( sUserCacheDir.Right(1) != _T("\\") )
	{
		sUserCacheDir += _T("\\");
	}

	//If no original path was provided then use the cache path
	CString sOriginalPath = szOriginalPath ? szOriginalPath : _T("");

	if ( sOriginalPath.IsEmpty() )
		sOriginalPath = sUserCacheDir;

	if ( sOriginalPath.Right(1) != _T("\\") )
	{
		sOriginalPath += _T("\\");
	}

	CString sJobsManifestFile = sUserCacheDir + _T("eal_dm_jobs.eal");

	if ( ::PathFileExists(sJobsManifestFile) )
	{
		result &= RelativizeManifestContent( sJobsManifestFile, sOriginalPath );
	}

	WIN32_FIND_DATA	fd;
	ZeroMemory(&fd, sizeof(fd));

	HANDLE hFind = FindFirstFile(sUserCacheDir + _T("*.mfst"), &fd);

	while ( hFind != INVALID_HANDLE_VALUE )
	{
		if ( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
		{
			result &= RelativizeManifestContent(sUserCacheDir + fd.cFileName, sOriginalPath);
		}

		if ( !FindNextFile(hFind, &fd) )
		{
			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
	}

	return result;
}

extern "C" BOOL PASCAL EXPORT RelativizeManifestPaths(LPCTSTR szCacheDir, LPCTSTR szOriginalPath)
{
	bool result = true;

	WIN32_FIND_DATA	fd;
	ZeroMemory(&fd, sizeof(fd));

	CString sCacheDir = szCacheDir;
	if ( sCacheDir.Right(1) != _T("\\") )
	{
		sCacheDir += _T("\\");
	}

	//If no original path was provided then use the cache path
	CString sOriginalPath = szOriginalPath ? szOriginalPath : _T("");

	if ( sOriginalPath.IsEmpty() )
		sOriginalPath = sCacheDir;

	if ( sOriginalPath.Right(1) != _T("\\") )
	{
		sOriginalPath += _T("\\");
	}

	HANDLE hFind = FindFirstFile(sCacheDir + _T("{ * }"), &fd);

	while ( hFind != INVALID_HANDLE_VALUE )
	{
		if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			result &= RelativizeUserManifestPaths(sCacheDir + fd.cFileName, sOriginalPath + fd.cFileName);
		}

		if ( !FindNextFile(hFind, &fd) )
		{
			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
	}

	return result ? TRUE : FALSE;
}



static bool RelativizeManifestContent8(LPCTSTR szFileName, LPCTSTR szBaseDir, LPCTSTR szOrigBaseDir)
{
	bool result = false;

	CString sContent;
	CString sKey;
	CString sValue;
	CString sBaseDir = szBaseDir;
	CString sOrigBaseDir = szOrigBaseDir;

	LoadCStringFromFile(szFileName, sContent);

	Output(_T("Fixing manifest paths for %s"), szFileName);

	if ( !sContent.IsEmpty() )
	{
		Core::Parameterizer param;
		bool bModified = false;
		
		if ( param.Parse(sContent) )
		{
			long nNumJobs = param.GetLong(L"numjobs");

			for (long nCount = 0; nCount < nNumJobs; nCount++)
			{
				// Type
				sKey.Format(L"jobType%ld", nCount);
				unsigned long nJobType = param.GetLong(sKey);
				ASSERT(nJobType != 0);
				if (!nJobType || nJobType == 4 /*DIP*/) continue;

				// JobManifestName
				sKey.Format(L"jobManifest%ld", nCount);
				CString sJobManifestName = param.GetString(sKey);
				ASSERT(!sJobManifestName.IsEmpty());
				if (sJobManifestName.IsEmpty()) continue;

				sJobManifestName.Replace(_T("\\\\"), _T("\\"));
				
				// does it contain our old cache path
				CString sPart = sJobManifestName.Left( sOrigBaseDir.GetLength() );

				if ( sOrigBaseDir.CompareNoCase(sPart) == 0 )
				{
					CString sNewVal = sJobManifestName;
					sNewVal.Replace(sPart, sBaseDir);
					param.SetString(sKey, sNewVal);
					Output( _T("changed %s to %s"), (LPCTSTR) sJobManifestName, sNewVal);
					bModified = true;
				}

			
			}
		}
	
		if ( bModified )
		{
			result = param.GetPackedParameterString(sContent);
				
			if ( result )
			{
				result = SaveCStringToFile(szFileName, sContent);
			}
		}
		else
		{
			result = true;
		}
	}

	return result;
}

static bool RelativizeUserManifestPaths8(LPCTSTR szUserCacheDir, LPCTSTR szOriginalPath)
{
	bool result = true;

	CString sUserCacheDir = szUserCacheDir;
	if ( sUserCacheDir.Right(1) != _T("\\") )
	{
		sUserCacheDir += _T("\\");
	}

	//If no original path was provided then use the cache path
	CString sOriginalPath = szOriginalPath ? szOriginalPath : _T("");

	if ( sOriginalPath.IsEmpty() )
		sOriginalPath = sUserCacheDir;

	if ( sOriginalPath.Right(1) != _T("\\") )
	{
		sOriginalPath += _T("\\");
	}

	CString sJobsManifestFile = sUserCacheDir + _T("eal_dm_jobs.eal");

	if ( ::PathFileExists(sJobsManifestFile) )
	{
		result &= RelativizeManifestContent8( sJobsManifestFile, sOriginalPath, sUserCacheDir);
	}

	WIN32_FIND_DATA	fd;
	ZeroMemory(&fd, sizeof(fd));

	HANDLE hFind = FindFirstFile(sUserCacheDir + _T("*.mfst"), &fd);

	while ( hFind != INVALID_HANDLE_VALUE )
	{
		if ( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
		{
			result &= RelativizeManifestContent8(sUserCacheDir + fd.cFileName, sOriginalPath, sUserCacheDir);
		}

		if ( !FindNextFile(hFind, &fd) )
		{
			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
	}

	return result;
}

extern "C" BOOL PASCAL EXPORT RelativizeManifestPaths8(LPCTSTR szCacheDir, LPCTSTR szOriginalPath)
{
	bool result = true;

	WIN32_FIND_DATA	fd;
	ZeroMemory(&fd, sizeof(fd));

	CString sCacheDir = szCacheDir;
	if ( sCacheDir.Right(1) != _T("\\") )
	{
		sCacheDir += _T("\\");
	}

	//If no original path was provided then use the cache path
	CString sOriginalPath = szOriginalPath ? szOriginalPath : _T("");

	if ( sOriginalPath.IsEmpty() )
		sOriginalPath = sCacheDir;

	if ( sOriginalPath.Right(1) != _T("\\") )
	{
		sOriginalPath += _T("\\");
	}

	HANDLE hFind = FindFirstFile(sCacheDir + _T("{ * }"), &fd);

	while ( hFind != INVALID_HANDLE_VALUE )
	{
		if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			result &= RelativizeUserManifestPaths8(sCacheDir + fd.cFileName, sOriginalPath + fd.cFileName);
		}

		if ( !FindNextFile(hFind, &fd) )
		{
			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
	}

	return result ? TRUE : FALSE;
}

static bool ConvertWideCharToMCBS(LPCWSTR WideString, char *sBuffer, size_t nBuffSize)
{
	sBuffer[0] = NULL;

	if ( wcslen(WideString) > 0 )
	{
		/*
		char *buffer = NULL;
		int size = WideCharToMultiByte(	CP_UTF8,        // code page = ASCII
			0,				// performance and mapping flags
			WideString,		// wide-character string
			-1,				// auto terminate the string
			buffer,			// buffer for new string
			0,				// size of buffer
			NULL,			// default for unmappable chars
			NULL			// set when default char used
			);
		*/

		// Allocate space for our buffer
		int mbSize = WideCharToMultiByte(CP_UTF8, 0, WideString, -1, sBuffer, (int)nBuffSize, NULL, NULL);

		if (mbSize == 0)												
		{
			switch (GetLastError())
			{
			case ERROR_INSUFFICIENT_BUFFER:
				// DRMLogger(_T("ConvertWideCharToMCBS: Buffer of %d is too small for string.\n"), size);
				break;

			case ERROR_INVALID_FLAGS:
				// DRMLogger(_T("ConvertWideCharToMCBS: Flags are invalid.\n"));
				break;

			case ERROR_INVALID_PARAMETER:
				// DRMLogger(_T("ConvertWideCharToMCBS: A Parameter is invalid.\n"));
				break;

			default:
				// DRMLogger(_T("ConvertWideCharToMCBS: Unknown error %d\n"), GetLastError());
				break;			
			}

		}

	}	// If there is something to translate

	return sBuffer[0] != NULL;		

} // ConvertWideCharToMCBS

static bool LoadCStringFromFile(const CString& sFilename, CString& string)
{
	CFile file;
	CFileException err;
	if (!::PathFileExists(sFilename)) return false;

	if( !file.Open(sFilename, CFile::modeRead | CFile::shareDenyRead, &err) )
	{
		TCHAR   szErrorMessage[512];
		err.GetErrorMessage(szErrorMessage, 512);
		Output(_T("LoadCStringFromFile(%s) failed\n%s"), (LPCTSTR) sFilename, szErrorMessage);
		return false;
	}

	// Read the file into a buffer
	unsigned long nFileSize = (unsigned long) file.GetLength();
	char* pBuf = new char[nFileSize];
	file.Read(pBuf, nFileSize);

	// If we've read in a Unicode file, set the string (after the signature)
	if (IsTextUnicode(pBuf, nFileSize, NULL))
	{
		unsigned long nStringLength = (nFileSize-2) / sizeof(TCHAR);	// minus the signature size
		string.SetString(((TCHAR*) pBuf)+1, nStringLength);
	}
	else
	{
		// Otherwise do a Multibyte->Unicode conversion
		unsigned long nStringLength = 0;	// calculate stringlength

		TCHAR* pUnicodeBuf = new TCHAR[nFileSize];
		nStringLength = MultiByteToWideChar(CP_UTF8, 0, pBuf, nFileSize, pUnicodeBuf, nFileSize);
		string.SetString(pUnicodeBuf, nStringLength);
		delete[] pUnicodeBuf;
	}

	delete[] pBuf;
	file.Close();

	return true;
}

static bool SaveCStringToFile(const CString& sFilename, CString& string)
{
	CFile file;
	CFileException err;
	if (!::PathFileExists(sFilename)) return false;

	if( !file.Open(sFilename, CFile::modeWrite | CFile::modeCreate, &err) )
	{
		TCHAR szErrorMessage[512];
		err.GetErrorMessage(szErrorMessage, 512);
		Output(_T("SaveCStringToFile(%s) failed\n%s"), (LPCTSTR) sFilename, szErrorMessage);
		return false;
	}

	// Unicode UTF-16LittleEndian signature
	if (sizeof(TCHAR) == 2)
	{
		char sig[2];
		sig[0] = (byte) 0xFF;
		sig[1] = (byte) 0xFE;	
		file.Write(sig, 2);
		file.Write(string, string.GetLength() * 2);
	}
	else
	{
		file.Write(string, string.GetLength());
	}
	
	file.Close();

	return true;
}

static CString ReadRegString( HKEY hRoot, LPCTSTR lpSubKey, LPCTSTR lpValueName )
{
	CString sResult;
	HKEY hKey = NULL;
	if ( RegOpenKey(hRoot, lpSubKey, &hKey) == ERROR_SUCCESS )
	{
		DWORD cbData = 0;
		DWORD keytype =  REG_SZ;

		if ( RegQueryValueEx(hKey, lpValueName, NULL, &keytype, NULL, &cbData) == ERROR_SUCCESS ) 
		{
			if ( RegQueryValueEx(hKey, lpValueName, NULL, &keytype, (LPBYTE) sResult.GetBufferSetLength(cbData+1), &cbData) == ERROR_SUCCESS )
			{
				sResult.ReleaseBuffer();
			}
			else
			{
				sResult.ReleaseBuffer(-1);
			}
		}

		RegCloseKey(hKey);
	}

	return sResult;
}

static bool WriteIniString( LPCTSTR lpIniFile, LPCTSTR lpSection, LPCTSTR lpKey, LPCTSTR lpEntry )
{
	return WritePrivateProfileString(lpSection, lpKey, lpEntry, lpIniFile) == TRUE;
}

static CString ReadIniString( LPCTSTR lpIniFile, LPCTSTR lpSection, LPCTSTR lpEntry )
{
	CString sValue;
	GetPrivateProfileString(lpSection, lpEntry, NULL, sValue.GetBufferSetLength(1024), 1024, lpIniFile );
	sValue.ReleaseBuffer();

	return sValue;
}

static int FindNoCase(const tstring& strString, const tstring& strLookFor, int nStartAt)
{
	CString strLower = strString.c_str();
	strLower.MakeLower();

	CString strLookForLower = strLookFor.c_str();
	strLookForLower.MakeLower();

	return strLower.Find(strLookForLower, nStartAt);
}

static int FindNoCase(const CString& strString, const CString& strLookFor, int nStartAt)
{
	CString strLower = strString;
	strLower.MakeLower();

	CString strLookForLower = strLookFor;
	strLookForLower.MakeLower();

	return strLower.Find(strLookForLower, nStartAt);
}

static void Output( LPCTSTR szFormat, ... )
{
#ifdef _DEBUG
	va_list argptr;
	va_start(argptr, szFormat);

	TCHAR szBuffer[1024];
	_vsntprintf_s( szBuffer, _countof(szBuffer) - 1, szFormat, argptr );
	wcscat_s(szBuffer, _T("\n"));

	OutputDebugString( szBuffer );

	va_end(argptr);
#endif
}

static bool GetParentInfo(DWORD pid, PROCESSENTRY32* pPe)
{
	bool result = false;
	
	if ( pPe )
	{
		ZeroMemory(pPe, sizeof(PROCESSENTRY32));
		pPe->dwSize = sizeof(PROCESSENTRY32);
	}

	if ( pid == 0 )
		pid = GetCurrentProcessId();

	HANDLE	hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

	if ( hSnapshot != INVALID_HANDLE_VALUE )
	{
		PROCESSENTRY32	pe;
		pe.dwSize = sizeof(pe);

		if ( Process32First( hSnapshot, &pe ) )
		{
			DWORD parentPID = 0;
			do
			{
				if ( parentPID != 0 )
				{
					if ( pe.th32ProcessID == parentPID )
					{
						//Got our parent information!
						if ( pPe )
						{
							memcpy( pPe, &pe, sizeof(pe) );
							result = true;
							break;
						}
					}
				}
				else if ( pe.th32ProcessID == pid )
				{
					//Got it! Save parent PID and restart
					parentPID = pe.th32ParentProcessID;
					
					if ( !Process32First( hSnapshot, &pe ) )
						break;

					continue;
				}

				ZeroMemory(&pe, sizeof(pe));
				pe.dwSize = sizeof(pe);

			} while ( Process32Next( hSnapshot, &pe ) );
		}

		CloseHandle( hSnapshot );
	}
	else
	{
		Output(_T("CreateToolhelp32Snapshot failed. Last error: %u"), GetLastError());
	}

	return result;
}

static const LPCTSTR LanguageCodes[] = 
{
	L"en_US",	// 0	English (US)
	L"en_GB",	// 1	English (Great Britain)
	L"cs_CZ",	// 2	Czech
	L"da_DK",	// 3	Danish
	L"de_DE",	// 4	German
	L"el_GR",	// 5	Greek
	L"es_ES",	// 6	Spanish (Spain)
	L"es_MX",	// 7	Spanish (Mexico)
	L"fi_FI",	// 8	Finnish
	L"fr_FR",	// 9	French
	L"hu_HU",	// 10	Hungarian
	L"it_IT",	// 11	Italian
	L"ja_JP",	// 12	Japanese
	L"ko_KR",	// 13	Korean
	L"nl_NL",	// 14	Dutch
	L"no_NO",	// 15	Norwegian
	L"pl_PL",	// 16	Polish
	L"pt_BR",	// 17	Portuguese (Brazil)
	L"pt_PT",	// 18	Portuguese (Portugal)
	L"sv_SE",	// 19	Swedish
	L"zh_CN",	// 20	Chinese (China)
	L"zh_TW",	// 21	Chinese (Taiwan)
	L"th_TH",	// 22	Thai
	L"ru_RU"	// 23	Russian
};


static CString GetISOLocale(const CString& strLocale)
{
	CString strResult;

	if ( strLocale.GetLength() == 5 )
	{
		//Verify it is a supported language
		for ( int i = 0; i < _countof(LanguageCodes); i++ )
		{
			if ( strLocale.CompareNoCase(LanguageCodes[i]) == 0 )
			{
				strResult = LanguageCodes[i]; //Proper case
				break;
			}
		}
	}
	else if ( strLocale.GetLength() == 2 )
	{
		//Verify it is a supported language
		for ( int i = 0; i < _countof(LanguageCodes); i++ )
		{
			CString smallCode = LanguageCodes[i];
			smallCode = smallCode.Left(2);

			if ( strLocale.CompareNoCase(smallCode) == 0 )
			{
				strResult = LanguageCodes[i]; //Full ISO
				break;
			}
		}
	}

	return strResult;
}


#ifdef DLL_DELETES
bool DeleteFolderIfPresent( const CString sFolderName, bool bSafety /* = false */, bool bIncludingRoot /* = false */, void *pProgressBar /* = NULL */, void (*pfcnCallback)(void*) /* = NULL */ )
{
	bool bRetVal = true;

	// Make working copy of folder name
	CString sWorkingFolderName = sFolderName;

	// Path and search construction constants.
	const CString ALL_FILES_AND_FOLDERS  = L"*.*"; // An implementation of search criteria.
	const char    SEPARATOR              = '\\';
	const CString THIS_DIRECTORY         = L".";
	const CString PARENT_DIRECTORY       = L"..";
	const CString UNICODE_PREFIX         = L"\\\\?\\";
	static const size_t NPOS             = -1;    // NULL position constant for any index of a basic_string derivative.

	// Debug / Exception constants.
	const CString ID                               = L"EnvUtils::DeleteFolderIfPresent ERROR: ";
	const CString ERROR_PATH_DOES_NOT_EXIST		   = L"Path does not exist: ";
	const CString ERROR_PATH_EMPTY                 = L"Path is empty.";
	const CString ERROR_PATH_NOT_SAFE              = L"Path is out of safety range: ";
	const CString ERROR_CANNOT_MAKE_PATH_DELETABLE = L"Cannot make path deletable: ";
	const CString ERROR_CANNOT_DELETE_FILE         = L"Cannot delete file: ";
	const CString ERROR_CANNOT_DELETE_DIRECTORY    = L"Cannot delete directory: ";

	// Locals.
	WIN32_FIND_DATAW  oFindData;					 // Data structure containing file / folder data.
	HANDLE            hSearch;						 // File system search handle.
	CString			  sCriteria;					 // Criteria used to search for files / folder.
	CString           sName;                         // The name of a file system node.
	CString           sCurrentPath;					 // 'TargetPath' + 'name'
	CString           sDebugMsg;					 // Debug Message

	// Only process path's with values.
	if ( !sWorkingFolderName.IsEmpty() )
	{   
		int               nFirstSeparatorIndex;			 // Index of the first SEPARATOR in TargetPath.
		int               nLastSeparatorIndex;			 // Index of the last SEPARATOR in TargetPath.
		int               nSeparatorStartIndex = 0;		 // Index of where to begin scanning for SEPARATORs in TargetPath.
		int               nFirstLastSeparatorDifference; // nLastSeparator - nFirstSeparator
		bool              bSafetyCheckPassed = true;	 // Defaulted to true.
		
		// Grab the difference in position between the first and last path separation characters taking into account any existing UNICODE_PREFIX.
		if ( sWorkingFolderName.Compare( UNICODE_PREFIX ) == 0 ) { nSeparatorStartIndex = UNICODE_PREFIX.GetLength(); }
		nFirstSeparatorIndex = sWorkingFolderName.Find( SEPARATOR, nSeparatorStartIndex );
		nLastSeparatorIndex  = sWorkingFolderName.ReverseFind( SEPARATOR );
		nFirstLastSeparatorDifference = ( ( nLastSeparatorIndex  != NPOS ) ? nLastSeparatorIndex  : 0 ) - 
			( ( nFirstSeparatorIndex != NPOS ) ? nFirstSeparatorIndex : 0 );

		// Safety check. This operates by examining the distance between the first and last non-UNICODE_PREFIX separators. If there is not at least
		// one character in between then we know the path is a root (ex. c:\\, d:\\, \\, etc.)
		if ( bSafety && ( nFirstLastSeparatorDifference <= 1 ) )
		{ 
			bRetVal = false;
			bSafetyCheckPassed = false; // Flag a failure in the safety check.

#ifdef _DEBUG
			sDebugMsg = ID + ERROR_PATH_NOT_SAFE + sWorkingFolderName + L"\r\n";
			::OutputDebugString( sDebugMsg );
#endif
		}  // if ( Safety...

		// Did the safety check pass?
		if ( bSafetyCheckPassed )
		{
			bool bSearchResultsExist;			 // Do file search results exist?
		
			// Trim an existing trailing backslash.
			if ( ( sWorkingFolderName.GetAt(sWorkingFolderName.GetLength() - 1) == SEPARATOR ) && ( nFirstLastSeparatorDifference > 1 ) )
				sWorkingFolderName.Delete( sWorkingFolderName.GetLength() - 1, sWorkingFolderName.GetLength() ); 

			// Pre-pend the Unicode prefix if it's not there.
			//                  if ( sWorkingFolderName.Compare( UNICODE_PREFIX ) != 0 )
			//					  sWorkingFolderName.Insert( 0, UNICODE_PREFIX );
			// Setup the criteria and perform a search for the first file system node.
			sCriteria           = sWorkingFolderName + SEPARATOR + ALL_FILES_AND_FOLDERS;
			hSearch             = ::FindFirstFile( sCriteria, &oFindData ); // find the first file
			bSearchResultsExist = ( hSearch != INVALID_HANDLE_VALUE );

			// Do search results exist?
			if ( bSearchResultsExist )
			{
				// Enumerate the search results.
				while ( bSearchResultsExist )
				{	
					bool              bIsFile;						 // Is the current file system node a file?
					bool              bIsSubDirectory;				 // Is the current file system node a child folder?
					bool              bSearchResultsExist;			 // Do file search results exist?
					
					// Build the current path.
					sName        = oFindData.cFileName; 
					sCurrentPath = sWorkingFolderName + SEPARATOR + sName; 

					// Determine if the file system node is a file or child folder ("." and ".." folders are ignored).
					bIsFile                  = !( oFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ? true : false;
					bIsSubDirectory    = ( !bIsFile && ( ( sName != THIS_DIRECTORY ) && ( sName != PARENT_DIRECTORY ) ) ) ? true : false; 

					// If we have a file or child folder then process it.
					if ( bIsFile || bIsSubDirectory ) 
					{
						// Make it deletable.
						if ( ::SetFileAttributesW( sCurrentPath, FILE_ATTRIBUTE_NORMAL ) ) 
						{
							// Are we dealing with a file?
							if ( bIsFile ) 
							{ 
								// Yes, delete it.
								if ( !::DeleteFile( sCurrentPath ) )      
								{
									bRetVal = false;
#ifdef _DEBUG
									sDebugMsg = ID + ERROR_CANNOT_DELETE_FILE + sCurrentPath + L"\r\n";
									::OutputDebugString( sDebugMsg );
#endif
								} // if ( !::DeleteFileW...
								else
								{
									// If a progress bar and a progress bar callback was passed in, call it
									if (pProgressBar && pfcnCallback)
										pfcnCallback(pProgressBar);
								}
							} 
							else // It's a isSubDirectory
							{ 
								DeleteFolderIfPresent( sCurrentPath, false, false, pProgressBar, pfcnCallback ); // Recurse.
								// Delete the subdirectory.
								if ( !::RemoveDirectory( sCurrentPath ) ) 
								{     
									bRetVal = false;
#ifdef _DEBUG
									sDebugMsg = ID + ERROR_CANNOT_DELETE_DIRECTORY + sCurrentPath + L"\r\n";
									::OutputDebugString( sDebugMsg );
#endif
								} // if ( !::RemoveDirectoryW...
							} // if ( isFile )
						}
						else
						{
							bRetVal = false;
#ifdef _DEBUG
							sDebugMsg = ID + ERROR_CANNOT_MAKE_PATH_DELETABLE + sCurrentPath + L"\r\n";
							::OutputDebugString( sDebugMsg );
#endif
						} // if ( ::SetFileAttributesW...
					} // if  ( isFile || isSubDirectory...
					bSearchResultsExist = ::FindNextFile( hSearch, &oFindData ) ? true : false;
				} // while ( searchResultsExist )...

				if ( hSearch != INVALID_HANDLE_VALUE ) ::FindClose ( hSearch ); // Cleanup 

				// Delete the root?
				if ( bIncludingRoot )
				{
					if ( ::SetFileAttributes( sWorkingFolderName, FILE_ATTRIBUTE_NORMAL ) )
					{
						// Delete the root directory.
						if ( !::RemoveDirectory( sWorkingFolderName ) ) 
						{     
							bRetVal = false;

#ifdef _DEBUG
							sDebugMsg = ID + ERROR_CANNOT_DELETE_DIRECTORY + sWorkingFolderName + L"\r\n";
							::OutputDebugString( sDebugMsg );
#endif
						} // if ( !::RemoveDirectoryW...
					}
					else
					{
						bRetVal = false;
#ifdef _DEBUG
						sDebugMsg = ID + ERROR_CANNOT_MAKE_PATH_DELETABLE + sWorkingFolderName + L"\r\n";
						::OutputDebugString( sDebugMsg );
#endif
					} // if ( ::SetFileAttributesW...
				} // if ( IncludingRoot )...
			}
			else
			{
				bRetVal = false;

#ifdef _DEBUG
				sDebugMsg = ID + ERROR_PATH_DOES_NOT_EXIST + sWorkingFolderName + L"\r\n";
				::OutputDebugString( sDebugMsg );
#endif
			} // if ( searchResultsExist )
		} // if ( safetyCheckPassed )
	}
	else
	{
		bRetVal = false;
#ifdef _DEBUG
		sDebugMsg = ID + ERROR_PATH_EMPTY + L"\r\n";
		::OutputDebugString( sDebugMsg );
#endif
	} // if ( !sWorkingFolderName.Empty() )
	return bRetVal;
}
#endif

bool GiveDirectoryFullAccessToGroup(LPCTSTR lpPath, LPTSTR lpGroup)
{
	HANDLE hDir = CreateFile(lpPath,READ_CONTROL|WRITE_DAC,0,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
	if(hDir == INVALID_HANDLE_VALUE) return false;

	ACL* pOldDACL=NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	GetSecurityInfo(hDir,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,&pOldDACL,NULL,&pSD);

	EXPLICIT_ACCESS ea={0};
	ea.grfAccessMode = GRANT_ACCESS;
	ea.grfAccessPermissions = GENERIC_ALL;
	ea.grfInheritance = CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;  	

	PSID newSID;

	if(ConvertStringSidToSid(lpGroup, &newSID))
	{
		ea.Trustee.ptstrName = (LPTSTR)newSID;

		ACL* pNewDACL = NULL;
		SetEntriesInAcl(1,&ea,pOldDACL,&pNewDACL);

		SetSecurityInfo(hDir,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,pNewDACL,NULL);

		LocalFree(pSD);
		LocalFree(pNewDACL);
		CloseHandle(hDir);

		return true;
	}

	return false;
}


extern "C" void PASCAL EXPORT GrantEveryoneAccessToFile(wchar_t* sFileName)
{
	if(!GiveDirectoryFullAccessToGroup(sFileName, L"S-1-5-32-545")) return; // Users
	if(!GiveDirectoryFullAccessToGroup(sFileName, L"S-1-1-0")) return; // Everyone
}

extern "C" void PASCAL EXPORT CleanUserConfigDir()
{
	/*MessageBox(NULL,L"CleanUserConfigDir",L"CleanUserConfigDir",0);*/
	CleanUserDataFolders();
}


extern "C" BOOL PASCAL EXPORT GrabDBGPrivilege()
{
	TOKEN_PRIVILEGES Priv, PrivOld;
	DWORD cbPriv = sizeof(PrivOld);
	HANDLE hToken;

	// get current thread token
	if (!OpenThreadToken(GetCurrentThread(),
		TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,
		FALSE, &hToken))
	{
		if (GetLastError() != ERROR_NO_TOKEN)
			return FALSE;
		// revert to the process token, if not impersonating
		if (!OpenProcessToken(GetCurrentProcess(),
			TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,
			&hToken))
			return FALSE;
	}

	Priv.PrivilegeCount                        = 1;
	Priv.Privileges[0].Attributes      = SE_PRIVILEGE_ENABLED;
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Priv.Privileges[0].Luid);

	// try to enable the privilege
	if (!AdjustTokenPrivileges(hToken,
		FALSE,
		&Priv,
		sizeof(Priv),
		&PrivOld,
		&cbPriv))
	{
		
		DWORD dwError = GetLastError();
		CloseHandle(hToken);
		return SetLastError(dwError), FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		// the SE_DEBUG_NAME privilege is not in the caller's token
		CloseHandle(hToken);
		return SetLastError(ERROR_ACCESS_DENIED), FALSE;
	}

	return TRUE;
}


extern "C" BOOL PASCAL EXPORT IsUpdate()
{
	PROCESSENTRY32 pe={0};
	pe.dwSize = sizeof(PROCESSENTRY32);
	
	if(GetParentInfo(GetCurrentProcessId(), &pe))
	{
		if(lstrcmpi(pe.szExeFile, L"EAProxyInstaller.exe")==0)	// EADM updates are installed via EAProxyInstaller.exe
			return true;

		if(lstrcmpi(pe.szExeFile, L"EADMClientService.exe")==0)	// EADM updates are installed via EADMClientService.exe
			return true;

		if(lstrcmpi(pe.szExeFile, L"OriginClientService.exe")==0)	// EADM updates are installed via EADMClientService.exe
			return true;
	}

	return false;
}




BOOL SearchIGODlls( DWORD dwPID )
{
	HANDLE hModuleSnap;
	BOOL found = FALSE;
	MODULEENTRY32W me32 = {0};

	// Take a snapshot of all modules in the specified process.
	hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID );
	if( hModuleSnap == INVALID_HANDLE_VALUE )
	{
		return found;
	}

	// Set the size of the structure before using it.
	me32.dwSize = sizeof( MODULEENTRY32W );

	// Retrieve information about the first module,
	// and exit if unsuccessful
	if( !Module32FirstW( hModuleSnap, &me32 ) )
	{
		CloseHandle( hModuleSnap );           // clean the snapshot object
		return found;
	}

	// Now walk the module list of the process,
	// and display information about each module
	do
	{
		if( _wcsicmp(  me32.szModule, L"igo32.dll") == 0 ||  _wcsicmp(  me32.szModule, L"igo32d.dll") == 0 )
		{
			// TODO add module path verifiction? me32.szExePath
			found = TRUE;
			break;
		}
	} while( Module32NextW( hModuleSnap, &me32 ) );

	CloseHandle( hModuleSnap );
	return found;
}

extern "C" BOOL PASCAL EXPORT GetLockingProcessList(wchar_t *processList, DWORD numberOfElements)
{
	HANDLE hProcessSnap = NULL;
	PROCESSENTRY32W pe32 = {0};
	BOOL elemetsAdded = FALSE;

	memset(processList, 0, numberOfElements * sizeof(wchar_t));
	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hProcessSnap == INVALID_HANDLE_VALUE )
	{
		return FALSE;
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof( PROCESSENTRY32W );

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if( !Process32FirstW( hProcessSnap, &pe32 ) )
	{
		CloseHandle( hProcessSnap );          // clean the snapshot object
		return FALSE;
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
		// looks for igo32.dll / igo32d.dll
		if( SearchIGODlls( pe32.th32ProcessID ) )
		{
			// add process to list
			if( elemetsAdded )
				wcscat_s(processList, numberOfElements, L", ");
			else
				elemetsAdded = TRUE;

			wcscat_s(processList, numberOfElements, pe32.szExeFile);
		}

	} while( Process32NextW( hProcessSnap, &pe32 ) );

	CloseHandle( hProcessSnap );

	return elemetsAdded;
}

extern "C" void PASCAL EXPORT RunProcess(wchar_t *fullPath, wchar_t *currentDir, bool wait, wchar_t *cmdLine)
{
	if(!fullPath || !currentDir)
		return;

	SHELLEXECUTEINFO shellExecuteInfo;
	memset( &shellExecuteInfo, 0, sizeof( shellExecuteInfo ) );
	shellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
	shellExecuteInfo.lpFile = fullPath;
	if (cmdLine!=NULL)
		shellExecuteInfo.lpParameters = cmdLine;
	
	shellExecuteInfo.lpVerb = L"open";
	shellExecuteInfo.nShow = SW_SHOWNORMAL;
	shellExecuteInfo.cbSize = sizeof( shellExecuteInfo );
	shellExecuteInfo.lpDirectory = currentDir;
	if ( ShellExecuteEx( &shellExecuteInfo ) == FALSE )
	{
		//error
	}
	else
	{
		if ( wait==true && shellExecuteInfo.hProcess )
		{
			WaitForSingleObject( shellExecuteInfo.hProcess, INFINITE ); // Wait or timeout
			CloseHandle( shellExecuteInfo.hProcess );	// Cleanup.
		}
	}
}

bool MoveFolder(tstring srcPath, tstring destPath)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	tstring szTmp;
  
	tstring szSrcPath = srcPath;
	tstring szDesPath = destPath;
  

	// add trailing separators
	if (!szSrcPath.empty() && szSrcPath[szSrcPath.length()-1] != L'\\')
		szSrcPath += L"\\";
	
	if (!szDesPath.empty() && szDesPath[szDesPath.length()-1] != L'\\')
		szDesPath += L"\\";
 
	szTmp = szSrcPath;
	szTmp += L"*";

	tstring szNewSrcPath;
	tstring szNewDesPath;

	hFind = FindFirstFile(szTmp.c_str(), &FindFileData);
	if(hFind == INVALID_HANDLE_VALUE) return FALSE;

	do
	{
		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if(wcscmp(FindFileData.cFileName, L"."))
			{
				if(wcscmp(FindFileData.cFileName, L".."))
				{
				szNewDesPath = szDesPath + FindFileData.cFileName + L"\\";
				szNewSrcPath = szSrcPath + FindFileData.cFileName + L"\\";

				SHCreateDirectoryExW(NULL, szNewDesPath.c_str(), NULL);
				MoveFolder(szNewSrcPath, szNewDesPath);
				}
			}
		}
		else
		{
			tstring szSrcFile;
			tstring szDestFile;
		
			szDestFile = szDesPath + FindFileData.cFileName;
			szSrcFile = szSrcPath + FindFileData.cFileName;
		
			MoveFileExW((const wchar_t*)szSrcFile.c_str(), (const wchar_t*)szDestFile.c_str(), MOVEFILE_COPY_ALLOWED);
		}
	}
	while(FindNextFile(hFind, &FindFileData));

	FindClose(hFind);

	// remove old folder, if it is empty
	//if(!EA::IO::Directory::Exists(srcPath.c_str()) || (EA::IO::Directory::IsDirectoryEmpty(srcPath.c_str(), EA::IO::kDirectoryEntryFile, true) && EA::IO::Directory::IsDirectoryEmpty(srcPath.c_str(), EA::IO::kDirectoryEntryDirectory, false)))
	//	EA::IO::Directory::Remove(srcPath.c_str());

	return true;
}


extern "C" BOOL PASCAL EXPORT MigrateCacheAndSettings()
{
	//migrate settings + cache + data from EADM 7.x and 8.0.x.x to Origin

	const wchar_t* kDownloadInPlaceDir = L"\\Origin Games";

	const wchar_t* kApplicationFolder = L"\\Electronic Arts\\EADM";
	const wchar_t* kNewApplicationFolder = L"\\Origin";

	int ret = 0;	// 0 -> okay
					// 1 - means do not remove the old program folder, because it is used
					// 2 - means do not remove the old data folder, because it is used
					// any other value means error

	wchar_t bufferApp[ MAX_PATH ] = {0};
	wchar_t bufferAllUserNew[ MAX_PATH ] =  {0};
	wchar_t bufferDIP[ MAX_PATH ] =  {0};
	tstring applicationDataFolder;
	tstring applicationDataFolderNew;
	tstring applicationInstallFolder;
	tstring cacheFolder;
	tstring dipFolder;
	tstring cacheFolderNew;
	tstring dipFolderNew;

	bool bEACoreSessionExists = false;

	DebugMessageBox(L"MigrateCacheAndSettings");

	SHGetFolderPathW( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, bufferApp );
	applicationDataFolder = bufferApp;
	applicationDataFolder += kApplicationFolder;

	applicationDataFolderNew = bufferApp;
	applicationDataFolderNew += kNewApplicationFolder;

	// search eacore_app.ini
	EA::IO::String16 eacoreIniName;
	CString sClientPath = ReadRegString(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Electronic Arts\\EA Core"), _T("ClientPath"));
	eacoreIniName.assign(sClientPath);

	EA::IO::String16::size_type pos = eacoreIniName.find_last_of(L"\\");
	if( pos != EA::IO::String16::npos )
	{
		if( pos+1 < eacoreIniName.length() )
		{
			wchar_t bufferAllUser[ MAX_PATH ] =  {0};
			
			eacoreIniName.replace( pos, eacoreIniName.length() - (pos), (L"\0") );
			applicationInstallFolder = eacoreIniName.c_str();
			eacoreIniName += L"\\EACore_App.ini";

			// search eacore_app.ini for SharedServerId
			EA::IO::IniFile eacoreAppIniFile((const char16_t*)eacoreIniName.c_str());

			EA::IO::String16 value;
			ret = eacoreAppIniFile.ReadEntry( (const char16_t*)L"EACore", (const char16_t*)L"SharedServerId", value );

			if( SHGetFolderPathW( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, bufferAllUser ) == S_OK )
			{
				const wchar_t* kCacheDirId = L"base_cache_dir";

				
				wcscpy_s(bufferAllUserNew, sizeof(bufferAllUserNew)/sizeof(bufferAllUserNew[0]), bufferAllUser);
				wcscat_s(bufferAllUser, sizeof(bufferAllUser)/sizeof(bufferAllUser[0]), L"\\Electronic Arts\\EA Core\\prefs\\");
				wcscat_s(bufferAllUser, sizeof(bufferAllUser)/sizeof(bufferAllUser[0]), value.c_str());
				wcscat_s(bufferAllUser, sizeof(bufferAllUser)/sizeof(bufferAllUser[0]), L".ini");

				wcscat_s(bufferAllUserNew, sizeof(bufferAllUserNew)/sizeof(bufferAllUserNew[0]), L"\\Electronic Arts\\EA Core\\prefs\\");
				wcscat_s(bufferAllUserNew, sizeof(bufferAllUserNew)/sizeof(bufferAllUserNew[0]), L"ORIGIN");
				wcscat_s(bufferAllUserNew, sizeof(bufferAllUserNew)/sizeof(bufferAllUserNew[0]), L".ini");

				EA::IO::IniFile appIniFile((const char16_t*)bufferAllUser);

				// extract DIP & download cache folders from EACORE ini file
				if (ret != -1)
				{
					const wchar_t* kDownloadInPlaceDirId = L"base_install_dir";
					ret = appIniFile.ReadEntry( (const char16_t*)L"EACore", (const char16_t*)kDownloadInPlaceDirId, value );
					if( ret != -1 )
					{
						dipFolder.assign(value.c_str());
					}
				}


				ret = appIniFile.ReadEntry( (const char16_t*)L"EACore", (const char16_t*)kCacheDirId, value );
				if( ret != -1 )
				{
					cacheFolder.assign(value.c_str());

					ret = appIniFile.ReadEntry( (const char16_t*)L"EACoreSession", (const char16_t*)L"full_base_cache_dir", value );
					if( ret != -1 )
					{
						bEACoreSessionExists = true;
						cacheFolder.assign(value.c_str());
					}
				}
			}


			eacoreAppIniFile.Close();
			// change agent from old value to "ORIGIN"
			WriteIniString( bufferAllUser, L"EACore", L"core.agents.3.path", L"\"Origin.exe\" /startDownload:{data1}" );

			// change EACORE prefs file from old name to to "ORIGIN.ini"
			EA::IO::File::Move(bufferAllUser, bufferAllUserNew);
		}
	}

	SHGetFolderPath( NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, bufferDIP );
	dipFolderNew = bufferDIP;
	dipFolderNew += kDownloadInPlaceDir;

	if( cacheFolder.empty() )
	{
		const wchar_t* kDownloadCacheDir = L"\\DownloadCache";
		cacheFolder = applicationDataFolderNew;
		cacheFolder += kDownloadCacheDir;
	}

	// remove trailing SEPARATORS
	if (!cacheFolder.empty() && cacheFolder[cacheFolder.length()-1] == L'\\')
		cacheFolder.resize(cacheFolder.length()-1);

	if (!dipFolder.empty() && dipFolder[dipFolder.length()-1] == L'\\')
		dipFolder.resize(dipFolder.length()-1);


	// migrate cache folder
	cacheFolderNew = cacheFolder;
	bool bMoveDataFolder = true;
	bool bMoveCacheFolder = false;
	bool bCopyDataFolder = false;
	bool bDIPNotUsed = false;

	// remove EA
	int sFound = FindNoCase(cacheFolderNew, L"\\Electronic Arts");
	if (sFound >=0)
	{
		cacheFolderNew.replace( sFound, wcslen(L"\\Electronic Arts"), L"" );
	}

	// replace EADM with Origin
	sFound = FindNoCase(cacheFolderNew, L"\\EADM");
	if (sFound >=0)
	{
		cacheFolderNew.replace( sFound, wcslen(L"\\EADM"), L"\\Origin" );
	}

	if(cacheFolderNew.compare(cacheFolder) != 0)
		bMoveCacheFolder = true;

	// if DIP folder does not exist or is empty, than we can get ride of it
	//if(dipFolder.empty() || !EA::IO::Directory::Exists(dipFolder.c_str()) || (EA::IO::Directory::IsDirectoryEmpty(dipFolder.c_str(), EA::IO::kDirectoryEntryFile, true) && EA::IO::Directory::IsDirectoryEmpty(dipFolder.c_str(), EA::IO::kDirectoryEntryDirectory, false)))
	{
		bDIPNotUsed = true;
		
		if(!dipFolder.empty())
			EA::IO::Directory::Remove(dipFolder.c_str());
	}
	// ***************************************************************************
	// check if the dip folder is inside our cache, data or application directory
	// ***************************************************************************

	if(!bDIPNotUsed)
	{
		sFound = FindNoCase(dipFolder, applicationDataFolder);	// dip is inside our old data folder
		if (sFound >=0)
		{
			// the old folder must stay, just move non DIP stuff out of it
			bMoveDataFolder = false;
			bCopyDataFolder = true;
		}
	
		sFound = FindNoCase(dipFolder, cacheFolder);	// dip is inside our old cache folder - do not move or change the cache folder!
		if (sFound >=0)
		{
			bMoveCacheFolder = false;
		}
	

		ret = 0; //reset this variable, it was used before....
		if (!applicationInstallFolder.empty())
		{
			sFound = FindNoCase(dipFolder, applicationInstallFolder);	// dip is inside our old application install folder
			if (sFound >=0)
			{
				ret = 1; // do not forcefully remove the old application folder
			}
		}

		if (!bMoveDataFolder)
		{
			// dip is inside our old data folder
			ret = 2; // do not remove the old data folder via the uninstaller
		}
	}

	// update EACore's ORIGIN.ini
	if (bMoveCacheFolder)
	{
		WriteIniString( bufferAllUserNew, L"EACore", L"base_cache_dir", cacheFolderNew.c_str() );
		if (bEACoreSessionExists)
			WriteIniString( bufferAllUserNew, L"EACoreSession", L"full_base_cache_dir", cacheFolderNew.c_str() );
	}
	
	if(bDIPNotUsed)
		WriteIniString( bufferAllUserNew, L"EACore", L"base_install_dir", dipFolderNew.c_str() );

	// migrate folders
	if (bMoveDataFolder)
	{
		RelativizeManifestPaths8(cacheFolder.c_str(), cacheFolderNew.c_str());
		if (!EA::IO::File::Move(applicationDataFolder.c_str(), applicationDataFolderNew.c_str()))
			MoveFolder(applicationDataFolder.c_str(), applicationDataFolderNew.c_str());
	}

	SHCreateDirectoryExW(NULL, applicationDataFolderNew.c_str(), NULL);
	GrantEveryoneAccessToFile((wchar_t*)applicationDataFolderNew.c_str());

	if (bMoveCacheFolder)
	{
		RelativizeManifestPaths8(cacheFolder.c_str(), cacheFolderNew.c_str());
		if (!EA::IO::File::Move(cacheFolder.c_str(), cacheFolderNew.c_str()))
			MoveFolder(cacheFolder, cacheFolderNew);
	}

	// NOTE: No longer migrating settings through the installer. The new settings file
	// uses QDomDocument and we don't want to link against Qt in the installer. Don't 
	// delete the old file either so the client can find it the first time it runs
	if (bCopyDataFolder || bMoveDataFolder/*move could fail if the Origin folder exists*/)
	{
		// move files & known folders
		EA::IO::File::Move((applicationDataFolder+L"\\AvatarsCache").c_str(), (applicationDataFolderNew+L"\\AvatarsCache").c_str());
		EA::IO::File::Move((applicationDataFolder+L"\\BoxArt").c_str(), (applicationDataFolderNew+L"\\BoxArt").c_str());
		EA::IO::File::Move((applicationDataFolder+L"\\logs").c_str(), (applicationDataFolderNew+L"\\logs").c_str());
		EA::IO::File::Move((applicationDataFolder+L"\\EADMUI_App.ini").c_str(), (applicationDataFolderNew+L"\\EADMUI_App.ini").c_str());
		EA::IO::File::Move((applicationDataFolder+L"\\EADMUI_log.log").c_str(), (applicationDataFolderNew+L"\\EADMUI_log.log").c_str());
		EA::IO::File::Move((applicationDataFolder+L"\\EADM_log.log").c_str(), (applicationDataFolderNew+L"\\EADM_log.log").c_str());
		EA::IO::File::Move((applicationDataFolder+L"\\Telemetry").c_str(), (applicationDataFolderNew+L"\\Telemetry").c_str());
		// rename ini

		/* Move the language settings from EADM to Origin, if necessary */
		tstring appIniPath(applicationDataFolderNew+L"\\Origin_App.ini");
		if (!EA::IO::File::Exists(appIniPath.c_str()))
		{
			CString oldLangvalue = ReadIniString((applicationDataFolderNew+L"\\EADMUI_App.ini").c_str(),L"EADMUI",L"Locale");
			EA::IO::File::Create(appIniPath.c_str());
			WriteIniString(appIniPath.c_str(),L"Origin",L"Locale",oldLangvalue);
		}

		// Delete the old file.
		EA::IO::File::Remove((applicationDataFolderNew+L"\\EADMUI_App.ini").c_str());

	}

	return ret;
}
/*
LRESULT CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	gProgressBarWnd = hWndDlg;
	switch(Msg)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		break;
	}

	return FALSE;
}
*/
static HWND gProgressBarDialog = NULL;
static HWND gProgressBar = NULL;
static HANDLE gProgressBarThread = NULL;
static HANDLE gCreateProgressGUIThread = NULL;

DWORD WINAPI UpdateProgress(LPVOID lpThreadParameter)
{
	static int i=0;

	do
	{
		if(i>100)
			i=0;
	
		SendMessage(gProgressBar, PBM_SETPOS, i, 0);
		Sleep(33);
		i++;

	}
	while(gProgressBar!=NULL);
	
	return 0;
}


DWORD WINAPI CreateProgressGUI(LPVOID lpThreadParameter)
{
	//initialize progress controls
	INITCOMMONCONTROLSEX icce = { sizeof(INITCOMMONCONTROLSEX), ICC_PROGRESS_CLASS };
	InitCommonControlsEx(&icce);

	//create our progress bar window		
	gProgressBarDialog = CreateDialog(AfxGetResourceHandle(), MAKEINTRESOURCE(IDD_PROGRESSBARDLG), 0, 0);
	gProgressBar = GetDlgItem(gProgressBarDialog, IDC_PROGRESSBAR);

	//set the progress bar initial position
	SendMessage(gProgressBar, PBM_SETPOS, 0, 0);
	wchar_t *msg = (wchar_t*)lpThreadParameter;
	SetDlgItemText(gProgressBarDialog, IDC_PROGRESSBARTEXT, msg);
	delete [] msg;

	// show the progress bar window
	ShowWindow(gProgressBarDialog, SW_SHOW);

	//begin updating thread...
	gProgressBarThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UpdateProgress, NULL, 0, 0);
		
	while (gProgressBar!=NULL)
	{
		MSG messages;
		if(PeekMessage(&messages, 0,  0, 0, PM_REMOVE)!=0)
		{
			TranslateMessage(&messages);
			DispatchMessage(&messages);
		}
	}
		
	return 0;
}


extern "C" void PASCAL EXPORT ShowProgressWindow(wchar_t* message, bool start)
{
	if(start)
	{	
		// copy the message for the thread...
		wchar_t *msg = new wchar_t[1024];
		wcscpy_s(msg, 1023, message);
		gCreateProgressGUIThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CreateProgressGUI, (LPVOID)msg, 0, 0);
	}
	else
	{
		// end the threads...
		gProgressBar = 0;
		CloseHandle(gProgressBarThread);
		CloseHandle(gCreateProgressGUIThread);
		DestroyWindow(gProgressBarDialog);
	}

}


bool FindEAL(CString directory)
{
    CString szSearch;
    CString szDirectory;

    HANDLE hFind = NULL;
    WIN32_FIND_DATA FindFileData;

    szSearch = directory + L"\\*.*";
    hFind = FindFirstFile(szSearch, &FindFileData);
    bool found = false;

    if(hFind == INVALID_HANDLE_VALUE)
        return false;
    do {
        if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(FindFileData.cFileName, L".") != 0 && wcscmp(FindFileData.cFileName, L"..") != 0)
        {
            CString dir = directory;
            dir += L"\\";
            dir += FindFileData.cFileName;
            found = FindEAL(dir);
            if (found)
            {
                FindClose(hFind);
                return found;
            }
        }
        else
        {
            CString fileName = FindFileData.cFileName;
            if (fileName.CompareNoCase(L"eal_dm_jobs.eal") == 0)
            {
                FindClose(hFind);
                found = true;
                return found;
            }
        }
    } while(FindNextFile(hFind, &FindFileData));

    FindClose(hFind);
    return found;
}


extern "C" BOOL PASCAL EXPORT DoPendingDownloadsExist()
{
    BOOL ret = false;

    wchar_t bufferCache[ MAX_PATH ] = {0};
    tstring cacheFolder;

    DebugMessageBox(L"DoPendingDownloadsExist");


    // search eacore_app.ini
    EA::IO::String16 eacoreIniName;
    CString sClientPath = ReadRegString(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Electronic Arts\\EA Core"), _T("ClientPath"));
    eacoreIniName.assign(sClientPath);

    EA::IO::String16::size_type pos = eacoreIniName.find_last_of(L"\\");
    if( pos != EA::IO::String16::npos )
    {
        if( pos+1 < eacoreIniName.length() )
        {
			wchar_t bufferAllUser[ MAX_PATH ] =  {0};
            eacoreIniName.replace( pos, eacoreIniName.length() - (pos), (L"\0") );
            eacoreIniName += L"\\EACore_App.ini";

            // search eacore_app.ini for SharedServerId
            EA::IO::IniFile eacoreAppIniFile((const char16_t*)eacoreIniName.c_str());

            EA::IO::String16 value;
            ret = eacoreAppIniFile.ReadEntry( (const char16_t*)L"EACore", (const char16_t*)L"SharedServerId", value );

            if( SHGetFolderPathW( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, bufferAllUser ) == S_OK )
            {
				const wchar_t* kCacheDirId = L"base_cache_dir";
				wchar_t bufferAllUserNew[ MAX_PATH ] =  {0};
			
                wcscpy_s(bufferAllUserNew, sizeof(bufferAllUserNew)/sizeof(bufferAllUserNew[0]), bufferAllUser);
                wcscat_s(bufferAllUser, sizeof(bufferAllUser)/sizeof(bufferAllUser[0]), L"\\Electronic Arts\\EA Core\\prefs\\");
                wcscat_s(bufferAllUser, sizeof(bufferAllUser)/sizeof(bufferAllUser[0]), value.c_str());
                wcscat_s(bufferAllUser, sizeof(bufferAllUser)/sizeof(bufferAllUser[0]), L".ini");

                EA::IO::IniFile appIniFile((const char16_t*)bufferAllUser);

                // extract download cache folders from EACORE ini file
                ret = appIniFile.ReadEntry( (const char16_t*)L"EACore", (const char16_t*)kCacheDirId, value );
                if( ret != -1 )
                {
                    cacheFolder.assign(value.c_str());

                    ret = appIniFile.ReadEntry( (const char16_t*)L"EACoreSession", (const char16_t*)L"full_base_cache_dir", value );
                    if( ret != -1 )
                    {
                        cacheFolder.assign(value.c_str());
                    }
                }
            }
            eacoreAppIniFile.Close();
        }
    }

    if(cacheFolder.empty() || !EA::IO::Directory::Exists(cacheFolder.c_str()))
    {
        // search in the settings.xml file from 8.x for the cache folder

        SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, bufferCache );
        tstring settingsFile = bufferCache;
        settingsFile += L"\\Origin\\Settings.xml";

        if (EA::IO::File::Exists(settingsFile.c_str()))
        {
            // get the cache folder from the settings file
            CString fileContent;
            if (LoadCStringFromFile(settingsFile.c_str(), fileContent))
            {
                const CString match("Setting key=\"CacheDir\" type=\"10\" value=\"");
                int posStart = fileContent.Find(match);

                if (posStart != -1)
                {
                    posStart += match.GetLength(); // advance cursor pos
                    int posEnd = fileContent.Find(L"\"", posStart );
                    if (posEnd != -1)
                        cacheFolder = fileContent.Mid(posStart, posEnd - posStart);
                }
            }
        }
        else
        {
            SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, bufferCache );
            tstring settingsFile = bufferCache;
            settingsFile += L"\\Origin\\Settings.xml";

            if (EA::IO::File::Exists(settingsFile.c_str()))
            {
                // get the cache folder from the settings file
                CString fileContent;
                if (LoadCStringFromFile(settingsFile.c_str(), fileContent))
                {
                    const CString match("Setting key=\"CacheDir\" type=\"10\" value=\"");
                    int posStart = fileContent.Find(match);

                    if (posStart != -1)
                    {
                        posStart += match.GetLength(); // advance cursor pos
                        int posEnd = fileContent.Find(L"\"", posStart );
                        if (posEnd != -1)
                            cacheFolder = fileContent.Mid(posStart, posEnd - posStart);
                    }
                }
            }
        }
    }
    
    ret = false;

    // scan the download cache folder for eal_dm_jobs.eal
    if (!cacheFolder.empty())
    {
        return FindEAL(cacheFolder.c_str());
    }

    return ret;
}

extern "C" BOOL PASCAL EXPORT IsUserInCanada()
{
    GEOID id = GetUserGeoID(GEOCLASS_NATION);
    if (id == 39) // Canada ; See: https://msdn.microsoft.com/en-us/library/windows/desktop/dd374073(v=vs.85).aspx
        return TRUE;

    return FALSE;
}