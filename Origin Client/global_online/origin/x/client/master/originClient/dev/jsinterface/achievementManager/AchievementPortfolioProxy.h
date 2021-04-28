#ifndef _ACHIEVEMENTPORTFOLIO_PROXY_H
#define _ACHIEVEMENTPORTFOLIO_PROXY_H

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
#include <engine/achievements/achievementportfolio.h>
#include <services/plugin/PluginAPI.h>

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            class AchievementSetProxy;

            class ORIGIN_PLUGIN_API AchievementPortfolioProxy : public QObject
            {
                friend class AchievementManagerProxy;
                Q_OBJECT
            public:
                Q_PROPERTY(bool available READ available);
                bool available();

                // Get all the achievement sets.
                Q_PROPERTY(QObjectList achievementSets READ achievementSets);
                QObjectList achievementSets();

                // Get the total experience points XP.
                Q_PROPERTY(int totalXp READ totalXp);
                int totalXp();

                // Get the total experience points XP.
                Q_PROPERTY(int totalRp READ totalRp);
                int totalRp();

                // Get the users total experience points XP.
                Q_PROPERTY(int earnedXp READ xp);
                int xp() const;

                // Get the users total reward points RP.
                Q_PROPERTY(int earnedRp READ rp);
                int rp() const;

                Q_PROPERTY(QString userId READ userId);
                QString userId() const;

                Q_PROPERTY(QString personaId READ personaId);
                QString personaId() const;

                // Get the total achievements the user can achieve over all games.
                Q_PROPERTY(int total READ total);
                int total();

                // Get the total granted achievements the user achieved over all games.
                Q_PROPERTY(int achieved READ achieved);
                int achieved();

                // Get achievement set
                Q_INVOKABLE QVariant getAchievementSet(const QString &name);

                // Invoke a refresh on the achievement set.
                Q_INVOKABLE void updateAchievementSet(const QString &name);

                // Invoke refresh points
                Q_INVOKABLE void updatePoints();

            public:
                // The web widget framework destructs us so we need a public destructor
                // instead of a static destroy()
                ~AchievementPortfolioProxy();

                static AchievementPortfolioProxy * create();
                static AchievementPortfolioProxy * instance();

                // Returns the EntitlementProxy for a given entitlement
                AchievementSetProxy* createProxy(Origin::Engine::Achievements::AchievementSetRef set, bool bCreate = true, bool * bExisting = NULL);

            private slots:
                void setAdded(Origin::Engine::Achievements::AchievementSetRef);
                void setRemoved(Origin::Engine::Achievements::AchievementSetRef);
                void granted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef);

            signals:
                void availableUpdated();                                ///< Signals that the portfolio availablility is updated.    
                void achievementSetAdded(QObject * set);                ///< Signals when a new achievementset is added.
                void achievementSetRemoved(QObject * set);              ///< Signals when an achievementset is removed.
                void achievementGranted(QObject *, QObject *);          ///< Signals that an achievement has been granted for the user.
                void xpChanged(int newXPValue);                         ///< Signals the new value of the experience points.
                void rpChanged(int newRPValue);                         ///< Signals the new value of the reward points.

            private:
                AchievementPortfolioProxy(Origin::Engine::Achievements::AchievementPortfolioRef portfolio, QObject * parent);

                QString currentUserId() const;
                bool isValidAchievementSet( Origin::Engine::Achievements::AchievementSetRef set ) const;
            private:
                QHash<Origin::Engine::Achievements::AchievementSet *, AchievementSetProxy *> mProxies;

                Origin::Engine::Achievements::AchievementPortfolioRef mAchievementPortfolio;
            };

        }
    }
}

#endif //_ACHIEVEMENTPORTFOLIO_PROXY_H
