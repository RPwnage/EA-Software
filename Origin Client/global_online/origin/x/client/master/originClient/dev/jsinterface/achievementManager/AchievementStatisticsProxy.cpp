///////////////////////////////////////////////////////////////////////////////
/// \file       AchievementStatisticsProxy.cpp
/// This file contains the implementation of the AchievementStatisticsProxy. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#include "AchievementStatisticsProxy.h"
#include "AchievementManagerProxy.h"
#include "AchievementPortfolioProxy.h"
#include "AchievementSetProxy.h"
#include "AchievementProxy.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            // NOTE: This code is only using the Proxies, so it could run in JScript as well.
            QVariant AchievementStatisticsProxy::mostXpIn(int period)
            {
                return highestIn(period, &AchievementProxy::xp);
            }

            QVariant AchievementStatisticsProxy::mostRpIn(int period)
            {
                return highestIn(period, &AchievementProxy::rp);
            }

            QVariant AchievementStatisticsProxy::mostCntIn(int period)
            {
                return highestIn(period, NULL);
            }

            QVariant AchievementStatisticsProxy::topAchievement()
            {
                int top = 0;
                AchievementSetProxy * achievementSet = NULL;
                AchievementProxy * achievement = NULL;
                QDateTime topUpdateTime;

                foreach(QObject * setObj, mAchievementManagerProxy->achievementSets())
                {
                    AchievementSetProxy * set = dynamic_cast<AchievementSetProxy *>(setObj);

                    if(set)
                    {
                        foreach(QObject * achObj, set->achievements())
                        {
                            AchievementProxy * ach = dynamic_cast<AchievementProxy *>(achObj);

                            if(ach->achieved())
                            {
                                if(ach->xp()>top)
                                {
                                    top = ach->xp();
                                    achievementSet = set;
                                    achievement = ach;
                                    topUpdateTime = ach->updateTime();
                                }
                                else
                                {
                                    if(ach->xp() == top && ach->updateTime()>topUpdateTime)
                                    {
                                        top = ach->xp();
                                        achievementSet = set;
                                        achievement = ach;
                                        topUpdateTime = ach->updateTime();
                                    }
                                }
                            }
                        }
                    }
                }

                if(top>0)
                {
                    return QVariant::fromValue<QObject *>(new AchievementStatisticsAchievement(achievementSet, achievement));
                }

                // Not found
                return QVariant();
            }

            QObjectList AchievementStatisticsProxy::withinReach(int count)
            {
                QObjectList list;

                // Find all pinned achievements.
                foreach(QObject * setObj, mAchievementManagerProxy->achievementSets())
                {
                    AchievementSetProxy * set = dynamic_cast<AchievementSetProxy *>(setObj);

                    if(set)
                    {
                        foreach(QObject * achObj, set->achievements())
                        {
                            AchievementProxy * ach = dynamic_cast<AchievementProxy *>(achObj);

                            if(!ach->achieved() && ach->pinned())
                            {
                                list.push_back(new AchievementStatisticsAchievement(set, ach));
                            }
                        }
                    }
                }

                if(list.size() >= count && count>0)goto done;

                // Process origin meta achievements.
                foreach(QObject * setObj, mAchievementManagerProxy->achievementSets())
                {
                    AchievementSetProxy * set = dynamic_cast<AchievementSetProxy *>(setObj);

                    // We are treating quests differently
                    if(!set->id().startsWith("META_"))
                        continue;

                    if(set)
                    {
                        foreach(QObject * achObj, set->achievements())
                        {
                            AchievementProxy * ach = dynamic_cast<AchievementProxy *>(achObj);

                            if(!ach->achieved() && !ach->pinned() && !ach->withinReachHidden() && ach->percent()>=0.5f)
                            {
                                list.push_back(new AchievementStatisticsAchievement(set, ach));
                            }
                        }
                    }
                }

                if(list.size() >= count && count>0)goto done;

                for(int level=1; level<5; level++)
                {
                    foreach(QObject * setObj, mAchievementManagerProxy->achievementSets())
                    {
                        AchievementSetProxy * set = dynamic_cast<AchievementSetProxy *>(setObj);

                        // We are treating quests differently
                        if(set->id().startsWith("META_"))
                            continue;

                        if(set)
                        {
                            int achievedLevel = 0;
                            foreach(QObject * achObj, set->achievements())
                            {
                                AchievementProxy * ach = dynamic_cast<AchievementProxy *>(achObj);

                                if(!ach->achieved() && !ach->pinned() && !ach->withinReachHidden())
                                {
                                    achievedLevel++;

                                    if(achievedLevel == level)
                                    {
                                        list.push_back(new AchievementStatisticsAchievement(set, ach));
                                        
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    if(list.size() >= count && count>0)goto done;
                }
done:
                if(count>0)
                {
                    while(list.size()>count)
                    {
                        delete list.back();
                        list.pop_back();
                    }
                }
                return list;
            }


            struct QDateTimeDescending
            {
                bool operator()(QObject * obj_a, QObject * obj_b) const
                {
                    AchievementStatisticsAchievement * a = dynamic_cast<AchievementStatisticsAchievement *>(obj_a);
                    AchievementStatisticsAchievement * b = dynamic_cast<AchievementStatisticsAchievement *>(obj_b);

                    if(a && b)
                    {
                        return a->mAchievement->updateTime() > b->mAchievement->updateTime();
                    }
                    return true;
                }
            };

            QObjectList AchievementStatisticsProxy::earnedRecently(int count)
            {
                QObjectList earned;

                foreach(QObject * setObj, mAchievementManagerProxy->achievementSets())
                {
                    AchievementSetProxy * set = dynamic_cast<AchievementSetProxy *>(setObj);

                    if(set)
                    {
                        foreach(QObject * achObj, set->achievements())
                        {
                            AchievementProxy * ach = dynamic_cast<AchievementProxy *>(achObj);

                            if(ach->achieved())
                            {
                                earned.push_back(new AchievementStatisticsAchievement(set, ach));
                            }
                        }
                    }
                }

                qSort(earned.begin(), earned.end(), QDateTimeDescending());

                if(count>0)
                {
                    while(earned.size()>count)
                    {
                        delete earned.back();
                        earned.pop_back();
                    }
                }

                return earned;
            }



            QVariant AchievementStatisticsProxy::highestIn(int period, IntPropertyFunc func )
            {
                // This function will iterate over the users achievements, and return the start date of the period, and the amount of points during that period.

                QHash<int, int> histogram;

                foreach(QObject * setObj, mAchievementManagerProxy->achievementSets())
                {
                    AchievementSetProxy * set = dynamic_cast<AchievementSetProxy *>(setObj);

                    if(set)
                    {
                        foreach(QObject * achObj, set->achievements())
                        {
                            AchievementProxy * ach = dynamic_cast<AchievementProxy *>(achObj);

                            if(ach->achieved())
                            {
                                if(func == NULL)
                                {
                                    // Count achievements.
                                    histogram[key(period, ach->updateTime())] += 1;
                                }
                                else
                                {
                                    histogram[key(period, ach->updateTime())] += (ach->*func)();
                                }
                            }
                        }                                                                   
                    }
                }


                QHash<int, int>::iterator highest = histogram.end();

                for(QHash<int, int>::iterator i = histogram.begin(); i != histogram.end(); i++)
                {
                    if(highest != histogram.end())
                    {
                        if(highest.value() < i.value())
                        {
                            highest = i;
                        }
                        else
                        {
                            // Take the most recent value.
                            if(highest.value() == i.value() && highest.key()<i.key())
                            {
                                highest = i;
                            }
                        }
                    }
                    else
                    {
                        highest = i;
                    }
                }

                if(highest != histogram.end())
                {
                    return QVariant::fromValue<QObject *>(new AchievementStatisticsMostPointsInPeriod(dateFromKey(period, highest.key()), highest.value()));
                }

                return QVariant();
            }

            AchievementStatisticsProxy::AchievementStatisticsProxy(AchievementManagerProxy * achievementManagerProxy) :
                QObject(achievementManagerProxy),
                mAchievementManagerProxy(achievementManagerProxy)
            {

            }

            int AchievementStatisticsProxy::key( int period, const QDateTime &achievedDate )
            {
                switch(period)
                {
                case 3: // Yearly
                    return achievedDate.date().year();
                case 2:// Monthly
                    return achievedDate.date().year() * 12 + achievedDate.date().month() - 1;
                case 1: // Weekly
                    {
                        int weekNumber, year;

                        weekNumber = achievedDate.date().weekNumber(&year);

                        return year * 53 + weekNumber - 1;
                    }
                case 0: // Daily
                default:
                    return achievedDate.date().year() * 366 + achievedDate.date().dayOfYear() - 1;
                }
            }

            QDateTime AchievementStatisticsProxy::dateFromKey( int period, int key )
            {
                switch(period)
                {
                case 3: // Yearly
                    return QDateTime(QDate(key, 1, 1));
                case 2:// Monthly
                    return QDateTime(QDate(key/12, key%12 + 1, 1));
                case 1: // Weekly
                    {
                        QDateTime date(QDate(key/53, 1, 1));

                        int day = date.date().dayOfWeek() - 1;
                        
                        if(day>3)
                        {
                            date = date.addDays(7-day);
                        }
                        else
                        {
                            date = date.addDays(-day);
                        }

                        date = date.addDays(key%53 * 7);
                        
                        return date;
                    }
                case 0: // Daily
                default:
                    {
                        QDateTime date(QDate(key/366, 1, 1));
                        date = date.addDays(key%366);
                        return date;
                    }
                }
            }
        }
    }
}

                                                              