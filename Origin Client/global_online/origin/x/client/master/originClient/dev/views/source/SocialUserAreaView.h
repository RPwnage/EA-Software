/////////////////////////////////////////////////////////////////////////////
// SocialUserAreaView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef SOCIAL_USER_AREA_VIEW_H
#define SOCIAL_USER_AREA_VIEW_H

#include <QWidget>

#include "chat/NucleusID.h"
#include "services/plugin/PluginAPI.h"

namespace Ui
{
    class SocialUserAreaView;
}


namespace Origin
{
    namespace Chat
    {
        class RemoteUser;
    }

    namespace Engine
    {
    namespace Social
    {
        class SocialController;
    }
    }

    namespace Client
    {
        class AchievementMiniView;

        class ORIGIN_PLUGIN_API SocialUserAreaView : public QWidget
        {
            Q_OBJECT

        public:
            SocialUserAreaView(QWidget *parent = 0);
            ~SocialUserAreaView();

            void init();
            void setUnderageMode(bool aUnderage);

        public slots:
            void setAchievementXp(const int& xp);
            void onAchievementEnabledChanged(const bool& enable);

            /// \brief updates user avatar on the upper right corner
            void onAvatarChanged(quint64 nucleusId);


        signals:
            void clickedShowFriendsList();
            void clickedMyUsername();
            void clickedMyAvatar();
            void clickedAchievements();

        protected slots:
            void setMyName(const QString &name);

            void onConnectionStateChanged(bool online);

            void updateOnlineFriendCount(unsigned int count);

        private:
            void setAvatar(QPixmap userAvatar);
            void updateUI(bool isOffline, bool isUnderage = false);

            AchievementMiniView* mAchievementMiniView;

            bool mOfflineMode;
            bool mUnderageMode;

            Engine::Social::SocialController *mSocialController;

            Ui::SocialUserAreaView* ui;
        };
    }
}

#endif
