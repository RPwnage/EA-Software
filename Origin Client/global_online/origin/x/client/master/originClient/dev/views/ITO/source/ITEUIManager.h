#ifndef ITEUIMANAGER
#define ITEUIMANAGER
#include <QObject>
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "engine/downloader/IContentInstallFlow.h"
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
class ITOViewController;
class ORIGIN_PLUGIN_API ITEUIManager: public QObject
{
    Q_OBJECT

public:
    static ITEUIManager* GetInstance();
    static void DeleteInstance();

    void showNotReadyToDownloadError(const QString& contentId);
    ITOViewController* viewController() const {return mVC;}

public slots:
    void StateChanged(QString newStateName);
    void onLoggedOut();

protected slots:
    void onInstallFinished();
    void onContentInstallFlowStateChanged(Origin::Downloader::ContentInstallFlowState newState);
    void onCoreError();
    void onInsertDisk(const Origin::Downloader::InstallArgumentRequest& request);
    void onError(qint32 errorType, qint32 errorCode, QString errorMessage, QString key);
    void onTouchupInstallerError();
    void onCancel();
    void onInstallFlowCanceled();
    void onSilentAutoUpdateDownloadError();
    void onSwitchToDownloadFromServer();
    void onDiscMsgRetry();
    void onInstallFlowStateChanged();
    void onDownloadGame();
    void onRetryInstallFlow();
    void onStopDownloadFlow();
    void onRedemptionDone();
    void onRetryDisc();
    void resetFlow();

private:
    explicit ITEUIManager();
    ~ITEUIManager();
    ITEUIManager & operator=(const ITEUIManager &rhs) {
		mVC = NULL;
		return *this;
	};

    void Init();
    void Shutdown();

    ITOViewController* mVC;
};
}
}

#endif // ITEUIMANAGER
