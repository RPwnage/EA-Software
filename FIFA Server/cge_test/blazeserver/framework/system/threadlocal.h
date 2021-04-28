/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE__THREADLOCAL_H
#define BLAZE__THREADLOCAL_H

/*** Include files *******************************************************************************/

#include "eathread/eathread_storage.h"
#include "eathread/eathread_thread.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

void registerThread();
void deregisterThread();
void freeThreadLocalInfo();
void printThreadLocals(EA::Thread::ThreadId threadId);

extern EA_THREAD_LOCAL bool gIsMainThread;
} // Blaze

#endif // BLAZE__THREADLOCAL_H

