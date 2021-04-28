/////////////////////////////////////////////////////////////////////////////
// ITOViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ITOViewController.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originscrollablemsgbox.h"
#include "origincheckbutton.h"
#include "originpushbutton.h"
#include "originmsgbox.h"
#include "originspinner.h"

#include "RedeemBrowser.h"

#include "engine/ito/InstallFlowManager.h"
#include "services/log/LogService.h"
#include "MainFlow.h"

#include "engine/content/ContentController.h"

#include "ITEPrepare.h"
#include "InstallSpaceInfo.h"
#include "ui_CloudDialog.h"

namespace Origin
{
namespace Client
{

namespace
{
    const QString BOLD = "<b>%1</b>";
}

ITOViewController::ITOViewController(const QString& gameName, QObject* parent, const bool& inDebug)
: QObject(parent)
, mGameName(gameName)
, mWindow(NULL)
, mInDebug(inDebug)
{

}


ITOViewController::~ITOViewController()
{
    closeWindow();
}


void ITOViewController::showPrepare(const Origin::Downloader::InstallArgumentRequest& request)
{
    mInstallArg = request;
    closeWindow();
    using namespace UIToolkit;
    Engine::InstallFlowManager::GetInstance().SetVariable("InstallSize", request.installedSize);
    ITEPrepareWidget* content = new ITEPrepareWidget();
    content->setOptionalEulaList(request.optionalEulas);
    mWindow = OriginWindow::promptNonModal(OriginMsgBox::NoIcon, tr("ebisu_client_ite_install_gametitle").arg(mGameName.toUpper()), 
        "", tr("ebisu_client_next"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    mWindow->setContent(content);
    mWindow->setBannerText(QString("%1 - %2").arg(tr("ebisu_client_ite_insufficient_disk_space_label")).arg(tr("ebisu_client_ite_insufficient_disk_space_desc")));
    mWindow->setBannerIconType(OriginBanner::Error);
    if(mWindow->manager())
        mWindow->manager()->setupButtonFocus();
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(onPrepareDone(int)));

    //we need to call this specifically AFTER mWindow has been created, because although updateDiskSpaceText() gets called as a course of init() for ITEPrepareWidget()
    //it needs mWindow to be able to make the error banner be visible
    content->updateDiskSpaceText();
    mWindow->showWindow();
    ORIGIN_LOG_EVENT << "Show ITE dialog";
}


void ITOViewController::onPrepareDone(int result)
{
    if(result == QDialog::Accepted)
    {
        Engine::InstallFlowManager::GetInstance().SetStateCommand("Next");

        if(mWindow && mWindow->content())
        {
            ITEPrepareWidget* content = dynamic_cast<ITEPrepareWidget*>(mWindow->content());
            if(content && content->installSpaceInfo())
            {
                InstallSpaceInfo* installInfo = content->installSpaceInfo();
                QList<UIToolkit::OriginCheckButton*> optionalCheckButtons = installInfo->optionalCheckButtons();
                unsigned short acceptedBits = 0;
                unsigned short bitFlag = 1;
                for (int i = 0; i < optionalCheckButtons.size(); ++i)
                {
                    if(optionalCheckButtons.at(i)->isChecked())
                    {
                        acceptedBits |= bitFlag;
                    }
                    else
                    {
                        QPair<int, unsigned short> item;
                        item.first = i;
                        if (mInstallArg.optionalEulas.listOfEulas.at(i).CancelWarning.count())
                        {
                            item.second = bitFlag;
                            mWarningList.append(item);
                        }

                    }
                    bitFlag <<= 1;
                }
                mInstallArgReponse.optionalComponentsToInstall = acceptedBits;
                mInstallArgReponse.installDesktopShortCut = installInfo->desktopShortcut()->isChecked();
                mInstallArgReponse.installStartMenuShortCut = installInfo->startmenuShortcut()->isChecked();
                Services::writeSetting(Services::SETTING_CREATEDESKTOPSHORTCUT, mInstallArgReponse.installDesktopShortCut, Services::Session::SessionService::currentSession());
                Services::writeSetting(Services::SETTING_CREATESTARTMENUSHORTCUT, mInstallArgReponse.installStartMenuShortCut, Services::Session::SessionService::currentSession());

                QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
                Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);

                if (mWarningList.count() == 0)
                {
                    if (entitlement && entitlement->localContent()->installFlow())
                        entitlement->localContent()->installFlow()->setInstallArguments(mInstallArgReponse);
                    closeWindow();
                }
                else
                {
                    showInstallArgumentsWarnings();
                }
            }
        }
    }
    else
    {
        QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
        Engine::Content::EntitlementRef entRef = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);

        if(entRef)
        {
            showCancelConfirmation();
        }
    }
}


void ITOViewController::showInstallArgumentsWarnings()
{
    if (mWarningList.isEmpty())
    {
        return;
    }
    closeWindow();
    QPair<int, unsigned short> item = mWarningList.front();
    mWindow = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::NoIcon, tr("ebisu_client_warning_caps"),
        mInstallArg.optionalEulas.listOfEulas.at(item.first).CancelWarning, tr("ebisu_client_yes"), tr("ebisu_client_no"), QDialogButtonBox::No);
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    mWindow->setObjectName(QString::number(item.second));
    mWindow->showWindow();
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(onInstallArgumentsWarningDone(const int&)));
}


void ITOViewController::onInstallArgumentsWarningDone(const int& result)
{
    if (result == QDialog::Rejected)
    {
        // this is the persisting info of the placement in the bit flag.
        const unsigned short bitFlag = static_cast<unsigned short>(mWindow->objectName().toInt());
        mInstallArgReponse.optionalComponentsToInstall |= bitFlag;
    }

    closeWindow();
    mWarningList.pop_front();
    if (mWarningList.isEmpty())
    {
        QString contentIdToInstall = Engine::InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
        Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);
        if (entitlement && entitlement->localContent()->installFlow())
            entitlement->localContent()->installFlow()->setInstallArguments(mInstallArgReponse);
    }
    else
    {
        showInstallArgumentsWarnings();
    }
}


void ITOViewController::showInstallFailed()
{
    closeWindow();
    using namespace Origin::UIToolkit;
    mWindow = OriginWindow::alertNonModal(OriginMsgBox::Error, 
        tr("ebisu_client_installation_failed_uppercase"),
        tr("ebisu_client_installation_failed").arg(BOLD.arg(mGameName)), tr("ebisu_client_close"));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(onFinishedDone()));
    mWindow->showWindow();
}


void ITOViewController::showFailedNoGameName()
{
    closeWindow();
    using namespace Origin::UIToolkit;
    mWindow = OriginWindow::alertNonModal(OriginMsgBox::Error, 
        tr("ebisu_client_installation_failed_uppercase"),
        tr("ebisu_client_installation_failed_no_gamename"), tr("ebisu_client_close"));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(onFinishedDone()));
    mWindow->showWindow();
}


void ITOViewController::showFreeToPlayFailNoEntitlement()
{
    closeWindow();
    using namespace Origin::UIToolkit;
    mWindow = OriginWindow::alertNonModal(OriginMsgBox::Error, 
        tr("ebisu_client_installation_failed_uppercase"),
        tr("ebisu_client_installation_ftp_no_ent").arg(BOLD.arg(mGameName)), tr("ebisu_client_close"));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(onFinishedDone()));
    mWindow->showWindow();
}

void ITOViewController::showSystemReqFailed()
{
    closeWindow();
    using namespace Origin::UIToolkit;
    mWindow = OriginWindow::alertNonModal(OriginMsgBox::Error, 
        tr("ebisu_client_system_reqs_not_met_title"),
        tr("ebisu_client_system_reqs_not_met_message").arg(mGameName), tr("ebisu_client_close"));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(onFinishedDone()));
    mWindow->showWindow();
}

void ITOViewController::showDLCSystemReqFailed()
{
    closeWindow();
    using namespace Origin::UIToolkit;
    mWindow = OriginWindow::alertNonModal(OriginMsgBox::Error, 
        tr("ebisu_client_installation_failed_uppercase"),
        tr("ebisu_client_installation_failed").arg(mGameName), tr("ebisu_client_close"));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(onFinishedDone()));
    mWindow->showWindow();
}

void ITOViewController::showRedeem()
{
    closeWindow();
    RedeemBrowser* redeemBrowser = NULL;
    if (mWindow == NULL || mInDebug)
    {
        ORIGIN_LOG_EVENT << "ITE - Creating redeem browser.";
        redeemBrowser = new RedeemBrowser(NULL, RedeemBrowser::ITE, RedeemBrowser::OriginCodeITE, NULL, QString());
        using namespace Origin::UIToolkit;
        mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, redeemBrowser);
        mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
        ORIGIN_VERIFY_CONNECT(redeemBrowser, SIGNAL(closeWindow()), this, SIGNAL(redemptionDone()));
        ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), mWindow, SLOT(hide()));
        ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(showCancelConfirmation()));

        mWindow->setObjectName("RedeemCodeWindow");
        mWindow->setTitleBarText(tr("ebisu_client_ite_activate").arg(tr("application_name")).arg(mGameName));
        Engine::InstallFlowManager::GetInstance().SetVariable("WrongCode",false);	// need to reset this too
    }
    ORIGIN_LOG_EVENT << "ITE - Showing redeem browser.";
    mWindow->showWindow();

    // start at the beginning
    if(redeemBrowser)
    {
        bool wrongCode = Engine::InstallFlowManager::GetInstance().GetVariable("WrongCode").toBool();
        if(wrongCode)
        {
            redeemBrowser->loadRedeemURL();
            Engine::InstallFlowManager::GetInstance().SetVariable("WrongCode",false);
        }
    }
}


void ITOViewController::showWrongCode()
{
    closeWindow();
    using namespace Origin::UIToolkit;
    mWindow = OriginWindow::promptNonModal(OriginMsgBox::Notice,
        tr("ebisu_client_wrong_code_title_uppercase"), 
        tr("ebisu_client_wrong_code_desc").arg(BOLD.arg(mGameName)),
        tr("ebisu_client_ok_uppercase"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    Engine::InstallFlowManager::GetInstance().SetVariable("WrongCode", true);
    // go to the next state whether 'next' or the close button is clicked
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(onGoToNext(int)));
    mWindow->showWindow();
}


void ITOViewController::onGoToNext(int result)
{
    if(result == QDialog::Accepted)
    {
        Engine::InstallFlowManager::GetInstance().SetStateCommand("Next");
    }
    else
    {
        showCancelConfirmation();
    }
}


void ITOViewController::showDiscCheck()
{
    using namespace Origin::UIToolkit;
    OriginWindow* window = OriginWindow::alertNonModal(OriginMsgBox::NoIcon,
        tr("ebisu_client_insert_disc_title").arg("1"),
        tr("ebisu_client_insert_disc_desc_no_gamename").arg("1"), tr("ebisu_client_ok_uppercase"));
    window->showWindow();
    ORIGIN_VERIFY_CONNECT(window, SIGNAL(accepted()), this, SIGNAL(retryDisc()));
    ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), this, SIGNAL(cancel()));
}


void ITOViewController::showNotReadyToDownload()
{
    using namespace UIToolkit;
    QString msg = tr ("ebisu_client_not_released_cant_install").arg(BOLD.arg(mGameName));
    OriginWindow* unreleasedErrMsg = OriginWindow::alertNonModal(OriginMsgBox::Notice, tr("ebisu_client_game_not_yet_released_caps"), msg, tr("ebisu_client_close"));
    unreleasedErrMsg->showWindow();
}


void ITOViewController::showInsertDisk(const int& discNum, const int& totalDiscs)
{
    if(!mInDebug && (discNum == -1 || mWindow))
        return;
    QString title, text;
    // If there is only one disc - wording is different
    if(totalDiscs == 1)
    {
        title = tr("ebisu_client_insert_disc_title").arg("");
        text = tr("ebisu_client_insert_single_disc_desc").arg(BOLD.arg(mGameName));
    }
    else
    {
        title = tr("ebisu_client_insert_disc_title").arg(discNum);
        text = tr("ebisu_client_insert_disc_desc").arg(discNum).arg(BOLD.arg(mGameName));
    }
    mWindow = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::NoIcon, title, text, tr("ebisu_client_ok_uppercase"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(accepted()), this, SIGNAL(retryDisc()));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(accepted()), this, SLOT(closeWindow()));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(showCancelConfirmation()));
    mWindow->showWindow();
}


void ITOViewController::showWrongDisk(const int& expectedDisc)
{
    if(!mInDebug && (expectedDisc == -1 || mWindow))
        return;
    mWindow = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::Notice, tr("ebisu_client_wrong_disc_title"),
        tr("ebisu_client_wrong_disc_desc2").arg(expectedDisc).arg(BOLD.arg(mGameName)),
        tr("ebisu_client_ok_uppercase"), tr("ebisu_client_cancel"), QDialogButtonBox::Yes);
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(accepted()), this, SIGNAL(retryDisc()));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(accepted()), this, SLOT(closeWindow()));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(showCancelConfirmation()));
    mWindow->showWindow();
}


void ITOViewController::onFinishedDone(int playGame)
{
    closeWindow();
    ORIGIN_VERIFY_DISCONNECT(sender(), SIGNAL(finished(int)), this, SLOT(onFinishedDone()));
    if(playGame)
        Engine::InstallFlowManager::GetInstance().SetVariable("PlayGame", playGame);
    Engine::InstallFlowManager::GetInstance().SetStateCommand("Finish");
    emit resetFlow();
}


void ITOViewController::onInsertInitialDiscDone(int result)
{
    if(result == QDialog::Accepted)
        Engine::InstallFlowManager::GetInstance().SetStateCommand("Retry");
    else
    {
        emit cancel();
    }
}


void ITOViewController::showReadDiscError()
{
    closeWindow();
    using namespace Origin::UIToolkit;
    mWindow = OriginWindow::alertNonModalScrollable(OriginScrollableMsgBox::NoIcon,
        tr("ebisu_client_ite_UNABLE_TO_READ_DISC"),
        tr("ebisu_client_ite_disk_read_error_desc"), tr("ebisu_client_cancel"));
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SIGNAL(cancel()));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(finished(int)), this, SLOT(closeWindow()));

    Ui::CloudDialog ui;
    QWidget* commandlinkContent = new QWidget();
    ui.setupUi(commandlinkContent);
    ui.link1->setText(tr("ebisu_client_ite_try_again"));
    ui.link1->setDescription(tr("ebisu_client_disc_error_try_again_description"));
    ORIGIN_VERIFY_CONNECT(ui.link1, SIGNAL(clicked()), this, SIGNAL(retryInstallFlow()));
    ORIGIN_VERIFY_CONNECT(ui.link1, SIGNAL(clicked()), this, SLOT(closeWindow()));
    ui.link2->setText(tr("ebisu_client_ite_disk_read_error_download_game"));
    ui.link2->setDescription(tr("ebisu_client_disc_error_download_game_description").arg(tr("application_name")));
    ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), this, SIGNAL(downloadGame()));
    ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), this, SLOT(closeWindow()));
    mWindow->setContent(commandlinkContent);
    mWindow->showWindow();
}


void ITOViewController::showDownloadPreparing()
{
    closeWindow();
    using namespace Origin::UIToolkit;
    // in case you can't cancel, wait for the downloader to be in a kPaused state 
    OriginSpinner* spinner = new OriginSpinner();
    mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Close, spinner, OriginWindow::MsgBox);
    mWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_pre_download") + "...", "");
    spinner->startSpinner();
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
    mWindow->showWindow();
}


void ITOViewController::showCancelConfirmation()
{
    if(mWindow)
        mWindow->hide();
    using namespace Origin::UIToolkit;
    OriginWindow* window = OriginWindow::promptNonModal(OriginMsgBox::Notice, tr("ebisu_client_ite_cancel_installation_CANCEL_INSTALLATION"), 
        tr("ebisu_client_ite_cancel_installation_of_game").arg(BOLD.arg(mGameName)), 
        tr("ebisu_client_yes"), tr("ebisu_client_no"), QDialogButtonBox::No);
    ORIGIN_VERIFY_CONNECT(window, SIGNAL(accepted()), this, SIGNAL(cancel()));
    ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), this, SLOT(showCurrentWindow()));
    window->showWindow();
}


void ITOViewController::showCurrentWindow()
{
    if(mWindow)
    {
        mWindow->showWindow();
    }
}


void ITOViewController::closeWindow()
{
    if(mWindow && !mInDebug)
    {
        ORIGIN_VERIFY_DISCONNECT(mWindow, 0, this, 0);
        mWindow->deleteLater();
        mWindow = NULL;
    }
}

}
}
