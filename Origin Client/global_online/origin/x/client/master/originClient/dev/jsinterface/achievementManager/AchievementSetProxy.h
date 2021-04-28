#ifndef _ACHIEVEMENTSETPROXY_H
#define _ACHIEVEMENTSETPROXY_H

/**********************************************************************************************************
* This class is part of Origin's JavaScript bindings and is not intended for use from C++
*
* All changes to this class should be reflected in the documentation in jsinterface/doc
*
* See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
* ********************************************************************************************************/

#include <QObject>
#include <engine/achievements/achievementset.h>
#include <AchievementSetProxy.h>
#include <services/plugin/PluginAPI.h>

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            class AchievementProxy;
			class AchievementSetExpansionProxy;
            
            class ORIGIN_PLUGIN_API AchievementSetProxy : public QObject
            {
                friend class AchievementSetExpansionProxy;
                friend class AchievementManagerProxy;
                friend class AchievementPortfolioProxy;
                Q_OBJECT
            public:
                
                Q_PROPERTY(QString id READ id);
                QString id() const; ///< The identifier of the achievement set.

                Q_PROPERTY(QObjectList achievements READ achievements);
                QObjectList achievements() const;  ///< Get all the achievement sets.

                Q_PROPERTY(QObjectList expansions READ expansions);
                QObjectList expansions() const; ///< Get all the expansions belonging to the set.
              
                Q_PROPERTY(int total READ total);       
                int total() const;  ///< Get the total achievements a user can achieve with this game.

                Q_PROPERTY(int achieved READ achieved);
                int achieved() const; ///< Get the total granted achievements a user has achieved with this game.

                Q_PROPERTY(int totalXp READ totalXp);       
                int totalXp() const;  ///< Get the total Xp for all achievements in the set.

                Q_PROPERTY(int totalRp READ totalRp);       
                int totalRp() const;  ///< Get the total Rp for all achievements in the set.

                Q_PROPERTY(int earnedXp READ earnedXp);       
                int earnedXp() const;  ///< Get the earned Xp for all the achieved achievements in the set.

                Q_PROPERTY(int earnedRp READ earnedRp);
                int earnedRp() const; ///< Get the earned Rp for all the achieved achievements in the set.

                Q_PROPERTY(QString platform READ platform);
                QString platform() const; ///< Returns a comma separated list of platforms.

                Q_PROPERTY(QString displayName READ displayName);
                QString displayName() const; ///< The name of the game associated with this achievement set.

                Q_PROPERTY(bool isUpdated READ isUpdated);
                bool isUpdated() const; ///< Indicated whether the achievementset was updated in the last refresh.

                Q_PROPERTY(QDateTime lastUpdated READ lastUpdated);
                QDateTime lastUpdated() const; ///< The DateTime of the last successful update in UTC.

                Q_INVOKABLE QVariant getAchievement(QString id);    ///< Get the achievement by id.

                Q_INVOKABLE QVariant getExpansion(QString id);      ///< Get the expansion by id.

            private slots:
                void added(Origin::Engine::Achievements::AchievementRef);
                void removed(Origin::Engine::Achievements::AchievementRef);
                void expansionAdded(QString);
                void expansionRemoved(QString);

           signals:
                void updated();
                void achievementAdded(QObject *);
                void achievementRemoved(QObject *);
                void expansionAdded(QObject *);
                void expansionRemoved(QObject *);

            private:
                AchievementProxy* createProxy(Origin::Engine::Achievements::AchievementRef achievement, bool bCreate, bool &bExisting);
                AchievementSetExpansionProxy* createAchievementSetExpansionProxy(QString id, bool bCreate, bool &bExisting);

            private:
                explicit AchievementSetProxy(Origin::Engine::Achievements::AchievementSetRef achievementSet, QObject *parent);

                Origin::Engine::Achievements::AchievementSetRef mAchievementSet;

                QHash<Origin::Engine::Achievements::Achievement *, AchievementProxy *> mProxies;
                QHash<QString, AchievementSetExpansionProxy *> mExpansionProxies;
            };
        }
    }
}


#endif