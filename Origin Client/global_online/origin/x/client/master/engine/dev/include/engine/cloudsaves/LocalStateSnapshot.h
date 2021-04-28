#ifndef _CLOUDSAVES_LOCALSTATESNAPSHOT_H
#define _CLOUDSAVES_LOCALSTATESNAPSHOT_H

#include <limits>
#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QMutex>

#include "FileFingerprint.h"
#include "LocalInstance.h"
#include "StateSnapshot.h"

#include "services/plugin/PluginAPI.h"

class QDataStream;

namespace Origin
{
namespace Engine
{ 
namespace CloudSaves
{
    class RemoteStateSnapshot;

    // This is thread safe for adding files concurrently but nothing else
    class ORIGIN_PLUGIN_API LocalStateSnapshot : public StateSnapshot
    {
    public:
        LocalStateSnapshot(const QUuid &lineage = QUuid());
        
        // Creates us from a remote snapshot under the assumption that a successful
        // sync has completed
        LocalStateSnapshot(const RemoteStateSnapshot &);

        // Tries to add a fingerprint from another local state snapshot if the
        // file hasn't changed in the meantime
        bool importLocalFile(const QFileInfo &info, LocalInstance::FileTrust trust, const LocalStateSnapshot &);

        // Requires an open file to prevent races with the file at the path target
        // changing
        void addLocalFile(const QFile &file, LocalInstance::FileTrust trust, const FileFingerprint &fingerprint);

        bool load(QIODevice *dev);
        bool load(QDataStream &loadStream);

        bool save(QIODevice *dev) const;
        bool save(QDataStream &saveStream) const;

        QDateTime lastModified() const;

        const QUuid &lineage() const
        {
            return m_lineage;
        }

    protected:
        void addLocalInstance(const LocalInstance &instance, const FileFingerprint &fingerprint, const QByteArray &instanceId);

        struct CacheEntry
        {
            CacheEntry()
            {
            }

            CacheEntry(const LocalInstance &newInstance, const FileFingerprint &newFingerprint, const QByteArray &newInstanceId) :
                instance(newInstance),
                fingerprint(newFingerprint),
                instanceId(newInstanceId)
            {
            }

            LocalInstance instance;
            FileFingerprint fingerprint;
            QByteArray instanceId;
        };

        // Keyed by the path 
        typedef QHash<QString, CacheEntry> MD5Cache;
        MD5Cache m_md5Cache;

        // Makes adding files thread safe
        QMutex m_addFileMutex;

        QUuid m_lineage;
    };
}
}
}

#endif
