//
//  DownloadService.h
//  
//
//  Created by Trent Tuggle on 6/25/12.
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#ifndef _DownloadService_h
#define _DownloadService_h

#include "Command.h"
#include "CommandDispatcher.h"

#include "services/session/LoginRegistrationSession.h"
#include "engine/login/LoginController.h"
#include "engine/login/User.h"
#include "engine/downloader/IContentInstallFlow.h"
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "engine/content/ContentTypes.h"
#include "engine/content/Entitlement.h"

#include <limits>
#include <QDateTime>

/////////////////////////////////////////////////////////////////////////
//
// DownloadService

class DownloadService : public QObject
{
    Q_OBJECT
    
public:
    
    static void init();
    
    static DownloadService* instance();
    
    static void registerCommands(CommandDispatcher& dispatcher);
 
    static void execCommand(QStringList const& args);
    
protected:
    void login(QString username, QString password);
    void listEntitlements();
    void download(const QString entitlement);
    void pause(int jobId);
    void resume(int jobId);
    void cancel(int jobId);
    void simulateExit();
    void listJobs();
    void clearDownloadCache();
    
private:
    
    DownloadService();
    
    static DownloadService* sInstance;
    
    CommandDispatcher* mCli;
    
    Origin::Services::Session::LoginRegistrationCredentials mRegistrationCredentials;
    Origin::Engine::UserRef mUser;
    
    QList<Origin::Downloader::IContentInstallFlow*> mPendingFlows;
    
    // Stored lists of pending request and response data.
    QList<Origin::Downloader::InstallLanguageRequest> mPendingEulaLanguages;
    QList<Origin::Downloader::EulaStateRequest> mPendingEulas;
    QList<Origin::Downloader::EulaStateResponse> mPendingEulaResponses;
    
private slots:
    // LoginController slots
    void LoginController_onLoginComplete(Origin::Engine::LoginController::Error);
    void LoginController_userLoggedIn(Origin::Engine::UserRef);
    
    // ContentController slots
    void ContentController_autoUpdateCheckCompleted(bool started, QStringList autoUpdateList);
    void ContentController_updateStarted();
    void ContentController_updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>);
    void ContentController_updateFailed();
    void ContentController_entitlementSignatureVerificationFailed();
    void ContentController_playStarted(QString const& contentId);
    void ContentController_playFinished(QString const& contentId);
    void ContentController_serverUpdateFinished();
    void ContentController_activeFlowsChanged();
    void ContentController_lastServerTimeUpdated(const QDateTime &);
    void ContentController_lastActiveFlowPaused(bool success);

    // IContentInstallFlow slots
    void IContentInstallFlow_stateChanged (Origin::Downloader::ContentInstallFlowState newState);
    void IContentInstallFlow_progressUpdate (Origin::Downloader::ContentInstallFlowState state, const Origin::Downloader::InstallProgressState& stateProgress);
    void IContentInstallFlow_error (qint32 errorType, qint32 errorCode , QString errorMessage, QString key);
    void IContentInstallFlow_warn (qint32 warnCode, const QString& warnMessage);
    void IContentInstallFlow_pendingInstallArguments (const Origin::Downloader::InstallArgumentRequest&);
    void IContentInstallFlow_pendingInstallLanguage (const Origin::Downloader::InstallLanguageRequest&);
    void IContentInstallFlow_pendingEulaState (const Origin::Downloader::EulaStateRequest&);
    void IContentInstallFlow_pendingDiscChange (const Origin::Downloader::InstallArgumentRequest&);
    void IContentInstallFlow_pendingCancelDialog(const Origin::Downloader::CancelDialogRequest&);
    void IContentInstallFlow_pending3PDDDialog(const Origin::Downloader::ThirdPartyDDRequest&);

    // User input response slots
    void setEulaLanguage_response(QStringList input);
    void setEulaState_response(QStringList input);
};


#endif
