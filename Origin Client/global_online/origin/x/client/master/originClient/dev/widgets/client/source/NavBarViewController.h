/////////////////////////////////////////////////////////////////////////////
// NavBarViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef NAV_BAR_VIEW_CONTROLLER_H
#define NAV_BAR_VIEW_CONTROLLER_H

#include <QWidget>
#include "engine/login/User.h"
#include "services/plugin/PluginAPI.h"
#include "RedeemBrowser.h"

namespace Origin
{
    namespace Client
    {
        class NavBarView;
        class SocialUserAreaViewController;

        class ORIGIN_PLUGIN_API NavBarViewController : public QWidget
        {
            Q_OBJECT

        public:

            NavBarViewController(Engine::UserRef user, QWidget *parent);
            ~NavBarViewController();

            void init();

            void updateMyGamesTab();
            void updateStoreTab();
            void clearTabs();

            void enableMyOriginTab(bool enable);

        signals:

            void logout();
            void exit();
            void showMyGames();
            void showStore();
            void addFriend();
            void goOnline();
            void showRedemption (RedeemBrowser::SourceType src, RedeemBrowser::RequestorID requestorID);

            void clickedShowFriendsList();
            void clickedMyUsername();
            void clickedMyAvatar();
            void clickedAchievements();

        private:

            Engine::UserWRef                mUser;
            NavBarView*                     mNavBarView;
            SocialUserAreaViewController*   mSocialUserAreaViewController;
        };
    }
}

#endif
