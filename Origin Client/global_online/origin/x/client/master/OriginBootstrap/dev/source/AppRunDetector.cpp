///////////////////////////////////////////////////////////////////////////////
// AppRunDetector.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
// Created by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#include "AppRunDetector.h"
#include <TlHelp32.h>
#include <math.h>

#define WM_COMMANDLINE_ACCEPTED WM_USER + 100

static const size_t BUFFER_SIZE = 1024;
static const unsigned int SND_MSG_TIMEOUT = 5000; // 5 seconds
namespace EA
{

	AppRunDetector::AppRunDetector(const wchar_t* pAppName, const wchar_t* pDetectorWindowClass, const wchar_t* pDetectorWindowName, const wchar_t *pWindowClass, HINSTANCE /*instance*/)
		: mbAnotherInstanceWasRunningGlobal(false),
		mbAnotherInstanceWasRunningLocal(false),
		mpAppName(pAppName),
		mpDetectorWindowClass(pDetectorWindowClass),
		mpDetectorWindowName(pDetectorWindowName),
        mpWindowClass (pWindowClass),
#if defined(_WIN32)
		mhLocalInstanceMutex(NULL),
		mhGlobalInstanceMutex(NULL)
#endif
	{
	}

	AppRunDetector::~AppRunDetector()
	{
#if defined(_WIN32)
		if(mhLocalInstanceMutex)
		{
			CloseHandle(mhLocalInstanceMutex);
			mhLocalInstanceMutex = NULL;
		}
		if(mhGlobalInstanceMutex)
		{
			CloseHandle(mhGlobalInstanceMutex);
			mhGlobalInstanceMutex = NULL;
		}
#endif
	}

	void AppRunDetector::Invalidate()
	{
		mbAnotherInstanceWasRunningGlobal = false;
		mbAnotherInstanceWasRunningLocal = false;

#if defined(_WIN32)
		if(mhLocalInstanceMutex)
		{
			CloseHandle(mhLocalInstanceMutex);
			mhLocalInstanceMutex = NULL;
		}
		if(mhGlobalInstanceMutex)
		{
			CloseHandle(mhGlobalInstanceMutex);
			mhGlobalInstanceMutex = NULL;
		}
#endif
	}


	bool AppRunDetector::IsAnotherInstanceRunning()
	{
		// If we already called this and it was true, we return true instead of try to lock 
		// the mutex again, which we ourselves currently own.
		if(mbAnotherInstanceWasRunningGlobal)
			return true;

		mbAnotherInstanceWasRunningGlobal = false;

		if(!mhGlobalInstanceMutex)
		{
			SetLastError(0);
			wchar_t globalName[BUFFER_SIZE+1]={0};
			wcscpy_s(globalName, BUFFER_SIZE, L"Global\\");
			wcscat_s(globalName, BUFFER_SIZE, mpAppName);
			mhGlobalInstanceMutex = CreateMutexW(0, TRUE, globalName);

			// EA_ASSERT(mhLocalInstanceMutex);

			if(mhGlobalInstanceMutex)
			{
				const DWORD dwError = GetLastError();
				if(dwError == ERROR_ALREADY_EXISTS)
				{
					//CloseHandle(mhGlobalInstanceMutex); // Do not close the handle, we need to hold it to reflect the correct reference count of the global mutex!
                                                          // If we close it and the "other" instance exits, the reference count of the global mutex will be 0 and it will be destroyed!   
					mhGlobalInstanceMutex = NULL;
					mbAnotherInstanceWasRunningGlobal = true;
				}
			}
			else
			{
                const DWORD dwError = GetLastError();
				if(dwError == ERROR_ACCESS_DENIED)
				{
					mhGlobalInstanceMutex = NULL;
					mbAnotherInstanceWasRunningGlobal = true;
				}
			}
		}
		// Else the first time we called this, another instance wasn't running and we 
		// were the first. On any subsequent calls of this function, we assume that 
		// other apps that start up will quit if they find we are running, as we are.

		return mbAnotherInstanceWasRunningGlobal;
	}


	bool AppRunDetector::WasAnotherInstanceRunning()
	{
		return mbAnotherInstanceWasRunningGlobal;
	}


	bool AppRunDetector::IsAnotherInstanceRunningLocal()
	{
		mbAnotherInstanceWasRunningLocal = false;

		if(!mhLocalInstanceMutex)
		{
			SetLastError(0);
			wchar_t LocalName[BUFFER_SIZE+1]={0};
			wchar_t UserName[BUFFER_SIZE+1]={0};
			DWORD size=sizeof(UserName)/sizeof(UserName[0]);
			GetUserNameW(UserName, &size); 
			wcscpy_s(LocalName, BUFFER_SIZE, L"Local\\");	// seems like \Local might be ignored by WinXP, so I added the username to make it session dependant
			wcscat_s(LocalName, BUFFER_SIZE, UserName);
			wcscat_s(LocalName, BUFFER_SIZE, mpAppName);
			mhLocalInstanceMutex = CreateMutexW(0, TRUE, LocalName);

			// EA_ASSERT(mhLocalInstanceMutex);

			if(mhLocalInstanceMutex)
			{
				const DWORD dwError = GetLastError();

				if(dwError == ERROR_ALREADY_EXISTS)
				{
					CloseHandle(mhLocalInstanceMutex); // We need to do this, right?
					mhLocalInstanceMutex = NULL;
					mbAnotherInstanceWasRunningLocal = true;
				}
			}		
			else
			{
				const DWORD dwError = GetLastError();  		
				if(dwError == ERROR_ACCESS_DENIED)
				{
					mhLocalInstanceMutex = NULL;
					mbAnotherInstanceWasRunningLocal = true;
				}
			}
		}     

		// Else the first time we called this, another instance wasn't running and we 
		// were the first. On any subsequent calls of this function, we assume that 
		// other apps that start up will quit if they find we are running, as we are.

		return mbAnotherInstanceWasRunningLocal;
	}


	bool AppRunDetector::WasAnotherInstanceRunningLocal()
	{
		return mbAnotherInstanceWasRunningLocal;
	}


    typedef struct
    {
        DWORD primaryInstancePID;
        const wchar_t *windowClass; 
    } searchWindowParam;

    BOOL CALLBACK enumWindowsProc(__in  HWND hWnd, __in  LPARAM lParam) 
    {
        DWORD dwTestPID = 0;
        searchWindowParam *searchParam = (searchWindowParam *)lParam;

        DWORD primaryInstancePID = searchParam->primaryInstancePID;
        ::GetWindowThreadProcessId(hWnd, &dwTestPID);

        if (dwTestPID == primaryInstancePID && IsWindowVisible(hWnd))
        {
            WCHAR buffer [512];
            int len = GetClassName(hWnd, buffer, 512);
            if (len)
            {
                if (wcscmp (&buffer[0], searchParam->windowClass) == 0)
                {
                    ::SetForegroundWindow(hWnd);
                    return false;
                }
            }
        }
        return true;
    }

	bool AppRunDetector::ActivateRunningInstance(std::wstring argument, bool bFwdAppFocus /*=true*/)
	{
#if defined(_WIN32)
        //the distinction between mpDetectorWindowClass and mpWindowClass is that with Qt4, the window class was QWidget, and with Qt5, it changed to Qt5QWindowIcon
        //but we still need to be able to detect whether an older version of client is already running, so with 9.4+ we create a "detector" window in the client
        //of type QWidget, and of name "Origin"
        HWND hwndFirst = FindWindowW(mpDetectorWindowClass, mpDetectorWindowName);

		// Note process Id
		DWORD dwPrimaryInstancePID;
		::GetWindowThreadProcessId(hwndFirst, &dwPrimaryInstancePID);
		if(!hwndFirst)
			return false;

		//
		// Transfer command line using dummy source window
		// Create a stupid window class
		WNDCLASS wClass;
		memset(&wClass, 0, sizeof(wClass));
		wClass.style = 0;
		wClass.lpfnWndProc = DefWindowProc;
		wClass.hInstance = GetModuleHandle(NULL);
		wClass.lpszClassName = L"AppRunDetectorHidden";

		RegisterClass(&wClass);

		// Make a fake source window
		HWND srcHwnd = CreateWindowEx(0, wClass.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

		// Set up our copy data
		COPYDATASTRUCT copyData;

		// Set up a string for Win32 to copy
		copyData.dwData = kWin32ActivateIdentifier;

        if (argument.length() > 0)
        {
            copyData.cbData = argument.length() * sizeof(wchar_t);
            copyData.lpData = reinterpret_cast<void *>(const_cast<wchar_t*>(argument.c_str()));
        }
        else
        {
            copyData.cbData = 0;
            copyData.lpData = NULL;
        }

        // If the send message doesn't succeed, handle it ourselves.
		if(SendMessageTimeout(hwndFirst, WM_COPYDATA, reinterpret_cast<WPARAM>(srcHwnd), reinterpret_cast<LPARAM>(&copyData), SMTO_BLOCK, SND_MSG_TIMEOUT, NULL) == 0)
        {
		    DestroyWindow(srcHwnd);
            return false;
        }

        // Now lets check for 2 seconds on whether the other Origin is handling our request.
        DWORD now = GetTickCount();

        MSG msg;
        do
        {
            if(PeekMessage(&msg, srcHwnd, WM_COMMANDLINE_ACCEPTED, WM_COMMANDLINE_ACCEPTED, PM_REMOVE))
            {
                if(msg.message == WM_COMMANDLINE_ACCEPTED)
                {
                    if(msg.wParam == 0)
                    {
                        // The other Origin informed us that it is not able to handle the request. Lets continue taking care of it.
                        DestroyWindow(srcHwnd);
                        return false;
                    }
                    else
                    {
                        // The other Origin informed us that it is taking care of business.
                        break;
                    }
                }
            }
            Sleep(10);
        }
        while(GetTickCount() < now + 2000);

        DestroyWindow(srcHwnd);

		//
		// Pull original instance forward (if not explicitly disabled through the command line option)
		if (bFwdAppFocus)
		{
            //with 9.4+(Qt5) we needed to add a dummy window used to detect an instance of origin already running
            //and that's stored in hwndFirst
            //but that's not the main window so we bringing that forward won't help
            //to determine the actual window, we need to search for the actual application window which is of type Qt5QWindowIcon
            //but we can't just use the name as "Origin" because existence of EACore.ini changes the window title to "Origin <version> R&D mode..."
            //so we can't use FindWindow, but need to use EnumWindows instead, looking for a window that matches process id + window class
            searchWindowParam clientWindowInfo;
            clientWindowInfo.primaryInstancePID = dwPrimaryInstancePID;
            clientWindowInfo.windowClass = mpWindowClass;

           ::EnumWindows( enumWindowsProc, (LPARAM)&clientWindowInfo );
		}

		return true;
#else
		return false;
#endif
	}

	bool AppRunDetector::ForceRunningInstanceToTerminate()
	{
#if defined(_WIN32)
		HWND hwnd = FindWindowW(mpDetectorWindowClass, mpDetectorWindowName);
		if(hwnd)
		{
			DWORD processId = 0;
			HANDLE processHandle = NULL;
			GetWindowThreadProcessId(hwnd, &processId);
			processHandle = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, processId);

			if(processHandle)
			{
				TerminateProcess(processHandle, 0);
				CloseHandle(processHandle);

				return true;
			}

			// last check, just to be sure it worked....
			if(	FindWindowW(mpDetectorWindowClass, mpDetectorWindowName) == NULL )
				return true;
			else
				return false;

		}
        else if(IsAnotherInstanceRunning())
        {
            // haven't find "origin" window
            // but we found "origin" process
            // kill the process
            PROCESSENTRY32 entry;
            entry.dwSize = sizeof(PROCESSENTRY32);

            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

            if (Process32First(snapshot, &entry) == TRUE)
            {
                while (Process32Next(snapshot, &entry) == TRUE)
                {
                    if (wcsicmp(entry.szExeFile, L"origin.exe") == 0)
                    {  
                        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				        TerminateProcess(hProcess, 0);
                        CloseHandle(hProcess);
                    }
                }
            }

            CloseHandle(snapshot);

        }
#endif

		return true;
	}

	void AppRunDetector::ShowWindowsRunningApplication()
	{
#if defined(_WIN32)	
		//lpWindowName is the appName, because all child dialog were register like QWidget class
		//and we must show just the main interface.
		HWND hwnd = FindWindowW(mpWindowClass, L"Origin");
		SendMessageTimeout(hwnd, WM_SYSCOMMAND, SC_RESTORE, SC_RESTORE, SMTO_BLOCK, SND_MSG_TIMEOUT, NULL);
		SetForegroundWindow(hwnd);
#endif

	}

} // namespace EA


