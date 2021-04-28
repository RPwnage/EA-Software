#ifndef _CLOUDSAVES_REMOTEPUSH_H
#define _CLOUDSAVES_REMOTEPUSH_H

#include <QString>
#include <QObject>

#include "engine/cloudsaves/LocalStateSnapshot.h"
#include "engine/cloudsaves/RemoteStateSnapshot.h"
#include "engine/cloudsaves/AbstractStateTransfer.h" 
#include "services/plugin/PluginAPI.h"
#include "RemoteSync.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves 
{
	class S3Operation;
	class AuthorizedS3Transfer;

	class ORIGIN_PLUGIN_API RemotePush : public AbstractStateTransfer 
	{
		Q_OBJECT
	public:
		RemotePush(RemoteSync *sync);
		~RemotePush();
		
	signals:
		void succeeded();
		void failed(Origin::Engine::CloudSaves::SyncFailure);
		void progress(qint64 completedBytes, qint64 totalBytes);

	protected slots:
		void commitPush();
		void rollbackPush();
		void finalizePush(AuthorizedS3Transfer *transfer, S3Operation *operation);

	private:
		void run();

		RemoteStateSnapshot *m_targetState;
		ChangeSet *m_changeSet;
	};
}
}
}

#endif
