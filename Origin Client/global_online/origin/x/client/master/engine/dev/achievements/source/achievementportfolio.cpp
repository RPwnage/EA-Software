///////////////////////////////////////////////////////////////////////////////
/// \file       achievementportfolio.cpp
/// This file contains the implementation of the AchievementPortfolio. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#include "services/achievements/AchievementServiceClient.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/common/XmlUtil.h"

#include "engine/achievements/achievementmanager.h"
#include "TelemetryAPIDLL.h"

using namespace Origin::Services::Achievements;
using namespace Origin::Services::Session;

#define ENABLE_META_ACHIEVEMENTS 0

namespace Origin
{
    namespace Engine
    {
        namespace Achievements
        {
            bool AchievementPortfolio::available() const
            {
                return mAvailable;
            }


            int AchievementPortfolio::total() const
            {
                int total = 0;
                foreach(AchievementSetRef set, mAchievementSets)
                {
                    total += set->achievements().size();
                }
                return total;
            }

            int AchievementPortfolio::granted() const
            {
                int granted = 0;
                foreach(AchievementSetRef set, mAchievementSets)
                {
                    foreach(AchievementRef a, set->achievements())
                    {
                        if(a->count()>0)
                        {
                            granted ++;
                        }
                    }
                }
                return granted;
            }

            int AchievementPortfolio::xp() const
            {
                return mXp;
            }

            int AchievementPortfolio::rp() const
            {
                return mRp;
            }

            QString AchievementPortfolio::personaId() const
            {
                return mPersonaId;
            }

            QString AchievementPortfolio::userId() const
            {
                return mUserId;
            }

            bool AchievementPortfolio::isUser() const
            {
                return mUserId == SessionService::instance()->nucleusUser(SessionService::instance()->currentSession());
            }

            void AchievementPortfolio::refresh()
            {
                refreshPoints();

                updateAchievementSets();
            }

            const QList<AchievementSetRef> &AchievementPortfolio::achievementSets() const
            {
                return mAchievementSets;
            }

            void AchievementPortfolio::pointsUpdateCompleted()
            {
                Origin::Services::Achievements::AchievementProgressQueryResponse* response = dynamic_cast<Origin::Services::Achievements::AchievementProgressQueryResponse * >(sender());

                if(response)
				{
					if(response->error() == Origin::Services::restErrorSuccess)
					{
						if(!mAvailable)
						{
							mAvailable = true;
							emit availableUpdated();
						}

						//int achieved = response->achieved();
						mXp = response->xp();
						mRp = response->rp();
						mNumAchievements = response->achieved();

						// For now we are not doing anything with achieved.
						// mAchieved = response->achieved();

						response->deleteLater();

						emit xpChanged(mXp);
						emit rpChanged(mRp);

                        serialize();

                        GetTelemetryInterface()->Metric_ACHIEVEMENT_ACHIEVED(mNumAchievements, mXp, mRp);
					}
					else
					{
						if(response->httpStatus() == Origin::Services::Http403ClientErrorForbidden )
						{
							mAvailable = false;
							emit availableUpdated();
						}
					}
				}
            }

            void AchievementPortfolio::refreshPoints()
            {
                AchievementProgressQueryResponse * resp = AchievementServiceClient::points(mPersonaId);

                if(resp != NULL)
                {
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(pointsUpdateCompleted()));
                }
            }

            void AchievementPortfolio::updateAchievementSets( const QStringList &achievementSets /*= QStringList()*/, 
                const QList<Engine::Content::EntitlementRef> &entitlements/*= QList<Engine::Content::EntitlementRef>()*/)
            {
                Q_ASSERT(achievementSets.size() == entitlements.size());

                int added = 0;

                QStringList updateQueue;
                QList<Engine::Content::EntitlementRef> entitlementQueue;
                QString personaId = mPersonaId;

                if(achievementSets.isEmpty())
                {
                    for(int i=0; i<mAchievementSets.size(); i++)
                    {
                        QString achievementSetId = mAchievementSets[i]->achievementSetId();

                        if(!updateQueue.contains(achievementSetId))
                        {
                            added++;
                            updateQueue.push_back(achievementSetId);
                            entitlementQueue.push_back(mAchievementSets[i]->entitlement());
                        }

#if ENABLE_META_ACHIEVEMENTS
                        if(!updateQueue.contains("META_" + achievementSetId))
                        {
                            added++;
                            updateQueue.push_back("META_" + achievementSetId);
                            entitlementQueue.push_back(mAchievementSets[i]->entitlement());
                        }
#endif
                    }
                }
                else
                {
                    for(int i=0; i<achievementSets.size(); i++)
                    {
                        if(!updateQueue.contains(achievementSets[i]))
                        {
                            added += 2;
                            updateQueue.push_back(achievementSets[i]);
                            entitlementQueue.push_back(entitlements[i]);
#if ENABLE_META_ACHIEVEMENTS
                            updateQueue.push_back("META_" + achievementSets[i]);
                            entitlementQueue.push_back(entitlements[i]);
#endif
                        }
                    }
                }

                if(mPersonaId.isEmpty())
                {
                    SessionRef session = SessionService::instance()->currentSession();

                    if(session)
                    {
                        personaId = SessionService::instance()->nucleusPersona(session);
                    }
                }

                if(added>0)
                {
                    while(updateQueue.size()>0)
                    {
                        QString achievementSetId = updateQueue.front();
                        Engine::Content::EntitlementRef entitlement = entitlementQueue.front();

                        updateQueue.pop_front();
                        entitlementQueue.pop_front();

                        if(!achievementSetId.isEmpty())
                        {
                            QStringList names = getAchievementSetIds(achievementSetId);

                            foreach(QString name, names)
                            {
                                mAchievementSetToEntitlementMap[name] = entitlement;
                            }

                            updateAchievementSet(achievementSetId);
                        }
                    }
                }
            }

            AchievementSetRef AchievementPortfolio::getAchievementSet(QString achievementSetId)
            {
                // The achievement set Id consists of [META_]FRANCISEID_MASTERTITLEID_SKUID_ITEMID the achievementSet can actually be defined as FRANCISEID_MASTERTITLEID
                // So we need to search the proper achievement set.

                AchievementSetRef set = mAchievementSetMap[achievementSetId];

                if(set.isNull())
                {
                    QStringList names = getAchievementSetIds(achievementSetId);

                    foreach(QString name, names)
                    {
                        set = mAchievementSetMap[name];

                        if(!set.isNull())
                        {
                            // add some additional links to the map.

                            Engine::Content::EntitlementWRef entitlement = mAchievementSetToEntitlementMap[name];

                            // Re-register the entitlement and set

                            QStringList setIdnames = getAchievementSetIds(achievementSetId);

                            foreach(QString setId, setIdnames)
                            {
                                mAchievementSetMap[setId] = set;
                                mAchievementSetToEntitlementMap[setId] = entitlement;
                            }
                            break;
                        }
                    }
                }
                return set;
            }

            void AchievementPortfolio::serialize(QDomElement &node, int version, bool bSave)
            {
                if(bSave)
                {
                    Origin::Util::XmlUtil::setInt(node, "xp", mXp);
                    Origin::Util::XmlUtil::setInt(node, "rp", mRp);

                    foreach(AchievementSetRef set, mAchievementSets)
                    {
                        // Only save to the offline cache if the achievement set is not deleted.
                        if(!set->deleted())
                        {
                            // Save portfolio
                            QDomElement as = Origin::Util::XmlUtil::newElement(node, "as");
                            Origin::Util::XmlUtil::setString(as, "id", set->achievementSetId());
                            Origin::Util::XmlUtil::setString(as, "name", set->displayName());
                            Origin::Util::XmlUtil::setString(as, "pf", set->platform());

                            set->serialize(as, version, bSave);
                        }
                    }
                }
                else
                {
                    mXp = Origin::Util::XmlUtil::getInt(node, "xp");
                    mRp = Origin::Util::XmlUtil::getInt(node, "rp");

                    for(QDomElement as = node.firstChildElement("as"); !as.isNull(); as = as.nextSiblingElement("as"))
                    {
                        QString setId = Origin::Util::XmlUtil::getString(as, "id");
                        QString displayName = Origin::Util::XmlUtil::getString(as, "name");
                        QString platform = Origin::Util::XmlUtil::getString(as, "pf");

                        if(!setId.isEmpty())
                        {
                            bool bCreated = false;

                            AchievementSetRef set = getAchievementSet(setId);

                            if(set.isNull())
                            {
                                set = createAchievementSet(setId, displayName, platform);
                                bCreated = true;
                            }

                            set->serialize(as, version, bSave);

                            if(bCreated) emit added(set);
                        }
                    }
                }
            }

            void AchievementPortfolio::serialize()
            {
                // Only serialize for the current user, and when there are no other updates coming.
                if(isUser() && mPendingUpdates.size() == 0)
                {                          
                    AchievementManager::instance()->serialize(true);
                }
            }


            void AchievementPortfolio::granted(server::AchievementNotificationT notification)
            {
                AchievementSetRef set = getAchievementSet(notification.data.prodid);

                if(!set.isNull())
                {
                    foreach (AchievementRef a, set->achievements())
                    {
                        if(a->id() == notification.data.achid)
                        {
                            updateAchievementSet(set->achievementSetId());

                            break;
                        }
                    }
                }

                // The point value is probably updated as well.
                refreshPoints();
            }

            void AchievementPortfolio::clear()
            {
                foreach (AchievementSetRef set, mAchievementSets)
                {
                    emit removed(set);
                    set->deleteLater();
                }

                mAchievementSetMap.clear();
                mAchievementSets.clear();

                mXp = 0;
                mRp = 0;
            }

            QStringList AchievementPortfolio::getAchievementSetIds(QString setId) const
            {
                QStringList names;

                QStringList keys = setId.split("_");

                do
                {
                    QString name = keys.join("_");
                    names.append(name);
                    keys.pop_back();
                }
                while(keys.size()>2 || (keys.size()>1 && keys[0] != "META"));

                return names;
            }


            AchievementSetRef AchievementPortfolio::createAchievementSet(const QString &achievementSetId, const QString &name, const QString &platform)
            {
                AchievementSetRef set = AchievementSetRef(new AchievementSet(achievementSetId, name, platform, mAchievementSetToEntitlementMap[achievementSetId].toStrongRef()));
                set->setSelf(set);

                QStringList names = getAchievementSetIds(achievementSetId);

                foreach(QString name, names)
                {
                    mAchievementSetMap[name] = set;
                }

                mAchievementSets.append(set);

                ORIGIN_VERIFY_CONNECT(set.data(), SIGNAL(achievementGranted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)),
                    this, SLOT(achievementGranted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)));

                return set; 
            }


            void AchievementPortfolio::achievementSetUpdateCompleted()
            {
                Origin::Services::Achievements::AchievementSetQueryResponse* response = dynamic_cast<Origin::Services::Achievements::AchievementSetQueryResponse * >(sender());

                if(response)
                {
                    if(response->httpStatus() == Origin::Services::Http200Ok)
                    {
                        if(!mAvailable)
                        {
                            mAvailable = true;
                            emit availableUpdated();
                        }

                        bool bCreated = false;
                        Origin::Engine::Content::EntitlementWRef e = mAchievementSetToEntitlementMap[response->achievementSet().setId];
                        AchievementSetRef set = getAchievementSet(response->achievementSet().setId);

                        if(set.isNull())
                        {
                            //ORIGIN_LOG_DEBUG << "Creating Achievement Set: " << response->achievementSet().setId;

                            set = createAchievementSet(response->achievementSet().setId);

                            if(!e.isNull())
                            {
                                Origin::Engine::Content::EntitlementRef entitlement = e.toStrongRef();
                                set->setExpansionOwnershipFromEntitlement(entitlement);
                            }

                            bCreated = true;
                        }
                        else
                        {
                            
                            if(!e.isNull())
                            {
                                Origin::Engine::Content::EntitlementRef entitlement = e.toStrongRef();

                                set->updateFromEntitlement(entitlement);
                            }
                        }

                        set->update(response->achievementSet().achievements, response->achievementSet().expansions, response->achievementSet().name, response->achievementSet().platform, mSuppressGranted);

                        if(bCreated) emit added(set);
                    }
                    else
                    {
                        if(response->httpStatus() == Origin::Services::Http403ClientErrorForbidden )
                        {
                            mAvailable = false;
                            emit availableUpdated();
                        }

                        AchievementSetRef set = getAchievementSet(response->achievementSet().setId);

                        if(!set.isNull())
                        {
                            set->setUpdated(false);
                        }
                    }

                    // Removed the response from the pending update list.
                    QMutexLocker lock(&mPendingUpdateGuard);
                    mPendingUpdates.removeOne(response);

                    response->deleteLater();

                    serialize();

                    if(mPendingUpdates.empty())
                    {
                        mSuppressGranted = false;
                    }
                }
            }

            void AchievementPortfolio::achievementSetsUpdateCompleted()
            {
                Origin::Services::Achievements::AchievementSetsQueryResponse* response = dynamic_cast<Origin::Services::Achievements::AchievementSetsQueryResponse * >(sender());

                if(response)
                {
                    if(response->httpStatus() == Origin::Services::Http200Ok)
                    {
                        if(!mAvailable)
                        {
                            mAvailable = true;
                            emit availableUpdated();
                        }

                        if(isUser())
                        {
                            foreach(Origin::Engine::Achievements::AchievementSetRef set, mAchievementSets)
                            {
                                set->setDeleted(true);
                            }


                            foreach(const Origin::Services::Achievements::AchievementSetT & s, response->achievementSets())
                            {
                                bool bCreated = false;

                                AchievementSetRef set = getAchievementSet(s.setId);

                                if(set.isNull())
                                {
                                    //ORIGIN_LOG_DEBUG << "Creating Achievement Set: " << s.setId;

                                    set = createAchievementSet(s.setId, s.name, s.platform);
                                    bCreated = true;
                                }

                                set->update(s.achievements, s.expansions, s.name, s.platform, mSuppressGranted);
                                set->setDeleted(false);

                                if(bCreated) emit added(set);
                            }
                        }
                        else
                        {
                            foreach(const Origin::Services::Achievements::AchievementSetT & s, response->achievementSets())
                            {
                                updateAchievementSet(s.setId);
                            }
                        }
                    }
                    else if(response->httpStatus() == Origin::Services::Http403ClientErrorForbidden )
                    {
                        mAvailable = false;
                        emit availableUpdated();
                    }

                    response->deleteLater();

                    serialize();
                }
            }

            void AchievementPortfolio::achievementGranted(Origin::Engine::Achievements::AchievementSetRef set, Origin::Engine::Achievements::AchievementRef achievement)
            {
                // Only fire this signal if it is for the current user. We do not want to show notification for other users achievements (Yet...)
                if(isUser())
                {
                    emit granted(set, achievement);
                }
            }

            void AchievementPortfolio::updateAchievementSet(const QString &achievementSetId)
            {
                QMutexLocker lock(&mPendingUpdateGuard);

                foreach(Origin::Services::Achievements::AchievementSetQueryResponse * resp, mPendingUpdates)
                {
                    if(resp->achievementSet().setId == achievementSetId)
                    {
                        // Already an update in progress.
                        return;
                    }
                }

                // This can be optimized later, as we may not need to ask for localized strings and metadata every time.
                AchievementSetQueryResponse * resp = AchievementServiceClient::achievements(mPersonaId, achievementSetId, QLocale().name(), true);

                mPendingUpdates.append(resp);

                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(achievementSetUpdateCompleted()));
            }

            AchievementPortfolio::AchievementPortfolio(const QString &userId, const QString &personaId, QObject * parent) : 
                QObject(parent),
                mAvailable(false),
                mSuppressGranted(true),
                mXp(0),
                mRp(0),
                mUserId(userId),
                mPersonaId(personaId)
            {
                // Get all the users achievements.

                AchievementSetsQueryResponse * resp = AchievementServiceClient::achievementSets(mPersonaId, QLocale().name(), true);

                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(achievementSetsUpdateCompleted()));
            }

            AchievementPortfolio::~AchievementPortfolio()
            {
                clear();
            }
        }
    }
}
