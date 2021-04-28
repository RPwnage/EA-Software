#include "engine/cloudsaves/StateSnapshot.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    StateSnapshot::StateSnapshot() : 
        m_valid(false)
    {
    }
        
    qint64 StateSnapshot::localSize() const
    {
        qint64 totalSize = 0;

        for(QHash<FileFingerprint, SyncableFile>::const_iterator it = m_fileHash.constBegin();
            it != m_fileHash.constEnd();
            it++)
        {
            totalSize += it.value().localSize();
        }

        return totalSize;
    }

    qint64 StateSnapshot::remoteSize() const
    {
        qint64 totalSize = 0;

        for(QHash<FileFingerprint, SyncableFile>::const_iterator it = m_fileHash.constBegin();
            it != m_fileHash.constEnd();
            it++)
        {
            totalSize += it.value().remoteSize();
        }

        return totalSize;
    }
}
}
}
