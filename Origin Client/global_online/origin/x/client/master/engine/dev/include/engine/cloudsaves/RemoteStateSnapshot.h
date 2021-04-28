#ifndef _CLOUDSAVES_REMOTESTATESNAPSHOT_H
#define _CLOUDSAVES_REMOTESTATESNAPSHOT_H

#include "FileFingerprint.h"
#include "LocalInstance.h"
#include "StateSnapshot.h"

#include <QList>
#include <QDomDocument>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    class LocalStateSnapshot;
    class PathSubstituter;

    class ORIGIN_PLUGIN_API RemoteStateSnapshot : public StateSnapshot
    {
    public:
        // Creates us from save manifest XML
        RemoteStateSnapshot(const QDomDocument &, const PathSubstituter &substituter);

        // Creates us from a local snapshot optionally using any remote existing paths from another snapshot
        RemoteStateSnapshot(const LocalStateSnapshot &, const RemoteStateSnapshot *);

        // Creates an empty remote state
        RemoteStateSnapshot();

        QDomDocument toXml(const PathSubstituter &substituter);
    
        const QUuid &lineage() const
        {
            return m_lineage;
        }

    private:
        QUuid m_lineage;
    };
}
}
}

#endif
