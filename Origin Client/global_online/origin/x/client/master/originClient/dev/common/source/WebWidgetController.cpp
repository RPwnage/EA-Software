#include <QtWebKit/QWebSettings>
#include <QDesktopServices>
#include <QCryptographicHash>
#include <QDateTime>

#include "WebWidgetController.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"
#include "services/settings/SettingsManager.h"
#include "services/network/NetworkAccessManager.h"
#include "services/log/LogService.h"

#include "engine/login/LoginController.h"

#include "engine/social/SocialController.h"
#include "chat/OriginConnection.h"

#include "WebWidget/NativeInterface.h"
#include "WebWidget/NativeInterfaceRegistry.h"
#include "WebWidget/WidgetRepository.h"
#include "WebWidget/WidgetPage.h"

#include "AchievementManagerProxy.h"
#include "EntitlementManager.h"
#include "OnlineStatusProxyFactory.h"
#include "MenuProxyFactory.h"
#include "OriginSocialProxy.h"
#include "OriginChatProxy.h"
#include "DateFormatProxy.h"
#include "CapsLockProxy.h"
#include "BroadcastProxy.h"
#include "ClientNavigationProxy.h"
#include "ClientSettingsProxy.h"
#include "ContentOperationQueueControllerProxy.h"
#include "OriginUserProxy.h"
#include "InstallDirectoryManager.h"
#include "OriginPageVisibilityFactory.h"
#include "MenuProxyFactoryFactory.h"
#include "OriginCommonProxy.h"
#include "SubscriptionManagerProxy.h"
#include "TelemetryClientProxy.h"
#include "ProductQueryProxy.h"
#include "VoiceProxy.h"
#include "GeolocationQueryProxy.h"
#include "OnTheHouseQueryProxy.h"
#include "AudioPlayer.h"
#include "jsinterface/debug/Debug.h"

#include "TelemetryAPIDLL.h"


#ifdef _WIN32
#include <Windows.h>
#include <ShlObj.h>
#endif

namespace
{
    Origin::Client::WebWidgetController *WebWidgetControllerInstance = NULL;

    QString roamingDataDirectory()
    {
#ifdef _WIN32
        // Get the path to roaming app data
        wchar_t appDataPath[MAX_PATH + 1];
        if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath) != S_OK)
        {
            ORIGIN_ASSERT(0);
            return QString();
        }
        
        return QDir::fromNativeSeparators(QString::fromUtf16(appDataPath)) + "/Origin";
#else
        return Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DataLocation);
#endif
    }
}

using namespace Origin;
using namespace WebWidget;

namespace Origin
{
namespace Client
{

WebWidgetController::WebWidgetController()
{
    // Connect login/logout events
    ORIGIN_VERIFY_CONNECT(Engine::LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)),
        this, SLOT(userLoggedIn(Origin::Engine::UserRef)));
    
    ORIGIN_VERIFY_CONNECT(Engine::LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)),
        this, SLOT(userLoggedOut(Origin::Engine::UserRef)));

    // All the proxies are under this namespace
    using namespace JsInterface;

    // Register our non-user-specific interfaces
    NativeInterfaceRegistry::registerInterfaceFactory(
            NativeInterfaceRegistry::defaultFeatureUri("onlineStatus"),
            new OnlineStatusProxyFactory);
    
    NativeInterface dateFormat("dateFormat", new DateFormatProxy, true);
    NativeInterfaceRegistry::registerSharedInterface(dateFormat);

    NativeInterface clientNavigation("clientNavigation", new ClientNavigationProxy, true);
    NativeInterfaceRegistry::registerSharedInterface(clientNavigation);

    NativeInterface capsLock("capsLock", new CapsLockProxy, true);
    NativeInterfaceRegistry::registerSharedInterface(capsLock);
    
    NativeInterface telemetryClient("telemetryClient", new TelemetryClientProxy, true);
    NativeInterfaceRegistry::registerSharedInterface(telemetryClient);
    
    NativeInterface geolocationQuery("geolocationQuery", new GeolocationQueryProxy, true);
    NativeInterfaceRegistry::registerSharedInterface(geolocationQuery);

    NativeInterface audioPlayer("audioPlayer", new AudioPlayer, true);
    NativeInterfaceRegistry::registerSharedInterface(audioPlayer);

    NativeInterface consoleProxy("originConsole", new Console, true);
    NativeInterfaceRegistry::registerSharedInterface(consoleProxy);

    NativeInterfaceRegistry::registerInterfaceFactory(
            NativeInterfaceRegistry::defaultFeatureUri("originPageVisibility"),
            new OriginPageVisibilityFactory);

    NativeInterfaceRegistry::registerInterfaceFactory(
        NativeInterfaceRegistry::defaultFeatureUri("nativeMenu"),
        new MenuProxyFactoryFactory);
    
    // Create our widget repository
    QList<QDir> widgetSearchPaths;

    // Then search our resources
    widgetSearchPaths.append(QDir(":/compiledWidgets/"));

    // Load our update public key
    QFile keyFile(":/keys/widget-update-public.pem");
    
    if (!keyFile.open(QIODevice::ReadOnly))
    {
        // Bad
        ORIGIN_ASSERT(0);
    }

    RsaPublicKey updateKey = RsaPublicKey::fromPemEncodedData(&keyFile);

    // Determine our update cache dir
    const QDir updateCacheDir(roamingDataDirectory() + "/Widget Updates");
    QDir("/").mkdir(updateCacheDir.absolutePath());

    mWidgetRepository = new WidgetRepository(
        widgetSearchPaths,
        Services::readSetting(Services::SETTING_WebWidgetUpdateURL).toString(),
        updateKey,
        updateCacheDir,
        Services::NetworkAccessManager::threadDefaultInstance());

    // Capture update events for telemetry
    ORIGIN_VERIFY_CONNECT(mWidgetRepository, SIGNAL(updateStarted(const WebWidget::UpdateIdentifier &)),
        this, SLOT(updateStarted(const WebWidget::UpdateIdentifier &)));

    ORIGIN_VERIFY_CONNECT(mWidgetRepository, SIGNAL(updateFinished(const WebWidget::UpdateIdentifier &, WebWidget::UpdateError)),
        this, SLOT(updateFinished(const WebWidget::UpdateIdentifier &, WebWidget::UpdateError)));

    // Start looking for mygames and chat updates while we're idling at login
    mWidgetRepository->startPeriodicUpdateCheck("mygames");
    mWidgetRepository->startPeriodicUpdateCheck("chat");
    mWidgetRepository->startPeriodicUpdateCheck("achievements");
    mWidgetRepository->startPeriodicUpdateCheck("settings");
    mWidgetRepository->startPeriodicUpdateCheck("oig");
    mWidgetRepository->startPeriodicUpdateCheck("globalprogress");
    mWidgetRepository->startPeriodicUpdateCheck("netpromoter");
    mWidgetRepository->startPeriodicUpdateCheck("imageprocessing");

    // Enable persistent storage
    enablePersistentStorage();
}

WebWidgetController::~WebWidgetController()
{
    delete mWidgetRepository;
}

void WebWidgetController::create()
{
    ORIGIN_ASSERT(WebWidgetControllerInstance == NULL);
    WebWidgetControllerInstance = new WebWidgetController();
}

void WebWidgetController::destroy()
{
    delete WebWidgetControllerInstance;
}

bool WebWidgetController::tracedWidgetLoad(WidgetPage *page, const QString &name, const InstalledWidget &widget, const WebWidget::WidgetLink &link) const
{
    if (page->loadWidget(widget, link))
    {
        const QString version(page->widgetConfiguration().version());
        const QString displayVersion(version.isNull() ? "unspecified" : version);

        ORIGIN_LOG_DEBUG << "Loaded web widget \"" << name << "\" version " << displayVersion.toLatin1().constData();
        GetTelemetryInterface()->Metric_WEBWIDGET_LOADED(name.toLatin1().data(), version.toLatin1().data());

        return true;
    }
    else 
    {
        ORIGIN_LOG_ERROR << "Failed to load web widget \"" << name << "\"";
        return false;
    }
}

bool WebWidgetController::loadUserSpecificWidget(WidgetPage *page, const QString &name, Engine::UserRef user, const WebWidget::WidgetLink &link) const
{
    // Build an authority using the widget name, user ID and machine hash
    // The widget name and user ID are used to correctly sandbox the widget for local storage
    // The machine hash is added to avoid leaking information about the user ID in to the hash in case the web widget
    // is remotely compromised
    QCryptographicHash hasher(QCryptographicHash::Sha1);

    // Salt with arbitrary random prefix to confuse people without disassemblers
    hasher.addData("9a2ca2606e2ae7f07c609754ca58ba07-");
    hasher.addData(QString::number(user->userId()).toLatin1());
    hasher.addData("-");
    hasher.addData(Services::PlatformService::machineHashAsString().toLatin1());
    hasher.addData("-");
    hasher.addData(name.toUtf8());

    // Keep the name as plain text so it's visible for debugging reasons
    const QString authority = name + "-" + hasher.result().toHex();

    return tracedWidgetLoad(page, name, mWidgetRepository->installedWidget(name, authority), link);
}

bool WebWidgetController::loadSharedWidget(WidgetPage *page, const QString &name, const WebWidget::WidgetLink &link) const
{
    return tracedWidgetLoad(page, name, mWidgetRepository->installedWidget(name), link);
}
   
void WebWidgetController::userLoggedIn(Origin::Engine::UserRef user)
{
    using namespace JsInterface;

    // Register our user specific interfaces
    NativeInterface entitlementManager("entitlementManager", EntitlementManager::create(), true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(entitlementManager));
    
    
    NativeInterface originChat("originChat", new OriginChatProxy(user->socialControllerInstance()), true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(originChat));
    
    NativeInterface originUser("originUser", new OriginUserProxy(user), true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(originUser));

    // This is debatably user specific. All the settings we currently expose are user specific
    NativeInterface contentOperationQueueController("contentOperationQueueController", new ContentOperationQueueControllerProxy, true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(contentOperationQueueController));

    NativeInterface clientSettings("clientSettings", new ClientSettingsProxy, true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(clientSettings));

    NativeInterface installDirectoryManager("installDirectoryManager", new InstallDirectoryManager, true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(installDirectoryManager));

    NativeInterface achievementManager("achievementManager", AchievementManagerProxy::create(), true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(achievementManager));

    NativeInterface originCommon("originCommon", new OriginCommonProxy, true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(originCommon));

    // Engine::Content::ProductInfo has a hidden dependency on the current session so this needs to be user-specifc
    NativeInterface productQuery("productQuery", new ProductQueryProxy, true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(productQuery));

    NativeInterface onTheHouseQuery("onTheHouseQuery", new OnTheHouseQueryProxy, true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(onTheHouseQuery));

#ifdef ORIGIN_PC
    NativeInterface broadcast("broadcast", new BroadcastProxy, true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(broadcast));
#endif

    NativeInterface subscriptionManager("subscriptionManager", SubscriptionManagerProxy::create(), true);
    mUserSpecificInterfaces.append(
        NativeInterfaceRegistry::registerSharedInterface(subscriptionManager));
    emit allProxiesLoaded();
}

void WebWidgetController::userLoggedOut(Origin::Engine::UserRef)
{
    // Unregister all the user specific interfaces
    while(!mUserSpecificInterfaces.isEmpty()) 
    {
        NativeInterfaceRegistry::unregisterInterface(mUserSpecificInterfaces.takeFirst());
    }
}

void WebWidgetController::enablePersistentStorage()
{
    // LoginViewController::init already enables persistent storage, so don't do it here again, it will crash QtWebkit!!!
    //QWebSettings::enablePersistentStorage(roamingDataDirectory() + "/Web Storage");
}
    
void WebWidgetController::updateStarted(const WebWidget::UpdateIdentifier &identifier)
{
    // Record the time this update started for telemetry
    mUpdateStartMSecsSinceEpoch[identifier.etag()] = QDateTime::currentMSecsSinceEpoch();

    ORIGIN_LOG_EVENT << "Web widget update " << identifier.etag() << " started"; 
    GetTelemetryInterface()->Metric_WEBWIDGET_DL_START(identifier.etag().data());
}

void WebWidgetController::updateFinished(const WebWidget::UpdateIdentifier &identifier, WebWidget::UpdateError error)
{
    const qint64 durationMsecs = QDateTime::currentMSecsSinceEpoch() - mUpdateStartMSecsSinceEpoch.take(identifier.etag());

    if (error == WebWidget::UpdateNoError)
    {
        ORIGIN_LOG_EVENT << "Web widget update " << identifier.etag() << " finished successfully" 
            << " in " << durationMsecs << " milliseconds";
        
        GetTelemetryInterface()->Metric_WEBWIDGET_DL_SUCCESS(identifier.etag().data(), static_cast<unsigned int>(durationMsecs));
    }
    else
    {
        ORIGIN_LOG_EVENT << "Web widget update " << identifier.etag() << " failed"
            << " with error code " << int(error)
            << " in " << durationMsecs << " milliseconds";
        
        GetTelemetryInterface()->Metric_WEBWIDGET_DL_ERROR(identifier.etag().data(), int(error));
    }
}

WebWidgetController* WebWidgetController::instance()
{
    return WebWidgetControllerInstance;
}

}
}
