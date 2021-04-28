///////////////////////////////////////////////////////////////////////////////
/// \file       AchievementSetProxy.cpp
/// This file contains the implementation of the AchievementSetProxy. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
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

            QObjectList AchievementSetProxy::achievements() const
            {
                QObjectList list;

                foreach(AchievementProxy * proxy, mProxies)
                {
                    list.append(proxy);
                }

                qSort(list.begin(), list.end(), IndexAscending());

                return list;
            }

            QObjectList AchievementSetProxy::expansions() const
            {
                QObjectList list;

                for(int i=0; i<mExpansionProxies.size(); i++)
                {
                    foreach(AchievementSetExpansionProxy * proxy, mExpansionProxies)
                    {
                        if(proxy->achievements().count()>0 && i==mAchievementSet->findExpansion(proxy->id())->order)
                        {
                            list.append(proxy);
                        }
                    }
                }
                return list;
            }

            QString AchievementSetProxy::id() const
            {
                return mAchievementSet->achievementSetId();
            }

            int AchievementSetProxy::total() const
            {
                return mAchievementSet->achievements().size();
            }

            int AchievementSetProxy::achieved() const
            {
                return mAchievementSet->achieved();
            }

            int AchievementSetProxy::totalXp() const
            {
                return mAchievementSet->totalXp();
            }

            int AchievementSetProxy::totalRp() const
            {
                return mAchievementSet->totalRp();
            }

            int AchievementSetProxy::earnedXp() const
            {
                return mAchievementSet->earnedXp();
            }

            int AchievementSetProxy::earnedRp() const
            {
                return mAchievementSet->earnedRp();
            }

            QString AchievementSetProxy::platform() const
            {
                return mAchievementSet->platform();
            }

            QString AchievementSetProxy::displayName() const
            {
                return mAchievementSet->displayName();
            }

            bool AchievementSetProxy::isUpdated() const
            {
                return mAchievementSet->isUpdated();
            }

            QDateTime AchievementSetProxy::lastUpdated() const
            {
                return mAchievementSet->lastUpdated();
            }


            QVariant AchievementSetProxy::getAchievement(QString id)
            {
                Origin::Engine::Achievements::AchievementRef achievement = mAchievementSet->getAchievement(id);

                if(!achievement.isNull())
                {
                    return QVariant::fromValue<QObject *>(mProxies[achievement.data()]);

                }
                return QVariant();
            }

            QVariant AchievementSetProxy::getExpansion(QString id)
            {
                AchievementSetExpansionProxy * proxy = mExpansionProxies[id];

                if(proxy != NULL)
                {
                    return QVariant::fromValue<QObject *>(proxy);
                }
                
                return QVariant();
            }

            void AchievementSetProxy::added(Origin::Engine::Achievements::AchievementRef achievement)
            {
                bool bExisted = false;

                AchievementProxy * proxy = createProxy(achievement, true, bExisted);

                if(!bExisted)
                {
                    emit achievementAdded(proxy);
                }
            }

            void AchievementSetProxy::removed(Origin::Engine::Achievements::AchievementRef achievement)
            {
                bool bExisted = false;

                AchievementProxy * proxy = createProxy(achievement, false, bExisted);

                if(proxy != NULL)
                {
                    mProxies.remove(achievement.data());

                    emit achievementRemoved(proxy);
                }
            }

            void AchievementSetProxy::expansionAdded(QString id)
            {
                bool bExisted = false;

                AchievementSetExpansionProxy * proxy = createAchievementSetExpansionProxy(id, true, bExisted);

                if(!bExisted)
                {
                    emit expansionAdded(proxy);
                }
            }

            void AchievementSetProxy::expansionRemoved(QString id)
            {
                bool bExisted = false;

                AchievementSetExpansionProxy * proxy = createAchievementSetExpansionProxy(id, false, bExisted);

                if(proxy != NULL)
                {
                    mExpansionProxies.remove(id);

                    emit expansionRemoved(proxy);
                }
            }

            AchievementProxy* AchievementSetProxy::createProxy(Origin::Engine::Achievements::AchievementRef a, bool bCreate, bool & bExisted)
            {
                AchievementProxy * proxy = mProxies[a.data()];

                if (proxy == NULL)
                {
                    bExisted = false;

                    if(bCreate)
                    {
                        proxy = new AchievementProxy(a, this);
                        mProxies[a.data()] = proxy;
                    }
                }
                else
                {
                    bExisted = true;
                }

                return proxy;
            }

            AchievementSetExpansionProxy* AchievementSetProxy::createAchievementSetExpansionProxy(QString id, bool bCreate, bool &bExisted)
            {
                AchievementSetExpansionProxy * proxy = mExpansionProxies[id];

                if (proxy == NULL)
                {
                    bExisted = false;

                    if(bCreate)
                    {
                        proxy = new AchievementSetExpansionProxy(this, id);
                        mExpansionProxies[id] = proxy;
                    }
                }
                else
                {
                    bExisted = true;
                }

                return proxy;

            }

            AchievementSetProxy::AchievementSetProxy(Origin::Engine::Achievements::AchievementSetRef achievementSet, QObject *parent) :
                QObject(parent),
                mAchievementSet(achievementSet)
            {
                ORIGIN_VERIFY_CONNECT(achievementSet.data(), SIGNAL(added(Origin::Engine::Achievements::AchievementRef)), this, SLOT(added(Origin::Engine::Achievements::AchievementRef)));
                ORIGIN_VERIFY_CONNECT(achievementSet.data(), SIGNAL(removed(Origin::Engine::Achievements::AchievementRef)), this, SLOT(removed(Origin::Engine::Achievements::AchievementRef)));
                ORIGIN_VERIFY_CONNECT(achievementSet.data(), SIGNAL(expansionAdded(QString)), this, SLOT(expansionAdded(QString)));
                ORIGIN_VERIFY_CONNECT(achievementSet.data(), SIGNAL(expansionRemoved(QString)), this, SLOT(expansionRemoved(QString)));
                ORIGIN_VERIFY_CONNECT(achievementSet.data(), SIGNAL(updated()), this, SIGNAL(updated()));
                
                foreach(Origin::Engine::Achievements::AchievementRef a, achievementSet->achievements())
                {
                    bool bExisted;
                    AchievementProxy * proxy = createProxy(a, true, bExisted);

                    mProxies[a.data()] = proxy;

                    emit achievementAdded(proxy);
                }

                foreach(const Origin::Engine::Achievements::AchievementSetExpansion &e, achievementSet->expansions())
                {
                    bool bExisted;
                    AchievementSetExpansionProxy * proxy = createAchievementSetExpansionProxy(e.id, true, bExisted);

                    mExpansionProxies[e.id] = proxy;

                    emit expansionAdded((QObject *)proxy);
                }
            }
        }
    }
}
