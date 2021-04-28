/////////////////////////////////////////////////////////////////////////////
// PlayViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "PlayViewController.h"

#include "engine/content/ContentController.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/GamesController.h"
#include "engine/content/PlayFlow.h"
#include "engine/igo/IGOController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "Service/SDKService/SDK_ServiceArea.h"
#include "TelemetryAPIDLL.h"

#include "origincheckbutton.h"
#include "originmsgbox.h"
#include "originscrollablemsgbox.h"
#include "origintransferbar.h"
#include "originwindow.h"
#include "originwindowmanager.h"

#include "LocalizedContentString.h"
#include "MultiContentProgressIntegrator.h"

#include "PlayView.h"
#include "MultiLaunchView.h"
#include "MainFlow.h"
#include "ClientFlow.h"
#include "ContentFlowController.h"
#include "ThirdPartyDDDialog.h"
#include "LocalContentViewController.h"
#include "LocalizedDateFormatter.h"
#include "OriginApplication.h"
#include "TelemetryAPIDLL.h"
#include "DialogController.h"
#include "services/qt/QtUtil.h"


namespace Origin
{
namespace Client
{
namespace
{
    const int LAUNCHING_TIME_OUT = 3000;
    const int HOURS_IN_DAY = 24;
}

PlayViewController::PlayViewController(const QString& productId, const bool& inDebug, QObject *parent)
: QObject(parent)
, mInDebug(inDebug)
{
    //save off the ptr to entitlement so that we don't have to repeatedly call entitlemenByContentId which could be slow (if it's stored as a list)
    mEntitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productId);

#ifdef Q_OS_WIN
    mFlashTimer.stop();
    ORIGIN_VERIFY_CONNECT(&mFlashTimer, &QTimer::timeout, this, &PlayViewController::onFlashTimer);
#endif
}


PlayViewController::~PlayViewController()
{
#ifdef Q_OS_WIN 
    mFlashTimer.stop();
    ORIGIN_VERIFY_DISCONNECT(&mFlashTimer, &QTimer::timeout, this, &PlayViewController::onFlashTimer);
#endif

    closeWindow();
}


void PlayViewController::closeWindow()
{
    DialogController::instance()->closeGroup(rejectGroup());
}


void PlayViewController::emitStopFlow()
    {
    emit stopFlow();
    }

#ifdef Q_OS_WIN 
void PlayViewController::onFlashTimer()
{
#if FIX_FOR_X
    if (mWindow)
    {
		FlashWindow((HWND)mWindow->winId(), mFlashState);
        mFlashState = !mFlashState; // toggle
    }
#endif
}
#endif

void PlayViewController::showUpdateRequiredDialog()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";
    
    // Make sure the client is visible when we show this dialog. #7 & #8:
    // https://developer.origin.com/documentation/display/EBI/Origin+Quiet+Mode+PRD#OriginQuietModePRD-1.2.Ignorenonmandatorypatches%5CMandatory%5C
    ClientFlow::instance()->minimizeClientWindow(false);

    QJsonObject persistingObj;
    persistingObj["selectedOption"] = "UpdateCancelLaunch";
    QJsonObject content;
    content["checkeddefault"] = static_cast<bool>(Services::readSetting(Services::SETTING_AUTOPATCH, Engine::LoginController::currentUser()->getSession()));
    content["checkboxtext"] =  DialogController::MessageTemplate::toAttributeFriendlyText(tr("ebisu_client_settings_autopatch"));

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-singlecheckbox",
        tr("ebisu_client_update_required").toUpper(), tr("ebisu_client_game_update_required_text").arg("<strong>" + mEntitlement->contentConfiguration()->displayName() + "</strong>"), 
        tr("ebisu_client_game_update_button"), tr("ebisu_client_cancel"), "yes", content, this, "onUpdateRequiredPromptDone", persistingObj);
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::onUpdateRequiredPromptDone(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        const bool keepUpToDate = Services::JsonValueValidator::validate(obj["chkChecked"]).toBool();
        Services::writeSetting(Services::SETTING_AUTOPATCH, keepUpToDate, Engine::LoginController::currentUser()->getSession());
        emit updateExistAnswer(UpdateCancelLaunch);
    }
    else
    {
        emit stopFlow();
    }

    // to address https://developer.origin.com/support/browse/ORPLAT-1129 - sparta launches but a required update is needed and Origin is minimized
    // but because a program cannot take focus when minimized, the least we can do is flash the taskbar icon (sparta should be minimized).
#ifdef Q_OS_WIN 
    const int kFlashInterval = 500; // milliseconds
    mFlashState = true;
    mFlashTimer.stop();
    mFlashTimer.start(kFlashInterval);
#endif
}


void PlayViewController::showUpdateExistsDialog()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";
    
    QJsonArray commandbuttons;
    commandbuttons.append(PlayView::commandLinkObj("PlayWhileUpdating", "otkicon-downloadnegative", tr("ebisu_client_game_update_button"), tr("ebisu_client_you_need_to_restart_game")));
    commandbuttons.append(PlayView::commandLinkObj("PlayWithoutUpdate", "otkicon-close", tr("ebisu_client_play_without_updating"), tr("ebisu_client_play_without_updating_description").arg("<strong>" + mEntitlement->contentConfiguration()->displayName() + "</strong>")));

    QJsonObject content;
    content["commandbuttons"] = commandbuttons;
    content["checkeddefault"] = static_cast<bool>(Services::readSetting(Services::SETTING_AUTOPATCH, Engine::LoginController::currentUser()->getSession()));
    content["checkboxtext"] =  DialogController::MessageTemplate::toAttributeFriendlyText(tr("ebisu_client_settings_autopatch"));

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-playupdateexists", 
        tr("ebisu_client_game_update_available_title"), tr("ebisu_client_update_for_game_available").arg("<strong>" + mEntitlement->contentConfiguration()->displayName() + "</strong>"), 
        tr("ebisu_client_cancel"), content, this, "onUpdatePromptDone");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::showPluginIncompatible()
    {
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("",
        tr("ebisu_client_plugin_incompatible_title"), tr("ebisu_client_plugin_incompatible_description").arg("<strong>" + mEntitlement->contentConfiguration()->displayName() + "</strong>"), 
        tr("ebisu_client_close"), QJsonObject(), this, "emitStopFlow");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
    }


void PlayViewController::showSystemRequirementsNotMet(const QString& info)
{
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("",
        tr("ebisu_client_system_reqs_not_met_title"),
        tr("ebisu_client_higher_operatingsystem_required_description").arg(mEntitlement->contentConfiguration()->displayName()).arg(tr("ebisu_client_osx").append(" ").append(info)), 
        tr("ebisu_client_close"), QJsonObject(), this, "emitStopFlow");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::onUpdatePromptDone(QJsonObject obj)
{
#if FIX_FOR_X
#ifdef Q_OS_WIN 
	mFlashTimer.stop();
	if (mWindow)
		FlashWindow((HWND)mWindow->winId(), false);
#endif
#endif
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        const bool keepUpToDate = Services::JsonValueValidator::validate(obj["chkChecked"]).toBool();
        Services::writeSetting(Services::SETTING_AUTOPATCH, keepUpToDate, Engine::LoginController::currentUser()->getSession());
        const QString selectedOption = Services::JsonValueValidator::validate(obj["selectedOption"]).toString();
        UpdateExistAnswers answer = CancelLaunch;
        if(selectedOption.compare("PlayWithoutUpdate", Qt::CaseInsensitive) == 0)
        {
            answer = PlayWithoutUpdate;
        }
        else if(selectedOption.compare("PlayWhileUpdating", Qt::CaseInsensitive) == 0)
        {
            answer = PlayWhileUpdating;
        }
        else if(selectedOption.compare("UpdateCancelLaunch", Qt::CaseInsensitive) == 0)
        {
            answer = UpdateCancelLaunch;
        }
        emit updateExistAnswer(answer);
    }
    else
{
        emit stopFlow();
    }
}



void PlayViewController::showPDLCDownloadingWarning()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";
    
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-progress", 
        tr("ebisu_client_dlc_download_warning_title"), tr("ebisu_client_dlc_download_warning_text"), 
        tr("ebisu_client_play_now"), tr("ebisu_client_cancel"), "yes", QJsonObject(), this, "onPdlcDownloadingWarningAnswer");
    mt.overrideId = mEntitlement->contentConfiguration()->productId() + "_PlayflowDownload";
    mt.rejectGroup = rejectGroup();

    // the integrator is the child object of the progress bar, so Qt will delete the integrator when appropriate
    QList<Engine::Content::EntitlementRef> activeChildren;
    if(mInDebug)
    {
        activeChildren.append(mEntitlement);
    }
    else
    {
        foreach(Engine::Content::EntitlementRef ent, mEntitlement->children())
        {
            if(ent->localContent() && ent->localContent()->installFlow() && ent->localContent()->installFlow()->operationType() != Downloader::kOperation_None)
            {
                activeChildren.append(ent);
            }
        }   
    }
    MultiContentProgressIntegrator* integrator = new MultiContentProgressIntegrator(activeChildren, mt.overrideId, this, mInDebug);
    ORIGIN_VERIFY_CONNECT(integrator, SIGNAL(PDLCDownloadsComplete(const QString&)), this, SLOT(showPDLCDownloadsComplete(const QString&)));
    ORIGIN_VERIFY_CONNECT(integrator, SIGNAL(updateValues(const QString&, const QJsonObject&)), this, SLOT(onUpdateDownloadProgress(const QString&, const QJsonObject&)));
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::showPDLCDownloadsComplete(const QString& overrideId)
{
    if(sender())
    {
        sender()->disconnect();
        sender()->deleteLater();
    }
    if(overrideId.count())
    {
        DialogController::instance()->closeMessageDialog(DialogController::SPA, overrideId);
    }
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        tr("ebisu_client_dlc_download_complete"), tr("ebisu_client_global_dlc_downloads_complete"), 
        tr("ebisu_client_play_now"), tr("ebisu_client_cancel"), "yes", QJsonObject(), this, "onPdlcDownloadingWarningAnswer");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::onPdlcDownloadingWarningAnswer(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    emit pdlcDownloadingWarningAnswer(accepted ? QDialog::Accepted : QDialog::Rejected);
}


void PlayViewController::showFirstLaunchMessageDialog()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

#if 0 //TODO FIXUP_ORIGINX
    closeWindow();
    mWindow = UIToolkit::OriginWindow::promptNonModalScrollable(UIToolkit::OriginScrollableMsgBox::NoIcon, mEntitlement->contentConfiguration()->displayName().toUpper(),
        mEntitlement->contentConfiguration()->gameLaunchMessage(), tr("ebisu_client_continue"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(onFirstLaunchMessageComplete(const int&)));
    mWindow->showWindow();
#endif
}


void PlayViewController::onFirstLaunchMessageComplete(const int& result)
{
    closeWindow();
    if (result == QDialog::Accepted)
    {
        Engine::Content::GamesController* gamesController = Engine::Content::GamesController::currentUserGamesController();
        if (gamesController)
        {
            gamesController->setFirstLaunchMessageShown(mEntitlement->contentConfiguration()->productId(), "0");
        }
        emit firstLaunchMessageComplete();
    }
    else
    {
        emit stopFlow();
    }
}


void PlayViewController::showStartingLimitedTrialDialog()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";
    
    const int secondsTillExpire = mEntitlement->contentConfiguration()->trialDuration() * 3600; // maybe this should be downloadExpiration?
    const QString timeLeftMessage = LocalizedDateFormatter().formatInterval(secondsTillExpire, LocalizedDateFormatter::Hours, LocalizedDateFormatter::Days);

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        tr("ebisu_client_free_trial_starting_timed_trial_title"), tr("ebisu_client_free_trial_starting_timed_trial_message_alt").arg(timeLeftMessage), 
        tr("ebisu_client_play_now"), tr("ebisu_client_cancel"), "yes", QJsonObject(), this, "onStartingTimedTrialDone");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::onStartingTimedTrialDone(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        emit trialCheckComplete();
    }
    else
    {
        emit stopFlow();
    }
}


void PlayViewController::showStartingTimedTrialDialog()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

    const int secondsTillExpire = mEntitlement->contentConfiguration()->trialDuration() * 3600;
    const QString timeLeftMessage = LocalizedDateFormatter().formatInterval(secondsTillExpire, LocalizedDateFormatter::Hours, LocalizedDateFormatter::Days);

#if 0 //TODO FIXUP_ORIGINX
    mWindow = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::NoIcon, mEntitlement->contentConfiguration()->displayName().toUpper(),
        tr("ebisu_subs_trial_window_launch_body").arg(timeLeftMessage), tr("ebisu_client_play_now"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    mWindow->setTitleBarText(tr("ebisu_subs_trial_window_launch_window_title"));
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);

    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SIGNAL(stopFlow()));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(accepted()), this, SIGNAL(trialCheckComplete()));

    mWindow->showWindow();
#endif
}


void PlayViewController::showOnlineRequiredForTrialDialog()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";
    
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        tr("ebisu_client_must_be_online_to_launch_vault_game_title"), tr("ebisu_client_free_trial_online_required_message"), 
        tr("ebisu_client_ok_uppercase"), QJsonObject(), this, "emitStopFlow");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::showUpgradeSaveGameWarning()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

#if 0 //TODO FIXUP_ORIGINX
    closeWindow();
    mWindow = UIToolkit::OriginWindow::promptNonModalScrollable(UIToolkit::OriginScrollableMsgBox::Info,
        tr("ebisu_client_upgrade_warning_header"), tr("ebisu_client_upgrade_warning_body"),
        tr("ebisu_client_ok_uppercase"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    mWindow->setTitleBarText(tr("ebisu_client_upgrade_warning_title"));

    QLabel* lblText = mWindow->scrollableMsgBox()->getTextLabel();
    lblText->setTextInteractionFlags((Qt::TextInteractionFlags)(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse));
    lblText->setOpenExternalLinks(false);
    ORIGIN_VERIFY_CONNECT(lblText, SIGNAL(linkActivated(const QString&)), this, SLOT(onLinkActivated(const QString&)));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(onUpgradeSaveGameWarningDone(const int&)));
    mWindow->showWindow();
#endif
}


void PlayViewController::onUpgradeSaveGameWarningDone(const int& result)
{
    closeWindow();
    if (result == QDialog::Accepted)
    {
        Engine::Content::GamesController* gamesController = Engine::Content::GamesController::currentUserGamesController();
        if (gamesController)
            gamesController->setUpgradedSaveGameWarningShown(mEntitlement->contentConfiguration()->productId(), true);
        emit upgradeSaveWarningComplete();
    }
    else
    {
        emit stopFlow();
    }
}


void PlayViewController::onLinkActivated(const QString& link)
{
    if (sender() == NULL)
        return;

    if (link.contains("openAccessFAQ", Qt::CaseInsensitive))
    {
        QString url = Services::readSetting(Services::SETTING_SubscriptionFaqURL, Services::Session::SessionService::currentSession()).toString();
        OriginApplication::instance().configCEURL(url);
        Services::PlatformService::asyncOpenUrl(QUrl::fromEncoded(url.toUtf8()));
        GetTelemetryInterface()->Metric_SUBSCRIPTION_FAQ_PAGE_REQUEST(sender()->objectName().toStdString().c_str());
    }
}


void PlayViewController::showGameUpdatingWarning()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";
    
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-progress", 
        tr("ebisu_client_updating_game_uppercase"), tr("ebisu_client_game_update_updating_message").arg("<strong>"+mEntitlement->contentConfiguration()->displayName()+"</strong>"),
        tr("ebisu_client_play_now"), tr("ebisu_client_cancel"), "yes", QJsonObject(), this, "onPlayGameWhileUpdatingDone");
    mt.overrideId = mEntitlement->contentConfiguration()->productId() + "_PlayflowUpdate";
    mt.rejectGroup = rejectGroup();

    // the integrator is the child object of the progress bar, so Qt will delete the integrator when appropriate
    QList<Engine::Content::EntitlementRef> entitlementList;
    entitlementList.append(mEntitlement);
    // Will get deleted with the progress bar
    MultiContentProgressIntegrator* multiProgress = new MultiContentProgressIntegrator(entitlementList, mt.overrideId, this, mInDebug);
    ORIGIN_VERIFY_CONNECT(multiProgress, SIGNAL(updateValues(const QString&, const QJsonObject&)), this, SLOT(onUpdateDownloadProgress(const QString&, const QJsonObject&)));
    ORIGIN_VERIFY_CONNECT(multiProgress, SIGNAL(PDLCDownloadsComplete(const QString&)), this, SLOT(showUpdateCompleteDialog(const QString&)));
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::onUpdateDownloadProgress(const QString& id, const QJsonObject& updateContent)
    {
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-progress", 
        tr("ebisu_client_updating_game_uppercase"), tr("ebisu_client_game_update_updating_message").arg("<strong>"+mEntitlement->contentConfiguration()->displayName()+"</strong>"),
        tr("ebisu_client_play_now"), tr("ebisu_client_cancel"), "yes", updateContent, this, "onPlayGameWhileUpdatingDone");
    mt.overrideId = id;
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->updateMessageDialog(DialogController::SPA, mt);
    }


void PlayViewController::showUpdateCompleteDialog(const QString& overrideId)
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";

    // Sender = MultiContentProgressIntegrator
    if(sender())
    {
        sender()->disconnect();
        sender()->deleteLater();
    }
    if(overrideId.count())
    {
        DialogController::instance()->closeMessageDialog(DialogController::SPA, overrideId);
    }
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        tr("ebisu_client_game_update_complete"),
        tr("ebisu_client_game_update_up_to_date").arg("<strong>" + mEntitlement->contentConfiguration()->displayName() + "</strong>"),
        tr("ebisu_client_play_now"), tr("ebisu_client_cancel"), "yes", QJsonObject(), this, "onPlayGameWhileUpdatingDone");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::show3PDDPlayDialog()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";
    
    QString gameType = "origin";
    QString platformName = "origin";
    switch(mEntitlement->contentConfiguration()->partnerPlatform())
    {
    case Origin::Services::Publishing::PartnerPlatformSteam:
        gameType = "steam";
        platformName = tr("ebisu_client_steam");
        break;
    case Origin::Services::Publishing::PartnerPlatformGamesForWindows:
        gameType = "gfwl";
        platformName = tr("ebisu_client_games_for_windows_live");
        break;
    case Origin::Services::Publishing::PartnerPlatformOtherExternal:
        gameType = "other";
        platformName = tr("ebisu_client_3PDD_an_external_application");
        break;
    default:
        // explicitly ignore other cases to keep clang quiet
        break;
    }
    GetTelemetryInterface()->Metric_3PDD_PLAY_DIALOG_SHOW();
    GetTelemetryInterface()->Metric_3PDD_PLAY_TYPE(gameType.toUtf8().data());

    const QString text = QString("%0<br><br><strong>%1</strong> %2")
        .arg(tr("ebisu_client_3PDD_launch_desc").arg("<strong>"+mEntitlement->contentConfiguration()->displayName()+"</strong>").arg(platformName))
        .arg(tr("ebisu_client_product_code_with_colon"))
        .arg(mEntitlement->contentConfiguration()->cdKey());

    QJsonObject content;
    content["checkeddefault"] = false;
    content["checkboxtext"] =  DialogController::MessageTemplate::toAttributeFriendlyText(tr("ebisu_client_dont_show_dialog"));
    QJsonObject persistingInfo;
    persistingInfo["monitoredInstall"] = mEntitlement->contentConfiguration()->monitorInstall();

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-singlecheckbox", tr("ebisu_client_play_through_uppercase").arg(platformName.toUpper()), 
        text, tr("ebisu_client_play"), tr("ebisu_client_cancel"), "yes", content, this, "on3PDDPlayDone", persistingInfo);
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::on3PDDPlayDone(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        const bool monitorInstall = Services::JsonValueValidator::validate(obj["monitoredInstall"]).toBool();
        const bool dontShow = Services::JsonValueValidator::validate(obj["chkChecked"]).toBool();
        Services::writeSetting(Services::SETTING_3PDD_PLAYDIALOG_SHOW, !dontShow, Engine::LoginController::currentUser()->getSession());
        if(monitorInstall == false)
            ClientFlow::instance()->minimizeClientToTray();
            }
    else
    {
        emit stopFlow();
    }

    emit play3PDDGameAnswer(accepted);
}


void PlayViewController::showOIGSettings()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";
    
    QJsonObject content;
    content["checkeddefault"] = true;
    content["checkboxtext"] = tr("ebisu_client_igo_mac_warning_checkbox").arg(tr("ebisu_client_igo_name"));
    const QString text = QString("%0<br><br>%1")
        .arg(tr("ebisu_client_igo_mac_warning_description").arg(tr("ebisu_client_igo_name")))
        .arg(Services::FormatToolTip(tr("ebisu_client_igo_mac_warning_tooltip")));
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-singlecheckbox", 
        tr("ebisu_client_igo_mac_warning_header_CAPS").arg(tr("ebisu_client_igo_name_CAPS")), 
        text,
        tr("ebisu_client_play_game"), tr("ebisu_client_cancel"), "yes", content, this, "onOigSettingsDone");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::onOigSettingsDone(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    const bool oigEnabled = Services::JsonValueValidator::validate(obj["chkChecked"]).toBool();
    emit oigSettingsAnswer(accepted ? QDialog::Accepted : QDialog::Rejected, oigEnabled);
    }


QString PlayViewController::determineTitle()
{
    QString title = "";
    if(mEntitlement->contentConfiguration()->monitorPlay())
        title = tr("ebisu_client_launching");
    else
    {
        QString platformText;
        if(mEntitlement->contentConfiguration()->partnerPlatform() == Services::Publishing::PartnerPlatformSteam)
            platformText = tr("ebisu_client_steam");
        else
            platformText = tr("ebisu_client_3PDD_an_external_application");
        platformText = platformText.toUpper();
        title = tr("ebisu_client_launching_through").arg(platformText);
    }
    return title;
}


QString PlayViewController::rejectGroup() const
{
    return mInDebug ? "" : mEntitlement->contentConfiguration()->productId() + "_PlayFlow";
}


void PlayViewController::showMultipleLaunchDialog()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";
    
    QStringList launchList;
    if(mInDebug && mEntitlement->localContent()->launchTitleList().count() < 2)
    {
        launchList << "Horse Illustrated Championship Season: The Ultimate Riding Adventure" << "Ready 2 Rumble Boxing: Round 2" << "Rumble Racing" << "Shox" << "SSX (2 copies)" << "SSX Tricky" << "SSX 3" << "Freekstyle" << "Def Jam Vendetta" 
            << "Need for Speed Underground" << "Need for Speed Pro Street" << "Lord of the Rings - The Two Towers" << "Enter the Matrix" 
            << "SOCOM US Navy Seals (with headset)" << "World Destruction League - Thunder Tanks" << "Tomb Raider Angel of Darkness" << "X Squad" << "Devil May Cry" 
            << "007 Nightfire" << "007 Agent Under Fire" << "Metal Gear Solid 2 - Sons of Liberty" << "Medal of Honor Frontline" 
            << "Medal of Honor Rising Sun" << "The Simpsons Game" << "Catwoman" << "Quake III - Revolution" << "Freedom Fighters" << "NBA Street" 
            << "NBA Street Volume 2" << "Knockout Kings 2001" << "Tiger Wood PGA Tour 2001" << "Tiger Wood PGA Tour 2002" 
            << "Tiger Wood PGA Tour 2003" << "March Madness 2002" << "NBA Live 2003" << "NHL 2001" << "NHL 2004 (2 copies)" << "FIFA 2001" << "Madden 2008";

    }
    else
    {
        launchList = mEntitlement->localContent()->launchTitleList();
    }

    QJsonArray options;
    for(int i = 0, numOptions = launchList.count(); i < numOptions; i++)
    {
        QJsonObject option;
        option["title"] = DialogController::MessageTemplate::toAttributeFriendlyText(launchList[i]);
        options.append(option);
    }

    QJsonObject content;
    content["dontshowtext"] = DialogController::MessageTemplate::toAttributeFriendlyText(tr("ebisu_client_dont_show_dialog"));
    content["options"] = options;

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-playmultilaunch", 
        tr("ebisu_client_launch_options_caps"),
        tr("ebisu_client_multilauncher_text").arg(mEntitlement->contentConfiguration()->displayName()),
        tr("ebisu_client_play"), tr("ebisu_client_cancel"), "yes", content, this, "onMultiLaunchTitleChoosen");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::onMultiLaunchTitleChoosen(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        const bool dontShow = Services::JsonValueValidator::validate(obj["dontShow"]).toBool();
        const int index = Services::JsonValueValidator::validate(obj["option"]).toString().toInt();
        if(mEntitlement->localContent()->launchTitleList().count())
        {
            QString launchTitle = mEntitlement->localContent()->launchTitleList().at(index);
            mEntitlement->localContent()->playFlow()->setLauncherByTitle(launchTitle);
            Engine::Content::GamesController* gamesController = Engine::Content::GamesController::currentUserGamesController();
            if(dontShow && gamesController)
                gamesController->setGameMultiLaunchDefault(mEntitlement->contentConfiguration()->productId(), launchTitle);
        }
        emit launchQueueEmpty();
    }
    else
    {
        emit stopFlow();
        }
    }


void PlayViewController::showDefaultLaunchDialog()
{
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        determineTitle(),
        "<strong>" + mEntitlement->contentConfiguration()->displayName() + "</strong>",
        tr("ebisu_client_cancel"), QJsonObject(), this, "emitStopFlow");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
    QTimer::singleShot(LAUNCHING_TIME_OUT, this, SIGNAL(launchQueueEmpty()));
}


void PlayViewController::showInstallFlowResuming()
{
    // If an alternate launch software is currently running and caused the
    // game to launch which resulted in this error, let's minimize the 
    // launcher so the user can resolve the issue.
    Origin::SDK::SDK_ServiceArea::instance()->MinimizeRequest();
    ORIGIN_LOG_DEBUG << "MinimizeRequest";
    
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", 
        tr("ebisu_client_launching"),
        tr("ebisu_client_resuming_download_of_game").arg("<strong>" + mEntitlement->contentConfiguration()->displayName() + "</strong>"),
        tr("ebisu_client_cancel"), QJsonObject(), this, "emitStopFlow");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void PlayViewController::onPlayGameWhileUpdatingDone(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    emit playGameWhileUpdatingAnswer(accepted ? QDialog::Accepted : QDialog::Rejected);
}


void PlayViewController::showDownloadDebugDialog()
{
    MainFlow::instance()->contentFlowController()->localContentViewController()->showDownloadDebugInfo(mEntitlement);
}


void PlayViewController::showUpdateDebugDialog()
{
    MainFlow::instance()->contentFlowController()->localContentViewController()->showUpdateDebugInfo(mEntitlement);
}

}
}
