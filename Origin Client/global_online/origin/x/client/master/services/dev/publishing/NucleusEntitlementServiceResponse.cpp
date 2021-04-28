///////////////////////////////////////////////////////////////////////////////
// EntitlementServiceResponse.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/publishing/NucleusEntitlementServiceResponse.h"
#include "services/publishing/SignatureVerifier.h"
#include "services/common/XmlUtil.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include <QDomDocument>
#include <QDomElement>

namespace Origin
{

namespace Services
{

namespace Publishing
{

const QByteArray DATE_HEADER("Date");
const QByteArray CURRENT_TIME_HEADER("X-Origin-CurrentTime");
const QByteArray INCREMENTAL_RESPONSE_HEADER("X-Origin-Entitlements-Modified-Since");

NucleusEntitlementServiceResponse::NucleusEntitlementServiceResponse(AuthNetworkRequest reply, EntitlementRefreshType refreshType, const QString& masterTitleId)
    : OriginAuthServiceResponse(reply)
    , m_refreshType(refreshType)
    , m_masterTitleId(masterTitleId)
    , m_status(EntitlementRefreshStatusUnknownError)
{
}

bool NucleusEntitlementServiceResponse::parseSuccessBody(QIODevice *body)
{
    // Prioritize on standard 'Date' header
    const QByteArray &dateHeader = reply()->rawHeader(DATE_HEADER);
    if (!dateHeader.isEmpty())
    {
        m_currentServerTime = QDateTime::fromString(dateHeader, Qt::RFC2822Date);
    }
    else
    {
        m_currentServerTime = QDateTime();
    }

    // Fall back to old 'X-Origin-CurrentTime' header
    if (!m_currentServerTime.isValid() && reply()->hasRawHeader(CURRENT_TIME_HEADER))
    {
        bool parsedOkay = false;
        qint64 xOriginCurrentTime = reply()->rawHeader(CURRENT_TIME_HEADER).toLongLong(&parsedOkay);

        if (parsedOkay)
        {
            m_currentServerTime = QDateTime::fromMSecsSinceEpoch(xOriginCurrentTime * 1000).toUTC();
        }
    }

    if (reply()->hasRawHeader(INCREMENTAL_RESPONSE_HEADER))
    {
        m_refreshType = EntitlementRefreshTypeIncremental;
    }

    m_responseBody = body->readAll();

    const QByteArray &key = PlatformService::machineHashAsString().toUtf8();
    const QByteArray &signature = QByteArray::fromBase64(reply()->rawHeader(SignatureVerifier::SIGNATURE_HEADER));
    if (!SignatureVerifier::verify(key, m_responseBody, signature))
    {
        m_status = EntitlementRefreshStatusSignatureInvalid;
        return false;
    }

    if(!NucleusEntitlement::parseJson(m_responseBody, m_entitlements))
    {
        m_status = EntitlementRefreshStatusBadPayload;
        return false;
    }

    m_status = EntitlementRefreshStatusOK;
    return true;
}

void NucleusEntitlementServiceResponse::processReply()
{
    if(reply()->error() != QNetworkReply::NoError)
    {
        m_status = EntitlementRefreshStatusServerError;
    }

    OriginAuthServiceResponse::processReply();
}

}

}

}
