#ifndef _CLOUDSAVES_LOCALSTATECALCULATOR_H
#define _CLOUDSAVES_LOCALSTATECALCULATOR_H

#include <QString>
#include <QObject>
#include <QThread>

#include "LocalStateSnapshot.h"
#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{ 
namespace CloudSaves
{
    class ORIGIN_PLUGIN_API LocalStateCalculator : public QThread
    {
        Q_OBJECT
    public:
        LocalStateCalculator(Content::EntitlementRef entitlement, const LocalStateSnapshot &previousSync)
        : mEntitlement(entitlement)
        , mPreviousSync(previousSync)
        , mCalculatedState(NULL)
        {
            setObjectName("LocalStateCalculator Thread");
            moveToThread(this);
        }

    signals:
        void finished(Origin::Engine::CloudSaves::LocalStateSnapshot *snapshot);

    private:
        void run();
        bool fingerprintAndAddPath(const QString &path, LocalInstance::FileTrust trust);

        Content::EntitlementRef mEntitlement;
        const LocalStateSnapshot &mPreviousSync;
        LocalStateSnapshot *mCalculatedState;
    };

    class ORIGIN_PLUGIN_API LocalAutoreleasePool
    {
    public:
    
        LocalAutoreleasePool();
        ~LocalAutoreleasePool();
        
    private:
    
        void* pool;
    };

}
}
}

#endif
