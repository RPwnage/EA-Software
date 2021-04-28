//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Rasmus Storjohann

#ifndef TRUSTED_CLOCK_H
#define TRUSTED_CLOCK_H

#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QReadWriteLock>
#include <QNetworkReply>

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Services
{

/**
 * Return the current time from a source that the user can not easily tamper with. This
 * relies on a server-generated time stamp sent to the client and a monotone timer in 
 * the client. The clock is guaranteed to be accurate but may not be precise, i.e. it may
 * be off by a couple of seconds.
 *
 * This implementation assumes that QElapsedTimer does not overflow, which is supported by
 * the documentation but needs to be verified.
 *
 * To ensure robust operation, the clock always returns a valid time, even when not
 * initialized. Calling code should check and log (using telemetry) when the clock has not
 * been initialized, but should proceed to use the returned time anyway.
 */
class ORIGIN_PLUGIN_API TrustedClock : public QObject
{
    Q_OBJECT
public:
    static TrustedClock* instance();

    /// Initialize from server time returned in entitlements response.  Telemetry is collected if time is invalid.
    void initialize(const QDateTime& serverDateTime);

    /// Return true if the time has been set and is valid. If false, the time
    /// returned is vulnerable to user tampering.
    bool isInitialized() const;

    /// Return the current time. If the clock has not been initialized, it falls back
    /// on system time.
    QDateTime nowUTC() const;

    /// Return the elapsed timer used to implement this clock.
    QElapsedTimer const& timer() const { return mElapsedTimer; }

signals:
    void reset();

private:
    /// Creates a fully functional but untrusted clock instance
    TrustedClock();
    static TrustedClock* sInstance;

    QReadWriteLock mutable mLock;
    QDateTime mBaseUTC;
    QElapsedTimer mElapsedTimer;
    bool mInitialized;
};

}

}

#endif // TRUSTED_CLOCK_H
