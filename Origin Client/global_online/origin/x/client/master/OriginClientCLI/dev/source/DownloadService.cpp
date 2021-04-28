//
//  DownloadService.cpp
//  
//
//  Created by Trent Tuggle on 6/26/12.
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#include "DownloadService.h"
#include "Console.h"
#include "services/debug/DebugService.h"
#include "services/session/SessionService.h"
#include "services/session/LoginRegistrationSession.h"
#include "engine/login/LoginCredentials.h"
#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "engine/content/Entitlement.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/downloader/ContentServices.h"

#include <iostream>

using Origin::Engine::LoginController;
using Origin::Services::Session::LoginRegistrationCredentials;
using Origin::Downloader::IContentInstallFlow;

DownloadService* DownloadService::sInstance = 0;

void DownloadService::init()
{
    LoginController::init();

    if (! sInstance) sInstance = new DownloadService();
    
}

DownloadService::DownloadService()
{
    
    ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(loginComplete(Origin::Engine::LoginController::Error)), this, SLOT(LoginController_onLoginComplete(Origin::Engine::LoginController::Error)));
    ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)), this, SLOT(LoginController_userLoggedIn(Origin::Engine::UserRef)));
}

DownloadService* DownloadService::instance()
{
    return sInstance;
}

void DownloadService::registerCommands(CommandDispatcher& dispatcher)
{
    instance()->mCli = &dispatcher;
    dispatcher.add(Command("dl", "help,login,list,start,pause,resume,cancel", DownloadService::execCommand));
}

void DownloadService::execCommand(QStringList const& args)
{
    // Determine which sub-command was executed.
    QString command = args.value(1);
    
    if (QString::compare(command, "login", Qt::CaseInsensitive) == 0)
    {
        instance()->login(args.value(2), args.value(3));
    }
    
    else if (QString::compare(command, "start", Qt::CaseInsensitive) == 0)
    {
        instance()->download(args.value(2));
    }
    
    else if (QString::compare(command, "pause", Qt::CaseInsensitive) == 0)
    {
        instance()->pause(args.value(2).toInt());
    }

    else if (QString::compare(command, "resume", Qt::CaseInsensitive) == 0)
    {
        instance()->resume(args.value(2).toInt());
    }

    else if (QString::compare(command, "cancel", Qt::CaseInsensitive) == 0)
    {
        instance()->cancel(args.value(2).toInt());
    }
    
    else if (QString::compare(command, "list", Qt::CaseInsensitive) == 0)
    {
        instance()->listEntitlements();
    }
    
    else if (QString::compare(command, "simulateExit", Qt::CaseInsensitive) == 0)
    {
        instance()->simulateExit();
    }
    else if (QString::compare(command, "clear", Qt::CaseInsensitive) == 0)
    {
        instance()->clearDownloadCache();
    }
    
    else if (QString::compare(command, "jobs", Qt::CaseInsensitive) == 0)
    {
        instance()->listJobs();
    }
    else
    {
        CONSOLE_WRITE << "\n\nDOWNLOADER COMMANDS\n"
        "    login -- login, optionally using the specified credentials\n"
        "    list -- list the downloadable entitlements\n"
        "    jobs -- list the active download jobs (flows)\n"
        "    start <Entitlement # in list> -- start a download job\n"
        "    pause <Entitlement # in list> -- pause a download job\n"
        "    resume <Entitlement # in list> -- resume a paused download job\n"
        "    cancel <Entitlement # in list> -- cancel a download job\n"
        "    simulateExit -- suspends all active downloads via the ContentController\n"
        "    clear -- clear the download cache\n"
       ;
    }
    
}


void DownloadService::login(QString username, QString password)
{
    if (username.isNull()) username = "rbinns_gamesim2";
    if (password.isNull()) password = "G4m#S1m9";
//    if (username.isNull()) username = "rbinns_integr";
//    if (password.isNull()) password = "op87)%Zh";
   
    mRegistrationCredentials = LoginRegistrationCredentials(username, password);
    
    LoginController::credentialLoginAsync(mRegistrationCredentials, Origin::Services::Session::LOGIN_MANUAL);
}

void DownloadService::listEntitlements()
{
    if (! Origin::Engine::Content::ContentController::currentUserContentController()) { CONSOLE_WRITE << "Not logged in."; return; }
    
    typedef QList<QSharedPointer<Origin::Engine::Content::Entitlement> > EntitlementsList;
    
    EntitlementsList list = Origin::Engine::Content::ContentController::currentUserContentController()->entitlements();
    int count = list.size();
    
    CONSOLE_WRITE << count << " entitlements:";
    
    for (int i = 0; i != count; ++i)
    {
	if(list.at(i)->localContent()->installFlow() != NULL)
	{
        	CONSOLE_WRITE << "    [" << i << "]   " << list.at(i)->contentConfiguration()->displayName() << " (" << list.at(i)->localContent()->installFlow()->getFlowState().toString() << ')';
	}
	else
	{
        	CONSOLE_WRITE << "    [" << i << "]   " << list.at(i)->contentConfiguration()->displayName() << " (no install flow - not downloadable)";
	}
    }
}

void DownloadService::download(const QString entitlement)
{
    if (! Origin::Engine::Content::ContentController::currentUserContentController()) { CONSOLE_WRITE << "Not logged in."; return; }

    int downloadId = entitlement.toInt();
    
    typedef QList<QSharedPointer<Origin::Engine::Content::Entitlement> > EntitlementsList;
    EntitlementsList list = Origin::Engine::Content::ContentController::currentUserContentController()->entitlements();
    
    if ( downloadId >= list.size() )
    {
        CONSOLE_WRITE << "Cannot download entitlement ID " << downloadId << " -- choose a valid entitlement from the following list:\n";
        listEntitlements();
        return;
    }
    
    CONSOLE_WRITE << "Starting download of " << list.at(downloadId)->contentConfiguration()->displayName();
    
    // Create and save the flow.
    IContentInstallFlow* flow = list.at(downloadId)->localContent()->installFlow();
    
    // Monitor this flow
    connect(flow, SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(IContentInstallFlow_stateChanged (Origin::Downloader::ContentInstallFlowState)));
    connect(flow, SIGNAL(progressUpdate(Origin::Downloader::ContentInstallFlowState, const Origin::Downloader::InstallProgressState&)), this, SLOT(IContentInstallFlow_progressUpdate (Origin::Downloader::ContentInstallFlowState, const Origin::Downloader::InstallProgressState&)));
    connect(flow, SIGNAL(IContentInstallFlow_error(qint32, qint32, QString, QString)), this, SLOT(IContentInstallFlow_error (qint32, qint32, QString, QString)));
    connect(flow, SIGNAL(warn(qint32, const QString&)), this, SLOT(IContentInstallFlow_warn(qint32, const QString&)));
    connect(flow, SIGNAL(pendingInstallArguments(const Origin::Downloader::InstallArgumentRequest&)), this, SLOT(IContentInstallFlow_pendingInstallArguments(const Origin::Downloader::InstallArgumentRequest&)));
    connect(flow, SIGNAL(pendingInstallLanguage(const Origin::Downloader::InstallLanguageRequest&)), this, SLOT(IContentInstallFlow_pendingInstallLanguage(const Origin::Downloader::InstallLanguageRequest&)));
    connect(flow, SIGNAL(pendingEulaState(const Origin::Downloader::EulaStateRequest&)), this, SLOT(IContentInstallFlow_pendingEulaState(const Origin::Downloader::EulaStateRequest&)));
    connect(flow, SIGNAL(pendingDiscChange(const Origin::Downloader::InstallArgumentRequest&)), this, SLOT(IContentInstallFlow_pendingDiscChange(const Origin::Downloader::InstallArgumentRequest&)));
    connect(flow, SIGNAL(pendingCancelDialog(const Origin::Downloader::CancelDialogRequest&)), this, SLOT(IContentInstallFlow_pendingCancelDialog(const Origin::Downloader::CancelDialogRequest&)));
    connect(flow, SIGNAL(pending3PDDDialog(const Origin::Downloader::ThirdPartyDDRequest&)), this, SLOT(IContentInstallFlow_pending3PDDDialog(const Origin::Downloader::ThirdPartyDDRequest&)));
    
    flow->begin();
}

void DownloadService::pause(int jobId)
{
    if (! Origin::Engine::Content::ContentController::currentUserContentController()) { CONSOLE_WRITE << "Not logged in."; return; }

    typedef QList<QSharedPointer<Origin::Engine::Content::Entitlement> > EntitlementsList;
    EntitlementsList list = Origin::Engine::Content::ContentController::currentUserContentController()->entitlements();
    IContentInstallFlow* flow = list.at(jobId)->localContent()->installFlow();
    if(flow && flow->canPause())
    {
        flow->pause(false);
    }
    else
    {
        CONSOLE_WRITE << "Entitlement at [" << jobId << "] is not in a pausable state.";
    } 
}

void DownloadService::resume(int jobId)
{
    typedef QList<QSharedPointer<Origin::Engine::Content::Entitlement> > EntitlementsList;
    EntitlementsList list = Origin::Engine::Content::ContentController::currentUserContentController()->entitlements();
    IContentInstallFlow* flow = list.at(jobId)->localContent()->installFlow();
    if(flow && flow->canResume())
    {
        flow->resume();
    }
    else
    {
        CONSOLE_WRITE << "Entitlement at [" << jobId << "] is not in a resumable state.";
    } 
}

void DownloadService::listJobs()
{
    if (! Origin::Engine::Content::ContentController::currentUserContentController()) { CONSOLE_WRITE << "Not logged in."; return; }

    // Prepare to iterate over the list of active flows.
    QMultiHash<QString, Origin::Engine::Content::EntitlementRef> currentJobs = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementsWithActiveFlows();
    CONSOLE_WRITE << "Have " << currentJobs.size() << " flows:\n";
    
    // Iterate over the list of active flows.
    int id=0;
    for (QHashIterator<QString, Origin::Engine::Content::EntitlementRef> i(currentJobs); i.hasNext(); ++id)
    {
        i.next();
        IContentInstallFlow* flow = i.value()->localContent()->installFlow();
        CONSOLE_WRITE << "        [" << id << "] " << qPrintable(i.value()->contentConfiguration()->displayName()) << "  (" << flow->getFlowState().operator const char*() << ") Installing in: " << qPrintable(i.key());
    }
}
void DownloadService::cancel(int jobId)
{
    typedef QList<QSharedPointer<Origin::Engine::Content::Entitlement> > EntitlementsList;
    EntitlementsList list = Origin::Engine::Content::ContentController::currentUserContentController()->entitlements();
    IContentInstallFlow* flow = list.at(jobId)->localContent()->installFlow();
    if(flow && flow->canCancel())
    {
        flow->cancel();
    }
    else
    {
        CONSOLE_WRITE << "Entitlement at [" << jobId << "] is not in a cancelable state.";
    } 
}

void DownloadService::simulateExit()
{
    CONSOLE_WRITE << "Suspending all active flows";
    Origin::Engine::Content::ContentController::currentUserContentController()->suspendAllActiveFlows(false);
}

void DownloadService::clearDownloadCache()
{
    CONSOLE_WRITE << "Clearing the download cache for all entitlements...";
    
    typedef QList<QSharedPointer<Origin::Engine::Content::Entitlement> > EntitlementsList;
    EntitlementsList list = Origin::Engine::Content::ContentController::currentUserContentController()->entitlements();
    
    for (int i=0; i !=list.size(); ++i)
    {
        CONSOLE_WRITE << "     Clearing cache for " << qPrintable(list.at(i)->contentConfiguration()->displayName());
        Origin::Downloader::ContentServices::ClearDownloadCache(list.at(i).data());
    }
}


void DownloadService::LoginController_userLoggedIn(Origin::Engine::UserRef user)
{
    CONSOLE_WRITE << "*** GOT USER ***\n";
    mUser = user;
}

void DownloadService::LoginController_onLoginComplete(Origin::Engine::LoginController::Error error)
{
    if (error == LoginController::LOGIN_SUCCEEDED)
    {
        CONSOLE_WRITE << "*** Downloader logged in ***\n";
        
        // Create a content controller.
        mUser->createContentController();
        Origin::Engine::Content::ContentController* controller = Origin::Engine::Content::ContentController::currentUserContentController();
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(activeFlowsChanged()), this, SLOT(ContentController_activeFlowsChanged()));
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(autoUpdateCheckCompleted(bool, QStringList)), this, SLOT(ContentController_autoUpdateCheckCompleted(bool, QStringList)));
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(updateStarted()), this, SLOT(ContentController_updateStarted()));
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), this, SLOT(ContentController_updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)));
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(updateFailed()), this, SLOT(ContentController_updateFailed()));
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(entitlementSignatureVerificationFailed()), this, SLOT(ContentController_entitlementSignatureVerificationFailed()));
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(playStarted(QString const&)), this, SLOT(ContentController_playStarted(QString const&)));
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(playFinished(QString const&)), this, SLOT(ContentController_playFinished(QString const&)));
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(serverUpdateFinished()), this, SLOT(ContentController_serverUpdateFinished()));
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(activeFlowsChanged()), this, SLOT(ContentController_activeFlowsChanged()));
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(lastServerTimeUpdated(const QDateTime &)), this, SLOT(ContentController_lastServerTimeUpdated(const QDateTime &)));
        ORIGIN_VERIFY_CONNECT(controller, SIGNAL(lastActiveFlowPaused(bool)), this, SLOT(ContentController_lastActiveFlowPaused(bool)));
        
        // Refresh entitlements.
        controller->refresh();
    }
    else
        CONSOLE_WRITE << "*** Login failed. ***\n";
}

//
// ContentController slots
//

void DownloadService::ContentController_autoUpdateCheckCompleted(bool, QStringList)
{
    CONSOLE_WRITE << "ContentController_autoUpdateCheckCompleted";
}

void DownloadService::ContentController_updateStarted()
{
    CONSOLE_WRITE << "ContentController_updateStarted";
}

void DownloadService::ContentController_updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)
{
    CONSOLE_WRITE << "ContentController_updateFinished";
}

void DownloadService::ContentController_updateFailed()
{
    CONSOLE_WRITE << "ContentController_updateFailed";
}

void DownloadService::ContentController_entitlementSignatureVerificationFailed()
{
    CONSOLE_WRITE << "ContentController_entitlementSignatureVerificationFailed";
}

void DownloadService::ContentController_playStarted(QString const&)
{
    CONSOLE_WRITE << "ContentController_playStarted";
}

void DownloadService::ContentController_playFinished(QString const&)
{
    CONSOLE_WRITE << "ContentController_playFinished";
}

void DownloadService::ContentController_serverUpdateFinished()
{
    CONSOLE_WRITE << "ContentController_serverUpdateFinished";
}

void DownloadService::ContentController_activeFlowsChanged()
{
    CONSOLE_WRITE << "ContentController_activeFlowsChanged";
}

void DownloadService::ContentController_lastServerTimeUpdated(const QDateTime &)
{
    CONSOLE_WRITE << "ContentController_lastServerTimeUpdated";
}

void DownloadService::ContentController_lastActiveFlowPaused(bool)
{
    CONSOLE_WRITE << "ContentController_lastActiveFlowPaused";
}

//
// IContentInstallFlow slots
//

void DownloadService::IContentInstallFlow_stateChanged (Origin::Downloader::ContentInstallFlowState newState)
{
    CONSOLE_WRITE << "Got a stateChanged to " << newState.operator const char*();
}

void DownloadService::IContentInstallFlow_progressUpdate (Origin::Downloader::ContentInstallFlowState state, const Origin::Downloader::InstallProgressState& stateProgress)
{
    //CONSOLE_WRITE << "Got a progressUpdate";
}

void DownloadService::IContentInstallFlow_error (qint32 errorType, qint32 errorCode , QString errorMessage, QString key)
{
    CONSOLE_WRITE << "Got a IContentInstallFlow_error";
}

void DownloadService::IContentInstallFlow_warn (qint32 warnCode, const QString& warnMessage)
{
    CONSOLE_WRITE << "Got a warn";
}

void DownloadService::IContentInstallFlow_pendingInstallArguments (const Origin::Downloader::InstallArgumentRequest&)
{
    CONSOLE_WRITE << "Got a pendingInstallArguments -- installing without argument.";
    
    // Save the flow that signalled us.  (Bail if we cannot obtain it.)
    IContentInstallFlow* flow = dynamic_cast<IContentInstallFlow*>(sender());
    if (flow == 0) return; // Bail if we cannot find the sending flow.
    
    // Create a response.
    Origin::Downloader::InstallArgumentResponse response;
    response.optionalComponentsToInstall = 0;
    response.installDesktopShortCut = false;
    response.installStartMenuShortCut = false;
    flow->setInstallArguments(response);
}

void DownloadService::IContentInstallFlow_pendingInstallLanguage (const Origin::Downloader::InstallLanguageRequest& req)
{
    // Save the flow that signalled us.  (Bail if we cannot obtain it.)
    IContentInstallFlow* flow = dynamic_cast<IContentInstallFlow*>(sender());
    if (flow == 0) return; // Bail if we cannot find the sending flow.
    mPendingFlows.prepend(flow);

    // Save the pending request data.
    mPendingEulaLanguages.prepend(req);
    
    int count = req.availableLanguages.size();
    
    CONSOLE_WRITE << "Choose a language:";
    for (int i = 0; i != count; ++i)
        CONSOLE_WRITE << "    [" << i << "]   " << qPrintable(req.availableLanguages.at(i));

    mCli->takeInputFromNextLine(this, "setEulaLanguage_response");
}

void DownloadService::setEulaLanguage_response(QStringList input)
{
    // Obtain the flow and request data.
    IContentInstallFlow* flow = mPendingFlows.takeLast();
    Origin::Downloader::InstallLanguageRequest req = mPendingEulaLanguages.takeLast();

    // If the user repsonse is invalid,
    int userSelectedIndex = input[0].toInt();
    if (userSelectedIndex >= req.availableLanguages.size())
    {
        // Notify the user, save our data, and wait for input again.
        CONSOLE_WRITE << "Invalid selection.  Choose an integer from 0 to " << req.availableLanguages.size();
        mPendingFlows.append(flow);
        mPendingEulaLanguages.append(req);
        mCli->takeInputFromNextLine(this, SLOT(setEulaLanguage_response(QStringList)));
        return;
    }
    
    CONSOLE_WRITE << "Selecting language " << qPrintable(req.availableLanguages.at(userSelectedIndex));
    
    // Create response from the user input.
    Origin::Downloader::EulaLanguageResponse response;
    response.selectedLanguage = req.availableLanguages.at(userSelectedIndex);
    
    // Call Q_INVOKABLE virtual void setEulaLanguage (const Origin::Downloader::EulaLanguageResponse&) = 0;
    QMetaObject::invokeMethod( flow, "setEulaLanguage", Q_ARG(Origin::Downloader::EulaLanguageResponse, response) );
}

void DownloadService::IContentInstallFlow_pendingEulaState (const Origin::Downloader::EulaStateRequest& req)
{
    // Save the flow that signalled us.  (Bail if we cannot obtain it.)
    IContentInstallFlow* flow = dynamic_cast<IContentInstallFlow*>(sender());
    if (flow == 0) return; // Bail if we cannot find the sending flow.
    mPendingFlows.prepend(flow);
    
    // Save the pending request data.
    mPendingEulas.prepend(req);
    
    // Create data for the pending response.
    Origin::Downloader::EulaStateResponse response;
    response.acceptedBits = 0;
    mPendingEulaResponses.prepend(response);
    
    int count = req.eulas.listOfEulas.size();
    
    CONSOLE_WRITE << "Must accept " << count << " EULAs...";
    
    // Prompt the user with the first EULA.
    Origin::Downloader::Eula firstEula = mPendingEulas.last().eulas.listOfEulas.first();
    CONSOLE_WRITE << "    Do you accept " << qPrintable(firstEula.Description) << " (Y/N)?";
    
    mCli->takeInputFromNextLine(this, "setEulaState_response");
}

void DownloadService::setEulaState_response(QStringList input)
{
    // Obtain the flow, request, and response data.
    IContentInstallFlow* flow = mPendingFlows.takeLast();
    Origin::Downloader::EulaStateRequest req = mPendingEulas.takeLast();
    Origin::Downloader::EulaStateResponse response = mPendingEulaResponses.takeLast();
    
    // Update the response based on user input.
    Origin::Downloader::Eula eula = req.eulas.listOfEulas.takeFirst();
    response.acceptedBits <<= 1;
    if (QString::compare(input[0].left(1), "y", Qt::CaseInsensitive) == 0)
    {
        CONSOLE_WRITE << "          " << qPrintable(eula.Description) << " Accepted.";
        response.acceptedBits &= 1;
    }
    else if (QString::compare(input[0].left(1), "n", Qt::CaseInsensitive) == 0)
    {
        CONSOLE_WRITE << "           " << qPrintable(eula.Description) << " Rejected.";
        response.acceptedBits &= 0;
    }
    else
    {
        // Re-prompt the user for this EULA.
        CONSOLE_WRITE << "    Must answer Y or N.";
        
        // Save state for next time.
        mPendingFlows.append(flow);
        mPendingEulas.append(req);
        mPendingEulaResponses.append(response);
        mCli->takeInputFromNextLine(this, "setEulaState_response");
    }
    
    // If there are remaining EULAs to be accepted, prompt the user.
    if (req.eulas.listOfEulas.size())
    {
        // Prompt the user for the next EULA.
        Origin::Downloader::Eula nextEula = req.eulas.listOfEulas.first();
        CONSOLE_WRITE << "    Do you accept " << qPrintable(nextEula.Description) << " (Y/N)?";
        mCli->takeInputFromNextLine(this, "setEulaState_response");
        
        // Save state.
        mPendingFlows.append(flow);
        mPendingEulas.append(req);
        mPendingEulaResponses.append(response);
    }
    
    // Otherwise,
    else
    {
        // Send the response on to the user.
        flow->setEulaState(response);
    }
}

void DownloadService::IContentInstallFlow_pendingDiscChange (const Origin::Downloader::InstallArgumentRequest&)
{
    CONSOLE_WRITE << "Got a pendingDiscChange";
}

void DownloadService::IContentInstallFlow_pendingCancelDialog(const Origin::Downloader::CancelDialogRequest& req)
{
    CONSOLE_WRITE << "Got a pendingCancelDialog for " << qPrintable(req.contentDisplayName);
}

void DownloadService::IContentInstallFlow_pending3PDDDialog(const Origin::Downloader::ThirdPartyDDRequest& req)
{
    CONSOLE_WRITE << "Got a pending3PDDDialog for contentId " << qPrintable(req.contentId);
}

