/*H********************************************************************************/
/*!
    \File voipprivpc.c

    \Description
        Private functions for VoIP module use.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/17/2004 (jbrookes) First Version
    \Version 1.1 11/18/2008 (mclouatre) Move mem group fcts to ../voippriv.c
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include <stdio.h>
#include <stdarg.h>
#include <wtypes.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipTimer

    \Description
        Returns a high-resolution platform-dependent time value.

    \Input *pTimer  - storage for timer value

    \Output
        None

    \Version 1.0 08/26/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
uint32_t VoipTimer(VoipTimerT *pTimer)
{
    static LARGE_INTEGER TicksPerSecond = { 0, 0 };
    static DOUBLE fTicksPerMicrosecond;
    LARGE_INTEGER Time;
    
    // init?
    if (TicksPerSecond.QuadPart == 0)
    {
        QueryPerformanceFrequency(&TicksPerSecond);
        fTicksPerMicrosecond = (DOUBLE)TicksPerSecond.QuadPart * 0.000001;
    }

    QueryPerformanceCounter(&Time);
    return((uint32_t)((DOUBLE)Time.QuadPart / fTicksPerMicrosecond));
}

/*F********************************************************************************/
/*!
    \Function VoipTimerDiff

    \Description
        Differences two timers (a-b).

    \Input *pTimerA     - pointer to first timer
    \Input *pTimerB     - pointer to second timer
    
    \Output
        int32_t             - TimerA - TimerB

    \Version 1.0 08/26/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t VoipTimerDiff(const VoipTimerT *pTimerA, const VoipTimerT *pTimerB)
{
    __int64 timerA, timerB;

    timerA = ((__int64)pTimerA->uHi << 32) | pTimerA->uLo;
    timerB = ((__int64)pTimerB->uHi << 32) | pTimerA->uLo;

    return((int32_t)(timerA - timerB));
}

