#include <jlib/Time.h>

#ifdef _WIN32
#include <jlib/windefs.h>
#else
#include <unistd.h>
#endif

namespace jlib
{

static LARGE_INTEGER freq;
static LARGE_INTEGER start;

INT64 Time::getTimeAsMSEC(void)
{
	static bool once = true;

	if (once)
	{
#ifdef _WIN32
		QueryPerformanceFrequency (&freq);
		QueryPerformanceCounter (&start);
		freq.QuadPart /= 1000;
#else
#error "Port this"
#endif
		once = false;
	}

#ifdef _WIN32
	LARGE_INTEGER now;

	QueryPerformanceCounter (&now);

	INT64 delta = now.QuadPart - start.QuadPart;
	return (INT64) (delta / freq.QuadPart);
#else
#error "Port this"
#endif

}

void Time::sleep(int msec)
{
#ifdef _WIN32
    Sleep(msec);
#else
    usleep((msec) * 1000);
#endif
}

}