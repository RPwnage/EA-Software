///////////////////////////////////////////////////////////////////////////////
/// \file       achievement.cpp
/// This file contains the implementation of an Achievement. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#include "services/common/XmlUtil.h"

#include "engine/achievements/achievement.h"
#include "engine/achievements/achievementset.h"
#include "TelemetryAPIDLL.h"
#include "services/network/NetworkAccessManager.h"
#include "services/debug/DebugService.h"

#define ENABLE_ACHIEVEMENT_IMAGE_CACHE 0

namespace Origin
{
    namespace Engine
    {
        namespace Achievements
        {
            Achievement::Achievement(QString id, const Origin::Services::Achievements::AchievementT &achievement, AchievementSet * parent) :
                QObject(parent), 
                mId(id),
                mDeleted(false)
            {
                // Only record the time if there is progress on the achievement.
                if(achievement.p != 0 || achievement.cnt != 0)
                {
                    if(achievement.u != -1)
                    {
                        mUpdateTime = QDateTime::fromMSecsSinceEpoch(achievement.u * 1000ULL);
                    }

                    if(achievement.e != -1)
                    {
                        mExpirationTime = QDateTime::fromMSecsSinceEpoch(achievement.e * 1000ULL);
                    }
                }

                mProgress       = achievement.p;
                mTotal          = achievement.t;
                mCount          = achievement.cnt;
                mXp             = achievement.xp;
                mRp             = achievement.rp;
                mXp_t           = achievement.xp_t;
                mRp_t           = achievement.rp_t;
                mName           = achievement.name;
                mDescription    = achievement.desc;
                mHowto          = achievement.howto;
                mImageName      = achievement.img;
                mIcons          = achievement.icons;

                if(achievement.hasExp)
                {
                    mExpansionId = achievement.expId;
                    parent->addExpansion(achievement.expId, achievement.expName);
                }
                else
                {
                    parent->addExpansion("", parent->displayName());
                }

                mAchievementSet = parent;

                updateImageCache();
            }

            Achievement::Achievement(QString id, AchievementSet * parent) : 
                QObject(parent), 
                mId(id),
                mDeleted(false)
            {
                mAchievementSet = parent;
            }

            void Achievement::setSelf(AchievementRef self)
            {
                Q_ASSERT(this == self.data());

                mSelf = self;
            }

            void Achievement::serialize(QDomElement &elem, int version, bool bSave)
            {
                if(bSave)
                {
                    Origin::Util::XmlUtil::setDateTime(elem, "u", mUpdateTime);
                    Origin::Util::XmlUtil::setDateTime(elem, "e", mExpirationTime);

                    Origin::Util::XmlUtil::setInt(elem, "p", mProgress);
                    Origin::Util::XmlUtil::setInt(elem, "t", mTotal);
                    Origin::Util::XmlUtil::setInt(elem, "c", mCount);
                    Origin::Util::XmlUtil::setInt(elem, "xp", mXp);
                    Origin::Util::XmlUtil::setInt(elem, "rp", mRp);
                    Origin::Util::XmlUtil::setString(elem, "xp_t", mXp_t);
                    Origin::Util::XmlUtil::setString(elem, "rp_t", mRp_t);

                    Origin::Util::XmlUtil::setString(elem, "name", mName);
                    Origin::Util::XmlUtil::setString(elem, "desc", mDescription);
                    Origin::Util::XmlUtil::setString(elem, "howto", mHowto);
                    Origin::Util::XmlUtil::setString(elem, "img", mImageName);
                    Origin::Util::XmlUtil::setString(elem, "expId", mExpansionId);

                    for(QHash<int, QString>::iterator icon = mIcons.begin(); icon!= mIcons.end(); icon++)
                    {
                          QDomElement i = Origin::Util::XmlUtil::newElement(elem, "icon");

                          Origin::Util::XmlUtil::setInt(i, "size", icon.key());
                          Origin::Util::XmlUtil::setString(i, "url", icon.value());
                    }
                }
                else
                {
                    mUpdateTime = Origin::Util::XmlUtil::getDateTime(elem, "u");
                    mExpirationTime = Origin::Util::XmlUtil::getDateTime(elem, "e");

                    mProgress   = Origin::Util::XmlUtil::getInt(elem, "p");
                    mTotal      = Origin::Util::XmlUtil::getInt(elem, "t");
                    mCount      = Origin::Util::XmlUtil::getInt(elem, "c");
                    mXp         = Origin::Util::XmlUtil::getInt(elem, "xp");
                    mRp         = Origin::Util::XmlUtil::getInt(elem, "rp");
                    mXp_t       = Origin::Util::XmlUtil::getString(elem, "xp_t");
                    if(mXp_t.isEmpty())mXp_t = QString::number(mXp);    // Values may not be in the stored data.
                    mRp_t       = Origin::Util::XmlUtil::getString(elem, "rp_t");
                    if(mRp_t.isEmpty())mRp_t = QString::number(mRp);    // Values may not be in the stored data.

                    mName       = Origin::Util::XmlUtil::getString(elem, "name");
                    mDescription = Origin::Util::XmlUtil::getString(elem, "desc");
                    mHowto      = Origin::Util::XmlUtil::getString(elem, "howto");
                    mImageName  = Origin::Util::XmlUtil::getString(elem, "img");
                    mExpansionId = Origin::Util::XmlUtil::getString(elem, "expId");

                    mIcons.clear();

                    for(QDomElement i = elem.firstChildElement("icon"); !i.isNull(); i = i.nextSiblingElement("icon"))
                    {
                        int size = Origin::Util::XmlUtil::getInt(i, "size");
                        QString url = Origin::Util::XmlUtil::getString(i, "url");

                        mIcons[size] = url;
                    }
                }
            }


            void Achievement::update(const Origin::Services::Achievements::AchievementT & achievement, bool bSuppressGranted)
            {
                bool bChanged = false;
                bool bCountChanged = false;
                bool bProgressChanged = false;

                // Only record the time if there is progress on the achievement.
                if(achievement.p != 0 || achievement.cnt != 0)
                {
                    QDateTime t;

                    if(achievement.u != -1)
                    {
                        t = QDateTime::fromMSecsSinceEpoch(achievement.u * 1000ULL);

                        if(t != mUpdateTime)
                        {
                            mUpdateTime = t;
                            bChanged = true;
                        }
                    }
                    else
                    {
                        if(mUpdateTime.isValid())
                        {
                            mUpdateTime = QDateTime();
                            bChanged = true;
                        }
                    }
                    

                    if(achievement.e != -1)
                    {
                        t = QDateTime::fromMSecsSinceEpoch(achievement.e * 1000ULL);

                        if(t != mExpirationTime)
                        {
                            mExpirationTime = t;
                            bChanged = true;
                        }
                    }
                    else
                    {
                        if(mExpirationTime.isValid())
                        {
                            mExpirationTime = QDateTime();
                            bChanged = true;
                        }
                    }
                }

                if(mProgress != achievement.p)
                {
                    mProgress = achievement.p;
                    bProgressChanged = true;
                }


                if(mTotal != achievement.t)
                {
                    mTotal = achievement.t;
                    bProgressChanged = true;
                }

                if(mCount != achievement.cnt)
                {
                    mCount = achievement.cnt;
                    bProgressChanged = true;
                    bCountChanged = true;
                }

                if(mXp != achievement.xp && achievement.xp != 0)
                {
                    mXp = achievement.xp;
                    bChanged = true;
                }

                if(mRp != achievement.rp && achievement.rp != 0)
                {
                    mRp = achievement.rp;
                    bChanged = true;
                }

                if(mXp_t != achievement.xp_t && achievement.xp_t != "")
                {
                    mXp_t = achievement.xp_t;
                    bChanged = true;
                }

                if(mRp_t != achievement.rp_t && achievement.rp_t != "")
                {
                    mRp_t = achievement.rp_t;
                    bChanged = true;
                }

                if(mName != achievement.name && !achievement.name.isEmpty())
                {
                    mName = achievement.name;
                    bChanged = true;
                }

                if(mDescription != achievement.desc)
                {
                    mDescription = achievement.desc;
                    bChanged = true;
                }
                
                if(mHowto != achievement.howto)
                {
                    mHowto = achievement.howto;
                    bChanged = true;
                }

                mIcons = achievement.icons;

                if(achievement.hasExp)
                {
                    mAchievementSet->addExpansion(achievement.expId, achievement.expName);
                    mExpansionId = achievement.expId;
                }
                
                if(bChanged)
                {
                    emit updated();
                    updateImageCache();
                }

                if(bProgressChanged)
                {
                    emit progressChanged(mProgress, mTotal, mCount);
                }

                if(bCountChanged)
                {
                    // Make sure that self is set.
                    Q_ASSERT(!mSelf.isNull());

                    if(!mAchievementSet->entitlement().isNull())
                    {
                        GetTelemetryInterface()->Metric_ACHIEVEMENT_GRANT_DETECTED(
                            mAchievementSet->entitlement()->contentConfiguration()->productId().toLatin1(),
                            mAchievementSet->achievementSetId().toLatin1(),
                            mId.toLatin1());
                    }

                    if(!bSuppressGranted)
                    {
                        emit granted(mSelf);
                    }
                }
            }


            QString Achievement::id() const
            {
                return mId;
            }

            QString Achievement::expansionId() const
            {
                return mExpansionId;
            }

            QDateTime Achievement::updateTime() const
            {
                return mUpdateTime;
            }

            QDateTime Achievement::expirationTime() const 
            {
                return mExpirationTime;
            }

            int Achievement::progress() const
            {
                return mProgress;
            }

            int Achievement::total() const
            {
                return mTotal;
            }

            int Achievement::count() const
            {
                return mCount;
            }

            int Achievement::xp() const
            {
                return mXp;
            }

            int Achievement::rp() const
            {
                return mRp;
            }

            QString Achievement::xp_t() const
            {
                return mXp_t;
            }

            QString Achievement::rp_t() const
            {
                return mRp_t;
            }

            QString Achievement::getImageUrl(int size) const
            {
                return mIcons[size];
            }

            QString Achievement::name() const
            {
                return mName;
            }

            QString Achievement::description() const
            {
                return mDescription;
            }

            QString Achievement::howto() const 
            {
                return mHowto;
            }

            QString Achievement::imageName() const
            {
                return mImageName;
            }

            void Achievement::updateImageCache() const
            {
#if ENABLE_ACHIEVEMENT_IMAGE_CACHE
                Origin::Services::NetworkAccessManager *pNetworkManager = Origin::Services::NetworkAccessManager::threadDefaultInstance();

                for(QHash<int, QString>::const_iterator i = mIcons.begin(); i!= mIcons.end(); i++)
                {
                    QNetworkRequest request(i.value());

                    QNetworkReply *reply = pNetworkManager->get(request);

                    // Disregard the reply, as we are only interested in caching the response.
                    ORIGIN_VERIFY_CONNECT(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
                }
#endif
            }
        }
    }
}