#ifndef _CLOUDSAVES_LASTSYNCSTORE_H
#define _CLOUDSAVES_LASTSYNCSTORE_H

#include <QString>
#include <QDir>
#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"
#include "LocalStateSnapshot.h"

namespace Origin
{
namespace Engine
{ 
namespace CloudSaves
{
    namespace LastSyncStore
    {
        enum LocalStateType { SyncState, ClientState };

        ORIGIN_PLUGIN_API LocalStateSnapshot *localStateForCloudSavesId(Origin::Engine::Content::EntitlementRef entitlement, LocalStateType localStateType, bool *ok = NULL);
        ORIGIN_PLUGIN_API bool setLocalStateForCloudSavesId(const QString &cloudSavesId, LocalStateType localStateType, const LocalStateSnapshot &toSave);

        ORIGIN_PLUGIN_API LocalStateSnapshot *lastSyncForCloudSavesId(Origin::Engine::Content::EntitlementRef entitlement);
        ORIGIN_PLUGIN_API bool setLastSyncForCloudSavesId(const QString &cloudSavesId, const LocalStateSnapshot &);
    }
}
}
}

#endif
