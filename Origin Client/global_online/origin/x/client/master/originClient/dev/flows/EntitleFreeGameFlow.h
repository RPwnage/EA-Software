///////////////////////////////////////////////////////////////////////////////
// EntitleFreeGameFlow.h
//
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ENTITLEFREEGAMEFLOW_H
#define ENTITLEFREEGAMEFLOW_H

#include "flows/AbstractFlow.h"
#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

class QNetworkReply;

namespace Origin
{
namespace Client
{
class ORIGIN_PLUGIN_API EntitleFreeGameFlow : public AbstractFlow
{
    Q_OBJECT

public:
    EntitleFreeGameFlow(const Engine::Content::EntitlementRef& entitlement);

    virtual void start();

signals:
    void finished(EntitleFreeGameFlowResult);

private slots:
    void onResponse();

private:
    void makeRequest(const QUrl& url);

    const Engine::Content::EntitlementRef mEntitlement;
    QNetworkReply *mReply;
    quint8 mRetryCount;

    const quint8 MAX_RETRIES = 3;
};

} // Client
} // Origin

#endif // ENTITLEFREEGAMEFLOW_H
