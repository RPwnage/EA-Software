/////////////////////////////////////////////////////////////////////////////
// SocialUserAreaViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef SOCIAL_USER_AREA_VIEW_CONTROLLER_H
#define SOCIAL_USER_AREA_VIEW_CONTROLLER_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }
    namespace Client
    {
        class SocialUserAreaView;

        class ORIGIN_PLUGIN_API SocialUserAreaViewController : public QObject
        {
            Q_OBJECT

        public:

            SocialUserAreaViewController(QWidget *parent=0);
            ~SocialUserAreaViewController();

            void init();
            void setUnderageMode(bool aUnderage);

            QWidget* view();

        signals:
            void clickedShowFriendsList();
            void clickedMyUsername();
            void clickedMyAvatar();
            void clickedAchievements();

        private:
            SocialUserAreaView* mSocialUserAreaView;

        };
    }
}

#endif //SOCIAL_USER_AREA_VIEW_CONTROLLER_H
