///////////////////////////////////////////////////////////////////////////////
/// \file       AchievementProxy.cpp
/// This file contains the implementation of the AchievementProxy. 
///
///	\author     Hans van Veenendaal
/// \date       2010-2012
// \copyright  Copyright (c) 2012 Electronic Arts. All rights reserved.

#include "AchievementProxy.h"
#include "AchievementSetProxy.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/session/SessionService.h"

using namespace Origin::Services;

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            AchievementProxy::AchievementProxy(Origin::Engine::Achievements::AchievementRef achievement, AchievementSetProxy * parent) :
                QObject(parent), 
                mAchievement(achievement),
                mPinned(QString("Achieve-Pinned-") + parent->id() + "-" + achievement->id(), Variant::Bool, false, Setting::GlobalPerUser),
                mbWithinReachHidden(QString("Achieve-Hidden-") + parent->id() + "-" + achievement->id(), Variant::Bool, false, Setting::GlobalPerUser),
                mAchievementSet(parent)
            {
                ORIGIN_VERIFY_CONNECT(achievement.data(), SIGNAL(progressChanged(int, int, int)), this, SIGNAL(achievementProgressChanged(int, int, int)));
                ORIGIN_VERIFY_CONNECT(achievement.data(), SIGNAL(updated()), this, SIGNAL(achievementUpdated()));
                ORIGIN_VERIFY_CONNECT(achievement.data(), SIGNAL(granted(Origin::Engine::Achievements::AchievementRef)), this, SIGNAL(achievementGranted()));
            }

            QString AchievementProxy::id() const
            {
                return mAchievement->id();
            }

            QString AchievementProxy::expansionId() const
            {
                return mAchievement->expansionId();
            }

            QDateTime AchievementProxy::updateTime() const
            {
                return mAchievement->updateTime();
            }

            QDateTime AchievementProxy::expirationTime() const
            {
                return mAchievement->expirationTime();
            }

            int AchievementProxy::progress() const
            {
                return mAchievement->progress();
            }

            int AchievementProxy::total() const
            {
                return mAchievement->total();
            }

            int AchievementProxy::count() const
            {
                return mAchievement->count();
            }

            int AchievementProxy::xp() const
            {
                return mAchievement->xp();
            }

            int AchievementProxy::rp() const
            {
                return mAchievement->rp();
            }

            QString AchievementProxy::xp_t() const
            {
                return mAchievement->xp_t();
            }

            QString AchievementProxy::rp_t() const
            {
                return mAchievement->rp_t();
            }

            QString AchievementProxy::getImageUrl(int size) const
            {
                return mAchievement->getImageUrl(size);
            }

            QString AchievementProxy::name() const
            {
                return mAchievement->name();
            }

            QString AchievementProxy::description() const
            {
                return mAchievement->description();
            }

            QString AchievementProxy::howto() const
            {
                return mAchievement->howto();
            }

            QObject * AchievementProxy::achievementSet() const
            {
                return mAchievementSet;
            }

            bool AchievementProxy::achieved() const
            {
                return mAchievement->count()>0;
            }

            float AchievementProxy::percent() const
            {
                // In some cases the progress may go to zero when the achievement is achieved. Always check whether achieved() returns true
                return (float)mAchievement->progress()/(float)mAchievement->total(); 
            }

            bool AchievementProxy::pinned() const
            {
                return readSetting(mPinned, Session::SessionService::currentSession()).toQVariant().toBool();
            }

            void AchievementProxy::setPinned(bool bNewPinned)
            {
                bool bPinned = pinned();

                if(bPinned != bNewPinned)
                {
                    writeSetting(mPinned, bNewPinned, Session::SessionService::currentSession());

                    if(bNewPinned)
                    {
                        setWithinReachHidden(false);
                    }

                    emit pinnedChanged(bNewPinned);
                }
            }

            bool AchievementProxy::withinReachHidden() const
            {
                return readSetting(mbWithinReachHidden, Session::SessionService::currentSession()).toQVariant().toBool();
            }

            void AchievementProxy::setWithinReachHidden(bool bNewWithinReachHidden)
            {
                bool bWithinReachHidden = withinReachHidden(); 

                if(bWithinReachHidden != bNewWithinReachHidden)
                {
                    writeSetting(mbWithinReachHidden, bNewWithinReachHidden, Session::SessionService::currentSession());

                    if(bNewWithinReachHidden)
                    {
                        setPinned(false);
                    }

                    emit withinReachHiddenChanged(bNewWithinReachHidden);
                }
            }        	

		}
    }
}
