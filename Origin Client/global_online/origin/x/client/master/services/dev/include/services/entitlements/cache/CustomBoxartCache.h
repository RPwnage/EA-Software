#ifndef CUSTOMBOXARTCACHE_H
#define CUSTOMBOXARTCACHE_H

#include <QString>

#include "services/session/AbstractSession.h"
#include "services/entitlements/BoxartData.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Services
{

namespace Entitlements
{

namespace Cache
{

class ORIGIN_PLUGIN_API CustomBoxartCache : public QObject

{
    Q_OBJECT

public:

    static void init();
    static void release();
    static CustomBoxartCache* instance();

    static bool isDirty();
    static void resetDirtyBit(bool isDirty);

    QString getCacheDirectory(Origin::Services::Session::SessionRef session);

    bool saveCache(Origin::Services::Session::SessionRef session, const QMap<QString, Services::Entitlements::Parser::BoxartData>& productIdToBoxartMap);
    bool readCache(Origin::Services::Session::SessionRef session, QMap<QString, Services::Entitlements::Parser::BoxartData>& productIdToBoxartMap);

public slots:


private:
    CustomBoxartCache();
    virtual ~CustomBoxartCache();

    static bool sIsDirty;

    static const QString sCacheFileName;
    static const QString sCacheVersion;

    QString getCacheFilePath(Origin::Services::Session::SessionRef session);

};

}

}

}

}

#endif // CUSTOMBOXARTCACHE_H
