#ifndef _CLOUDSAVES_CHANGESET_H
#define _CLOUDSAVES_CHANGESET_H

#include <QList>

#include "SyncableFile.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{ 

namespace CloudSaves
{
    // forward declarations
    class StateSnapshot;
    
    struct LocalInstanceChange
    {
        LocalInstanceChange(const SyncableFile &newFrom, const SyncableFile &newTo) :
            from(newFrom), to(newTo)
        {
        }
        
        SyncableFile from;
        SyncableFile to;
    };

    class ORIGIN_PLUGIN_API ChangeSet
    {
    public:
        ChangeSet(const StateSnapshot &from, const StateSnapshot &to);
        explicit ChangeSet(const StateSnapshot &to);

        bool isEmpty() const;

        // Added the "to" set
        const QList<SyncableFile> &added() const
        {
            return m_added;
        }

        // Removed from the "to" set
        const QList<SyncableFile> &deleted() const
        {
            return m_deleted;
        }

        // File exists on both but the local paths changed
        const QList<LocalInstanceChange> &localInstancesChanged() const
        {
            return m_localInstancesChanged;
        }

    protected:
        QList<SyncableFile> m_added;
        QList<SyncableFile> m_deleted;
        QList<LocalInstanceChange> m_localInstancesChanged;
        QStringList m_addedFileNames;
        QStringList m_deletedFileNames;
    };
}

}
}

#endif
