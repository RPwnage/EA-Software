//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#ifndef ORIGINENTITLEMENT_H
#define ORIGINENTITLEMENT_H

#include <QObject>
#include <QList>
#include <QHash>
#include <QSharedPointer>
#include <QReadWriteLock>

#include "services/common/Accessor.h"

#include "services/publishing/CatalogDefinition.h"
#include "services/publishing/NucleusEntitlement.h"

#include "services/plugin/PluginAPI.h"

#include "engine/content/ContentTypes.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/login/User.h"
#include "engine/subscription/SubscriptionManager.h"

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            class LocalContent;

            enum EntitlementFilter
            {
                OwnedContent    = 0x01,
                UnownedContent  = 0x02,
                AllContent      = OwnedContent | UnownedContent,
            };

            class ORIGIN_PLUGIN_API Entitlement : public QObject
            {
                friend class BaseGamesController;
                friend class BaseGame;
                Q_OBJECT

            public:
                static EntitlementRef create(Services::Publishing::CatalogDefinitionRef def,
                    const Services::Publishing::NucleusEntitlementRef nucleusEntitlement = Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT);

            protected:
                Entitlement(ContentConfigurationRef def);
                void init(EntitlementRef ref);

            public:
                ACCESSOR(ContentController, contentController);

                const Services::Publishing::NucleusEntitlementRef primaryNucleusEntitlement() const;

                bool isOwned() const { return primaryNucleusEntitlement()->isValid(); }

                bool isSuppressed() { return m_suppressed; }

                /// \brief Returns the content configuration.
                ContentConfigurationRef contentConfiguration() const { return m_contentConfiguration; }

                const LocalContent* localContent() const { return m_localContent; }
                LocalContent* localContent() { return m_localContent; }

                void addParentEntitlement(EntitlementRef ref);
                void removeParentEntitlement(EntitlementRef ref);

                /// \brief TBD.
                /// \return TBD.
                EntitlementRef parent() const;

                /// \brief returns a list of parents (since child content refers to mastertitleID, it's possible to have more than one parent)
                EntitlementRefList parentList() const { QReadLocker lock(&m_parentLock); return m_parents; }

                /// \brief TBD.
                /// \return TBD.
                virtual EntitlementRefList children(EntitlementFilter filter = OwnedContent) const { return EntitlementRefList(); }

                virtual QStringList childEntitlementProductIds(EntitlementFilter filter = OwnedContent) const { return QStringList(); }

                /// \brief TBD.
                void ensureVisible();

                /// \brief Returns true if this entitlement has any unowned child content that was published less than 28 days ago.
                virtual bool hasNewUnownedChildContent() const { return false; }

                /// \brief Gets the 'Published Date' of latest available unowned child content
                virtual QDateTime latestChildPublishedDate() const { return QDateTime(); }

                /// \brief returns true if extra DLC is available through a subscription.
                virtual Subscription::SubscriptionUpgradeEnum isUpgradeAvailable() const { return Subscription::UPGRADEAVAILABLE_NONE; }

            signals:
                /// \brief TBD.
                void requestVisible();

                /// \brief Signals that a child has been added to this entitlement.
                void childAdded(Origin::Engine::Content::EntitlementRef);

                /// \brief Signals that a child has been removed from this entitlement.
                void childRemoved(Origin::Engine::Content::EntitlementRef);

            protected:
                void suppress(bool suppressed) { m_suppressed = suppressed; }

            protected:
                EntitlementWRef m_self;

            private:
                static bool compareParentEntitlements(const EntitlementRef& parentA, const EntitlementRef& parentB);

                mutable QReadWriteLock m_parentLock;
                EntitlementRefList m_parents;

                ContentConfigurationRef m_contentConfiguration;
                LocalContent* m_localContent;

                bool m_suppressed;
            };


            class BaseGame;
            typedef QSharedPointer<BaseGame> BaseGameRef;
            typedef QList<BaseGameRef> BaseGameList;
            typedef QSet<BaseGameRef> BaseGameSet;

            class ORIGIN_PLUGIN_API BaseGame : public Entitlement
            {
                friend class BaseGamesController;
                Q_OBJECT

            public:
                static BaseGameRef create(Services::Publishing::CatalogDefinitionRef def,
                    const Services::Publishing::NucleusEntitlementRef nucleusEntitlement = Services::Publishing::NucleusEntitlement::INVALID_ENTITLEMENT);

            protected:
                BaseGame(ContentConfigurationRef def);

            public:
                EntitlementRefList getPurchasableExtraContent() const { QReadLocker lock(&m_extraContentLock); return m_unownedExtraContent.values(); }

                /// \brief TBD.
                /// \return TBD.
                virtual EntitlementRefList children(EntitlementFilter filter = OwnedContent) const;
                virtual QStringList childEntitlementProductIds(EntitlementFilter filter = OwnedContent) const;

                virtual QDateTime latestChildPublishedDate() const;
                virtual bool hasNewUnownedChildContent() const;
                virtual Subscription::SubscriptionUpgradeEnum isUpgradeAvailable() const;

            protected:
                bool addExtraContent(EntitlementRef ref);
                bool disownExtraContent(EntitlementRef ref);
                bool deleteExtraContent(EntitlementRef ref);

            private:
                mutable QReadWriteLock m_extraContentLock;
                QHash<QString, EntitlementRef> m_ownedExtraContent;
                QHash<QString, EntitlementRef> m_unownedExtraContent;
            };
        } // namespace Content
    } // namespace Engine
} // namespace Origin

#endif // ORIGINENTITLEMENT_H
