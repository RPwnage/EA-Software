/////////////////////////////////////////////////////////////////////////////
// OriginDeveloperProxy.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "OriginDeveloperProxy.h"
#include "DeveloperToolViewController.h"
#include "OriginDeveloperTool/OdtActivation.h"
#include "OriginDeveloperTool/CalculateCrcHelper.h"
#include "utilities/CloudSaveDebugActions.h"
#include "engine/content/CloudContent.h"
#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "engine/content/RetrieveDiPManifestHelper.h"
#include "engine/content/ProductArt.h"

#include "services/platform/PlatformService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/qt/QtUtil.h"
#include "services/publishing/DownloadUrlServiceClient.h"
#include "services/crypto/SimpleEncryption.h"
#include "TelemetryAPIDLL.h"

#include "widgets/odc/source/CodeRedemptionDebugViewController.h"

#include "originwindow.h"
#include "originmsgbox.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QPair>

#include "version/version.h"

#define QStringPair QPair<QString, QString>

using namespace Origin::Services;

namespace Origin
{

namespace Plugins
{

namespace DeveloperTool
{

namespace JsInterface
{

const static QList<QStringPair> HOST_LIST = QList<QStringPair>() <<
QStringPair(OBFUSCATE_STRING("approve-prod"), OBFUSCATE_STRING("https://approve.www.cms.origin.com")) <<
QStringPair(OBFUSCATE_STRING("review-prod"), OBFUSCATE_STRING("https://review.www.cms.origin.com")) <<
QStringPair(OBFUSCATE_STRING("preview-prod"), OBFUSCATE_STRING("https://preview.www.cms.origin.com")) <<
QStringPair(OBFUSCATE_STRING("geoip-prod"), OBFUSCATE_STRING("https://store.dm.origin.com")) <<
QStringPair(OBFUSCATE_STRING("approve-fc-qa"), OBFUSCATE_STRING("https://fc.qa.approve.www.cms.origin.com")) <<
QStringPair(OBFUSCATE_STRING("review-fc-qa"), OBFUSCATE_STRING("https://fc.qa.review.www.cms.origin.com")) <<
QStringPair(OBFUSCATE_STRING("preview-fc-qa"), OBFUSCATE_STRING("https://fc.qa.preview.www.cms.origin.com")) <<
QStringPair(OBFUSCATE_STRING("approve-qa"), OBFUSCATE_STRING("https://qa.approve.www.cms.origin.com")) <<
QStringPair(OBFUSCATE_STRING("review-qa"), OBFUSCATE_STRING("https://qa.review.www.cms.origin.com")) <<
QStringPair(OBFUSCATE_STRING("preview-qa"), OBFUSCATE_STRING("https://qa.preview.www.cms.origin.com"));
    
const static QList<QStringPair> PATH_LIST = QList<QStringPair>() <<
QStringPair(OBFUSCATE_STRING("storeInitialURL"), OBFUSCATE_STRING("/{locale}store/")) <<
QStringPair(OBFUSCATE_STRING("pdlcStoreURL"), OBFUSCATE_STRING("/{locale}store/addonstore/{masterTitleId}")) <<
QStringPair(OBFUSCATE_STRING("storeMasterTitleURL"), OBFUSCATE_STRING("/{locale}store/buy/{masterTitleId}")) <<
QStringPair(OBFUSCATE_STRING("storeProductURL"), OBFUSCATE_STRING("/{locale}store/sku/{offerId}")) <<
QStringPair(OBFUSCATE_STRING("freeGamesURL"), OBFUSCATE_STRING("/{locale}store/shop/free")) <<
QStringPair(OBFUSCATE_STRING("entitleFreeGamesURL"), OBFUSCATE_STRING("/{locale}store/v1/checkout/{userId}/{offerId}?authToken={authToken}")) <<
QStringPair(OBFUSCATE_STRING("onTheHouseURL"), OBFUSCATE_STRING("/{locale}store/free-games/on-the-house?intcmp={trackingParam}")) <<
QStringPair(OBFUSCATE_STRING("checkoutURL"), OBFUSCATE_STRING("/{locale}store/odc/widget/checkout/empty/redirect/{profile}/{offers}?profile={profile}"));

OriginDeveloperProxy::OriginDeveloperProxy()
    : mCrcHelper(NULL)
{
    // Populate store URL map from static host/path fragments.
    for (auto hostIt = HOST_LIST.constBegin(); hostIt != HOST_LIST.constEnd(); ++hostIt)
    {
        const QStringPair& hostInfo = *hostIt;
        for (auto pathIt = PATH_LIST.constBegin(); pathIt != PATH_LIST.constEnd(); ++pathIt)
        {
            const QStringPair& pathInfo = *pathIt;
            mStoreUrls[hostInfo.first][pathInfo.first] = hostInfo.second + pathInfo.second;
        }
    }
}

OriginDeveloperProxy::~OriginDeveloperProxy()
{
    foreach(const QString& tempManifestFile, mManifestFileCleanup)
    {
        QFile::remove(tempManifestFile);
    }
    mManifestFileCleanup.clear();
}

qint64 OriginDeveloperProxy::userId()
{
    return Engine::LoginController::currentUser()->userId();
}

QString OriginDeveloperProxy::odtVersion()
{
    return EALS_VERSION_P_DELIMITED;
}

QString OriginDeveloperProxy::storeEnvironment()
{
    const QString& storeInitialURL = Services::readSetting(Services::SETTING_StoreInitialURL).toString();
    for (auto it = HOST_LIST.constBegin(); it != HOST_LIST.constEnd(); ++it)
    {
        const QString& host = (*it).first;
        const QString& domain = (*it).second;
        if (storeInitialURL.startsWith(domain, Qt::CaseInsensitive))
        {
            return host;
        }
    }

    return QString();
}

QVariant OriginDeveloperProxy::storeUrls(const QString& environment)
{
    return mStoreUrls[environment];
}

QString OriginDeveloperProxy::selectFile(QString startingDirectory, QString filter, QString operation, QString telemetryType)
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED(telemetryType.toUtf8().data());

    QString path, retval;

    if(startingDirectory.compare("desktop", Qt::CaseInsensitive) == 0)
    {
        path = Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DesktopLocation);
    }
    else if(startingDirectory.compare("user_docs", Qt::CaseInsensitive) == 0)
    {
        path = Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DocumentsLocation);
    }
    else if(startingDirectory.compare("appdata", Qt::CaseInsensitive) == 0)
    {
        path = Origin::Services::PlatformService::commonAppDataPath();
    }

    if(operation.compare("save", Qt::CaseInsensitive) == 0)
    {
        retval = QFileDialog::getSaveFileName(NULL, QString(), path, filter);
    }
    else
    {
        retval = QFileDialog::getOpenFileName(NULL, QString(), path, filter);
    }
 
    return retval;
}

QString OriginDeveloperProxy::selectFolder()
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("shared_network_override_folder");

    return QFileDialog::getExistingDirectory();
}

bool OriginDeveloperProxy::importFile()
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("import");

    bool retval = false;
    QString fileName = QFileDialog::getOpenFileName(NULL, QString(), Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DesktopLocation), "*.ini");
    if(!fileName.isEmpty())
    {
        retval = Engine::Config::ConfigFile::modifiedConfigFile().loadConfigFile(fileName);
    }
    return retval;
}

bool OriginDeveloperProxy::exportFile()
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("export");

    bool retval = false;
    QString fileName = QFileDialog::getSaveFileName(NULL, QString(), Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DesktopLocation), "EACore.ini");
    if(!fileName.isEmpty())
    {
        retval = Engine::Config::ConfigFile::modifiedConfigFile().saveConfigFile(fileName);
    }
    return retval;
}

void OriginDeveloperProxy::removeContent(QString id)
{
    Engine::Config::ConfigFile::modifiedConfigFile().removeAllOverridesForId(id);
}

QStringList OriginDeveloperProxy::unownedIds()
{
    QStringList retval;
    
    QStringList allOverriddenIds = Engine::Config::ConfigFile::modifiedConfigFile().getOverriddenIds();
    for(QStringList::const_iterator it = allOverriddenIds.constBegin(); it != allOverriddenIds.constEnd(); ++it)
    {
        if(Engine::Content::ContentController::currentUserContentController()->entitlementById(*it).isNull())
        {
            retval.append(*it);
        }
    }

    return retval;
}

QStringList OriginDeveloperProxy::cloudConfiguration(QString id)
{
    QStringList retval;

    Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementById(id);

    if(!ent.isNull() && ent->localContent()->cloudContent()->hasCloudSaveSupport())
    {
        Client::CloudSaveDebugActions cloudSaveDebug(ent);
        retval = cloudSaveDebug.eligibleSaveFiles();
    }

    return retval;
}

void OriginDeveloperProxy::clearRemoteArea(QString id)
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("clear_remote_cloud");

    Engine::Content::EntitlementRef ent = Engine::Content::ContentController::currentUserContentController()->entitlementById(id);

    if(!ent.isNull())
    {
        Client::CloudSaveDebugActions cloudSaveDebug(ent);
        cloudSaveDebug.clearRemoteArea();
    }
}

QVariantList OriginDeveloperProxy::softwareBuildMap(QString id)
{
    QString checkId = id.trimmed();
    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementById(checkId);

    if(!entitlement.isNull())
    {
        return convertSoftwareBuildMap(entitlement->contentConfiguration()->softwareBuildMap());
    }
    else
    {
        return QVariantList();
    }
}

void OriginDeveloperProxy::getDownloadUrl(const QString& offerId, const QString& buildId, const char* myResultSlot)
{
    QSharedPointer<Engine::Content::Entitlement> entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(offerId);

    if(!entitlement.isNull())
    {
        QString serverBuildId = buildId;

        // If we don't have 1102/1103 permissions or the offer is not an OriginPlugin, we can't do any of this anyway
        if(entitlement->contentConfiguration()->originPermissions() == Origin::Services::Publishing::OriginPermissionsNormal &&
           entitlement->contentConfiguration()->packageType() != Origin::Services::Publishing::PackageTypeOriginPlugin)
        {
            serverBuildId.clear();
        }

        Origin::Services::Publishing::DownloadUrlResponse* resp = Origin::Services::Publishing::DownloadUrlProviderManager::instance()->downloadUrl(Origin::Services::Session::SessionService::currentSession(), offerId, serverBuildId);
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, myResultSlot);
    }
}

void OriginDeveloperProxy::downloadCrc(QString offerId, QString version)
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("download_crc");

    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(offerId);
    
    if(!entitlement.isNull())
    {
        mCrcHelper = new CalculateCrcHelper(entitlement->contentConfiguration());
        ORIGIN_VERIFY_CONNECT(mCrcHelper, SIGNAL(finished()), this, SLOT(onCrcFinished()));
    
        getDownloadUrl(offerId, version, SLOT(onUrlFetchForCrcFinished()));
    }
}

void OriginDeveloperProxy::onUrlFetchForCrcFinished()
{
    QString url;

    Origin::Services::Publishing::DownloadUrlResponse* response = dynamic_cast<Origin::Services::Publishing::DownloadUrlResponse*>(sender());
    if(response != NULL)
    {
        ORIGIN_VERIFY_DISCONNECT(response, SIGNAL(finished()), this, SLOT(completeInitializeProtocolWithJitUrl()));
        response->deleteLater();

        if(response->error() == Origin::Services::restErrorSuccess)
        {
            url = response->downloadUrl().mURL;
        }
    }
    if(!mCrcHelper->calculateCrc(url))
    {
        onCrcFinished();
    }
}

void OriginDeveloperProxy::onCrcFinished()
{
    if(mCrcHelper)
    {
        mCrcHelper->deleteLater();
        mCrcHelper = NULL;
    }
}

void OriginDeveloperProxy::downloadBuildInBrowser(QString offerId, QString version)
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("download_in_browser");

    getDownloadUrl(offerId, version, SLOT(onUrlFetchForDownloadFinished()));
}

void OriginDeveloperProxy::onUrlFetchForDownloadFinished()
{
    Origin::Services::Publishing::DownloadUrlResponse* response = dynamic_cast<Origin::Services::Publishing::DownloadUrlResponse*>(sender());
    if(response != NULL)
    {
        if(response->error() == Origin::Services::restErrorSuccess)
        {
            ORIGIN_LOG_ACTION << "Opening URL in system browser: " << response->downloadUrl().mURL;
            Origin::Services::PlatformService::asyncOpenUrl(response->downloadUrl().mURL);
            response->deleteLater();
            return;
        }
    }

    UIToolkit::OriginWindow::alert(UIToolkit::OriginMsgBox::Error, "DOWNLOAD IN BROWSER", "Failed to retrieve download URL from server.", tr("ebisu_client_notranslate_close"));
}

void OriginDeveloperProxy::downloadBuildInstallerXml(QString offerId, QString version, QString localOverride)
{ 
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("view_installer_data_xml");

    if(version.isEmpty() && !localOverride.isEmpty())
    {
        fetchAndDisplayManifest(offerId, localOverride);
    }
    else
    {
        getDownloadUrl(offerId, version, SLOT(onUrlFetchForManifestFinished()));
    }
}

bool OriginDeveloperProxy::fetchAndDisplayManifest(const QString& offerId, const QString& dipUrl)
{
    QSharedPointer<Engine::Content::Entitlement> entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(offerId);

    if(!entitlement.isNull())
    {
        Origin::Downloader::RetrieveDiPManifestHelper dipHelper(dipUrl, entitlement->contentConfiguration(), true);

        if(dipHelper.retrieveDiPManifestSync())
        {
            mManifestFileCleanup.insert(dipHelper.manifestFilePath());
            Origin::Services::PlatformService::asyncOpenUrlAndSwitchToApp(QString("file:///%1").arg(dipHelper.manifestFilePath()));
            return true;
        }
        else
        {
            UIToolkit::OriginWindow::alert(UIToolkit::OriginMsgBox::Error, "VIEW INSTALLER DATA XML", QString("Failed to extract installer data XML from download URL, check client log for more details.").arg(dipUrl), tr("ebisu_client_notranslate_close"));
        }
    }
    else
    {
        UIToolkit::OriginWindow::alert(UIToolkit::OriginMsgBox::Error, "VIEW INSTALLER DATA XML", "Cannot show installer data XML for unowned content.", tr("ebisu_client_notranslate_close"));
    }

    return false;
}

void OriginDeveloperProxy::onUrlFetchForManifestFinished()
{
    QString jitUrl;
    QString offerId;

    Origin::Services::Publishing::DownloadUrlResponse* response = dynamic_cast<Origin::Services::Publishing::DownloadUrlResponse*>(sender());

    if(response != NULL && response->error() == Origin::Services::restErrorSuccess)
    {
        jitUrl = response->downloadUrl().mURL;
        offerId = response->downloadUrl().mOfferId;
        response->deleteLater();
    }

    if(!jitUrl.isEmpty())
    {
        fetchAndDisplayManifest(offerId, jitUrl);
    }
    else
    {
        UIToolkit::OriginWindow::alert(UIToolkit::OriginMsgBox::Error, "VIEW INSTALLER DATA XML", "Failed to retrieve JIT download URL from server.", tr("ebisu_client_notranslate_close"));
    }
}

void OriginDeveloperProxy::launchCodeRedemptionDebugger()
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("launch_code_redemption_debugger");

    Client::CodeRedemptionDebugViewController* codeRedemptionWindow = Origin::Client::CodeRedemptionDebugViewController::instance();
    codeRedemptionWindow->show();
}

void OriginDeveloperProxy::launchTelemetryLiveViewer()
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("launch_telemetry_live_viewer");

    mTelemetryDebugView.showWindow();
}

void OriginDeveloperProxy::launchTrialResetTool(const QString& contentId)
{
    QString url = OBFUSCATE_STRING("origin://launchgame/OFB-EAST:109552300?Title=Origin Trial Reset Tool&ProductId=OFB-EAST:109552300&AutoDownload=1&CommandParams=-contentId:%1 -userId:%2");
    url = url.arg(contentId).arg(userId());
    PlatformService::asyncOpenUrl(url);
}

void OriginDeveloperProxy::viewODTDocumentation()
{
    QString url = OBFUSCATE_STRING("https://docs.developer.ea.com/display/Origin/Origin+Developer+Tool");
    PlatformService::asyncOpenUrl(url);
}

void OriginDeveloperProxy::openClientLog()
{
    QString clientLogFile(PlatformService::clientLogFilename());
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("view_client_logs");
    
    if (!QFile::exists(clientLogFile))
    {
        UIToolkit::OriginWindow::alert(UIToolkit::OriginMsgBox::Error, "VIEW CLIENT LOG", "Could not load client log.  File does not exist: " + clientLogFile, tr("ebisu_client_notranslate_close"));
    }
    else
    {
        PlatformService::asyncOpenUrl(QUrl("file:///" + clientLogFile));
    }
}

void OriginDeveloperProxy::openBootstrapperLog()
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("view_bootstrapper_log");

    QString bootstrapperLogFile(PlatformService::bootstrapperLogFilename());
    
    if (!QFile::exists(bootstrapperLogFile))
    {
        UIToolkit::OriginWindow::alert(UIToolkit::OriginMsgBox::Error, "VIEW BOOTSTRAPPER LOG", "Could not load bootstrapper log.  File does not exist: " + bootstrapperLogFile, tr("ebisu_client_notranslate_close"));
    }
    else
    {
        PlatformService::asyncOpenUrl(QUrl("file:///" + bootstrapperLogFile));
    }
}

void OriginDeveloperProxy::restartClient()
{
    DeveloperToolViewController::instance()->restart();
}

void OriginDeveloperProxy::deactivate()
{
    OdtActivation::deactivate();
}

QVariantList OriginDeveloperProxy::convertSoftwareBuildMap(const Services::Publishing::SoftwareBuildMap &softwareBuilds)
{
    QVariantList convertedBuildMap;

    foreach(const QString& buildId, softwareBuilds.keys())
    {
        QVariantMap buildData;

        buildData.insert("buildLiveDate", softwareBuilds[buildId].liveDate());
        buildData.insert("buildReleaseVersion", softwareBuilds[buildId].buildReleaseVersion());
        buildData.insert("buildId", buildId);

        switch(softwareBuilds[buildId].type())
        {
        case Origin::Services::Publishing::SoftwareBuildLive:
            buildData.insert("urlType", "Live");
            break;
        case Origin::Services::Publishing::SoftwareBuildStaged:
            buildData.insert("urlType", "Staged");
            break;
        case Origin::Services::Publishing::SoftwareBuildOther:
        default:
            buildData.insert("urlType", "");
            break;
        }

        buildData.insert("fileName", softwareBuilds[buildId].fileLink().split('/').takeLast());
        buildData.insert("buildNotes", softwareBuilds[buildId].notes());

        convertedBuildMap.append(buildData);
    }

    return convertedBuildMap;
}

} // namespace JsInterface

} // namespace DeveloperTool

} // namespace Plugins

} // namespace Origin