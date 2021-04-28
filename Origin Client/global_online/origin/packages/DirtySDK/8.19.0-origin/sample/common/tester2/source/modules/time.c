/*H********************************************************************************/
/*!
    \File time.c

    \Description
        Test the plat-time functionality.

    \Copyright
        Copyright (c) 2012 Electronic Arts

    \Version 07/12/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "zlib.h"
#include "testermodules.h"

// include for ds_plattimetotime() test
#if (defined(DIRTYCODE_PS3))
#include <cell/rtc.h>
#elif (defined(DIRTYCODE_PSP2))
#ifndef SCE_RTC_USE_LIBC_TIME_H
#define SCE_RTC_USE_LIBC_TIME_H 1
#endif
#include <rtc.h>
#elif (defined(DIRTYCODE_XENON))
#include <xtl.h>
#endif

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// Variables

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CmdTime

    \Description
        Test the time functions

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output standard return value

    \Version 07/12/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdTime(ZContext *argz, int32_t argc, char *argv[])
{
    char strBuf[20];
    struct tm curtime;
    uint32_t uTime;
    int32_t iZone;
    void *pPlatTime = NULL;

    // get current time... this is equivalent to time(0)
    uTime = ds_timeinsecs();
    ZPrintf("time: ds_timeinsecs()=%d\n", uTime);

    // convert to tm time
    ds_secstotime(&curtime, uTime);
    ZPrintf("time: ds_secstotime()=%d/%d/%d %d:%d:%d\n",
        curtime.tm_mon+1, curtime.tm_mday, curtime.tm_year+1900,
        curtime.tm_hour, curtime.tm_min, curtime.tm_sec);

    // test ISO_8601 conversion
    ZPrintf("time: ds_timetostr(ISO_8601)=%s\n", ds_timetostr(&curtime, TIMETOSTRING_CONVERSION_ISO_8601, 1, strBuf));

    // get localtime
    memset(&curtime, 0, sizeof(curtime));
    ds_localtime(&curtime, uTime);
    ZPrintf("time: ds_localtime()=%d/%d/%d %d:%d:%d\n",
        curtime.tm_mon+1, curtime.tm_mday, curtime.tm_year+1900,
        curtime.tm_hour, curtime.tm_min, curtime.tm_sec);

    // timezone (delta between local and GMT time in seconds)
    iZone = ds_timezone();
    ZPrintf("time: ds_timezone=%ds, %dh\n", iZone, iZone/(60*60));

    // TODO - test ds_strtotime()

    // test ds_plattimetotime
    #if defined(DIRTYCODE_PS3)
    {
        CellRtcTick cellTick;
        cellRtcGetCurrentTick(&cellTick);
        pPlatTime = &cellTick;
    }
    #elif defined(DIRTYCODE_XENON)
    {
        FILETIME fileTime;
        GetSystemTimeAsFileTime(&fileTime);
        pPlatTime = &fileTime;
    }
    #endif
    if (pPlatTime != NULL)
    {
        memset(&curtime, 0, sizeof(curtime));
        ds_plattimetotime(&curtime, pPlatTime);
        ZPrintf("time: ds_plattimetotime()=%d/%d/%d %d:%d:%d\n",
            curtime.tm_mon+1, curtime.tm_mday, curtime.tm_year+1900,
            curtime.tm_hour, curtime.tm_min, curtime.tm_sec);
    }

    return(0);
}


