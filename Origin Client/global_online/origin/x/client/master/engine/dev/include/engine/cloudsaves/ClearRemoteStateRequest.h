#ifndef _CLOUDSAVES_CLEARREMOTESTATEREQUEST_H
#define _CLOUDSAVES_CLEARREMOTESTATEREQUEST_H

#include <QObject>

#include "RemoteSyncSession.h"
#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    class ORIGIN_PLUGIN_API ClearRemoteStateRequest : public QObject
    {
        Q_OBJECT
    public:
        ClearRemoteStateRequest(Content::EntitlementRef entitlement, const bool& clearOldStoragePath);
        ~ClearRemoteStateRequest();

        Content::EntitlementRef entitlement() const { return m_entitlement; }

    signals:
        void succeeded();
        void failed();

    public slots:
        void start();

    protected slots:
        void remoteStateDiscovered(Origin::Engine::CloudSaves::RemoteStateSnapshot *remote);

    private:
        RemoteSyncSession *m_syncSession;
        Content::EntitlementRef m_entitlement;
    };
}
}
}

#endif
