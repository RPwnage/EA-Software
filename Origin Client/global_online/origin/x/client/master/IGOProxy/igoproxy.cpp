#include <windows.h>
#include "Helpers.h"

namespace IGOProxy
{
	//////////////////////////////////////////////////////////////////////////////////////////////

	// Lookup the offsets we could use in IGO for a specific API/driver.
	int LookupDXOffsets(TCHAR* apiName)
	{
   		if (_wcsicmp(apiName, L"DX10") == 0)
			return LookupDX10Offsets();

   		if (_wcsicmp(apiName, L"DX11") == 0)
			return LookupDX11Offsets();

		if (_wcsicmp(apiName, L"DX12") == 0)
			return LookupDX12Offsets();

		IGOProxyLogError("Unsupported API %S", apiName);
		return -1;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////

	// Inject IGO in given process
	int InjectInProcess(DWORD pID, DWORD tID)
	{
#ifdef _WIN64
#ifdef _DEBUG
		const wchar_t * igoName = L"igo64d.dll";
#else
		const wchar_t * igoName = L"igo64.dll";
#endif

#else
		// just in case we ever want to support injecting from a 64-bit process(launcher) into a 32-bit one(game?)
#ifdef _DEBUG
		const wchar_t * igoName = L"igo32d.dll";
#else
		const wchar_t * igoName = L"igo32.dll";
#endif

#endif

		int ret = -1;

		HMODULE hIGO = GetModuleHandle(igoName);
		if (!hIGO)
			hIGO = LoadLibrary(igoName);
		if (!hIGO)
		{
			IGOProxyLogError("Unable to load DLL (%S)", igoName);
			return ret;
		}

		// get process handle with the right to create a remote thread
		HANDLE procHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_VM_OPERATION, FALSE, pID);
		if (procHandle)
		{
			HANDLE threadHandle = (tID != 0) ? OpenThread(THREAD_SUSPEND_RESUME, FALSE, tID) : NULL;
			// don't error out, if thread is not valid, we may not have a thread handle depending on the way the process was created

			typedef bool(*InjectIGOFunc)(HANDLE, HANDLE);
			InjectIGOFunc InjectIGODll = (InjectIGOFunc)GetProcAddress(hIGO, "InjectIGODll");

			if (InjectIGODll)
				ret = InjectIGODll(procHandle, threadHandle);    // do the actual injection

			// don't sleep here - ever!!!
			// DX9 hooking offsets for x64 bit games will be created anyway in the target process, if it lives long enough, if not, then it is not a game and we don't have to care

			CloseHandle(procHandle);

			if (threadHandle)
				CloseHandle(threadHandle);
		}

		else
		{
			DWORD error = GetLastError();
			IGOProxyLogError("Unable to open process (%d)", error);
		}

		return ret;
	}

} // IGOProxy

//////////////////////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow)
{
	int ret = -1;

	// parse command line
	LPWSTR *szArglist = NULL;
	int nArgs = 0;
	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if( NULL == szArglist )
		return ret;

	// what kind of work do we need to do?
	if (nArgs >= 2)
	{
		if (_wcsicmp(szArglist[1], L"-L") == 0)
		{
			if (nArgs == 3)
				ret = IGOProxy::LookupDXOffsets(szArglist[2]);

			else
				IGOProxyLogError("Got lookup command with bad args");
		}

		else
		{
			// Default is old behavior -> inject
			// get the process id
			DWORD pID = nArgs >= 2 ? _wtoi(szArglist[1]) : 0;
			// get the thread id
			DWORD tID = nArgs >= 3 ? _wtoi(szArglist[2]) : 0;

			if (pID > 0)
				ret = IGOProxy::InjectInProcess(pID, tID);
			else
				IGOProxyLogError("Invalid processID (%d)", pID);
		}
	}

	else
		IGOProxyLogError("No command found");

	LocalFree(szArglist);
	return ret;
}
