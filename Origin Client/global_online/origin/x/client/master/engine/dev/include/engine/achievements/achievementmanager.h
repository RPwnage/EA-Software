///////////////////////////////////////////////////////////////////////////////
/// \file       achievementmanager.h
/// This file contains the declaration of the AchievementManager. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#ifndef __SDK_ACHIEVEMENT_MANAGER_H__
#define __SDK_ACHIEVEMENT_MANAGER_H__

#include <QHash>
#include <QList>
#include <QSharedPointer>
#include <QStringList>
#include <QObject>
#include <QDomDocument>
#include "achievementportfolio.h"
#include "services/session/SessionService.h"
#include "services/settings/Setting.h"
#include "services/plugin/PluginAPI.h"
#include "engine/content/Entitlement.h"
#include "achievementsetreleaseinfo.h"

#define ACHIEVEMENT_DOCUMENT_VERSION 1      ///<The currently supported document version. Increase when breaking changes are made.

namespace Origin
{
    namespace Engine
    {
        namespace Achievements
        {
            struct AchievementGrantRecord
            {
                QString personaId;  ///< The personaId associated with this achievement.
                QString appId;      ///< The mdmTitleId associated with the achievement.
                QString setId;      ///< The AchievementSetId for this achievement.
                QString id;         ///< The AchievementId for this achievement.
                QString code;       ///< The AchievementGrandCode for this achievement.
                int progress;       ///< The Progress to be awarded to the achievement.
                quint64 timestamp;  ///< Timestamp to prevent retransmitting. 
            };

            struct PostEventRecord
            {
                QString personaId;  ///< The personaId associated with this achievement.
                QString appId;      ///< The mdmTitleId associated with the achievement.
                QString setId;      ///< The AchievementSetId for this achievement.
                QString events;     ///< The events to send.
                quint64 timestamp;  ///< Timestamp to prevent retransmitting. 
            };

            class ORIGIN_PLUGIN_API AchievementManager : public QObject
            {
                Q_OBJECT
            public:
                typedef QList<AchievementPortfolioRef> AchievementPortfolioList;
                typedef QHash<quint64, AchievementPortfolioRef> AchievementPortfolioMap;  ///< Map the user id to the portfolio map.

                ~AchievementManager();

            public:
                const AchievementPortfolioList & achievementPortfolios() const;	                            ///< The list of all the achievementPortfolios.
				const AchievementSetReleaseInfoList &achievementSetReleaseInfo() const;						///< Get a list of achievement set release info.
                AchievementPortfolioRef achievementPortfolioByUserId(const QString &userId) const;          ///< Returns the achievementPortfolio for the specified user.
                AchievementPortfolioRef achievementPortfolioByPersonaId(const QString &personaId) const;    ///< Returns the achievementPortfolio for the specified persona.
            
                void grantQueuedAchievements();                                                             ///< Try to grant the queued achievements if any.
                void postQueuedEvents();                                                                    ///< Try posting the queued achievement events.
                void queueAchievementGrant(const QString personaId, const QString &appId, const QString &achievementSetId, const QString &achivementId, const QString &achievementCode, int progress); ///< qag == queueAchievementGrant. When a direct post fails a delayed post will be done whenever the achievements are reloaded from disk.
                void queueAchievementEvent(QString PersonaId, QString masterTitleId, QString achievementSet, QString events);

            
            signals:
                void added(Origin::Engine::Achievements::AchievementPortfolioRef addedPortfolio);           ///< Signal indicating that an achievement portfolio has been added to the achievement manager.
                void removed(Origin::Engine::Achievements::AchievementPortfolioRef removedPortfolio);       ///< Signal indicating that an achievement portfolio has been removed from the achievement manager.
                void granted(Origin::Engine::Achievements::AchievementSetRef set, Origin::Engine::Achievements::AchievementRef achievement);               ///< Signal indicating that an achievement has been granted.
				void achievementSetReleaseInfoUpdated();													///< Signals that achievement set release info has been updated.

                void enabledChanged(bool bEnabled);                     ///< Signal to indicate that the configuration returned from the server doesn't match the defaults.
                void baseUrlChanged(QString baseUrl);                   ///< Signals that the base URL has changed.

            public slots:
                void clear();                                           ///< Removes the content of the achievement manager. Call/Connect this when the user disappears.

            private slots:
                void onUserLoggedIn(Origin::Engine::UserRef user);
                void onUserOnlineStatusChanged();
                void onQueueAction();
                void granted(QByteArray grantedAchievement);            ///< Call this from the dirty bit code to start the process of the achievement in Origin.
                void achievementGrantCompleted();                       ///< Called when the granting of the achievement has completed.
                void eventPostCompleted();                              ///< Called when the posting of the event has completed.
                void achievementSetReleaseInfoCompleted();				///< Called when the information from the achievement service regarding achievementSetReleaseInfo has completed.

            public:
                bool enabled() const;                                   ///< Indicates whether the achievement manager is enabled. false means do not show achievement
                QString baseUrl() const;                                ///< The base Url for the achievement main page, and the achievement detail pages.
                AchievementPortfolioRef currentUsersPortfolio() const;  ///< Return the portfolio of the user.

                AchievementPortfolioRef createAchievementPortfolio(QString userId, QString personaId); ///< Creates a new achievementportfolio for the user. 

				void updateAchievementSetReleaseInfo();					///< Ask the achievement service for all the release offers.

                void serialize(bool bSave = true);       ///< Load or save the current state of the achievement manager.
                void serializeAchievementGrantQueue(QDomElement root, int version, bool bSave);  ///< Store the pending achievement queue.
                void serializePostEventQueue(QDomElement root, int version, bool bSave);  ///< Store the pending achievement queue.

            private:
                bool mEnabled;                                          ///< Determines whether achievements are enabled in the client.
                QString mBaseUrl;                                       ///< The base url for the achievement summary page, and achievement game detail pages.
                bool mConfigurationLoaded;
                AchievementPortfolioMap mAchievementPortfolioMap;			///< The map between user and the user's portfolio.
                AchievementPortfolioList mAchievementPortfolios;			///< A list of all the portfolio's in the manager.
				AchievementSetReleaseInfoList mAchievementSetReleaseInfo;	///< A list of information about the released achievement sets.

            private:
                static AchievementManager * gInstance;                  ///< The achievement manager instance.

            public:

                /// \brief Initialize the event handler for dirty bit
                void initializeEventHandler();

                /// \brief Initialize the AchievementManager.
                /// Make sure to call this function before using any of the other functions in the AchievementManager, as this is spinning up a worker thread for updating achievement information in
                /// the background
                /// \param owner The owner of the entitlement manager. Can be NULL
                static void init(QObject * owner = NULL);

                /// \brief Shut down the AchievementManager.
                /// Call this on shutdown of the applications so any pending request, and any outstanding network connection can be terminated.
                static void shutdown();

                /// \brief Get the Achievement Manager instance.
                /// The instance pointer is the entry point to the Achievement manager subsystem. Make sure to call init() before calling instance(), as it will return NULL if 
                /// not properly initialized.
                static AchievementManager * instance();

            private:
                /// \brief Constructor.
                explicit AchievementManager(QObject * owner);

                QString cacheFolder() const;
                QList<AchievementGrantRecord>   mAchievementGrantQueue;
                QList<PostEventRecord>          mPostEventQueue;
                QMutex mAchievementGrantQueueMutex;
                QMutex mAchievementSerializationMutex;
                QPointer<Origin::Services::Setting> mLastAchievementsSubmitted;
                QPointer<Origin::Services::Setting> mLastEventsSubmitted;

                QTimer mOnlineTimer;
            };

            /// \brief 
            typedef QSharedPointer<AchievementManager> AchievementManagerRef; 
        }
    }
}

Q_DECLARE_METATYPE(Origin::Engine::Achievements::AchievementManagerRef);

#endif //__SDK_ACHIEVEMENT_MANAGER_H__
