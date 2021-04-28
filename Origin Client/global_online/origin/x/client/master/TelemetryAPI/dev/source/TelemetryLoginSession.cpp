///////////////////////////////////////////////////////////////////////////////
// TelemetryLoginSession.cpp
//
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "EASTL/string.h"
#include "eathread/eathread_futex.h"
#include "eathread/eathread_thread.h"
#include "TelemetryLoginSession.h"

TelemetryLoginSession::TelemetryLoginSession()
    :m_startedByLocalHost(false)
    ,m_localHostOriginUri("")
    ,m_lock(NULL)
{
}

TelemetryLoginSession::~TelemetryLoginSession()
{
}

void TelemetryLoginSession::setStartedByLocalHost(bool startedByLocalHost)
{
    m_startedByLocalHost = startedByLocalHost; 
}

void TelemetryLoginSession::setLocalHostOriginUri(const char8_t *uri)
{
    m_localHostOriginUri = uri; 
}

void TelemetryLoginSession::reset ()
{
    m_startedByLocalHost = false;
    m_localHostOriginUri.clear();
}

const bool TelemetryLoginSession::startedByLocalHost() const
{
    return m_startedByLocalHost;
}

const char8_t *TelemetryLoginSession::localHostOriginUri() const
{
    return m_localHostOriginUri.c_str();
}

EA::Thread::Futex& TelemetryLoginSession::GetLock()
{
    if (m_lock == NULL)
        m_lock = new EA::Thread::Futex();

    return *m_lock; 
}
