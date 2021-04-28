#ifndef __ORIGIN_MAC_TYPES_H__
#define __ORIGIN_MAC_TYPES_H__

#include <time.h>

/// \ingroup types
/// \brief System time.
///
typedef struct _OriginTypeT {
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDayOfWeek;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
    unsigned short wMilliseconds;
}	OriginTimeT;


#endif //__ORIGIN_MAC_TYPES_H__