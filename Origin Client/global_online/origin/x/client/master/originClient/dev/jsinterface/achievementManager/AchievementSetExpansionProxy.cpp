///////////////////////////////////////////////////////////////////////////////
/// \file       AchievementSetExpansionProxy.cpp
/// This file contains the implementation of the AchievementSetExpansionProxy. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2013
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#include <engine/achievements/achievement.h>
#include <services/debug/DebugService.h>

#include "AchievementSetExpansionProxy.h"
#include "AchievementManagerProxy.h"
#include "AchievementSetProxy.h"
#include "AchievementProxy.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            struct IndexAscending
            {
                bool operator()(QObject * obj_a, QObject * obj_b) const
                {
                    AchievementProxy * a = dynamic_cast<AchievementProxy *>(obj_a);
                    AchievementProxy * b = dynamic_cast<AchievementProxy *>(obj_b);

                    if(a && b)
                    {
                        return a->id() < b->id();
                    }
                    return true;
                }
            };

            QList<AchievementProxy *> AchievementSetExpansionProxy::getAchievements() const
            {
                QList<AchievementProxy *> list;

                foreach(AchievementProxy * proxy, mAchievementSet->mProxies)
                {
                    if(proxy->expansionId() == mId)
                    {
                        list.append(proxy);
                    }
                }

                return list;
            }

            AchievementProxy * AchievementSetExpansionProxy::getAchievement(QString id) const
            {
                foreach(AchievementProxy * proxy, getAchievements())
                {
                    if(proxy->id() == id)
                    {
                        return proxy;
                    }
                }
                return NULL;
            }


            QObjectList AchievementSetExpansionProxy::achievements() const
            {
                QObjectList list;

                foreach(AchievementProxy * proxy, getAchievements())
                {
                    list.append(proxy);
                }

                qSort(list.begin(), list.end(), IndexAscending());

                return list;
            }

            QString AchievementSetExpansionProxy::id() const
            {
                return mId;
            }

            int AchievementSetExpansionProxy::total() const
            {
                return getAchievements().size();
            }

            int AchievementSetExpansionProxy::achieved() const
            {
                int achieved = 0;

                foreach(AchievementProxy * a, getAchievements())
                {
                    if(a->count() > 0)
                    {
                        achieved++;
                    }
                }
                return achieved;
            }

            int AchievementSetExpansionProxy::totalXp() const
            {
                int xp = 0;

                foreach(AchievementProxy * a, getAchievements())
                {
                    xp += a->xp();
                }
                return xp;
            }

            int AchievementSetExpansionProxy::totalRp() const
            {
                int rp = 0;

                foreach(AchievementProxy * a, getAchievements())
                {
                    rp += a->rp();
                }
                return rp;
            }

            int AchievementSetExpansionProxy::earnedXp() const
            {
                int xp = 0;

                foreach(AchievementProxy * a, getAchievements())
                {
                    // experience points are counted per achievements
                    if(a->count()>0)
                    {
                        xp += a->count() * a->xp();
                    }
                }

                return xp;
            }

            int AchievementSetExpansionProxy::earnedRp() const
            {
                int rp = 0;

                foreach(AchievementProxy * a, getAchievements())
                {
                    // reward points only apply the first time...
                    if(a->count()>0)
                    {
                        rp += a->rp();
                    }
                }
                return rp;
            }

            QString AchievementSetExpansionProxy::platform() const
            {
                return mAchievementSet->platform();
            }

            QString AchievementSetExpansionProxy::displayName() const
            {
                return mAchievementSet->mAchievementSet->findExpansion(mId)->name;
            }

            bool AchievementSetExpansionProxy::isOwned() const
            {
                return mAchievementSet->mAchievementSet->findExpansion(mId)->owned;
            }

            AchievementSetExpansionProxy::AchievementSetExpansionProxy(AchievementSetProxy * achievementSet, const QString &id) :
                QObject(achievementSet),
                mAchievementSet(achievementSet),
                mId(id)
            {

            }
        }
    }
}
