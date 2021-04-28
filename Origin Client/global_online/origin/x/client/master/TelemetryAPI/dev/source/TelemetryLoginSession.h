///////////////////////////////////////////////////////////////////////////////
// TelemetryLoginSession.h
//
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef TELEMETRYLOGINSESSION_H
#define TELEMETRYLOGINSESSION_H

#include "EASTL/string.h"
#include "eathread/eathread_futex.h"
#include "eathread/eathread_thread.h"

class TelemetryLoginSession
{

    public:	
        TelemetryLoginSession();
        ~TelemetryLoginSession();

        void setStartedByLocalHost(bool startedByLocalHost);
        void setLocalHostOriginUri(const char8_t *uri); 

        const bool startedByLocalHost() const;
        const char8_t *localHostOriginUri() const;

        void reset ();

        EA::Thread::Futex& GetLock();

    private:
        bool m_startedByLocalHost;
        eastl::string8 m_localHostOriginUri;
        EA::Thread::Futex *m_lock;

};

#endif //TELEMETRYLOGINSESSION_H
