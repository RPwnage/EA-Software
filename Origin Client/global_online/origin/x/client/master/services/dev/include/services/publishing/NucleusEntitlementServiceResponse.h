///////////////////////////////////////////////////////////////////////////////
// EntitlementServiceResponse.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _BaseGameServiceResponse_H_INCLUDED_
#define _BaseGameServiceResponse_H_INCLUDED_

#include "services/rest/OriginAuthServiceResponse.h"
#include "services/rest/OriginServiceMaps.h"
#include "services/publishing/NucleusEntitlement.h"

#include <QList>

namespace Origin
{

namespace Services
{

namespace Publishing
{
    
enum EntitlementRefreshStatus
{
    EntitlementRefreshStatusUnknownError = -1,  ///< Unknown error
	EntitlementRefreshStatusOK = 0,             ///< No error
    EntitlementRefreshStatusSignatureInvalid,   ///< Signature validation failed
    EntitlementRefreshStatusBadPayload,         ///< Server response payload could not be parsed
    EntitlementRefreshStatusServerError,        ///< Server reported an error, check QNetworkReply* object for details
};

enum EntitlementRefreshType 
{
    EntitlementRefreshTypeFull,
    EntitlementRefreshTypeIncremental,
    EntitlementRefreshTypeSingle,
    EntitlementRefreshTypeExtraContent,
    EntitlementRefreshTypeSDK,
};

class NucleusEntitlementServiceResponse : public OriginAuthServiceResponse
{
public:
    explicit NucleusEntitlementServiceResponse(AuthNetworkRequest reply, EntitlementRefreshType refreshType, const QString& masterTitleId = "");

    QList<NucleusEntitlementRef> const& entitlements() const { return m_entitlements;}
    QDateTime const& currentServerTime() const { return m_currentServerTime; }

    EntitlementRefreshType refreshType() const { return m_refreshType; }
    EntitlementRefreshStatus refreshStatus() const { return m_status; }

    const QString& extraContentRequestedMasterTitleId() const { return m_masterTitleId; }

    const QByteArray &responseBody() const { return m_responseBody; }

protected:
    virtual bool parseSuccessBody(QIODevice*);
    virtual void processReply();

private:
    QList<NucleusEntitlementRef> m_entitlements;
    QDateTime m_currentServerTime;
    EntitlementRefreshType m_refreshType;
    QString m_masterTitleId;
    EntitlementRefreshStatus m_status;
    QByteArray m_responseBody;
};

}

}

}

#endif