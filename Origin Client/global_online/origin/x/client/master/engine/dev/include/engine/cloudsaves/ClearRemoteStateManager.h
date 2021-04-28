#ifndef _CLOUDSAVES_CLEARREMOTESTATEMANAGER_H
#define _CLOUDSAVES_CLEARREMOTESTATEMANAGER_H

#include <QObject>
#include <QString>

#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    class ORIGIN_PLUGIN_API ClearRemoteStateManager : public QObject
    {
        Q_OBJECT
    public:
        static ClearRemoteStateManager *instance();
        void clearRemoteStateForEntitlement(Content::EntitlementRef entitlement, const bool& clearOldStoragePath = false);

    private:
        ClearRemoteStateManager(){}
        ClearRemoteStateManager(const ClearRemoteStateManager &);
        ClearRemoteStateManager & operator=(const ClearRemoteStateManager &);

    protected slots:
        void succeeded();
    };
}
}
}

#endif
