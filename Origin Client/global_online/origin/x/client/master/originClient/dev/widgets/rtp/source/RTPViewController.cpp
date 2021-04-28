/////////////////////////////////////////////////////////////////////////////
// RTPViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "RTPViewController.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/Entitlement.h"
#include "ui_CloudDialog.h"
#include "ClientFlow.h"
#include "EbisuSystemTray.h"
#include "DialogController.h"
#include <QJsonObject>
#include "PlayView.h"

namespace Origin
{
namespace Client
{
RTPViewController::RTPViewController(QObject* parent)
: QObject(parent)
{
}


RTPViewController::~RTPViewController()
{
}


void RTPViewController::showOfflineErrorDialog()
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_not_connected_uppercase"), 
        tr("ebisu_client_reconnect_message_text").arg(tr("application_name")), tr("ebisu_client_ok_uppercase")));
}


void RTPViewController::showGameShouldBeInstalledErrorDialog(const QString& gameTitle)
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_r2p_error_title"), 
        tr("ebisu_client_r2p_game_should_be_installed").arg(gameTitle), tr("ebisu_client_close")));
}


void RTPViewController::showGameUnreleasedErroDialog(const QString& gameTitle, const QString& releasedate)
{
    QString text;
    // releasedate is empty string if the ORC state was RC_Unreleased (release date in the FAR future)
    if(releasedate.isEmpty())
        text = tr("ebisu_client_not_released_cant_play").arg("<strong>"+gameTitle+"</strong>");
    else
        text = tr("ebisu_clent_game_unreleased_text").arg("<strong>"+gameTitle+"</strong>").arg(releasedate);

    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_game_unreleased_header"), 
        text, tr("ebisu_client_close")));
}


void RTPViewController::showWrongCodeDialog(const QString& gameTitle)
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_wrong_code_title_uppercase"), 
        tr("ebisu_client_rtp_wrong_code_desc").arg("<strong>"+gameTitle+"</strong>"), tr("ebisu_client_close")));
}


void RTPViewController::showNotEntitledDialog(const QString& eaId, const QString& gameTitle, const rtpHiddenUI& showOfAgeContent, const QString& storeLink)
{
    QJsonObject content;
    QJsonObject persistingObj;
    persistingObj["storeLink"] = storeLink;

    QJsonArray commandbuttons;
    QJsonObject cb = PlayView::commandLinkObj("redeem", "otkicon-settings", tr("ebisu_client_rtp_redeem_button_title"), tr("ebisu_client_rtp_redeem_button_text"));
    commandbuttons.append(cb);
    cb = PlayView::commandLinkObj("not-user", "otkicon-refresh", tr("ebisu_client_rtp_not_user_button_title").arg(eaId), tr("ebisu_client_rtp_not_user_button_text").arg(tr("application_name")));
    commandbuttons.append(cb);
    if(storeLink.count() && (showOfAgeContent == ShowOfAgeContent))
    {
        cb = PlayView::commandLinkObj("store", "otkicon-store", tr("ebisu_client_rtp_purchase_button_title"), DialogController::MessageTemplate::toAttributeFriendlyText(tr("ebisu_client_rtp_purchase_button_text")));
        commandbuttons.append(cb);
    }
    content["commandbuttons"] = commandbuttons;

    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("origin-dialog-content-commandbuttons", 
        tr("ebisu_client_rtp_dialog_title"), 
        tr("ebisu_client_rtp_dialog_text").arg("<strong>" + gameTitle + "</strong>").arg(tr("application_name")), 
        tr("ebisu_client_cancel"), content, this, "onCommandLinksDone", persistingObj));
}


void RTPViewController::onCommandLinksDone(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if (accepted)
    {
        const QString optionSelected = Services::JsonValueValidator::validate(obj["selectedOption"]).toString();
        if (QString::compare("redeem", optionSelected, Qt::CaseInsensitive) == 0)
        {
            emit redeemGame();
        }
        else if (QString::compare("not-user", optionSelected, Qt::CaseInsensitive) == 0)
        {
            emit notThisUser();
        }
        else if (QString::compare("store", optionSelected, Qt::CaseInsensitive) == 0)
        {
            const QString storeLink = obj["storeLink"].toString();
            emit purchaseGame(storeLink);
        }
        else if (QString::compare("launch-odt", optionSelected, Qt::CaseInsensitive) == 0)
        {
            emit showDeveloperTool();
        }
        else
        {
            emit cancel();
        }
    }
    else
    {
        emit cancel();
    }
}


void RTPViewController::showIncorrectEnvironment(const QString& gameTitle, const QString& currentEnv, const rtpHiddenUI& developerModeEnabled)
{
    QJsonObject content;
    QJsonArray commandbuttons;
    QJsonObject cb;
    if(developerModeEnabled == ShowDeveloperMode)
    {
        cb = PlayView::commandLinkObj("launch-odt", "otkicon-settings", tr("ebisu_client_launch_odt"), tr("ebisu_client_launch_origin_in_different_environment").arg(tr("application_name")));
        commandbuttons.append(cb);
    }
    cb = PlayView::commandLinkObj("not-user", "otkicon-refresh", tr("ebisu_client_login_to_different_account"), tr("ebisu_client_login_with_another_account").arg(tr("application_name")).arg(gameTitle).arg(currentEnv));
    commandbuttons.append(cb);
    cb = PlayView::commandLinkObj("redeem", "otkicon-key", tr("ebisu_client_activate_game_in_environment").arg(currentEnv), tr("ebisu_client_use_product_code_to_activate_game").arg(tr("application_name")).arg(currentEnv));
    commandbuttons.append(cb);
    content["commandbuttons"] = commandbuttons;

    const QString text = QString("%0 <p>%1</p>").arg(tr("ebisu_client_game_game_may_be_active_in_other_environment").arg(gameTitle)).arg(tr("ebisu_client_current_environment").arg(currentEnv));
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("origin-dialog-content-commandbuttons", 
        tr("ebisu_client_game_not_activated_in_environment_title"), text,
        tr("ebisu_client_cancel"), content, this, "onCommandLinksDone"));
}


void RTPViewController::showOnlineRequiredForEarlyTrialDialog(const QString& gameTitle)
{
#if 0 //TODO FIXUP_ORIGINX
    UIToolkit::OriginWindow* window = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::Error, tr("ebisu_subs_trial_offline_notification_title"),
        tr("ebisu_subs_trial_offline_notification").arg("<b>" + gameTitle + "</b>"), tr("ebisu_client_single_login_go_online"), tr("ebisu_client_cancel"));
    window->setTitleBarText(tr("ebisu_subs_trial_window_launch_window_title"));
    ORIGIN_VERIFY_CONNECT_EX(window, SIGNAL(accepted()), this, SLOT(onGoOnline()), Qt::QueuedConnection);
    window->showWindow();
#endif
}


void RTPViewController::showOnlineRequiredForTrialDialog()
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_must_be_online_to_launch_vault_game_title"), 
        tr("ebisu_client_free_trial_online_required_message"), tr("ebisu_client_ok_uppercase")));
}


void RTPViewController::onGoOnline()
{
    ClientFlow::instance()->requestOnlineMode();
}


void RTPViewController::showFreeTrialExpiredDialog(const Engine::Content::EntitlementRef entitlement)
{
    const QString gameTitle = entitlement->contentConfiguration()->displayName();
    QJsonObject persistingInfo;
    persistingInfo["offerId"] = entitlement->contentConfiguration()->productId();

    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_free_trial_used_on_account_titlebar").arg(gameTitle.toUpper()), 
        tr("ebisu_client_free_trial_used_on_account_message").arg(gameTitle), 
        tr("ebisu_client_view_in_store"), tr("ebisu_client_cancel"), "yes", 
        QJsonObject(), this, "onFreeTrialExpiredDone", persistingInfo));
}


void RTPViewController::onFreeTrialExpiredDone(QJsonObject obj)
{
    if(obj["accepted"].toBool())
    {
        const QString offerId = obj["offerId"].toString();
        ClientFlow* clientFlow = ClientFlow::instance();
        EbisuSystemTray* sysTray = EbisuSystemTray::instance();

        if(sysTray)
            sysTray->showPrimaryWindow();

        if(clientFlow)
            clientFlow->showProductIDInStore(offerId);
    }
    else
    {
        emit cancel();
    }
}


void RTPViewController::showEarlyTrialExpiredDialog(const Engine::Content::EntitlementRef entitlement)
{
    using namespace Origin::UIToolkit;

    const QString gameTitle = entitlement->contentConfiguration()->displayName();
    const QString title = tr("ebisu_subs_trial_expired_header").arg(gameTitle.toUpper());
    const QString text = tr("ebisu_subs_trial_expired_body").arg(gameTitle);

#if 0 //TODO FIXUP_ORIGINX
    OriginWindow* window = OriginWindow::promptNonModalScrollable(OriginScrollableMsgBox::Error, title,
        text, tr("ebisu_client_buy_now"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    window->setTitleBarText(tr("ebisu_subs_trial_window_launch_window_title"));
    window->showWindow();

    QSignalMapper* signalMapper = new QSignalMapper(window);
    signalMapper->setMapping(window->button(QDialogButtonBox::Yes), entitlement->contentConfiguration()->productId());
    ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Yes), SIGNAL(clicked()), signalMapper, SLOT(map()));
    ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(const QString&)), this, SLOT(showProductIdInStore(const QString&)));
#endif
}


void RTPViewController::showEarlyTrialDisabledByAdminDialog(const Engine::Content::EntitlementRef entitlement)
{
#if 0 //TODO FIXUP_ORIGINX
    using namespace Origin::UIToolkit;
    OriginWindow* window = OriginWindow::promptNonModalScrollable(OriginScrollableMsgBox::Error, tr("ebisu_subs_trial_unavailable_title"),
        tr("ebisu_subs_trial_unavailable_body"), tr("ebisu_client_view_in_store"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    window->setTitleBarText(tr("ebisu_subs_trial_window_launch_window_title"));
    window->showWindow();

    QSignalMapper* signalMapper = new QSignalMapper(window);
    signalMapper->setMapping(window->button(QDialogButtonBox::Yes), entitlement->contentConfiguration()->productId());
    ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Yes), SIGNAL(clicked()), signalMapper, SLOT(map()));
    ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(const QString&)), this, SLOT(showProductIdInStore(const QString&)));
#endif
}


void RTPViewController::showBusyDialog()
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_hang_on_title"), 
        tr("origin_client_busy_wait_before_continuing"), tr("ebisu_client_ok_uppercase")));
}


void RTPViewController::showSystemRequirementsNotMetDialog(const Engine::Content::EntitlementRef entitlement)
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_system_reqs_not_met_title"), 
        tr("ebisu_client_system_reqs_not_met_message").arg(entitlement->contentConfiguration()->displayName()), tr("ebisu_client_close")));
}


void RTPViewController::showSubscriptionOfflinePlayExpired()
{
#if 0 //TODO FIXUP_ORIGINX
    UIToolkit::OriginWindow* window = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::Error, 
        tr("Subs_Launch_OfflineExpireMessage_Title_Header"), 
        tr("Subs_Launch_OfflineExpireMessage_Title_Body"), 
        tr("ebisu_client_can_not_go_online_title"), tr("ebisu_client_cancel"));
    ORIGIN_VERIFY_CONNECT_EX(window, SIGNAL(accepted()), this, SIGNAL(goOnline()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(window, SIGNAL(rejected()), this, SIGNAL(cancel()), Qt::QueuedConnection);
    window->showWindow();
#endif
}


void RTPViewController::showSubscriptionMembershipExpired(const Engine::Content::EntitlementRef entitlement, const bool& isUnderAge)
{
#if 0 //TODO FIXUP_ORIGINX
    using namespace Origin::UIToolkit;
    OriginWindow* window = NULL;
    if(isUnderAge)
    {
        window = OriginWindow::alertNonModal(OriginMsgBox::Error, 
            tr("Subs_Library_Banner_Expired_Header"), 
            tr("Subs_Launch_SubExpiredMessage_Title_Body").arg("<b>"+entitlement->contentConfiguration()->displayName()+"</b>"),
            tr("ebisu_client_cancel"));
    }
    else
    {
        window = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
            NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Cancel);
        window->scrollableMsgBox()->setup(OriginScrollableMsgBox::Error, 
            tr("Subs_Library_Banner_Expired_Header"), 
            tr("Subs_Launch_SubExpiredMessage_Title_Body").arg("<b>"+entitlement->contentConfiguration()->displayName()+"</b>"));
        ORIGIN_VERIFY_DISCONNECT(window, SIGNAL(closeWindow()), window, SIGNAL(rejected()));
        ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), window, SLOT(reject()));
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(closeWindow()), window, SLOT(reject()));

        QWidget* content = new QWidget();
        Ui::CloudDialog ui;
        ui.setupUi(content);
#ifdef ORIGIN_MAC
        content->layout()->setSpacing(15);
#endif

        if(isUnderAge)
        {
            ui.link2->setVisible(false);
        }
        else
        {
            QSignalMapper* signalMapper = new QSignalMapper(this);
            signalMapper->setMapping(ui.link2, entitlement->contentConfiguration()->masterTitleId());
            ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), signalMapper, SLOT(map()));
            ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(const QString&)), this, SIGNAL(purchaseGame(const QString&)));
            ORIGIN_VERIFY_CONNECT_EX(ui.link2, SIGNAL(clicked()), window, SLOT(accept()), Qt::QueuedConnection);
        }

        ui.link1->setText(tr("Subs_Launch_SubExpiredMessage_Renew_Header"));
        ui.link1->setDescription(tr("Subs_Launch_SubExpiredMessage_Renew_Body"));
        ui.link2->setText(tr("Subs_Launch_SubExpiredMessage_Purchase_Header"));
        ui.link2->setDescription(tr("Subs_Launch_SubExpiredMessage_Purchase_Body"));

        window->setContent(content);
        ORIGIN_VERIFY_CONNECT_EX(window, SIGNAL(rejected()), this, SIGNAL(cancel()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(ui.link1, SIGNAL(clicked()), this, SIGNAL(renewSubscriptionMembership()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(ui.link1, SIGNAL(clicked()), window, SLOT(accept()), Qt::QueuedConnection);
    }
    window->setTitleBarText(tr("Subs_Launch_SubExpiredMessage_Window_Header"));
    window->showWindow();
#endif
}


void RTPViewController::showSubscriptionNotAvailableOnPlatform(const QString& eaId, const QString& gameTitle, const bool& isUnderAge, const QString& storeLink)
{
#if 0 //TODO FIXUP_ORIGINX
    using namespace Origin::UIToolkit;
    OriginWindow* window = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
        NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Cancel);
    window->scrollableMsgBox()->setup(OriginScrollableMsgBox::NoIcon, 
        tr("ebisu_subs_windows_only_title"), 
        tr("ebisu_client_rtp_dialog_text").arg("<b>"+gameTitle+"</b>").arg(tr("application_name")));
    ORIGIN_VERIFY_DISCONNECT(window, SIGNAL(closeWindow()), window, SIGNAL(rejected()));
    ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), window, SLOT(reject()));
    ORIGIN_VERIFY_CONNECT(window, SIGNAL(closeWindow()), window, SLOT(reject()));

    QWidget* content = new QWidget();
    Ui::RtpNotEntitledDialog ui;
    ui.setupUi(content);
#ifdef ORIGIN_MAC
    content->layout()->setSpacing(15);
#endif

    if(storeLink.isEmpty() || isUnderAge)
    {
        ui.link3->setVisible(false);
    }
    else
    {
        QSignalMapper* signalMapper = new QSignalMapper(this);
        signalMapper->setMapping(ui.link3, storeLink);
        ORIGIN_VERIFY_CONNECT(ui.link3, SIGNAL(clicked()), signalMapper, SLOT(map()));
        ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(const QString&)), this, SIGNAL(purchaseGame(const QString&)));
        ORIGIN_VERIFY_CONNECT_EX(ui.link3, SIGNAL(clicked()), window, SLOT(accept()), Qt::QueuedConnection);
    }

    ui.link1->setText(tr("ebisu_client_rtp_redeem_button_title"));
    ui.link1->setDescription(tr("ebisu_client_rtp_redeem_button_text"));
    ui.link2->setText(tr("ebisu_client_rtp_not_user_button_title").arg(eaId));
    ui.link2->setDescription(tr("ebisu_client_rtp_not_user_button_text").arg(tr("application_name")));
    ui.link3->setText(tr("ebisu_client_rtp_purchase_button_title"));
    ui.link3->setDescription(tr("ebisu_client_rtp_purchase_button_text"));

    window->setContent(content);
    ORIGIN_VERIFY_CONNECT_EX(window, SIGNAL(rejected()), this, SIGNAL(cancel()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(ui.link1, SIGNAL(clicked()), this, SIGNAL(redeemGame()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(ui.link1, SIGNAL(clicked()), window, SLOT(accept()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(ui.link2, SIGNAL(clicked()), this, SIGNAL(notThisUser()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(ui.link2, SIGNAL(clicked()), window, SLOT(accept()), Qt::QueuedConnection);

    window->showWindow();
#endif
}

void RTPViewController::showEntitlementRetiredFromSubscription(const Engine::Content::EntitlementRef entitlement, const bool& isUnderAge)
{
#if 0 //TODO FIXUP_ORIGINX
    UIToolkit::OriginWindow* window = NULL;
    if(isUnderAge)
    {
        window = UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, 
            tr("Subs_Launch_RetiredMessage_Title_Header"),
            tr("Subs_Launch_RetiredMessage_Title_Body").arg("<b>"+entitlement->contentConfiguration()->displayName()+"</b>"),
            tr("ebisu_client_cancel"));
    }
    else
    {
        window = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::Error, 
            tr("Subs_Launch_RetiredMessage_Title_Header"),
            tr("Subs_Launch_RetiredMessage_Title_Body").arg("<b>"+entitlement->contentConfiguration()->displayName()+"</b>"),
            tr("ebisu_client_buy_in_store_intercaps"),
            tr("ebisu_client_cancel"));

        QSignalMapper* signalMapper = new QSignalMapper(this);
        signalMapper->setMapping(window->button(QDialogButtonBox::Yes), entitlement->contentConfiguration()->masterTitleId());
        ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Yes), SIGNAL(clicked()), signalMapper, SLOT(map()));
        ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(const QString&)), this, SIGNAL(purchaseGame(const QString&)));
        ORIGIN_VERIFY_CONNECT_EX(window->button(QDialogButtonBox::Yes), SIGNAL(clicked()), window, SLOT(accept()), Qt::QueuedConnection);
    }
    ORIGIN_VERIFY_CONNECT_EX(window, SIGNAL(rejected()), this, SIGNAL(cancel()), Qt::QueuedConnection);
    window->setTitleBarText(tr("Subs_Launch_RetiredMessage_Window_Header"));
    window->showWindow();
#endif
}

}
}
