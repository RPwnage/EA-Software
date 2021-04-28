#ifndef _CLOUDSAVES_SYNCABLEFILE_H
#define _CLOUDSAVES_SYNCABLEFILE_H

#include "FileFingerprint.h"
#include "LocalInstance.h"

#include "services/plugin/PluginAPI.h"

#include <QByteArray>
#include <QStringList>
#include <QSet>

namespace Origin
{
namespace Engine
{
namespace CloudSaves 
{
    typedef QSet<LocalInstance> LocalInstances;
    
    class ORIGIN_PLUGIN_API SyncableFile
    {
    public:
        SyncableFile(const FileFingerprint &fingerprint, const LocalInstances &localIntances, const QString &remotePath = QString());
        
        // Null constructor
        SyncableFile() : m_null(true)
        {
        }

        QString remotePath() const { return m_remotePath; }

        void addLocalInstance(const LocalInstance &instance);
        
        const LocalInstances &localInstances() const { return m_localInstances; }

        const FileFingerprint &fingerprint() const { return m_fingerprint; }

        bool isNull() const { return m_null; }
        
        // localSize = remoteSize * number of local copies 
        qint64 localSize() const { return m_fingerprint.size() * m_localInstances.size(); }
        qint64 remoteSize() const { return m_fingerprint.size(); }

    protected:

        QString m_remotePath;
        LocalInstances m_localInstances;
        FileFingerprint m_fingerprint;
        
        bool m_null;
    };
}
}
}

#endif
