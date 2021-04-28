//    Copyright (c) 2014-2015, Electronic Arts
//    All rights reserved.
//    Author: Kevin Wertman

#include "engine/content/Entitlement.h"
#include "services/log/LogService.h"
#include "engine/subscription/SubscriptionManager.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"

#include "engine/content/ContentController.h"
#include "engine/content/CatalogDefinitionController.h"

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            EntitlementRef Entitlement::create(Services::Publishing::CatalogDefinitionRef def,
                const Services::Publishing::NucleusEntitlementRef nucleusEntitlement)
            {
                ContentConfigurationRef configuration = ContentConfiguration::create(def, nucleusEntitlement);
                EntitlementRef ref(new Entitlement(configuration));

                ref->init(ref);

                return ref;
            }

            Entitlement::Entitlement(ContentConfigurationRef def)
                : m_contentConfiguration(def)
                , m_localContent(nullptr)
                , m_suppressed(false)
            {
            }

            void Entitlement::init(EntitlementRef ref)
            {
                m_self = ref;
                m_contentConfiguration->setEntitlement(ref);
                setContentController(ContentController::currentUserContentController());

                // Extra content needs to know about their parents before having its local content initialized
                // so it can know where the install directory, etc are.
                if (m_contentConfiguration->canBeExtraContent())
                {
                    const BaseGameSet &baseGames = contentController()->baseGamesController()->baseGamesByChild(m_self);
                    for (auto baseGame = baseGames.begin(); baseGame != baseGames.end(); ++baseGame)
                    {
                        addParentEntitlement(*baseGame);
                    }
                }

                m_localContent = new LocalContent(contentController()->user(), m_self.toStrongRef());
                //we need to make this call after the accessors for local content are setup
                m_localContent->init();

                m_contentConfiguration->checkVersionMismatch();

                // Suppress immediately if the user owns one of the suppressing offers.
                foreach (const QString& offerId, primaryNucleusEntitlement()->suppressedBy())
                {
                    if (contentController() && !contentController()->nucleusEntitlementController()->entitlements(offerId).isEmpty())
                    {
                        suppress(true);
                        break;
                    }
                }
            }

            EntitlementRef Entitlement::parent() const
            {
                QReadLocker lock(&m_parentLock);
                if(m_parents.isEmpty())
                {
                    EntitlementRef noparent(NULL);
                    return noparent;
                }
                else
                {
                    return m_parents.at(0);
                }
            }

            void Entitlement::addParentEntitlement(EntitlementRef ref)
            {
                QWriteLocker lock(&m_parentLock);

                // Make sure we aren't adding the same parent twice...
                // FGPE can't be a parent of other FGPE
                if (-1 == m_parents.indexOf(ref) &&
                    (Services::Publishing::OriginDisplayTypeFullGamePlusExpansion != m_contentConfiguration->originDisplayType() ||
                    Services::Publishing::OriginDisplayTypeFullGamePlusExpansion != ref->contentConfiguration()->originDisplayType()))
                {
                    m_parents.push_back(ref);
                    qStableSort(m_parents.begin(), m_parents.end(), Engine::Content::Entitlement::compareParentEntitlements);
                }
            }

            void Entitlement::removeParentEntitlement(EntitlementRef ref)
            {
                QWriteLocker lock(&m_parentLock);
                m_parents.removeOne(ref);
            }

            bool Entitlement::compareParentEntitlements(const EntitlementRef& parentA, const EntitlementRef& parentB)
            {
                bool parentAWins = true;

                const Services::Publishing::OriginDisplayType parentADisplayType = parentA->contentConfiguration()->originDisplayType();
                const Services::Publishing::OriginDisplayType parentBDisplayType = parentB->contentConfiguration()->originDisplayType();

                const int parentARank = parentA->contentConfiguration()->rank();
                const int parentBRank = parentB->contentConfiguration()->rank();

                if (parentADisplayType != parentBDisplayType)
                {
                    parentAWins = (parentADisplayType == Services::Publishing::OriginDisplayTypeFullGame);
                }
                else if (parentARank != parentBRank)
                {
                    parentAWins = (parentARank > parentBRank);
                }

                return parentAWins;
            }

            const Origin::Services::Publishing::NucleusEntitlementRef Entitlement::primaryNucleusEntitlement() const
            {
                return m_contentConfiguration->nucleusEntitlement();
            }

            void Entitlement::ensureVisible()
            {
                emit requestVisible();
            }


            BaseGameRef BaseGame::create(Services::Publishing::CatalogDefinitionRef def,
                const Services::Publishing::NucleusEntitlementRef nucleusEntitlement)
            {
                ContentConfigurationRef configuration = ContentConfiguration::create(def, nucleusEntitlement);
                BaseGameRef ref(new BaseGame(configuration));

                ref->init(ref);

                return ref;
            }

            BaseGame::BaseGame(ContentConfigurationRef def)
                : Entitlement(def)
            {
            }

            EntitlementRefList BaseGame::children(EntitlementFilter filter /*= OwnedContent*/) const
            {
                QReadLocker lock(&m_extraContentLock);

                EntitlementRefList childEntitlements;

                if (filter & OwnedContent)
                {
                    childEntitlements.append(m_ownedExtraContent.values());
                }

                if (filter & UnownedContent)
                {
                    childEntitlements.append(m_unownedExtraContent.values());
                }

                return childEntitlements;
            }

            QStringList BaseGame::childEntitlementProductIds(EntitlementFilter filter /*= OwnedContent*/) const
            {
                QReadLocker lock(&m_extraContentLock);

                QStringList productIds;

                if (filter & OwnedContent)
                {
                    productIds.append(m_ownedExtraContent.keys());
                }

                if (filter & UnownedContent)
                {
                    productIds.append(m_unownedExtraContent.keys());
                }

                return productIds;
            }

            bool BaseGame::addExtraContent(EntitlementRef extraContent)
            {
                QWriteLocker lock(&m_extraContentLock);

                const QString &productId = extraContent->contentConfiguration()->productId();

                // Base games + expansions cannot be parents of other base games plus expansions
                if (contentConfiguration()->originDisplayType() == extraContent->contentConfiguration()->originDisplayType())
                {
                    return false;
                }

                if (extraContent->isOwned())
                {
                    if (m_ownedExtraContent.contains(productId))
                    {
                        return false;
                    }

                    extraContent->addParentEntitlement(m_self);
                    m_ownedExtraContent.insert(productId, extraContent);
                    if (m_unownedExtraContent.contains(productId))
                    {
                        ORIGIN_ASSERT(m_unownedExtraContent[productId] == extraContent);
                        m_unownedExtraContent.remove(productId);
                    }
                    else
                    {
                        lock.unlock();
                        emit childAdded(extraContent);
                    }

                    return true;
                }
                else if (extraContent->contentConfiguration()->purchasable())
                {
                    if (m_unownedExtraContent.contains(productId))
                    {
                        return false;
                    }

                    // To show unowned, unpublished offers, the offer must be flagged as store-preview content and the parent must have special permissions.
                    // This must be calculated here instead of ContentConfiguration::purchasable() because the extracontent may have multiple parent base games
                    // and some parents may have special permissions while others do not.
                    const bool showStorePreview = Services::readSetting(Services::SETTING_AddonPreviewAllowed).toQVariant().toBool() &&
                        contentConfiguration()->originPermissions() >= Services::Publishing::OriginPermissions1102;
                    if (!extraContent->contentConfiguration()->published() && !showStorePreview)
                    {
                        return false;
                    }

                    extraContent->addParentEntitlement(m_self);
                    m_unownedExtraContent.insert(productId, extraContent);

                    lock.unlock();
                    emit childAdded(extraContent);

                    return true;
                }
                else
                {
                    return false;
                }
            }

            bool BaseGame::disownExtraContent(EntitlementRef extraContent)
            {
                QWriteLocker lock(&m_extraContentLock);

                const QString &productId = extraContent->contentConfiguration()->productId();
                if (!m_ownedExtraContent.contains(productId))
                {
                    return false;
                }

                m_ownedExtraContent.remove(productId);
                m_unownedExtraContent.insert(productId, extraContent);

                return true;
            }

            bool BaseGame::deleteExtraContent(EntitlementRef extraContent)
            {
                QWriteLocker lock(&m_extraContentLock);

                const QString &productId = extraContent->contentConfiguration()->productId();

                if (m_ownedExtraContent.contains(productId))
                {
                    m_ownedExtraContent.remove(productId);
                    extraContent->removeParentEntitlement(m_self);

                    lock.unlock();
                    emit childRemoved(extraContent);

                    return true;
                }
                else if (m_unownedExtraContent.contains(productId))
                {
                    m_unownedExtraContent.remove(productId);
                    extraContent->removeParentEntitlement(m_self);

                    lock.unlock();
                    emit childRemoved(extraContent);

                    return true;
                }
                else
                {
                    return false;
                }
            }

            Origin::Engine::Subscription::SubscriptionUpgradeEnum BaseGame::isUpgradeAvailable() const
            {
                if (contentConfiguration()->itemSubType() != Services::Publishing::ItemSubTypeAlpha &&
                    contentConfiguration()->itemSubType() != Services::Publishing::ItemSubTypeBeta &&
                    contentConfiguration()->itemSubType() != Services::Publishing::ItemSubTypeDemo &&
                    contentConfiguration()->itemSubType() != Services::Publishing::ItemSubTypeNonOrigin && 
                    !contentConfiguration()->suppressVaultUpgrade())
                {
                    return Subscription::SubscriptionManager::instance()->isUpgradeAvailable(contentConfiguration()->masterTitleId(), contentConfiguration()->rank());
                }
                else
                {
                    return Subscription::UPGRADEAVAILABLE_NONE;
                }
            }

            QDateTime BaseGame::latestChildPublishedDate() const
            {
                QReadLocker lock(&m_extraContentLock);

                QDateTime latestChildPublishedDate; // invalid

                if (contentConfiguration()->addonsAvailable())
                {
                    foreach (const EntitlementRef unownedChild, m_unownedExtraContent.values())
                    {
                        ContentConfigurationRef childConfiguration = unownedChild->contentConfiguration();
                        if (childConfiguration->publishedDate().isValid())
                        {
                            QDateTime publishedDate = childConfiguration->publishedDate().toLocalTime();

                            if (latestChildPublishedDate.isNull() || publishedDate > latestChildPublishedDate)
                            {
                                latestChildPublishedDate = publishedDate;
                            }
                        }
                    }
                }

                return latestChildPublishedDate;
            }

            bool BaseGame::hasNewUnownedChildContent() const
            {
                QReadLocker lock(&m_extraContentLock);

                foreach (const EntitlementRef unownedChild, m_unownedExtraContent.values())
                {
                    if (unownedChild->contentConfiguration()->newlyPublished())
                    {
                        return true;
                    }
                }

                return false;
            }
        } // namespace Content
    } // namespace Engine
} // namespace Origin
