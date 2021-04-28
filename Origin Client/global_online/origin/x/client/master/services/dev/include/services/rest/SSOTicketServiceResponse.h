///////////////////////////////////////////////////////////////////////////////
// SSOTicketServiceResponse.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SSOTICKETSERVICERESPONSE_H_INCLUDED_
#define _SSOTICKETSERVICERESPONSE_H_INCLUDED_

#include <limits>
#include <QDateTime>

#include "OriginAuthServiceResponse.h"
#include "OriginServiceMaps.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        struct TicketInfo
        {
            ///
            /// SSO Ticket container
            ///
            QString ticket;

            ///
            /// expiry UTC date for SSO Ticket container
            ///
            QDateTime expiry;
        };


        class ORIGIN_PLUGIN_API SSOTicketServiceResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit SSOTicketServiceResponse(AuthNetworkRequest, const QString &ticketElement);

            /// 
            /// Returns the SSOTicket info data
            /// 
            const TicketInfo ticketInfo() const { return mTicketInfo;}

        signals:
            ///
            /// signal emitted on success containing the SSO Ticket
            ///
            void success(QString);

        protected:
            bool parseSuccessBody(QIODevice *body);
            ///
            /// SSO Ticket data container
            ///
            TicketInfo mTicketInfo;

            QString mTicketElementName;
        };
    }
}

#endif


