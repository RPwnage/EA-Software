#ifndef REMOTESYNCVIEW_H
#define REMOTESYNCVIEW_H

#include <QObject>
#include "engine/cloudsaves/RemoteSync.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
    class ORIGIN_PLUGIN_API RemoteSyncViewController : public QObject
    {
        Q_OBJECT
    public:

        /// \brief had to create CTOR to initialize member var
        RemoteSyncViewController();

        void untrustedRemoteFilesDetected();
        void syncConflictDetected(const QDateTime &localLastModified, const QDateTime &remoteLastModified);
        void aboutToDeleteSync();

        /// brief cancels current operation
        void cancelOperation();


    private slots:
        void useRemoteState();
        void useLocalState();
        void cancelSync();

    private:
        bool mIsCancelled; /// set to true to cancel op.

    };
}
}

#endif
