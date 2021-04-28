/*! ************************************************************************************************/
/*!
    \file idlerutil.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_IDLER_UTIL_H
#define BLAZE_GAMEMANAGER_IDLER_UTIL_H

#include "framework/system/fiber.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief Util for implementing a periodic idle loop
    ***************************************************************************************************/
    class IdlerUtil
    {
    public:
        typedef enum IdleResult
        {
            NO_NEW_IDLE,
            SCHEDULE_IDLE_IMMEDIATELY,
            SCHEDULE_IDLE_NORMALLY
        } IdleResult;

        void scheduleNextIdle(const TimeValue& offsetStartTime, const TimeValue& defaultIdlePeriod, IdleResult idleResult = SCHEDULE_IDLE_NORMALLY);

    protected:
        IdlerUtil(const char8_t* logPrefix);
        virtual ~IdlerUtil();
        void idleTimerCallback(TimeValue defaultIdlePeriod);
        void shutdownIdler();

        /*! ************************************************************************************************/
        /*! \brief implements the idle
            \return SCHEDULE_IDLE_IMMEDIATELY to schedule next idle immediately, NO_NEW_IDLE to not schedule, SCHEDULE_IDLE_NORMALLY uses default period
        ***************************************************************************************************/
        virtual IdleResult idle() = 0;

    private:
        TimerId mIdleTimerId;
        Fiber::FiberId mIdleFiberId; // used to track last idle fiber, and check whether it hit exception/crashed
        TimeValue mLastIdleLength;
        eastl::string mLogPrefix;
    };

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_IDLER_UTIL_H
