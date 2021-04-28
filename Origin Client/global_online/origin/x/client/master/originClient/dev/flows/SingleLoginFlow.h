///////////////////////////////////////////////////////////////////////////////
// SingleLoginFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef SINGLE_LOGIN_FLOW_H
#define SINGLE_LOGIN_FLOW_H

// Because of header cycles, it is necessary to define the LoginType enumeration before
// including SingleLoginViewController (which must be included because the QScopedPointer
// required a fully defined type, on mac).
namespace Origin
{
    namespace Client
    {
        /// \brief Enumeration of the different single login cases.
        enum LoginType
        {
            PrimaryLogin, ///< Initial client login.
            SecondaryLogin ///< Secondary client login, typically when re-acquiring token.
        };
    }
}

#include "flows/AbstractFlow.h"
#include "widgets/login/source/SingleLoginViewController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class SingleLoginViewController;

        /// \brief Handles all high-level actions related to single login.
        class ORIGIN_PLUGIN_API SingleLoginFlow : public AbstractFlow
        {
            Q_OBJECT

        public:
            /// \brief The SingleLoginFlow constructor.
            SingleLoginFlow();

            /// \brief Public interface for starting the single login flow.
            virtual void start();

            /// \brief Closes single login dialogs if they are open
            void closeSingleLoginDialogs();

        protected slots:
            /// \brief Checks if the user is online.
            void checkUserOnline();

            /// \brief Checks if the user is in cloud saves.
            void checkUserInCloudSaves();

            /// \brief Slot that is triggered when the user elects to log in offline.
            void onLoginOffline();

            /// \brief Slot that is triggered when the user elects to log in online.
            void onLoginOnline();

            /// \brief Slot that is triggered when the user elects to cancel the login.
            void onCancelLogin();

            /// \brief Slot that is triggered when the check user online response is received.
            /// \param isUserOnline True if the user is online on another machine.
            void onCheckUserOnlineFinished(bool isUserOnline);

            /// \brief Slot that is triggered when the check user in cloud saves response is received.
            /// \param isSomebodyWriting True if somebody is currently writing to the cloud using the same user.
            void onCheckCloudSavesCompleted(bool isSomebodyWriting);

            /// \brief Slot that is triggered when there is a login conflict.
            void onFirstUserConflict();

        signals:
            /// \brief Emitted when the single login flow has finished.
            /// \param result The result of the single login flow.
            void finished(SingleLoginFlowResult result);

            /// \brief Emitted when the account in use dialog is shown.
            void accountInUse();

        private:

            QScopedPointer<SingleLoginViewController> mSingleLoginViewController; ///< Pointer to the single login view controller.
        };
    }
}

#endif // SINGLE_LOGIN_FLOW_H
