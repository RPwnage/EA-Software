#ifndef _CLOUDSAVES_ABSTRACTSTATETRANSFER_H
#define _CLOUDSAVES_ABSTRACTSTATETRANSFER_H

#include <QObject>
#include <QThread>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{ 
namespace CloudSaves
{
    class ChangeSet;
    class RemoteSync;

    class ORIGIN_PLUGIN_API AbstractStateTransfer : public QThread
    {
    protected:
        AbstractStateTransfer(RemoteSync *sync) : m_sync(sync)
        {
            moveToThread(this);
        }

        RemoteSync *remoteSync() { return m_sync; }

    protected:
        RemoteSync *m_sync;
    };
}
}
}

#endif
