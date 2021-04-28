#include "engine/cloudsaves/LocalStateSnapshot.h"
#include "engine/cloudsaves/LastSyncStore.h"
#include "engine/cloudsaves/FilesystemSupport.h"
#include "services/debug/DebugService.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves 
{
    namespace LastSyncStore
    {
        static QString localStateFilePath(const QString &cloudSavesId, LocalStateType localStateType)
        {
            using namespace FilesystemSupport;

            QDir cloudSavesRoot = cloudSavesDirectory(LocalRoot);

            QString fileName = cloudSavesId;
            switch(localStateType)
            {
                case SyncState:
                    fileName += ".lastsync";
                    break;
                case ClientState:
                    fileName += ".client";
                    break;
                default:
                    ORIGIN_ASSERT(0);
                    fileName += ".lastsync";
                    break;
            }
            return cloudSavesRoot.absoluteFilePath(fileName);
        }

        LocalStateSnapshot *localStateForCloudSavesId(Origin::Engine::Content::EntitlementRef entitlement, LocalStateType localStateType, bool *ok)
        {
            QStringList cloudSavesIdsToCheck;
            
            //add the main cloudSavesId to the list
            cloudSavesIdsToCheck << entitlement->contentConfiguration()->cloudSavesId();

            //see if we have any fall back ids, if so add them to the list
            if(entitlement->contentConfiguration()->cloudSaveContentIDFallback().size())
            {
                cloudSavesIdsToCheck = cloudSavesIdsToCheck + entitlement->contentConfiguration()->cloudSaveContentIDFallback();
            }

            LocalStateSnapshot *loadedSnapshot = new LocalStateSnapshot();

            if (ok)
            {
                *ok = false;
            }

            //loop through the list of cloudSavesIds to check
            for (QStringList::const_iterator it = cloudSavesIdsToCheck.constBegin(); it != cloudSavesIdsToCheck.constEnd(); it++)
            {
                QString cloudSavesIdToCheck = *it;
                QFile localState(localStateFilePath(cloudSavesIdToCheck, localStateType));
                
                if (localState.open(QIODevice::ReadOnly))
                {
                    bool result = loadedSnapshot->load(&localState);
                    localState.close();

                    // If we can load one, break out use that local state
                    if (result)
                    {
                        if (ok)
                        {
                            *ok = true;
                        }

                        break;
                    }
                }
            }

            return loadedSnapshot;
        }

        bool setLocalStateForCloudSavesId(const QString &cloudSavesId, LocalStateType localStateType, const LocalStateSnapshot &toSave)
        {
            QFile localState(localStateFilePath(cloudSavesId, localStateType));

            localState.open(QIODevice::WriteOnly | QIODevice::Truncate);

            return toSave.save(&localState);
        }
        
        LocalStateSnapshot *lastSyncForCloudSavesId(Origin::Engine::Content::EntitlementRef entitlement)
        {
            return localStateForCloudSavesId(entitlement, SyncState);
        }

        bool setLastSyncForCloudSavesId(const QString &cloudSavesId, const LocalStateSnapshot &toSave) 
        {
            return setLocalStateForCloudSavesId(cloudSavesId, SyncState, toSave);
        }
    }
}
}
}
