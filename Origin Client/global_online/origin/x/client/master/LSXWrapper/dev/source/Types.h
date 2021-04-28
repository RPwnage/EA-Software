#ifndef __TYPES_H__
#define __TYPES_H__

#ifdef _WIN32

#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
typedef SYSTEMTIME OriginTimeT;

#else

struct OriginTimeT
{
	uint16_t wYear;
	uint16_t wMonth;
	uint16_t wDayOfWeek;
	uint16_t wDay;
	uint16_t wHour;
	uint16_t wMinute;
	uint16_t wSecond;
	uint16_t wMilliseconds;
};

#endif

#endif //__TYPES_H__