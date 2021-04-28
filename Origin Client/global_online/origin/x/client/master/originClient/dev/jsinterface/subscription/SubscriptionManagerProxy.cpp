///////////////////////////////////////////////////////////////////////////////
/// \file       SubscriptionManagerProxy.cpp
/// This file contains the implementation of the SubscriptionManagerProxy. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2014
// \copyright  Copyright (c) 2014 Electronic Arts. All rights reserved.

#include "TelemetryAPIDLL.h"
#include <services/session/SessionService.h>
#include <services/debug/DebugService.h>
#include "SubscriptionManagerProxy.h"
#include <services/platform/TrustedClock.h>
#include <serverEnumStrings.h>
#include "LocalizedDateFormatter.h"

using namespace Origin::Services::Session;
using namespace Origin::Engine::Subscription;

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            SubscriptionManagerProxy * SubscriptionManagerProxy::s_SubscriptionManagerProxyInstance = NULL;

            bool SubscriptionManagerProxy::enabled() const
            {
                return SubscriptionManager::instance()->enabled();
            }


            // The user has a subscription
            bool SubscriptionManagerProxy::hasSubscription() const
            {
                return SubscriptionManager::instance()->userHasSubscription();
            }

            bool SubscriptionManagerProxy::isExpired() const
            {
                return !SubscriptionManager::instance()->isActive();
            }

			bool SubscriptionManagerProxy::isActive() const
			{
				return SubscriptionManager::instance()->isActive();
			}

            QString SubscriptionManagerProxy::status() const
            {
                server::SubscriptionStatusT status = Origin::Engine::Subscription::SubscriptionManager::instance()->status();

                if(status>=0 && status<server::SUBSCRIPTIONSTATUS_ITEM_COUNT)
                {
                    return server::SubscriptionStatusStrings[status];
                }
                return "UNKNOWN";
            }

            QString SubscriptionManagerProxy::firstSignupDate() const
            {
                const QDateTime dateTime = SubscriptionManager::instance()->firstSignupDate();
                if (dateTime.isValid())
                {
                    return LocalizedDateFormatter().format(dateTime, LocalizedDateFormatter::LongFormat);
                }
                else
                {
                    return "";
                }
            }

            // The subscription expiration date
            QDateTime SubscriptionManagerProxy::expirationDate() const
            {
                return SubscriptionManager::instance()->expiration();
            }

            QVariant SubscriptionManagerProxy::timeRemaining() const
            {
                long remainingTime = SubscriptionManager::instance()->timeRemaining();

                if(remainingTime >= 0)
                {
                    return QVariant(static_cast<int>(remainingTime));
                }
                else
                {
                    return QVariant();
                }
            }

            // The amount of time in seconds remaining for offline play.
            long SubscriptionManagerProxy::offlinePlayRemaining() const
            {
                return SubscriptionManager::instance()->offlinePlayRemaining();
            }


            bool SubscriptionManagerProxy::isAvailableFromSubscription( const QString &offer ) const
            {
                return Origin::Engine::Subscription::SubscriptionManager::instance()->isAvailableFromSubscription(offer);
            }

            SubscriptionManagerProxy::SubscriptionManagerProxy()
            {
                ORIGIN_VERIFY_CONNECT(SubscriptionManager::instance(), SIGNAL(updated()), this, SIGNAL(updated()));
                ORIGIN_VERIFY_CONNECT(SubscriptionManager::instance(), SIGNAL(entitlementUpgraded()), this, SIGNAL(entitlementUpgraded()));
                ORIGIN_VERIFY_CONNECT(SubscriptionManager::instance(), SIGNAL(entitlementRemoved(const QString&)), this, SIGNAL(entitlementRemoved(const QString&)));
            }

            SubscriptionManagerProxy::~SubscriptionManagerProxy()
            {
                
            }

            SubscriptionManagerProxy * SubscriptionManagerProxy::create()
            {
                s_SubscriptionManagerProxyInstance = new SubscriptionManagerProxy();
                return s_SubscriptionManagerProxyInstance;
            }

            SubscriptionManagerProxy * SubscriptionManagerProxy::instance()
            {
                Q_ASSERT(s_SubscriptionManagerProxyInstance != NULL);

                return s_SubscriptionManagerProxyInstance;
            }

        }
    }
}
