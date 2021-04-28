#ifndef _ACHIEVEMENTMANAGER_PROXY_H
#define _ACHIEVEMENTMANAGER_PROXY_H

/**********************************************************************************************************
* This class is part of Origin's JavaScript bindings and is not intended for use from C++
*
* All changes to this class should be reflected in the documentation in jsinterface/doc
*
* See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
* ********************************************************************************************************/

#include <QObject>
#include <QHash>
#include <QSharedPointer>
#include <QVariant>
#include <engine/achievements/achievement.h>
#include <engine/achievements/achievementmanager.h>
#include <engine/achievements/achievementsetreleaseinfo.h>
#include <services/plugin/PluginAPI.h>

#include "AchievementStatisticsProxy.h"
#include "AchievementSetReleaseInfoProxy.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            class AchievementSetProxy;
            class AchievementPortfolioProxy;
			class AchievementSetReleaseInfoProxy;

            class ORIGIN_PLUGIN_API AchievementManagerProxy : public QObject
            {
                Q_OBJECT
            public:
                // Get all the achievement portfolios
                Q_PROPERTY(QObjectList achievementPortfolios READ achievementPortfolios);
                QObjectList achievementPortfolios() const;

                // Get all the achievement sets.
                Q_PROPERTY(QObjectList achievementSets READ achievementSets);
                QObjectList achievementSets() const;

				// Get the achievement set release info. This returns all the published achievement sets.
				Q_PROPERTY(QObjectList achievementSetReleaseInfo READ achievementSetReleaseInfo);
				QObjectList achievementSetReleaseInfo();

                // Get the users total experience points XP.
                Q_PROPERTY(int xp READ xp);
                int xp() const;

                // Get the users total reward points RP.
                Q_PROPERTY(int rp READ rp);
                int rp() const;

                // Get the total achievements the user can achieve over all games.
                Q_PROPERTY(int total READ total);
                int total() const;

                // Get the total granted achievements the user achieved over all games.
                Q_PROPERTY(int achieved READ achieved);
                int achieved() const;

                // Indicate whether achievements are enabled.
                Q_PROPERTY(bool enabled READ enabled);
                bool enabled() const;

                Q_PROPERTY(QString baseUrl READ baseUrl);
                QString baseUrl() const;


                // Get achievement set
                Q_INVOKABLE QVariant getAchievementSet(const QString &name) const;

                // Invoke a refresh on the achievement set.
                Q_INVOKABLE void updateAchievementSet(const QString &name);

                // Invoke refresh points
                Q_INVOKABLE void updatePoints();

                // Get the achievement portfolio for a user.
                Q_INVOKABLE void updateAchievementPortfolio(QString userId, QString personaId);

                // Get a achievement portfolio by UserId
                Q_INVOKABLE QVariant getAchievementPortfolioForCurrentUser();

                // Get a achievement portfolio by UserId
                Q_INVOKABLE QVariant getAchievementPortfolioByUserId(QString userId);

                // Get a achievement portfolio by PersonaId
                Q_INVOKABLE QVariant getAchievementPortfolioByPersonaId(QString personaId);

                // Returns a pointer to the AchievementStatsProxy. This can be used to calculated 
                Q_INVOKABLE QObject *getAchievementStatistics();

            public:
                // The web widget framework destructs us so we need a public destructor
                // instead of a static destroy()
                ~AchievementManagerProxy();

                static AchievementManagerProxy * create();
                static AchievementManagerProxy * instance();

                // Returns the EntitlementProxy for a given entitlement
                AchievementPortfolioProxy * createProxy(Origin::Engine::Achievements::AchievementPortfolioRef portfolio, bool bCreate, bool &bExisted);
				AchievementSetReleaseInfoProxy * createAchievementSetReleaseInfoProxy(const Origin::Engine::Achievements::AchievementSetReleaseInfo * releaseInfo);

            private slots:
                void portfolioAdded(Origin::Engine::Achievements::AchievementPortfolioRef);
                void portfolioRemoved(Origin::Engine::Achievements::AchievementPortfolioRef);

            signals:
                // Users portfolio events.
                void achievementSetAdded(QObject * set);                        ///< Signals when a new achievementset is added.
                void achievementSetRemoved(QObject * set);                      ///< Signals when an achievementset is removed.
				void achievementSetReleaseInfoUpdated();						///< Signals that the achievement set release info is updated.
                void xpChanged(int newXPValue);                                 ///< Signals the new value of the experience points.
                void rpChanged(int newRPValue);                                 ///< Signals the new value of the reward points.
                void achievementGranted(QObject * achievementSet, QObject * achievement);  ///< Signal the page that an achievement has been awarded.

                // Achievement manager events.
                void achievementPortfolioAdded(QObject * portfolio);    ///< Signals when a new achievement portfolio is added.
                void achievementPortfolioRemoved(QObject * portfolio);  ///< Signals when an achievement portfolio is removed.
                void enabledChanged(bool bEnable);                      ///< Signals that the achievements are enabled/disabled.
                void baseUrlChanged(QString baseUrl);                   ///< Signals that the base Url for the achievement overview page/achievement details page has changed.

            private:
                AchievementManagerProxy();

            private:

                AchievementPortfolioProxy * mTheUsersPortfolio;
                AchievementStatisticsProxy * mTheUsersStatistics;

                QHash<Origin::Engine::Achievements::AchievementPortfolio *, AchievementPortfolioProxy *> mProxies;
				QHash<const Origin::Engine::Achievements::AchievementSetReleaseInfo *, AchievementSetReleaseInfoProxy *> mAchievementSetReleaseInfoProxies;

                Origin::Engine::Achievements::AchievementManager * mAchievementManager;
            };

        }
    }
}

#endif //_ACHIEVEMENTMANAGER_PROXY_H
