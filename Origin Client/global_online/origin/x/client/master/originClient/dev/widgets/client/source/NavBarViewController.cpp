/////////////////////////////////////////////////////////////////////////////
// NavBarViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "NavBarViewController.h"
#include "NavBarView.h"
#include "SocialUserAreaViewController.h"
#include "Services/debug/DebugService.h"
#include "Services/settings/SettingsManager.h"
#include "Services/session/SessionService.h"

#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"

#include <QLayout>

namespace Origin
{
    namespace Client
    {
        NavBarViewController::NavBarViewController(Engine::UserRef user, QWidget *parent)
            : QWidget(parent)
            , mUser(user)
            , mNavBarView(NULL)
        {
            mNavBarView = new NavBarView(parent);

            // Add and connect the social user-area (user's avatar, "show friends", etc)
            mSocialUserAreaViewController = new SocialUserAreaViewController();
            mNavBarView->getSocialUserAreaLayout()->addWidget(mSocialUserAreaViewController->view());

            ORIGIN_VERIFY_CONNECT(mSocialUserAreaViewController, SIGNAL(clickedShowFriendsList()), this, SIGNAL(clickedShowFriendsList()));
            ORIGIN_VERIFY_CONNECT(mSocialUserAreaViewController, SIGNAL(clickedMyUsername()), this, SIGNAL(clickedMyUsername()));
            ORIGIN_VERIFY_CONNECT(mSocialUserAreaViewController, SIGNAL(clickedMyAvatar()), this, SIGNAL(clickedMyAvatar()));
            ORIGIN_VERIFY_CONNECT(mSocialUserAreaViewController, SIGNAL(clickedAchievements()), this, SIGNAL(clickedAchievements()));

            init();
        }

        NavBarViewController::~NavBarViewController()
        {

        }

        void NavBarViewController::init()
        {           
            // Pass these onto our parent controller
            ORIGIN_VERIFY_CONNECT(mNavBarView, SIGNAL(logout()), this, SIGNAL(logout()));
            ORIGIN_VERIFY_CONNECT(mNavBarView, SIGNAL(exit()), this, SIGNAL(exit()));
            ORIGIN_VERIFY_CONNECT(mNavBarView, SIGNAL(showStore()), this, SIGNAL(showStore()));
            ORIGIN_VERIFY_CONNECT(mNavBarView, SIGNAL(showMyGames()), this, SIGNAL(showMyGames()));
            ORIGIN_VERIFY_CONNECT(mNavBarView, SIGNAL(addFriend()), this, SIGNAL(addFriend()));

            mNavBarView->handleUnderAgeMode(Origin::Engine::LoginController::isUserLoggedIn() ? Origin::Engine::LoginController::currentUser()->isUnderAge() : false);
            mSocialUserAreaViewController->setUnderageMode(Origin::Engine::LoginController::isUserLoggedIn() ? Origin::Engine::LoginController::currentUser()->isUnderAge() : false);

//            ORIGIN_VERIFY_CONNECT(this, SIGNAL(goOnline()), mNavBarView->getUserArea(), SLOT(goOnline()));
            //ORIGIN_VERIFY_CONNECT(this, SIGNAL(initUserArea(bool)), mNavBarView->getUserArea(), SLOT(userOnlineStatusChanged(bool)));

//            ORIGIN_VERIFY_CONNECT(mNavBarView->getUserArea(), SIGNAL (showRedemption(RedeemBrowser::SourceType, RedeemBrowser::RequestorID)), this, SIGNAL (showRedemption(RedeemBrowser::SourceType, RedeemBrowser::RequestorID)));
        }

        void NavBarViewController::updateStoreTab()
        {
            mNavBarView->updateStoreTab();
        }

        void NavBarViewController::updateMyGamesTab()
        {
#ifdef NOT_ORIGIN_X
            //need to deep link to the my games tab here once we have to setup, so we would need to open the SPA directly to the my games page
            mNavBarView->updateMyGamesTab();
#endif
        }

        void NavBarViewController::clearTabs()
        {
            mNavBarView->clearTabs();
        }
    }
}
