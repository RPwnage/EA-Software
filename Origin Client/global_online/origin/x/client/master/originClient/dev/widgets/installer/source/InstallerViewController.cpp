/////////////////////////////////////////////////////////////////////////////
// InstallerViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "InstallerViewController.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/GamesController.h"
#include "engine/content/LocalContent.h"
#include "services/log/LogService.h"
#include "services/downloader/StringHelpers.h"
#include "services/escalation/IEscalationService.h"
#include "services/platform/PlatformService.h"
#include "services/settings/SettingsManager.h"
#include "InstallerView.h"
#include "OriginApplication.h"
#include "originwindow.h"
#include "originpushbutton.h"
#include "TelemetryAPIDLL.h"
#include "ClientFlow.h"
#include "engine/ito/InstallFlowManager.h"
#include "engine/igo/IGOController.h"
#include "ITOFlow.h"
#include "DialogController.h"
#include "LocalizedContentString.h"
#include <QLabel>
#include <QDesktopServices>


namespace Origin
{
namespace Client
{

static const QString KEY_SIZE1 = "size1";
static const QString KEY_SIZE2 = "size2";
static const QString KEY_SIZE1TEXT = "size1text";
static const QString KEY_SIZE2TEXT = "size2text";
static const QString KEY_EXTRACHECKBUTTONS = "extracheckbuttons";
static const QString KEY_INSUFFICIENTSPACE = "insufficientspace";
static const QString KEY_INSUFFICIENTSPACETEXT = "insufficientspacetext";
InstallerViewController::InstallerViewController(Engine::Content::EntitlementRef entitlement, const bool& inDebug, QObject* parent)
: QObject(parent)
, mEntitlement(entitlement)
, mInDebug(inDebug)
{
    ORIGIN_VERIFY_CONNECT(DialogController::instance(), SIGNAL(linkClicked(const QJsonObject&)), this, SLOT(onDialogLinkClicked(const QJsonObject&)));
}


InstallerViewController::~InstallerViewController()
{
    DialogController::instance()->closeGroup(rejectGroup());
    //always clear this for now. It
    if(!mInDebug)
    {
        // If there are some error dialogs up - we don't want to delete them 
        // because if the flow ends and an error dialog is up the dialog will disappear and the user
        // won't know why. The dialog will delete itself on close.
        mErrorWindows.clear();
    }
}


void InstallerViewController::emitStopFlow()
{
    emit stopFlow();
}


void InstallerViewController::showInstallArguments(const Origin::Downloader::InstallArgumentRequest& request)
{
    mInstallArg = request;
    InstallerView::DownloadInfoContent content(request);

    QJsonObject contentObj;
    contentObj[KEY_INSUFFICIENTSPACE] = false;
    contentObj[KEY_SIZE1TEXT] = tr("ebisu_client_download_size_colon");
    contentObj[KEY_SIZE1] = LocalizedContentString().formatBytes(request.downloadSize);
    contentObj[KEY_SIZE2TEXT] = tr("ebisu_client_size_after_install_colon");
    contentObj[KEY_SIZE2] = LocalizedContentString().formatBytes(request.installedSize);
    contentObj[KEY_EXTRACHECKBUTTONS] = content.checkBoxContent;

    // If we have a download override from catalog - tell the user additional downloading might happen
    if(mEntitlement->contentConfiguration()->downloadSize())
        content.text += QString("<br><br>%1").arg(tr("ebisu_client_game_might_download_additional_content"));

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-downloadinfo", content.title, content.text, tr("ebisu_client_next"), tr("ebisu_client_cancel"), "yes", contentObj, this, "onInstallArgumentsDone");
    mt.overrideId = "downloadinfo_"+request.productId;
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void InstallerViewController::showInsufficientFreeSpaceWindow(const Origin::Downloader::InstallArgumentRequest& request)
{
    mInstallArg = request;
    InstallerView::DownloadInfoContent content(request);

    QJsonObject contentObj;
    contentObj[KEY_INSUFFICIENTSPACE] = true;
    contentObj[KEY_INSUFFICIENTSPACETEXT] = tr("ebisu_client_ite_insufficient_disk_space_desc");
    contentObj[KEY_SIZE1TEXT] = tr("ebisu_client_size_after_install_colon");
    contentObj[KEY_SIZE1] = LocalizedContentString().formatBytes(request.installedSize);
    contentObj[KEY_SIZE2TEXT] = tr("ebisu_client_disk_space_available");
    contentObj[KEY_SIZE2] = LocalizedContentString().formatBytes(Services::PlatformService::GetFreeDiskSpace(request.installPath));
    contentObj[KEY_EXTRACHECKBUTTONS] = content.checkBoxContent;

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-downloadinfo", content.title, content.text, 
        tr("ebisu_client_next"), tr("ebisu_client_cancel"), "yes", contentObj, this, "onInstallArgumentsDone");
    mt.acceptEnabled = false;
    mt.overrideId = "downloadinfo_"+request.productId;
    mt.rejectGroup = rejectGroup();

    // whether download dialog needs warning message
    if(!request.isDip)
    {
        mt.text += "<br />";
        mt.text += tr("ebisu_client_install_size_warning");
    }

    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
    QTimer::singleShot(8000, this, SLOT(emitRetryDiskSpace()));
}


void InstallerViewController::onInstallArgumentsDone(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        const QJsonArray optionalCheckButtons = Services::JsonValueValidator::validate(obj[KEY_EXTRACHECKBUTTONS]).toArray();
        unsigned short acceptedBits = 0;
        unsigned short bitFlag = 1;
        mInstallArgReponse.installDesktopShortCut = false;
        mInstallArgReponse.installStartMenuShortCut = false;
        QList<QPair<int, unsigned short>> warningList;
        for(int i = 0; i < optionalCheckButtons.size(); ++i)
        {
            const QJsonObject chkValue = optionalCheckButtons.at(i).toObject();
            const bool checked = Services::JsonValueValidator::validate(chkValue["checked"]).toBool();
            const QString id = Services::JsonValueValidator::validate(chkValue["id"]).toString();
            if(id == InstallerView::DownloadInfoContent::KEY_DESKTOPSHORTCUT)
            {
                mInstallArgReponse.installDesktopShortCut = checked;
            }
            else if(id == InstallerView::DownloadInfoContent::KEY_STARTMENUSHORTCUT)
            {
                mInstallArgReponse.installStartMenuShortCut = checked;
            }
            else
            {
                if(checked)
                {
                    acceptedBits |= bitFlag;
                }
                else
                {
                    QPair<int, unsigned short> item;
                    item.first = id.split("chkb")[0].toInt();
                    if(mInstallArg.optionalEulas.listOfEulas.at(item.first).CancelWarning.count())
                    {
                        item.second = bitFlag;
                        warningList.append(item);
                    }
                    
                }
                bitFlag <<= 1;
            }
        }
        mInstallArgReponse.optionalComponentsToInstall = acceptedBits;
        Services::writeSetting(Services::SETTING_CREATEDESKTOPSHORTCUT, mInstallArgReponse.installDesktopShortCut, Services::Session::SessionService::currentSession());
        Services::writeSetting(Services::SETTING_CREATESTARTMENUSHORTCUT, mInstallArgReponse.installStartMenuShortCut, Services::Session::SessionService::currentSession());
        if(warningList.count() == 0)
        {
            emit installArgumentsCreated(mEntitlement->contentConfiguration()->productId(), mInstallArgReponse);
        }
        else
        {
            showInstallArgumentsWarnings(warningList);
        }
    }
    else
    {
        emit stopFlow();
    }
}


void InstallerViewController::showInstallArgumentsWarnings(const QList<QPair<int, unsigned short>>& warningList)
{
    for(int i = 0, numWarnings = warningList.count(); i < numWarnings; i++)
    {
        QPair<int, unsigned short> item = warningList[i];
        QJsonObject persisting;
        persisting["bitFlag"] = item.second;
        persisting["index"] = i;
        persisting["totalwarnings"] = numWarnings;
        DialogController::MessageTemplate mt = DialogController::MessageTemplate("", tr("ebisu_client_warning_caps"), mInstallArg.optionalEulas.listOfEulas.at(item.first).CancelWarning, 
            tr("ebisu_client_yes"), tr("ebisu_client_no"), "no", QJsonObject(), this, "onInstallArgumentsWarningDone", persisting);
        mt.rejectGroup = rejectGroup();
        DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
    }
}


void InstallerViewController::onInstallArgumentsWarningDone(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    const unsigned short bitFlag = static_cast<unsigned short>(Services::JsonValueValidator::validate(obj["bitFlag"]).toInt());
    const int index = Services::JsonValueValidator::validate(obj["index"]).toInt();
    const int numWarnings = Services::JsonValueValidator::validate(obj["totalwarnings"]).toInt();
    if(accepted == false)
    {
        mInstallArgReponse.optionalComponentsToInstall |= bitFlag;
    }
    if(index == (numWarnings - 1))
    {
        emit installArgumentsCreated(mEntitlement->contentConfiguration()->productId(), mInstallArgReponse);
    }
}


void InstallerViewController::emitRetryDiskSpace()
{
    emit retryDiskSpace(mInstallArg);
}


void InstallerViewController::showFolderAccessErrorWindow(const Origin::Downloader::InstallArgumentRequest& request)
{
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("", tr("ebisu_client_download_didnt_work_caps"), 
        tr("ebisu_client_download_failed_due_disc_corruption").arg(request.installPath), tr("ebisu_client_ok_uppercase"), QJsonObject(), this, "emitStopFlow");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void InstallerViewController::showLanguageSelection(const Origin::Downloader::InstallLanguageRequest& input)
{
    // If there is only one choice, show it any way to allow the user to abort, but change the wording 
    QString title, text;
    if(input.availableLanguages.size() == 1)
    {
        title = tr("ebisu_client_language_selection_one_title");
        text = tr("ebisu_client_language_selection_one_text");
    }
    else
    {
        title = tr("ebisu_client_language_selection_many_title");
        text = tr("ebisu_client_language_selection_many_text");
    }

    const QJsonObject content = InstallerView::formatLanguageSelection(input);
    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-downloadlanguageselection", 
        title, text, tr("ebisu_client_accept"), tr("ebisu_client_cancel"), "yes", content, this, "onSelectInstallLanguageDone");
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void InstallerViewController::onSelectInstallLanguageDone(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        Downloader::EulaLanguageResponse response;
        response.selectedLanguage = Services::JsonValueValidator::validate(obj["localeId"]).toString();
        emit languageSelected(mEntitlement->contentConfiguration()->productId(), response);
    }
    else
    {
        //this dialog is shared between ITO and regular downloads, we to make sure we cancel the ITO flow when we 
        //cancel out of this dialog. A more generic way of canceling ITO should be tied to the installflow cancelling signal, but that is a much bigger change.
        //so we will do it this way until we do a clean up of the ITO code (in the backlog)
        if(mEntitlement && mEntitlement->localContent()->installFlow() && mEntitlement->localContent()->installFlow()->isITO())
        {
            Engine::InstallFlowManager::GetInstance().SetStateCommand("Cancel");
            Engine::InstallFlowManager::GetInstance().SetVariable("CompleteDialogType", ITOFlow::Canceled);
        }
        emit stopFlow();
    }
}


void InstallerViewController::showEulaSummaryDialog(const Origin::Downloader::EulaStateRequest& request)
{
    mEulaRequest = request;
    // Change the description text "By <downloading/installing/preloading> <game title> you agree to the following:"
    QString title, text, titlebarText, btnText;		
    title = mEulaRequest.eulas.listOfEulas.size() > 1 ? tr("ebisu_client_eula_title_plural") : tr("ebisu_client_eula_title");

    if (mEulaRequest.isPreload) // Check for preload
    {
        text = tr("ebisu_client_eula_summary_desc_preload").arg(mEulaRequest.eulas.gameTitle);
        titlebarText = tr("ebisu_client_eula_summary_title_preload");
        btnText = tr("ebisu_client_preload_now");
    }
    else if (mEulaRequest.isLocalSource) // Check for ITO
    {
        text = tr("ebisu_client_eula_summary_desc_ITO").arg(mEulaRequest.eulas.gameTitle);
        titlebarText = tr("ebisu_client_eula_summary_title_ITO");
        btnText = tr("ebisu_client_install_now");
    }
    else
    {
        text = tr("ebisu_client_eula_summary_desc").arg(mEulaRequest.eulas.gameTitle);
        titlebarText = tr("ebisu_client_eula_summary_title");

        if (mEulaRequest.isUpdate)
            btnText = tr("ebisu_client_game_update_install_update");
        else
            btnText = tr("ebisu_client_next");
    }

    QJsonArray eulas;
    for (int i = 0, numEulas = mEulaRequest.eulas.listOfEulas.count(); i < numEulas; i++)
    {
        eulas.append(InstallerView::eulaObject(mEulaRequest.eulas.listOfEulas[i], i, numEulas));
    }

    QJsonObject content;
    content["agreechecktext"] = mEulaRequest.eulas.listOfEulas.size() > 1 ? tr("ebisu_client_eula_agree_plural") : tr("ebisu_client_eula_agree_singular");
    content["eulas"] = eulas;

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-downloadeulasummary", 
        title, text, btnText, tr("ebisu_client_cancel"), "yes", content, this, "onEULAConsolidatedDone");
    mt.acceptEnabled = false;
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void InstallerViewController::onEULAConsolidatedDone(QJsonObject obj)
{
    // Convert the file names back to .tmp files. We do this so if the user cancels the
    // download we will auto clean these files up automatically.
    for (int i = 0, numEulas = mEulaRequest.eulas.listOfEulas.count(); i < numEulas; i++)
    {
        convertFileType(mEulaRequest.eulas.listOfEulas[i].FileName, ".tmp");
    }
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        Downloader::EulaStateResponse eulaState;
        eulaState.acceptedBits = 0xffffffff;
        emit eulaStateDone(mEntitlement->contentConfiguration()->productId(), eulaState);
    }
    else
    {
        emit stopFlow();
    }
}


void InstallerViewController::onEulaSequentialDone(QJsonObject obj)
{
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    if(accepted)
    {
        const int index = Services::JsonValueValidator::validate(obj["index"]).toInt();
        const int numEulas = Services::JsonValueValidator::validate(obj["numeulas"]).toInt();
        if(index == (numEulas - 1))
        {
            Downloader::EulaStateResponse eulaState;
            eulaState.acceptedBits = 0xffffffff;
            emit eulaStateDone(mEntitlement->contentConfiguration()->productId(), eulaState);
        }
    }
    else
    {
        emit stopFlow();
    }
}


void InstallerViewController::showCancelDialog(const Origin::Downloader::CancelDialogRequest& request, const installerUIStates& uiState)
{
    if(mEntitlement.isNull() || mEntitlement->localContent() == NULL || mEntitlement->localContent()->installFlow() == NULL)
        return;

    QString title, text;
    switch(request.state)
    {
    case Downloader::kOperation_ITO:
    case Downloader::kOperation_Install:
        title = tr("ebisu_client_clear_install_uppercase");
        text = tr("ebisu_client_clear_install_text").arg(request.contentDisplayName);
        break;
    case Downloader::kOperation_Preload:
        title = tr("ebisu_client_clear_preload_uppercase");
        text = tr("ebisu_client_clear_preload_text").arg(request.contentDisplayName);
        break;
    case Downloader::kOperation_Repair:
        title = tr("ebisu_client_cancel_repair_question_CAPS");
        text = tr("ebisu_client_cancel_repair_description").arg(request.contentDisplayName);
        break;
    case Downloader::kOperation_Update:
        title = tr("ebisu_client_cancel_update_question_CAPS");
        text = tr("ebisu_client_cancel_update_description").arg(request.contentDisplayName);
        break;
    case Downloader::kOperation_Download:
    default:
        title = tr("ebisu_client_clear_download_uppercase");
        text = tr("ebisu_client_clear_download_text").arg(request.contentDisplayName);
        break;
    }

    bool canCancel = true;
    if(mEntitlement->localContent()->installFlow()->isDynamicDownload() && mEntitlement->localContent()->playing())
        canCancel = false;
    else
        canCancel = mEntitlement->localContent()->installFlow()->canCancel();

    if(request.isIGO)
    {
        UIToolkit::OriginWindow* window = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::NoIcon, title, text, tr("ebisu_client_yes"), tr("ebisu_client_no"));
        window->button(QDialogButtonBox::Yes)->setEnabled(canCancel);
        window->setAttribute(Qt::WA_DeleteOnClose, false);
        ORIGIN_VERIFY_CONNECT_EX(window, SIGNAL(accepted()), this, SIGNAL(stopFlow()), Qt::QueuedConnection);
        pushAndShowErrorWindow(window, request.productId + " - Close");
    }
    else
    {
        DialogController::MessageTemplate mt = DialogController::MessageTemplate("", title, text, tr("ebisu_client_yes"), tr("ebisu_client_no"), "yes", QJsonObject(), this, "onCancelDone");
        mt.rejectGroup = rejectGroup();
        mt.overrideId = request.productId + " - Close";
        mt.acceptEnabled = canCancel;
        mt.highPriority = true;
        if(uiState == ShowRegular)
        {
            DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
        }
        else
        {
            DialogController::instance()->updateMessageDialog(DialogController::SPA, mt);
        }
    }
}


void InstallerViewController::onCancelDone(QJsonObject result)
{
    const bool accepted = result["accepted"].toBool();
    if(accepted)
    {
        emit stopFlow();
    }
}


void InstallerViewController::showCancelWithDLCDependentsDialog(const Origin::Downloader::CancelDialogRequest& request, const QStringList& childrenNames, const installerUIStates& uiState)
{
    QString title, text;
    switch(request.state)
    {
    case Downloader::kOperation_Update:
        title = tr("ebisu_client_sure_you_want_to_cancel_this_update_title");
        text = tr("ebisu_client_dlc_wont_be_installed_if_update_is_cancelled").arg(childrenNames.join(", "));
        break;
    case Downloader::kOperation_Preload:
    case Downloader::kOperation_Repair:
    case Downloader::kOperation_ITO:
    case Downloader::kOperation_Install:
    case Downloader::kOperation_Download:
    default:
        title = tr("HOW DID YOU GET HERE?");
        text = tr("This is awkward...");
        break;
    }

    bool canCancel = true;
    if(mEntitlement->localContent()->installFlow()->isDynamicDownload() && mEntitlement->localContent()->playing())
        canCancel = false;
    else
        canCancel = mEntitlement->localContent()->installFlow()->canCancel();

    if(request.isIGO)
    {
        UIToolkit::OriginWindow* window = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::Notice, title, text, tr("ebisu_client_yes"), tr("ebisu_client_no"));
        window->button(QDialogButtonBox::Yes)->setEnabled(canCancel);
        window->setAttribute(Qt::WA_DeleteOnClose, false);
        ORIGIN_VERIFY_CONNECT_EX(window, SIGNAL(accepted()), this, SIGNAL(stopFlowForSelfAndDependents()), Qt::QueuedConnection);
        pushAndShowErrorWindow(window, request.productId + " - Close");
    }
    else
    {
        DialogController::MessageTemplate mt = DialogController::MessageTemplate("", title, text, tr("ebisu_client_yes"), tr("ebisu_client_no"), "no", QJsonObject(), this, "onCancelWithDLCDependentsDone");
        mt.overrideId = request.productId + " - Close";
        mt.acceptEnabled = canCancel;
        mt.highPriority = true;
        mt.rejectGroup = rejectGroup();
        if(uiState == ShowRegular)
        {
            DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
        }
        else
        {
            DialogController::instance()->updateMessageDialog(DialogController::SPA, mt);
        }
    }
}


void InstallerViewController::onCancelWithDLCDependentsDone(QJsonObject result)
{
    const bool accepted = result["accepted"].toBool();
    if(accepted)
    {
        emit stopFlowForSelfAndDependents();
    }
}


void InstallerViewController::showDLCOperationFinished()
{
    // Keep as origin window. This window is OIG only.
    UIToolkit::OriginWindow* window = UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::NoIcon, 
        tr("ebisu_client_additional_content_has_finished_downloading_title"), 
        tr("ebisu_client_additional_content_has_finished_downloading_description").arg(mEntitlement->contentConfiguration()->displayName()), tr("ebisu_client_close"));
    if(mInDebug)
    {
        pushAndShowErrorWindow(window, mEntitlement->contentConfiguration()->productId() + " - DLCOperationFinished");
    }
    else if(Engine::IGOController::instance()->isActive())
    {
        window->configForOIG();
        Engine::IGOController::instance()->igoShowError(window);
    }
}


void InstallerViewController::showPatchErrorDialog()
{
    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_game_update_apply_fail_header"), 
        tr("ebisu_client_game_update_apply_fail_text"), tr("ebisu_client_retry"), tr("ebisu_client_ok_uppercase"), "yes", QJsonObject(), this, "onPatchErrorDone"));
}


void InstallerViewController::onPatchErrorDone(QJsonObject obj)
{
    if(obj["accepted"].toBool())
    {
        emit continueInstall();
    }
    else
    {
        emit clearRTPLaunch();
    }
}


QString InstallerViewController::rejectGroup() const
{
    return mInDebug ? "" : "InstallerFlow_" + mEntitlement->contentConfiguration()->productId();
}


void InstallerViewController::pushAndShowErrorWindow(const QString& title, const QString& text, const QString& key)
{
    if(mErrorWindows.contains(key))
        mErrorWindows.value(key)->showWindow();
    else
    {
        UIToolkit::OriginWindow* window = UIToolkit::OriginWindow::alertNonModal(UIToolkit::OriginMsgBox::Error, title, text, tr("ebisu_client_close"));

        QLabel* lblText = window->msgBox()->getTextLabel();
        lblText->setObjectName(key);
        lblText->setTextInteractionFlags((Qt::TextInteractionFlags)(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse));
        lblText->setOpenExternalLinks(false);

        ORIGIN_VERIFY_CONNECT(window, SIGNAL(finished(int)), this, SLOT(onRemoveErrorDialog()));

        if(Engine::IGOController::instance()->isActive())
        {
            window->configForOIG();
            Engine::IGOController::instance()->igoShowError(window);
        }
        window->showWindow();
        mErrorWindows.insert(key, window);
    }
}


void InstallerViewController::pushAndShowErrorWindow(UIToolkit::OriginWindow* window, const QString& key)
{
    if(mErrorWindows.contains(key))
    {
        window->deleteLater();
        mErrorWindows.value(key)->showWindow();
    }
    else
    {
        if(Engine::IGOController::instance()->isActive())
        {
            window->configForOIG();
            Engine::IGOController::instance()->igoShowError(window);
        }
        window->showWindow();
        ORIGIN_VERIFY_CONNECT_EX(window, SIGNAL(finished(int)), this, SLOT(onRemoveErrorDialog()), Qt::QueuedConnection);
        mErrorWindows.insert(key, window);
    }
}


void InstallerViewController::onRemoveErrorDialog()
{
    UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(this->sender());
    if(window)
    {
        const QString key = mErrorWindows.key(window);
        mErrorWindows.remove(key);
        ORIGIN_VERIFY_DISCONNECT(window, NULL, this, NULL);
        window->close();
    }
}


void InstallerViewController::showInstallThroughSteam(const installerUIStates& monitored)
{
    GetTelemetryInterface()->Metric_3PDD_INSTALL_TYPE(QString("steam").toUtf8().data());
    const QString displayName = mEntitlement->contentConfiguration()->displayName();
    const QString platformNameText = tr("ebisu_client_steam");
    const QString promptText = tr("ebisu_client_3PDD_install_desc").arg(mEntitlement->contentConfiguration()->displayName()).arg(tr("ebisu_client_steamworks")).arg(platformNameText).arg(tr("application_name"));
    QString step2summary = "";
    if(monitored == ShowMonitored)
        step2summary = tr("ebisu_client_3PDD_monitored_install_through_desc").arg(displayName).arg(platformNameText).arg(platformNameText).arg(tr("application_name"));
    else
    {
        step2summary = tr("ebisu_client_3PDD_install_through_desc").arg(displayName).arg(platformNameText).arg(tr("application_name"));
        step2summary += " ";
        step2summary += tr("ebisu_client_steam_support").arg("contactSteam");
    }
    showThirdParty(promptText, platformNameText, step2summary, monitored);
}


void InstallerViewController::showInstallThroughWindowsLive(const installerUIStates& monitored)
{
    GetTelemetryInterface()->Metric_3PDD_INSTALL_TYPE(QString("gfwl").toUtf8().data());
    const QString displayName = mEntitlement->contentConfiguration()->displayName();
    const QString platformNameText = tr("ebisu_client_games_for_windows_live");
    const QString promptText = tr("ebisu_client_3PDD_monitored_install_summary").arg(displayName).arg(tr("ebisu_client_games_for_windows_live_game")).arg(platformNameText).arg(tr("application_name"));
    QString step2summary = "";
    if(monitored == ShowMonitored)
        step2summary = tr("ebisu_client_3PDD_monitored_install_through_desc").arg(displayName).arg(platformNameText).arg(platformNameText).arg(tr("application_name"));
    else
        step2summary = tr("ebisu_client_3PDD_install_through_desc").arg(displayName).arg(platformNameText).arg(tr("application_name"));
    showThirdParty(promptText, platformNameText, step2summary, monitored);
}


void InstallerViewController::showInstallThrougOther(const installerUIStates& monitored)
{
    GetTelemetryInterface()->Metric_3PDD_INSTALL_TYPE(QString("other").toUtf8().data());
    const QString displayName = mEntitlement->contentConfiguration()->displayName();
    const QString platformNameText = tr("ebisu_client_3PDD_an_external_application");
    const QString promptText = tr("ebisu_client_3PDD_monitored_install_summary").arg(displayName).arg(tr("ebisu_client_3PDD_game")).arg(platformNameText).arg(tr("application_name"));
    QString step2summary = "";
    if(monitored == ShowMonitored)
        step2summary = tr("ebisu_client_3PDD_monitored_install_through_desc").arg(displayName).arg(platformNameText).arg(platformNameText).arg(tr("application_name"));
    else
        step2summary = tr("ebisu_client_3PDD_install_through_desc").arg(displayName).arg(platformNameText).arg(tr("application_name"));
    showThirdParty(promptText, platformNameText, step2summary, monitored);
}


void InstallerViewController::showThirdParty(const QString& promptText, const QString& platformNameText, const QString& step2summary, const bool& monitoredInstall)
{
    GetTelemetryInterface()->Metric_3PDD_INSTALL_DIALOG_SHOW();
    QJsonObject contentObj;
    const QString key = mEntitlement->contentConfiguration()->cdKey().count() ? mEntitlement->contentConfiguration()->cdKey() : "no_cd_key";
    contentObj["step1title"] = tr("ebisu_client_3PDD_1_copy_your_product_code");
    contentObj["step1desc"] = tr("ebisu_client_3PDD_copy_your_product_code_desc");
    contentObj["cdkeytitle"] = tr("ebisu_client_product_code_with_colon");
    contentObj["cdkey"] = key;
    contentObj["step2title"] = tr("ebisu_client_3PDD_2_Install_Through").arg(platformNameText);
    contentObj["step2desc"] = DialogController::MessageTemplate::toAttributeFriendlyText(step2summary);

    QJsonObject persistingData;
    persistingData["monitoredInstall"] = monitoredInstall;

    DialogController::MessageTemplate mt = DialogController::MessageTemplate("origin-dialog-content-downloadthirdparty", tr("ebisu_client_important_installation_notice").toUpper(), 
        promptText, tr("ebisu_client_install"), tr("ebisu_client_cancel"), "yes", contentObj, this, "on3PDDInstallDone", persistingData);
    mt.rejectGroup = rejectGroup();
    DialogController::instance()->showMessageDialog(DialogController::SPA, mt);
}


void InstallerViewController::on3PDDInstallDone(QJsonObject result)
{
    const bool accepted = result["accepted"].toBool();
    const bool monitoredInstall = result["monitoredInstall"].toBool();

    if (mEntitlement->localContent()->installFlow())
        mEntitlement->localContent()->installFlow()->install(accepted);

    if(accepted)
    {
        if(monitoredInstall)
        {
            onMinimizeClientToTray();
        }
    }
    else
    {
        GetTelemetryInterface()->Metric_3PDD_INSTALL_DIALOG_CANCEL();
    }
}


void InstallerViewController::onMinimizeClientToTray()
{
	ClientFlow::instance()->minimizeClientToTray();
}


// bringing this over from CoreError.cpp from 8.6 for the following reasons
// "We need to have some fairly extensive error messaging to the user.  
//  Historically we did localization by encoding errors into a localizable key.  
//  (example EC:7049:404)  In the string table if there was a user visible string it would use that.  
//  If not it would show the error code.  There were far too many error codes to localize all of them 
//  due to also having Windows OS errors and HTTP errors, etc.
//
// "We couldnt just show an English error because of the Tulum laws; cant show English to the French or they might combust."
// - Alex Z.

// returns a string formatted as such:
//		Error Code:<service id>:<error number>
//		Description of what went wrong
//		Suggested corrective actions
const QString InstallerViewController::GetGenericErrorString(int serviceId, int iErrorCode)
{
	// This is the return string
	QString sErrorString;

	QString sErrorLocator;           // This is the error code in corestrings.xml
	QString sProblemDescription;     // This is the error code in message format
	QString sProblemCorrection;      // This is the error code's suggested corrective action.

	// Create the XML Error Locator Entry (EC:<serviceId>:<ErrorNumber>
	sErrorLocator.append(QString("EC:%1:%2").arg(serviceId).arg(iErrorCode));
	sProblemDescription = tr(sErrorLocator.toLatin1().constData());

	// Find the problem description, and the corrective actions	
	if(sProblemDescription != sErrorLocator)
	{ 
		// Line 2 : Problem Description
		sErrorString.append(sProblemDescription);

		// Find the corrective action. 
		// Create the XML Error Locator Entry "EC:<serviceId>:<ErrorNumber>-CA"
		sErrorLocator.append("-CA");

		sProblemCorrection = tr(sErrorLocator.toLatin1().constData());

		if(sProblemDescription != sErrorLocator)
			// Line 3: Corrective Actions
			sErrorString.append(QString("<br>%1").arg(sProblemCorrection));
	}
	else  // No specific error code - use generic service error
	{
		// Create the XML Service Error Locator Entry (EC:<serviceId>)
		sErrorLocator = QString("EC:%1").arg(serviceId);
		sProblemDescription = tr(sErrorLocator.toLatin1().constData());

		// Find the problem description, and the corrective actions	
		if(sProblemDescription != sErrorLocator)
		{
			// Line 2 : Problem Description
			sErrorString.append(sProblemDescription);

			// Create the XML Error Locator Entry "EC:<serviceId>-CA"
			sErrorLocator.append("-CA");

			sProblemCorrection = tr(sErrorLocator.toLatin1().constData());

			if(sProblemDescription != sErrorLocator)
				// Line 3: Corrective Actions
				sErrorString.append(QString("<br>%1").arg(sProblemCorrection));

		}
		// else - not even a generic service error message. 
		// We are stuck with a numeric error.
	}

	if(!sErrorString.isEmpty())
		sErrorString.append("<br>");

	// Line 1: Localized readable error code
	sErrorString.append(QString("%1 %2:%3").arg(tr("generic_error")).arg(serviceId).arg(iErrorCode));
	return sErrorString;
}


// from 8.6 CoreError
enum ServiceID
{
	kDownloadMgr		= 7049,
	kDecryptMgr			= 32791,
	kUnpackMgr			= 17854,
	kCoreSvc			= 60122,    // 0xEADA
	kContentEntitlement = 10000,
	kHTTPConnection		= 49220,	//0xco44;
	kUnknown			= 5
};


void InstallerViewController::logUserFacingErrors(const QString& error, const QString& context)
{
    if(mInDebug == false)
    {
        const QString productId = mEntitlement->contentConfiguration()->productId();
        //  LOG:  Notify error about installer flow
        ORIGIN_LOG_EVENT << "Error(" << productId << "): " << error << ", Context(" << productId << "): " << context;

        //  TELEMETRY:  Notify error about installer flow
        const QString areaError = QString("InstallerView_UserFacing:" + error);
        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(productId.toLocal8Bit().constData(), areaError.toUtf8().data(), context.toUtf8().data(), __FILE__, __LINE__);
    }
}


QPair<QString, QString> InstallerViewController::installErrorText(qint32 type, qint32 code, const QString& context)
{
    QString openOER = "";
#if defined(ORIGIN_PC)
    openOER = QString("<br/><br/>%1").arg(tr("ebisu_client_help_us_improve_submit_error_report").arg(tr("application_name")));
    openOER.remove("\\");
#endif
    if(mEntitlement.isNull() || !mEntitlement->localContent()->installFlow())
    {
        if (mEntitlement.isNull())
            logUserFacingErrors("InstallerViewController::installErrorText", "Entitlement does not exist");
        else
            logUserFacingErrors("InstallerViewController::installErrorText", "InstallFlow does not exist");

        QString errorTitle = tr("ebisu_client_error_uppercase");
        QString errorMessage = tr("error_generic").arg(QString("%1:%2").arg(type).arg(code));
        errorMessage += openOER;
        return QPair<QString, QString>(errorTitle, errorMessage);
    }

    Downloader::ContentInstallFlowState flowstate = mEntitlement->localContent()->installFlow()->getFlowState();
    const QString mProductId = mEntitlement->contentConfiguration()->productId();
    const QString gameName = !mEntitlement.isNull() ? mEntitlement->contentConfiguration()->displayName() : tr("ebisu_igo_unknown_game");

    if(mInDebug == false)
        GetTelemetryInterface()->Metric_DL_ERRORBOX(mProductId.toLocal8Bit().constData(), code, type);
    QString errorTitle = tr("ebisu_client_error_uppercase");
    QString errorMessage = tr("error_generic").arg(QString("%1:%2").arg(type).arg(code));

    switch(type)
    {
    case Downloader::DownloadError::FileOpen:
        if(mInDebug || flowstate == Downloader::ContentInstallFlowState::kUnpacking)
        {
            // EC:17854:1 - couldn't open viv file
            errorMessage = GetGenericErrorString(kUnpackMgr, 2);
            logUserFacingErrors("DownloadError::FileOpen", "Cannot open VIV file");
        }
        break;

    case Downloader::DownloadError::FileRead:
        errorTitle = tr("ebisu_client_download_forbidden_error_title");
        if(mEntitlement->contentConfiguration()->hasOverride() && (flowstate == Downloader::ContentInstallFlowState::kReadyToStart))
        {
            errorMessage = tr("ebisu_client_download_override_error");
            logUserFacingErrors("DownloadError::FileRead", "Download override error");
        }
        else if(mInDebug || flowstate == Downloader::ContentInstallFlowState::kUnpacking)
        {
            // EC:17854:4 - couldn't read viv file
            errorMessage = tr("ebisu_client_file_read_error_text");
            logUserFacingErrors("DownloadError::FileRead", "Cannot read VIV file");
        }
        break;

    case Downloader::DownloadError::FileWrite:
    case Downloader::UnpackError::IOWriteFailed:
    case Downloader::UnpackError::IOCreateOpenFailed:
    case Downloader::ProtocolError::ContentFolderError:
    case Downloader::UnpackError::DirectoryNotFound:
    case Downloader::PatchBuilderError::FileWriteFailure:
        if(code == Services::PlatformService::ErrorDiskFull)
        {
            errorTitle = tr("ebisu_client_hard_drive_full_caps");
            errorMessage = tr("ebisu_client_hard_drive_full_description");
            logUserFacingErrors("DownloadError::FileWrite", "Disk is full");
        }
        else if (code == Services::PlatformService::ErrorAccessDenied)
        {
            errorTitle = tr("ebisu_client_download_didnt_work_caps");
            errorMessage = tr("ebisu_client_download_failed_due_disc_corruption").arg(context);
            logUserFacingErrors(QString("Downloader::AccessDenied (%1:%2)").arg(type).arg(code), "File/folder write error during download");
        }
        else
        {
            errorTitle = tr("ebisu_client_generic_download_error_title");
            errorMessage = tr("ebisu_client_folder_not_available_error").arg(gameName).arg(context);
            logUserFacingErrors(QString("Downloader::IOError (%1:%2)").arg(type).arg(code), "File/folder write error during download");
        }
        break;

    case Downloader::FlowError::OSRequirementNotMet:
        {
            QString osName;
            int32_t osMajor = (code >> 24);
            int32_t osMinor = (code >> 16) & 0x000000ff;
            //int32_t osBuild = (code >>  8) & 0x000000ff;
            //int32_t osRev   = (code >>  0) & 0x000000ff;

#if defined(ORIGIN_PC)
            switch(osMajor * 10 + osMinor)
            {
            case 51:
                osName = tr("ebisu_client_windows_xp");
                break;
            case 60:
                osName = tr("ebisu_client_windows_vista");
                break;
            case 61:
                osName = tr("ebisu_client_windows_7");
                break;
            case 62:
                osName = tr("ebisu_client_windows_8");
                break;
            default:
                osName = tr("--");
                break;
            }
            QString osVersion(QString::number(osMajor).append(".").append(QString::number(osMinor)));
#elif defined(ORIGIN_MAC)
            osName = tr("ebisu_client_osx");
            int32_t osBuild = (code >>  8) & 0x000000ff;
            QString osVersion(QString("%1.%2.%3").arg(osMajor).arg(osMinor).arg(osBuild));
#else
#error "require platform-specific mapping"
#endif

            errorTitle = tr("ebisu_os_unsupported_title");
            
#if defined(ORIGIN_PC)
            bool isRunningInCompatibilityMode = !Origin::Services::PlatformService::getCompatibilityMode().isEmpty();
#else
            bool isRunningInCompatibilityMode = false;  // no compatibility mode on OSX
#endif
            errorMessage = isRunningInCompatibilityMode ? tr("ebisu_client_os_requirement_error_text").arg(gameName).arg(osName).arg(osVersion) + tr(" ") + tr("ebisu_client_os_compatibilitymode_error_game_launch_message") : tr("ebisu_client_os_requirement_error_text").arg(gameName).arg(osName).arg(osVersion);

            logUserFacingErrors("FlowError::OSRequirementNotMet", QString("Min OS requirement not met (Running in compatibility mode %1)").arg(isRunningInCompatibilityMode));
        }
        break;

        case Downloader::FlowError::ClientVersionRequirementNotMet:
        {   
            int32_t vMajor = (code >> 24);
            int32_t vMinor = (code >> 16) & 0x000000ff;
            int32_t vBuild = (code >>  8) & 0x000000ff;
            //int32_t vRev   = (code >>  0) & 0x000000ff;
            QString clientVersion = QString("%1.%2.%3").arg(vMajor).arg(vMinor).arg(vBuild);

            errorTitle = tr("ebisu_client_unsupported_title");
            errorMessage = tr("ebisu_client_version_requirement_error_text").arg(gameName).arg(clientVersion);
            logUserFacingErrors("FlowError::ClientVersionRequirementNotMet", "Min client version requirement not met");
        }
        break;

    case Downloader::ProtocolError::EscalationFailureAdminRequired:
    case Downloader::ProtocolError::EscalationFailureUnknown:
    case Downloader::ProtocolError::EscalationFailureUACTimedOut:
    case Downloader::ProtocolError::EscalationFailureUserCancelled:
    case Downloader::ProtocolError::EscalationFailureUACRequestAlreadyPending:
        {
            errorTitle = tr("ebisu_client_download_paused_header");
            bool isITO = OriginApplication::instance().isITE();
            if(isITO)
                errorMessage = tr("ebisu_client_escalation_fail_error_text_install").arg("<strong>"+mEntitlement->contentConfiguration()->displayName()+"</strong>");
            else
                errorMessage = tr("ebisu_client_escalation_fail_error_text").arg("<strong>"+mEntitlement->contentConfiguration()->displayName()+"</strong>");

            QString error = Downloader::ErrorTranslator::ErrorString(static_cast<Origin::Downloader::ContentDownloadError::type>(type));
            logUserFacingErrors(error, "Download could not start");
        }
        break;

    case Downloader::ProtocolError::ContentInvalid:
    case Downloader::ProtocolError::ZipContentInvalid:
    case Downloader::ProtocolError::RedownloadRequestFailed:
        errorTitle = tr("ebisu_client_download_forbidden_error_title");
        errorMessage = tr("ebisu_client_redownload_request_fail_text");
        logUserFacingErrors("ProtocolError::ContentInvalid", "Content Invalid");
        break;

    case Downloader::FlowError::JitUrlRequestFailed:
        if(code == Services::restErrorInvalidBuildIdOverride)
        {
            errorTitle = tr("ebisu_client_notranslate_invalid_build_id_override_title");
            errorMessage = tr("ebisu_client_notranslate_invalid_build_id_override_text");
            logUserFacingErrors("FlowError::JitUrlRequestFailed", "Invalid Build Id Override");
        }
        else
        {
            errorTitle = tr("ebisu_client_download_forbidden_error_title");
            errorMessage = tr("ebisu_client_download_session_error_text");
            logUserFacingErrors("FlowError::JitUrlRequestFailed", "JITURL failed");
        }
        break;

    case Downloader::ProtocolError::DownloadSessionFailure: //131076
    case Downloader::DownloadError::TemporaryNetworkFailure: // 196624
    case Downloader::DownloadError::NetworkHostNotFound: // 196621
    case Downloader::DownloadError::NetworkProxyConnectionRefused: // 196625
    case Downloader::DownloadError::NetworkProxyConnectionClosed: // 196626
    case Downloader::DownloadError::NetworkOperationCanceled: // 196622
        {
            errorTitle = tr("ebisu_client_download_forbidden_error_title");
            errorMessage = tr("ebisu_client_network_timeout_error_text");
            logUserFacingErrors("DownloadError::TemporaryNetworkFailure", "Download failed (network)");
        }
        break;
    case Downloader::DownloadError::HttpError:
        errorTitle = tr("ebisu_client_network_timeout_header");
        errorMessage = tr("ebisu_client_network_timeout_error_text");
        logUserFacingErrors("DownloadError::HttpError", QString::number(code));
        break;

    case Downloader::UnpackError::CRCFailed:	// from old 8.6 CoreError.cpp localized error strings (see comments above function)
        if(mInDebug || flowstate == Downloader::ContentInstallFlowState::kUnpacking)
        {
            // EC:17854:9 - packed file corrupt
            errorMessage = GetGenericErrorString(kUnpackMgr, 9);
            logUserFacingErrors("ContentInstallFlowState::kUnpacking", "Corrupt packed file");
        }
        break;

    case Downloader::FlowError::DecryptionFailed:
        if(code == -1)
        {
            // EC:32791:16 - decrypt problem occurred - restart
            errorMessage = GetGenericErrorString(kDecryptMgr, 16);
            logUserFacingErrors("FlowError::DecryptionFailed", "Problem occurred");
        }
        else if(code == -2)
        {
            // EC:32791:17 - decrypt problem occurred - temp server problem - wait & try again
            errorMessage = GetGenericErrorString(kDecryptMgr, 17);
            logUserFacingErrors("FlowError::DecryptionFailed", "Server problem");
        }
        else
        {
            // EC:32791:<code> codes 15-23 are from 8.6 
            errorMessage = GetGenericErrorString(kDecryptMgr, code);
            logUserFacingErrors("FlowError::DecryptionFailed", QString::number(code));
        }
        break;

    case Downloader::InstallError::Escalation:
        if(code == Escalation::kCommandErrorNotElevatedUser)
        {
            errorMessage = tr("error_must_be_admin");
            logUserFacingErrors("Escalation::kCommandErrorNotElevatedUser", "Admin escalation");
        }
        else if(code == Escalation::kCommandErrorProcessExecutionFailure)
        {
            errorMessage = tr("escalation_install_could_not_start");
            logUserFacingErrors("Escalation::kCommandErrorNotElevatedUser", "Escalation failure");
        }
        else if(code == Escalation::kCommandErrorUACRequestAlreadyPending)
        {
            errorMessage = tr("escalation_update_install_could_not_start");
            logUserFacingErrors("Escalation::kCommandErrorUACRequestAlreadyPending", "Escalation failure");
        }
        break;

    case Downloader::FlowError::LanguageSelectionEmpty:
        {
            errorTitle = tr("ebisu_client_error_no_languages_title");
            errorMessage = tr("ebisu_client_error_no_languages_text");
            logUserFacingErrors("FlowError::LanguageSelectionEmpty", "required language not found in install");
        }
        break;

    default:
        break;
    }

    errorMessage += openOER;
    return QPair<QString, QString>(errorTitle, errorMessage);
}


void InstallerViewController::onDialogLinkClicked(const QJsonObject& obj)
{
    const QString href = Services::JsonValueValidator::validate(obj["href"]).toString();
    // If the print link was clicked and there isn't an active modal dialog showing (a print or save dialog)
    if (href.contains("eula-", Qt::CaseInsensitive))
    {
        const int eulaIndex = href.split("eula-")[1].toInt();
        if (mEulaRequest.eulas.listOfEulas.isEmpty())
        {
            ORIGIN_LOG_ERROR << QString("No eulas to open %0 (%1)").arg(mEntitlement->contentConfiguration()->displayName()).arg(mEntitlement->contentConfiguration()->productId());
            return;
        }

        Downloader::Eula& curEula = mEulaRequest.eulas.listOfEulas[eulaIndex];
        if (curEula.IsRtfFile)
        {
            convertFileType(curEula.FileName, ".rtf");
        }
        else if (Downloader::StringHelpers::containsHtmlTag(curEula.Description))
        {
            convertFileType(curEula.FileName, ".html");
        }
        else
        {
            convertFileType(curEula.FileName, ".txt");
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(curEula.FileName));
    }
    else if(href.contains("contactSteam", Qt::CaseInsensitive))
    {
        Services::PlatformService::asyncOpenUrl(QUrl::fromEncoded(Services::readSetting(Services::SETTING_SteamSupportURL).toString().toUtf8()));
    }
}

bool InstallerViewController::convertFileType(QString& filename, const QString& fileType)
{
    QFile file(filename);
    if (file.exists())
    {
        filename = filename.split(".").first() + fileType;
        file.rename(filename);
        return true;
    }
    return false;
}

}
}