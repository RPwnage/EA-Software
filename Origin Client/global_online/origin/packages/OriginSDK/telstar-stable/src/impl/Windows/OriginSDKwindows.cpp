#include "stdafx.h"

#include "OriginSDK/OriginTypes.h"

#include <string>

#include "../OriginSDKimpl.h"
#include "../OriginErrorFunctions.h"
#include "../OriginSDKMemory.h"

#include "OriginSDKwindows.h"

#ifdef ORIGIN_PC

#include <psapi.h> 
#include <tchar.h>
#include <atlbase.h>
#include <atlconv.h>
#define MAX_PROCESSES 1024 	

#ifdef UNICODE
typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, Origin::Allocator<wchar_t>> tstring;
#define _TSTR(STR) EA_WCHAR(STR)
#define _TCHR(PTR) (wchar_t*)(PTR)
#else
typedef Origin::AllocString tstring;
#define _TSTR(STR) STR
#define _TCHR(PTR) (char*)(PTR)
#endif

#ifndef WIN64
#define INSTALL_REGISTRY_LOCATION _TSTR("SOFTWARE\\Origin")
#else
#define INSTALL_REGISTRY_LOCATION _TSTR("SOFTWARE\\Wow6432Node\\Origin")
#endif


#ifndef WIN64
#define IGO_DEBUG_API_DLL	_TSTR("IGO32d.dll")
#define IGO_API_DLL			_TSTR("IGO32.dll")
#else
#define IGO_DEBUG_API_DLL	_TSTR("IGO64d.dll")
#define IGO_API_DLL			_TSTR("IGO64.dll")
#endif

#include "stdafx.h"

#include "OriginSDK/OriginSDK.h"

// Forward declarations
tstring FindOrigin();
tstring FindIGOHookAPI(bool loadDebugVersion);

DWORD FindRunningProcess(__in_z LPCTSTR fileName, __in_z LPCTSTR altfileName) 
{ 
	HANDLE  hProcess; 
	DWORD   i, cProcesses, processId = 0; 

	DWORD processIds[MAX_PROCESSES];
	TCHAR baseName[MAX_PATH];

	if (EnumProcesses(processIds, MAX_PROCESSES*sizeof(DWORD), &cProcesses)) 
	{ 
		cProcesses /= sizeof(DWORD); 
		for (i = 0; i < cProcesses; i++) 
		{ 
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processIds[i]); 
			if (hProcess != NULL) 
			{ 
				if (GetModuleBaseName(hProcess, NULL, baseName, MAX_PATH) > 0) 
				{ 
					if (!lstrcmpi(baseName, fileName)) 
					{ 
						processId = processIds[i]; 
						CloseHandle(hProcess); 
						break; 
					} 

					if (!lstrcmpi(baseName, altfileName)) 
					{ 
						processId = processIds[i]; 
						CloseHandle(hProcess); 
						break; 
					} 
				} 
				CloseHandle(hProcess); 
			} 
		} 
	} 
	return processId; 
}

unsigned int FindProcessOrigin()
{
	tstring origin = FindOrigin();

	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];

	_tsplitpath_s(_TCHR(origin.c_str()), NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);

	TCHAR originName[_MAX_FNAME+_MAX_EXT+1];
	_stprintf_s(originName, _T("%s%s"), fname, ext);

	TCHAR originDebugName[_MAX_FNAME+_MAX_EXT+1];
	_stprintf_s(originDebugName, _T("%sDebug%s"), fname, ext);

	return FindRunningProcess(originName, originDebugName);
}

tstring FindOrigin()
{
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _TCHR(INSTALL_REGISTRY_LOCATION), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		TCHAR Data[MAX_PATH];
		DWORD Count = MAX_PATH * sizeof(TCHAR);
		DWORD Type = 0;

		if(RegQueryValueEx(hKey, _TCHR(_TSTR("ClientPath")), NULL, &Type, (LPBYTE)Data, &Count) == ERROR_SUCCESS)
		{
			tstring ret((tstring::value_type*)(Data));
			return ret;
		}
	}

	return tstring(_TSTR(""));
}

tstring FindIGOHookAPI(bool loadDebugVersion)
{
	HKEY hKey ;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _TCHR(INSTALL_REGISTRY_LOCATION), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		TCHAR Data[MAX_PATH];
		DWORD Count = MAX_PATH * sizeof(tstring::value_type);
		DWORD Type = 0;

		if(RegQueryValueEx(hKey, _TCHR(_TSTR("ClientPath")), NULL, &Type, (LPBYTE)Data, &Count) == ERROR_SUCCESS)
		{
			TCHAR Drive[_MAX_DRIVE];
			TCHAR Dir[_MAX_DIR];

			_tsplitpath_s(Data, Drive, _MAX_DRIVE, Dir, _MAX_DIR, NULL, 0, NULL, 0);

			tstring ret((tstring::value_type*)Drive);
			ret += (tstring::value_type*)Dir;
			ret += loadDebugVersion ? IGO_DEBUG_API_DLL : IGO_API_DLL;

			return ret;
		}
	}

	return tstring(_TSTR(""));
} 

tstring GenerateExecutablePath(const OriginStartupInputT& input)
{
#if defined(ORIGIN_DEVELOPMENT)
    tstring OriginLocation;
    if (input.OverrideOriginExecutable)
        OriginLocation = CA2T(input.OverrideOriginExecutable);
    else
        OriginLocation = FindOrigin();

    return OriginLocation;
#else
	return FindOrigin();
#endif	
}

tstring GenerateCommandLine(const OriginStartupInputT& input)
{
#if defined(ORIGIN_DEVELOPMENT)
	tstring OriginCmdline = _TSTR("Origin.exe");
	if (input.OverrideOriginCommandLine)
	{
		OriginCmdline += _TSTR(" ");
		OriginCmdline += CA2T(input.OverrideOriginCommandLine);
	}
	if (input.OverrideLsxPort != 0)
	{
		OriginCmdline += _TSTR(" -LsxPort:");
		char portNum[8];
		snprintf(portNum, 8, "%d", input.OverrideLsxPort);
		OriginCmdline += CA2T(portNum);
	}
	if (input.OriginLaunchSetting == OriginStartupInputT::OriginLaunchSetting_Always)
	{
		OriginCmdline += _TSTR(" -Origin_MultipleInstances");
	}

	return OriginCmdline;
#else
	return tstring();
#endif
}

namespace Origin
{

OriginErrorT OriginSDK::StartOrigin(const OriginStartupInputT& input)
{		
	// When running the unit test we do not want the SDK to start Origin.
	if(m_Flags & ORIGIN_FLAG_UNIT_TEST_MODE )
		return REPORTERROR(ORIGIN_SUCCESS);

	tstring Originlocation = GenerateExecutablePath(input);

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));

	si.cb = sizeof(si);
	
	TCHAR Drive[_MAX_DRIVE];
	TCHAR Dir[_MAX_DIR];

	_tsplitpath_s(_TCHR(Originlocation.c_str()), Drive, _MAX_DRIVE, Dir, _MAX_DIR, NULL, 0, NULL, 0);

	tstring path((tstring::value_type*)Drive);
	path += (tstring::value_type*)Dir;

	tstring OriginCmdline = GenerateCommandLine(input);
	if (CreateProcess(_TCHR(Originlocation.c_str()), _TCHR(OriginCmdline.c_str()), NULL, NULL, false, CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP, NULL, _TCHR(path.c_str()), &si, &pi))
	{
		return REPORTERROR(ORIGIN_SUCCESS);
	}

	return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED);
}

OriginErrorT OriginSDK::StartIGO(bool loadDebugVersion)
{
		// When running the unit test we do not want the SDK to start the IGO.
	if(m_Flags & ORIGIN_FLAG_UNIT_TEST_MODE)
		return REPORTERROR(ORIGIN_SUCCESS);

    HMODULE hModule = GetModuleHandle(loadDebugVersion ? IGO_DEBUG_API_DLL : IGO_API_DLL);
	if (!hModule)
		hModule = LoadLibrary(_TCHR(FindIGOHookAPI(loadDebugVersion).c_str()));
	
	if(hModule != NULL)
	{
		return REPORTERROR(ORIGIN_SUCCESS);
	}
	return REPORTERROR(ORIGIN_WARNING_IGO_NOTLOADED);
}

bool OriginSDK::IsOriginInstalled()
{
	return FindOrigin().compare(_TSTR("")) != 0;
}

}

#endif
