#ifndef _ENTITLEMENTREMOTESYNCPROXY_H
#define _ENTITLEMENTREMOTESYNCPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>

#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"
 
namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    /// \brief Provides users with the ability to store and access their game-generated data (such as saves,
    /// key bindings and other user configurations) from any Origin-equipped machine. Currently all code is
    /// in packages, so it is not documented further at this time.
	class RemoteSync;
}
}
}

namespace Origin
{
namespace Client
{
namespace JsInterface
{

// Duck typed with EntitlementServerOperationProxy
class ORIGIN_PLUGIN_API EntitlementRemoteSyncProxy : public QObject
{
	Q_OBJECT
public:
	explicit EntitlementRemoteSyncProxy(QObject *parent, Engine::Content::EntitlementRef entitlement);

	// These are stubs for duck typing
	Q_INVOKABLE void pause();
	Q_INVOKABLE void resume();

	// Cancels the current sync
	Q_INVOKABLE void cancel();

	// Returns the progress of the sync
	Q_PROPERTY(float progress READ progress)
	float progress();

	Q_PROPERTY(QString phase READ phase);
	QString phase();

	Q_PROPERTY(bool pausable READ pausable);
    bool pausable() { return false; }
	
    Q_PROPERTY(bool resumable READ resumable);
    bool resumable() { return false; }

signals:
	void changed();

private slots:
	void remoteSyncProgressed(float progress, qint64, qint64);
	void remoteSyncCreated(Origin::Engine::CloudSaves::RemoteSync *);

private:
	Engine::CloudSaves::RemoteSync* remoteSync();

	float mProgress;
	Engine::Content::EntitlementRef mEntitlement;
};

}
}
}

#endif
