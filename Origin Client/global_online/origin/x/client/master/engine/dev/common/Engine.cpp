//  Engine.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "engine/Engine.h"
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "engine/login/SingleLoginController.h"
#include "engine/content/Entitlement.h"
#include "services/downloader/DownloaderErrors.h"
#include "engine/content/NonOriginContentCache.h"
#include "engine/content/NonOriginGameData.h"
#include "services/session/SessionService.h"
#include "../cloudsaves/source/CloudSaves.h"
#include "engine/achievements/achievementmanager.h"
#include "engine/subscription/subscriptionmanager.h"
#include "engine/multiplayer/Invite.h"
#include "engine/social/ConversationManager.h"
#include "engine/dirtybits/DirtyBitsClient.h"
#include "engine/content/CatalogDefinitionController.h"
#include "engine/content/EntitlementCache.h"
#include "services/debug/DebugService.h"
#include "engine/plugin/PluginManager.h"
#include "engine/content/DynamicContentTypes.h"
#include "engine/content/NucleusEntitlementController.h"
#include "engine/cloudsaves/RemoteSync.h"

// -- BEGIN OMINOUS WARNING --
// Nothing in engine.cpp uses these files, but they are needed by Origin 
// plugins. The linker (at least on Windows) will toss out these definitions
// and they will not be exported. If you don't want to spend 3hrs of your day
// chasing down obscure linking issues, do NOT remove these #includes.
#include "engine/config/ConfigFile.h"
// -- END OMINOUS WARNING --

using namespace Origin::Engine::Content;
using namespace Origin::Downloader;
using namespace Origin::Engine::Achievements;
using namespace Origin::Engine::Social;
using namespace Origin::Engine::Subscription;

namespace Origin
{
    namespace Engine
    {
        void init()
        {
            qRegisterMetaType<uint32_t>("uint32_t");
            qRegisterMetaType<uint64_t>("uint64_t");
            qRegisterMetaType<QObjectList>("QObjectList");
            qRegisterMetaType<QWebPage*>("QWebPage*");
            qRegisterMetaType<QElapsedTimer*>("QElapsedTimer*");
            qRegisterMetaType<QList<QString> >("QList<QString>");
            qRegisterMetaType<QSet<QString> >("QSet<QString>");
            qRegisterMetaType<UserRef>("Origin::Engine::UserRef");
            qRegisterMetaType<EntitlementRef>("Origin::Engine::Content::EntitlementRef");
            qRegisterMetaType<EntitlementWRef>("Origin::Engine::Content::EntitlementWRef");
            qRegisterMetaType<InstallArgumentRequest>("Origin::Downloader::InstallArgumentRequest");
            qRegisterMetaType<InstallLanguageRequest>("Origin::Downloader::InstallLanguageRequest");
            qRegisterMetaType<EulaStateRequest>("Origin::Downloader::EulaStateRequest");
            qRegisterMetaType<InstallArgumentResponse>("Origin::Downloader::InstallArgumentResponse");
            qRegisterMetaType<EulaLanguageResponse>("Origin::Downloader::EulaLanguageResponse");
            qRegisterMetaType<EulaStateResponse>("Origin::Downloader::EulaStateResponse");
            qRegisterMetaType<CancelDialogRequest>("Origin::Downloader::CancelDialogRequest");
            qRegisterMetaType<ThirdPartyDDRequest>("Origin::Downloader::ThirdPartyDDRequest");
            qRegisterMetaType<DownloadErrorInfo>("Origin::Downloader::DownloadErrorInfo");
            qRegisterMetaType<QList<EntitlementRef> >("QList<Origin::Engine::Content::EntitlementRef>");
            qRegisterMetaType<EntitlementRefList>("Origin::Engine::Content::EntitlementRefList");
            qRegisterMetaType<EntitlementRefSet>("Origin::Engine::Content::EntitlementRefSet");
            qRegisterMetaType<ContentInstallFlowState>("Origin::Downloader::ContentInstallFlowState");
            qRegisterMetaType<LoginController::Error>("Origin::Engine::LoginController::Error");
            qRegisterMetaType<InstallProgressState>("Origin::Downloader::InstallProgressState");
            qRegisterMetaType<AchievementManagerRef>("Origin::Engine::Achievements::AchievementManagerRef");
            qRegisterMetaType<AchievementPortfolioRef>("Origin::Engine::Achievements::AchievementPortfolioRef");
            qRegisterMetaType<SubscriptionManagerRef>("Origin::Engine::Subscription::SubscriptionManagerRef");
            qRegisterMetaType<SubscriptionRedemptionError>("Origin::Engine::Subscription::SubscriptionRedemptionError");
            qRegisterMetaType<AchievementSetRef>("Origin::Engine::Achievements::AchievementSetRef");
            qRegisterMetaType<AchievementRef>("Origin::Engine::Achievements::AchievementRef");
            qRegisterMetaType<MultiplayerInvite::Invite>("Origin::Engine::MultiplayerInvite::Invite");
            qRegisterMetaType<QSharedPointer<Conversation> >("QSharedPointer<Origin::Engine::Social::Conversation>");
            qRegisterMetaType<Conversation::CreationReason>("Origin::Engine::Social::Conversation::CreationReason");
            qRegisterMetaType<Conversation::FinishReason>("Origin::Engine::Social::Conversation::FinishReason");
            qRegisterMetaType<Chat::OriginGameActivity>("Origin::Chat::OriginGameActivity");
            qRegisterMetaType<BaseGameRef>("Origin::Engine::Content::BaseGameRef");
            qRegisterMetaType<NonOriginGameData>("Origin::Engine::Content::NonOriginGameData");
            qRegisterMetaType<DynamicDownloadChunkStateT>("Origin::Engine::Content::DynamicDownloadChunkStateT");
            qRegisterMetaType<DynamicDownloadChunkRequirementT>("Origin::Engine::Content::DynamicDownloadChunkRequirementT");
            qRegisterMetaType<ContentRefreshMode>("Origin::Engine::Content::ContentRefreshMode");
            qRegisterMetaType<LocalContent::PlayResult>("Origin::Engine::Content::LocalContent::PlayResult");
            qRegisterMetaType<Origin::Engine::CloudSaves::RemoteSync::TransferDirection>("Origin::Engine::CloudSaves::RemoteSync::TransferDirection");
            qRegisterMetaType<Origin::Engine::IIGOCommandController::CallOrigin>("Origin::Engine::IIGOCommandController::CallOrigin");

            LoginController::init();
            // IGOController initialized on first use, omitted here, see LocalContent::play()
            SingleLoginController::init();

            NucleusEntitlementController::init();

            Cache::NonOriginContentCache::init();
            EntitlementCache::init();

            CloudSaves::init();

            // Initialize the achievement service. 
            AchievementManager::init();

            DirtyBitsClient::init();
            
            ORIGIN_VERIFY_CONNECT( 
                Origin::Services::Session::SessionService::instance(), 
                SIGNAL(beginSessionComplete(Origin::Services::Session::SessionError,Origin::Services::Session::SessionRef,QSharedPointer<Origin::Services::Session::AbstractSessionConfiguration>)), 
                Content::EntitlementCache::instance(), 
                SLOT(onBeginSessionComplete(Origin::Services::Session::SessionError,Origin::Services::Session::SessionRef))
                );

            SubscriptionManager::init();
            
            PluginManager::init();
        }

        void release()
        {
            PluginManager::release();

            EntitlementCache::release();    
            Cache::NonOriginContentCache::release();

            SingleLoginController::release();
            IGOController::release();
            LoginController::release();

            CatalogDefinitionController::release();
        }
    }
}
