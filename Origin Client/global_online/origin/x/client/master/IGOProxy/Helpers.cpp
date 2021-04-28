#include "Helpers.h"

#include <stdio.h>
#include <Windows.h>
#include <WinError.h>
#include <Shlobj.h>
#include <Shlwapi.h>

#include "md5.h"

namespace IGOProxy
{
	// Create a simple invisible window to associate with our API devices.
	LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		// sort through and find what code to run for the message given
		switch (message)
		{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		} break;
		}

		// Handle any messages the switch statement didn't
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	HWND createRenderWindow(LPCTSTR title)
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;

			// init window class
			WNDCLASSEX windowClass;
			ZeroMemory(&windowClass, sizeof(WNDCLASSEX));
			windowClass.cbSize = sizeof(WNDCLASSEX);
			windowClass.style = CS_HREDRAW | CS_VREDRAW;
			windowClass.lpfnWndProc = WindowProc;
			windowClass.hInstance = GetModuleHandle(NULL);
			windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
			windowClass.lpszClassName = L"HelperRenderWindow";
			RegisterClassEx(&windowClass);
		}

		HWND wnd = CreateWindowEx(NULL,
			L"HelperRenderWindow",    // name of the window class
			title,
			WS_OVERLAPPEDWINDOW,
			300,
			300,
			100,
			100,
			NULL,					// we have no parent window, NULL
			NULL,					// we aren't using menus, NULL
			GetModuleHandle(NULL),  // application handle
			NULL);					// used with multiple windows, NULL

		if (!wnd)
			IGOProxyLogError("Unable to create render window %S", title);

		return wnd;
	}

	LPVOID GetInterfaceMethod(LPVOID intf, DWORD methodIndex)
	{
#ifdef _WIN64
		return *(LPVOID*)(*(ULONG_PTR*)intf + methodIndex * 8);
#else
		return *(LPVOID*)(*(ULONG_PTR*)intf + methodIndex * 4);
#endif
	}

	ULONG WINAPI GetSizeOfImage(HMODULE module)
	{
		if (*((WORD *)module) == IMAGE_DOS_SIGNATURE)
		{
			DWORD offset = ((IMAGE_DOS_HEADER*)module)->e_lfanew;
			PIMAGE_NT_HEADERS headers = (IMAGE_NT_HEADERS *)((ULONG_PTR)module + offset);
			if (headers->Signature == IMAGE_NT_SIGNATURE)
			{
				if (headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
					return ((PIMAGE_NT_HEADERS64)headers)->OptionalHeader.SizeOfImage;
				else
					return ((PIMAGE_NT_HEADERS32)headers)->OptionalHeader.SizeOfImage;
			}
		}

		return 0;
	}

	bool ComputeFileMd5(HMODULE module, char md5[33])
	{
		WCHAR fileName[MAX_PATH];
		DWORD fileNameLen = GetModuleFileName(module, fileName, MAX_PATH);
		if (fileNameLen == 0 || fileNameLen == MAX_PATH)
		{
			DWORD error = GetLastError();
			IGOProxyLogError("Unable to retrieve the module file name - err=%d", error);
			return false;
		}

		HANDLE fd = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fd == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			IGOProxyLogError("Unable to open module file '%S' - err=%d", fileName, error);
			return false;
		}

		bool retVal = false;

		LARGE_INTEGER fileSizeInfo;
		if (GetFileSizeEx(fd, &fileSizeInfo))
		{
            DWORD fileSize = static_cast<DWORD>(fileSizeInfo.QuadPart);
			char* data = new char[fileSize];
			ZeroMemory(data, fileSize);

			DWORD readSize = 0;
			BOOL success = ReadFile(fd, data, fileSize, &readSize, NULL);
			if (success && readSize == fileSize)
			{
				calculate_md5(data, readSize, md5);
				retVal = true;
			}

			else
			{
				DWORD error = GetLastError();
				IGOProxyLogError("Failed to retrieve the module content for MD5 (%d/%lld) - err=%d", readSize, fileSize, error);
			}

			delete[] data;
		}

		else
		{
			DWORD error = GetLastError();
			IGOProxyLogError("Failed to retrieve the module file size '%S' - err=%d", fileName, error);
		}

		CloseHandle(fd);

		return retVal;
	}

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

		if (FAILED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, buffer)))
		{
            if (!GetEnvironmentVariableW(L"ProgramData", buffer, MAX_PATH))
                return false;
		}

		return buffer[0] != '\0';
	}

	int StoreLookupData(const char* fileName, void* data, size_t dataSize)
	{
		WCHAR commonDataPath[MAX_PATH];
		if (GetCommonDataPath(commonDataPath))
		{
			// First validate path to final location
			WCHAR filePath[MAX_PATH];
			StringCchPrintfW(filePath, _countof(filePath), L"%s\\Origin", commonDataPath);
			if (!DirectoryExists(filePath))
			{
				if (CreateDirectory(filePath, NULL) == FALSE)
				{
					DWORD error = GetLastError();
					IGOProxyLogError("Unable to create directory '%S' - err=%d", error);
					return -1;
				}
			}

			StringCchPrintfW(filePath, _countof(filePath), L"%s\\Origin\\IGOCache", commonDataPath);
			if (!DirectoryExists(filePath))
			{
				if (CreateDirectory(filePath, NULL) == FALSE)
				{
					DWORD error = GetLastError();
					IGOProxyLogError("Unable to create directory '%S' - err=%d", error);
					return -1;
				}
			}

			StringCchPrintfW(filePath, _countof(filePath), L"%s\\Origin\\IGOCache\\%S", commonDataPath, fileName);
			HANDLE fd = CreateFile(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (fd != INVALID_HANDLE_VALUE)
			{
				DWORD writeSize = 0;
				if (WriteFile(fd, data, static_cast<DWORD>(dataSize), &writeSize, NULL) == FALSE)
				{
					DWORD error = GetLastError();
					IGOProxyLogError("Failed to save file content '%S' - err=%d", filePath, error);
				}

				else
				if (writeSize != dataSize)
				{
					DWORD error = GetLastError();
					IGOProxyLogError("Failed to save the entire file content (%d/%d) '%S' - err=%d", writeSize, dataSize, filePath, error);
				}

				FlushFileBuffers(fd);
				CloseHandle(fd);
				return 0;
			}

			else
			{
				DWORD error = GetLastError();
				IGOProxyLogError("Failed to create file '%S' - err=%d", filePath, error);
				return -1;
			}
		}

		else
		{
			IGOProxyLogError("Unable to get common data path");
			return -1;
		}
	}

	//////////////////////////////////////

	Logger *Logger::instance()
	{
		static Logger logger;
		return &logger;
	}

	Logger::Logger() :
		mLogHandle(NULL)
	{
	}

	Logger::~Logger()
	{
		if (mLogHandle)
		{
			CloseHandle(mLogHandle);
			mLogHandle = NULL;
		}
	}

	void Logger::info(const char *file, int line, const char *fmt, ...)
	{
		checkLogFile();

		if (mLogHandle == INVALID_HANDLE_VALUE)
			return;

		va_list args;
		va_start(args, fmt);
		writeLog("INFO", file, line, fmt, args);
		va_end(args);
	}

	void Logger::error(const char *file, int line, const char *fmt, ...)
	{
		checkLogFile();

		if (mLogHandle == INVALID_HANDLE_VALUE)
			return;

		va_list args;
		va_start(args, fmt);
		writeLog("WARN", file, line, fmt, args);
		va_end(args);
	}

	const char* Logger::timeString()
	{
		static char szTime[128];
		SYSTEMTIME time;
		GetLocalTime(&time);

		GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &time, "hh:mm:ss tt", szTime, sizeof(szTime) / sizeof(szTime[0]));
		return szTime;
	}

	const WCHAR *Logger::appName()
	{
		// NOTE: we can't use strings.h because we don't want to depend on msvcrt.lib
		static WCHAR szExePath[MAX_PATH] = { 0 };
		DWORD size = GetModuleFileName(GetModuleHandle(NULL), szExePath, MAX_PATH);
		int i = size - 1;

		// search backwards for path delimiter
		while (i >= 0 && szExePath[i] != '\\')
		{
			// remove .exe
			if (szExePath[i] == '.')
				szExePath[i] = 0;
			i--;
		}

		if (i >= 0 && szExePath[i] == '\\')
			i++;

		return ((i >= 0) ? (&szExePath[i]) : (NULL));
	}

	const char* Logger::basename(const char *path)
	{
		// NOTE: we can't use strings.h because we don't want to depend on msvcrt.lib

		// path must be null terminated
		const char *ptr = path;
		while (*ptr)
			ptr++;

		__int64 i = ptr - path - 1;

		// search backwards for path delimiter
		while (i >= 0 && path[i] != '\\')
			i--;

		if (i >= 0 && path[i] == '\\')
			i++;

		return ((i >= 0) ? (&path[i]) : (NULL));
	}

	HRESULT Logger::writeParameters(STRSAFE_LPSTR string, int stringSizeInBytes, const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		HRESULT hr = StringCbVPrintfA(string, stringSizeInBytes, fmt, args);
		va_end(args);
		return hr;
	}

	HRESULT Logger::writeParameters(STRSAFE_LPSTR string, int stringSizeInBytes, const char *fmt, va_list arglist)
	{
		HRESULT hr = StringCbVPrintfA(string, stringSizeInBytes, fmt, arglist);
		return hr;
	}

	void Logger::writeLog(const char *type, const char *file, int line, const char *fmt, va_list args)
	{
		char msg[4096] = { 0 };
		if (STRSAFE_E_INVALID_PARAMETER != writeParameters(msg, sizeof(msg), fmt, args))
		{
			static char* LOGFORMAT = "%s\t%s\t%d\t%20s:%5d\t\t%s\r\n";

			char log[2 * 4096] = { 0 };
			const char *baseName = basename(file);
			if (STRSAFE_E_INVALID_PARAMETER != writeParameters(log, sizeof(log), LOGFORMAT, type, timeString(), GetCurrentThreadId(), baseName != NULL ? baseName : "NULL", line, msg))
			{
				size_t size = 0;
				if (SUCCEEDED(StringCbLengthA(log, sizeof(log), &size)))
				{
					DWORD writeSize = 0;
					WriteFile(mLogHandle, log, static_cast<DWORD>(size), &writeSize, NULL);

					FlushFileBuffers(mLogHandle);
				}
			}
		}
	}

	void Logger::checkLogFile()
	{
		if (mLogHandle)
			return;

		mLogHandle = INVALID_HANDLE_VALUE;

		WCHAR commonDataPath[MAX_PATH];
		if (GetCommonDataPath(commonDataPath))
		{
			static wchar_t* LOGFILE = L"%s\\Origin\\Logs\\%s_%i_Log.txt";

			WCHAR filePath[MAX_PATH];
			StringCchPrintfW(filePath, _countof(filePath), L"%s\\Origin", commonDataPath);
			if (!DirectoryExists(filePath))
			{
				if (CreateDirectory(filePath, NULL) == FALSE)
				{
					DWORD error = GetLastError();
					IGOProxyLogError("Unable to create directory '%S' - err=%d", error);
					return;
				}
			}

			StringCchPrintfW(filePath, _countof(filePath), L"%s\\Origin\\Logs", commonDataPath);
			if (!DirectoryExists(filePath))
			{
				if (CreateDirectory(filePath, NULL) == FALSE)
				{
					DWORD error = GetLastError();
					IGOProxyLogError("Unable to create directory '%S' - err=%d", error);
					return;
				}
			}

			const WCHAR *appname = appName();
			StringCchPrintf(mLogFile, _countof(mLogFile), LOGFILE, commonDataPath, appname != NULL ? appname : L"NULL", GetCurrentProcessId());
			mLogHandle = CreateFile(mLogFile, (GENERIC_READ | GENERIC_WRITE), FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (mLogHandle != INVALID_HANDLE_VALUE)
			{
				DWORD bytesWritten = 0;

				// write header
				char tmp[256];
				int tmpSize = StringCbPrintfA(tmp, sizeof(tmp), "Process Information\r\n");

				// header
				WriteFile(mLogHandle, tmp, tmpSize, &bytesWritten, NULL);

				tmpSize = StringCbPrintfA(tmp, sizeof(tmp), "    PID: %d\r\n", GetCurrentProcessId());
				WriteFile(mLogHandle, tmp, tmpSize, &bytesWritten, NULL);

				WCHAR szExePath[MAX_PATH];
				GetModuleFileName(GetModuleHandle(NULL), szExePath, MAX_PATH);
				tmpSize = StringCbPrintfA(tmp, sizeof(tmp), "    EXE: %S\r\n", szExePath);
				WriteFile(mLogHandle, tmp, tmpSize, &bytesWritten, NULL);

				// format the date
				char szTime[128];
				SYSTEMTIME time;
				GetLocalTime(&time);
				int size = GetDateFormatA(NULL, 0, &time, "ddd',' MMM dd yyyy ", szTime, sizeof(szTime) / sizeof(szTime[0]));
				if (size > 0)
					GetTimeFormatA(NULL, 0, &time, "hh:mm:ss tt", &szTime[size - 1], sizeof(szTime) / sizeof(szTime[0]) - size);
				else
					szTime[0] = 0;

				tmpSize = StringCbPrintfA(tmp, sizeof(tmp), "STARTED: %s\r\n", szTime);
				WriteFile(mLogHandle, tmp, tmpSize, &bytesWritten, NULL);

			}
		}
		else
		{
			mLogHandle = INVALID_HANDLE_VALUE;
			mLogFile[0] = 0;
		}
	}

} // namespace IGOProxy