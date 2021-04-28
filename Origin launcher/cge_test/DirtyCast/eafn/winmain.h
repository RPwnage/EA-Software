/*H********************************************************************************/
/*!
    \File winmain.h

    \Description
        Windows based main() handler. Implements a console with stdout/stderr support,
        pausing and simple signal simulation.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 12/18/2004 (gschaefer) Initial version
*/
/********************************************************************************H*/

#ifndef _winmain_h
#define _winmain_h

/*** Include files ****************************************************************/

#include <signal.h>

/*** Defines **********************************************************************/

// windows has incomplete SIGxxx definitions -- add the ones we want
#ifndef SIGHUP
#define SIGHUP 1
#endif

// remap signal calls to our handler
#define signal(iKind, pFunc) WinSignal(iKind, pFunc)

/*** Type Definitions *************************************************************/

typedef void (WinSignalHandlerT)(int32_t);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// expose under real name in case someone wants to call it directly
#ifdef __cplusplus
extern "C"
#endif
WinSignalHandlerT *WinSignal(int32_t iSigNum, WinSignalHandlerT* pSigProc);

#endif
