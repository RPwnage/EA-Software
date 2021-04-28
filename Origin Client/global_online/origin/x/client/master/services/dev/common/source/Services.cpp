//  Services.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "services/Services.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"
#include "services/platform/PlatformJumplist.h"
#include "services/platform/TrustedClock.h"
#include "services/config/OriginConfigService.h"
#include "services/session/SessionService.h"
#include "services/settings/SettingsManager.h"
#include "services/settings/Setting.h"
#include "services/downloader/UnpackService.h"
#include "services/downloader/DownloadService.h"
#include "services/rest/OriginServiceValues.h"
#include "services/rest/HttpStatusCodes.h"
#include "services/rest/OriginServiceResponse.h"
#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/connection/ConnectionStates.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/network/NetworkCookieJar.h"
#include "services/publishing/DownloadUrlServiceClient.h"
#include "services/publishing/NucleusEntitlementServiceResponse.h"
#include "services/publishing/NucleusEntitlement.h"
#include "services/publishing/CatalogDefinition.h"
#include "services/publishing/SignatureVerifier.h"
#include "services/entitlements/BoxartData.h"
#include "services/heartbeat/Heartbeat.h"
#include "services/voice/VoiceDeviceService.h"
#include "services/voice/VoiceService.h"
#include "services/process/IProcess.h"
#include "TelemetryAPIDLL.h"

#include <QDir>

#if defined (ORIGIN_MAC)
#  include "services/process/ProcessOSX.h"
#endif


// note: these cannot be place inside a namespace according to Qt documentation
inline void initResources()
{
    Q_INIT_RESOURCE(services);
}

inline void cleanupResources()
{
    Q_CLEANUP_RESOURCE(services);
}


namespace Origin
{
    namespace Services
    {
        // Temporary values to be used for development only
        const QString sEnvironment("dev");
        const QString sIniFilePath("");

        static bool sIsInitialized = false;

        bool init(int argc, char* argv[])
        {
            bool configOk = true;
            if (!sIsInitialized)
            {
                initResources();

                qRegisterMetaType<Session::SessionError> ("Origin::Services::Session::SessionError");
                qRegisterMetaType<Session::SessionRef> ("Origin::Services::Session::SessionRef");
                qRegisterMetaType<Session::AccessToken> ("Origin::Services::Session::AccessToken");
                qRegisterMetaType<Session::SsoToken> ("Origin::Services::Session::SsoToken");
                qRegisterMetaType<Session::AuthenticationState> ("Origin::Services::Session::AuthenticationState");
                qRegisterMetaType<Setting> ("Origin::Services::Setting");
                qRegisterMetaType<Variant> ("Origin::Services::Variant");
                qRegisterMetaType<restError>("Origin::Services::restError");
                qRegisterMetaType<HttpStatusCode>("Origin::Services::HttpStatusCode");
                qRegisterMetaType<LogMsgType>("Origin::Services::LogMsgType");
                qRegisterMetaType<Connection::ConnectionStateField>("Origin::Services::Connection::ConnectionStateField");
                qRegisterMetaType<Setting>("Origin::Services::Setting");
                qRegisterMetaType<Variant>("Origin::Services::Variant");
                qRegisterMetaType<Entitlements::Parser::BoxartData>("Origin::Services::Entitlements::Parser::BoxartData");
                qRegisterMetaType<Publishing::NucleusEntitlementRef>("Origin::Services::Publishing::NucleusEntitlementRef");
                qRegisterMetaType<Publishing::CatalogDefinitionRef>("Origin::Services::Publishing::CatalogDefinitionRef");
                qRegisterMetaType<Publishing::EntitlementRefreshType>("Origin::Services::Publishing::EntitlementRefreshType");
                qRegisterMetaType<IProcess::ProcessState>("Origin::Services::IProcess::ProcessState");
                qRegisterMetaType<IProcess::ExitStatus>("Origin::Services::IProcess::ExitStatus");
#if ENABLE_VOICE_CHAT
                qRegisterMetaType<Voice::AudioDeviceType>("Origin::Services::Voice::AudioDeviceType");
                qRegisterMetaType<Voice::AudioDeviceRole>("Origin::Services::Voice::AudioDeviceRole");
#endif
                DebugService::init();
                LogService::init(argc, argv);
                OriginTelemetry::init("");
                PlatformService::init();
                TrustedClock::instance(); // avoid non-thread-safe instantiation later
#if defined (ORIGIN_MAC)
                initializeProcessNotificationHandler();
#endif
                Network::GlobalConnectionStateMonitor::init();
                RestAuthenticationFailureNotifier::init();
                Session::SessionService::init();
                Connection::ConnectionStatesService::init(); //needs to happen after SessionService so that it can listen for signals from SessionService
                SettingsManager::init();
                               
                configOk = OriginConfigService::init(); // need the environment, so must happen after SettingsManager

                // re-parse overrides
                SettingsManager::instance()->loadOverrideSettings();

                // Now we can properly set the logging level
#if ORIGIN_DEBUG
                bool logDebug = true;
#else
                bool logDebug = Origin::Services::readSetting(Origin::Services::SETTING_LogDebug,  Origin::Services::Session::SessionRef());
#endif
                Logger::Instance()->SetLogDebug(logDebug);

                // Once SettingsManager is initialized, we need to have the
                // connection monitor pull some settings to finish 
                // initialization
                Network::GlobalConnectionStateMonitor::postSettingsInit();

                Publishing::DownloadUrlProviderManager::init();
                Publishing::DownloadUrlServiceClient::init();
                Downloader::DownloadService::init();
                Downloader::UnpackService::init();
                Publishing::SignatureVerifier::init();
                Publishing::NucleusEntitlement::init();
                PlatformJumplist::init();
                VoiceService::init();

                ORIGIN_VERIFY_CONNECT(
                    Session::SessionService::instance(),
                    SIGNAL(endSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef)),
                    Origin::Services::NetworkAccessManagerFactory::instance()->sharedCookieJar(),
                    SLOT(clear())
                    );
                ORIGIN_VERIFY_CONNECT(
                    Session::SessionService::instance(),
                    SIGNAL(endSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef)),
                    Origin::Services::SettingsManager::instance(),
                    SLOT(onLogout())
                    );

                // Heartbeat should be initialized shortly after NetworkAccessManagerFactory has been created
                // because Heartbeat has to know about each created NetworkAccessManager created by the
                // factory in order to report its necessary stats.
                Origin::Services::Heartbeat::create();

                sIsInitialized = true;
            }
            return configOk;
        }

        void release()
        {
            if (sIsInitialized)
            {
                VoiceService::release();
                PlatformJumplist::release();
                Publishing::NucleusEntitlement::release();
                Publishing::SignatureVerifier::release();
                Downloader::UnpackService::release();
                Downloader::DownloadService::release();
                Publishing::DownloadUrlServiceClient::release();
                Publishing::DownloadUrlProviderManager::release();
                OriginConfigService::release();
                SettingsManager::free();
                Connection::ConnectionStatesService::release();
                Session::SessionService::release();
                RestAuthenticationFailureNotifier::release();
                Network::GlobalConnectionStateMonitor::release();
                PlatformService::release();
                OriginTelemetry::release();
                LogService::release();
                DebugService::release();

                cleanupResources();

                sIsInitialized = false;
            }
        }

        QString const& environment()
        {
            return sEnvironment;
        }
    }
}
