#ifndef _HELPERS_DOT_H
#define _HELPERS_DOT_H

#include <Windows.h>
#include <strsafe.h>

#define IGOProxyLogInfo(fmt, ...) IGOProxy::Logger::instance()->info(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define IGOProxyLogError(fmt, ...) IGOProxy::Logger::instance()->error(__FILE__, __LINE__, fmt, __VA_ARGS__)


namespace IGOProxy
{
	// Create window to be used when instantiating graphics devices/swap chains
	HWND createRenderWindow(LPCTSTR title);

	// Extract the address of a specific interface method
	LPVOID GetInterfaceMethod(LPVOID intf, DWORD methodIndex);

	// Get the size in bytes of a loaded module
	ULONG WINAPI GetSizeOfImage(HMODULE module);

	// Get md5 of a specific file content
	bool ComputeFileMd5(HMODULE module, char md5[33]);

    // API-specific offset collection functions
    int LookupDX10Offsets();
    int LookupDX11Offsets();
	int LookupDX12Offsets();

	// Get common data path
	bool GetCommonDataPath(WCHAR buffer[MAX_PATH]);

	// Store the final data in the IGO cache
	int StoreLookupData(const char* fileName, void* data, size_t dataSize);

	//////////////////////////////////////

	class Logger
	{
	public:
		static Logger *instance();
		~Logger();

		void info(const char *file, int line, const char *fmt, ...);
		void error(const char *file, int line, const char *fmt, ...);

	private:
		Logger();

		WCHAR mLogFile[MAX_PATH];
		HANDLE mLogHandle;

		////

		const char* timeString();
		const WCHAR* appName();
		const char* basename(const char *path);

		void writeLog(const char *type, const char *file, int line, const char *fmt, va_list arglist);
		void checkLogFile();

	private:
		HRESULT writeParameters(STRSAFE_LPSTR string, int stringSize, const char *fmt, ...);
		HRESULT writeParameters(STRSAFE_LPSTR string, int stringSizeInBytes, const char *fmt, va_list arglist);
	};
}

#endif