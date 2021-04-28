/////////////////////////////////////////////////////////////////////////////
// InstallerViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef INSTALLERVIEWCONTROLLER_H
#define INSTALLERVIEWCONTROLLER_H

#include <QWidget>
#include <QSharedPointer>
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "engine/content/Entitlement.h"
#include "engine/cloudsaves/RemoteSync.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
    class OriginWindow;
}

namespace Client
{
class ORIGIN_PLUGIN_API InstallerViewController : public QObject
{
	Q_OBJECT

public:
    enum installerUIStates
    {
        ShowRegular = -1,
        ShowNothing = 0,
        ShowMonitored,
        UpdateOnly
    };
    InstallerViewController(Engine::Content::EntitlementRef entitlement, const bool& inDebug = false, QObject *parent = 0);
	~InstallerViewController();

    void setInDebug(const bool& inDebug) {mInDebug = inDebug;}
    void showInstallArguments(const Origin::Downloader::InstallArgumentRequest& request);
    void showInsufficientFreeSpaceWindow(const Origin::Downloader::InstallArgumentRequest& request);
	void showFolderAccessErrorWindow(const Origin::Downloader::InstallArgumentRequest& request);
    void showLanguageSelection(const Origin::Downloader::InstallLanguageRequest& input);
    void showEulaSummaryDialog(const Origin::Downloader::EulaStateRequest& request);
    void showCancelDialog(const Origin::Downloader::CancelDialogRequest& request, const installerUIStates& uiState = ShowRegular);
    void showCancelWithDLCDependentsDialog(const Origin::Downloader::CancelDialogRequest& request, const QStringList& childrenNames, const installerUIStates& uiState = ShowRegular);
    void showDLCOperationFinished();
    void showInstallArgumentsWarnings(const QList<QPair<int, unsigned short>>& warningList);
    void pushAndShowErrorWindow(const QString& title, const QString& text, const QString& key);

    void showPatchErrorDialog();
    void showInstallThroughSteam(const installerUIStates& monitored);
    void showInstallThroughWindowsLive(const installerUIStates& monitored);
    void showInstallThrougOther(const installerUIStates& monitored);
    void setInstallArgSize(const qint64& size) {mInstallArg.installedSize = size;}

    QPair<QString, QString> installErrorText(qint32 type, qint32 code, const QString& context);

public slots:
    void onInstallArgumentsDone(QJsonObject);
    void onInstallArgumentsWarningDone(QJsonObject);
    void onPatchErrorDone(QJsonObject);
    void on3PDDInstallDone(QJsonObject);
    void onCancelDone(QJsonObject);
    void onEULAConsolidatedDone(QJsonObject);
    void onCancelWithDLCDependentsDone(QJsonObject);
    void onSelectInstallLanguageDone(QJsonObject);
    void onEulaSequentialDone(QJsonObject);
    void emitStopFlow();

signals:
    void retryDiskSpace(const Origin::Downloader::InstallArgumentRequest&);
    void installArgumentsCreated(const QString& productId, Origin::Downloader::InstallArgumentResponse args);
    void languageSelected(const QString& productId, Origin::Downloader::EulaLanguageResponse response);
    void eulaStateDone(const QString&, Origin::Downloader::EulaStateResponse);
    void stopFlow();
    void stopFlowForSelfAndDependents();

    void continueInstall();
    void clearRTPLaunch();


private slots:
	void onMinimizeClientToTray();
    void onRemoveErrorDialog();
    void emitRetryDiskSpace();
    void onDialogLinkClicked(const QJsonObject&);

private:
    QString rejectGroup() const;
    void pushAndShowErrorWindow(UIToolkit::OriginWindow* window, const QString& key);
    void logUserFacingErrors(const QString& error, const QString& context);
	const QString GetGenericErrorString(int serviceId, int iErrorCode);
    void showThirdParty(const QString& promptText, const QString& platformNameText, const QString& step2summary, const bool& monitoredInstall);
    bool convertFileType(QString& filename, const QString& fileType);

    QMap<QString, Origin::UIToolkit::OriginWindow*> mErrorWindows;
    Engine::Content::EntitlementRef mEntitlement;
    bool mInDebug;
    Downloader::InstallArgumentRequest mInstallArg;
    Downloader::InstallArgumentResponse mInstallArgReponse;
    Downloader::EulaStateRequest mEulaRequest;
};
}
}
#endif