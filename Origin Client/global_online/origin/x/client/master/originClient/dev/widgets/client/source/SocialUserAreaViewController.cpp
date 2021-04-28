/////////////////////////////////////////////////////////////////////////////
// SocialUserAreaViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "SocialUserAreaViewController.h"
#include "services/debug/DebugService.h"
#include "SocialUserAreaView.h"
#include "engine/achievements/achievementmanager.h"


namespace Origin
{
    using namespace UIToolkit;
    namespace Client
    {

        SocialUserAreaViewController::SocialUserAreaViewController(QWidget *parent)
        {
            mSocialUserAreaView = new SocialUserAreaView(parent);

            init();
        }

        SocialUserAreaViewController::~SocialUserAreaViewController()
        {
        }

        void SocialUserAreaViewController::init()
        {           
            // Pass these onto our parent controller
            ORIGIN_VERIFY_CONNECT(mSocialUserAreaView, SIGNAL(clickedShowFriendsList()), this, SIGNAL(clickedShowFriendsList()));
            ORIGIN_VERIFY_CONNECT(mSocialUserAreaView, SIGNAL(clickedMyUsername()), this, SIGNAL(clickedMyUsername()));
            ORIGIN_VERIFY_CONNECT(mSocialUserAreaView, SIGNAL(clickedMyAvatar()), this, SIGNAL(clickedMyAvatar()));
            ORIGIN_VERIFY_CONNECT(mSocialUserAreaView, SIGNAL(clickedAchievements()), this, SIGNAL(clickedAchievements()));

            Engine::Achievements::AchievementManager * achievementManager = Engine::Achievements::AchievementManager::instance();
            ORIGIN_VERIFY_CONNECT(achievementManager, SIGNAL(enabledChanged(bool)), mSocialUserAreaView, SLOT(onAchievementEnabledChanged(const bool&)));
            mSocialUserAreaView->onAchievementEnabledChanged(achievementManager->enabled());

            Engine::Achievements::AchievementPortfolioRef portfolio = achievementManager->currentUsersPortfolio();
            ORIGIN_VERIFY_CONNECT(portfolio.data(), SIGNAL(xpChanged(int)), mSocialUserAreaView, SLOT(setAchievementXp(const int&)));
            mSocialUserAreaView->setAchievementXp(portfolio->xp());
        }

        QWidget* SocialUserAreaViewController::view()
        {
            return mSocialUserAreaView;
        }

        void SocialUserAreaViewController::setUnderageMode(bool aUnderage)
        {
            if (mSocialUserAreaView)
                mSocialUserAreaView->setUnderageMode(aUnderage);
        }
    }
}
