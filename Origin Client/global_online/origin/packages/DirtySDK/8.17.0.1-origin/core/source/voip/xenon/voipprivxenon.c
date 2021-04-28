/*H********************************************************************************/
/*!
    \File voipprivxenon.c

    \Description
        Private functions for VoIP module use.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/17/2004 (jbrookes) First Version
    \Version 1.1 11/18/2008 (mclouatre) Move mem group fcts to ../voippriv.c
    \Version 1.2 10/26/2009 (mclouatre) Renamed from xbox/voipprivxbox.c to xenon/voipprivxenon.c
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <xtl.h>
#include <xonline.h>
#include <stdio.h>
#include <stdarg.h>

#include "platform.h"
#include "voipdef.h"
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
        High-resolution timer using microsecond scale.

    \Input *pTimer  - timer

    \Output
        uint32_t    - current microsecond count

    \Notes
        This timer overflows in just over an hour.

    \Version 1.0 09/09/2005 (jbrookes) First Version
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
    \Function   VoipXuidFromUser

    \Description
        Convert "machine address" (textual form of XUID) into an actual XUID.

    \Input *pXuid           - [out] storage for decoded XUID
    \Input *pVoipUser       - source textual XUID to convert to a binary XUID
    
    \Output
        None.

    \Version 1.0 05/27/2005 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipXuidFromUser(XUID *pXuid, VoipUserT *pVoipUser)
{
    const char *pAddrText;
    char cNum;
    int32_t iChar;
    ULONGLONG *pXuidData = (ULONGLONG *)pXuid;

    // convert from string to qword
    for (iChar = 0, *pXuidData = 0, pAddrText = pVoipUser->strUserId + 1; iChar < 16; iChar++)
    {
        // make room for next entry
        *pXuidData <<= 4;

        // add in character
        cNum = pAddrText[iChar];
        cNum -= ((cNum >= 0x30) && (cNum < 0x3a)) ? '0' : 0x57;
        *pXuidData |= (ULONGLONG)cNum;
    }
}
