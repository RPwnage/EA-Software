#ifndef _CLOUDSAVES_STATESNAPSHOT_H
#define _CLOUDSAVES_STATESNAPSHOT_H

#include <QHash>
#include <QByteArray>
#include <QStringList>
#include <QUuid>

#include "SyncableFile.h" 
#include "FileFingerprint.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    class ORIGIN_PLUGIN_API StateSnapshot
    {
    public:
        // Creates an invalid snapshot
        StateSnapshot();

        virtual ~StateSnapshot()
        {
        }

        bool isValid() const { return m_valid; }
        bool isEmpty() const { return m_fileHash.isEmpty(); }
        
        //  Used to set this valid after we build it from scratch
        void setValid(bool valid) 
        {
            m_valid = valid;
        }

        const QHash<FileFingerprint, SyncableFile> &fileHash() const
        {
            return m_fileHash;
        }

        qint64 localSize() const;
        qint64 remoteSize() const;

        virtual const QUuid &lineage() const = 0;

    protected:
    
        typedef QHash<FileFingerprint, SyncableFile> FileHashes;
        FileHashes m_fileHash;
        bool m_valid;
    };
}
}
}

#endif
