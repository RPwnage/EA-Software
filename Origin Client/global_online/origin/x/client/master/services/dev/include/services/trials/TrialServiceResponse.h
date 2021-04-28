///////////////////////////////////////////////////////////////////////////////
// TrialServiceResponse.h
//
// Author: Hans van Veenendaal
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __TRIALSERVICERESPONSE_H__
#define __TRIALSERVICERESPONSE_H__

#include "services/rest/OriginAuthServiceResponse.h"
#include "services/rest/OriginServiceMaps.h"
#include "services/plugin/PluginAPI.h"

#include <QVector>
#include <QHash>
#include <server.h>

namespace Origin
{
    namespace Services
    {
        namespace Trials
        {
            class ORIGIN_PLUGIN_API TrialBurnTimeResponse : public OriginAuthServiceResponse
            {
            public:
                explicit TrialBurnTimeResponse(AuthNetworkRequest);

                bool hasTimeLeft() { return m_burnTime.burnTimeResponse.hasTimeLeft; }
                int timeSlice() { return m_burnTime.burnTimeResponse.intervalSec; }
                int timeRemaining() { return m_burnTime.burnTimeResponse.leftTrialSec; }
                int timeInitial() { return m_burnTime.burnTimeResponse.totalTrialSec; }
                QString timeTicket() { return m_burnTime.burnTimeResponse.timeTicket; }
                QDateTime timeStamp() { return m_timeStamp; }

            protected:
                bool parseSuccessBody(QIODevice*);

            private:
                server::burnTimeT m_burnTime;
                QDateTime m_timeStamp;
            };

            class ORIGIN_PLUGIN_API TrialCheckTimeResponse : public OriginAuthServiceResponse
            {
            public:
                explicit TrialCheckTimeResponse(AuthNetworkRequest);

                bool hasTimeLeft() { return m_checkTime.checkTimeResponse.hasTimeLeft; }
                int timeRemaining() { return m_checkTime.checkTimeResponse.leftTrialSec; }
                int timeInitial() { return m_checkTime.checkTimeResponse.totalTrialSec; }
                QDateTime timeStamp() { return m_timeStamp; }

            protected:
                bool parseSuccessBody(QIODevice*);

            private:
                server::checkTimeT m_checkTime;
                QDateTime m_timeStamp;
            };

            class ORIGIN_PLUGIN_API TrialGrantTimeResponse : public OriginAuthServiceResponse
            {
            public:
                explicit TrialGrantTimeResponse(AuthNetworkRequest);
                bool hasTimeLeft() { return m_grantTime.grantTimeResponse.hasTimeLeft; }
                int timeRemaining() { return m_grantTime.grantTimeResponse.leftTrialSec; }
                int timeInitial() { return m_grantTime.grantTimeResponse.totalTrialSec; }
                int grantedTime() { return m_grantTime.grantTimeResponse.grantedSec; }
                int totalGranted() { return m_grantTime.grantTimeResponse.totalGrantedSec; }
                QDateTime timeStamp() { return m_timeStamp; }

            protected:
                bool parseSuccessBody(QIODevice*);

            private:
                server::grantTimeT m_grantTime;
                QDateTime m_timeStamp;
            };
        }
    }
}

#endif //__TRIALSERVICERESPONSE_H__
