#ifndef _CLOUDSAVES_USAGEQUERY_H
#define _CLOUDSAVES_USAGEQUERY_H

#include <QObject>
#include <QString>
#include <QByteArray>

#include "engine/login/LoginController.h"
#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class CloudSyncServiceClient;
    }
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    class ORIGIN_PLUGIN_API UsageQuery : public QObject
    {
        Q_OBJECT
    public:
        UsageQuery(Content::EntitlementRef entitlement, const QByteArray &lastEtag = QByteArray());
        ~UsageQuery();

        Content::EntitlementRef entitlement() const
        {
            return mEntitlement;
        }
    
    signals:
        void succeeded(qint64 localSize, QByteArray etag);
        void unchanged();
        void failed();

    public slots:
        void start();

    protected slots:
        virtual void manifestRequestFinished();
        virtual void manifestDiscoverySucceeded();

    private:
        Content::EntitlementRef mEntitlement;
        QByteArray mLastEtag;
    };
}
}
}

#endif
