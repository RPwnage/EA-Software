#include "Timer.h"
#include "services/log/LogService.h"

#include <QObject>

#ifdef Q_OS_WIN
#include <Windows.h>
#else
#include <sys/time.h>
#endif

// Replaces GetTick() & 16ms resolution GetTick() with a much higher resolution timer that's
// then rounded down to ms resolution.
qint64 GetTick()
{
	return (GetTick_us() / qint64(1000));
}

qint64 GetTick_us()
{
	qint64 ret = 0;

#ifdef Q_OS_WIN
	static bool warningShown = false;
	// Maintain timer resolution for converting to per/second
	LARGE_INTEGER liQueryPerfFreq = {0};
	// Note: Some CPUs change this for throttling so no longer caching
	BOOL success = QueryPerformanceFrequency(&liQueryPerfFreq);
	if(!success || liQueryPerfFreq.QuadPart == 0)
	{
		// If we get here, the QueryPerformanceFrequency call failed or returned a frequency of 0.  MSDN documentation indicates that
		// this is because the user's hardware does not support a high-resolution performance counter. Instead, we will use the less
		// accurate GetTickCount()/GetTickCount64(). http://msdn.microsoft.com/en-us/library/windows/desktop/ms644905%28v=VS.85%29.aspx
		// Windows Vista+ supports GetTickCount64(), which doesn't suffer from the 49.7 day rollover problem that GetTickCount() does.
		typedef ULONGLONG (WINAPI *pGetTickCount64)(void);
		pGetTickCount64 pGTC64 = (pGetTickCount64)(uintptr_t)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetTickCount64");

		if(pGTC64)
			ret = pGTC64() * qint64(1000);
		else
			ret = static_cast<qint64>(GetTickCount()) * qint64(1000);

		if(!warningShown)
		{
			ORIGIN_LOG_WARNING << "GetTick_us(): Falling back to GetTickCount" << (pGTC64 ? "64" : "") <<
				" because " << (success ? "frequency is 0" : "QueryPerformanceFrequency call failed") << ".",
			warningShown = true;
		}
	}
	else
	{
		LARGE_INTEGER liRet;
		QueryPerformanceCounter(&liRet);

		ret = (qint64) ( ( (double)liRet.QuadPart / (double)liQueryPerfFreq.QuadPart ) * 1000000.0 );
	}

// Test for a bug we're experiencing where the timer appears to show many seconds ahead of a later request.
//#ifdef _DEBUG
//	LARGE_INTEGER liRetTest;
//	QueryPerformanceCounter(&liRetTest);
//	ORIGIN_ASSERT(liRetTest.QuadPart > liRet.QuadPart);
//#endif
#else
	timeval time;
	gettimeofday(&time, NULL);
	ret = (time.tv_sec * qint64(1000000)) + (time.tv_usec);
#endif

	return ret;
}
