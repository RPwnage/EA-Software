/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/system/fibersemaphore.h"
#include "framework/system/fibermanager.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
/*** Public Methods ******************************************************************************/
/*** Private Methods ******************************************************************************/

FiberSemaphore::FiberSemaphore()
{
    clear();
}

FiberSemaphore::~FiberSemaphore()
{
    clear();
}

/*! ***************************************************************************/
/*! \brief initialize the semaphore for use.  Semaphores must be initialized before use.

    \param name The name of this semaphore (debug purposes only)
           threshold The threshold to set on the semaphore. 
           (Threshold = # of signals it takes to wake up next in line)
           count The initial count to set on the semaphore.
*******************************************************************************/
void FiberSemaphore::initialize(const char8_t* name, SemaphoreId id, int32_t threshold/* = 1*/, int32_t count/* = 0*/)
{
    mName = name;
    mId = id;
    mCount = count;
    mThreshold = threshold;
    mSignalError = ERR_OK;
}

void FiberSemaphore::clear()
{
    mId = INVALID_SEMAPHORE_ID;
    mWaitingEvents.clear();
}

Fiber::Priority FiberSemaphore::getNextWaitingFiberPriority()
{
    if(!mWaitingEvents.empty())
    {
        return mWaitingEvents.front().second;
    }
    return TimeValue::getTimeOfDay().getMicroSeconds();
}

/*! ***************************************************************************/
/*! \brief wait on the semaphore

  \ \note the semaphore is currently implemented as single use -- we expire/release
          the semaphore after it has been waited and signaled on (regardless of order)
          and there are no others waiting on this semaphore
*******************************************************************************/
BlazeRpcError FiberSemaphore::wait(bool* releaseWhenDone)
{
    EA_ASSERT((mId != INVALID_SEMAPHORE_ID) &&
            "Trying to wait on an uninitialized semaphore!");

    BlazeRpcError sleepError = ERR_OK;
    if ((!mWaitingEvents.empty()) ||
        (mCount < mThreshold))
    {
        eastl::pair<Fiber::EventHandle, Fiber::Priority> listEntry(Fiber::getNextEventHandle(), Fiber::getCurrentPriority());
        mWaitingEvents.push_back(listEntry); //store the priority with the event in case of delay signal
        sleepError = Fiber::wait(listEntry.first, this->mName.c_str()); 

        // take ourself out of the waitlist
        mWaitingEvents.remove(listEntry);
    }
    mCount -= mThreshold;
    BlazeRpcError returnError = (sleepError != ERR_OK) ? sleepError : mSignalError;
    mSignalError = ERR_OK; // reset the mSignalError

    if (mWaitingEvents.empty())
        *releaseWhenDone = true;

    return returnError;
}

void FiberSemaphore::signal(BlazeRpcError reason)
{
    EA_ASSERT_MESSAGE(mId != INVALID_SEMAPHORE_ID, "Trying to wait on an uninitialized semaphore!");

    mCount++;
    mSignalError = reason;
    if (!mWaitingEvents.empty() && (reason != ERR_OK || mCount >= mThreshold))
    {
   
        eastl::pair<const Fiber::EventHandle, Fiber::Priority>& entry = mWaitingEvents.front();
        if (reason != Blaze::ERR_OK)
        {
            BLAZE_TRACE_LOG(Log::SYSTEM, "[FiberSemaphore].signal: waking up event " << entry.first.eventId << " due to  signalError " 
                << (ErrorHelp::getErrorName(reason)) << " on semaphore " << mName.c_str() << " (" << mId << ")");
        }

        Fiber::signal(entry.first, reason);
    }
}

} // Blaze

