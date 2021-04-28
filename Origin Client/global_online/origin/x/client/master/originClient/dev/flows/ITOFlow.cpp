//  ITOFlow.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include <stdint.h>
#include "flows/ITOFlow.h"
#include "engine/ito/InstallFlowManager.h"
#include "OriginApplication.h"
#include "ITEUIManager.h"
#include "TelemetryAPIDLL.h"
#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "services/log/LogService.h"
#include "flows/ClientFlow.h"
#include "CommandLine.h"
#include "StoreUrlBuilder.h"
#include "services/network/NetworkAccessManager.h"
#include "services/common/VersionInfo.h"
using namespace Origin::Engine;
using namespace Origin::Services;
using namespace Origin::Client;

void ReadContentIds(ScratchPad &scratchpad, QDomDocument &doc)
{
    QDomNodeList contentIds = doc.elementsByTagName("contentID");

    int count = contentIds.size();

    if(count>0)
    {
        QDomElement elem = contentIds.at(0).toElement();
        QString ContentId = elem.text();
        QString ContentIds;

        for(int i=0; i<contentIds.size(); i++)
        {
            elem = contentIds.at(i).toElement();
            ContentIds += elem.text() + ",";
        }

        ContentIds = ContentIds.left(ContentIds.length()-1);

        scratchpad.SetValue("ContentId", ContentId);
        scratchpad.SetValue("ContentIds", ContentIds);
    }
}

void ReadFreeGameOfferId(ScratchPad &scratchpad, QDomDocument &doc)
{
    QDomNodeList offerIdList = doc.elementsByTagName("freeGameRedeemOfferId");
    if(offerIdList.size())
    {
        QDomElement elem = offerIdList.at(0).toElement();
        QString offerId = elem.text();
        scratchpad.SetValue("RedeemOfferId", offerId);
    }
}


QString ReadGameTitle(QString locale, QDomDocument &doc)
{
    QString         title;

    // check for DIP manifest version 4.0 or higher
    bool preDIP4manifest = true;

    QDomElement oDiPManifestHeader = doc.elementsByTagName(QString("DiPManifest")).item(0).toElement();
    if (!oDiPManifestHeader.isNull() && QDomNode::ElementNode == oDiPManifestHeader.nodeType())
    {
        // Retrieve the manifest version
        if (Origin::VersionInfo(oDiPManifestHeader.attribute("version")) >= Origin::VersionInfo(4,0))
        {
            preDIP4manifest = false;
        }
    }

    if (preDIP4manifest)
    {
        QDomNodeList    localeInfos = doc.elementsByTagName("localeInfo");

        for (int i=0; i<localeInfos.size(); i++)
        {
            QDomNode node = localeInfos.at(i);
            QString loc = node.attributes().namedItem("locale").toAttr().value();
            if (loc == locale)
            {
                title = node.firstChildElement("title").text();
                break;
            }
            else if ((loc == "en_US") || (i == 0))
            {
                title = node.firstChildElement("title").text();
            }
        }
    }
    else
    {
        QDomNodeList gameTitles = doc.elementsByTagName("gameTitle");

        for (int i=0; i < gameTitles.size(); i++)
        {
            QDomNode node = gameTitles.at(i);
            QString loc = node.attributes().namedItem("locale").toAttr().value();
            QDomElement node_element = node.toElement();
            if (loc == locale)
            {
                title = node_element.text();
                break;
            }
            else if ((loc == "en_US") || (i == 0))
            {
                title = node_element.text();
            }
        }
    }

    return title;
}

void ReadGameTitle(ScratchPad &scratchpad, QString locale, QDomDocument &doc)
{
    QString title = ReadGameTitle(locale, doc);

    scratchpad.SetValue("GameName", title);
}

void ProcessCommandLine(ScratchPad &scratchpad)
{    
    QString CommandLine = Origin::Services::readSetting(Origin::Services::SETTING_COMMANDLINE, Origin::Services::Session::SessionRef());
    QStringList argv = Origin::Client::CommandLine::parseCommandLineWithQuotes(CommandLine);
    int argc = argv.size();

    QDomDocument metadatadoc;

    QString CONTENTIDS = "/contentIds:";
    QString LANGUAGE = "/language:";
    QString CONTENTLOCATION = "/contentLocation:";
    QString DOWNLOAD_REQUIRED = "/downloadRequired";
    QString FREE_GAME = "/free";
    QString bannerPath = "";
    QString F2P_EULA_PREAPPROVED = "/EULAsPreApproved";
    QString F2P_GAMEOPTIONS = "/HideGameInstallOptions";
    QString IS_DLC = "/isITODLC";

    bool isUsingLocalSource = false;

    InstallFlowManager::GetInstance().SetVariable("ContentIsPDLC", false);
    InstallFlowManager::GetInstance().SetVariable("WaitForPDLCRefresh", false); // this is to wait for extra content refresh (default OFF)

    for(int i=0; i<argc; i++)
    {
        QString Token = argv[i];

        if(Token.startsWith(CONTENTIDS))
        {
            QString path = Token.mid(CONTENTIDS.length());

            // Remove outer quotes
            path.replace(QRegExp("^\"|\"$"), ""); // remove leading and trailing quotes

            // Parse out the path for the banner image
            bannerPath = path.left(path.lastIndexOf(QString("\\")));

            QFile file(path);
            if(file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                if(metadatadoc.setContent(&file))
                {
                    QString locale = QLocale().name();
                    if (locale == "nb_NO")
                        locale = "no_NO";
                    ReadContentIds(scratchpad, metadatadoc);
                    ReadGameTitle(scratchpad, locale, metadatadoc);
                    ReadFreeGameOfferId(scratchpad,metadatadoc);
                }
            }
            else
            {
                scratchpad.SetValue("ProcessCommandLineValid", false);
                return;
            }
        }

        if(Token.startsWith(FREE_GAME))
        {
            scratchpad.SetValue("FreeToPlay", true);
        }

        if (Token.startsWith(F2P_EULA_PREAPPROVED, Qt::CaseInsensitive))
        {
            scratchpad.SetValue("F2PEulasAccepted", true);     // Free to Play optimization to bypass eulas (already accepted on the webpage)
        }

        if (Token.startsWith(F2P_GAMEOPTIONS, Qt::CaseInsensitive))
        {
            scratchpad.SetValue("F2PDefaultInstall", true);    // Free to Play optimization to install with default settings (install location, create desktop/start menu shortcuts)
        }

        if(Token.startsWith(LANGUAGE))
        {
            QString locale = Token.mid(LANGUAGE.length());
            scratchpad.SetValue("Locale", locale);
            ReadGameTitle(scratchpad, locale, metadatadoc);
        }

        if(Token.startsWith(CONTENTLOCATION))
        {

            QString source = Token.mid(CONTENTLOCATION.length());
            // Remove outer quotes
            source.replace(QRegExp("^\"|\"$"), ""); // remove leading and trailing quotes

            if(source.isEmpty())
            {
                ORIGIN_ASSERT_MESSAGE(0, "/ContentLocation param passed in but EMPTY");
                ORIGIN_LOG_ERROR << "ITO FLOW: /ContentLocation passed in but EMPTY";
            }
            else
            {
                isUsingLocalSource = true;
            }
            scratchpad.SetValue("Source", source);
        }
        if(Token == DOWNLOAD_REQUIRED)
        {
            scratchpad.SetValue("DownloadRequired", true);
        }

        if (Token == IS_DLC)
        {
            InstallFlowManager::GetInstance().SetVariable("ContentIsPDLC", true);
            InstallFlowManager::GetInstance().SetVariable("WaitForPDLCRefresh", true);
        }
    }

    scratchpad.SetValue("UsingLocalSource", isUsingLocalSource);

    // Create both a localized and non-localized path to the banner image (which may or may not exist on the installation disk)
    InstallFlowManager::GetInstance().SetVariable("BannerPath", bannerPath + "\\banner.png");
    InstallFlowManager::GetInstance().SetVariable("BannerPathLocale", bannerPath + "\\banner_" + scratchpad.GetValue("Locale").toString() + ".png");


    Origin::Services::SettingsManager::instance()->unset(Origin::Services::SETTING_COMMANDLINE);
    scratchpad.SetValue("ProcessCommandLineValid", true);


    //ITEUIManager::GetInstance()->postInit();
}

void Init(ScratchPad &scratchpad)
{
    // wait for the entitlement refresh that was kicked off after logging in
    OriginApplication& app = OriginApplication::instance();

    app.setITEActive(true);

    // EBIBUGS-27913: Disconnect if it was already connected.
    ORIGIN_VERIFY_DISCONNECT(Origin::Engine::LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), ITEUIManager::GetInstance(), SLOT(onLoggedOut()));
    ORIGIN_VERIFY_DISCONNECT(&InstallFlowManager::GetInstance(), SIGNAL(stateChanged(QString)), ITEUIManager::GetInstance(), SLOT(StateChanged(QString)));

    // Reconnect
    ORIGIN_VERIFY_CONNECT(Origin::Engine::LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), ITEUIManager::GetInstance(), SLOT(onLoggedOut()));
    ORIGIN_VERIFY_CONNECT_EX(&InstallFlowManager::GetInstance(), SIGNAL(stateChanged(QString)), ITEUIManager::GetInstance(), SLOT(StateChanged(QString)), Qt::QueuedConnection);

    scratchpad.SetValue("RedeemOfferId", "Invalid_redeemOfferId");
    scratchpad.SetValue("ContentId", "Invalid_contentId");
    scratchpad.SetValue("ContentIds", "Invalid_contentIds");
    scratchpad.SetValue("GameName", QObject::tr("ebisu_client_3PDD_game"));
    scratchpad.SetValue("Banner", "Invalid Banner");
    scratchpad.SetValue("Source", "Invalid Path");
    scratchpad.SetValue("Locale", "");
    scratchpad.SetValue("ProcessCommandLineValid", false);
    scratchpad.SetValue("RefreshEntitlements", false);
    scratchpad.SetValue("EntitlementsRefreshFinished", true);//this needs to be true for the initial flow, if code redemption is required, it will be reset to false at that point
    scratchpad.SetValue("BaseGameInstalled", false);    //to distinguish between installation finished for base game or for update
    scratchpad.SetValue("WaitingCommand", "");    //to distinguish between installation finished for base game or for update
    scratchpad.SetValue("RefreshSuccess", true);  //assume success
    scratchpad.SetValue("UsingLocalSource", true);
    scratchpad.SetValue("FreeToPlay", false);
    scratchpad.SetValue("GameDetailsShown", false);
    scratchpad.SetValue("F2PEulasAccepted", false);     // Free to Play optimization to bypass eulas (already accepted on the webpage)
    scratchpad.SetValue("F2PDefaultInstall", false);    // Free to Play optimization to install with default settings (install location, create desktop/start menu shortcuts)
    GetTelemetryInterface()->Metric_ITE_CLIENT_RUNNING(true);
}

void Init_ProcessCommandLine(ScratchPad &scratchpad)
{
    ProcessCommandLine(scratchpad);
}

void Init_PostProcessCommandLine(ScratchPad &scratchpad)
{
    QString dipLocation = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR, Origin::Services::Session::SessionRef());
    scratchpad.SetValue("InstallFolder", dipLocation);
    scratchpad.SetValue("InstallSize", -1);
    scratchpad.SetValue("AvailableSpace", PlatformService::GetFreeDiskSpace(dipLocation));
    scratchpad.SetValue("DesktopShortcut", true);
    scratchpad.SetValue("ProgramShortcut", true);
    scratchpad.SetValue("PlayGame", false);
    scratchpad.SetValue("CompleteDialogType", ITOFlow::InstallFailed); // 0 - install success, 1 - install failed, 2 - patch failed
    scratchpad.SetValue("ProgressDialogType", 0); // 0 - regular install 1- patch install
    scratchpad.SetValue("DownloadRequired", false);
    scratchpad.SetValue("SuppressCancelState", false);
    scratchpad.SetValue("Failed", false);
}

bool UsingLocalSource (ScratchPad &scratchpad)
{
    return scratchpad.GetValue("UsingLocalSource").toBool();
}

bool NotUsingLocalSource (ScratchPad &scratchpad)
{
    return !UsingLocalSource(scratchpad);
}

bool FreeToPlay (ScratchPad &scratchpad)
{
    return scratchpad.GetValue("FreeToPlay").toBool();
}

bool NotFreeToPlay (ScratchPad &scratchpad)
{
    return !FreeToPlay(scratchpad);
}

void CleanUp(ScratchPad &/*scratchPad*/)
{
    OriginApplication& app = OriginApplication::instance();
    app.setITEActive(false);

    InstallFlowManager::GetInstance().SetVariable("ContentIds", "");  // unset as LocalContent::downloadAllPDLC() uses it to see if it needs to exclude a DLC from auto-downloading

    // to prevent double-connections cause ITEUIManager to get multiple calls on subsequent ITO sessions.
    ORIGIN_VERIFY_DISCONNECT(&InstallFlowManager::GetInstance(), SIGNAL(stateChanged(QString)), ITEUIManager::GetInstance(), SLOT(StateChanged(QString)));

    Origin::Services::SettingsManager::instance()->unset(Origin::Services::SETTING_COMMANDLINE);
}

void CheckDiskSpace(ScratchPad &scratchpad)
{
    QString dipLocation = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR, Origin::Services::Session::SessionRef());
    scratchpad.SetValue("AvailableSpace", PlatformService::GetFreeDiskSpace(dipLocation));
}

void RefreshEntitlements(ScratchPad &/*scratchpad*/)
{
    //reset it before we call refresh
    InstallFlowManager::GetInstance().SetVariable("EntitlementsRefreshFinished", false);
    InstallFlowManager::GetInstance().SetVariable("RefreshSuccess", true);  //assume success
    Origin::Engine::Content::ContentController::currentUserContentController()->refreshUserGameLibrary(Origin::Engine::Content::ContentUpdates);
}

void CheckEntitlements(ScratchPad &/*scratchpad*/)
{
    // get content ids from disk
    QString contentIds = InstallFlowManager::GetInstance().GetVariable("ContentIds").toString();
    QStringList contentIdList;
    contentIdList = contentIds.split(",");

    // search for content ids
    InstallFlowManager::GetInstance().SetVariable("ContentIdToInstall", "");

    for( int i = 0; i < contentIdList.size(); ++i )
    {
        Origin::Engine::Content::EntitlementRef entitlement = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId((contentIdList.at(i)));

        if(entitlement)
        {
            // save off the content id of the game to be installed
            InstallFlowManager::GetInstance().SetVariable("ContentIdToInstall", contentIdList.at(i));
            InstallFlowManager::GetInstance().SetVariable("GameName", entitlement->contentConfiguration()->displayName());
            break;
        }
    }
}

void ResetCodeRedeemed(ScratchPad &/*scratchpad*/)
{
    InstallFlowManager::GetInstance().SetVariable("CodeRedeemed", false);
}

void LaunchGame(ScratchPad &scratchPad)
{
    QString content = scratchPad.GetValue("ContentIdToInstall").toString();
    Origin::Engine::Content::EntitlementRef entitlement = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(content);
    if(entitlement)
        entitlement->localContent()->play();
    ORIGIN_LOG_EVENT << "Install Through Ebisu: Launching game " << content.toUtf8().constData();
}

void NotReadyToDownloadError (ScratchPad &scratchPad)
{

    QString contentIdToInstall = InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
    Origin::Engine::Content::EntitlementRef entitlement = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);
    if( entitlement && !entitlement->localContent()->installed(true))
    {
        if (entitlement->localContent()->state() == Origin::Engine::Content::LocalContent::Unreleased)
        {
            ITEUIManager::GetInstance()->showNotReadyToDownloadError (contentIdToInstall);
        }
    }
}

void StartDownload (ScratchPad &scratchPad)
{
    scratchPad.SetValue("Command", "");
    scratchPad.SetValue("ProgressDialogType", 0);

    QString content = scratchPad.GetValue("ContentIdToInstall").toString();
    Origin::Engine::Content::EntitlementRef entitlement = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(content);
    if(entitlement)
    {
        entitlement->localContent()->download( scratchPad.GetValue("Locale").toString());
        ORIGIN_LOG_EVENT << "Install Through Ebisu: Starting download(from server) for  " << content.toUtf8().constData();
    }
}


void StartInstall(ScratchPad &scratchPad)
{
    QString content = scratchPad.GetValue("ContentIdToInstall").toString();

    scratchPad.SetValue("ProgressDialogType", 0);

    Origin::Engine::Content::EntitlementRef entitlement = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(content);

    if(entitlement)
    {
        //since we come back thru here on retry while on Prepare Download dialog (after installfolder is changed or disk space is changed), we don't want to restart the download
        //so check if one already exists
        bool showingPrepareDownloadDialog = false;
        if (entitlement->localContent()->installFlow() && 
            entitlement->localContent()->installFlow()->isActive() && 
            entitlement->localContent()->installFlow()->getFlowState() == Origin::Downloader::ContentInstallFlowState::kPendingInstallInfo)
        {
            showingPrepareDownloadDialog = true;
        }

        if (!showingPrepareDownloadDialog)
        {
            QString srcPathWithFileProtocol = "file:///" + scratchPad.GetValue("Source").toString();
            if(entitlement->localContent()->state() == Origin::Engine::Content::LocalContent::ReadyToInstall)
            {
                entitlement->localContent()->install();

                //when installing we need to automatically advance to the disk install progress state
                scratchPad.SetValue("Command", "Next");
            }
            else
            {
                // check for ITO DLC - make sure we have language and install directory first, otherwise fail with appropriate message (OFM- )
                if (entitlement->contentConfiguration()->isPDLC())
                {
                    Origin::Engine::Content::EntitlementRef parent = entitlement->parent();
                    bool parent_locale_found = false;
                    bool parent_install_dir_found = false;

                    if (parent && parent->localContent()->installed())
                    {
                        QString installFolder = PlatformService::localDirectoryFromOSPath(parent->contentConfiguration()->installCheck());

                        if (!installFolder.isEmpty())
                            parent_install_dir_found = true;
                    }

                    if (parent && !parent->localContent()->installedLocale().isEmpty())
                        parent_locale_found = true;

                    if (!parent_install_dir_found || !parent_locale_found)
                    {
                        if (!parent_install_dir_found)
                        {
                            ORIGIN_LOG_ERROR << "Install Through Ebisu: Starting to install DLC failed " << content.toUtf8().constData() << " parent install directory not found";
                        }
                        else
                        {
                            ORIGIN_LOG_ERROR << "Install Through Ebisu: Starting to install DLC failed " << content.toUtf8().constData() << " parent locale not found";
                        }
                        InstallFlowManager::GetInstance().SetVariable("CompleteDialogType", ITOFlow::DLCSysReqFailed);
                        scratchPad.SetValue("Command", "Finish");
                        return;
                    }
                    else
                        scratchPad.SetValue("Command", "Next"); // move it along as we won't be showing a dialog
                }
                else
                {
                    ORIGIN_LOG_EVENT << "ITO: entitlement not PDLC";
                }

                //if we are not using a local source, clear out the src path to let the download flow know it should grab it from
                //CDN
                if(!UsingLocalSource(scratchPad))
                    srcPathWithFileProtocol.clear();

                int ito_flags = Origin::Engine::Content::LocalContent::kDownloadFlag_isITO;
                if (scratchPad.GetValue("F2PEulasAccepted").toBool())
                    ito_flags |= Origin::Engine::Content::LocalContent::kDownloadFlag_eulas_accepted;
                if (scratchPad.GetValue("F2PDefaultInstall").toBool())
                    ito_flags |= Origin::Engine::Content::LocalContent::kDownloadFlag_default_install;

                bool success = entitlement->localContent()->download( scratchPad.GetValue("Locale").toString(), ito_flags, srcPathWithFileProtocol);
                if(success)
                {
                    ORIGIN_LOG_EVENT << "Install Through Ebisu: Starting to install " << content.toUtf8().constData();
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Install Through Ebisu: Starting to install failed " << content.toUtf8().constData();

                    //for now, just check for sysreq failure, probably shoulad add others
                    if (entitlement->localContent()->downloadableStatus() == Origin::Engine::Content::LocalContent::DS_WrongArchitecture)
                    {
                        InstallFlowManager::GetInstance().SetVariable("CompleteDialogType", ITOFlow::SysReqFailed);
                        scratchPad.SetValue("Command", "Finish");
                    }
                    else
                    {
                        scratchPad.SetValue("CompleteDialogType", ITOFlow::Success);
                        scratchPad.SetValue("Command", "Cancel");
                    }
                }
            }

        }
    }
    else
    {
        ORIGIN_LOG_EVENT << "Install Through Ebisu: Unable to find entitlement to start install for " << content.toUtf8().constData();
    }

}

void StartPatch(ScratchPad &scratchPad)
{
    QString content = scratchPad.GetValue("ContentIdToInstall").toString();
    scratchPad.SetValue("Command", "");
    scratchPad.SetValue("ProgressDialogType", 1);
    Origin::Engine::Content::EntitlementRef entitlement = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(content);

    if(entitlement)
    {
        //		QString srcPathWithFileProtocol = "file:///" + scratchPad.GetValue("Source").toString();
        if (entitlement->localContent()->updatable() && entitlement->localContent()->update(true /*suppress*/)) //prevent showing any error dialogs during update phase of ITO
        {
            ORIGIN_LOG_EVENT << "Install Through Ebisu: Starting patch for " << content.toUtf8().constData();
        }
        else    //unable to start the update, just proceed to complete
        {
            ORIGIN_LOG_EVENT << "Install Through Ebisu: Unable to start patch for " << content.toUtf8().constData() << " consider ITO install completed";
            scratchPad.SetValue("Command", "Installed");
        }
    }
    else
    {
        ORIGIN_LOG_EVENT << "Install Through Ebisu: Unable to find entitlment to starting patch for " << content.toUtf8().constData();
    }
}


void LimitedUser(ScratchPad &/*scratchpad*/)
{
    OriginApplication& app = OriginApplication::instance();

    app.showLimitedUserWarning(true);

    ORIGIN_LOG_EVENT << "Install Through Ebisu: can't install due to being a limited user in xp";
}


void SetLaunchScratch(ScratchPad &scratchpad)
{
    scratchpad.SetValue("PlayGame", true);
}

bool EntitlementsRefreshed(ScratchPad &/*scratchpad*/)
{
    bool refreshed = InstallFlowManager::GetInstance().GetVariable("EntitlementsRefreshFinished").toBool();
    return refreshed;
}

bool EntitlementMatches(ScratchPad &/*scratchpad*/)
{
    bool found = false;

    if( !InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString().isEmpty() )
        found = true;

#if 1 //CODE_REDEMPTION_TEMP
    if( InstallFlowManager::GetInstance().GetVariable("BypassWrongCode").toBool() )
        found = true;
#endif

    return found;
}

bool RefreshFailed (ScratchPad &/*scratchpad*/)
{
    bool refreshFailed = !InstallFlowManager::GetInstance().GetVariable("RefreshSuccess").toBool();
    ORIGIN_LOG_EVENT << "ITO: refreshFailed = " << refreshFailed;
    return refreshFailed;
}

bool CodeRedeemed(ScratchPad &/*scratchpad*/)
{
    bool redeemed = InstallFlowManager::GetInstance().GetVariable("CodeRedeemed").toBool();

    ORIGIN_LOG_EVENT << "Install Through Ebisu: Redeem code " << (redeemed ? "succeeded" : "failed");

    return redeemed;
}

bool GameInstalled(ScratchPad &/*scratchpad*/)
{
    bool installed = false;

    QString contentIdToInstall = InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();

    Origin::Engine::Content::EntitlementRef entitlement = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);
    if(entitlement)
    {
        //check the installed state but make sure we pass in true to force the refresh of the state
        installed = entitlement->localContent()->installed(true);
    }    
    return installed;
}

// Check if entitlement is ready for copying / installing from media
bool GameReadyToDownload(ScratchPad &/*scratchpad*/)
{
    bool readyToDownload = false;

    QString contentIdToInstall = InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
    Origin::Engine::Content::EntitlementRef entitlement = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);

    //allow unreleased but games that are 1102/1103 to download as well
    if (entitlement)
    {
        if (!entitlement->localContent()->installed(true))
        {
            bool isReadyToDownload = entitlement->localContent()->state() == Origin::Engine::Content::LocalContent::ReadyToDownload;
            bool isReadyToInstall = entitlement->localContent()->state() == Origin::Engine::Content::LocalContent::ReadyToInstall;
            bool isUnreleased = entitlement->localContent()->state() == Origin::Engine::Content::LocalContent::Unreleased;
            Origin::Services::Publishing::OriginPermissions perms = entitlement->contentConfiguration()->originPermissions();
            bool is1102or1103 = (perms == Origin::Services::Publishing::OriginPermissions1102 || perms == Origin::Services::Publishing::OriginPermissions1103);
            readyToDownload = (isReadyToDownload || isReadyToInstall || (isUnreleased && is1102or1103));
        }
    }

    return readyToDownload;
}

bool GameNotReadyToDownload (ScratchPad &scratchpad)
{
    return !GameReadyToDownload(scratchpad);
}

bool ProcessCommandCheckOK(ScratchPad &scratchpad)
{
    return scratchpad.GetValue("ProcessCommandLineValid").toBool();
}

bool ProcessCommandCheckNotOK(ScratchPad &scratchpad)
{
    return !ProcessCommandCheckOK(scratchpad);
}

void RefreshEntitlementsIfNeeded()
{
    if( InstallFlowManager::GetInstance().GetVariable("RefreshEntitlements").toBool() )
    {
        InstallFlowManager::GetInstance().SetVariable("RefreshEntitlements", false);
        Origin::Engine::Content::ContentController::currentUserContentController()->refreshUserGameLibrary(Origin::Engine::Content::ContentUpdates);
    }
}

void ExitTasks(ScratchPad &scratchPad)
{
    CleanUp(scratchPad);

    if(UsingLocalSource(scratchPad))
    {
        // possibly refresh entitlements if cancelled from code redeem
        RefreshEntitlementsIfNeeded();
    }

    //kick off game if chosen
    if(GameInstalled(scratchPad) && scratchPad.GetValue("PlayGame").toBool())
        LaunchGame(scratchPad);
}

bool IsOnlineMode(ScratchPad &/*scratchpad*/)
{
    return Origin::Services::Connection::ConnectionStatesService::isUserOnline (Origin::Engine::LoginController::currentUser()->getSession());
}

bool IsOfflineMode(ScratchPad &/*scratchpad*/)
{
    return ! Origin::Services::Connection::ConnectionStatesService::isUserOnline (Origin::Engine::LoginController::currentUser()->getSession());
}

//MY: added so we can send telemetry info for cancel/success/failure
void OnCancel (ScratchPad &scratchpad)
{
    if(!scratchpad.GetValue("Failed").toBool())
    {
        const TYPE_S8 reasonStr = "User cancelled ITE installation";
        GetTelemetryInterface()->Metric_ITE_CLIENT_CANCELLED(reasonStr);

        ORIGIN_LOG_EVENT << "ITE was cancelled";
    }
}

void OnInstallSuccess (ScratchPad &/*scratchpad*/)
{
    GetTelemetryInterface()->Metric_ITE_CLIENT_SUCCESS ();

    ORIGIN_LOG_EVENT << "ITE succesfully installed";
}

void OnFreeToPlayNoEnt (ScratchPad &scratchpad)
{
    ITOFlow::instance()->requestFreeGameEntitlement();
}

void OnShowNewEntitlement (ScratchPad &scratchpad)
{
    // Force a scroll to the entitlement
    QString contentIdToInstall = InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
    Origin::Engine::Content::EntitlementRef entRef = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);

    //if its free to play and we haven't requested to show the game details, lets show it.
    if(entRef && scratchpad.GetValue("FreeToPlay").toBool() && !scratchpad.GetValue("GameDetailsShown").toBool() )
    {
        ClientFlow::instance()->showGameDetails(entRef);
    }
    else
    {
        Origin::Client::ClientFlow::instance()->showMyGames();

        if(entRef)
            entRef->ensureVisible();
    }
    ORIGIN_LOG_EVENT << "ITE Showing new entitlement";

}


bool Always(ScratchPad &/*scratchpad*/)	    {	return true;  }
bool Next(ScratchPad &scratchpad)		    {	return scratchpad.GetValue("Command").toString() == "Next";  }
bool Cancel(ScratchPad &scratchpad)		    {	return scratchpad.GetValue("Command").toString() == "Cancel";  }
bool Retry(ScratchPad &scratchpad)		    {	return scratchpad.GetValue("Command").toString() == "Retry";  }
bool Finish(ScratchPad &scratchpad)		    {	return scratchpad.GetValue("Command").toString() == "Finish";  }
bool Installed(ScratchPad &scratchpad)	    {	return scratchpad.GetValue("Command").toString() == "Installed";  }
bool Refreshed(ScratchPad &scratchpad)	    {	return scratchpad.GetValue("Command").toString() == "Refreshed";  }
bool ITE_Download(ScratchPad &scratchpad)	{	return scratchpad.GetValue("Command").toString() == "ITE_Download";  }
bool GoToPrepare(ScratchPad &scratchpad)	{	return scratchpad.GetValue("Command").toString() == "GoToPrepare";  }
bool ReadyToPatch (ScratchPad &scratchpad)  {   return scratchpad.GetValue("Command").toString() == "ReadyToPatch"; }
bool ReadyToDownloadFromServer (ScratchPad &scratchpad) {   return scratchpad.GetValue("Command").toString() == "ReadyToDownloadFromServer"; }

bool IsXPLimitedUser(ScratchPad &/*scratchpad*/)	
{	
    OriginApplication& app = OriginApplication::instance();
    return app.IsLimitedUser();  
}

bool InstallFlowBuilderVersion1(SimpleStateMachine * pMachine, SimpleState *&pInitialState)
{
    SimpleState *pInitial				= pMachine->AddState("Initial", Init, NULL, NULL);
    SimpleState *pInitial_Process		= pMachine->AddState("Initial_Process", Init_ProcessCommandLine, NULL, NULL);
    SimpleState *pInitial_Post			= pMachine->AddState("Initial_Post", Init_PostProcessCommandLine, NULL, NULL);
    SimpleState *pInitial_DiscCheck		= pMachine->AddState("ITEInitial_DiscCheck", NULL, NULL, NULL);
    SimpleState *pRedeem				= pMachine->AddState("ITERedemptionCode", NULL, NULL, NULL);
    SimpleState *pRefreshEntitlements	= pMachine->AddState("RefreshEntitlements", RefreshEntitlements, NULL, NULL);
    SimpleState *pRedeemWaitForRefreshEntitlements = pMachine->AddState("RedeemWaitForRefreshEntitlements", NULL, NULL, NULL);
    SimpleState *pWaitForRefreshEntitlements = pMachine->AddState("WaitForRefreshEntitlements", NULL, NULL, NULL);
    SimpleState *pCheckRefreshEntitlementsDone = pMachine->AddState("CheckRefreshEntitlementsDone", NULL, NULL, NULL);
    SimpleState *pCheckEntitlements	    = pMachine->AddState("CheckEntitlements", CheckEntitlements, NULL, NULL);
    SimpleState *pCheckReadyToDownload  = pMachine->AddState("CheckReadyToDownload", NULL, NULL, NULL);
    SimpleState *pNotReadyToDownload    = pMachine->AddState("NotReadyToDownload", NotReadyToDownloadError, NULL, NULL);
    SimpleState *pPrepareDownload		= pMachine->AddState("PrepareDownload", NULL, NULL, NULL);
    SimpleState *pPrepareLaunch	        = pMachine->AddState("PrepareLaunch", SetLaunchScratch, NULL, NULL);
    SimpleState *pWrongCode             = pMachine->AddState("ITEWrongCode", ResetCodeRedeemed, NULL, NULL);
    SimpleState *pCheckOnline           = pMachine->AddState("CheckOnline", NULL, NULL, NULL);
    SimpleState *pLimitedUser			= pMachine->AddState("ITELimitedUser", LimitedUser, NULL, NULL);
    SimpleState *pPrepare				= pMachine->AddState("LanguageAndITEPrepare", StartInstall, NULL, NULL);
    SimpleState *pPrepareCheckDiskSpace = pMachine->AddState("ITEPrepareDiskSpace", CheckDiskSpace, NULL, NULL);
    SimpleState *pProgress				= pMachine->AddState("ITEInstallProgress", NULL, NULL, NULL);
    SimpleState *pPatchCheckDiskSpace = pMachine->AddState("ITEPatchDiskSpace", CheckDiskSpace, NULL, NULL);    
    SimpleState *pCancel                = pMachine->AddState("CancelPlaceHolder", OnCancel, NULL, NULL);
    SimpleState *pExit					= pMachine->AddState("Exit", ExitTasks, NULL, NULL);
    SimpleState *pAutoPatchWaitToPrepare = pMachine->AddState("WaitToAutoPatchPrepare", NULL, NULL, NULL);    //added to wait for the content install flow to reset
    SimpleState *pAutoPatchPrepare  =     pMachine->AddState("AutoPatchPrepare", StartPatch, NULL, NULL);
    SimpleState *pInstallSuccess		= pMachine->AddState("InstallSuccess", OnInstallSuccess, NULL, NULL);
    SimpleState *pITE_Download  		= pMachine->AddState("ITE_Download", NULL, NULL, NULL);
    SimpleState *pITE_DownloadWaitForCancelCompletion = pMachine->AddState("ITE_DownloadWaitForCancel", NULL, NULL, NULL);
    SimpleState *pITE_DownloadStart     = pMachine->AddState("ITE_DownloadStart", StartDownload, NULL, NULL);
    SimpleState *pDone					= pMachine->AddState ("ITEComplete", NULL, NULL, NULL);
    SimpleState *pFreeToPlayNoEnt		= pMachine->AddState ("FTPNoEnt", OnFreeToPlayNoEnt, NULL, NULL);
    SimpleState *pShowNewEntitlement    = pMachine->AddState ("ShowNewEntitlement", OnShowNewEntitlement, NULL, NULL);

    pInitial->AddTransition("InitialToProcessCommand", pInitial_Process, ::Always);
    pInitial_Process->AddTransition("ProcessCommandToInitialPost", pInitial_Post, ProcessCommandCheckOK);
    pInitial_Process->AddTransition("ProcessCommandToInsertDisk", pInitial_DiscCheck, ProcessCommandCheckNotOK);
    pInitial_Post->AddTransition("InitialToCheckEntitlements", pCheckEntitlements, EntitlementsRefreshed);
    pInitial_DiscCheck->AddTransition("InitialDiscCheck", pInitial_Process, Retry);
    pInitial_DiscCheck->AddTransition("InitialDiscCheck", pCancel, Cancel);

    pRedeem->AddTransition("RedeemToCheckEntitlements", pRedeemWaitForRefreshEntitlements, Next);
    pRedeem->AddTransition("RedeemToCancel", pCancel, Cancel);

    pRedeemWaitForRefreshEntitlements->AddTransition("RedeemRefreshEntitlementsToCheckEntitlements", pCheckEntitlements, EntitlementsRefreshed);
    pRedeemWaitForRefreshEntitlements->AddTransition("RedeemRefreshEntitlementsToWaitForRefreshEntitlements", pWaitForRefreshEntitlements, ::Always);

    // refresh entitlements
    pRefreshEntitlements->AddTransition("RefreshEntitlementsToCheckEntitlements", pCheckEntitlements, EntitlementsRefreshed);
    //if EntitlementsRefreshed fails, have it go to another state so it can loop back to check EntitlementsRefreshed again
    pRefreshEntitlements->AddTransition("RefreshEntitlementsToWaitForRefreshEntitlements", pWaitForRefreshEntitlements, ::Always);

    //we would like to loop back to pRefreshEntitlements but that has an associated onEnter function that initiates a refresh which we don't want to do
    //we just want to check against EntitlementsRefreshed so create another state, it will keep looping back and forth between these 2 states until
    //EntitlementsRefreshed eventually returns true
    pWaitForRefreshEntitlements->AddTransition("WaitForRefreshEntitlementsToCheckRefreshEntitlementsDone", pCheckRefreshEntitlementsDone, Refreshed);

    //this will advance down to pCheckEntitlements if EntitlementsRefrshed returns true
    pCheckRefreshEntitlementsDone->AddTransition("CheckRefreshEntitlementsDoneToCheckEntitlements", pCheckEntitlements, EntitlementsRefreshed);

    // check entitlements
    pCheckEntitlements->AddTransition("CheckEntitlementsToCancel", pCancel, RefreshFailed); //the refresh() call returned updateFailed() signal
    pCheckEntitlements->AddTransition("CheckEntitlementsToCheckReadyToDownload", pCheckReadyToDownload, EntitlementMatches);
    pCheckEntitlements->AddTransition("CheckEntitlementsToWrongCode", pWrongCode, CodeRedeemed);
    pCheckEntitlements->AddTransition("CheckEntitlementsToCheckOnline", pCheckOnline, ::Always);

    // wrong code
    pWrongCode->AddTransition("WrongCodeToCheckOnline", pCheckOnline, Next);

    // check ready to download (for ITO, it's actually copying from media)
    pCheckReadyToDownload->AddTransition("CheckReadyToDownloadToPrepareLaunch", pPrepareLaunch, GameInstalled); //we check if the game is installed first.
    pCheckReadyToDownload->AddTransition("CheckReadyToDownloadToNotReadyToDownload", pNotReadyToDownload, GameNotReadyToDownload); //first check for error cases that would prevent the start of download
    pCheckReadyToDownload->AddTransition("CheckReadyToDownloadToPrepareDownload", pPrepareDownload, GameReadyToDownload);
    pCheckReadyToDownload->AddTransition("CheckReadyToDownloadToExit", pExit, ::Always);

    //not able to start the download
    pNotReadyToDownload->AddTransition ("NotReadyToDownloadToExit", pExit, ::Always);

    // prepare download
    pPrepareDownload->AddTransition("PrepareDownloadToCantInstall", pLimitedUser, IsXPLimitedUser);
    pPrepareDownload->AddTransition("PrepareDownloadToShowContent", pShowNewEntitlement, ::Always);

    // prepare launch
    pPrepareLaunch->AddTransition("PrepareLaunchToFinish", pExit, ::Always);

    // Show the newly added entitlement in My Games
    pShowNewEntitlement->AddTransition("ShowNewEntitlementToPrepare", pPrepare, ::Always);

    // limited user error
    pLimitedUser->AddTransition("CannotInstall", pDone, ::Always);

    // check online
    pCheckOnline->AddTransition("CheckOnlineToRedeem", pRedeem, NotFreeToPlay);
    pCheckOnline->AddTransition("CheckOnlineToNoEntError", pFreeToPlayNoEnt, FreeToPlay);

    // free to play entitlement flow
    pFreeToPlayNoEnt->AddTransition("FTPNoEntToSelfEntitleFailed", pDone, Finish);
    pFreeToPlayNoEnt->AddTransition("FTPNoEntToSelfEntitleSuccess", pRefreshEntitlements, Next);

    //prepare dialog
    pPrepare->AddTransition("PrepareToError", pDone, Finish);
    pPrepare->AddTransition("PrepareToProgress", pProgress, Next);
    pPrepare->AddTransition("PrepareToCheckDisk", pPrepareCheckDiskSpace, Retry);
    pPrepareCheckDiskSpace->AddTransition("CheckDiskToPrepare", pPrepare, ::Always);
    pPrepare->AddTransition("PrepareToCancel", pCancel, Cancel);
    pPrepare->AddTransition("PrepareToITE_Download", pITE_Download, ITE_Download);
    pPrepare->AddTransition("PrepareToReadyToDownloadFromServer", pITE_DownloadStart, ReadyToDownloadFromServer);

    //progress screen

    pProgress->AddTransition("ProgressToExit", pExit, NotUsingLocalSource);
    pProgress->AddTransition("ProgressToCancel", pCancel, Cancel);
    pProgress->AddTransition("ProgressToInstallSuccess", pInstallSuccess, Installed);
    pProgress->AddTransition("ProgressToITE_Download", pITE_Download, ITE_Download);
    pProgress->AddTransition("ProgressToPrepare", pPrepare, GoToPrepare);

    // Download(from server instead) from Disc read error box
    pITE_Download->AddTransition ("ITE_DownloadProceedToDownload", pITE_DownloadStart, ReadyToDownloadFromServer);
    pITE_Download->AddTransition ("ITE_DownloadToWaitForCancel", pITE_DownloadWaitForCancelCompletion, ::Always);

    //wait for cancel to complete before initiating download from server
    pITE_DownloadWaitForCancelCompletion->AddTransition ("ITE_DownloadWaitForCancelToComplete", pITE_DownloadStart, ReadyToDownloadFromServer);

    pITE_DownloadStart->AddTransition("ITE_DownloadFromServerToExit ", pExit, ::Always);

    //cancel
    pCancel->AddTransition ("CancelToDone", pDone, ::Always);

    //onSuccesful install
    pInstallSuccess->AddTransition("SuccessInstallToPatch", pAutoPatchWaitToPrepare, IsOnlineMode);
    pInstallSuccess->AddTransition("SuccessInstallToDone", pDone, IsOfflineMode);

    // auto patching
    pAutoPatchWaitToPrepare->AddTransition ("WaitToStartPatchPrepare", pAutoPatchPrepare, ReadyToPatch);

    pAutoPatchPrepare->AddTransition("PatchStartedNowExit", pExit, ::Always);   // this state kicks off update - but now we need to exit ITO to allow user to logout.

    //added final state to handle display of failure/success UI
    pDone->AddTransition ("Done", pExit, Finish);

    // Optionally set the initial state on the first state. 
    pInitialState = pInitial;

    return true;
}



namespace Origin
{
    namespace Client
    {
        static ITOFlow* sITOFlowInstance = NULL;

        void ITOFlow::create()
        {
            ORIGIN_ASSERT(!sITOFlowInstance);
            if (!sITOFlowInstance)
            {
                sITOFlowInstance = new ITOFlow();
            }
        }

        void ITOFlow::destroy()
        {
            ORIGIN_ASSERT(sITOFlowInstance);
            if (sITOFlowInstance)
            {
                sITOFlowInstance->deleteLater();
                sITOFlowInstance = NULL;
            }
        }

        ITOFlow* ITOFlow::instance()
        {
            return sITOFlowInstance;
        }

        ITOFlow::ITOFlow()
            :mFreeGameEntitlementReply(NULL)
            ,mFreeGameRequestTimer(NULL)
        {
            mFreeGameRequestTimer = new QTimer(this);
        }

        void ITOFlow::initiateFreeGamesRequest(const QUrl &url, QNetworkReply **reply)
        {
            QNetworkRequest req(url);
            req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

            *reply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->post(req, "");

            ORIGIN_VERIFY_CONNECT_EX(*reply, SIGNAL(finished()), this, SLOT(freeGameEntitlementResponseReceived()), Qt::QueuedConnection);
        }

        void ITOFlow::setFreeEntitlementFail()
        {
            InstallFlowManager::GetInstance().SetVariable("CompleteDialogType", ITOFlow::FreeToPlayNoEntitlement);

            const TYPE_S8 reasonStr = "No Free To Play Entitlement";
            GetTelemetryInterface()->Metric_ITE_CLIENT_FAILED(reasonStr);

            ORIGIN_LOG_EVENT << "ITE failed install, no Ent";
            InstallFlowManager::GetInstance().SetStateCommand("Finish");

            mFreeGameRequestTimer->stop();

        }

        void ITOFlow::requestFreeGameEntitlement()
        {
            StoreUrlBuilder urlBuilder;
            QUrl entitleFreeGameUrl = urlBuilder.entitleFreeGamesUrl(InstallFlowManager::GetInstance().GetVariable("RedeemOfferId").toString());
            const int freeGameRequestTimeOutMSecs = 30000;

            //we user a member variable for the reply here so we can abort it, if the total time is longer than 30 secs then we can abort
            ORIGIN_ASSERT(!mFreeGameEntitlementReply);
            initiateFreeGamesRequest(entitleFreeGameUrl, &mFreeGameEntitlementReply);

            mFreeGameRequestTimer->setSingleShot(true);
            mFreeGameRequestTimer->start(freeGameRequestTimeOutMSecs);

            ORIGIN_VERIFY_CONNECT_EX(mFreeGameRequestTimer, SIGNAL(timeout()), this, SLOT(onFreeGameRequestTimeout()), Qt::QueuedConnection);
        }

        void ITOFlow::onFreeGameRequestTimeout()
        {
            mFreeGameRequestTimer->stop();

            if(mFreeGameEntitlementReply)
            {
                mFreeGameEntitlementReply->abort();
                mFreeGameEntitlementReply->deleteLater();
                mFreeGameEntitlementReply = NULL;
            }
            setFreeEntitlementFail();
        }

        void ITOFlow::freeGameEntitlementResponseReceived()
        {
            if(mFreeGameEntitlementReply)
            {
                int httpResponse = mFreeGameEntitlementReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                //if its a redirect, lets grab the location and make the request with the redirect uri
                //happens with the case where we are missing the cookie, we will be redirected to EADP
                if(httpResponse == Http302Found)
                {
                    //grab the new url to go to
                    QString locationHeader = mFreeGameEntitlementReply->header(QNetworkRequest::LocationHeader).toString();

                    //clear out the reply for the new request
                    mFreeGameEntitlementReply->deleteLater();
                    mFreeGameEntitlementReply = NULL;

                    //initial new request
                    initiateFreeGamesRequest(locationHeader, &mFreeGameEntitlementReply);
                }
                else
                    if(httpResponse == Http200Ok)
                    {
                        bool success = false;

                        QJsonParseError jsonResult;
                        QJsonDocument jsonDoc = QJsonDocument::fromJson(mFreeGameEntitlementReply->readAll(), &jsonResult);

                        //check if its a valid JSON response
                        if(jsonResult.error == QJsonParseError::NoError)
                        {
                            QJsonValue statusJsonValue = jsonDoc.object().value("status");

                            //check if it has a "status" key
                            if(!statusJsonValue.isUndefined())
                            {
                                bool status = statusJsonValue.toBool();
                                if(status)
                                {
                                    //status is valid, lets see if the offerId matches just in case
                                    QJsonValue productIdJsonValue = jsonDoc.object().value("productId");

                                    if(!productIdJsonValue.isUndefined())
                                    {
                                        QString productId = productIdJsonValue.toString();
                                        if(productId.compare(InstallFlowManager::GetInstance().GetVariable("RedeemOfferId").toString(), Qt::CaseInsensitive) == 0)
                                        {
                                            //offerId matches
                                            InstallFlowManager::GetInstance().SetStateCommand("Next");
                                            mFreeGameRequestTimer->stop();

                                            success = true;
                                        }

                                    }
                                }
                            }
                        }

                        //we didn't succeed so lets set up the fail dialog and exit the ITO
                        if(!success)
                        {
                            setFreeEntitlementFail();
                        }
                        mFreeGameEntitlementReply->deleteLater();
                        mFreeGameEntitlementReply = NULL;
                    }
                    else
                    {
                        setFreeEntitlementFail();
                        mFreeGameEntitlementReply->deleteLater();
                        mFreeGameEntitlementReply = NULL;
                    }

            }
        }

        bool ITOFlow::isReadDiscErrorType (qint32 errorType)
        {
            //treat all these as ITO disk read errors
            Origin::Downloader::ContentDownloadError::type downloadErrorType = static_cast <Origin::Downloader::ContentDownloadError::type> (errorType);

            if (downloadErrorType == (Origin::Downloader::ContentDownloadError::type)Origin::Downloader::DownloadError::ClusterRead ||
                downloadErrorType == (Origin::Downloader::ContentDownloadError::type)Origin::Downloader::DownloadError::ContentUnavailable ||
                downloadErrorType == (Origin::Downloader::ContentDownloadError::type)Origin::Downloader::DownloadError::FileRead ||
                downloadErrorType == (Origin::Downloader::ContentDownloadError::type)Origin::Downloader::DownloadError::FileOpen ||
                downloadErrorType == (Origin::Downloader::ContentDownloadError::type)Origin::Downloader::DownloadError::FileSeek ||
                downloadErrorType == (Origin::Downloader::ContentDownloadError::type)Origin::Downloader::DownloadError::FilePermission ||
                downloadErrorType == (Origin::Downloader::ContentDownloadError::type)Origin::Downloader::UnpackError::CRCFailed ||
                downloadErrorType == (Origin::Downloader::ContentDownloadError::type)Origin::Downloader::UnpackError::StreamUnpackFailed)
            {
                return true;
            }
            return false;
        }

        QString ITOFlow::getGameTitleFromManifest()
        {
            QString title = "";

            QString CommandLine = Origin::Services::readSetting(Origin::Services::SETTING_COMMANDLINE, Origin::Services::Session::SessionRef());
            QStringList argv = Origin::Client::CommandLine::parseCommandLineWithQuotes(CommandLine);
            int argc = argv.size();

            QDomDocument metadatadoc;

            QString CONTENTIDS = "/contentIds:";
            QString LANGUAGE = "/language:";
            QString CONTENTLOCATION = "/contentLocation:";
            QString DOWNLOAD_REQUIRED = "/downloadRequired";

            QString bannerPath = "";

            for(int i=0; i<argc; i++)
            {
                QString Token = argv[i];

                if(Token.startsWith(CONTENTIDS))
                {
                    QString path = Token.mid(CONTENTIDS.length());

                    // Remove outer quotes
                    path.replace(QRegExp("^\"|\"$"), ""); // remove leading and trailing quotes

                    // Parse out the path for the banner image
                    bannerPath = path.left(path.lastIndexOf(QString("\\")));

                    QFile file(path);
                    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
                    {
                        if(metadatadoc.setContent(&file))
                        {
                            QString locale = QLocale().name();
                            if (locale == "nb_NO")
                                locale = "no_NO";
                            title = ReadGameTitle(locale, metadatadoc);
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                if(Token.startsWith(LANGUAGE))
                {
                    QString locale = Token.mid(LANGUAGE.length());
                    title = ReadGameTitle(locale, metadatadoc);
                    break;
                }
            }

            return title;
        }

        void ITOFlow::onMyGamesLoaded()
        {
            //not running ITO so we don't care
            if (!OriginApplication::instance().isITE())
                return;

            //we have to listen for the mygames to be loaded otherwise generating the url for the games detail page will fail
            //as it will have a blank base url

            //so when we launch ITO from the login screen, we need to make sure the my games is loaded so we have teh logic in this slot
            if(InstallFlowManager::GetInstance().GetVariable("FreeToPlay").toBool())
            {
                InstallFlowManager::GetInstance().SetVariable("GameDetailsShown", true);

                QString contentIdToInstall = InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
                Origin::Engine::Content::EntitlementRef entRef = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(contentIdToInstall);
                if (entRef)
                    ClientFlow::instance()->showGameDetails(entRef);
            }
        }

        void ITOFlow::start()
        {
            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();

            //this one will stay connected and will listen in case refresh is called after code redemption
            ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), this, SLOT(onCurrentUserEntitlementsUpdated(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)));
            ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(updateFailed()), this, SLOT(onCurrentUserEntitlementsUpdateFailed()));

            //to listen for launch via command line
            ORIGIN_VERIFY_CONNECT (Origin::Client::MainFlow::instance(), SIGNAL (startITOviaCommandLine()), this, SLOT (onStartITOviaCommandLine ()));
            ORIGIN_VERIFY_CONNECT (Origin::Client::ClientFlow::instance(), SIGNAL (myGamesLoadFinished()), this, SLOT (onMyGamesLoaded ()));

            if (contentController && contentController->initialFullEntitlementRefreshed())
            {
                startITO();
            }
            else
            {
                ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(initialFullUpdateCompleted()), this, SLOT(onEntitlementsUpdatedForITOStart()));
                ORIGIN_VERIFY_CONNECT(contentController, SIGNAL(updateFailed()), this, SLOT(onEntitlementsUpdatedForITOStart()));
            }

        }

        void ITOFlow::onEntitlementsUpdatedForITOStart()
        {
            Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();
            if (contentController && contentController->initialFullEntitlementRefreshed())
            {
                ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(updateFailed()), this, SLOT(onEntitlementsUpdatedForITOStart()));
                ORIGIN_VERIFY_DISCONNECT(contentController, SIGNAL(initialFullUpdateCompleted()), this, SLOT(onEntitlementsUpdatedForITOStart()));
                startITO();
            }
        }

        void ITOFlow::onCurrentUserEntitlementsUpdated(const QList<Origin::Engine::Content::EntitlementRef> newlyAdded, const QList<Origin::Engine::Content::EntitlementRef> newlyRemoved)
        {
            bool isITEActive = OriginApplication::instance().isITE();
            bool refreshed = InstallFlowManager::GetInstance().GetVariable("EntitlementsRefreshFinished").toBool();

            if(isITEActive && !refreshed)
            {
                //if we were unable to access the server, it could fall back to the offline cache and return updateFinished(), BUT
                //newlyAdded should be empty in that case
                //so use that as a distinction, i.e. if newlyAdded is empty assume failure
                if (newlyAdded.isEmpty())
                {
                    bool isPDLC = InstallFlowManager::GetInstance().GetVariable("ContentIsPDLC").toBool();
                    bool wait_for_extra_content = InstallFlowManager::GetInstance().GetVariable("WaitForPDLCRefresh").toBool();
                    Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();

                    // the assumption here is that the autorun will confirm that the parent base game is installed so there will be at least one
                    // base game entitlement with children which will kick off a second updateFinished() signal which is connected to this slot.
                    if (isPDLC && wait_for_extra_content && contentController && contentController->hasExtraContent())
                    {
                        InstallFlowManager::GetInstance().SetVariable("WaitForPDLCRefresh", false); // turn it off so if it isn't in the extra content refresh it isn't there
                        return;
                    }

                    InstallFlowManager::GetInstance().SetVariable("RefreshSuccess", false);
                    InstallFlowManager::GetInstance().SetVariable("EntitlementsRefreshFinished", true);
                    InstallFlowManager::GetInstance().SetStateCommand("Refreshed");  //to advance the WaitingForEntitlementRefresh state
                    return;
                }   

                //lets find out what content ID we are using;
                ScratchPad pad;
                CheckEntitlements(pad);

                //grab the contentID;
                QString content = InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
                Origin::Engine::Content::EntitlementRef entitlement = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(content);
                if(entitlement && entitlement->localContent()->installFlow())
                {
                    //when we've refreshed the entitlements, the entitlement may not be in a downloadable state yet, the install flow state machine as not transitioned to the proper states
                    //we ::Always connect first and disconnect after to ensure we don't miss the kReadyToStart signal
                    ORIGIN_VERIFY_CONNECT_EX(entitlement->localContent()->installFlow(), SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onEntitlementStateChanged(Origin::Downloader::ContentInstallFlowState)), Qt::UniqueConnection);

                    //lets check if by chance we are ok to start, if not we wil wait for the state changed signal
                    if(entitlement->localContent()->installFlow()->canBegin())
                    {
                        //if we are lets disconnect and go ahead and being the install
                        ORIGIN_VERIFY_DISCONNECT(entitlement->localContent()->installFlow(), SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onEntitlementStateChanged(Origin::Downloader::ContentInstallFlowState)));

                        InstallFlowManager::GetInstance().SetVariable("RefreshSuccess", true);
                        InstallFlowManager::GetInstance().SetVariable("EntitlementsRefreshFinished", true);
                        InstallFlowManager::GetInstance().SetStateCommand("Refreshed");  //to advance the WaitingForEntitlementRefresh state
                    }
                }
            }
        }

        void ITOFlow::onCurrentUserEntitlementsUpdateFailed()
        {
            bool isITEActive = OriginApplication::instance().isITE();
            bool refreshed = InstallFlowManager::GetInstance().GetVariable("EntitlementsRefreshFinished").toBool();

            if(isITEActive && !refreshed)
            {
                InstallFlowManager::GetInstance().SetVariable("EntitlementsRefreshFinished", true);
                InstallFlowManager::GetInstance().SetVariable("RefreshSuccess", false);
                InstallFlowManager::GetInstance().SetStateCommand("Refreshed");  //to advance the WaitingForEntitlementRefresh state
            }
        }

        void ITOFlow::onEntitlementStateChanged(Origin::Downloader::ContentInstallFlowState newState)
        {
            switch(newState)
            {
            case Origin::Downloader::ContentInstallFlowState::kReadyToStart:
                {
                    InstallFlowManager::GetInstance().SetVariable("RefreshSuccess", true);
                    InstallFlowManager::GetInstance().SetVariable("EntitlementsRefreshFinished", true);
                    InstallFlowManager::GetInstance().SetStateCommand("Refreshed");  //to advance the WaitingForEntitlementRefresh state

                    //once we've reached this state we do not want to listen for state changes any more, we really only need for ITO redemption flow
                    QString content = InstallFlowManager::GetInstance().GetVariable("ContentIdToInstall").toString();
                    Origin::Engine::Content::EntitlementRef entitlement = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementByContentId(content);
                    if(entitlement && entitlement->localContent()->installFlow())
                    {
                        ORIGIN_VERIFY_DISCONNECT(entitlement->localContent()->installFlow(), SIGNAL(stateChanged(Origin::Downloader::ContentInstallFlowState)), this, SLOT(onEntitlementStateChanged(Origin::Downloader::ContentInstallFlowState)));
                    }

                }
                break;
            }


        }

        void ITOFlow::onStartITOviaCommandLine()
        {
            //leave signal connected in case there's more than one ITO install during a session
            startITO();
        }

        void ITOFlow::startITO()
        {
            /// TODO: Check whether we should run the installer flow now.
            bool isITEFlag = OriginApplication::instance().isITE();

            if(isITEFlag)
            {
                // Prepare the install manager for a possible install.
                InstallFlowManager &ifm = InstallFlowManager::GetInstance();

                ifm.Register(1, ::InstallFlowBuilderVersion1);
                // TODO: Add more builders if necessary

                int Version = 1;
                // TODO: Check command line arguments what version we need: -v1, -v2 

                if(ifm.Init(Version))
                {
                    ifm.Run();
                    return;
                }
                else
                {
                    InstallFlowManager::DeleteInstance();
                }
            }
        }
    }
}
