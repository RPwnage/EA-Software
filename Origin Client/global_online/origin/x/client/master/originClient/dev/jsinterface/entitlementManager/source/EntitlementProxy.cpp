/////////////////////////////////////////////////////////////////////////////
// EntitlementProxy.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include <QString>
#include <QMovie>
#include <QAction>
#include <QDebug>

#include <services/debug/DebugService.h>
#include <services/log/LogService.h>
#include <services/platform/EnvUtils.h>
#include "services/publishing/PricingServiceClient.h"
#include "services/trials/TrialServiceClient.h"
#include <services/platform/TrustedClock.h>
#include "engine/cloudsaves/LocalStateBackup.h"
#include "engine/content/CloudContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/CustomBoxartController.h"
#include "engine/content/Entitlement.h"
#include "engine/content/GamesController.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ProductArt.h"
#include <engine/downloader/ContentInstallFlowState.h>
#include "engine/login/LoginController.h"
#include "engine/content/PlayFlow.h"
#include "engine/content/UpdateCheckFlow.h"
#include "engine/subscription/SubscriptionManager.h"
#include "utilities/LocalizedContentString.h"

#include "EntitlementManager.h"
#include "EntitlementProxy.h"
#include "EntitlementInstallFlowProxy.h"
#include "NonOriginGameProxy.h"
#include "BoxartProxy.h"

#include "EntitlementCloudSaveProxy.h"
#include "CloudSaveDebugActions.h"
#include "ClientFlow.h"

#include "MainFlow.h"
#include "InstallerFlow.h"
#include "ContentFlowController.h"
#include "LocalContentViewController.h"
#include "TelemetryAPIDLL.h"
#include "SaveBackupRestoreFlow.h"

#include "Qt/OriginWindow.h"

using namespace Origin::Engine;

#if ORIGIN_DEBUG
#define ENABLE_BOX_ART_TRACING 0
#else
#define ENABLE_BOX_ART_TRACING 0
#endif

namespace
{
    // Publishing doesn't allow us to show release dates more than a year in the future as they may be unreliable
    const int MaximumFutureReleaseDisplayDays = 365;

    // Reveal date is the earliest time we can show a given release date
    QDateTime revealDateForReleaseDate(const QDateTime &orcDate)
    {
        return orcDate.addDays(-MaximumFutureReleaseDisplayDays);
    }

    // Display date is a null QVariant if the orcDate is invalid or we haven't hit the reveal date
    QVariant displayDateForReleaseDate(const QDateTime &orcDate)
    {
        if (!orcDate.isValid())
        {
            return QVariant();
        }

        return orcDate;
    }
}

namespace Origin
{
namespace Client
{
namespace JsInterface
{

EntitlementProxy::EntitlementProxy(Origin::Engine::Content::EntitlementRef entitlement, QObject *parent) :
    QObject(parent),
	mEntitlement(entitlement),
	mCloudSaves(NULL),
	mInstallFlowProxy(new EntitlementInstallFlowProxy(this, mEntitlement)),
    mNonOriginGame(NULL),
    mBoxart(NULL),
    mCloudDebugActions(NULL),
    mDisplayDateRevealTimer(NULL)
{
    using Origin::Engine::Subscription::SubscriptionManager;

    ORIGIN_VERIFY_CONNECT(entitlement->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT(onStateChanged()));

    ORIGIN_VERIFY_CONNECT(entitlement->localContent()->playFlow().data(), SIGNAL(stopped()), this, SIGNAL(changed()));
    ORIGIN_VERIFY_CONNECT(entitlement->localContent()->playFlow().data(), SIGNAL(started()), this, SIGNAL(changed()));

	ORIGIN_VERIFY_CONNECT(entitlement.data(), SIGNAL(requestVisible()), this, SIGNAL(ensureVisible()));

    ORIGIN_VERIFY_CONNECT(entitlement.data(), SIGNAL(childAdded(Origin::Engine::Content::EntitlementRef)),
        this, SLOT(childAdded(Origin::Engine::Content::EntitlementRef)));
    
	ORIGIN_VERIFY_CONNECT(entitlement.data(), SIGNAL(childRemoved(Origin::Engine::Content::EntitlementRef)),
        this, SLOT(childRemoved(Origin::Engine::Content::EntitlementRef)));

    ORIGIN_VERIFY_CONNECT(Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
        this, SLOT(settingChanged(const QString&)));

    ORIGIN_VERIFY_CONNECT(Engine::Content::GamesController::currentUserGamesController(), SIGNAL(playingGameTimeUpdateCompleted(const QString &, const QString &)),
        this, SLOT(playingGameTimeUpdated(const QString &, const QString &)));

    ORIGIN_VERIFY_CONNECT(Engine::Content::ContentController::currentUserContentController()->boxartController(),
        SIGNAL(boxartChanged(const QString&)), this, SLOT(onBoxartChanged(const QString&)));

    mCurrentFavoriteValue = favorite();
    mCurrentHiddenValue = hidden();
    mCurrentMultiLaunchDefault = multiLaunchDefault();

    // Make sure we emit changed() once we reveal various ORC dates to the user
    ORIGIN_VERIFY_CONNECT(mEntitlement->contentConfiguration().data(), SIGNAL(configurationChanged()),
        this, SLOT(onContentConfigurationChanged()));
    armDisplayDateRevealTimer();

    ORIGIN_VERIFY_CONNECT(SubscriptionManager::instance(), &SubscriptionManager::entitleFailed, this, &EntitlementProxy::onEntitleFailed);
    ORIGIN_VERIFY_CONNECT(SubscriptionManager::instance(), &SubscriptionManager::revokeFailed, this, &EntitlementProxy::onRevokeFailed);
}

QDateTime EntitlementProxy::entitleDate()
{
	return mEntitlement->contentConfiguration()->entitleDate();
}

QStringList EntitlementProxy::boxartUrls()
{
    QStringList urls;
    if (!boxart().isNull())
    {
        urls << mBoxart->boxartUrls();
    }

    return urls;
}

QStringList EntitlementProxy::multiLaunchTitles()
{
    QStringList launchTitleList = mEntitlement->localContent()->launchTitleList();

    Engine::Content::GamesController* gamesController = Engine::Content::GamesController::currentUserGamesController();
    const QString title = gamesController ? gamesController->gameMultiLaunchDefault(id()) : Engine::Content::GamesController::SHOW_ALL_TITLES;
    if(title != Engine::Content::GamesController::SHOW_ALL_TITLES && title != "" && launchTitleList.contains(title))
    {
        // Find it in the current list and move it to the top.
        launchTitleList.move(launchTitleList.indexOf(title), 0);
    }

    return launchTitleList;
}

QStringList EntitlementProxy::platformsSupported()
{
	QStringList platforms;

	foreach (Services::PlatformService::OriginPlatform platform, mEntitlement->contentConfiguration()->platformsSupported())
	{
        if (platform == Services::PlatformService::PlatformWindows) 
		{
			platforms.append("PC");
		}
        else if (platform == Services::PlatformService::PlatformMacOS) 
		{
			platforms.append("MAC");
		}
		else 
		{
			platforms.append("OTHER");
		}
	}

	return platforms;
}

QStringList EntitlementProxy::localesSupported()
{
    return mEntitlement->contentConfiguration()->supportedLocales();
}

QStringList EntitlementProxy::bannerUrls()
{
    using namespace Engine::Content::ProductArt;
    return urlsForType(mEntitlement->contentConfiguration(), OwnedGameDetailsBanner);
}

QString EntitlementProxy::contentId()
{
    return mEntitlement->contentConfiguration()->contentId();
}

QString EntitlementProxy::financeId()
{
    return mEntitlement->contentConfiguration()->financeId();
}

QString EntitlementProxy::productId()
{
    return mEntitlement->contentConfiguration()->productId();
}

QStringList EntitlementProxy::relatedGameContentIds()
{
    return mEntitlement->contentConfiguration()->relatedGameContentIds();
}

QString EntitlementProxy::id()
{
    return mEntitlement->contentConfiguration()->productId();
}

QString EntitlementProxy::title()
{
	return mEntitlement->contentConfiguration()->displayName();
}

QVariant EntitlementProxy::manualUrl()
{
    const QString url(mEntitlement->contentConfiguration()->manualUrl());

    if (url.isEmpty())
    {
        return QVariant();
    }

	return url;
}

QVariant EntitlementProxy::displayUnlockStartDate()
{
	return displayDateForReleaseDate(mEntitlement->contentConfiguration()->releaseDate());
}

QVariant EntitlementProxy::displayDownloadStartDate()
{
    return displayDateForReleaseDate(mEntitlement->contentConfiguration()->downloadStartDate());
}

QVariant EntitlementProxy::displayExpirationDate()
{
    return displayDateForReleaseDate(mEntitlement->contentConfiguration()->useEndDate());
}

QVariant EntitlementProxy::preAnnouncementDisplayString()
{
    // note this isn't a date, it's a string
    return mEntitlement->contentConfiguration()->preAnnouncementDisplayDate();
}

QDateTime EntitlementProxy::lastPlayedDate()
{
	return mEntitlement->localContent()->lastPlayDate();
}

QString EntitlementProxy::achievementSet()	
{
	//TODO (Hans): Make sure that when we are supporting more than two platforms this code gets modified to take that into account.

	QString achievementSet = mEntitlement->contentConfiguration()->achievementSet();

	if(achievementSet.isEmpty())
	{
		achievementSet = mEntitlement->contentConfiguration()->achievementSet(Origin::Services::PlatformService::PlatformWindows);
	}

	return achievementSet;
}

void EntitlementProxy::play()
{
    Origin::Engine::Content::EntitlementRef suppressingEntitlement;

    // If the current entitlement is suppressed use the suppressing entitlement to start the game.
    if(mEntitlement->contentController()->isEntitlementSuppressed(mEntitlement, &suppressingEntitlement))
    {
        MainFlow::instance()->contentFlowController()->startPlayFlow(suppressingEntitlement->contentConfiguration()->productId(), false);
    }
    else
    {
        MainFlow::instance()->contentFlowController()->startPlayFlow(mEntitlement->contentConfiguration()->productId(), false);
    }
}

void EntitlementProxy::startDownload()
{
    Origin::Engine::Content::ContentController* cc = mEntitlement->contentController();
    if (!cc)
    {
        // TBD: telemetry here?
        return;
    }

    Origin::Engine::Content::ContentController::DownloadPreflightingResult result;
    cc->doDownloadPreflighting(*mEntitlement->localContent(), result);
    if (result >= Origin::Engine::Content::ContentController::CONTINUE_DOWNLOAD)
        mEntitlement->localContent()->download("");
}

QVariant EntitlementProxy::cloudSaves()
{
	if (!mCloudSaves && mEntitlement->localContent()->cloudContent()->hasCloudSaveSupport())
	{
		mCloudSaves = new EntitlementCloudSaveProxy(this, mEntitlement);

		// emit changed() whenever our cloud save properties change
		ORIGIN_VERIFY_CONNECT(mCloudSaves, SIGNAL(changed()),
			this, SIGNAL(changed()));
	}

	return QVariant::fromValue<QObject*>(mCloudSaves);
}

QVariant EntitlementProxy::nonOriginGame()
{
    if (mEntitlement->contentConfiguration()->isNonOriginGame())
    {
        if (!mNonOriginGame)
        {
            mNonOriginGame = new NonOriginGameProxy(this, mEntitlement);
        }

       return QVariant::fromValue<QObject*>(mNonOriginGame);
    }

   return QVariant();
}

QVariant EntitlementProxy::boxart()
{
   if (!mBoxart)
   {
       mBoxart = new BoxartProxy(this, mEntitlement);
   }

   return QVariant::fromValue<QObject*>(mBoxart);
}

QString EntitlementProxy::registrationCode()
{
	return mEntitlement->contentConfiguration()->cdKey();
}

void EntitlementProxy::uninstall()
{
    if (mEntitlement->contentConfiguration()->isPDLC())
    {
        Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showUninstallConfirmationForDLC(mEntitlement);
    }
    else
    {
#ifdef ORIGIN_MAC
        Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showUninstallConfirmation(mEntitlement);
#else
        Downloader::IContentInstallFlow* installFlow = NULL;
        if (mEntitlement && mEntitlement->localContent() && mEntitlement->localContent()->installFlow())
        {
            installFlow = mEntitlement->localContent()->installFlow();
        }

        if (installFlow && installFlow->isDynamicDownload() && installFlow->getFlowState() == Downloader::ContentInstallFlowState::kPaused && mEntitlement->localContent()->playable())
        {
            Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showUninstallConfirmation(mEntitlement);
        }
        else
        {
            GetTelemetryInterface()->Metric_GAME_UNINSTALL_START(mEntitlement->contentConfiguration()->productId().toUtf8().data());
            mEntitlement->localContent()->unInstall();
        }
#endif
    }

    // Make sure we refresh the view (especially when doing the uninstall from the game details)
    emit changed();
}

bool EntitlementProxy::repairSupported()
{
    return mEntitlement->localContent()->repairable();
}

bool EntitlementProxy::updateSupported()
{
	return dipUpdateSupported() && !repairing();
}

QVariant EntitlementProxy::availableUpdateVersion() 
{
    if (mEntitlement->localContent()->updateAvailable())
    {
        return QVariant(mEntitlement->contentConfiguration()->version());
    }
    
    return QVariant();
}

bool EntitlementProxy::updating()
{
    return mEntitlement->localContent()->updating();
}

bool EntitlementProxy::repairing()
{
    return mEntitlement->localContent()->repairing();
}

void EntitlementProxy::checkForUpdateAndInstall()
{
    Origin::Engine::Content::UpdateCheckFlow* updateCheck = new Origin::Engine::Content::UpdateCheckFlow(mEntitlement);
    ORIGIN_VERIFY_CONNECT(updateCheck, SIGNAL(updateAvailable()), this, SLOT(onUpdateCheckCompletedForInstall()));
    ORIGIN_VERIFY_CONNECT(updateCheck, SIGNAL(noUpdateAvailable()), this, SLOT(onUpdateCheckCompletedForInstall()));
    updateCheck->begin();
}

void EntitlementProxy::onUpdateCheckCompletedForInstall()
{
    Origin::Engine::Content::UpdateCheckFlow* updateCheck = dynamic_cast<Origin::Engine::Content::UpdateCheckFlow*>(sender());

    if(updateCheck != NULL)
    {
        updateCheck->deleteLater();
    }

    // we run through the update check flow just to make sure we are updating to the most recent version
    // Whether an update actually exists or not is handled in the normal install flow, and the user feedback 
    // flows out of the install flow state changes, so we can't skip it here even if no update exists.
    installUpdate();
}

void EntitlementProxy::onUnownedContentPricingAvailable()
{
    Services::Publishing::PricingResponse *response = dynamic_cast<Services::Publishing::PricingResponse *>(sender());
    if (!response)
    {
        return;
    }

    const Content::EntitlementRefList &children = mEntitlement->children(Content::UnownedContent);
    foreach (const Content::EntitlementRef &child, children)
    {
        Content::ContentConfigurationRef configuration = child->contentConfiguration();
        auto price = response->prices().find(configuration->productId());

        if (price != response->prices().end())
        {
            configuration->setPricingData(price.value());
        }
    }
}

void EntitlementProxy::timedTrialGrantTime()
{
    auto resp = Services::Trials::TrialServiceClient::grantTime(mEntitlement->contentConfiguration()->contentId());
    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), mEntitlement->localContent(), SLOT(updateTrialTime()));
}

void EntitlementProxy::onEntitleFailed(const QString &offerId)
{
    if (offerId == mEntitlement->contentConfiguration()->productId())
    {
        emit entitleFailed();
    }
}

void EntitlementProxy::onRevokeFailed(const QString &offerId)
{
    if(offerId == mEntitlement->contentConfiguration()->productId())
    {
        emit revokeFailed();
    }
}


void EntitlementProxy::installUpdate()
{
    mEntitlement->localContent()->update();
}

void EntitlementProxy::purchase()
{
    bool gameOwned = owned();
    if(gameOwned)
    {
        if(mEntitlement->contentConfiguration()->isEntitledFromSubscription() && Engine::Subscription::SubscriptionManager::instance()->isActive() == false)
        {
            gameOwned = false;
        }
    }

    if (!gameOwned && purchasable())
    {
        if (!mEntitlement->parent().isNull())
        {
            if (mEntitlement->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeOnTheHouse && mEntitlement->contentConfiguration()->isFreeProduct())
            {
                MainFlow::instance()->contentFlowController()->entitleFreeGame(mEntitlement);
            }
            else if(Engine::IGOController::instance()->isActive())
            {
                Engine::IGOController::instance()->igoShowCheckoutUI(mEntitlement);
            }
            else
            {
                mPdlcViewController.purchase(mEntitlement);
            }
        }
        else
        {
            ClientFlow::instance()->showMasterTitleInStore(mEntitlement->contentConfiguration()->masterTitleId());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// Subscription functions.

bool EntitlementProxy::isEntitledFromSubscription() const
{
    return mEntitlement->contentConfiguration()->isEntitledFromSubscription();
}

bool EntitlementProxy::hasEntitlementUpgraded() const
{
    return mEntitlement->contentController()->hasEntitlementUpgraded(mEntitlement);
}

bool EntitlementProxy::isAvailableFromSubscription() const
{
    using Origin::Engine::Subscription::SubscriptionManager;

    return SubscriptionManager::instance()->isAvailableFromSubscription(mEntitlement->contentConfiguration()->productId());
}

bool EntitlementProxy::isSuppressed() const
{
    return mEntitlement->contentController()->isEntitlementSuppressed(mEntitlement);
}

QString EntitlementProxy::upgradeTypeAvailable() const
{
    if(!isSuppressed())
    {
        switch(mEntitlement->isUpgradeAvailable())
        {
        case Engine::Subscription::UPGRADEAVAILABLE_BASEGAME_ONLY:
            return "BASEGAME_ONLY";
        case Engine::Subscription::UPGRADEAVAILABLE_DLC_ONLY:
            return "DLC_ONLY";
        case Engine::Subscription::UPGRADEAVAILABLE_BASEGAME_AND_DLC:
            return "BASEGAME_AND_DLC";
        case Engine::Subscription::UPGRADEAVAILABLE_NONE:
        default:
            return "NONE";
        }
    }
    return "NONE";
}

QVariant EntitlementProxy::subscriptionRetiring() const
{
    if(mEntitlement->contentConfiguration()->hasExpiration())
    {
        return mEntitlement->contentConfiguration()->originSubscriptionUseEndDate();
    }
    else
    {
        return QVariant();
    }
}


QVariant EntitlementProxy::subscriptionRetiringTime() const
{
    int debug = Services::readSetting(Services::SETTING_SubscriptionEntitlementRetiringTime);
    if(debug >= 0)
    {
        return debug;
    }
    if(mEntitlement->contentConfiguration()->hasExpiration())
    {
        debug = Origin::Services::TrustedClock::instance()->nowUTC().secsTo(mEntitlement->contentConfiguration()->originSubscriptionUseEndDate());
        return debug > 0 ? debug : 0;
    }
    else
    {
        return QVariant();
    }
}

bool EntitlementProxy::isUpgradeBackupSaveAvailable()
{
    const bool debug_isUpgradeBackupSaveAvailable = SaveBackupRestoreFlow::isUpgradeBackupSaveAvailable(mEntitlement);
    return debug_isUpgradeBackupSaveAvailable;
}

void EntitlementProxy::restoreSubscriptionUpgradeBackupSave()
{
    Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showSaveBackupRestoreWarning(mEntitlement);
}

void EntitlementProxy::entitle()
{
    const QString &offerId = mEntitlement->contentConfiguration()->productId();
    Engine::Subscription::SubscriptionManager *subscriptionManager;

    subscriptionManager = Engine::Subscription::SubscriptionManager::instance();
    if (subscriptionManager->isActive() && isAvailableFromSubscription() && !owned())
    {
        subscriptionManager->entitle(offerId);
    }
    else
    {
        onEntitleFailed(offerId);
    }
}

void EntitlementProxy::revoke()
{
    const QString &offerId = mEntitlement->contentConfiguration()->productId();
    Engine::Subscription::SubscriptionManager *subscriptionManager;

    subscriptionManager = Engine::Subscription::SubscriptionManager::instance();
    if(subscriptionManager->isActive() && isEntitledFromSubscription())
    {
        subscriptionManager->revoke(offerId);
    }
    else
    {
        onRevokeFailed(offerId);
    }
}

void EntitlementProxy::upgrade()
{
    // This is DLC
    if(mEntitlement->parent())
    {
        Engine::Content::EntitlementRef parent = mEntitlement->parent();
        if(parent->contentConfiguration()->showSubsSaveGameWarning())
            Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showDLCUpgradeWarning(parent, mEntitlement->contentConfiguration()->productId(), mEntitlement->contentConfiguration()->displayName());
        else
            Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showDLCUpgradeStandard(parent, mEntitlement->contentConfiguration()->displayName());
    }
    else
    {
        if(mEntitlement->contentConfiguration()->showSubsSaveGameWarning())
            Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showUpgradeWarning(mEntitlement);
        else
            Engine::Subscription::SubscriptionManager::instance()->upgrade(mEntitlement->contentConfiguration()->masterTitleId());
    }
}

void EntitlementProxy::downgrade()
{
    Origin::Engine::Subscription::SubscriptionManager::instance()->downgrade(mEntitlement->contentConfiguration()->masterTitleId());
}

void EntitlementProxy::repair()
{
    // we run through the update check flow just to make sure we are repairing to the most recent version
    // technically it's not necessary otherwise.
    Origin::Engine::Content::UpdateCheckFlow* updateCheck = new Origin::Engine::Content::UpdateCheckFlow(mEntitlement);
    ORIGIN_VERIFY_CONNECT(updateCheck, SIGNAL(updateAvailable()), this, SLOT(onUpdateCheckCompletedForRepair()));
    ORIGIN_VERIFY_CONNECT(updateCheck, SIGNAL(noUpdateAvailable()), this, SLOT(onUpdateCheckCompletedForRepair()));
    updateCheck->begin();
}

void EntitlementProxy::onUpdateCheckCompletedForRepair()
{
    Origin::Engine::Content::UpdateCheckFlow* updateCheck = dynamic_cast<Origin::Engine::Content::UpdateCheckFlow*>(sender());

    if(updateCheck != NULL)
    {
        updateCheck->deleteLater();
    }

    // We don't care if the update is available or not, we just ran it to ensure we are repairing to the most recent version...
	mEntitlement->localContent()->repair();
}

bool EntitlementProxy::dipUpdateSupported()
{
   return mEntitlement->localContent()->updatable();
}


QVariant EntitlementProxy::downloadOperation()
{
    Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
    if (installFlow && (installFlow->operationType() == Downloader::kOperation_Download || installFlow->operationType() == Downloader::kOperation_Preload))
    {
        return QVariant::fromValue<QObject*>(mInstallFlowProxy);
    }
    return QVariant();
}

QVariant EntitlementProxy::updateOperation()
{
    Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
    if (installFlow && installFlow->operationType() == Downloader::kOperation_Update)
    {
        return QVariant::fromValue<QObject*>(mInstallFlowProxy);
    }
    return QVariant();
}

QVariant EntitlementProxy::unpackOperation()
{
    Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
    if (installFlow && installFlow->operationType() == Downloader::kOperation_Unpack)
    {
        return QVariant::fromValue<QObject*>(mInstallFlowProxy);
    }
    return QVariant();
}

QVariant EntitlementProxy::repairOperation()
{
    Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
    if (installFlow && installFlow->operationType() == Downloader::kOperation_Repair)
    {
        return QVariant::fromValue<QObject*>(mInstallFlowProxy);
    }
    return QVariant();
}

bool EntitlementProxy::autoUpdateEnabled()
{
    return dipUpdateSupported() &&
        Engine::Content::ContentController::currentUserContentController()->autoPatchingEnabled();
}

QString EntitlementProxy::longDescription()
{
    return mEntitlement->contentConfiguration()->longDescription();
}

QString EntitlementProxy::shortDescription()
{
    return mEntitlement->contentConfiguration()->shortDescription();
}

QVariant EntitlementProxy::installOperation()
{
    Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
    if (installFlow && ((installFlow->operationType() == Downloader::kOperation_Install) || (installFlow->operationType() == Downloader::kOperation_ITO)))
    {
        return QVariant::fromValue<QObject*>(mInstallFlowProxy);
    }
    return QVariant();
}

QVariant EntitlementProxy::currentOperation()
{
    Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
    if (installFlow && installFlow->operationType() != Downloader::kOperation_None)
    {
        return QVariant::fromValue<QObject*>(mInstallFlowProxy);
    }
    return QVariant();
}

bool EntitlementProxy::dynamicContentSupportEnabled()
{
    return mEntitlement->localContent()->isAllowedToPlay() &&
        mEntitlement->contentConfiguration()->softwareBuildMetadata().mbDynamicContentSupportEnabled;
}

bool EntitlementProxy::playable()
{
    return mEntitlement->localContent()->playable();
}

bool EntitlementProxy::playableBitSet()
{
    return mEntitlement->contentConfiguration()->isSupportedByThisMachine();
}

bool EntitlementProxy::inTrash()
{
	return mEntitlement->localContent()->inTrash();
}

void EntitlementProxy::restoreFromTrash()
{
	if (!mEntitlement->localContent()->restoreFromTrash())
    {
        Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showCanNotRestoreFromTrash(mEntitlement);
    }
    else
        onStateChanged();
}

bool EntitlementProxy::downloadable()
{
    return mEntitlement->localContent()->downloadable();
}

bool EntitlementProxy::published()
{
    return mEntitlement->contentConfiguration()->published();
}

bool EntitlementProxy::newlyPublished()
{
    return mEntitlement->contentConfiguration()->newlyPublished();
}

bool EntitlementProxy::downloadDateBypassed()
{
    return (mEntitlement->contentConfiguration()->originPermissions() == Origin::Services::Publishing::OriginPermissions1103);
}

bool EntitlementProxy::releaseDateBypassed()
{
    return (mEntitlement->contentConfiguration()->originPermissions() > Origin::Services::Publishing::OriginPermissionsNormal);
}

QVariant EntitlementProxy::installed()
{
    using namespace Engine::Content;

    switch(mEntitlement->localContent()->installState(true))
    {
    case LocalContent::ContentInstalled:
        return QVariant(true);
    case LocalContent::ContentNotInstalled:
        return QVariant(false);
    case LocalContent::NoInstallCheck:
        // Fall out
        break;
    }
        
    return QVariant();
}

bool EntitlementProxy::uninstallable()
{
    return mEntitlement->localContent()->unInstallable();
}

bool EntitlementProxy::hasUninstaller()
{
    return mEntitlement->localContent()->hasUninstaller();
}

bool EntitlementProxy::playing()
{
	return mEntitlement->localContent()->playing();
}

QVariant EntitlementProxy::debugInfo()
{
	QVariantMap summaryInfo;
    QVariantMap detailedInfo;

    mEntitlement->contentConfiguration()->debugInfo(summaryInfo, detailedInfo);

    if (summaryInfo.isEmpty() && detailedInfo.isEmpty())
    {
        // No debug information to show
        // We null out so the UI knows not to display any debug indicators
        return QVariant();
    }

    QVariantMap ret;
    ret["summary"] = summaryInfo;
    ret["details"] = detailedInfo;

    return ret;
}

qint64 EntitlementProxy::totalSecondsPlayed()
{
	return mEntitlement->localContent()->totalPlaySeconds();
}

QVariant EntitlementProxy::parent()
{
	Content::EntitlementRef entitlement = mEntitlement->parent();

	if (!entitlement)
	{
		return QVariant();
	}

	return QVariant::fromValue<QObject*>(EntitlementManager::instance()->proxyForEntitlement(entitlement));
}

QObjectList EntitlementProxy::addons()
{
    QObjectList ret;

    foreach (Content::EntitlementRef child, mEntitlement->children(Content::AllContent))
    {
        if (Services::Publishing::OriginDisplayTypeAddon == child->contentConfiguration()->originDisplayType())
        {
            ret.append(EntitlementManager::instance()->proxyForEntitlement(child));
        }
    }

    return ret;
}

void EntitlementProxy::fetchUnownedContentPricing()
{
    Services::Publishing::PricingServiceClient *pricingService = Services::Publishing::PricingServiceClient::instance();
    QList<Services::Publishing::PricingResponse *> responses = pricingService->getPricing(
        Services::Session::SessionService::currentSession(),
        mEntitlement->childEntitlementProductIds(Content::UnownedContent),
        mEntitlement->contentConfiguration()->masterTitleId());

    foreach (Services::Publishing::PricingResponse *resp, responses)
    {
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onUnownedContentPricingAvailable()));
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), resp, SLOT(deleteLater()));
    }
}

QObjectList EntitlementProxy::expansions()
{
    QObjectList ret;

    foreach (Content::EntitlementRef child, mEntitlement->children(Content::AllContent))
    {
        if (child->contentConfiguration()->canBeExpansion())
        {
            ret.append(EntitlementManager::instance()->proxyForEntitlement(child));
        }
    }

    return ret;
}

bool EntitlementProxy::isPULC()
{
    return mEntitlement->contentConfiguration()->isPULC();
}

QString EntitlementProxy::displayLocation()
{
	const Origin::Services::Publishing::OriginDisplayType originDisplayType(mEntitlement->contentConfiguration()->originDisplayType());

    // TODO TS3_HARD_CODE_REMOVE
    if (mEntitlement->contentConfiguration()->isSuppressedSims3Expansion())
    {
        return "EXPANSIONS";
    }

    switch (originDisplayType)
    {
    case Origin::Services::Publishing::OriginDisplayTypeExpansion:
        return "EXPANSIONS";
    case Origin::Services::Publishing::OriginDisplayTypeAddon:
        return "ADDONS";
    case Origin::Services::Publishing::OriginDisplayTypeFullGame:
        return "GAMES";
    case Origin::Services::Publishing::OriginDisplayTypeFullGamePlusExpansion:
        return "GAMES|EXPANSIONS";
    case Origin::Services::Publishing::OriginDisplayTypeGameOnly:
    case Origin::Services::Publishing::OriginDisplayTypeNone:
        return QString();
    }

	// Shouldn't happen
	return QString();
}

QString EntitlementProxy::productType()
{
    return mEntitlement->contentConfiguration()->productType();
}

QString EntitlementProxy::packageType()
{
    const Services::Publishing::PackageType packageType(mEntitlement->contentConfiguration()->packageType());

    switch (packageType)
    {
    case Services::Publishing::PackageTypeDownloadInPlace:
        return "DOWNLOAD_IN_PLACE";
    case Services::Publishing::PackageTypeSingle:
        return "SINGLE";
    case Services::Publishing::PackageTypeUnpacked:
        return "UNPACKED";
    case Services::Publishing::PackageTypeExternalUrl:
        return "EXTERNAL_URL";
    case Services::Publishing::PackageTypeOriginPlugin:
        return "ORIGIN_PLUGIN";
    case Services::Publishing::PackageTypeUnknown:
        return "UNKNOWN";
    }

	// Shouldn't happen
    ORIGIN_LOG_WARNING << "Valid package type not found for content " << mEntitlement->contentConfiguration()->productId();
	return QString();
}

bool EntitlementProxy::twitchBlacklisted()
{
    return mEntitlement->contentConfiguration()->twitchClientBlacklist();
}

bool EntitlementProxy::isPreorder()
{
    return mEntitlement->contentConfiguration()->isPreorder();
}

bool EntitlementProxy::isBrowserGame()
{
    return mEntitlement->contentConfiguration()->isBrowserGame();
}

QString EntitlementProxy::releaseStatus()
{
    switch (mEntitlement->localContent()->releaseControlState())
    {
    case Origin::Engine::Content::LocalContent::RC_Released:
        return "AVAILABLE";
    case Origin::Engine::Content::LocalContent::RC_Unreleased:
        return "UNRELEASED";
    case Origin::Engine::Content::LocalContent::RC_Preload:
        return "PRELOAD";
    case Origin::Engine::Content::LocalContent::RC_Expired:
        return "EXPIRED";
    default:
        ORIGIN_LOG_WARNING << "Unhandled releaseControlState [" << mEntitlement->localContent()->releaseControlState() << "]";     
        // we should never get here
        return "RELEASED";
    }
}

QString EntitlementProxy::itemSubType()
{
    switch (mEntitlement->contentConfiguration()->itemSubType())
    {
    case Origin::Services::Publishing::ItemSubTypeAlpha:
        return "ALPHA";
    case Origin::Services::Publishing::ItemSubTypeBeta:
        return "BETA";
    case Origin::Services::Publishing::ItemSubTypeTimedTrial_Access:
        return "TIMED_TRIAL_ACCESS";
    case Origin::Services::Publishing::ItemSubTypeTimedTrial_GameTime:
        return "TIMED_TRIAL_GAMETIME";
    case Origin::Services::Publishing::ItemSubTypeDemo:
        return "DEMO";
    case Origin::Services::Publishing::ItemSubTypeFreeWeekendTrial:
        return "FREE_WEEKEND_TRIAL";
    case Origin::Services::Publishing::ItemSubTypeLimitedTrial:
        return "LIMITED_TRIAL";
    case Origin::Services::Publishing::ItemSubTypeLimitedWeekendTrial:
        return "LIMITED_WEEKEND_TRIAL";
    case Origin::Services::Publishing::ItemSubTypeNonOrigin:
        return "NON_ORIGIN";
    case Origin::Services::Publishing::ItemSubTypeOnTheHouse:
        return "ON_THE_HOUSE";
    default:
        return "NORMAL_GAME";
    }
}

QVariant EntitlementProxy::testCode()
{
    const int testCode = mEntitlement->contentConfiguration()->testCode();

    if (testCode == Engine::Content::ContentConfiguration::NoTestCode)
    {
        return QVariant();
    }

    return QVariant(QString::number(testCode));
}

bool EntitlementProxy::hasPDLCStore()
{
    return mEntitlement->contentConfiguration()->addonsAvailable();
}

bool EntitlementProxy::favorite()
{
    return Content::GamesController::currentUserGamesController()->gameIsFavorite(mEntitlement);
}

void EntitlementProxy::setFavorite(bool favorite)
{
    Content::GamesController::currentUserGamesController()->setGameIsFavorite(mEntitlement, favorite);

    // Re-read our settings to emit changed
    updateFavoriteAndHidden();
}    

bool EntitlementProxy::hidden()
{
    return Content::GamesController::currentUserGamesController()->gameIsHidden(mEntitlement);
}

bool EntitlementProxy::isCompletelySuppressed()
{
    return mEntitlement->contentConfiguration()->isCompletelySuppressed();
}

QString EntitlementProxy::multiLaunchDefault()
{
    return Content::GamesController::currentUserGamesController()->gameMultiLaunchDefault(mEntitlement->contentConfiguration()->productId());
}

void EntitlementProxy::setHidden(bool hidden)
{
    Content::GamesController::currentUserGamesController()->setGameIsHidden(mEntitlement, hidden);
    
    // Re-read our settings to emit changed
    updateFavoriteAndHidden();
}

void EntitlementProxy::updateFavoriteAndHidden()
{
    // Check to see if our cloud settings have changed
    const bool isFavorite = favorite();
    const bool isHidden = hidden();

    if ((isFavorite != mCurrentFavoriteValue) ||
        (isHidden != mCurrentHiddenValue))
    {
        mCurrentFavoriteValue = isFavorite;
        mCurrentHiddenValue = isHidden;

        emit changed();
    }
}

void EntitlementProxy::updateMultiLaunchDefault()
{
    // Check to see if our multi launch default has changed.
    const QString multiLaunchTitle = multiLaunchDefault();

    if (multiLaunchTitle != mCurrentMultiLaunchDefault)
    {
        mCurrentMultiLaunchDefault = multiLaunchTitle;

        emit changed();
    }
}

bool EntitlementProxy::installable() 
{
    return mEntitlement->localContent()->installable();
}

void EntitlementProxy::install() 
{
    mEntitlement->localContent()->install();
}

void EntitlementProxy::childAdded(Origin::Engine::Content::EntitlementRef child)
{
    if (child->contentConfiguration()->originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeAddon)
    {
        emit addonAdded(EntitlementManager::instance()->proxyForEntitlement(child));
    }
    else if (child->contentConfiguration()->canBeExpansion())
    {
        emit expansionAdded(EntitlementManager::instance()->proxyForEntitlement(child));
    }
}

void EntitlementProxy::childRemoved(Origin::Engine::Content::EntitlementRef child)
{
    if (child->contentConfiguration()->originDisplayType() == Origin::Services::Publishing::OriginDisplayTypeAddon)
    {
        emit addonRemoved(EntitlementManager::instance()->proxyForEntitlement(child));
    }
    else if (child->contentConfiguration()->canBeExpansion())
    {
        emit expansionRemoved(EntitlementManager::instance()->proxyForEntitlement(child));
    }
}
 
void EntitlementProxy::settingChanged(const QString& settingName)
{
    if (settingName == Services::SETTING_AUTOPATCH.name())
    {
        if (dipUpdateSupported())
        {
            // autoUpdateEnabled may have changed
            emit changed();
        }
    }
    else if ((settingName == Services::SETTING_HiddenProductIds.name()) ||
             (settingName == Services::SETTING_FavoriteProductIds.name()))
    {
        updateFavoriteAndHidden();
    }
    else if (settingName == Services::SETTING_MULTILAUNCHPRODUCTIDSANDTITLES.name())
    {
        updateMultiLaunchDefault();
    }
}

void EntitlementProxy::onBoxartChanged(const QString& productId)
{
    if (mEntitlement->contentConfiguration()->productId() == productId)
    {
        emit changed();
    }
}

void EntitlementProxy::onStateChanged()
{
    emit changed();
}
    
QObjectList EntitlementProxy::debugActions()
{
    QObjectList actions;

    if (Origin::Services::readSetting(Origin::Services::SETTING_CloudSavesDebug) &&
        mEntitlement->localContent()->cloudContent()->hasCloudSaveSupport())
    {
        if (!mCloudDebugActions)
        {
            mCloudDebugActions = new CloudSaveDebugActions(mEntitlement, this);
        }

        QList<QAction*> cloudActions = mCloudDebugActions->actions();

        // Add all the cloud actions on
        for(QList<QAction*>::ConstIterator it = cloudActions.constBegin();
            it != cloudActions.constEnd();
            it++)
        {
            actions.append(*it);
        }
    }

    const bool inProd = Services::readSetting(Services::SETTING_EnvironmentName).toString().compare(Services::env::production, Qt::CaseInsensitive) == 0;
    if (inProd == false 
        && (mEntitlement->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_Access 
            || mEntitlement->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeTimedTrial_GameTime))
    {
        QAction* actionGrantTime = new QAction("Timed Trial Grant Time", this);
        ORIGIN_VERIFY_CONNECT(actionGrantTime, SIGNAL(triggered()), this, SLOT(timedTrialGrantTime()));
        actions.append(actionGrantTime);
    }

    return actions;
}
    
void EntitlementProxy::displayDateRevealTriggered()
{
    emit changed();
    // Arm the timer for the next release date
    armDisplayDateRevealTimer();
}

void EntitlementProxy::onContentConfigurationChanged()
{
    emit changed();
    armDisplayDateRevealTimer();
}

void EntitlementProxy::armDisplayDateRevealTimer()
{
    QList<QDateTime> releaseDates;
    int secondsToNextDisplayDateReveal = INT_MAX;
    const QDateTime currentDateTime(QDateTime::currentDateTime());

    releaseDates.append(mEntitlement->contentConfiguration()->releaseDate());
    releaseDates.append(mEntitlement->contentConfiguration()->downloadStartDate());
    releaseDates.append(mEntitlement->contentConfiguration()->useEndDate());

    for(QList<QDateTime>::ConstIterator it = releaseDates.constBegin();
        it != releaseDates.constEnd();
        it++)
    {
        const QDateTime &releaseDate = *it;

        if (!releaseDate.isValid())
        {
            // Invalid. Qt gives nonsense results below if we don't bail now.
            continue;
        }

        // Work backwards from the release date to the reveal date
        const QDateTime revealDate = revealDateForReleaseDate(releaseDate);

        if (currentDateTime.daysTo(revealDate) > 200)
        {
            // Don't overflow - we need to convert to a signed int of milliseconds below
            continue;
        }

        // Figure out how long it will take until we can reveal this release date
        int secondsToReveal = currentDateTime.secsTo(revealDate);
        
        if (secondsToReveal >= 0)
        {
            // Use the closest release date
            secondsToNextDisplayDateReveal = qMin(secondsToReveal, secondsToNextDisplayDateReveal);
        }
    }

    if (secondsToNextDisplayDateReveal != INT_MAX)
    {
        if (!mDisplayDateRevealTimer)
        {
            mDisplayDateRevealTimer = new QTimer(this);
            mDisplayDateRevealTimer->setSingleShot(true);

            ORIGIN_VERIFY_CONNECT(mDisplayDateRevealTimer, SIGNAL(timeout()),
                this, SLOT(displayDateRevealTriggered()));
        }

        // Give us an extra second to make sure we're clear of the reveal time
        mDisplayDateRevealTimer->start((secondsToNextDisplayDateReveal + 1) * 1000);
    }
    else
    {
        delete mDisplayDateRevealTimer;
        mDisplayDateRevealTimer = NULL;
    }
}

void EntitlementProxy::playingGameTimeUpdated(const QString &masterTitleId, const QString &multiplayerId)
{
    if (masterTitleId == mEntitlement->contentConfiguration()->masterTitleId() &&
        multiplayerId == mEntitlement->contentConfiguration()->multiplayerId())
    {
        emit changed();
    }
}

bool EntitlementProxy::unownedContentAvailable()
{
    Origin::Engine::Content::BaseGameRef baseGame = qSharedPointerDynamicCast<Origin::Engine::Content::BaseGame>(mEntitlement);
    return baseGame && !baseGame->getPurchasableExtraContent().empty();
}

bool EntitlementProxy::newUnownedContentAvailable()
{
    return mEntitlement->hasNewUnownedChildContent();
}

QDateTime EntitlementProxy::latestChildPublishedDate()
{
    return mEntitlement->latestChildPublishedDate();
}

QString EntitlementProxy::masterTitleId()
{
    return mEntitlement->contentConfiguration()->masterTitleId();
}

QString EntitlementProxy::franchiseId()
{
    return mEntitlement->contentConfiguration()->franchiseId();
}

QDateTime EntitlementProxy::terminationDate()
{
    return mEntitlement->contentConfiguration()->terminationDate();
}

QVariant EntitlementProxy::sortOrderDescending()
{
    if(mEntitlement->contentConfiguration()->sortOrderDescending() == -1)
    {
        return QVariant();
    }
    else
    {
        return mEntitlement->contentConfiguration()->sortOrderDescending();
    }
}

QVariant EntitlementProxy::extraContentDisplayGroupInfo()
{
    const QString groupKey = mEntitlement->contentConfiguration()->extraContentDisplayGroup();
    if(groupKey.isEmpty())
    {
        // No group means drop this content in the default display group.
        return QVariant();
    }

    QVariantMap ret;
    ret["groupKey"] = groupKey;
    ret["displayName"] = mEntitlement->contentConfiguration()->extraContentDisplayGroupDisplayName();
    ret["sortOrderAscending"] = mEntitlement->contentConfiguration()->extraContentDisplayGroupSortOrderAscending();

    return ret;
}

QVariant EntitlementProxy::timeRemainingTilTerminationInSecs()
{
    if (mEntitlement->contentConfiguration()->isFreeTrial())
    {
        qint64 trialTimeRemaining = mEntitlement->localContent()->trialTimeRemaining();
        switch (trialTimeRemaining)
        {
        case 0:
            // We are in the last time slice, shown the true time.
            trialTimeRemaining = mEntitlement->localContent()->trialTimeRemaining(true);
            return trialTimeRemaining * .001f;
            break;
        case Engine::Content::LocalContent::TrialError_DisabledByAdmin:
            return -1;
            break;
        case Engine::Content::LocalContent::TrialError_LimitedTrialNotStarted:
            return QVariant();
            break;
        default:
            //mEntitlement->localContent()->trialTimeRemaining is in msecs;
            return trialTimeRemaining * .001f;
            break;
        }
    }

    return QVariant();
}

bool EntitlementProxy::owned()
{
    return mEntitlement->contentConfiguration()->owned();
}

bool EntitlementProxy::purchasable()
{
    return mEntitlement->contentConfiguration()->purchasable();
}

bool EntitlementProxy::previewContent()
{
    return mEntitlement->contentConfiguration()->previewContent();
}

QObjectList EntitlementProxy::associations()
{
    QObjectList associations;

    Content::EntitlementRef parent = mEntitlement->parent();

    if (!parent.isNull())
    {
        foreach (const QString &id, mEntitlement->contentConfiguration()->associatedProductIds())
        {
            Content::EntitlementRef eref = mEntitlement->contentController()->entitlementByProductId(id, Content::AllContent);
            if (!eref.isNull())
            {
                associations.push_back(EntitlementManager::instance()->proxyForEntitlement(eref));
            }
        }
    }

    return associations;
}

QStringList EntitlementProxy::thumbnailUrls()
{
    using namespace Engine::Content::ProductArt;
    return urlsForType(mEntitlement->contentConfiguration(), ThumbnailBoxArt);
}

bool EntitlementProxy::hasPricingData()
{
    return mEntitlement->contentConfiguration()->hasPricingData();
}

bool EntitlementProxy::freeProduct()
{
    return mEntitlement->contentConfiguration()->isFreeProduct();
}

QVariant EntitlementProxy::currentPrice()
{
    return mEntitlement->contentConfiguration()->currentPrice();
}

QVariant EntitlementProxy::originalPrice()
{
    return mEntitlement->contentConfiguration()->originalPrice();
}

QVariant EntitlementProxy::priceDescription()
{
    return mEntitlement->contentConfiguration()->priceDescription();
}

bool EntitlementProxy::freeTrial()
{
    return mEntitlement->contentConfiguration()->isFreeTrial();
}

QString EntitlementProxy::lastInstallFlowAction()
{
    Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
    return installFlow ? LocalizedContentString().operationTypeCode(installFlow->lastFlowAction().state) : QString();
}

qint64 EntitlementProxy::lastInstallFlowActionTotalDataSize()
{
    Downloader::IContentInstallFlow *installFlow = mEntitlement->localContent()->installFlow();
    return installFlow ? installFlow->lastFlowAction().totalBytes : 0;
}

}
}
}
