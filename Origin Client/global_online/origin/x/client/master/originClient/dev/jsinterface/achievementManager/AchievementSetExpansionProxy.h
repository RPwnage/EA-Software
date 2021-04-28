#ifndef _ACHIEVEMENTSETEXPANSIONPROXY_H
#define _ACHIEVEMENTSETEXPANSIONPROXY_H

/**********************************************************************************************************
* This class is part of Origin's JavaScript bindings and is not intended for use from C++
*
* All changes to this class should be reflected in the documentation in jsinterface/doc
*
* See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
* ********************************************************************************************************/

#include <QObject>
#include <engine/achievements/achievementset.h>
#include <services/plugin/PluginAPI.h>

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            class AchievementProxy;
            class AchievementSetProxy;

            class ORIGIN_PLUGIN_API AchievementSetExpansionProxy : public QObject
            {
                friend class AchievementSetProxy;
                Q_OBJECT
            public:

                Q_PROPERTY(QString id READ id);
                QString id() const; ///< The identifier of the achievement set expansion.

                Q_PROPERTY(QObjectList achievements READ achievements);
                QObjectList achievements() const;  ///< Get all the achievement in this expansion.

                Q_PROPERTY(int total READ total);       
                int total() const;  ///< Get the total achievements in this expansion.

                Q_PROPERTY(int achieved READ achieved);
                int achieved() const; ///< Get the total granted achievements a user has achieved with this expansion.

                Q_PROPERTY(int totalXp READ totalXp);       
                int totalXp() const;  ///< Get the total Xp for all achievements in this expansion.

                Q_PROPERTY(int totalRp READ totalRp);       
                int totalRp() const;  ///< Get the total Rp for all achievements in this expansion.

                Q_PROPERTY(int earnedXp READ earnedXp);       
                int earnedXp() const;  ///< Get the earned Xp for all the achieved achievements in this expansion.

                Q_PROPERTY(int earnedRp READ earnedRp);
                int earnedRp() const; ///< Get the earned Rp for all the achieved achievements in this expansion.

                Q_PROPERTY(QString platform READ platform);
                QString platform() const; ///< Returns a comma separated list of platforms.

                Q_PROPERTY(QString displayName READ displayName);
                QString displayName() const; ///< The name of the expansion.

                Q_PROPERTY(bool owned READ isOwned);
                bool isOwned() const; ///< Indication on whether the user owns this expansion.
                
                Q_INVOKABLE QObject * getAchievementSet() { return (QObject *)mAchievementSet; }  ///< Get the containing achievement set.

            private:

                QList<AchievementProxy *> getAchievements() const;
                AchievementProxy * getAchievement(QString id) const;

                explicit AchievementSetExpansionProxy(AchievementSetProxy * achievementSet, const QString &id);

                AchievementSetProxy * mAchievementSet;
                QString mId;
            };
        }
    }
}


#endif // _ACHIEVEMENTSETEXPANSIONPROXY_H