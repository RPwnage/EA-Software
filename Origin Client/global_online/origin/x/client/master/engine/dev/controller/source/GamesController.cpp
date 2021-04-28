//    Copyright � 2011-2012, Electronic Arts
//    All rights reserved.

#include <limits>
#include <QDateTime>
#include <QFile>

#if defined(ORIGIN_PC)
#include <ShlObj.h>
#endif

#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "engine/content/GamesController.h"
#include "services/rest/GamesServiceClient.h"
#include "services/rest/NonOriginGameServiceClient.h"
#include "services/settings/SettingsManager.h"
#include "TelemetryAPIDLL.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"

#include <QDesktopServices>

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            namespace
            {
                const QString seperator = "#//";
            }

            const QString GamesController::SHOW_ALL_TITLES = "show_all_titles";

            GamesController::GamesController(Origin::Engine::UserRef user) 
                : m_User(user)
            {
            }

            GamesController::~GamesController()
            {
            }

            GamesController * GamesController::create(Origin::Engine::UserRef user)
            {
                return new GamesController(user);
            }

            GamesController * GamesController::currentUserGamesController()
            {
                if (!Origin::Engine::LoginController::currentUser().isNull())
                    return Origin::Engine::LoginController::currentUser()->gamesControllerInstance();
                else
                    return NULL;
            }

            UserRef GamesController::user() const
            {
                return m_User.toStrongRef();
            }

// get functions
            void GamesController::getPlayedGameTime(const QString& masterTitleId, const QString &multiplayerId)
            {
                if(!masterTitleId.isEmpty())
                {
                    Services::GameUsageResponse *response = Services::GamesServiceClient::instance()->playedTime(user()->getSession(), masterTitleId, multiplayerId);
                    if (response)
                        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onGetPlayedGameTimeFinished()));
                }
                else
                {
                    ORIGIN_LOG_WARNING << "trying to call getPlayedTime with empty masterTitleId";
                }
            }

            void GamesController::getNonOriginPlayedGameTime(const QString appId)
            {
                if(!appId.isEmpty())
                {
                    Origin::Services::AppUsageResponse *response = Origin::Services::NonOriginGameServiceClient::usageTime(user()->getSession(), appId);
                    if (response)
                        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onGetNonOriginGameUsageFinished()));
                }
                else
                {
                    ORIGIN_LOG_WARNING << "trying to call getNonOriginPlayedGameTime with empty appId";
                }
            }          

            quint64 GamesController::getCachedTimePlayed(Content::EntitlementRef entitlement) 
            {
                const QString masterTitleId = entitlement->contentConfiguration()->masterTitleId();
                const QString multiplayerId = entitlement->contentConfiguration()->multiplayerId();
                if(mGameTimePlayedMap.contains(masterTitleId) && mGameTimePlayedMap[masterTitleId].contains(multiplayerId))
                {
                    return mGameTimePlayedMap[masterTitleId][multiplayerId];
                }
                return 0;
            }

            QDateTime GamesController::getCachedLastTimePlayed(Content::EntitlementRef entitlement)
            {
                const QString masterTitleId = entitlement->contentConfiguration()->masterTitleId();
                const QString multiplayerId = entitlement->contentConfiguration()->multiplayerId();
                if(mGameLastTimePlayedMap.contains(masterTitleId) && mGameLastTimePlayedMap[masterTitleId].contains(multiplayerId))
                {
                    return mGameLastTimePlayedMap[masterTitleId][multiplayerId];
                }
                return QDateTime();
            }


// post functions
            void GamesController::postPlayingGame(Content::EntitlementRef entitlement)
            {
                const QString &masterTitleId = entitlement->contentConfiguration()->masterTitleId();
                const QString &multiplayerId = entitlement->contentConfiguration()->multiplayerId();

                if(!masterTitleId.isEmpty())
                {
                    Services::GameStartedResponse *response = Services::GamesServiceClient::instance()->playing(user()->getSession(), masterTitleId, multiplayerId);
                    if (response)
                        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onPostPlayingFinished()));
                }
                else
                {
                    ORIGIN_LOG_WARNING << "trying to call postPlayingGame with empty masterTitleId";
                }

            }

            void GamesController::postPlayingNonOriginGame(Content::EntitlementRef entitlement)
            {
                QString appId = entitlement->contentConfiguration()->productId();

                if(!appId.isEmpty())
                {
                    Origin::Services::AppStartedResponse* response = Origin::Services::NonOriginGameServiceClient::playing(user()->getSession(), appId);
                    if (response)
                        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(processPlayingNonOriginGameResponse()));
                }
                else
                {
                    ORIGIN_LOG_WARNING << "trying to call postPlayingNonOriginGame with empty appId";
                }

            }

            void GamesController::postFinishedPlayingGame(Content::EntitlementRef entitlement)
            {
                const QString &masterTitleId = entitlement->contentConfiguration()->masterTitleId();
                const QString &multiplayerId = entitlement->contentConfiguration()->multiplayerId();

                UserRef userRef = user();
                if(userRef.isNull())
                {
                    ORIGIN_LOG_WARNING << "trying to call postFinishedPlayingGame with an invalid user";
                }
                else if(!masterTitleId.isEmpty())
                {
                    Services::GameFinishedResponse *response = Services::GamesServiceClient::instance()->finishedPlaying(userRef->getSession(), masterTitleId, multiplayerId);
                    if (response)
                        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onPostFinishedPlayingFinished()));
                }
                else
                {
                    ORIGIN_LOG_WARNING << "trying to call postFinishedPlayingGame with empty masterTitleId";
                }

            }

            void GamesController::postFinishedPlayingNonOriginGame(Content::EntitlementRef entitlement)
            {
                QString appId = entitlement->contentConfiguration()->productId();

                if(!appId.isEmpty())
                {
                    Origin::Services::AppFinishedResponse *response = Origin::Services::NonOriginGameServiceClient::finishedPlaying(user()->getSession(), appId);
                    if (response)
                        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(processFinishedPlayingNonOriginGameResponse()));
                }
                else
                {
                    ORIGIN_LOG_WARNING << "trying to call postFinishedPlayingNonOriginGame with empty appId";
                }

            }

// get slots
            void GamesController::onGetPlayedGameTimeFinished()
            {
                Origin::Services::GameUsageResponse* response = dynamic_cast<Origin::Services::GameUsageResponse*>(sender());
                ORIGIN_ASSERT(response);

                const Services::GamePlayedStats& gps = response->gameStats();
                if(response->error() == Origin::Services::restErrorSuccess)
                {
                    mGameTimePlayedMap[gps.gameId][gps.multiplayerId] = gps.total;
                    mGameLastTimePlayedMap[gps.gameId][gps.multiplayerId] = gps.lastSessionEndTimeStamp;
                    emit playingGameTimeUpdateCompleted(gps.gameId, gps.multiplayerId);
                }
                else
                {
                    emit playingGameTimeUpdateFailed(gps.gameId, gps.multiplayerId);
                }

                response->deleteLater();

            }

            void GamesController::onGetNonOriginGameUsageFinished()
            {
                Origin::Services::AppUsageResponse* response = dynamic_cast<Origin::Services::AppUsageResponse*>(sender());
                ORIGIN_ASSERT(response);

                const Services::AppUsedStats& aus = response->appStats();
                if(response->error() == Origin::Services::restErrorSuccess)
                {
                    mGameTimePlayedMap[aus.appId][""] = aus.total;
                    mGameLastTimePlayedMap[aus.appId][""] = aus.lastSessionEndTimeStamp;
                    emit playingGameTimeUpdateCompleted(aus.appId, "");
                }
                else
                {
                    emit playingGameTimeUpdateFailed(aus.appId, "");
                }

                response->deleteLater();
            }

// post slots
            void GamesController::onPostPlayingFinished()
            {
                Origin::Services::GameStartedResponse* response = dynamic_cast<Origin::Services::GameStartedResponse*>(sender());
                ORIGIN_ASSERT(response);

                if(response->error() != Origin::Services::restErrorSuccess)
                {
                    ORIGIN_LOG_DEBUG << "unable to post playing in game service";
                }

                response->deleteLater();
            }

            void GamesController::processPlayingNonOriginGameResponse()
            {
                Origin::Services::AppStartedResponse* response = dynamic_cast<Origin::Services::AppStartedResponse*>(sender());
                ORIGIN_ASSERT(response);

                if(response->error() != Origin::Services::restErrorSuccess)
                {
                    ORIGIN_LOG_DEBUG << "unable to post playing in non Origin game service";
                }

                response->deleteLater();
            }

            void GamesController::onPostFinishedPlayingFinished()
            {
                Origin::Services::GameFinishedResponse* response = dynamic_cast<Origin::Services::GameFinishedResponse*>(sender());
                ORIGIN_ASSERT(response);

                const Services::GamePlayedStats& gps = response->gameStats();
                if(response->error() == Origin::Services::restErrorSuccess)
                {
                    mGameTimePlayedMap[gps.gameId][gps.multiplayerId] = gps.total;
                    mGameLastTimePlayedMap[gps.gameId][gps.multiplayerId] = gps.lastSessionEndTimeStamp;
                    emit playingGameTimeUpdateCompleted(gps.gameId, gps.multiplayerId);
                }
                else
                {
                    ORIGIN_LOG_DEBUG << "unable to post playing in game service";
                    emit playingGameTimeUpdateFailed(gps.gameId, gps.multiplayerId);
                }

                response->deleteLater();
            }

            void GamesController::processFinishedPlayingNonOriginGameResponse()
            {
                Origin::Services::AppFinishedResponse* response = dynamic_cast<Origin::Services::AppFinishedResponse*>(sender());
                ORIGIN_ASSERT(response);

                const Services::AppUsedStats& aus = response->appStats();
                if(response->error() == Origin::Services::restErrorSuccess)
                {
                    mGameTimePlayedMap[aus.appId][""] = aus.total;
                    mGameLastTimePlayedMap[aus.appId][""] = aus.lastSessionEndTimeStamp;
                    emit playingGameTimeUpdateCompleted(aus.appId, "");
                }
                else
                {
                    ORIGIN_LOG_DEBUG << "unable to post playing in game service";
                    emit playingGameTimeUpdateFailed(aus.appId, "");
                }

                response->deleteLater();
            }

            QString GamesController::buildSaveDataPath()
            {
#if defined(ORIGIN_PC)
                WCHAR buffer[MAX_PATH]; 
                SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, buffer);
                const QString basePath = QString::fromWCharArray(buffer) + "\\Origin\\SocialContent";
#else
                const QString basePath = Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DataLocation) % "/SocialContent";
#endif
                QDir dir(basePath);

                if(!dir.exists())
                {
                    dir.mkpath(basePath);  
                }

                QString nucleusId = Origin::Services::Session::SessionService::nucleusUser(user()->getSession());

                //we don't want to use the actual nucleusId as the file name, so we create a hash
                return basePath + "\\" + QString::number(qHash(nucleusId)) + ".dat";

            }
            QString GamesController::readGameSettingFromUserSetting(const QString& userSettingVal, const QString& productId)
            {
                QStringList setting = (userSettingVal.split(seperator)).filter(productId);
                // There should only be one match. Get the title. (e.g. 71067_Battlefield 3 - Single Player)
                return setting.isEmpty() ? "" : setting[0].remove(productId + "_");
            }

            void GamesController::formatGameSettingToUserSetting(QString& userSetting, const QString& settingValue, const QString& productId)
            {
                // Check to see if there is already a preferred title for that product id.
                const QStringList setting = (userSetting.split(seperator)).filter(productId);
                // There was a setting - remove it. It's intentional that this will happen if the user passes in "".
                if(setting.isEmpty() == false)
                {
                    // There should only be one.
                    userSetting.remove(seperator + setting[0]);
                }
                if(settingValue != "")
                {
                    // Add the new one.
                    userSetting.append(seperator + productId + "_" + settingValue);
                }
            }

            QString GamesController::gameMultiLaunchDefault(const QString& productId)
            { 
                using namespace Services;
                QString preferredTitles = readSetting(SETTING_MULTILAUNCHPRODUCTIDSANDTITLES, user()->getSession());
                return readGameSettingFromUserSetting(preferredTitles, productId);
            }

            void GamesController::setGameMultiLaunchDefault(const QString& productId, const QString& title)
            {
                using namespace Services;
                // Get the list of our preferred titles.
                QString preferredTitles = readSetting(SETTING_MULTILAUNCHPRODUCTIDSANDTITLES, user()->getSession());
                formatGameSettingToUserSetting(preferredTitles, title, productId);
                writeSetting(SETTING_MULTILAUNCHPRODUCTIDSANDTITLES, preferredTitles, user()->getSession());
            }

            bool GamesController::gameHasOIGManuallyDisabled(const QString& productId)
            {
                using namespace Services;
                QString oigDisabledGames = readSetting(SETTING_OIGDISABLEDGAMES, user()->getSession());
                return readGameSettingFromUserSetting(oigDisabledGames, productId) != "";
            }

            void GamesController::setGameHasOIGManuallyDisabled(const QString& productId, const bool& disabled)
            {
                using namespace Services;
                // Get the list of our preferred titles.
                QString userSetting = readSetting(SETTING_OIGDISABLEDGAMES, user()->getSession());
                formatGameSettingToUserSetting(userSetting, disabled ? "t" : "", productId);
                writeSetting(SETTING_OIGDISABLEDGAMES, userSetting);
            }

            QString GamesController::firstLaunchMessageShown(const QString& productId)
            {
                using namespace Services;
                QString firstLaunchMessageId = readSetting(SETTING_FIRSTLAUNCHMESSAGESHOWN, user()->getSession());
                return readGameSettingFromUserSetting(firstLaunchMessageId, productId);
            }

            void GamesController::setFirstLaunchMessageShown(const QString& productId, const QString& arg)
            {
                using namespace Services;
                QString userSetting = readSetting(SETTING_FIRSTLAUNCHMESSAGESHOWN, user()->getSession());
                formatGameSettingToUserSetting(userSetting, arg, productId);
                writeSetting(SETTING_FIRSTLAUNCHMESSAGESHOWN, userSetting, user()->getSession());
            }

            bool GamesController::upgradedSaveGameWarningShown(const QString& productId)
            {
                using namespace Services;
                QString warnedGames = readSetting(SETTING_UPGRADEDSAVEGAMESWARNED, user()->getSession());
                return readGameSettingFromUserSetting(warnedGames, productId) != "";
            }

            void GamesController::setUpgradedSaveGameWarningShown(const QString& productId, const bool& arg)
            {
                using namespace Services;
                // Get the list of our preferred titles.
                QString userSetting = readSetting(SETTING_UPGRADEDSAVEGAMESWARNED, user()->getSession());
                formatGameSettingToUserSetting(userSetting, arg ? "t" : "", productId);
                writeSetting(SETTING_UPGRADEDSAVEGAMESWARNED, userSetting);
            }

            QString GamesController::gameCommandLineArguments(const QString& productId)
            {
                using namespace Services;
                QString gameCommandLineArgs = readSetting(SETTING_GAMECOMMANDLINEARGUMENTS, user()->getSession());
                return readGameSettingFromUserSetting(gameCommandLineArgs, productId);
            }

            void GamesController::setGameCommandLineArguments(const QString& productId, const QString& arg)
            {
                using namespace Services;
                // Get the list of our preferred titles.
                QString userSetting = readSetting(SETTING_GAMECOMMANDLINEARGUMENTS, user()->getSession());
                formatGameSettingToUserSetting(userSetting, arg, productId);
                writeSetting(SETTING_GAMECOMMANDLINEARGUMENTS, userSetting);
            }

            bool GamesController::gameIsHidden(Content::EntitlementRef entitlement)
            { 
                using namespace Services;

                QString offerId = entitlement->contentConfiguration()->productId();
                QStringList hiddenSettings = readSetting(SETTING_HiddenProductIds, user()->getSession()).toStringList();

                // 9.4 - TODO: Remove migration code below
                // This is a 9.3 migration step from 9.2 and prior.  If the game is hidden by contentId, update it to be hidden by product id
                if(!entitlement->contentConfiguration()->isNonOriginGame())
                {
                    QString contentId = entitlement->contentConfiguration()->contentId();
                    if(!contentId.isEmpty() && contentId != offerId && hiddenSettings.contains(contentId))
                    {
                        ORIGIN_LOG_EVENT << "Migrating hidden setting from content id [" << contentId << "] to product id [" << offerId << "]";
                        hiddenSettings.removeAll(contentId);
                        hiddenSettings.append(offerId);
                        writeSetting(SETTING_HiddenProductIds, hiddenSettings, user()->getSession());
                    }
                }

                return hiddenSettings.contains(offerId);
            }

            void GamesController::setGameIsHidden(Content::EntitlementRef entitlement, bool hidden)
            {
                using namespace Services;

                if (gameIsHidden(entitlement) != hidden)
                {
                    QString offerId = entitlement->contentConfiguration()->productId();
                    QStringList hiddenSettings = readSetting(SETTING_HiddenProductIds, user()->getSession()).toStringList();

                    if (hidden)
                    {
                        GetTelemetryInterface()->Metric_ENTITLEMENT_HIDE(offerId.toUtf8().data());
                        hiddenSettings.append(offerId);
                    }
                    else
                    {
                        GetTelemetryInterface()->Metric_ENTITLEMENT_UNHIDE(offerId.toUtf8().data());
                        hiddenSettings.removeAll(offerId);
                    }

                    writeSetting(SETTING_HiddenProductIds, hiddenSettings, user()->getSession());
                }
            }

            bool GamesController::gameIsFavorite(Content::EntitlementRef entitlement)
            {
                using namespace Services;

                QString offerId = entitlement->contentConfiguration()->productId();
                QStringList favoriteSettings = readSetting(SETTING_FavoriteProductIds, user()->getSession()).toStringList();

                // 9.4 - TODO: Remove migration code below
                // This is a 9.3 migration step from 9.2 and prior.  If the game is hidden by contentId, update it to be hidden by product id
                if(!entitlement->contentConfiguration()->isNonOriginGame())
                {
                    QString contentId = entitlement->contentConfiguration()->contentId();
                    if(!contentId.isEmpty() && contentId != offerId && favoriteSettings.contains(contentId))
                    {
                        ORIGIN_LOG_EVENT << "Migrating favorite setting from content id [" << contentId << "] to product id [" << offerId << "]";
                        favoriteSettings.removeAll(contentId);
                        favoriteSettings.append(offerId);
                        writeSetting(SETTING_FavoriteProductIds, favoriteSettings, user()->getSession());
                    }
                }

                return favoriteSettings.contains(offerId);
            }

            void GamesController::setGameIsFavorite(Content::EntitlementRef entitlement, bool favorite)
            {
                using namespace Services;

                if (gameIsFavorite(entitlement) != favorite)
                {
                    QString offerId = entitlement->contentConfiguration()->productId();

                    QStringList currentlyFavorite(readSetting(SETTING_FavoriteProductIds, user()->getSession()).toStringList());
                    if (favorite)
                    {
                        GetTelemetryInterface()->Metric_ENTITLEMENT_FAVORITE(offerId.toUtf8().data());
                        currentlyFavorite.append(offerId);
                    }
                    else
                    {
                        GetTelemetryInterface()->Metric_ENTITLEMENT_UNFAVORITE(offerId.toUtf8().data());
                        currentlyFavorite.removeAll(offerId);
                    }

                    writeSetting(SETTING_FavoriteProductIds, currentlyFavorite, user()->getSession());
                }
            }
        } //namespace Content
    } //namespace Engine
}//namespace Origin

