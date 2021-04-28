#ifndef SUBSCRIPTION_MANAGER_PROXY_H
#define SUBSCRIPTION_MANAGER_PROXY_H

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
#include <engine/subscription/subscriptionmanager.h>
#include <EntitlementProxy.h>
#include <services/plugin/PluginAPI.h>

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            class ORIGIN_PLUGIN_API SubscriptionManagerProxy : public QObject
            {
                Q_OBJECT
            public:
                // Is the subscription enabled
                Q_PROPERTY(bool enabled READ enabled);
                bool enabled() const;

                // The user has a subscription entitlement
                Q_PROPERTY(bool hasSubscription READ hasSubscription);
                bool hasSubscription() const;

                // The users subscription has expired (or actually is not active.)
                Q_PROPERTY(bool isExpired READ isExpired);
                bool isExpired() const;

				// The users subscription is active
				Q_PROPERTY(bool isActive READ isActive);
				bool isActive() const;

                Q_PROPERTY(QString status READ status);
                QString status() const;

                Q_PROPERTY(QString firstSignupDate READ firstSignupDate);
                QString firstSignupDate() const;

                // The subscription expiration date
                Q_PROPERTY(QDateTime expirationDate READ expirationDate);
                QDateTime expirationDate() const;

                // The amount of time in seconds remaining till subscription membership expires.
                Q_PROPERTY(QVariant timeRemaining READ timeRemaining);
                QVariant timeRemaining() const;

                // The amount of time in seconds remaining for offline play.
                Q_PROPERTY(long offlinePlayRemaining READ offlinePlayRemaining);
                long offlinePlayRemaining() const;

                Q_INVOKABLE bool isAvailableFromSubscription(const QString &offer) const;

            public:
                // The web widget framework destructs us so we need a public destructor
                // instead of a static destroy()
                ~SubscriptionManagerProxy();

                static SubscriptionManagerProxy * create();
                static SubscriptionManagerProxy * instance();

            signals:
                void updated();
                void entitlementUpgraded();
                void entitlementRemoved(const QString& offerId);

            private:
                SubscriptionManagerProxy();

            private:
                static SubscriptionManagerProxy * s_SubscriptionManagerProxyInstance;
            };

        }
    }
}

#endif //SUBSCRIPTION_MANAGER_PROXY_H
