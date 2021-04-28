/*************************************************************************************************/
/*!
    \file alertinfo.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ALERTINFO_H
#define BLAZE_ALERTINFO_H

/*** Include files *******************************************************************************/
#include "EATDF/time.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class AlertInfo
{
public:
    AlertInfo(uint32_t logRateInMs) : mLast(0), mCount(0), mRate(logRateInMs * 1000) { }

    void increment() { ++mCount; }

    bool shouldLog()
    {
        return ((EA::TDF::TimeValue::getTimeOfDay() - mLast) > mRate);
    }

    void reset()
    {
        mCount = 0;
        mLast = EA::TDF::TimeValue::getTimeOfDay();
    }

    uint32_t getCount() const { return mCount; }

private:

    EA::TDF::TimeValue mLast;
    uint32_t mCount;
    EA::TDF::TimeValue mRate;
};

} // Blaze

#endif // BLAZE_ALERTINFO_H
