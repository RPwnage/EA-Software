///////////////////////////////////////////////////////////////////////////////
/// \file       SubscriptionManager.h
/// This file contains the declaration of the SubscriptionManager. 
/// When the user has an subscription entitlement this class manages anything related 
/// to that subscription.
///
///	\author     Hans van Veenendaal
/// \date       2010-2014
/// \copyright  Copyright (c) 2014 Electronic Arts. All rights reserved.

#ifndef __SUBSCRIPTION_MANAGER_H__
#define __SUBSCRIPTION_MANAGER_H__

#include <QObject>
#include <QSharedPointer>

namespace Origin
{
    namespace Engine
    {
        namespace Subscription
        {
            enum SubscriptionUpgradeEnum
            {
                UPGRADEAVAILABLE_NONE,
                UPGRADEAVAILABLE_BASEGAME_ONLY,
                UPGRADEAVAILABLE_DLC_ONLY,
                UPGRADEAVAILABLE_BASEGAME_AND_DLC
            };
        }
    }
}

#include "services/session/SessionService.h"
#include "services/settings/Setting.h"
#include "services/plugin/PluginAPI.h"
#include "engine/content/Entitlement.h"
#include "LSXWrapper.h"
#include "server.h"
#include "serverReader.h"
#include "serverWriter.h"
#include <QDomDocument>

#define SUBSCRIPTION_DOCUMENT_VERSION 1      ///<The currently supported document version. Increase when breaking changes are made.

extern qint64 SUBSCRIPTION_MAX_OFFLINE_PLAY_TIME;

namespace Origin
{
    namespace Engine
    {
        namespace Subscription
        {
            enum SubscriptionRedemptionError
            {
                // Error codes returned from the server
                RedemptionErrorAgeRestricted,
                RedemptionErrorLocation,
                RedemptionErrorMissingAuthorization,
                RedemptionErrorBadAuthorization,
                RedemptionErrorBadMethod,

                // Client-side error codes
                RedemptionErrorNoSubscription,
                RedemptionErrorNoUpgrade,
                RedemptionErrorUnknownGame,
                RedemptionErrorTimeout,
                RedemptionErrorUnknown
            };

            class ORIGIN_PLUGIN_API SubscriptionManager : public QObject
            {
                Q_OBJECT
            public:

                ~SubscriptionManager();

            public:
                typedef QMap<QString, server::gameT *> MasterTitleIdToGameMap;


                /// \brief get the subscription Id
                QString subscriptionId() const;

                /// \brief When this returns true, the Origin client is enabled for subscription.
                bool enabled() const;

                /// \brief When the user first signed up for a subscription.
                QDateTime firstSignupDate() const;

                /// \brief This function tells whether the account has a subscription associated with it.
                bool userHasSubscription() const;

                /// \brief indicates that the subscription is active.
                bool isActive() const;

                /// \brief This function tells whether the account has had multiple subscriptions.
                QDateTime startDate() const;

                /// \brief indicates that the subscription is in the free trial period.
                bool isFreeTrial() const;

                /// \brief indicates if the user has access to the subscription trial.
                /// -1 = not received yet, 0 = false, 1 = true
                int isTrialEligible() const {return mIsTrialEligibile;}

                /// \brief subscription status.
                server::SubscriptionStatusT status() const;

                /// \brief Returns the expiration of the subscription
                QDateTime expiration() const;

                /// \brief Returns the billing date of the next subscription
                QDateTime nextBillingDate() const;

                /// \brief Indicates whether the users entitlement can be upgraded.
                /// This compares the users entitlements with the offers that come in the subscription for
                /// a certain master title.
                /// \param masterTitleId The Master Title Id that you want to get information for.
                SubscriptionUpgradeEnum isUpgradeAvailable(const QString &masterTitleId, const int rank) const;

                /// \brief Check whether the master title has an upgrade available.
                void upgrade(const QString & masterTitleId);

                /// \brief Downgrade the game that is part of a subscription
                void downgrade(const QString & masterTitleId);

                /// \brief Check whether an offer is part of the subscription
                bool isAvailableFromSubscription(const QString &offerId) const;

                /// \brief Request an entitlement for an offer that is part of the subscription
                void entitle(const QString &offerId);

                /// \brief Revoke an entitlement for an offer that is part of the subscription
                void revoke(const QString &offerId);

                /// \brief The amount of time still available until Origin will expire the subscription.
                /// \return The remaining time until expiration, or 0 when expired. If user has/had no subscription returns -1
                long timeRemaining() const;

                /// \brief The amount of time still available playing offline until the user needs to go back online in seconds.
                long offlinePlayRemaining() const;

                /// \brief Return the last known good time (i.e. the last time the user was online) and the time could
                /// be trusted.  This value is serialized when the client is shut down so it is preserved across multiple
                /// sessions.
                qint64 lastKnownGoodTime() const;
                
                /// \brief Store the subscription information to disk. Used for offline play.
                void serialize(bool bSave = false);

                /// \brief Get the subscription trial eligibility from the server.
                /// Because we don't want to call this multiple times we are going to just call this from clientflow start instead of onLogin signal.
                /// That way it will fix our timing issue with the promo manager.
                void refreshSubscriptionTrialEligibility();

            signals:
                /// \brief This signal is fired when the subscription is updated.
                void updated();

                /// \brief Indication that we couldn't get the subscription information from the server.
                void updateFailed();

                /// \brief Upgrade Completed
                void entitlementUpgraded();

                /// \brief Remove Completed
                void entitlementRemoved(const QString &offerId);

                /// \brief Emitted when an entitle(offerId) call fails
                void entitleFailed(const QString &offerId, SubscriptionRedemptionError error);

                /// \brief Emitted when an revoke(offerId) call fails
                void revokeFailed(const QString &offerId, SubscriptionRedemptionError error);

                /// \brief Emitted when the upgrade for a given masterTitle is not available.
                void upgradeFailed(const QString &masterTitleId, SubscriptionRedemptionError error);

                /// \brief Emitted when the downgrade for a given masterTitle is not available.
                void downgradeFailed(const QString &masterTitleId, SubscriptionRedemptionError error);

                /// \brief Emitted when the subscription trial information is received.
                void trialEligibilityInfoReceived();

            public slots:
                /// \brief Call this slot when the user is logged in.
                void onLoggedIn(Origin::Engine::UserRef);

                /// \brief Call this slot when the user is logged out.
                void onLoggedOut(Origin::Engine::UserRef);

                /// \brief Dirtybit handler
                void catalogChanged(QByteArray);
                void subscriptionChanged(QByteArray);

            private slots:
                void onSubscriptionInfoAvailable();
                void onSubscriptionTrialEligibilityInfoAvailable();
                void onSubscriptionDetailInfoAvailable();
                void onSubscriptionVaultInfoAvailable();

                void onEntitleResponse();
                void onRevokeResponse();
                void onUpgradeResponse();
                void onDowngradeResponse();
                void onRefreshTimeout();

                void onOfflineTimeout();
                void onUpdateStarted();
                void onUpdateFinished(const Content::EntitlementRefList &added, const Content::EntitlementRefList &removed);
                
            private:
                static SubscriptionManager * gInstance;                  ///< The subscription manager instance.

            public:

                /// \brief Initialize the event handler for dirty bit
                void initializeEventHandler();

                /// \brief Initialize the SubscriptionManager.
                /// Make sure to call this function before using any of the other functions in the SubscriptionManager, as this is spinning up a worker thread for updating achievement information in
                /// the background
                /// \param owner The owner of the entitlement manager. Can be NULL
                static void init(QObject * owner = NULL);

                /// \brief Shut down the SubscriptionManager.
                /// Call this on shutdown of the applications so any pending request, and any outstanding network connection can be terminated.
                static void shutdown();

                /// \brief Get the Achievement Manager instance.
                /// The instance pointer is the entry point to the Achievement manager subsystem. Make sure to call init() before calling instance(), as it will return NULL if 
                /// not properly initialized.
                static SubscriptionManager * instance();

            private:
                /// \brief Constructor.
                explicit SubscriptionManager(QObject * owner);

                /// \brief Removes all user specific content from the SubscriptionManager.
                void clear();

                /// \brief Generate the location for the offline subscription cache.
                QString cacheFolder() const;

                /// \brief Get the subscription information from the server.
                void refreshSubscriptionInfo();

                /// \brief Get the subscription vault information from the server.
                void refreshVaultInfo();

                /// \brief Creates a quick mapping from master title to the game record.
                void createMasterTitleToGameMapping();

                /// \brief function used in serialize to easily put structures in the serialization document
                template <typename T> void AddSubDocument( QDomElement &root, T &subDocument )
                {
                    LSXWrapper subDoc;

                    server::Write(&subDoc, subDocument);
                    QDomNode subDocNode = root.ownerDocument().importNode(subDoc.GetRootNode(), true);
                    root.appendChild(subDocNode);
                }

                /// \brief function used in serialize to easily read structures in the serialization document
                template <typename T> void ReadSubDocument( QDomElement &root, const char * tag, T &subDocument )
                {
                    LSXWrapper doc(root.ownerDocument(), root);

                    if(doc.FirstChild(tag))
                    {
                        server::Read(&doc, subDocument);
                    }
                }

                /// \brief emits entitlementUpgraded for any pending master title upgrades that have been completed
                void checkPendingUpgrades();

                /// \brief Fingers out the telemetry we need to send for upgrade
                void sendUpgradeTelemetry(const bool& success, const QString& id, const QString& context);

                /// \brief Fingers out the telemetry we need to send for downgrade/remove start
                void sendRemoveStartTelemetry(const bool& success, const QString& id, const QString& context);

                /// \brief Fingers out the telemetry we need to send for downgrade/remove finished
                void sendRemoveFinishedTelemetry(const bool& success, const QString& id, const QString& context);

                /// \brief Parse the error code from a failed redemption
                SubscriptionRedemptionError parseRedemptionError(const QByteArray &body);

                /// \brief Kick off a thread to check the licenses.
                void refreshLicenses(bool bIsUserOnline, bool hasOfflineTimeRemaining);

            private:
                bool mEnabled;
                QTimer mExpiryTimer;

                server::SubscriptionInfoT mSubscriptionBaseInfo;
                server::SubscriptionDetailInfoT mSubscriptionInfo;
                server::SubscriptionVaultInfoT mSubscriptionVaultInfo;

                MasterTitleIdToGameMap mMasterTitleToGame;

                QSet<QString> mPendingEntitlements;
                QSet<QString> mPendingUpgrades;
                QTimer mRefreshTimeoutTimer;
                QTimer mOfflinePlayTickTimer;
                bool mRefreshRequested;
                bool mExpiredSubscriptionRequestRetry;
                int mIsTrialEligibile;

                mutable qint64 mGoodTimeOffset; 
                mutable qint64 mLastKnownGoodTime;
                mutable qint64 mLastSampleTime;         // For visual purposes to get the offline play time remaining in seconds.
            };

            /// \brief 
            typedef QSharedPointer<SubscriptionManager> SubscriptionManagerRef; 


            class OOARunner : public QObject
            {
                Q_OBJECT;
            public:
                OOARunner(bool bIsUserOnline, bool hasOfflineTimeRemaining) : 
                    m_bIsUserOnline(bIsUserOnline),
                    m_hasOfflineTimeRemaining(hasOfflineTimeRemaining)
                {

                }
            signals:
                void finished();
            public slots:
                void refreshLicenses();

            private:
                bool m_bIsUserOnline;
                bool m_hasOfflineTimeRemaining;
            };

        }
    }
}

Q_DECLARE_METATYPE(Origin::Engine::Subscription::SubscriptionManagerRef);

#endif //__SUBSCRIPTION_MANAGER_H__
