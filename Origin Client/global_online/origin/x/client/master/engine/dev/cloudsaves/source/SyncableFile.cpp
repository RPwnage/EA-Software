#include "engine/cloudsaves/SyncableFile.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    SyncableFile::SyncableFile(const FileFingerprint &fingerprint, const LocalInstances &localInstances, const QString &remotePath)
        : m_localInstances(localInstances)
        , m_fingerprint(fingerprint)
        , m_null(false)
    {
        if (remotePath.isNull())
        {
            // Build the remote path based on the size and hash of the file
            m_remotePath = "content/" + 
                           QString::number(fingerprint.size()) + "-" +
                           fingerprint.md5().toHex();
        }
        else
        {
            m_remotePath = remotePath;
        }
    }    


    void SyncableFile::addLocalInstance(const LocalInstance &instance)
    {
        m_localInstances << instance;
    }
}
}
}
