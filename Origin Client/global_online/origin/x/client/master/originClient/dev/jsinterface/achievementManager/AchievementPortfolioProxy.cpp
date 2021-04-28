///////////////////////////////////////////////////////////////////////////////
/// \file       AchievementPortfolioProxy.cpp
/// This file contains the implementation of the AchievementPortfolioProxy. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#include "AchievementPortfolioProxy.h"
#include "AchievementManagerProxy.h"
#include "AchievementSetProxy.h"
#include "AchievementProxy.h"
#include "TelemetryAPIDLL.h"
#include "services/session/SessionService.h"

#include "services/debug/DebugService.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            using namespace Origin::Engine::Achievements;

            bool AchievementPortfolioProxy::available()
            {
                return mAchievementPortfolio->available();
            }


            QObjectList AchievementPortfolioProxy::achievementSets()
            {
                QObjectList list;

                foreach(AchievementSetRef set, mAchievementPortfolio->achievementSets())
                {
                    if(!isValidAchievementSet(set))
                        continue;

                    AchievementSetProxy * proxy = createProxy(set);

                    if(proxy)
                    {
                        list.append(proxy);
                    }
                }

                return list;
            }

            QVariant AchievementPortfolioProxy::getAchievementSet(const QString &name)
            {
                AchievementSetRef set = mAchievementPortfolio->getAchievementSet(name);

                if(!set.isNull())
                {
                    AchievementSetProxy * proxy = createProxy(set);

                    if(proxy)
                    {
                        return QVariant::fromValue<QObject*>(proxy);
                    }
                }

                return QVariant();
            }

            void AchievementPortfolioProxy::updateAchievementSet(const QString &setId)
            {
                mAchievementPortfolio->updateAchievementSet(setId);
            }

            int AchievementPortfolioProxy::totalXp()
            {
                int total = 0;

                foreach(AchievementSetRef set, mAchievementPortfolio->achievementSets())
                {
                    if(!isValidAchievementSet(set))
                        continue;

                    AchievementSetProxy * proxy = createProxy(set);
                    total += proxy->totalXp();
                }
                return total;
            }

            int AchievementPortfolioProxy::totalRp()
            {
                int total = 0;

                foreach(AchievementSetRef set, mAchievementPortfolio->achievementSets())
                {
                    if(!isValidAchievementSet(set))
                        continue;

                    AchievementSetProxy * proxy = createProxy(set);
                    total += proxy->totalRp();
                }
                return total;
            }

            void AchievementPortfolioProxy::updatePoints()
            {
                mAchievementPortfolio->refreshPoints();
            }

            int AchievementPortfolioProxy::xp() const
            {
                return mAchievementPortfolio->xp();
            }

            int AchievementPortfolioProxy::rp() const
            {
                return mAchievementPortfolio->rp();
            }

            QString AchievementPortfolioProxy::userId() const
            {
                return mAchievementPortfolio->userId();
            }

            QString AchievementPortfolioProxy::personaId() const
            {
                return mAchievementPortfolio->personaId();
            }

            int AchievementPortfolioProxy::total()
            {
                int total = 0;

                foreach(AchievementSetRef set, mAchievementPortfolio->achievementSets())
                {
                    if(!isValidAchievementSet(set))
                        continue;

                    AchievementSetProxy * proxy = createProxy(set);

                    if(proxy)
                    {
                        total += proxy->total();
                    }
                }

                return total;
            }

            int AchievementPortfolioProxy::achieved()
            {
                int achieved = 0;

                foreach(AchievementSetRef set, mAchievementPortfolio->achievementSets())
                {
                    if(!isValidAchievementSet(set))
                        continue;

                    AchievementSetProxy * proxy = createProxy(set);

                    if(proxy)
                    {
                        achieved += proxy->achieved();
                    }
                }
                return achieved;
            }

            AchievementPortfolioProxy::AchievementPortfolioProxy(Origin::Engine::Achievements::AchievementPortfolioRef portfolio, QObject * parent) :
                QObject(parent)
            {
                mAchievementPortfolio = portfolio;
                
                ORIGIN_VERIFY_CONNECT(mAchievementPortfolio.data(), SIGNAL(added(Origin::Engine::Achievements::AchievementSetRef)), this, SLOT(setAdded(Origin::Engine::Achievements::AchievementSetRef)));
                ORIGIN_VERIFY_CONNECT(mAchievementPortfolio.data(), SIGNAL(removed(Origin::Engine::Achievements::AchievementSetRef)), this, SLOT(setRemoved(Origin::Engine::Achievements::AchievementSetRef)));

                ORIGIN_VERIFY_CONNECT(mAchievementPortfolio.data(), SIGNAL(xpChanged(int)), this, SIGNAL(xpChanged(int)));
                ORIGIN_VERIFY_CONNECT(mAchievementPortfolio.data(), SIGNAL(rpChanged(int)), this, SIGNAL(rpChanged(int)));

                ORIGIN_VERIFY_CONNECT(mAchievementPortfolio.data(), 
                                        SIGNAL(granted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)), 
                                        this, 
                                        SLOT(granted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)));

                ORIGIN_VERIFY_CONNECT(mAchievementPortfolio.data(), SIGNAL(availableUpdated()), this, SIGNAL(availableUpdated()));
            }

            AchievementPortfolioProxy::~AchievementPortfolioProxy()
            {

            }


            void AchievementPortfolioProxy::setAdded(Origin::Engine::Achievements::AchievementSetRef set)
            {
                if(!isValidAchievementSet(set))
                    return;
                bool bExisted = false;

                AchievementSetProxy * proxy = createProxy(set, true, &bExisted);

                if(!bExisted)
                {
                    emit achievementSetAdded(proxy);
                }
            }

            void AchievementPortfolioProxy::setRemoved(Origin::Engine::Achievements::AchievementSetRef set)
            {
                if(!isValidAchievementSet(set))
                    return;
                
                AchievementSetProxy * proxy = createProxy(set, false);

                if(proxy != NULL)
                {
                    mProxies.remove(set.data());

                    emit achievementSetRemoved(proxy);
                }
            }

            void AchievementPortfolioProxy::granted(Origin::Engine::Achievements::AchievementSetRef set, Origin::Engine::Achievements::AchievementRef achievement)
            {
                // Only show notifications for achievement sets that have entitlements associated with them.
                if(!isValidAchievementSet(set))
                    return;

                AchievementSetProxy * proxy = createProxy(set);

                if(proxy)
                {
                    AchievementProxy * achievementProxy = proxy->mProxies[achievement.data()];

                    if(achievementProxy != NULL)
                    {
                        emit achievementGranted(proxy, achievementProxy);
                    }
                }
            }

            AchievementSetProxy * AchievementPortfolioProxy::createProxy(Origin::Engine::Achievements::AchievementSetRef as, bool bCreate /* = true*/, bool * bExisted /*=NULL*/)
            {
                AchievementSetProxy * proxy = mProxies[as.data()];
                
                if(bExisted)
                {
                    *bExisted = (proxy != NULL);
                }

                if (proxy == NULL)
                {
                    if(bCreate)
                    {
                        proxy = new AchievementSetProxy(as, this);
                        mProxies[as.data()] = proxy;
                    }
                }

                return proxy;
            }

            QString AchievementPortfolioProxy::currentUserId() const
            {
                return Origin::Services::Session::SessionService::instance()->nucleusUser(Origin::Services::Session::SessionService::instance()->currentSession());
            }

            bool AchievementPortfolioProxy::isValidAchievementSet( AchievementSetRef set ) const
            {
                return (mAchievementPortfolio->userId() != currentUserId() || !set->entitlement().isNull()) && !set->achievementSetId().startsWith("CHALLENGES_") && !set->achievementSetId().startsWith("META_");
            }

        }
    }
}
