#ifndef NONORIGINCONTENTCACHE_H
#define NONORIGINCONTENTCACHE_H

#include <QString>
#include <QVector>
#include <QByteArray>

#include "services/session/AbstractSession.h"
#include "engine/content/NonOriginGameData.h"

namespace Origin
{

namespace Engine
{

namespace Content
{

namespace Cache
{

class NonOriginContentCache : public QObject

{
    Q_OBJECT

public:

    ///
    /// Create the instance
    ///
    static void init();

    ///
    /// Delete the instance
    ///
    static void release();

    ///
    /// Return the instance
    ///
    static NonOriginContentCache* instance();

    static bool isDirty();
    static void resetDirtyBit(bool isDirty);

    QString getCacheDirectory(Origin::Services::Session::SessionRef session);

    bool saveCache(Origin::Services::Session::SessionRef session, const QMap<QString, Engine::Content::NonOriginGameData>& productIdToNogMap);
    bool readCache(Origin::Services::Session::SessionRef session, QMap<QString, Engine::Content::NonOriginGameData>& productIdToNogMap);
    bool readLegacyCache(Origin::Services::Session::SessionRef session, QVector<QByteArray>& cache);
    bool removeLegacyCache(Origin::Services::Session::SessionRef session);

public slots:


private:
    NonOriginContentCache();
    virtual ~NonOriginContentCache();

    static bool sIsDirty;

    static const QString sCacheFileName;
    static const QString sCacheVersion;

    QString getCacheFilePath(Origin::Services::Session::SessionRef session);

};

}

}

}

}

#endif // NONORIGINCONTENTCACHE_H
