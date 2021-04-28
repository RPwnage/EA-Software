///////////////////////////////////////////////////////////////////////////////
// SSOFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef SSOFLOW_H
#define SSOFLOW_H

#include "AbstractFlow.h"

#include "engine/login/User.h"
#include "engine/login/LoginController.h"
#include "services/rest/AuthPortalServiceResponses.h"
#include "services/plugin/PluginAPI.h"

#include <QObject>
#include <QString>
#include <qtimer.h>

namespace Origin
{
    namespace Client
    {
        /// \brief Flow for Single Sign On.
        class ORIGIN_PLUGIN_API SSOFlow: public AbstractFlow
        {
            Q_OBJECT

        public:
            enum SSOLoginType
            {
                SSO_LAUNCHGAME = 0,
                SSO_ORIGINPROTOCOL,
                SSO_LOCALHOST
            };

            /// \brief The SSOFlow constructor.
			SSOFlow();

            /// \brief The SSOFlow destructor.
            virtual ~SSOFlow();

            /// \brief Starts the SSO flow.
            void start();

            /// \brief Stops the SSO flow.
            void stop();

            /// \brief Gets the auth token.
            /// \return The auth token.
            QString const& authToken() { return mAuthToken; }

            /// \brief Clears the auth token.
            /// \return The auth token that was cleared.
            QString consumeAuthToken()
            {
                QString token(mAuthToken);
                mAuthToken.clear();
                return token;
            }

            /// \brief set the appropriate SSO type 
            /// \param type of SSO (localhost, just SSO via protocol, SSO + launchgame via protocol)
            void setLoginType (SSOLoginType login) { mLoginType = login; }

            /// \brief return login type
            SSOLoginType loginType () { return mLoginType; }

            /// \brief return true if SSO is just for logging in (i.e. not launching game)
            bool loginOnly () { return (mLoginType == SSO_ORIGINPROTOCOL || mLoginType == SSO_LOCALHOST); }

            bool active() { return mActive; }

signals:
            /// \brief Emitted when the SSO flow finishes.
            /// \param result The result of the SSO flow.
            void finished (SSOFlowResult result);

            /// \brief Emitted when there we cannot determine if the requested SSO user is already logged in
            void ssoIdentifyError();

        protected slots:
            /// \brief Slot that is triggered when the identification of user is finished. 
            void onIdentifyFinished();

            /// \brief slot that is triggered when token retrieval response is finished
            void onTokenRetrieved();

        private:
            /// \brief retrieves the access_token from authorization Code so we can use that to identify if it's the same user
            void retrieveToken ();

            /// \brief Checks that we are still the same user.
            void identify();

            /// \brief Starts the logout process. 
            void logOut();
            
            /// \brief Starts the login process. 
            void logIn();

            /// \brief Kicks off download/launch.
            void launch();

            /// \brief Stops the flow and emits finished with a result of FLOW_FAILED.
            void flowFailed();

            /// \brief Enumeration of different states of the SSOFlow.
            enum SSOFlowState
            {
                SSO_IDLE = 0, ///< The flow has been stopped.
                SSO_RETRIEVING_TOKEN, ///< retrieving the access_token using authCode
                SSO_RERETRIEVING_TOKEN, ///< need to restart the retrieval of access_token since user has changed
                SSO_IDENTIFYING, ///< The user is being identified.
                SSO_REIDENTIFYING, ///< The user has changed and we need to reidentify.
                SSO_LOGGING_OUT, ///< The user is logging out.
                SSO_LOGGING_IN ///< The user is logging in.
            };

            SSOFlowState mState; ///< The current state.
            QString mAuthToken; ///< The current auth token
            QString mAuthCode; ///< The current auth code (if exists)
            QString mAuthRedirectUrl; ///< The current redirectUrl that is to be used with mAuthCode
            Origin::Services::AuthPortalTokenInfoResponse *mIdentifyReplyPtr; ///< used to retrieve userID associated with SSOtoken
            Origin::Services::AuthPortalTokensResponse *mTokenResponse; ///< used to retrieve access_token from authorization code
            SSOLoginType mLoginType;    //to distinguish between different SSO types (localhost/protocol/launchgame)
            QTimer mRequestTimeoutTimer;      ///< timer to wait for response
            bool mActive;               //set to true while we're in the SSOFlow
        };

        /// \return String representation of SSOLoginType value
        QString ssoFlowLoginTypeString(const SSOFlow::SSOLoginType type);

    } // namespace Client
} // namespace Origin

#endif //SSOFLOW_H
