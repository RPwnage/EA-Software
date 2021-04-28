#include "engine/cloudsaves/ChangeSet.h"
#include "engine/cloudsaves/StateSnapshot.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    typedef QHash<FileFingerprint, SyncableFile> FileHashes;
    
    ChangeSet::ChangeSet(const StateSnapshot &to)
    {
        const FileHashes &toFiles = to.fileHash();
        
        // Add all of our files to added list
        for(FileHashes::const_iterator it = toFiles.constBegin();
            it != toFiles.constEnd();
            it++)
        {
            m_added << *it;
        }
    }

    ChangeSet::ChangeSet(const StateSnapshot &from, const StateSnapshot &to)
    {
        const FileHashes &fromFiles = from.fileHash();
        const FileHashes &toFiles = to.fileHash();

        // Iterate over files in our "from" state
        for(FileHashes::const_iterator it = fromFiles.constBegin();
            it != fromFiles.constEnd();
            it++)
        {
            const SyncableFile &fromFile = it.value();

            // Look for this in our "to" state 
            FileHashes::const_iterator toFileResult = toFiles.constFind(it.key());

            if (toFileResult != toFiles.constEnd())
            {
                const SyncableFile &toFile = toFileResult.value();

                if (fromFile.localInstances() != toFile.localInstances())
                {
                    // Local paths changed
                    m_localInstancesChanged << LocalInstanceChange(fromFile, toFile);
                }
            }
            else
            {
                // File deleted
                m_deleted << fromFile;
            }
        }

        // Now iterate over "to" to find added files
        for(FileHashes::const_iterator it = toFiles.constBegin();
            it != toFiles.constEnd();
            it++)
        {
            const SyncableFile &toFile = it.value();

            if (!fromFiles.contains(it.key()))
            {
                // File added
                m_added << toFile;
            }
        }
    }
        
    bool ChangeSet::isEmpty() const
    {
        return m_added.isEmpty() &&
               m_deleted.isEmpty() &&
               m_localInstancesChanged.isEmpty();
    }
}
}
}
