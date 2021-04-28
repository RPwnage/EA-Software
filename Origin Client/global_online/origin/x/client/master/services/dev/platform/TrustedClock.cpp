//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Rasmus Storjohann

#include "TrustedClock.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include <QReadLocker>
#include <QWriteLocker>
#include <QLocale>

#include "TelemetryAPIDLL.h"

namespace Origin 
{

namespace Services
{

TrustedClock* TrustedClock::sInstance = NULL;

TrustedClock* TrustedClock::instance()
{
    if ( sInstance == NULL )
    {
        sInstance = new TrustedClock();
    }
    return sInstance;
}

TrustedClock::TrustedClock() 
    : mBaseUTC(QDateTime::currentDateTimeUtc())
    , mInitialized(false) // clock is functional but not initialized since we've used system time
{
    mElapsedTimer.start();
}

void TrustedClock::initialize(const QDateTime& serverDateTime)
{
    bool change = false;
    {
        QWriteLocker lock(&mLock);

        if (serverDateTime.isValid())
        {
            mBaseUTC = serverDateTime;
            mElapsedTimer.start();
            mInitialized = true;
            change = true;     
            ORIGIN_LOG_DEBUG << "Initialized trusted clock to: " << mBaseUTC.toString();
        }
        else
        {
            ORIGIN_LOG_ERROR << "Failed to initialize trusted clock.";
            GetTelemetryInterface()->Metric_ERROR_NOTIFY("TrustedClock", "serverDateTime invalid.");
        }
        // release the lock before emitting signal
    }
    if ( change )
    {
        emit reset();
    }
}

bool TrustedClock::isInitialized() const 
{ 
    QReadLocker lock(&mLock);
    return mInitialized; 
}

QDateTime TrustedClock::nowUTC() const
{
    QReadLocker lock(&mLock);
    ORIGIN_ASSERT(mBaseUTC.timeSpec() == Qt::UTC);

    QDateTime ret = mBaseUTC.addMSecs(mElapsedTimer.elapsed());
    ret.setTimeSpec(Qt::UTC);
    return ret;
}

}

}
