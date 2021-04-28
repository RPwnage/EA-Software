#ifndef SocialJsHelper_H
#define SocialJsHelper_H

#include <QObject>

// TODO: THIS DEPENDENCY NEEDS UPDATE ONCE OIG CLIENT/CONTROLLER FILES MOVED UP
#include "../../../originClient/dev/common/source/UIScope.h"
//#include "UIScope.h"

#include "engine/login/User.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

    namespace UIToolkit
    {
        class OriginWindow;
    }

    namespace Client
    {

        class OriginWebController;

        class ORIGIN_PLUGIN_API SocialJsHelper : public QObject
        {
            Q_OBJECT
        public:
            explicit SocialJsHelper(Engine::UserRef user, QObject *parent, UIScope scope = ClientScope);

            public slots:
                void showSearchFriends();
                void showChatWindowForFriend(const QString& friendID);
                void showFriendResultURL(const QString& urlToOpen);
                void addFriend(const QString& friendID, const QString &source = QString());
                void deleteFriend(const QString& friendID);
                void blockFriend(const QString& friendID, const QString& friendName = "");
                void unblockFriend(const QString& friendID);
                void reportFriend(const QString& friendID, const QString& friendName = "");
                void acceptFriendRequest(const QString& friendID);
                bool isPendingFriend(const QString& friendID);
                void showProductPage(const QString &productId);
                void inviteToMultiplayer(const QString& to, const QString& from, const QString& message);
                void showUserProfile();
                void showFriendProfile(const QString& friendID);
                void viewProfilePage(const QString &nucleusID);
                void showWindow(const QString& url);
                void closeWindow();
                void onCurrentWidgetChanged(int);
                bool isTriggeredFromIGO();

        private:
            Engine::UserRef mUser;

            UIToolkit::OriginWindow* mWindow;
            OriginWebController* mWindowWebController;

            /// \brief FriendId of the user currently in the profile web page
            QString mProfileViewingUserId;

            const UIScope mScope;
        };

    }
}

#endif
