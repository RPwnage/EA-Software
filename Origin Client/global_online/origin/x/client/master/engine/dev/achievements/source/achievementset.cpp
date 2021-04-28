///////////////////////////////////////////////////////////////////////////////
/// \file       achievementset.cpp
/// This file contains the implementation of an AchievementSet. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#include "services/platform/PlatformService.h"
#include "engine/achievements/achievementset.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"
#include "services/common/XmlUtil.h"
#include "TelemetryAPIDLL.h"

using namespace Origin::Services::PlatformService;

namespace Origin
{
    namespace Engine
    {
        namespace Achievements
        {

            AchievementSet::AchievementSet(const QString &achievementSetId, const QString &name, const QString &platform, Engine::Content::EntitlementRef entitlement) :
                mAchievementSetId(achievementSetId),
                mDisplayName(name),
                mEntitlement(entitlement),
                mDeleted(false)
            {
                if(!platform.isEmpty() && platform != "null")
                {
                    mPlatform = platform;
                    mDisplayName = name;
                    addExpansion("", mDisplayName, true);
                }
                else
                {
                    if(!entitlement.isNull())
                    {

                        mDisplayName = entitlement->contentConfiguration()->displayName();
                        addExpansion("", mDisplayName, true);

                        setPlatformFromEntitlement(entitlement);
                        setExpansionOwnershipFromEntitlement(entitlement);
                    }
                    else
                    {
                        if(achievementSetId.endsWith("origin"))
                        {
                            mDisplayName = QString(tr("application_name"));
                            mPlatform = "PC,MAC";
                        }
                        else
                        {
                            mPlatform = "OTHER";
                            mDisplayName = name;
                        }
                        addExpansion("", mDisplayName, true);
                    }
                }
            }

            void AchievementSet::setSelf(AchievementSetRef self)
            {
                Q_ASSERT(this == self.data());

                mSelf = self;
            }

            int AchievementSet::achieved() const
            {
                int cnt = 0;

                foreach(AchievementRef a, achievements())
                {
                    if(a->count() > 0)
                    {
                        cnt++;
                    }
                }
                return cnt;
            }

            int AchievementSet::totalXp() const
            {
                int xp = 0;

                foreach(Origin::Engine::Achievements::AchievementRef a, achievements())
                {
                    xp += a->xp();
                }

                return xp;
            }

            int AchievementSet::totalRp() const
            {
                int rp = 0;

                foreach(Origin::Engine::Achievements::AchievementRef a, achievements())
                {
                    rp += a->rp();
                }
                return rp;
            }

            int AchievementSet::earnedXp() const
            {
                int xp = 0;

                foreach(Origin::Engine::Achievements::AchievementRef a, achievements())
                {
                    // experience points are counted per achievements
                    if(a->count()>0)
                    {
                        xp += a->count() * a->xp();
                    }
                }

                return xp;
            }

            int AchievementSet::earnedRp() const
            {
                int rp = 0;

                foreach(Origin::Engine::Achievements::AchievementRef a, achievements())
                {
                    // reward points only apply the first time...
                    if(a->count()>0)
                    {
                        rp += a->rp();
                    }
                }
                return rp;
            }

            AchievementSetExpansionList::iterator AchievementSet::findExpansion(QString expansionId)
            {
                for(AchievementSetExpansionList::iterator i=mExpansions.begin(); i!=mExpansions.end(); i++)
                {
                    if(i->id == expansionId)
                        return i;
                }

                return mExpansions.end();
            }

            void AchievementSet::addExpansion(QString expansionId, QString expansionName, bool owned, int order)
            {
                AchievementSetExpansionList::iterator exp = findExpansion(expansionId);

                AchievementSetExpansion newExpansion;

                if(exp == mExpansions.end())
                {
                    newExpansion.id = expansionId;
                    newExpansion.name = expansionName;
                    newExpansion.owned = owned;
                    newExpansion.order = order;

                    mExpansions.push_back(newExpansion);

                    emit expansionAdded(expansionId);
                }
                else
                {
                    if(!owned)
                    {
                        exp->name = expansionName;
                    }
                    else
                    {
                        exp->owned = owned;
                    }

                    if(order>=0)
                    {
                        exp->order = order;
                    }

                    // Mark the expansion as not deleted, so it continues to exist in the offline cache.
                    exp->deleted = false;
                }
            }

            void AchievementSet::serialize(QDomElement &node, int version, bool bSave)
            {
                if(bSave)
                {
                    // Write the expansions.
                    foreach(AchievementSetExpansion exp, mExpansions)
                    {
                        if(!exp.deleted)
                        {
                            QDomElement e = Origin::Util::XmlUtil::newElement(node, "exp");

                            Origin::Util::XmlUtil::setString(e, "id", exp.id);
                            Origin::Util::XmlUtil::setString(e, "name", exp.name);
                            Origin::Util::XmlUtil::setBool(e, "owned", exp.owned);
                            Origin::Util::XmlUtil::setInt(e, "order", exp.order);
                        }
                    }

                    foreach(AchievementRef ach, mAchievements)
                    {
                        // Save achievement
                        if(!ach->deleted())
                        {
                            QDomElement a = Origin::Util::XmlUtil::newElement(node, "a");
                            Origin::Util::XmlUtil::setString(a, "ix", ach->id());
                            ach->serialize(a, version, bSave);
                        }
                    }
                    
                    Origin::Util::XmlUtil::setBool(node, "updated", mUpdated);
                    Origin::Util::XmlUtil::setDateTime(node, "lastUpdated", mLastUpdated);
                }
                else
                {
                    // Read the expansions.
                    for(QDomElement e = node.firstChildElement("exp"); !e.isNull(); e = e.nextSiblingElement("exp"))
                    {
                        QString id = Origin::Util::XmlUtil::getString(e, "id");

                        AchievementSetExpansionList::iterator exp = findExpansion(id);

                        if(exp == mExpansions.end())
                        {
                            AchievementSetExpansion newExpansion;
                            newExpansion.id = id;
                            newExpansion.name = Origin::Util::XmlUtil::getString(e, "name");
                            newExpansion.owned = Origin::Util::XmlUtil::getBool(e, "owned", false);
                            newExpansion.order = Origin::Util::XmlUtil::getInt(e, "order", -1);

                            mExpansions.push_back(newExpansion);
                        }
                        else
                        {
                            exp->name = Origin::Util::XmlUtil::getString(e, "name");
                            exp->owned |= Origin::Util::XmlUtil::getBool(e, "owned", false);
                        }
                    }

                    for(QDomElement a = node.firstChildElement("a"); !a.isNull(); a = a.nextSiblingElement("a"))
                    {
                        QString id = Origin::Util::XmlUtil::getString(a, "ix");

                        
                        AchievementRef ach = mAchievementMap[id];

                        if(ach.isNull())
                        {
                            ach = createAchievement(id);
                        }

                        ach->serialize(a, version, bSave);

                        emit added(ach);
                    }

                    mUpdated = Origin::Util::XmlUtil::getBool(node, "updated", false);
                    mLastUpdated = Origin::Util::XmlUtil::getDateTime(node, "lastUpdated");
                }
            }



            void AchievementSet::update(const Origin::Services::Achievements::AchievementMap & achievements, const Origin::Services::Achievements::ExpansionList &expansions, const QString &name, const QString &platform, bool bSuppressGranted)
            {
                if(!name.isEmpty())
                    mDisplayName = name;

                if(!platform.isEmpty())
                    mPlatform = platform;

                //ORIGIN_LOG_DEBUG << "Adding new achievements count: " << achievements.size();

                for(int i=0; i<mExpansions.size(); i++)
                {
                    // Mark the expansion as deleted. If the expansion exists it will be readded below.
                    mExpansions[i].deleted = true;
                }

                addExpansion("", mDisplayName, true, 0);

                for(int i=0; i<expansions.size(); i++)
                {
                    addExpansion(expansions[i].expansionId, expansions[i].expansionName, false, i+1);
                }

                foreach(Origin::Engine::Achievements::AchievementRef achievement, mAchievements)
                {
                    // Mark deleted, unless there is an update for it.
                    achievement->setDeleted(true);
                }

                for(Origin::Services::Achievements::AchievementMap::const_iterator pair = achievements.constBegin(); pair != achievements.constEnd(); pair++)
                {
                    //ORIGIN_LOG_DEBUG << "Adding achievement: " << pair.key() << " name: " << pair->name;

                    AchievementRef achievement;

                    if(mAchievementMap.contains(pair.key()))
                    {
                        achievement = mAchievementMap[pair.key()];
                        achievement->update(pair.value(), bSuppressGranted);
                        achievement->setDeleted(false);
                    }
                    else
                    {
                        achievement = createAchievement(pair.key(), pair.value());
                        
                        emit added(achievement);
                    }
                }

                mLastUpdated = QDateTime::currentDateTimeUtc();


                if(!entitlement().isNull())
                {
                    GetTelemetryInterface()->Metric_ACHIEVEMENT_ACHIEVED_PER_GAME(
                        entitlement()->contentConfiguration()->productId().toLatin1(),
                        mAchievementSetId.toLatin1(),
                        achieved(),
                        earnedXp(),
                        earnedRp());
                }

                setUpdated(true);
            }

            void AchievementSet::updateFromEntitlement(Origin::Engine::Content::EntitlementRef entitlement)
            {
                Origin::Engine::Content::EntitlementRef e = mEntitlement.toStrongRef();

                if(e != entitlement)
                {
                    mEntitlement = entitlement;

                    if(mDisplayName.isEmpty())
                        mDisplayName = entitlement->contentConfiguration()->displayName();

                    setPlatformFromEntitlement(entitlement);
                }

                // update expansion ownership
                setExpansionOwnershipFromEntitlement(entitlement);
            }

            AchievementRef AchievementSet::createAchievement(QString id, const Origin::Services::Achievements::AchievementT & achievement)
            {
                AchievementRef a = AchievementRef(new Achievement(id, achievement, this));
                a->setSelf(a);

                ORIGIN_VERIFY_CONNECT(a.data(), SIGNAL(granted(Origin::Engine::Achievements::AchievementRef)), this, SLOT(granted(Origin::Engine::Achievements::AchievementRef)));

                mAchievementMap[id] = a;
                mAchievements.append(a);

                return a;
            }

            AchievementRef AchievementSet::createAchievement(QString id)
            {
                AchievementRef a = AchievementRef(new Achievement(id, this));
                a->setSelf(a);

                ORIGIN_VERIFY_CONNECT(a.data(), SIGNAL(granted(Origin::Engine::Achievements::AchievementRef)), this, SLOT(granted(Origin::Engine::Achievements::AchievementRef)));

                mAchievementMap[id] = a;
                mAchievements.append(a);

                return a;
            }

            const QString &AchievementSet::achievementSetId() const 
            {
                return mAchievementSetId;
            }

            Origin::Engine::Content::EntitlementRef AchievementSet::entitlement() const 
            {
                return mEntitlement.toStrongRef();
            }

            const QList<AchievementRef> & AchievementSet::achievements() const
            {
                return mAchievements;
            }

            const QList<AchievementSetExpansion> & AchievementSet::expansions() const
            {
                return mExpansions;
            }

            const QString &AchievementSet::platform() const
            {
                return mPlatform;
            }

            const QString &AchievementSet::displayName() const
            {
                return mDisplayName;
            }

            void AchievementSet::granted(AchievementRef achievement)
            {
                // Make sure that self is set.
                Q_ASSERT(!mSelf.isNull());

                emit achievementGranted(mSelf, achievement);
            }

            void AchievementSet::setPlatformFromEntitlement( Engine::Content::EntitlementRef entitlement )
            {
                QStringList platforms;
                foreach(OriginPlatform p, entitlement->contentConfiguration()->platformsSupported())
                {
                    switch(p)
                    {
                    case PlatformWindows:
                        platforms.append("PC");
                        break;
                    case PlatformMacOS:
                        platforms.append("MAC");
                        break;
                    default:
                        platforms.append("OTHER");
                        break;
                    }
                }
                mPlatform = platforms.join(",");

                Q_ASSERT(!mPlatform.isEmpty());
            }

            void AchievementSet::setExpansionOwnershipFromEntitlement( Engine::Content::EntitlementRef entitlement )
            {
                QList<Engine::Content::EntitlementRef> childEntitlements = entitlement->children();

                foreach(Engine::Content::EntitlementRef e, childEntitlements)
                {
                    addExpansion(e->contentConfiguration()->financeId(), e->contentConfiguration()->displayName(), true);
                }
            }
        }
    }
}

