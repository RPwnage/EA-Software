///////////////////////////////////////////////////////////////////////////////
/// \file       SubscriptionManager.cpp
/// This file contains the implementation of the SubscriptionManager.
///
///	\author     Hans van Veenendaal
/// \date       2010-2015
/// \copyright  Copyright (c) 2014 Electronic Arts. All rights reserved.

#include <memory>
#include <QMetaType>
#include <QCryptographicHash>

#include "engine/content/ContentController.h"
#include "engine/content/NucleusEntitlementController.h"
#include "engine/subscription/SubscriptionManager.h"
#include "engine/subscription/OOALicenseManager.h"
#include "engine/login/LoginController.h"
#include "services/platform/TrustedClock.h"
#include "services/config/OriginConfigService.h"
#include "services/subscription/SubscriptionServiceClient.h"
#include "services/subscription/SubscriptionVaultServiceClient.h"
#include "services/subscription/SubscriptionTrialEligibilityServiceClient.h"
#include "services/subscription/SubscriptionTrialEligibilityServiceResponse.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "engine/dirtybits/DirtyBitsClient.h"
#include "engine/login/LoginController.h"
#include "TelemetryAPIDLL.h"

#ifdef _WIN32
#include <shlobj.h>
#include <shobjidl.h>
#endif

#include "NodeDocument.h"
#include "serverReader.h"
#include "serverWriter.h"
#include "services/common/XmlUtil.h"
#include "services/crypto/SimpleEncryption.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "services/rest/StoreServiceClient.h"
#include "ContentController.h"


#include "server.h"
#include "serverReader.h"
#include "NodeDocument.h"

const int SUBSCRIPTION_OFFLINE_PLAY_TICK = 300000;
qint64 SUBSCRIPTION_MAX_OFFLINE_PLAY_TIME = 30 * 24 * 60 * 60 * 1000ll;

using namespace Origin::Services::Session;

namespace Origin
{
    namespace Engine
    {
        namespace Subscription
        {
            static const int REFRESH_TIMEOUT_MSEC = 30000;

            SubscriptionManager * SubscriptionManager::gInstance = NULL;

            bool SubscriptionManager::enabled() const
            {
                return mEnabled;
            }

            QDateTime SubscriptionManager::firstSignupDate() const
            {
                return QDateTime::fromString(mSubscriptionBaseInfo.firstSignUpDate, Qt::ISODate);
            }

            bool SubscriptionManager::userHasSubscription() const
            {
#ifdef EA_PLATFORM_WINDOWS
                return !mSubscriptionInfo.Subscription.subscriptionUri.isEmpty();
#else
				return false;
#endif
            }

            bool SubscriptionManager::isActive() const
            {
                if(enabled() && userHasSubscription() && (offlinePlayRemaining()>0 || timeRemaining()>0))
                {
                    return status() == server::SUBSCRIPTIONSTATUS_ENABLED ||
                           status() == server::SUBSCRIPTIONSTATUS_PENDINGEXPIRED;
                }

                return false;
            }

            bool SubscriptionManager::isFreeTrial() const
            {
                return mSubscriptionInfo.Subscription.freeTrial;
            }

            QDateTime SubscriptionManager::startDate() const
            {
                QDateTime ret = QDateTime::fromString(mSubscriptionInfo.Subscription.subsStart, Qt::ISODate);
                ret.setTimeSpec(Qt::UTC);
                return ret;
            }

            server::SubscriptionStatusT SubscriptionManager::status() const
            {
                //const int debug = Services::readSetting(Services::SETTING_SubscriptionExpirationReason);
                //if(debug >= 0)
                //{
                //    return static_cast<server::SubscriptionStatusT>(debug);
                //}
                return mSubscriptionInfo.Subscription.status;
            }

            QDateTime SubscriptionManager::expiration() const
            {
                //const QString overrideExpirationDateSetting = Services::readSetting(Services::SETTING_SubscriptionExpirationDate);
                //if(!overrideExpirationDateSetting.isEmpty())
                //{
                //    const QString dateFormat = "yyyy-MM-dd hh:mm:ss.z";
                //    QDateTime parsedTime(QDateTime::fromString(overrideExpirationDateSetting, dateFormat));
                //    parsedTime.setTimeSpec(Qt::UTC);
                //    if(parsedTime.isValid())
                //        return parsedTime;
                //    else
                //    {
                //        ORIGIN_LOG_ERROR << "Subscription date override error - (" << overrideExpirationDateSetting << "); " <<
                //                         "Please check that the date is in this format (UTC TIME)--> " << dateFormat;
                //    }
                //}

                if(mSubscriptionInfo.Subscription.status == server::SUBSCRIPTIONSTATUS_ENABLED)
                {
                    foreach(const server::ScheduleOperationT &operation, mSubscriptionInfo.GetScheduleOperationResponse.ScheduleOperation)
                    {
                        if(operation.operationName == "RENEWED")
                        {
                            QDateTime ret = QDateTime::fromString(operation.scheduleDate, Qt::ISODate);
                            ret.setTimeSpec(Qt::UTC);
                            return ret;
                        }
                    }
                }

                if(mSubscriptionInfo.Subscription.status == server::SUBSCRIPTIONSTATUS_PENDINGEXPIRED)
                {
                    foreach(const server::ScheduleOperationT &operation, mSubscriptionInfo.GetScheduleOperationResponse.ScheduleOperation)
                    {
                        if(operation.operationName == "CANCELLED")
                        {
                            QDateTime ret = QDateTime::fromString(operation.scheduleDate, Qt::ISODate);
                            ret.setTimeSpec(Qt::UTC);
                            return ret;
                        }
                    }
                }
                
                return QDateTime::fromString(mSubscriptionInfo.Subscription.subsEnd, Qt::ISODate).addDays(1);
            }

            QDateTime SubscriptionManager::nextBillingDate() const
            {
                if(mSubscriptionInfo.Subscription.status == server::SUBSCRIPTIONSTATUS_ENABLED)
                {
                    for(std::size_t i = 0; i < mSubscriptionInfo.GetScheduleOperationResponse.ScheduleOperation.size(); i++)
                    {
                        if(mSubscriptionInfo.GetScheduleOperationResponse.ScheduleOperation[i].operationName == "RENEWED")
                        {
                            return QDateTime::fromString(mSubscriptionInfo.GetScheduleOperationResponse.ScheduleOperation[i].scheduleDate, Qt::ISODate);
                        }
                    }

                    // Fallback for when scheduled operation cannot be read properly. 
                    return QDateTime::fromString(mSubscriptionInfo.Subscription.subsEnd).addDays(1);
                }

                return QDateTime();
            }

            long SubscriptionManager::timeRemaining() const
            {
                if(enabled() && userHasSubscription())
                {
                    long remainingTime = Origin::Services::TrustedClock::instance()->nowUTC().secsTo(expiration());

                    return remainingTime < 0 ? 0 : remainingTime;
                }

                return -1;
            }

            long SubscriptionManager::offlinePlayRemaining() const
            {
                const qint64 now = Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch();

                long remaining = 0;

                if(Origin::Services::TrustedClock::instance()->isInitialized())
                {
                    Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
                    bool online = Services::Connection::ConnectionStatesService::isUserOnline(user->getSession());

                    if(online)
                    {
                        mLastKnownGoodTime = now;
                        mLastSampleTime = now;
                        mGoodTimeOffset = 0;
                    }

                    qint64 remaining1 = SUBSCRIPTION_MAX_OFFLINE_PLAY_TIME - (now - mLastKnownGoodTime);
                    qint64 remaining2 = expiration().toMSecsSinceEpoch() - now;

                    remaining = remaining1 < remaining2 ? remaining1 / 1000 : remaining2 / 1000;
                }
                else
                {
                    qint64 remaining1 = SUBSCRIPTION_MAX_OFFLINE_PLAY_TIME - (mGoodTimeOffset + (now - mLastSampleTime));
                    qint64 remaining2 = expiration().toMSecsSinceEpoch() - (mLastKnownGoodTime + mGoodTimeOffset + (now - mLastSampleTime));

                    remaining = remaining1 < remaining2 ? remaining1 / 1000 : remaining2 / 1000;
                }

                ORIGIN_LOG_DEBUG_IF(remaining < 0) << "Offline play has ran out! Go back online to extend offline play.";

                return remaining;
            }

            qint64 SubscriptionManager::lastKnownGoodTime() const
            {
                return mLastKnownGoodTime;
            }

            SubscriptionUpgradeEnum SubscriptionManager::isUpgradeAvailable(const QString &masterTitleId, const int rank) const
            {
                QString debug = Services::readSetting(Services::SETTING_SubscriptionEntitlementMasterIdUpdateAvaliable);
                if(QString::compare(debug, masterTitleId, Qt::CaseInsensitive) == 0)
                {
                    return UPGRADEAVAILABLE_BASEGAME_AND_DLC;
                }

                if (mPendingUpgrades.contains(masterTitleId))
                {
                    return UPGRADEAVAILABLE_NONE;
                }

                bool bBaseGameUpgradeAvailable = false;
                bool bDLCUpgradeAvailable = false;

                if(isActive())
                {
                    server::gameT *game = mMasterTitleToGame[masterTitleId];

                    if(game)
                    {
                        Origin::Engine::Content::ContentController * contentController = Origin::Engine::Content::ContentController::currentUserContentController();
                        if(!contentController) return UPGRADEAVAILABLE_NONE;

                        // Using the 'raw' nucleus entitlements to check as certain entitlements are filtered out as game only entitlements.
                        Origin::Engine::Content::NucleusEntitlementController *entitlementController = contentController->nucleusEntitlementController();
                        if(!entitlementController) return UPGRADEAVAILABLE_NONE;

                        Services::Publishing::NucleusEntitlementList list = entitlementController->entitlements();
                        
                        // Check whether the user owns the subscription base game.
                        // If the game we want to upgrade to is of lower rank than the game we have --> don't upgrade.
                        if (game->rank == -1 || rank == -1 || rank < game->rank)
                        {
                            for (unsigned int b = 0; b < game->basegames.basegame.size(); ++b)
                            {
                                bool found = false;
                                foreach(Origin::Services::Publishing::NucleusEntitlementRef e, list)
                                {
                                    // Make sure we are not looking at 'dead' entitlements
                                    QString offerId = e->productId();

                                    if (offerId == game->basegames.basegame[b])
                                    {
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found)
                                {
                                    bBaseGameUpgradeAvailable = true;
                                    ORIGIN_LOG_EVENT << "For " << game->displayName << " Offer " << game->basegames.basegame[b] << " Not Found.";
                                }
                            }
                        }

                        // Check whether the user has all the DLC/ULC.
                        for(unsigned int c = 0; c < game->extracontents.extracontent.size(); ++c)
                        {
                            bool found = false;
                            foreach(Origin::Services::Publishing::NucleusEntitlementRef e, list)
                            {
                                QString offerId = e->productId();
                                if(offerId == game->extracontents.extracontent[c])
                                {
                                    found = true;
                                    break;
                                }
                            }
                            if(!found)
                            {
                                bDLCUpgradeAvailable = true;
                                ORIGIN_LOG_EVENT << "For " << game->displayName << " Expansion Offer: " << game->extracontents.extracontent[c] << " Not Found.";
                            }
                        }

                        if(bBaseGameUpgradeAvailable && bDLCUpgradeAvailable)
                        {
                            return UPGRADEAVAILABLE_BASEGAME_AND_DLC;
                        }
                        else if(bBaseGameUpgradeAvailable)
                        {
                            return UPGRADEAVAILABLE_BASEGAME_ONLY;
                        }
                        else if(bDLCUpgradeAvailable)
                        {
                            return UPGRADEAVAILABLE_DLC_ONLY;
                        }
                    }
                }
                return UPGRADEAVAILABLE_NONE;
            }

            void SubscriptionManager::upgrade(const QString & masterTitleId)
            {
                const bool hasSubscription = userHasSubscription();

                if(hasSubscription && isUpgradeAvailable(masterTitleId, 0))
                {
                    server::gameT *game = mMasterTitleToGame[masterTitleId];
                    if(game)
                    {
                        Services::RedeemSubscriptionOfferResponse *response;

                        response = Services::StoreServiceClient::instance()->redeemSubscriptionOffer(subscriptionId(), game->offerId, masterTitleId);
                        response->setDeleteOnFinish(true);

                        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onUpgradeResponse()));
                    }
                    else
                    {
                        emit upgradeFailed(masterTitleId, RedemptionErrorUnknownGame);
                        sendUpgradeTelemetry(false, masterTitleId, "upgrade - no entitlement");
                    }
                }
                else
                {
                    emit upgradeFailed(masterTitleId, hasSubscription ? RedemptionErrorNoUpgrade : RedemptionErrorNoSubscription);
                    sendUpgradeTelemetry(false, masterTitleId, "upgrade - upgrade not available or no subs");
                }
            }

            void SubscriptionManager::downgrade(const QString & masterTitleId)
            {
                if(subscriptionId() != "")
                {
                    server::gameT *game = mMasterTitleToGame[masterTitleId];
                    if(game)
                    {
                        Services::RedeemSubscriptionOfferResponse *response;

                        response = Services::StoreServiceClient::instance()->revokeSubscriptionOffer(subscriptionId(), game->offerId, masterTitleId);
                        response->setDeleteOnFinish(true);

                        ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onDowngradeResponse()));
                        sendRemoveStartTelemetry(true, masterTitleId, "downgrade - success");
                    }
                    else
                    {
                        emit downgradeFailed(masterTitleId, RedemptionErrorUnknownGame);
                        sendRemoveStartTelemetry(false, masterTitleId, "downgrade - game not in the subscription");
                    }
                }
                else
                {
                    emit downgradeFailed(masterTitleId, RedemptionErrorNoSubscription);
                    sendRemoveStartTelemetry(false, masterTitleId, "downgrade - no subscription");
                }
            }
            bool SubscriptionManager::isAvailableFromSubscription(const QString &offerId) const
            {
                const QString debug = Services::readSetting(Services::SETTING_SubscriptionEntitlementOfferId);
                if(QString::compare(debug, offerId, Qt::CaseInsensitive) == 0)
                {
                    return true;
                }
                // Check whether the offer is in the subscription.
                for(std::vector<server::gameT>::const_iterator g = mSubscriptionVaultInfo.vaultGameInfo.begin(); g != mSubscriptionVaultInfo.vaultGameInfo.end(); ++g)
                {
                    for(std::vector<QString>::const_iterator b = g->basegames.basegame.begin(); b != g->basegames.basegame.end(); ++b)
                    {
                        if(*b == offerId)
                            return true;
                    }
                    for(std::vector<QString>::const_iterator e = g->extracontents.extracontent.begin(); e != g->extracontents.extracontent.end(); ++e)
                    {
                        if(*e == offerId)
                            return true;
                    }
                }

                return false;
            }

            void SubscriptionManager::entitle(const QString &offerId)
            {
                const bool hasSubscription = userHasSubscription();

                if(hasSubscription && isAvailableFromSubscription(offerId))
                {
                    Services::RedeemSubscriptionOfferResponse *response;

                    response = Services::StoreServiceClient::instance()->redeemSubscriptionOffer(subscriptionId(), offerId, offerId);

                    ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onEntitleResponse()));
                }
                else
                {
                    emit entitleFailed(offerId, hasSubscription ? RedemptionErrorUnknownGame : RedemptionErrorNoSubscription);
                    sendUpgradeTelemetry(false, offerId, "entitle - entitle not available or no subs");
                }
            }

            void SubscriptionManager::revoke(const QString &offerId)
            {
                const bool hasSubscription = userHasSubscription();

                if(hasSubscription && isAvailableFromSubscription(offerId))
                {
                    Services::RedeemSubscriptionOfferResponse *response;

                    response = Services::StoreServiceClient::instance()->revokeSubscriptionOffer(subscriptionId(), offerId, offerId);

                    ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onRevokeResponse()));
                    sendRemoveStartTelemetry(true, offerId, "revoke");
                }
                else
                {
                    emit revokeFailed(offerId, hasSubscription ? RedemptionErrorUnknownGame : RedemptionErrorNoSubscription);
                    sendRemoveStartTelemetry(false, offerId, "revoke - no subscription");
                }
            }

            //////////////////////////////////////////////////////////////////////////


            void SubscriptionManager::initializeEventHandler()
            {
                Origin::Engine::DirtyBitsClient::instance()->registerHandler(this, "catalogChanged", "catalog");
                Origin::Engine::DirtyBitsClient::instance()->registerHandler(this, "subscriptionChanged", "subscription");
            }

            void SubscriptionManager::onLoggedIn(Origin::Engine::UserRef)
            {
                if(mEnabled)
                {
                    initializeEventHandler();

                    // Get the users subscription information.
                    serialize();

                    refreshSubscriptionInfo();

                    mOfflinePlayTickTimer.start(SUBSCRIPTION_OFFLINE_PLAY_TICK);

                    Content::ContentController *contentController;

                    contentController = Content::ContentController::currentUserContentController();
                    if(contentController)
                    {
                        ORIGIN_VERIFY_CONNECT_EX(contentController, &Content::ContentController::updateStarted,
                                                 this, &SubscriptionManager::onUpdateStarted, Qt::UniqueConnection);

                        ORIGIN_VERIFY_CONNECT_EX(contentController, &Content::ContentController::updateFinished,
                                                 this, &SubscriptionManager::onUpdateFinished, Qt::UniqueConnection);
                    }
                }
            }

            void SubscriptionManager::onLoggedOut(Origin::Engine::UserRef)
            {
                clear();
            }

            void SubscriptionManager::catalogChanged(QByteArray payload)
            {
                INodeDocument * doc = CreateJsonDocument();
                doc->Parse(payload.data());

                server::dirtybitVaultChangeNotificationT msg;

                if(server::Read(doc, msg))
                {
                    if(msg.data.msgType == "vaultGameChange")
                    {
                        refreshSubscriptionInfo();
                    }
                }
            }

            void SubscriptionManager::subscriptionChanged(QByteArray payload)
            {
                refreshSubscriptionInfo();
            }

            void SubscriptionManager::onEntitleResponse()
            {
                Services::RedeemSubscriptionOfferResponse *response;

                response = dynamic_cast<Services::RedeemSubscriptionOfferResponse *>(sender());
                if(response)
                {
                    if(response->httpStatus() == Origin::Services::Http200Ok)
                    {
                        // TODO: Do something with the returned entitlements.
                        const server::RedemptionResponseT &entitlementResponse = response->entilements();

                        if(entitlementResponse.entitlements.size() > 0)
                        {
                            mPendingEntitlements.insert(response->transactionId());
                            mRefreshTimeoutTimer.start(REFRESH_TIMEOUT_MSEC);
                            mRefreshRequested = false;
                        }
                    }
                    else if (response->httpStatus() == Origin::Services::Http302Found)
                    {
                        Services::RedeemSubscriptionOfferResponse * resp = Services::StoreServiceClient::instance()->redeemSubscriptionOfferByUrl(response);

                        if (resp)
                        {
                            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onEntitleResponse()));
                        }
                        else
                        {
                            emit entitleFailed(response->transactionId(), RedemptionErrorUnknown);
                            sendUpgradeTelemetry(false, resp->transactionId(), "error: " + resp->errorString());
                        }
                    }
                    else
                    {
                        SubscriptionRedemptionError error = parseRedemptionError(response->reply()->readAll());
                        emit entitleFailed(response->transactionId(), error);
                        sendUpgradeTelemetry(false, response->transactionId(), "error: " + response->errorString());
                    }

                    response->deleteLater();
                }
            }

            void SubscriptionManager::onRevokeResponse()
            {
                Services::RedeemSubscriptionOfferResponse *response;

                response = dynamic_cast<Services::RedeemSubscriptionOfferResponse *>(sender());
                if(response)
                {
                    if(response->httpStatus() != Origin::Services::Http200Ok)
                    {
                        SubscriptionRedemptionError error = parseRedemptionError(response->reply()->readAll());
                        emit revokeFailed(response->transactionId(), error);
                        sendRemoveFinishedTelemetry(false, response->transactionId(), "error: " + response->errorString());
                    }
                    else
                    {
                        emit entitlementRemoved(response->offerId());
                        sendRemoveFinishedTelemetry(true, response->transactionId(), "success");
                    }

                    response->deleteLater();
                }
            }

            void SubscriptionManager::onUpgradeResponse()
            {
                Services::RedeemSubscriptionOfferResponse *response;

                response = dynamic_cast<Services::RedeemSubscriptionOfferResponse *>(sender());
                if(response)
                {
                    if(response->httpStatus() == Origin::Services::Http200Ok)
                    {
                        // TODO: Do something with the returned entitlements.
                        const server::RedemptionResponseT &entitlementResponse = response->entilements();

                        if(entitlementResponse.entitlements.size() > 0)
                        {
                            mPendingUpgrades.insert(response->transactionId());
                            mRefreshTimeoutTimer.start(REFRESH_TIMEOUT_MSEC);
                            mRefreshRequested = false;
                        }
                    }
                    else if(response->httpStatus() == Origin::Services::Http302Found)
                    {
                        Services::RedeemSubscriptionOfferResponse * resp = Services::StoreServiceClient::instance()->redeemSubscriptionOfferByUrl(response);

                        if(resp)
                        {
                            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onUpgradeResponse()));
                        }
                        else
                        {
                            emit upgradeFailed(response->transactionId(), RedemptionErrorUnknown);
                            sendUpgradeTelemetry(false, response->transactionId(), "response 302 - no offer response");
                        }
                    }
                    else
                    {
                        SubscriptionRedemptionError error = parseRedemptionError(response->reply()->readAll());
                        emit upgradeFailed(response->transactionId(), error);
                        sendUpgradeTelemetry(false, response->transactionId(), "response - not acceptable");
                    }

                    response->deleteLater();
                }
            }

            void SubscriptionManager::onDowngradeResponse()
            {
                Services::RedeemSubscriptionOfferResponse *response;

                response = dynamic_cast<Services::RedeemSubscriptionOfferResponse *>(sender());
                if(response)
                {
                    if(response->httpStatus() == Origin::Services::Http302Found)
                    {
                        Services::RedeemSubscriptionOfferResponse * resp = Services::StoreServiceClient::instance()->revokeSubscriptionOfferByUrl(response);

                        if(resp)
                        {
                            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onDowngradeResponse()));
                        }
                        else
                        {
                            emit downgradeFailed(response->transactionId(), RedemptionErrorUnknown);
                            sendRemoveFinishedTelemetry(false, response->transactionId(), "response 302 - not acceptable");
                        }
                    }
                    else
                    {
                        if(response->httpStatus() != Origin::Services::Http200Ok)
                        {
                            SubscriptionRedemptionError error = parseRedemptionError(response->reply()->readAll());
                            emit downgradeFailed(response->transactionId(), error);
                            sendRemoveFinishedTelemetry(false, response->transactionId(), "response - not acceptable | " + response->errorString());
                        }
                        else
                        {
                            emit entitlementRemoved(response->offerId());
                            sendRemoveFinishedTelemetry(true, response->transactionId(), "success");
                        }
                    }

                    response->deleteLater();
                }
            }

            void SubscriptionManager::onRefreshTimeout()
            {
                Content::ContentController *contentController;

                contentController = Content::ContentController::currentUserContentController();
                if(!contentController)
                {
                    return;
                }

                auto iter = mPendingEntitlements.begin();
                while(iter != mPendingEntitlements.end())
                {
                    Content::EntitlementRef entitlement = contentController->entitlementByProductId(*iter);

                    if(entitlement)
                    {
                        if(entitlement->contentConfiguration()->downloadable())
                        {
                            Content::ContentController::DownloadPreflightingResult result;

                            contentController->doDownloadPreflighting(*entitlement->localContent(), result);
                            if(result >= Origin::Engine::Content::ContentController::CONTINUE_DOWNLOAD)
                            {
                                entitlement->localContent()->download("");
                            }
                        }

                        iter = mPendingEntitlements.erase(iter);
                    }
                    else
                    {
                        ++iter;
                    }
                }

                checkPendingUpgrades();

                if(mRefreshRequested)
                {
                    for(iter = mPendingEntitlements.begin(); iter != mPendingEntitlements.end(); ++iter)
                    {
                        emit entitleFailed(*iter, RedemptionErrorTimeout);
                        sendUpgradeTelemetry(false, *iter, "refresh time out");
                    }

                    for(iter = mPendingUpgrades.begin(); iter != mPendingUpgrades.end(); ++iter)
                    {
                        emit upgradeFailed(*iter, RedemptionErrorTimeout);
                        sendUpgradeTelemetry(false, *iter, "refresh time out");
                    }

                    mPendingEntitlements.clear();
                    mPendingUpgrades.clear();

                    mRefreshRequested = false;
                }
                else if(!mPendingEntitlements.isEmpty() || !mPendingUpgrades.isEmpty())
                {
                    contentController->baseGamesController()->refresh(Content::ContentUpdates);
                    mRefreshTimeoutTimer.start(REFRESH_TIMEOUT_MSEC);
                    mRefreshRequested = true;
                }
            }

            void SubscriptionManager::onOfflineTimeout()
            {
                Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
                bool online = Services::Connection::ConnectionStatesService::isUserOnline(user->getSession());

                mLastSampleTime = Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch();

                if(online)
                {
                    // Save out the subscription information.
                    if(Origin::Services::TrustedClock::instance()->isInitialized())
                    {
                        mLastKnownGoodTime = mLastSampleTime;
                        mGoodTimeOffset = 0;
                    }
                    else
                    {
                        mGoodTimeOffset += SUBSCRIPTION_OFFLINE_PLAY_TICK;
                    }

                }
                else
                {
                    mGoodTimeOffset += SUBSCRIPTION_OFFLINE_PLAY_TICK;
                }

                refreshLicenses(online, offlinePlayRemaining() > 0);

                serialize(true);
            }

            /// \brief Kick off a thread to check the licenses.
            void SubscriptionManager::refreshLicenses(bool bIsUserOnline, bool hasOfflineTimeRemaining)
            {
                QThread* thread = new QThread;
                OOARunner* worker = new OOARunner(bIsUserOnline, hasOfflineTimeRemaining);
                worker->moveToThread(thread);
                ORIGIN_VERIFY_CONNECT(thread, SIGNAL(started()), worker, SLOT(refreshLicenses()));
                ORIGIN_VERIFY_CONNECT(worker, SIGNAL(finished()), thread, SLOT(quit()));
                ORIGIN_VERIFY_CONNECT(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
                ORIGIN_VERIFY_CONNECT(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
                thread->start();
            }

            void OOARunner::refreshLicenses()
            {
                // renew the saved OOA licenses.  This needs to be done regardless of whether we
                // are online or not.  If we are not online and the offlinePlayRemaining time has
                // expired, the saved licenses will be deleted, which will make the games unplayable.
                // This isn't much protection: the user can simple make a copy of the license and
                // restore it from the saved copy.
                RefreshOOALicenses(m_bIsUserOnline, m_hasOfflineTimeRemaining);

                emit finished();
            }


            void SubscriptionManager::onUpdateStarted()
            {
                refreshSubscriptionInfo();
            }

            void SubscriptionManager::onUpdateFinished(const Content::EntitlementRefList &added, const Content::EntitlementRefList &removed)
            {
                Content::ContentController *contentController = dynamic_cast<Content::ContentController *>(sender());

                if(!contentController)
                {
                    return;
                }

                for(auto iter = added.begin(); iter != added.end(); ++iter)
                {
                    Content::EntitlementRef entitlement = *iter;
                    auto pendingEntitlement = mPendingEntitlements.find(entitlement->contentConfiguration()->productId());

                    if(pendingEntitlement != mPendingEntitlements.end())
                    {
                        if(entitlement->contentConfiguration()->downloadable())
                        {
                            Content::ContentController::DownloadPreflightingResult result;

                            contentController->doDownloadPreflighting(*entitlement->localContent(), result);
                            if(result >= Origin::Engine::Content::ContentController::CONTINUE_DOWNLOAD)
                            {
                                entitlement->localContent()->download("");
                            }
                        }

                        sendUpgradeTelemetry(true, entitlement->contentConfiguration()->productId(), "onUpdateFinished");
                        mPendingEntitlements.erase(pendingEntitlement);
                    }
                }

                checkPendingUpgrades();

                if(Origin::Services::TrustedClock::instance()->isInitialized())
                {
                    mLastKnownGoodTime = Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch();
                    mLastSampleTime = mLastKnownGoodTime;
                    mGoodTimeOffset = 0;

                    serialize(true);
                }

            }

            void SubscriptionManager::serialize(bool bSave)
            {
                static QByteArray salt("hfdkjhsdf79734233hjkljsd");

                if(!mEnabled)
                    return;

                Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                if(session.isNull())
                    return;

                if(bSave)
                {
                    QDomDocument doc;

                    // Put the storing code here.
                    QDomElement root = doc.createElement("subs");
                    doc.appendChild(root);

                    Origin::Util::XmlUtil::setInt(root, "version", SUBSCRIPTION_DOCUMENT_VERSION);
                    Origin::Util::XmlUtil::setLongLong(root, "id", mGoodTimeOffset/60/1000);
                    Origin::Util::XmlUtil::setLongLong(root, "t", mLastKnownGoodTime);

                    AddSubDocument(root, mSubscriptionBaseInfo);
                    AddSubDocument(root, mSubscriptionInfo);
                    AddSubDocument(root, mSubscriptionVaultInfo);

#ifdef _DEBUG
                    QFile file(cacheFolder() + "/s.xml");
                    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
                    {
                        file.write(doc.toByteArray());
                        file.close();
                    }
#endif

                    QFile encryptedFile(cacheFolder() + "/s.dat");
                    if(encryptedFile.open(QIODevice::WriteOnly))
                    {
                        QByteArray data = Origin::Services::Crypto::SimpleEncryption().encrypt(doc.toByteArray().data()).c_str();
                        QByteArray check = QCryptographicHash::hash(data + salt, QCryptographicHash::Sha1).toHex().toLower();

                        encryptedFile.write(data);
                        encryptedFile.write("O");
                        encryptedFile.write(check);
                        encryptedFile.close();

                        // Store the latest checksum in the users settings.
                        Origin::Services::writeSetting(Origin::Services::SETTING_SubscriptionLastModifiedKey, QString(check), Origin::Services::Session::SessionService::currentSession());
                    }
                }
                else
                {
                    QDomDocument doc;

                    QFile file(cacheFolder() + "/s.dat");
                    if(file.open(QIODevice::ReadOnly))
                    {
                        QByteArray data = file.readAll();
                        QList<QByteArray> items = data.split('O');

                        if(items.size() == 2)
                        {
                            QByteArray check = QCryptographicHash::hash(items[0] + salt, QCryptographicHash::Sha1).toHex().toLower();

                            QString check2 = Origin::Services::readSetting(Origin::Services::SETTING_SubscriptionLastModifiedKey, Origin::Services::Session::SessionService::currentSession()).toString();

                            if(check == items[1] && QString(check) == check2)
                            {
                                QByteArray decryptedData = Origin::Services::Crypto::SimpleEncryption().decrypt(items[0].data()).c_str();

                                doc.setContent(decryptedData);
                                file.close();

                                // Put the loading code here.
                                QDomElement root = doc.firstChildElement("subs");

                                int version = Origin::Util::XmlUtil::getInt(root, "version");

                                Q_ASSERT(SUBSCRIPTION_DOCUMENT_VERSION == version && "Subscription document version doesn't match. Add code to handle version differences.");

                                if(version <= SUBSCRIPTION_DOCUMENT_VERSION)
                                {
                                    mGoodTimeOffset = Origin::Util::XmlUtil::getLongLong(root, "id") * 60 * 1000;
                                    mLastKnownGoodTime = Origin::Util::XmlUtil::getLongLong(root, "t");
                                    mLastSampleTime = Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch();

                                    ReadSubDocument(root, "SubscriptionInfo", mSubscriptionBaseInfo);
                                    ReadSubDocument(root, "SubscriptionDetailInfo", mSubscriptionInfo);
                                    ReadSubDocument(root, "SubscriptionVaultInfo", mSubscriptionVaultInfo);
                                }
                            }
                        }
                    }
                }
            }



            SubscriptionManager::SubscriptionManager(QObject * owner)
                : QObject(owner)
                , mEnabled(Origin::Services::OriginConfigService::instance()->subscriptionConfig().enabled || Origin::Services::readSetting(Origin::Services::SETTING_SubscriptionEnabled))
                , mRefreshRequested(false)
                , mExpiredSubscriptionRequestRetry(false)
                , mIsTrialEligibile(-1)
                , mGoodTimeOffset(0)
                , mLastSampleTime(Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch())
                , mLastKnownGoodTime(0)
            {
                gInstance = this;

                ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)), this, SLOT(onLoggedIn(Origin::Engine::UserRef)));
                ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), this, SLOT(onLoggedOut(Origin::Engine::UserRef)));

                ORIGIN_VERIFY_CONNECT(&mRefreshTimeoutTimer, &QTimer::timeout, this, &SubscriptionManager::onRefreshTimeout);
                mRefreshTimeoutTimer.setSingleShot(true);

                ORIGIN_VERIFY_CONNECT(&mOfflinePlayTickTimer, &QTimer::timeout, this, &SubscriptionManager::onOfflineTimeout);
                // SUBS TODO: REMOVE THIS AND MAKE SUBSCRIPTION_MAX_OFFLINE_PLAY_TIME CONST AGAIN.
                const int debug = Services::readSetting(Services::SETTING_SubscriptionMaxOfflinePlay);
                if(debug >= 0)
                {
                    SUBSCRIPTION_MAX_OFFLINE_PLAY_TIME = debug;
                }
            }

            SubscriptionManager::~SubscriptionManager()
            {
                clear();

                if(gInstance == this)
                {
                    gInstance = NULL;
                }
            }

            void SubscriptionManager::clear()
            {
                // Clean up the user specific information.
                mMasterTitleToGame.clear();
                mSubscriptionInfo = server::SubscriptionDetailInfoT();
                mSubscriptionVaultInfo = server::SubscriptionVaultInfoT();

                mOfflinePlayTickTimer.stop();
            }


            void SubscriptionManager::init(QObject * owner)
            {
                if(gInstance == NULL)
                {
                    new SubscriptionManager(owner);
                }
            }

            void SubscriptionManager::shutdown()
            {
                if(gInstance)
                {
                    delete gInstance;
                }
            }

            SubscriptionManager * SubscriptionManager::instance()
            {
                return gInstance;
            }

            QString SubscriptionManager::cacheFolder() const
            {
                Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

                if(!session.isNull())
                {
                    QString environment = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName);
                    QString userId = Origin::Services::Session::SessionService::nucleusUser(session);
                    userId = QCryptographicHash::hash(userId.toLatin1(), QCryptographicHash::Sha1).toHex().toUpper();

                    // compute the path to the achievement cache, which consists of a platform dependent
                    // root directory, hard-coded application specific directories, environment directory
                    // (omitted in production), and a per-user directory containing the actual cache files.
                    QStringList path;
#if defined(ORIGIN_PC)
                    WCHAR buffer[MAX_PATH];
                    SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer);
                    path << QString::fromWCharArray(buffer) << "Origin";
#elif defined(ORIGIN_MAC)
                    path << Origin::Services::PlatformService::commonAppDataPath();
#else
#error Must specialize for other platform.
#endif

                    path << "Subscription";
                    if(environment != "production")
                        path << environment;

                    path << userId;

                    QString cachePath = path.join("/");

                    if(QDir(cachePath).exists() || QDir().mkpath(cachePath))
                        return cachePath;

                }
                return QString();
            }

            void SubscriptionManager::refreshSubscriptionInfo()
            {
                mExpiredSubscriptionRequestRetry = false;

                Origin::Services::Subscription::SubscriptionInfoResponse * response = Origin::Services::Subscription::SubscriptionServiceClient::subscriptionInfo("Origin Membership", "ENABLED,PENDINGEXPIRED");

                ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onSubscriptionInfoAvailable()));
            }

            void SubscriptionManager::refreshSubscriptionTrialEligibility()
            {
                Origin::Services::Subscription::SubscriptionTrialEligibilityServiceResponse* response = Origin::Services::Subscription::SubscriptionTrialEligibilityServiceClient::subscriptionTrialEligibilityInfo();
                ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onSubscriptionTrialEligibilityInfoAvailable()));
            }

            void SubscriptionManager::refreshVaultInfo()
            {
                Origin::Services::Subscription::SubscriptionVaultInfoResponse * response = Origin::Services::Subscription::SubscriptionVaultServiceClient::subscriptionVaultInfo("Origin Membership");

                ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onSubscriptionVaultInfoAvailable()));
            }

            void SubscriptionManager::onSubscriptionInfoAvailable()
            {
                Origin::Services::Subscription::SubscriptionInfoResponse * response = dynamic_cast<Origin::Services::Subscription::SubscriptionInfoResponse *>(sender());

                if(response)
                {
                    if(response->httpStatus() == Origin::Services::Http200Ok)
                    {
                        mSubscriptionBaseInfo = response->info();

                        if(mSubscriptionBaseInfo.subscriptionUri.size() > 0)
                        {
                            Origin::Services::Subscription::SubscriptionDetailInfoResponse * resp = Origin::Services::Subscription::SubscriptionServiceClient::subscriptionDetailInfo(mSubscriptionBaseInfo.subscriptionUri.back());

                            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onSubscriptionDetailInfoAvailable()));
                        }
                        else
                        {
                            if(!mExpiredSubscriptionRequestRetry && !mSubscriptionBaseInfo.firstSignUpDate.isEmpty())
                            {
                                mExpiredSubscriptionRequestRetry = true;

                                Origin::Services::Subscription::SubscriptionInfoResponse * response = Origin::Services::Subscription::SubscriptionServiceClient::subscriptionInfo("Origin Membership", "EXPIRED");

                                ORIGIN_VERIFY_CONNECT(response, SIGNAL(finished()), this, SLOT(onSubscriptionInfoAvailable()));
                            }
                            else
                            {
                                mExpiredSubscriptionRequestRetry = false;
                                GetTelemetryInterface()->setSubscriberStatus(TELEMETRY_SUBSCRIBER_STATUS_NON_SUBSCRIBER);
                            }
                        }
                    }
                    else
                    {
                        if(response->httpStatus() == Origin::Services::Http404ClientErrorNotFound)
                        {
                            GetTelemetryInterface()->setSubscriberStatus(TELEMETRY_SUBSCRIBER_STATUS_NON_SUBSCRIBER);
                        }
                        emit updateFailed();
                    }

                    response->deleteLater();
                }
            }

            void SubscriptionManager::onSubscriptionTrialEligibilityInfoAvailable()
            {
                Origin::Services::Subscription::SubscriptionTrialEligibilityServiceResponse* response = dynamic_cast<Origin::Services::Subscription::SubscriptionTrialEligibilityServiceResponse*>(sender());

                if(response)
                {
                    if(response->httpStatus() == Origin::Services::Http200Ok)
                    {
                        mIsTrialEligibile = response->isTrialEligible();
                        ORIGIN_LOG_ACTION << "Subscription trial status received. Status: " << QString::number(mIsTrialEligibile);
                    }
                    else
                    {
                        ORIGIN_LOG_ERROR << "Subscription trial response status error: " << response->errorString();
                        mIsTrialEligibile = 0;
                    }

                    emit trialEligibilityInfoReceived();
                    response->deleteLater();
                }
            }

            void SubscriptionManager::onSubscriptionDetailInfoAvailable()
            {
                Origin::Services::Subscription::SubscriptionDetailInfoResponse * response = dynamic_cast<Origin::Services::Subscription::SubscriptionDetailInfoResponse *>(sender());

                if(response)
                {
                    if(response->httpStatus() == Origin::Services::Http200Ok)
                    {
                        mSubscriptionInfo = response->info();

                        serialize(true);

                        refreshVaultInfo();

                        switch(status())
                        {
                        case server::SUBSCRIPTIONSTATUS_PENDINGEXPIRED:
                        case server::SUBSCRIPTIONSTATUS_ENABLED:
                            GetTelemetryInterface()->Metric_SUBSCRIPTION_INFO(isFreeTrial(), mSubscriptionInfo.Subscription.subscriptionUri.toLatin1(), Origin::Services::PlatformService::machineHashAsString().toUtf8());
                            GetTelemetryInterface()->setSubscriberStatus(isFreeTrial() ? TELEMETRY_SUBSCRIBER_STATUS_TRIAL : TELEMETRY_SUBSCRIBER_STATUS_ACTIVE);
                            break;

                        case server::SUBSCRIPTIONSTATUS_EXPIRED:
                            GetTelemetryInterface()->setSubscriberStatus(TELEMETRY_SUBSCRIBER_STATUS_LAPSED);
                            break;

                        default:
                            GetTelemetryInterface()->setSubscriberStatus(TELEMETRY_SUBSCRIBER_STATUS_NON_SUBSCRIBER);
                            break;
                        }

                        emit updated();
                    }
                    else
                    {
                        emit updateFailed();
                    }

                    response->deleteLater();
                }
            }

            void SubscriptionManager::onSubscriptionVaultInfoAvailable()
            {
                Origin::Services::Subscription::SubscriptionVaultInfoResponse * response = dynamic_cast<Origin::Services::Subscription::SubscriptionVaultInfoResponse *>(sender());

                if(response)
                {
                    if(response->httpStatus() == Origin::Services::Http200Ok)
                    {
                        mSubscriptionVaultInfo = response->info();

                        createMasterTitleToGameMapping();

                        serialize(true);

                        emit updated();
                    }
                    else
                    {
                        emit updateFailed();
                    }

                    response->deleteLater();
                }
            }

            QString SubscriptionManager::subscriptionId() const
            {
                QStringList parts = mSubscriptionInfo.Subscription.subscriptionUri.split('/');

                if(parts.size() > 0)
                {
                    return parts.last();
                }
                else
                {
                    return "";
                }
            }

            void SubscriptionManager::createMasterTitleToGameMapping()
            {
                mMasterTitleToGame.clear();

                for(unsigned int g = 0; g < mSubscriptionVaultInfo.vaultGameInfo.size(); ++g)
                {
                    for(unsigned int m = 0; m < mSubscriptionVaultInfo.vaultGameInfo[g].masterTitleIds.masterTitleId.size(); ++m)
                    {
#ifdef ORIGIN_DEBUG
                        // Make sure the master title id hasn't been used yet.
                        if(mMasterTitleToGame.find(mSubscriptionVaultInfo.vaultGameInfo[g].masterTitleIds.masterTitleId[m]) != mMasterTitleToGame.end())
                        {
                            QString message;
                            QString game1 = mSubscriptionVaultInfo.vaultGameInfo[g].displayName;
                            QString game2 = mMasterTitleToGame[mSubscriptionVaultInfo.vaultGameInfo[g].masterTitleIds.masterTitleId[m]]->displayName;

                            message = QString("Game %1 and %2 have the same masterTitleId").arg(game1).arg(game2);

                            ORIGIN_ASSERT_MESSAGE(0, message.toUtf8());
                        }
#endif //ORIGIN_DEBUG
                        mMasterTitleToGame[mSubscriptionVaultInfo.vaultGameInfo[g].masterTitleIds.masterTitleId[m]] = &mSubscriptionVaultInfo.vaultGameInfo[g];
                    }
                }
            }

            void SubscriptionManager::checkPendingUpgrades()
            {
                bool upgradeCompleted = false;

                auto iter = mPendingUpgrades.begin();
                while (iter != mPendingUpgrades.end())
                {
                    if (!isUpgradeAvailable(*iter, 0))
                    {
                        sendUpgradeTelemetry(true, (*iter), "pending upgrades");
                        iter = mPendingUpgrades.erase(iter);
                        upgradeCompleted = true;
                    }
                    else
                    {
                        ++iter;
                    }
                }

                if (upgradeCompleted)
                {
                    emit entitlementUpgraded();
                }
            }

            void SubscriptionManager::sendUpgradeTelemetry(const bool& success, const QString& id, const QString& context)
            {
                QList<Content::EntitlementRef> fullGameEntitlements = Content::ContentController::currentUserContentController()->entitlementsBaseByMasterTitleId(id);

                if(fullGameEntitlements.size() == 0)
                {
                    Content::EntitlementRef game = Content::ContentController::currentUserContentController()->entitlementByProductId(id);
                    if(game.isNull())return;
                    fullGameEntitlements = Content::ContentController::currentUserContentController()->entitlementsBaseByMasterTitleId(game->contentConfiguration()->masterTitleId());
                }

                // Verify that the elapsed time between subscription activation and upgrade is included 
                const QString elapsedDays = QString::number(startDate().daysTo(Services::TrustedClock::instance()->nowUTC()));
                QString upgradeOfferId = "";
                QString ownedOfferId = "";

                foreach(Content::EntitlementRef e, fullGameEntitlements)
                {
                    // We aren't going to find the upgraded offer if it's an error
                    if(success && e->contentConfiguration()->isEntitledFromSubscription())
                    {
                        // Verify that the offer ID of the "vault edition" the user upgraded to is included.
                        upgradeOfferId = e->contentConfiguration()->productId();
                    }
                    else
                    {
                        // Verify that the offer ID of the owned content prior to upgrade is included.
                        ownedOfferId = e->contentConfiguration()->productId();
                    }

                    // Conditions to break
                    // We found both upgrade and owned offers
                    if(upgradeOfferId.count() && ownedOfferId.count())
                    {
                        ORIGIN_LOG_EVENT << "owed: " << ownedOfferId << " upgrade: " << upgradeOfferId;
                        break;
                    }
                    // This is an error, but we found the owned offer
                    else if(!success && ownedOfferId.count())
                    {
                        ORIGIN_LOG_ERROR << "Can't upgrade owed entitlement: " << ownedOfferId;
                        break;
                    }
                }

                const QString log = QString("Owned offer: %0 | Upgrade Offer: %1 | elapsedDays: %2 | success: %3 | context: %4")
                    .arg(ownedOfferId)
                    .arg(upgradeOfferId)
                    .arg(elapsedDays)
                    .arg(success ? "true" : "false")
                    .arg(context);
                ORIGIN_LOG_ERROR << log;
                GetTelemetryInterface()->Metric_SUBSCRIPTION_ENT_UPGRADED(ownedOfferId.toUtf8().data(), upgradeOfferId.toUtf8().data(), elapsedDays.toUtf8().data(), success, context.toUtf8().data());
            }

            void SubscriptionManager::sendRemoveStartTelemetry(const bool& success, const QString& id, const QString& context)
            {
                QList<Content::EntitlementRef> fullGameEntitlements = Content::ContentController::currentUserContentController()->entitlementsBaseByMasterTitleId(id);

                if(fullGameEntitlements.size() == 0)
                {
                    Content::EntitlementRef game = Content::ContentController::currentUserContentController()->entitlementByProductId(id);
                    if(game.isNull())
                    {
                        const QString log = QString("Can't find game | id: %1 | elapsedDays: %2 | success: %3 | context: %4")
                            .arg(id)
                            .arg("unknown")
                            .arg(success ? "true" : "false")
                            .arg(context);
                        ORIGIN_LOG_ERROR << log;
                        GetTelemetryInterface()->Metric_SUBSCRIPTION_ENT_REMOVE_START(id.toUtf8().data(), QString("unknown").toUtf8().data(), success, context.toUtf8().data());
                        return;
                    }
                    fullGameEntitlements = Content::ContentController::currentUserContentController()->entitlementsBaseByMasterTitleId(game->contentConfiguration()->masterTitleId());
                }

                QString elapsedDays = "";
                QString upgradeOfferId = "";
                foreach(Content::EntitlementRef e, fullGameEntitlements)
                {
                    if(e->contentConfiguration()->isEntitledFromSubscription())
                    {
                        // Verify that the offer ID of the "vault edition" offer is included.
                        upgradeOfferId = e->contentConfiguration()->productId();
                        // Verify that the elapsed time between offer acquisition and removal is included
                        elapsedDays = QString::number(e->contentConfiguration()->entitleDate().daysTo(Services::TrustedClock::instance()->nowUTC()));
                        break;
                    }
                }
                const QString log = QString("Remove start: %0 | id: %1 | elapsedDays: %2 | success: %3 | context: %4")
                    .arg(upgradeOfferId)
                    .arg(id)
                    .arg(elapsedDays)
                    .arg(success ? "true" : "false")
                    .arg(context);
                ORIGIN_LOG_ERROR << log;
                GetTelemetryInterface()->Metric_SUBSCRIPTION_ENT_REMOVE_START(upgradeOfferId.toUtf8().data(), elapsedDays.toUtf8().data(), success, context.toUtf8().data());
            }

            void SubscriptionManager::sendRemoveFinishedTelemetry(const bool& success, const QString& id, const QString& context)
            {
                const QString log = QString("id: %1 | success: %2 | context: %3")
                    .arg(id)
                    .arg(success ? "true" : "false")
                    .arg(context);
                ORIGIN_LOG_ERROR << log;
                GetTelemetryInterface()->Metric_SUBSCRIPTION_ENT_REMOVED(id.toUtf8().data(), success, context.toUtf8().data());
            }

            SubscriptionRedemptionError SubscriptionManager::parseRedemptionError(const QByteArray &body)
            {
                QJsonParseError jsonResult;
                const QJsonDocument &jsonDoc = QJsonDocument::fromJson(body, &jsonResult);

                if (jsonResult.error == QJsonParseError::NoError)
                {
                    const QJsonObject &jsonObj = jsonDoc.object();
                    const QString &error = jsonObj.value("error").toString().toLower();

                    if ("invalidrequestagerestricted" == error)
                    {
                        return RedemptionErrorAgeRestricted;
                    }
                    else if ("invalidrequestlocation" == error)
                    {
                        return RedemptionErrorLocation;
                    }
                    else if ("invalidrequestmissingauthorization" == error)
                    {
                        return RedemptionErrorMissingAuthorization;
                    }
                    else if ("invalidrequestbadauthorization" == error)
                    {
                        return RedemptionErrorBadAuthorization;
                    }
                    else if ("invalidRequestUnsupportedType" == error)
                    {
                        return RedemptionErrorBadMethod;
                    }
                }

                return RedemptionErrorUnknown;
            }
        }
    }
}


