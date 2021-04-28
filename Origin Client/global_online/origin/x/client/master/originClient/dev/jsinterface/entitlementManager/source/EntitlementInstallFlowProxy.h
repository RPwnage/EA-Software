#ifndef _ENTITLEMENTINSTALLFLOWPROXY_H
#define _ENTITLEMENTINSTALLFLOWPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QVariant>
#include "EntitlementServerOperationProxy.h"
#include "engine/content/Entitlement.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Downloader
{
    class IContentInstallFlow;
}
namespace Client
{
class LocalizedContentString;
namespace JsInterface
{

class ORIGIN_PLUGIN_API EntitlementInstallFlowProxy : public EntitlementServerOperationProxy
{
	Q_OBJECT

public:
	EntitlementInstallFlowProxy(QObject *parent, Origin::Engine::Content::EntitlementRef entitlement);

	// Can pause/resume the operation
	Q_INVOKABLE void pause();
	Q_INVOKABLE void resume();

	// Cancels the current operation if it's cancellable
	Q_INVOKABLE void cancel();

    /// \brief Returns operation phase of specific phase
    Q_INVOKABLE QString specificPhaseDisplay(const QString& state);

    /// \brief Returns wheither we should "light up" a flag, given flag name.
    Q_PROPERTY(bool shouldLightFlag READ shouldLightFlag)
    bool shouldLightFlag();

	// Returns the progress of the operation
	Q_PROPERTY(QVariant progress READ progress)
	QVariant progress();

    Q_PROPERTY(QString progressState READ progressState)
    QString progressState();

    Q_PROPERTY(QVariant bytesDownloaded READ bytesDownloaded)
    QVariant bytesDownloaded();

    Q_PROPERTY(QVariant totalFileSize READ totalFileSize)
    QVariant totalFileSize();

    Q_PROPERTY(QVariant bytesPerSecond READ bytesPerSecond)
    QVariant bytesPerSecond();

    Q_PROPERTY(QVariant secondsRemaining READ secondsRemaining)
    QVariant secondsRemaining();

    Q_PROPERTY(QString type READ type);
    QString type();

    Q_PROPERTY(QString typeDisplay READ typeDisplay);
    QString typeDisplay();

	Q_PROPERTY(QString phase READ phase);
	QString phase();

    Q_PROPERTY(QString phaseDisplay READ phaseDisplay);
    QString phaseDisplay();

	Q_PROPERTY(bool pausable READ pausable);
    bool pausable();
	
    Q_PROPERTY(bool resumable READ resumable);
    bool resumable();

    Q_PROPERTY(bool cancellable READ cancellable);
    bool cancellable();

    Q_PROPERTY(double playableAt READ playableAt);
    double playableAt();

    Q_PROPERTY(QVariant isDynamicOperation READ isDynamicOperation);
    bool isDynamicOperation();

private:
	Origin::Engine::Content::EntitlementRef mEntitlement;
    Downloader::IContentInstallFlow* mEntitlementInstallFlow;
    LocalizedContentString* mLocalizedContentStrings;
};

}
}
}

#endif
