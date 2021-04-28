/////////////////////////////////////////////////////////////////////////////
// LocalContentViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "LocalContentViewController.h"
#include "engine/cloudsaves/LocalStateBackup.h"
#include "engine/content/ContentController.h"
#include "engine/content/GamesController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/content/PlayFlow.h"
#include "engine/igo/IGOController.h"
#include "engine/subscription/SubscriptionManager.h"
#include "Service/SDKService/SDK_ServiceArea.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformJumplist.h"
#include "originbanner.h"
#include "originscrollablemsgbox.h"
#include "originwindow.h"
#include "originlabel.h"
#include "originwindowmanager.h"
#include "GamePropertiesViewControllerManager.h"
#include "TelemetryAPIDLL.h"
#include "services/settings/SettingsManager.h"
#include "StoreUrlBuilder.h"
#include "widgets/debug/source/DownloadDebugViewController.h"
#include "widgets/debug/source/UpdateDebugViewController.h"
#include "DownloadDebugView.h"
#include "UpdateDebugView.h"
#include "TelemetryAPIDLL.h"
#include "ClientFlow.h"
#include "OriginToastManager.h"
#include "ui_RtpNotEntitledDialog.h"
#include <QCheckBox>
#include "OriginApplication.h"
#include "LocalizedDateFormatter.h"
#include "DialogController.h"

namespace Origin
{
namespace Client
{

LocalContentViewController::LocalContentViewController(QObject* parent)
: QObject(parent)
{
    ORIGIN_VERIFY_CONNECT_EX(SDK::SDK_ServiceArea::instance(), SIGNAL(trialOffline(const Origin::Engine::Content::EntitlementRef)), this, SLOT(showTimedTrialOffline(const Origin::Engine::Content::EntitlementRef)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(SDK::SDK_ServiceArea::instance(), SIGNAL(trialExpired(const Origin::Engine::Content::EntitlementRef)), this, SLOT(closeTimedTrialEnding(const Origin::Engine::Content::EntitlementRef)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(Services::Connection::ConnectionStatesService::instance(), SIGNAL(nowOnlineGlobal()), this, SLOT(closeAllTimedTrialOffline()), Qt::QueuedConnection);
}


LocalContentViewController::~LocalContentViewController()
{
    UIToolkit::OriginWindow* dialog = NULL;
    while (!mWindowMap.empty())
    {
        QMap<QString, Origin::UIToolkit::OriginWindow*>::iterator i = mWindowMap.begin();
        dialog = (*i);
        // Remove it from the map
        ORIGIN_VERIFY_DISCONNECT(dialog, NULL, this, NULL);
        mWindowMap.erase(i);
        if (dialog)
            dialog->deleteLater();
    }
}


void LocalContentViewController::connectEntitlement(Origin::Engine::Content::EntitlementRef entitlement)
{
    ORIGIN_VERIFY_CONNECT_EX(entitlement->localContent(), SIGNAL(error(Origin::Engine::Content::EntitlementRef, int)), this, SLOT(showError(Origin::Engine::Content::EntitlementRef, int)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(entitlement->localContent(), SIGNAL(expireNotify(int)), this, SLOT(showTrialEnding(int)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(entitlement->localContent(), SIGNAL(timedTrialExpireNotify(const int&)), this, SLOT(showTimedTrialEnding(const int&)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(entitlement->localContent(), SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)), this, SLOT(closeTimedTrialEnding(const Origin::Engine::Content::EntitlementRef)), Qt::QueuedConnection);
}


void LocalContentViewController::showError(Origin::Engine::Content::EntitlementRef ent, int error)
{
    if(ent)
    {
        const QString productId = ent->contentConfiguration()->productId();
        QString title, text;
        switch(static_cast<Engine::Content::LocalContent::Error>(error))
        {
        case Engine::Content::LocalContent::ErrorFolderInUse:
            {
                title = tr("ebisu_client_download_overlapping_job_title");
                bool useDefaultMsg = true;
                QList<Origin::Engine::Content::EntitlementRef> children = ent->children();
                for(QList<Origin::Engine::Content::EntitlementRef>::const_iterator it = children.constBegin(); it != children.constEnd(); it++)
                {
                    if((*it)->localContent()
                        && (*it)->localContent()->installFlow() 
                        && (*it)->localContent()->installFlow()->isActive())
                    {
                        useDefaultMsg = false;
                        const QString parentName = "<strong>" + ent->contentConfiguration()->installationDirectory() + "</strong>";
                        text = tr("ebisu_client_download_overlapping_job_2_error")
                            .arg(parentName)
                            .arg(tr("ebisu_client_colon_placement").arg(parentName).arg("<strong>"+(*it)->contentConfiguration()->displayName()+"</strong>"));
                        break;
                    }
                }

                if(useDefaultMsg)
                {
                    text = tr("ebisu_client_download_overlapping_job_error");
                }
                DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", title, text, tr("ebisu_client_close")));
            }
            break;
        case Engine::Content::LocalContent::ErrorTimedTrialAccount:
        {
            title = tr("ebisu_subs_trial_account_used_notification");
            text = tr("ebisu_client_free_trial_used_on_account_message").arg(ent->contentConfiguration()->displayName());
//TODO FIXUP_ORIGINX
#if 0
            window = UIToolkit::OriginWindow::alertNonModal(icon, title, text, tr("ebisu_client_close"));
            window->setTitleBarText(tr("ebisu_subs_trial_window_launch_window_title"));
#endif
        }
        break;
        case Engine::Content::LocalContent::ErrorTimedTrialSystem:
        {
            title = tr("ebisu_subs_trial_pc_used_notification");
            text = tr("ebisu_client_free_trial_used_on_system_message").arg(ent->contentConfiguration()->displayName());
//TODO FIXUP_ORIGINX
#if 0
            window = UIToolkit::OriginWindow::alertNonModal(icon, title, text, tr("ebisu_client_close"));
            window->setTitleBarText(tr("ebisu_subs_trial_window_launch_window_title"));
#endif
        }
        break;
        case Engine::Content::LocalContent::ErrorGameTimeAccount:
            {
                title = tr("ebisu_client_free_trial_used_on_account_titlebar").arg(ent->contentConfiguration()->displayName().toUpper());
                text = tr("ebisu_client_free_trial_used_on_account_message").arg(ent->contentConfiguration()->displayName());
                DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", title, text, tr("ebisu_client_close")));
            }
            break;
        case Engine::Content::LocalContent::ErrorGameTimeSystem:
            {
                title = tr("ebisu_client_free_trial_used_on_system_titlebar").arg(ent->contentConfiguration()->displayName().toUpper());
                text = tr("ebisu_client_free_trial_used_on_system_message").arg(ent->contentConfiguration()->displayName());
                text = text.replace("\"", "'");
                DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", title, text, tr("ebisu_client_close")));
            }
            break;
        case Engine::Content::LocalContent::ErrorCOPPADownload:
            {
                title = tr("ebisu_client_unable_to_download_game_caps");
                text = tr("ebisu_client_cannot_download_game_coppa_description");
                
                QString downloadHelpUrl = Origin::Services::readSetting(Services::SETTING_COPPADownloadHelpURL);
                downloadHelpUrl.replace("{offerId}", QUrl::toPercentEncoding(ent->contentConfiguration()->productId()));
                if(!downloadHelpUrl.isEmpty())
                {
                    text += QString("<br><br>") + tr("ebisu_client_cannot_download_game_coppa_helplink").arg(downloadHelpUrl) ;
                }
                DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", title, text, tr("ebisu_client_close")));
            }
            break;
        case Engine::Content::LocalContent::ErrorCOPPAPlay:
            {
                title = tr("ebisu_client_unable_to_launch_game_CAPS");
                text = tr("ebisu_client_cannot_launch_game_coppa_description");

                QString playHelpUrl = Origin::Services::readSetting(Services::SETTING_COPPAPlayHelpURL);
                playHelpUrl.replace("{offerId}", QUrl::toPercentEncoding(ent->contentConfiguration()->productId()));
                if(!playHelpUrl.isEmpty())
                {
                    text += QString("<br><br>") + tr("ebisu_client_cannot_launch_game_coppa_helplink").arg(playHelpUrl) ;
                }
                DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", title, text, tr("ebisu_client_close")));
            }
            break;
        case Engine::Content::LocalContent::ErrorNothingUpdated:
            {
                // If the play flow is running PlayViewController::showUpdateCompleteDialog is most likely showing
                if(ent->localContent()->playFlow()->isRunning() == false)
                {
                title = tr("ebisu_client_youre_all_set_title");
                    text = tr("ebisu_client_game_update_up_to_date").arg(ent->contentConfiguration()->displayName());
                    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", title, text, tr("ebisu_client_close")));
                }
            }
            break;
        case Engine::Content::LocalContent::ErrorForbidden:
            {
                title = (ent->localContent()->installFlow() && ent->localContent()->installFlow()->isUsingLocalSource()) ? tr("ebisu_client_ito_installation_forbidden_error_title") : tr("ebisu_client_download_forbidden_error_title");
                text = tr("ebisu_client_internal_test_code_description").arg(ent->contentConfiguration()->displayName());
                DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", title, text, tr("ebisu_client_close")));
            }
            break;
        case Engine::Content::LocalContent::ErrorParentNeedsUpdate:
            {
                title = tr("ebisu_client_game_update_required_title");
                text = tr("ebisu_client_game_update_required_description").arg(ent->contentConfiguration()->displayName()).arg(ent->parent()->contentConfiguration()->displayName());
                QJsonObject persisting;
                persisting["productId"] = productId;
                DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", 
                    title, text, tr("ebisu_client_update_game_button"), tr("ebisu_client_cancel"), "yes", QJsonObject(), this, "updateParentAndDownload", persisting));
            }
            break;
        default:
            break;
        }
    }
}


void LocalContentViewController::showUACFailed()
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", 
        tr("ebisu_client_unable_to_download_allcaps"), 
        tr("ebisu_client_failed_enable_uac").replace('"', '\"'),
        tr("ebisu_client_retry"), tr("ebisu_client_cancel"), "yes", QJsonObject(), this, "onRetryDownloads"));
}

void LocalContentViewController::showBrokenContentWarning()
{
    UIToolkit::OriginWindow* window = 
        UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::Error, tr("ebisu_client_broken_games_detected_title"), 
        tr("ebisu_client_broken_games_detected_description"), 
        tr("ebisu_client_repair"), tr("ebisu_client_later"), QDialogButtonBox::Yes);
    ORIGIN_VERIFY_CONNECT(window, SIGNAL(accepted()), Engine::Content::ContentController::currentUserContentController(), SLOT(processBrokenContentItems()));
    ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), Engine::Content::ContentController::currentUserContentController(), SLOT(processBrokenContentItemsRejected()));
    window = pushAndShowWindow(window, "Broken_Content_Warning");
    if(window)
    {
        window->raise();
        window->activateWindow();
    }
}


void LocalContentViewController::onRetryDownloads(QJsonObject result)
{
    const bool accepted = Services::JsonValueValidator::validate(result["accepted"]).toBool();
    if(accepted == false)
        return;
    Engine::Content::ContentController::currentUserContentController()->processDownloaderAutostartItems();
}


void LocalContentViewController::showQueueHeadBusyDialog(Origin::Engine::Content::EntitlementRef head, Origin::Engine::Content::EntitlementRef ent)
{
    QString text = tr("origin_client_busy_wait_before_continuing");
    QString headproductid = "";
    const QString entProductid = ent->contentConfiguration()->productId();
    QString head_opt = "unknown";
    QString ent_opt = "unknown";
    if(head.isNull() == false && head->localContent() && head->localContent()->installFlow())
    {
        const QString headTitle = head->contentConfiguration()->displayName();
        const QString gameTitle = ent->contentConfiguration()->displayName();
        headproductid = head->contentConfiguration()->productId();

        const Downloader::ContentOperationType opt_type = (ent.isNull() == false && ent->localContent() && ent->localContent()->installFlow()) ? ent->localContent()->installFlow()->operationType() : Downloader::kOperation_None;

        if(ent.isNull() == false && ent->localContent() && ent->localContent()->playFlow() && ent->localContent()->playFlow().data()->isRunning())
        {
            ent_opt = "play";
            switch(head->localContent()->installFlow()->operationType())
            {
            case Downloader::kOperation_Repair:
                head_opt = "repair";
                text = tr("origin_client_busy_repairing_wait_before_launch").arg(headTitle).arg(gameTitle);
                break;
            case Downloader::kOperation_Update:
                head_opt = "update";
                text = tr("origin_client_busy_checkingforupdates_wait_before_launch").arg(headTitle).arg(gameTitle);
                break;
            case Downloader::kOperation_Install:
                head_opt = "install";
                text = tr("origin_client_busy_installing_wait_before_launch").arg(headTitle).arg(gameTitle);
                break;
            case Downloader::kOperation_ITO:
                head_opt = "ito";
                text = tr("origin_client_busy_installing_wait_before_launch").arg(headTitle).arg(gameTitle);
                break;
            default:
                if(head->localContent()->playing())
                {
                    head_opt = "playing";
                    text = tr("origin_client_busy_playing_wait_before_launch").arg(headTitle).arg(gameTitle);
                }
                break;
            }
        }
        else if(opt_type == Downloader::kOperation_Install || opt_type == Downloader::kOperation_ITO)
        {
            if(opt_type == Downloader::kOperation_Install)
                ent_opt = "install";
            else if(opt_type == Downloader::kOperation_ITO)
                ent_opt = "ito";
            switch(head->localContent()->installFlow()->operationType())
            {
            case Downloader::kOperation_Repair:
                head_opt = "repair";
                text = tr("origin_client_busy_repairing_wait_before_install").arg(headTitle).arg(gameTitle);
                break;
            case Downloader::kOperation_Update:
                head_opt = "update";
                text = tr("origin_client_busy_checkingforupdates_wait_before_install").arg(headTitle).arg(gameTitle);
                break;
            case Downloader::kOperation_Install:
                head_opt = "install";
                text = tr("origin_client_busy_installing_wait_before_install_another").arg(headTitle).arg(gameTitle);
                break;
            case Downloader::kOperation_ITO:
                head_opt = "ito";
                text = tr("origin_client_busy_installing_wait_before_install_another").arg(headTitle).arg(gameTitle);
                break;
            default:
                if(head->localContent()->playing())
                {
                    head_opt = "playing";
                    text = tr("origin_client_busy_playing_wait_before_install").arg(headTitle).arg(gameTitle);
                }
                break;
            }
        }
    }
    GetTelemetryInterface()->Metric_QUEUE_HEAD_BUSY_WARNING_SHOWN(headproductid.toUtf8(), head_opt.toUtf8(), entProductid.toUtf8(), ent_opt.toUtf8());
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("origin_client_wait_a_second_caps_ellipsis"), text, tr("ebisu_client_ok_uppercase")));
}


void LocalContentViewController::showSubscriptionEntitleError(const QString& offerId, Engine::Subscription::SubscriptionRedemptionError error)
{
    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(offerId, Engine::Content::AllContent);
    if(entitlement.isNull())
    {
        ORIGIN_LOG_EVENT << "Subscription Error - not showing error because entitlement couldn't be found: " << offerId;
        return;
    }

    QString text = (Engine::Subscription::RedemptionErrorAgeRestricted == error) ?
        tr("subs_ebisu_client_subscription_entitle_error_age_restricted_body") :
        tr("subs_ebisu_client_subscription_entitle_error_generic_body");

    UIToolkit::OriginWindow* window = 
        UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, tr("subs_ebisu_client_subscription_entitle_error_generic_title"), 
        text.arg("<b>" + entitlement->contentConfiguration()->displayName() + "</b>"), 
        tr("ebisu_client_close"));
    pushAndShowWindow(window, offerId + " Subscription_EntitleError");
    ORIGIN_LOG_EVENT << "Subscription Error - showing subscription error for entitlement: " << offerId;
}


void LocalContentViewController::showSubscriptionRemoveError(const QString& masterTitleId, Engine::Subscription::SubscriptionRedemptionError error)
{
    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementById(masterTitleId);
    if (entitlement.isNull())
    {
        ORIGIN_LOG_EVENT << "Subscription Error - not showing error because entitlement couldn't be found: " << masterTitleId;
        return;
    }
    ORIGIN_LOG_EVENT << "Subscription Error - Remove Error " << masterTitleId;
    UIToolkit::OriginWindow* window = 
        UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, tr("ebisu_subscriber_revoke_error_title"),
        tr("ebisu_subscriber_revoke_error_text").arg(entitlement->contentConfiguration()->displayName()),
        tr("ebisu_client_close"));
    pushAndShowWindow(window, masterTitleId + " Subscription_RemoveError");
    ORIGIN_LOG_EVENT << "Subscription Error - showing subscription error for entitlement: " << masterTitleId;
}


void LocalContentViewController::showSubscriptionDowngradeError(const QString& masterTitleId, Engine::Subscription::SubscriptionRedemptionError error)
{
    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementById(masterTitleId);
    if (entitlement.isNull())
    {
        ORIGIN_LOG_EVENT << "Subscription Error - not showing error because entitlement couldn't be found: " << masterTitleId;
        return;
    }

    ORIGIN_LOG_EVENT << "Subscription Error - Downgrade Error " << masterTitleId;
    UIToolkit::OriginWindow* window = 
        UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, tr("ebisu_subscriber_downgrade_error_title"),
        tr("ebisu_subscriber_revoke_error_text").arg(entitlement->contentConfiguration()->displayName()),
        tr("ebisu_client_close"));
    pushAndShowWindow(window, masterTitleId + " Subscription_DowngradeError");
    ORIGIN_LOG_EVENT << "Subscription Error - showing subscription error for entitlement: " << masterTitleId;
}


void LocalContentViewController::showSubscriptionUpgradeError(const QString& masterTitleId, Engine::Subscription::SubscriptionRedemptionError error)
{
    Engine::Content::EntitlementRefList entitlements = Engine::Content::ContentController::currentUserContentController()->entitlementsBaseByMasterTitleId(masterTitleId);
    if (entitlements.empty())
    {
        ORIGIN_LOG_EVENT << "Subscription Error - not showing error because entitlement couldn't be found: " << masterTitleId;
        return;
    }

    Engine::Content::EntitlementRef entitlement = entitlements.first();
    if (entitlement.isNull())
    {
        ORIGIN_LOG_EVENT << "Subscription Error - not showing error because entitlement couldn't be found: " << masterTitleId;
        return;
    }

    QString text = (Engine::Subscription::RedemptionErrorAgeRestricted == error) ?
        tr("subs_ebisu_client_subscription_upgrade_error_age_restricted_body") :
        tr("subs_ebisu_client_subscription_upgrade_error_generic_body");

    UIToolkit::OriginWindow* window = 
        UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, tr("subs_ebisu_client_subscription_upgrade_error_generic_title"),
        text.arg("<b>" + entitlement->contentConfiguration()->displayName() + "</b>"),
        tr("ebisu_client_close"));
    pushAndShowWindow(window, masterTitleId + " Subscription_UpgradeError");
    ORIGIN_LOG_EVENT << "Subscription Error - showing subscription error for entitlement: " << masterTitleId;
}


bool LocalContentViewController::containsAndShowWindowIfAlive(const QString& key)
{
    UIToolkit::OriginWindow* window = NULL;
    if(mWindowMap.contains(key))
    {
        window = mWindowMap.value(key);
        window->showWindow();
    }
    return window != NULL;
}


UIToolkit::OriginWindow *LocalContentViewController::pushAndShowWindow(UIToolkit::OriginWindow* window, const QString& key, const bool& showInOIG)
{
    if(mWindowMap.contains(key))
    {
        // If it's a duplicate, delete the one that was passed in.
        window->disconnect();
        window->deleteLater();
        window = mWindowMap.value(key);
    }
    else
    {
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(finished(int)), this, SLOT(onRemoveWindow()));
        mWindowMap.insert(key, window);
    }
    if(showInOIG && Engine::IGOController::instance()->isActive())
    {
        window->configForOIG();
        Engine::IGOController::instance()->igoShowError(window);
    }
    window->showWindow();
    return window;
}


void LocalContentViewController::onRemoveWindow()
{
    UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(this->sender());
    if(window)
    {
        const QString key = mWindowMap.key(window);
        mWindowMap.remove(key);
        ORIGIN_VERIFY_DISCONNECT(window, NULL, this, NULL);
        window->close();
    }
}


void LocalContentViewController::showGameNotInstalled(Engine::Content::EntitlementRef entitlement)
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_r2p_error_title"), 
        tr("ebisu_client_r2p_game_should_be_installed").arg(entitlement->contentConfiguration()->displayName()), 
        tr("ebisu_client_close")));
}


void LocalContentViewController::showGameProperties(Engine::Content::EntitlementRef entitlement)
{
    using namespace UIToolkit;
    OriginWindow* window = OriginWindow::promptNonModalScrollable(OriginScrollableMsgBox::NoIcon, tr("ebisu_client_game_properties_for_caps").arg(entitlement->contentConfiguration()->displayName().toUpper()), "", tr("ebisu_client_ok_uppercase"), tr("ebisu_client_cancel"), QDialogButtonBox::NoButton);
    window->scrollableMsgBox()->setHasDynamicWidth(true);
    GamePropertiesViewController* content = new GamePropertiesViewController(entitlement);
    if(entitlement->localContent()->playing())
    {
        window->setBannerText(tr("ebisu_client_you_need_to_restart_game"));
        window->setBannerIconType(OriginBanner::Info);
        window->setBannerVisible(true);
    }
    ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Yes), SIGNAL(clicked()), content, SLOT(onApplyChanges()));
    window->setContent(content);
    pushAndShowWindow(window, entitlement->contentConfiguration()->productId() + " Properties");
}


void LocalContentViewController::showParentNotInstalledPrompt(Engine::Content::EntitlementRef entitlement)
{
    if(entitlement && entitlement->parent() && entitlement->parent()->localContent())
    {
        Origin::Engine::Content::LocalContent* parentLocalContent = entitlement->parent()->localContent();
        const QString text = tr("ebisu_client_requires_base_game_text")
                                .arg(entitlement->parent()->contentConfiguration()->displayName())
                                .arg(entitlement->contentConfiguration()->displayName());

        QString okBtnText = "";
        if(parentLocalContent->downloadable())
            okBtnText = tr("ebisu_client_ite_disk_read_error_download_game");
        else if(parentLocalContent->installable())
            okBtnText = tr("ebisu_client_install_game");

        if(okBtnText == "")
        {
            DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_requires_base_game"), 
                text, tr("ebisu_client_ok_uppercase")));
        }
        else
        {
            parentLocalContent->appendPendingPurchasedDLCDownloads(entitlement->contentConfiguration()->productId());

            QJsonObject persisting;
            persisting["productId"] = entitlement->parent()->contentConfiguration()->productId();
            DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", 
                tr("ebisu_client_requires_base_game"), text, okBtnText, tr("ebisu_client_cancel"), "yes", QJsonObject(), this, "updateParentAndDownload", persisting));
        }
    }

        }


void LocalContentViewController::onDownloadOrInstallParent(QJsonObject result)
{
    const bool accepted = Services::JsonValueValidator::validate(result["accepted"]).toBool();
    if(accepted == false)
        return;
    const QString productId = Services::JsonValueValidator::validate(result["productId"]).toString();
    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productId);
    if(entitlement.isNull() == false)
    {
        if(entitlement->localContent()->downloadable())
            entitlement->localContent()->download("");
        else if(entitlement->localContent()->installable())
            entitlement->localContent()->install();
    }
}


void LocalContentViewController::showServerSettingsUnsavable()
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_hang_on_title"), 
        tr("ebisu_client_unable_to_change_server_setting_text"), tr("ebisu_client_ok_uppercase")));
}


void LocalContentViewController::showRemoveEntitlementPrompt(Engine::Content::EntitlementRef entitlement, const bool& showUninstall)
{
    using namespace Origin::UIToolkit;
    OriginWindow* window = NULL;
    QString text = tr("Subs_Library_TitleContextMenu_RemoveDialog_Title_Body");
    text.remove("\\");
    if(showUninstall)
    {
        window = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
            NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Cancel);
        window->scrollableMsgBox()->setup(OriginScrollableMsgBox::NoIcon, 
            tr("Subs_Library_TitleContextMenu_RemoveDialog_Title_Header").arg(entitlement->contentConfiguration()->displayName().toUpper()),
            text);
        window->setTitleBarText(tr("ebisu_client_remove_from_library"));
        QLabel* lblText = window->scrollableMsgBox()->getTextLabel();
        lblText->setObjectName(entitlement->contentConfiguration()->masterTitleId());
        lblText->setTextInteractionFlags((Qt::TextInteractionFlags)(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse));
        lblText->setOpenExternalLinks(false);
        ORIGIN_VERIFY_DISCONNECT(window, SIGNAL(closeWindow()), window, SIGNAL(rejected()));
        ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), window, SLOT(reject()));
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(closeWindow()), window, SLOT(reject()));
        ORIGIN_VERIFY_CONNECT(lblText, SIGNAL(linkActivated(const QString&)), this, SLOT(onLinkActivated(const QString&)));

        QWidget* content = new QWidget();
        Ui::RtpNotEntitledDialog ui;
        ui.setupUi(content);

        ui.link1->setText(tr("Subs_RemoveGameDialog_UninstallandRemove_Header"));
        ui.link1->setDescription(tr("Subs_RemoveGameDialog_UninstallandRemove_Body"));
        ui.link2->setText(tr("Subs_RemoveGameDialog_RemoveOnly_Header"));
        ui.link2->setDescription(tr("Subs_RemoveGameDialog_RemoveOnly_Body"));
        ui.link3->setText(tr("Subs_RemoveGameDialog_UninstallOnly_Header"));
        ui.link3->setDescription(tr("Subs_RemoveGameDialog_UninstallOnly_Body"));

        window->setContent(content);

        QSignalMapper* signalMapper3 = new QSignalMapper(window);
        signalMapper3->setMapping(ui.link1, entitlement->contentConfiguration()->productId());
        signalMapper3->setMapping(ui.link3, entitlement->contentConfiguration()->productId());
        ORIGIN_VERIFY_CONNECT_EX(ui.link1, SIGNAL(clicked()), signalMapper3, SLOT(map()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(ui.link3, SIGNAL(clicked()), signalMapper3, SLOT(map()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(signalMapper3, SIGNAL(mapped(const QString&)), this, SLOT(onUninstall(const QString&)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(ui.link3, SIGNAL(clicked()), window, SLOT(accept()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(ui.link1, SIGNAL(clicked()), window, SLOT(accept()), Qt::QueuedConnection);

        QSignalMapper* signalMapper2 = new QSignalMapper(window);
        signalMapper2->setMapping(ui.link1, entitlement->contentConfiguration()->masterTitleId());
        signalMapper2->setMapping(ui.link2, entitlement->contentConfiguration()->masterTitleId());
        ORIGIN_VERIFY_CONNECT_EX(ui.link1, SIGNAL(clicked()), signalMapper2, SLOT(map()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(ui.link2, SIGNAL(clicked()), signalMapper2, SLOT(map()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(signalMapper2, SIGNAL(mapped(const QString&)), this, SLOT(onRemoveEntitlement(const QString&)), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(ui.link2, SIGNAL(clicked()), window, SLOT(accept()), Qt::QueuedConnection);

        Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();
        if (contentController)
        {
            ui.link2->setVisible(contentController->entitlementsBaseByMasterTitleId_ExcludeSubscription(entitlement->contentConfiguration()->masterTitleId()).length() > 0);
        }
    }
    else
    {
        window = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::NoIcon, tr("Subs_Library_TitleContextMenu_RemoveDialog_Title_Header").arg(entitlement->contentConfiguration()->displayName().toUpper()), 
            text, tr("ebisu_client_remove_game"), tr("ebisu_client_cancel"));
        window->setTitleBarText(tr("ebisu_client_remove_from_library"));
        QLabel* lblText = window->msgBox()->getTextLabel();
        lblText->setObjectName(entitlement->contentConfiguration()->productId());
        lblText->setTextInteractionFlags((Qt::TextInteractionFlags)(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse));
        lblText->setOpenExternalLinks(false);
        QSignalMapper* signalMapper = new QSignalMapper(window);
        signalMapper->setMapping(window, entitlement->contentConfiguration()->masterTitleId());
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(accepted()), signalMapper, SLOT(map()));
        ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(const QString&)), this, SLOT(onRemoveEntitlement(const QString&)));
        ORIGIN_VERIFY_CONNECT(lblText, SIGNAL(linkActivated(const QString&)), this, SLOT(onLinkActivated(const QString&)));
    }
    pushAndShowWindow(window, entitlement->contentConfiguration()->productId() + " RemoveEntitlement");
}


void LocalContentViewController::showCanNotRestoreFromTrash(Engine::Content::EntitlementRef entitlement)
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_unable_to_restore_game_uppercase"), 
        tr("ebisu_client_mac_cannot_restore_body"), tr("ebisu_client_close")));
}  


void LocalContentViewController::showSignatureVerificationFailed()
{
    //UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, tr("ebisu_client_error_uppercase"), 
    //    tr("ebisu_client_error_occurred"), tr("ebisu_client_close"));
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_error_uppercase"), 
        tr("ebisu_client_error_occurred"), tr("ebisu_client_close")));
}


void LocalContentViewController::showTrialEnding(int timeRemainingInMinutes)
{
    Origin::Services::Session::SessionRef session =  Origin::Services::Session::SessionService::currentSession();
    Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();

    //lets make sure the session and content controller is still valid
    if(session && contentController)
    {

        Origin::Engine::Content::LocalContent* lc = dynamic_cast<Origin::Engine::Content::LocalContent*>(this->sender());

        if(lc && lc->playing() && lc->entitlement()->contentConfiguration()->isFreeTrial())
        {
            QString title;
            if(timeRemainingInMinutes > 1)
                title = tr("ebisu_client_free_trial_ending_title_minutes").arg(timeRemainingInMinutes);
            else
                title = tr("ebisu_client_free_trial_ending_title_minute").arg(timeRemainingInMinutes);
            DialogController::instance()->showMessageDialog(DialogController::OIG, DialogController::MessageTemplate("", title, 
                tr("ebisu_client_free_trial_ending_message"), 
                tr("ebisu_client_close")));

            // Show in IGO too
            QString message;
            if( timeRemainingInMinutes > 1 )
                message = tr("ebisu_client_free_trial_ending_notification_minutes").arg(timeRemainingInMinutes);
            else
                message = tr("ebisu_client_free_trial_ending_notification_minute").arg(timeRemainingInMinutes);
    
            emit Origin::Engine::IGOController::instance()->igoShowToast("POPUP_FREE_TRIAL_EXPIRING", message, "");
        }
    }
}


void LocalContentViewController::showTimedTrialEnding(const int& timeRemainingInMinutes)
{
    Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();
    Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();

    //lets make sure the session and content controller is still valid
    if (session && contentController)
    {
        Origin::Engine::Content::LocalContent* lc = dynamic_cast<Origin::Engine::Content::LocalContent*>(this->sender());

        if (lc && lc->playing())
        {
            const QString title = tr("ebisu_subs_trial_ending_notfication")
                .arg(LocalizedDateFormatter().formatInterval(timeRemainingInMinutes * 60, LocalizedDateFormatter::Minutes, LocalizedDateFormatter::Minutes, true))
                .arg(lc->entitlement()->contentConfiguration()->displayName().toUpper());
            UIToolkit::OriginWindow* window =
                UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Notice, title,
                tr("ebisu_subs_trial_ending_savegame_notification").arg(lc->entitlement()->contentConfiguration()->displayName()),
                tr("ebisu_client_close"));
            window->setTitleBarText(tr("ebisu_subs_trial_window_launch_window_title"));

            UIToolkit::OriginWindow* windowShown = pushAndShowWindow(window, "TimedTrialEnding_" + lc->entitlement()->contentConfiguration()->productId(), true);
            if (windowShown)
            {
                windowShown->msgBox()->setTitle(title);
            }

            QString toastText = tr("ebisu_subs_trial_ending_notification_upper").arg(LocalizedDateFormatter().formatInterval(timeRemainingInMinutes * 60, LocalizedDateFormatter::Minutes, LocalizedDateFormatter::Minutes));
            emit Origin::Engine::IGOController::instance()->igoShowToast("POPUP_FREE_TRIAL_EXPIRING", toastText, "");
        }
    }
}


void LocalContentViewController::closeTimedTrialEnding(const Engine::Content::EntitlementRef entitlement)
{
    if (entitlement && entitlement->contentConfiguration())
    {
        const QString key = "TimedTrialEnding_" + entitlement->contentConfiguration()->productId();
        UIToolkit::OriginWindow* window = mWindowMap.value(key);
        if (window)
        {
            window->close();
        }
    }
}


void LocalContentViewController::showTimedTrialOffline(const Origin::Engine::Content::EntitlementRef entitlement)
{
    const QString timeToShutDown = LocalizedDateFormatter().formatInterval(entitlement->localContent()->trialReconnectingTimeLeft()/1000, LocalizedDateFormatter::Seconds);
    const QString text = tr("ebisu_subs_trial_goonline_warning")
        .arg("<b>" + entitlement->contentConfiguration()->displayName() + "</b>")
        .arg(timeToShutDown);
    UIToolkit::OriginWindow* window =
        UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::Notice,
        tr("ebisu_subs_trial_online_notification_upper"),
        text,
        tr("ebisu_client_single_login_go_online"),
        tr("ebisu_client_close"));
    window->setTitleBarText(tr("ebisu_subs_trial_window_launch_window_title"));

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR << "ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR << "OriginToastManager not available";
        return;
    }

    otm->showToast("POPUP_FREE_TRIAL_EXPIRING", 
        tr("ebisu_subs_trial_connection_lost_warning")
            .arg(entitlement->contentConfiguration()->displayName())
            .arg(timeToShutDown),
        "");

    ORIGIN_VERIFY_CONNECT(window, SIGNAL(accepted()), clientFlow, SLOT(requestOnlineMode()));
    window = pushAndShowWindow(window, entitlement->contentConfiguration()->productId() + " showTimedTrialOffline", true);
    if (window && window->msgBox())
    {
        window->msgBox()->setText(text);
    }
    ClientFlow::instance()->silentRequestOnlineMode();
}


void LocalContentViewController::closeAllTimedTrialOffline()
{
    UIToolkit::OriginWindow* dialog = NULL;
    while (!mWindowMap.empty())
    {
        QMap<QString, Origin::UIToolkit::OriginWindow*>::iterator i = mWindowMap.begin();
        dialog = (*i);
        if (mWindowMap.key(dialog).contains("showTimedTrialOffline"))
        {
            // Remove it from the map
            ORIGIN_VERIFY_DISCONNECT(dialog, NULL, this, NULL);
            mWindowMap.erase(i);
            if (dialog)
                dialog->deleteLater();
        }
    }
}


void LocalContentViewController::showUninstallConfirmation(Engine::Content::EntitlementRef entitlement)
{
    QJsonObject persistingObj;
    persistingObj["productId"] = entitlement->contentConfiguration()->productId();
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("",
        tr("ebisu_client_confirm_uninstall_game").arg(entitlement->contentConfiguration()->displayName().toUpper()),
        tr("ebisu_client_confirm_uninstall_game_description"),
        tr("ebisu_client_uninstall"),
        tr("ebisu_client_cancel"),
        "no", QJsonObject(), this, "onUninstallConfirmationDone", persistingObj));

    // TELEMETRY: Uninstall clicked
    GetTelemetryInterface()->Metric_GAME_UNINSTALL_CLICKED(entitlement->contentConfiguration()->productId().toUtf8().data());
}


void LocalContentViewController::onUninstallConfirmationDone(QJsonObject result)
{
    const bool accepted = Services::JsonValueValidator::validate(result["accepted"]).toBool();
    const QString productId = Services::JsonValueValidator::validate(result["productId"]).toString();
    if(accepted)
    {
        onUninstall(productId);
    }
    else
    {
        onUninstallCancelled(productId);
    }
}

void LocalContentViewController::showUninstallConfirmationForDLC(Engine::Content::EntitlementRef entitlement)
{
    QJsonObject persistingObj;
    persistingObj["productId"] = entitlement->contentConfiguration()->productId();
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("",
        tr("ebisu_client_confirm_uninstall_game").arg(entitlement->contentConfiguration()->displayName().toUpper()),
        tr("ebisu_client_confirm_uninstall_dlc_description"),
        tr("ebisu_client_uninstall"),
        tr("ebisu_client_cancel"),
        "no", QJsonObject(), this, "onUninstallConfirmationDone", persistingObj));

    // TELEMETRY: Uninstall clicked
    GetTelemetryInterface()->Metric_GAME_UNINSTALL_CLICKED(entitlement->contentConfiguration()->productId().toUtf8().data());
}

void LocalContentViewController::showDownloadDebugInfo(Engine::Content::EntitlementRef entitlement)
{
    const QString key = entitlement->contentConfiguration()->productId() + " DownloadDebugInfo";
    if(containsAndShowWindowIfAlive(key) == false)
    {    
        DownloadDebugView* view = new DownloadDebugView();
        DownloadDebugViewController* ddvc = new DownloadDebugViewController(entitlement, view);

        using namespace UIToolkit;
        UIToolkit::OriginWindow* window = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
            NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Ok);
        window->setAttribute(Qt::WA_DeleteOnClose, false);
        window->setTitleBarText(tr("ebisu_client_notranslate_download_debug_title"));
        window->scrollableMsgBox()->setup(OriginScrollableMsgBox::NoIcon, entitlement->contentConfiguration()->displayName(), "");
        window->scrollableMsgBox()->setHasDynamicWidth(true);
        window->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_notranslate_export"));
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), ddvc, SLOT(deleteLater()));
        ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Ok), SIGNAL(clicked()), ddvc, SLOT(onExportRequested()));
        if(window->manager())
        {
            window->manager()->setupButtonFocus();
        }

        ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), this, SLOT(onRemoveWindow()));
        mWindowMap.insert(key, window);
        window->setContent(view);
        ORIGIN_LOG_EVENT << "User is viewing download debug dialog for " << entitlement->contentConfiguration()->productId();
        window->showWindow();
    }
}


void LocalContentViewController::showUpdateDebugInfo(Engine::Content::EntitlementRef entitlement)
{
    const QString key = entitlement->contentConfiguration()->productId() + " UpdateDebugInfo";
    if(containsAndShowWindowIfAlive(key) == false)
    {    
        UpdateDebugView* view = new UpdateDebugView();
        UpdateDebugViewController* udvc = new UpdateDebugViewController(entitlement, view);

        using namespace UIToolkit;
        const bool local = entitlement->contentConfiguration()->hasOverride();
        const QString titlebarText = tr("ebisu_client_notranslate_update_debug_title").arg(local ? tr("ebisu_client_notranslate_local") : tr("ebisu_client_notranslate_cdn"));
        UIToolkit::OriginWindow* window = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
            NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Ok);
        window->setAttribute(Qt::WA_DeleteOnClose, false);
        window->setTitleBarText(titlebarText);
        window->scrollableMsgBox()->setup(OriginScrollableMsgBox::NoIcon, tr("ebisu_client_notranslate_update_details"), "");
        window->scrollableMsgBox()->setHasDynamicWidth(true);
        window->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_notranslate_export"));
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), udvc, SLOT(deleteLater()));
        ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Ok), SIGNAL(clicked()), udvc, SLOT(onExportRequested()));
        if(window->manager())
        {
            window->manager()->setupButtonFocus();
        }

        ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), this, SLOT(onRemoveWindow()));
        mWindowMap.insert(key, window);
        window->setContent(view);
        window->showWindow();
        ORIGIN_LOG_EVENT << "User is viewing download debug dialog for " << entitlement->contentConfiguration()->productId();
    }
}


void LocalContentViewController::showUpgradeWarning(Engine::Content::EntitlementRef entitlement)
{
    UIToolkit::OriginWindow* window = UIToolkit::OriginWindow::promptNonModalScrollable(UIToolkit::OriginScrollableMsgBox::Info, 
        tr("ebisu_client_upgrade_warning_header"), tr("ebisu_client_upgrade_warning_body"), 
        tr("ebisu_client_upgrade_now"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    window->setTitleBarText(tr("ebisu_client_upgrade_warning_title"));

    QLabel* lblText = window->scrollableMsgBox()->getTextLabel();
    lblText->setObjectName(entitlement->contentConfiguration()->productId());
    lblText->setTextInteractionFlags((Qt::TextInteractionFlags)(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse));
    lblText->setOpenExternalLinks(false);
    ORIGIN_VERIFY_CONNECT(lblText, SIGNAL(linkActivated(const QString&)), this, SLOT(onLinkActivated(const QString&)));

    QSignalMapper* signalMapper = new QSignalMapper(window);
    signalMapper->setMapping(window->button(QDialogButtonBox::Yes), entitlement->contentConfiguration()->productId());
    ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Yes), SIGNAL(clicked()), signalMapper, SLOT(map()));
    ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(const QString&)), this, SLOT(onUpgrade(const QString&)));

    pushAndShowWindow(window, entitlement->contentConfiguration()->productId() + " UpgradeWarning");
}


void LocalContentViewController::showDLCUpgradeStandard(Engine::Content::EntitlementRef entitlement, const QString& dlcName)
{
    UIToolkit::OriginWindow* window = UIToolkit::OriginWindow::promptNonModalScrollable(UIToolkit::OriginScrollableMsgBox::Info, 
        tr("ebisu_client_dlc_upgrade_title"), tr("ebisu_client_dlc_upgrade_body").arg(dlcName),
        tr("ebisu_client_continue"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    window->setTitleBarText(tr("ebisu_client_upgrade_notice"));

    QSignalMapper* signalMapper = new QSignalMapper(window);
    signalMapper->setMapping(window->button(QDialogButtonBox::Yes), entitlement->contentConfiguration()->productId());
    ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Yes), SIGNAL(clicked()), signalMapper, SLOT(map()));
    ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(const QString&)), this, SLOT(onUpgrade(const QString&)));

    pushAndShowWindow(window, entitlement->contentConfiguration()->productId() + " DLC_Standard_Upgrade");
}


void LocalContentViewController::showSaveBackupRestoreWarning(Engine::Content::EntitlementRef entitlement)
{
    Engine::CloudSaves::LocalStateBackup localStateBackup = Engine::CloudSaves::LocalStateBackup(entitlement, Engine::CloudSaves::LocalStateBackup::BackupForSubscriptionUpgrade);
    QFileInfo file(localStateBackup.backupPath());

    UIToolkit::OriginWindow* window = UIToolkit::OriginWindow::promptNonModalScrollable(UIToolkit::OriginScrollableMsgBox::NoIcon,
        tr("ebisu_client_cloud_restore_header").arg(LocalizedDateFormatter().format(file.created(), LocalizedDateFormatter::ShortFormat)),
        tr("ebisu_client_cloud_restore_description"),
        tr("ebisu_client_restore"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    window->setTitleBarText(tr("ebisu_client_cloud_restore_title"));
    QSignalMapper* signalMapper = new QSignalMapper(window);
    signalMapper->setMapping(window->button(QDialogButtonBox::Yes), entitlement->contentConfiguration()->productId());
    ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Yes), SIGNAL(clicked()), signalMapper, SLOT(map()));
    ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(const QString&)), this, SIGNAL(restoreLocalBackupSave(const QString&)));
    pushAndShowWindow(window, entitlement->contentConfiguration()->productId() + " showSaveBackupRestoreWarning");
}


void LocalContentViewController::showDLCUpgradeWarning(Engine::Content::EntitlementRef entitlement, const QString& childOfferID, const QString& dlcName)
{
    UIToolkit::OriginWindow* window = UIToolkit::OriginWindow::promptNonModalScrollable(UIToolkit::OriginScrollableMsgBox::Info, 
        tr("ebisu_client_dlc_upgrade_title"), tr("ebisu_client_dlc_upgrade_warning_body").arg(dlcName),
        tr("ebisu_client_continue"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    window->setTitleBarText(tr("ebisu_client_upgrade_notice"));

    QLabel* lblText = window->scrollableMsgBox()->getTextLabel();
    lblText->setObjectName("Parent:" + entitlement->contentConfiguration()->productId() + " | DLC: " + childOfferID);
    lblText->setTextInteractionFlags((Qt::TextInteractionFlags)(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse));
    lblText->setOpenExternalLinks(false);
    ORIGIN_VERIFY_CONNECT(lblText, SIGNAL(linkActivated(const QString&)), this, SLOT(onLinkActivated(const QString&)));

    QSignalMapper* signalMapper = new QSignalMapper(window);
    signalMapper->setMapping(window->button(QDialogButtonBox::Yes), entitlement->contentConfiguration()->productId());
    ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Yes), SIGNAL(clicked()), signalMapper, SLOT(map()));
    ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(const QString&)), this, SLOT(onUpgrade(const QString&)));

    pushAndShowWindow(window, entitlement->contentConfiguration()->productId() + " DLC_UpgradeWarning");
}


void LocalContentViewController::showEntitleFreeGameError(Engine::Content::EntitlementRef entitlement)
{
    UIToolkit::OriginWindow* window = UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error,
        tr("ebisu_client_entitle_free_game_error_title"),
        tr("ebisu_client_entitle_free_game_error_body"),
        tr("ebisu_client_close"));
    pushAndShowWindow(window, entitlement->contentConfiguration()->productId() + " showEntitleFreeGameError");
}


void LocalContentViewController::showFloatingErrorWindow(const QString& title, const QString& text, const QString& key, const QString& source)
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", title, text, tr("ebisu_client_close")));
}

void LocalContentViewController::showSharedNetworkOverridePrompt(Engine::Content::EntitlementRef ent, const QString overridePath, const QString lastModifiedTime)
{
    QString dialogText = QString("The following build is available for %1 from the shared network override folder:<br><br>%2<br><br>Last modified on %3.<br><br>"
                                "Would you like to set a download override to this build?")
                                .arg("<b>" + ent->contentConfiguration()->displayName() + "</b>")
                                .arg("<b>" + overridePath + "</b>")
                                .arg("<b>" + lastModifiedTime + "</b>");
    
    UIToolkit::OriginWindow* window = 
        UIToolkit::OriginWindow::promptNonModalScrollable(UIToolkit::OriginScrollableMsgBox::Notice, "Set download override?",
        dialogText, "Yes", "No", QDialogButtonBox::Yes);
    window->setTitleBarText("Shared Network Override");
    window->setProperty("offerId", ent->contentConfiguration()->productId());

    Engine::Content::ContentController *contentController = Engine::Content::ContentController::currentUserContentController();

    ORIGIN_VERIFY_CONNECT(window, &UIToolkit::OriginWindow::accepted, contentController->sharedNetworkOverrideManager(),
        &Engine::Shared::SharedNetworkOverrideManager::onConfirmWindowAccepted);
    ORIGIN_VERIFY_CONNECT(window, &UIToolkit::OriginWindow::rejected, contentController->sharedNetworkOverrideManager(),
        &Engine::Shared::SharedNetworkOverrideManager::onConfirmWindowRejected);

    window = pushAndShowWindow(window, QString("Shared_Override_Prompt_%1").arg(ent->contentConfiguration()->productId()));
    if(window)
    {
        window->raise();
        window->activateWindow();
    }
}

void LocalContentViewController::showSharedNetworkOverrideFolderError(Engine::Content::EntitlementRef ent, const QString& path)
{
    QString dialogText = QString("You can not set a shared network override for %1 at:<br><br>%2<br><br>"
        "Would you like to retry the network location, or disable this shared network override?")
        .arg("<b>" + ent->contentConfiguration()->displayName() + "</b>")
        .arg("<b>" + path + "</b>");

    UIToolkit::OriginWindow* window = 
        UIToolkit::OriginWindow::promptNonModalScrollable(UIToolkit::OriginScrollableMsgBox::Error, "Inaccessible Network Location",
        dialogText, "Retry", "Disable", QDialogButtonBox::Yes);
    window->setTitleBarText("Shared Network Override");
    window->setProperty("offerId", ent->contentConfiguration()->productId());

    Engine::Content::ContentController *contentController = Engine::Content::ContentController::currentUserContentController();

    ORIGIN_VERIFY_CONNECT(window, &UIToolkit::OriginWindow::accepted, contentController->sharedNetworkOverrideManager(),
        &Engine::Shared::SharedNetworkOverrideManager::onErrorWindowRetry);
    ORIGIN_VERIFY_CONNECT(window, &UIToolkit::OriginWindow::rejected, contentController->sharedNetworkOverrideManager(),
        &Engine::Shared::SharedNetworkOverrideManager::onErrorWindowDisable);

    window = pushAndShowWindow(window, QString("Shared_Override_Folder_Error_%1").arg(ent->contentConfiguration()->productId()));
    if (window)
    {
        window->raise();
        window->activateWindow();
    }
}

void LocalContentViewController::showSharedNetworkOverrideInvalidTime(Engine::Content::EntitlementRef ent)
{
    QString dialogText = QString("The shared network override for %1 has an invalid scheduled time.  "
        "Please set a scheduled time that is in valid 24-hour clock format.")
        .arg("<b>" + ent->contentConfiguration()->displayName() + "</b>");
    
    UIToolkit::OriginWindow* window =
        UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, "Invalid Scheduled Time",
        dialogText, tr("ebisu_client_close"));
    window->setTitleBarText("Shared Network Override");

    pushAndShowWindow(window, QString("Shared_Override_Invalid_Time_%1").arg(ent->contentConfiguration()->productId()));
}

void LocalContentViewController::onUpgrade(const QString& productId)
{
    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productId);
    if(!entitlement.isNull())
    {
        Origin::Engine::Subscription::SubscriptionManager::instance()->upgrade(entitlement->contentConfiguration()->masterTitleId());    
    }
    else
    {
        ORIGIN_LOG_ERROR << "Upgrade Warning Error: Unable to upgrade " << productId << " (Can't find entitlement)";
    }
}

void LocalContentViewController::updateParentAndDownload(QJsonObject result)
{
    const bool accepted = Services::JsonValueValidator::validate(result["accepted"]).toBool();
    if(accepted == false)
        return;
    const QString productId = Services::JsonValueValidator::validate(result["productId"]).toString();
    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productId);
    if(!entitlement.isNull() && !entitlement->parent().isNull())
    {
        entitlement->localContent()->listenForParentStateChange();
        entitlement->parent()->localContent()->update();
    }
}


void LocalContentViewController::onUninstall(const QString& productId)
{
    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productId);

    // Safety bail out
    if (entitlement.isNull())
        return;

    // Don't uninstall games that are playing! TODO: Maybe move this to LocalContent::unInstall()
    if (!entitlement->localContent() || entitlement->localContent()->playing())
        return;

    // Another safety check
    if (!entitlement->contentConfiguration())
        return;

    // If PDLC, make sure the parent is not playing
    if (entitlement->contentConfiguration()->isPDLC())
    {
        Engine::Content::EntitlementRef parent = entitlement->parent();
        if (!parent.isNull())
        {
            if (parent->localContent() && parent->localContent()->playing())
                return;
        }
    }
    
    // TELEMETRY: Uninstall accepted by user (note: PC users don't get a choice, it just runs the uninstaller if available)
    GetTelemetryInterface()->Metric_GAME_UNINSTALL_START(productId.toUtf8().data());

    entitlement->localContent()->unInstall();
}


void LocalContentViewController::onUninstallCancelled(const QString& productId)
{
    // TELEMETRY: Uninstall declined by user (Mac only)
    GetTelemetryInterface()->Metric_GAME_UNINSTALL_CANCEL(productId.toUtf8().data());
}


void LocalContentViewController::onRemoveEntitlement(const QString& masterTitleId)
{
    Engine::Subscription::SubscriptionManager::instance()->downgrade(masterTitleId);
}

void LocalContentViewController::onOperationQueueProgressChanged()
{
    // Because this is a slot of the new head local content; we could convert the sender. However, I would prefer we get it manually for safety.
    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController()->head();
    if (entitlement && entitlement->localContent() && entitlement->localContent()->installFlow())
    {
        Downloader::InstallProgressState operationProgress = entitlement->localContent()->installFlow()->progressDetail();
        const bool isPaused = operationProgress.progressState(possibleMaskInstallState(entitlement).getValue()) == Downloader::InstallProgressState::kProgressState_Paused;
        double displayProgress = floor(operationProgress.mProgress * 100.0f);
        displayProgress = (displayProgress > 100.0) ? 100.0 : ((displayProgress < 0.0) ? 0.0 : displayProgress);
        Services::PlatformJumplist::update_progress(displayProgress, isPaused);
    }
    else
    {
        Services::PlatformJumplist::update_progress(0.0, false);
    }
}


Downloader::ContentInstallFlowState LocalContentViewController::possibleMaskInstallState(Engine::Content::EntitlementRef entitlement)
{
    Downloader::ContentInstallFlowState flowState = Downloader::ContentInstallFlowState::kInvalid;
    if (entitlement && entitlement->localContent() && entitlement->localContent()->installFlow())
    {
        Engine::Content::ContentOperationQueueController* queueController = Engine::LoginController::currentUser()->contentOperationQueueControllerInstance();
        flowState = entitlement->localContent()->installFlow()->getFlowState();
        if (queueController->indexInQueue(entitlement) == 0 && flowState == Downloader::ContentInstallFlowState::kEnqueued)
        {
            if (entitlement->localContent()->installFlow()->isEnqueuedAndPaused())
            {
                flowState = Downloader::ContentInstallFlowState::kPaused;
            }
            else
            {
                flowState = Downloader::ContentInstallFlowState::kInitializing;
            }
        }
    }
    return flowState;
}

}
}
