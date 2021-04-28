/////////////////////////////////////////////////////////////////////////////
// SingleLoginController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef SINGLE_LOGIN_CONTROLLER_H
#define SINGLE_LOGIN_CONTROLLER_H

#include <QObject>

#include "services/session/SessionService.h"
#include "services/plugin/PluginAPI.h"
#include "chat/Connection.h"
#include "engine/login/User.h"

class QTimer;

namespace Origin
{
    namespace Services
    {
        class UserOnlineResponse;
    }
}

namespace Origin
{
    namespace Engine
    {
        /// \brief Defines the API for checking if a user is online prior to logging in. 
        class ORIGIN_PLUGIN_API SingleLoginController : public QObject
        {
            Q_OBJECT

        public:

            /// \brief Prepares the SingleLoginController for use.
            static void init();

            /// \brief Frees up the SingleLoginController.
            static void release();

            /// \brief Returns the current SingleLoginController instance.
            static SingleLoginController* instance();

            /// \brief Performs initialization steps that cannot be performed at engine::init() time.
            void postInitialize();

            /// \brief Checks if the given user is online.
            /// \param session Reference to a session object containing information about the user.
            /// \param isSingleLoginEnabled Indicates whether Single Login is enabled.
            void checkUserOnline(Origin::Services::Session::SessionRef session, const bool isSingleLoginEnabled);
            
            /// \brief Checks if the given user is processing a cloud save.
            /// \param session Reference to a session object containing information about the user.
            void checkUserInCloudSaves(Origin::Services::Session::SessionRef session);

        signals:

            /// \brief Emitted when a user online check is finished.
            /// \param isUserOnline True if the user is online.
            void checkUserOnlineFinished(bool isUserOnline);
            
            /// \brief Emitted when a user in cloud saves check is finished.
            /// \param isUserInCloudSaves True if the user is in cloud saves.
            void checkUserInCloudSavesFinished(bool isUserInCloudSaves);
            
            /// \brief Emitted when the check user online request timed out.
            void checkUserOnlineTimeout();

            /// \brief Emitted when the user's account is logged in from another computer.
            void firstUserConflict();

        protected slots:

            /// \brief Slot that is triggered when the user online check finishes.
            void onCheckUserOnlineFinished();

            /// \brief Slot that is triggered when the user in cloud saves check finishes.
            void onCheckUserInCloudSavesFinished();

            /// \brief Slot that is triggered when the user online check times out.
            void onCheckUserOnlineTimeout();

            /// \brief Slot that is triggered when chat's been disconnected
            void onChatDisconnect(Origin::Chat::Connection::DisconnectReason);

            /// \brief Slot that is triggered when a user logs out.
            /// \param user A reference to the user that is logging out.
			void onUserLoggedOut(Origin::Engine::UserRef user);

        private:

            /// \brief The SingleLoginController constructor.
            SingleLoginController();

            bool mIsSingleLoginEnabled;

            /// \brief The response object for the user online check.
            Origin::Services::UserOnlineResponse *mUserOnlineResponse;
            
            /// \brief A QTimer object that handles the user online check timeout.
            QTimer* mUserOnlineTimeout;

            /// \brief The number of milliseconds to wait for the user online check.
            qint32 mTimeout;
        };
    }
}

#endif // SINGLE_LOGIN_CONTROLLER_H
