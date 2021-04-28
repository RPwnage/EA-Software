/*! ************************************************************************************************/
/*!
    \file idlerutil.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/idlerutil.h"
#include "framework/connection/selector.h"

namespace Blaze
{
namespace GameManager
{

    IdlerUtil::IdlerUtil(const char8_t* logPrefix) :
        mIdleTimerId(INVALID_TIMER_ID), mIdleFiberId(Fiber::INVALID_FIBER_ID), mLogPrefix(logPrefix)
    {
    }
    IdlerUtil::~IdlerUtil()
    {
        shutdownIdler();
    }

    void IdlerUtil::shutdownIdler()
    {
        if (mIdleTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mIdleTimerId);
            mIdleTimerId = INVALID_TIMER_ID;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief called to schedule the next idle
        \param[in] offsetStartTime Prior idle's start time or else current time. Added to idle period, for next idle time
        \param[in] idleResult specified to omit or override when to do next idle
    *************************************************************************************************/
    void IdlerUtil::scheduleNextIdle(const TimeValue& offsetStartTime, const TimeValue& defaultIdlePeriod, IdleResult idleResult /*= SCHEDULE_IDLE_NORMALLY */)
    {
        // If prior idle fiber crashed, its mIdleFiberId is now unused. To re-allow idling, unset mIdleTimerId
        if ((mIdleTimerId != INVALID_TIMER_ID) && (mIdleFiberId != Fiber::INVALID_FIBER_ID) && !Fiber::isFiberIdInUse(mIdleFiberId))
        {
            WARN_LOG(mLogPrefix.c_str() << ": Idle with timer id(" << mIdleTimerId << ") fiber id(" << mIdleFiberId << ") went away.");
            mIdleTimerId = INVALID_TIMER_ID;
        }

        // schedule idle only if one isn't already. Return if NO_NEW_IDLE
        if ((mIdleTimerId != INVALID_TIMER_ID) || (idleResult == NO_NEW_IDLE))
            return;

        TimeValue now = TimeValue::getTimeOfDay();
        TimeValue nextIdle = (idleResult == SCHEDULE_IDLE_IMMEDIATELY) ? now : offsetStartTime + defaultIdlePeriod;    
        if (nextIdle < now)
        {
            WARN_LOG(mLogPrefix.c_str() << ": Idle run time(" << mLastIdleLength.getMillis() << ") ms has overrun idle period(" <<
                defaultIdlePeriod.getMillis() << ") ms. Check throttling and/or lengthen idle delay time");
            nextIdle = now;
        }

        // mIdleFiberId will be set once we're in the idle timer callback
        mIdleTimerId = gSelector->scheduleTimerCall(nextIdle, this,
            &IdlerUtil::idleTimerCallback, defaultIdlePeriod, "IdlerUtil::idleTimerCallback");

        TRACE_LOG(mLogPrefix.c_str() << ": Scheduled idle with timer id(" << mIdleTimerId << ") for time(" << nextIdle.getMillis() << ") " << ((idleResult == SCHEDULE_IDLE_IMMEDIATELY) ? " with SCHEDULE_IDLE_IMMEDIATELY." : ""));
    }

    void IdlerUtil::idleTimerCallback(TimeValue defaultIdlePeriod)
    {
        mIdleFiberId = Fiber::getCurrentFiberId();
        TimeValue idleStart = TimeValue::getTimeOfDay();
        TRACE_LOG(mLogPrefix.c_str() << ": Entering idle with timer id(" << mIdleTimerId << "), fiber id(" << mIdleFiberId << ").");

        IdleResult idleResult = idle();
        mLastIdleLength = TimeValue::getTimeOfDay() - idleStart;
        mIdleTimerId = INVALID_TIMER_ID;

        scheduleNextIdle(idleStart, defaultIdlePeriod, idleResult);
        mIdleFiberId = Fiber::INVALID_FIBER_ID;
    }

} // namespace GameManager
} // namespace Blaze
