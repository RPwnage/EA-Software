/////////////////////////////////////////////////////////////////////////////
// ITOViewController.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef ITOVIEWCONTROLLER_H
#define ITOVIEWCONTROLLER_H

#include <QObject>
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
    class OriginWindow;
}
namespace Client
{

/// \brief Controller for 
class ORIGIN_PLUGIN_API ITOViewController : public QObject
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent - The parent of the View Controller.
    ITOViewController(const QString& gameName, QObject* parent = 0, const bool& inDebug = false);

    /// \brief Destructor
    ~ITOViewController();

    void showPrepare(const Origin::Downloader::InstallArgumentRequest& request);
    void showInstallFailed();
    void showFailedNoGameName();
    void showFreeToPlayFailNoEntitlement();
    void showRedeem();
    void showWrongCode();
    void showDiscCheck();
    void showNotReadyToDownload();
    void showSystemReqFailed();
    void showDLCSystemReqFailed();
    void showInsertDisk(const int& discNum = -1, const int& totalDiscs = -1);
    void showWrongDisk(const int& discNum = -1);
    void showReadDiscError();
    void showDownloadPreparing();
    void showInstallArgumentsWarnings();

signals:
    void playGame();
    void finished();
    void cancel();
    void retryInstallFlow();
    void downloadGame();
    void stopDownloadFlow();
    void redemptionDone();
    void retryDisc();
    void resetFlow();

public slots:
    void closeWindow();
    void showCancelConfirmation();
    void showCurrentWindow();


private slots:
    void onInstallArgumentsWarningDone(const int& result);
    void onPrepareDone(int result);
    void onFinishedDone(int playGame = 0);
    void onInsertInitialDiscDone(int result);
    void onGoToNext(int result);

private:
    QList<QPair<int, unsigned short>> mWarningList;
    Downloader::InstallArgumentRequest mInstallArg;
    Downloader::InstallArgumentResponse mInstallArgReponse;
    const QString mGameName;
    UIToolkit::OriginWindow* mWindow;
    const bool mInDebug;
};

}
}
#endif
