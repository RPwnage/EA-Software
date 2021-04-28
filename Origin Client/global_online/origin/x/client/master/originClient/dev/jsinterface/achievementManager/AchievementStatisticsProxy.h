#ifndef _ACHIEVEMENTSTATISTICS_PROXY_H
#define _ACHIEVEMENTSTATISTICS_PROXY_H

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
#include <QDateTime>

#include "AchievementProxy.h"
#include "WebWidget/ScriptOwnedObject.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            class AchievementManagerProxy;

            class ORIGIN_PLUGIN_API AchievementStatisticsMostPointsInPeriod : public WebWidget::ScriptOwnedObject
            {
                Q_OBJECT
            public:
                AchievementStatisticsMostPointsInPeriod(QDateTime when, int howMuch) : WebWidget::ScriptOwnedObject(NULL), mWhen(when), mHowMuch(howMuch) {}
                
                Q_PROPERTY(QDateTime when READ when); QDateTime when(){ return mWhen; }
                Q_PROPERTY(int howMuch READ howMuch); int howMuch(){ return mHowMuch; }

            private:
                QDateTime mWhen;
                int mHowMuch;
            };

            class ORIGIN_PLUGIN_API AchievementStatisticsAchievement : public WebWidget::ScriptOwnedObject
            {
                friend struct QDateTimeDescending;
                Q_OBJECT
            public:
                AchievementStatisticsAchievement(AchievementSetProxy * set, AchievementProxy * ach) : WebWidget::ScriptOwnedObject(NULL), mSet(set), mAchievement(ach) {}
                ~AchievementStatisticsAchievement()
                {

                }

                Q_PROPERTY(QObject * achievementSet READ achievementSet); QObject * achievementSet(){ return (QObject *) mSet; }
                Q_PROPERTY(QObject * achievement READ achievement); QObject * achievement(){ return (QObject *) mAchievement; }
                
            private:
                AchievementSetProxy * mSet;
                AchievementProxy * mAchievement;
            };


            typedef  int (AchievementProxy::*IntPropertyFunc)() const;

            class ORIGIN_PLUGIN_API AchievementStatisticsProxy : public QObject
            {
                friend class AchievementManagerProxy;
                Q_OBJECT
            public:

                Q_INVOKABLE QVariant mostXpIn(int period);   ///< Return the most xp scored in the specified period. Day=0, Week=1, Month=2, Year=3
                Q_INVOKABLE QVariant mostRpIn(int period);   ///< Return the most rp scored in the specified period. Day=0, Week=1, Month=2, Year=3
                Q_INVOKABLE QVariant mostCntIn(int period);  ///< Return the highest achievement count specified period. Day=0, Week=1, Month=2, Year=3
                Q_INVOKABLE QVariant topAchievement();       ///< Returns the best achievement ever achieved.
                Q_INVOKABLE QObjectList withinReach(int count); ///< Give up to count achievements that are within reach.
                Q_INVOKABLE QObjectList earnedRecently(int count); ///< Give up to count achievements that are earned recently.

            private:
                QVariant highestIn(int period, IntPropertyFunc func);

                int key( int period, const QDateTime &achievedDate );
                QDateTime dateFromKey( int period, int key );

                AchievementStatisticsProxy(AchievementManagerProxy * achievementManagerProxy);

            private:
                AchievementManagerProxy * mAchievementManagerProxy;
            };
        }
    }
}

#endif //_ACHIEVEMENTSTATISTICS_PROXY_H
