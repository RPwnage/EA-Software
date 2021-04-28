///////////////////////////////////////////////////////////////////////////////
// SSOTicketServiceResponse.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>
#include "services/rest/SSOTicketServiceResponse.h"

namespace
{
    using namespace Origin::Services;

    bool parseXmlPayload(const QString &ticketElementName, QIODevice *body, TicketInfo& ticketInfo)
    {
        QDomDocument doc;

        if (!doc.setContent(body))
        {
            // Not valid XML
            return false;
        }

        QDomElement tokenElement = doc.documentElement();
        if (tokenElement.isNull() || (tokenElement.tagName() != "token"))
        {
            // Invalid XML document
            return false;
        }

        for (QDomElement e = tokenElement.firstChildElement(); 
            !e.isNull(); 
            e = e.nextSiblingElement())
        {
            if (e.tagName() == ticketElementName)
            {
                ticketInfo.ticket = e.text();
            }
            else if (e.tagName() == "expiry")
            {
                ticketInfo.expiry.setMSecsSinceEpoch(e.text().toULongLong());
            }
        }

        return true;
    }
}


namespace Origin
{
    namespace Services
    {
        SSOTicketServiceResponse::SSOTicketServiceResponse(AuthNetworkRequest reply, const QString &ticketElementName) : 
            OriginAuthServiceResponse(reply),
            mTicketElementName(ticketElementName)
        {
        }

        bool SSOTicketServiceResponse::parseSuccessBody(QIODevice *body)
        {
            bool res = parseXmlPayload(mTicketElementName, body, mTicketInfo);
            if (res)
            {
                emit(success(mTicketInfo.ticket));
            }
            // any error state will be signaled by the base class.

            return res;
        }
    }
}
