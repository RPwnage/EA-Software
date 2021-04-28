///////////////////////////////////////////////////////////////////////////////
/// \file       AchievementManagerProxy.cpp
/// This file contains the implementation of the AchievementManagerProxy. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#include "AchievementManagerProxy.h"
#include "AchievementStatisticsProxy.h"
#include "AchievementPortfolioProxy.h"
#include "AchievementSetProxy.h"
#include "AchievementProxy.h"
#include "TelemetryAPIDLL.h"
#include <services/session/SessionService.h>

#include "services/debug/DebugService.h"

namespace
{
    Origin::Client::JsInterface::AchievementManagerProxy * AchievementManagerInstance = NULL;
}

using namespace Origin::Services::Session;
using namespace Origin::Engine::Achievements;

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            QObjectList AchievementManagerProxy::achievementPortfolios() const
            {
                QObjectList list;

                foreach(QObject * obj, mProxies)
                {
                    list.append(obj);
                }
                return list;
            }

            QObjectList AchievementManagerProxy::achievementSets() const
            {
                return mTheUsersPortfolio->achievementSets();
            }

			QObjectList AchievementManagerProxy::achievementSetReleaseInfo()
			{
				QObjectList list;

				foreach(const AchievementSetReleaseInfo &releaseInfo, mAchievementManager->achievementSetReleaseInfo())
				{
					AchievementSetReleaseInfoProxy * proxy = createAchievementSetReleaseInfoProxy(&releaseInfo);

					list.append(proxy);
				}
				return list;
			}


            QVariant AchievementManagerProxy::getAchievementSet(const QString &name) const
            {
                return mTheUsersPortfolio->getAchievementSet(name);
            }

            void AchievementManagerProxy::updateAchievementSet(const QString &setId)
            {
                mTheUsersPortfolio->updateAchievementSet(setId);
            }

            void AchievementManagerProxy::updatePoints()
            {
                mTheUsersPortfolio->updatePoints();
            }

            int AchievementManagerProxy::xp() const
            {
                return mTheUsersPortfolio->xp();
            }

            int AchievementManagerProxy::rp() const
            {
                return mTheUsersPortfolio->rp();
            }


            int AchievementManagerProxy::total() const
            {
                return mTheUsersPortfolio->total();
            }

            bool AchievementManagerProxy::enabled() const
            {
                return mAchievementManager->enabled();
            }

            QString AchievementManagerProxy::baseUrl() const
            {
                return mAchievementManager->baseUrl();
            }

            int AchievementManagerProxy::achieved() const
            {
                return mTheUsersPortfolio->achieved();
            }

            // Get the achievement portfolio for a user.
            void AchievementManagerProxy::updateAchievementPortfolio(QString userId, QString personaId)
            {
                mAchievementManager->createAchievementPortfolio(userId, personaId);
            }

            QVariant AchievementManagerProxy::getAchievementPortfolioForCurrentUser()
            {
                return getAchievementPortfolioByUserId(SessionService::instance()->nucleusUser(SessionService::currentSession()));
            }


            // Get a achievement portfolio by UserId
            QVariant AchievementManagerProxy::getAchievementPortfolioByUserId(QString userId)
            {
                AchievementPortfolioRef portfolio = mAchievementManager->achievementPortfolioByUserId(userId);

                if(!portfolio.isNull())
                {
                    bool bExisted;

                    AchievementPortfolioProxy * proxy = createProxy(portfolio, true, bExisted);

                    if(proxy)
                    {
                        return QVariant::fromValue<QObject*>(proxy);
                    }
                }
                return QVariant();
            }

            // Get a achievement portfolio by PersonaId
            QVariant AchievementManagerProxy::getAchievementPortfolioByPersonaId(QString personaId)
            {
                AchievementPortfolioRef portfolio = mAchievementManager->achievementPortfolioByPersonaId(personaId);

                if(!portfolio.isNull())
                {
                    bool bExisted;

                    AchievementPortfolioProxy * proxy = createProxy(portfolio, true, bExisted);

                    if(proxy)
                    {
                        return QVariant::fromValue<QObject*>(proxy);
                    }
                }
                return QVariant();
            }

            QObject * AchievementManagerProxy::getAchievementStatistics()
            {
                return mTheUsersStatistics;
            }

            AchievementManagerProxy::AchievementManagerProxy()
            {
                mAchievementManager = AchievementManager::instance();

                AchievementPortfolioRef portfolio = mAchievementManager->currentUsersPortfolio();

                // Create a default proxy for the users entitlements.
                bool bExisted;
                mTheUsersPortfolio = createProxy(portfolio, true, bExisted);
                mTheUsersStatistics = new AchievementStatisticsProxy(this);

                ORIGIN_VERIFY_CONNECT(mTheUsersPortfolio, SIGNAL(xpChanged(int)), this, SIGNAL(xpChanged(int)));
                ORIGIN_VERIFY_CONNECT(mTheUsersPortfolio, SIGNAL(rpChanged(int)), this, SIGNAL(rpChanged(int)));
                ORIGIN_VERIFY_CONNECT(mTheUsersPortfolio, SIGNAL(achievementSetAdded(QObject *)), this, SIGNAL(achievementSetAdded(QObject *)));
                ORIGIN_VERIFY_CONNECT(mTheUsersPortfolio, SIGNAL(achievementSetRemoved(QObject *)), this, SIGNAL(achievementSetRemoved(QObject *)));
                ORIGIN_VERIFY_CONNECT(mTheUsersPortfolio, SIGNAL(achievementGranted(QObject *, QObject *)), this, SIGNAL(achievementGranted(QObject *, QObject *)));

                ORIGIN_VERIFY_CONNECT(mAchievementManager, SIGNAL(enabledChanged(bool)), this, SIGNAL(enabledChanged(bool)));
                ORIGIN_VERIFY_CONNECT(mAchievementManager, SIGNAL(baseUrlChanged(QString)), this, SIGNAL(baseUrlChanged(QString)));
                ORIGIN_VERIFY_CONNECT(mAchievementManager, SIGNAL(added(Origin::Engine::Achievements::AchievementPortfolioRef)), this, SLOT(portfolioAdded(Origin::Engine::Achievements::AchievementPortfolioRef)));
                ORIGIN_VERIFY_CONNECT(mAchievementManager, SIGNAL(removed(Origin::Engine::Achievements::AchievementPortfolioRef)), this, SLOT(portfolioRemoved(Origin::Engine::Achievements::AchievementPortfolioRef)));

				ORIGIN_VERIFY_CONNECT(mAchievementManager, SIGNAL(achievementSetReleaseInfoUpdated()), this, SIGNAL(achievementSetReleaseInfoUpdated()));
            }

            AchievementManagerProxy::~AchievementManagerProxy()
            {
                if (AchievementManagerInstance == this)
                {
                    AchievementManagerInstance = NULL;
                }
            }


            void AchievementManagerProxy::portfolioAdded(AchievementPortfolioRef portfolio)
            {
                bool bExisted = false;

                AchievementPortfolioProxy * proxy = createProxy(portfolio, true, bExisted);

                if(!bExisted)
                {
                    emit achievementPortfolioAdded(proxy);
                }
            }

            void AchievementManagerProxy::portfolioRemoved(AchievementPortfolioRef portfolio)
            {
                bool bExisted = false;

                AchievementPortfolioProxy * proxy = createProxy(portfolio, false, bExisted);

                if(proxy != NULL)
                {
                    mProxies.remove(portfolio.data());

                    emit achievementPortfolioRemoved(proxy);
                }
            }

            AchievementManagerProxy * AchievementManagerProxy::create()
            {
                AchievementManagerInstance = new AchievementManagerProxy();
                return AchievementManagerInstance;
            }

            AchievementManagerProxy * AchievementManagerProxy::instance()
            {
                return AchievementManagerInstance;
            }

            AchievementPortfolioProxy * AchievementManagerProxy::createProxy(AchievementPortfolioRef portfolio, bool bCreate, bool &bExisted)
            {
                AchievementPortfolioProxy * proxy = mProxies[portfolio.data()];

                if (proxy == NULL)
                {
                    bExisted = false;
                    if(bCreate)
                    {
                        proxy = new AchievementPortfolioProxy(portfolio, this);
                        mProxies[portfolio.data()] = proxy;
                    }
                }
                else
                {
                    bExisted = true;
                }

                return proxy;
            }   

			AchievementSetReleaseInfoProxy * AchievementManagerProxy::createAchievementSetReleaseInfoProxy(const AchievementSetReleaseInfo * releaseInfo)
			{
				AchievementSetReleaseInfoProxy * proxy = mAchievementSetReleaseInfoProxies[releaseInfo];

				if (proxy == NULL)
				{
					proxy = new AchievementSetReleaseInfoProxy(releaseInfo, this);
					mAchievementSetReleaseInfoProxies[releaseInfo] = proxy;
				}
				return proxy;
			}
        }
    }
}
