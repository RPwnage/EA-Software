///////////////////////////////////////////////////////////////////////////////
// FlowResult.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef FLOW_RESULT_H
#define FLOW_RESULT_H

#include "services/plugin/PluginAPI.h"
#include <QString>

#define REAUTHENTICATION_ENABLED 0      //set this to 1 to enable reauthentication,

namespace Origin
{
    namespace Client
    {
        /// \brief Enumeration of flow results.
        enum Result
        {
            FLOW_SUCCEEDED, ///< The flow completed successfully.
            FLOW_FAILED ///< The flow failed prior to completing.
        };

        /// \return String representation of Result enum value
        QString resultString(const Result result);

        /// \brief Base class for the result of different flows.
        class ORIGIN_PLUGIN_API FlowResult
        {
        public:
            /// \brief The FlowResult constructor.
            FlowResult()
                : result(FLOW_FAILED)
            {
            }

            /// \brief The FlowResult constructor.
            /// \param _result The initial result of the flow.
            FlowResult(Result _result)
                : result(_result)
            {
            }

            Result result; ///< The result of the flow.
        };

        /// \brief Result of the login flow.
        class ORIGIN_PLUGIN_API LoginFlowResult : public virtual FlowResult
        {
        public:
            /// \brief Enumerations of flows that may result from the login flow.
            enum Transition
            {
                DEFAULT, ///< The login flow ended normally.
                CANCELLED, ///< The login flow was cancelled (close the application).
                REGISTRATION, ///< The user clicked "register a new account".
				RETRY_LOGIN	  ///< there was an error so we want to retry going thru the login flow
            };

            Transition transition; ///< The transition.

            /// \brief The LoginFlowResult constructor.
            /// \param r The flow result.
            /// \param t The transition.
            LoginFlowResult(Result r = FLOW_SUCCEEDED, Transition t = DEFAULT)
                : FlowResult(r)
                , transition(t)
            {
            }
        };

        /// \brief Result of the logout/exit flow.
        class ORIGIN_PLUGIN_API LogoutExitFlowResult : public FlowResult
        {
        public:
            /// \brief Enumerations of the action the user can take.
            enum Action
            {
                USER_NO_ACTION = -1, ///< The user's default action.
                USER_LOGGED_OUT, ///< The user clicked "log out".
                USER_EXITED, ///< The user clicked "exit".
                USER_RESTART, ///< The user restarted the client.
                USER_CANCELLED, ///< The logout/exit operation was cancelled.
                USER_FORCED_LOGGED_OUT_BADTOKEN,  ///< When we force the log out due to authentication failure 
                USER_FORCED_LOGGED_OUT_CREDENTIALCHANGE ///< When we force the log out due to password change
            };

            Action action; ///< The user action.

            /// \brief The LogoutExitFlowResult
            /// \param act The user action.
            LogoutExitFlowResult(Action act)
                : action(act)
            {
            }
        };

        /// \brief The credentials used in the login flow.
		struct LoginFlowCredentialData : public virtual FlowResult
		{
			QString loginUsername; ///< The username.
			QString loginPassword; ///< The password.
		};

        /// \brief Result of the client flow.
        class ORIGIN_PLUGIN_API ClientFlowResult : public FlowResult
        {
        public:
            /// \brief Enumerations of the ways the client flow can end.
            enum Transition
            {
                EXIT, ///< The user clicked "exit".
                LOGOUT, ///< The user clicked "log out".
            };

            /// \brief The transition.
            Transition transition;

            /// \brief The ClientFlowResult constructor.
            ClientFlowResult()
                : FlowResult(FLOW_SUCCEEDED)
                , transition(EXIT)
            {
            }
            
            /// \brief The ClientFlowResult constructor.
            /// \param result The flow result.
            /// \param trans The transition.
            ClientFlowResult(Result result, Transition trans)
                : FlowResult(result)
                , transition(trans)
            {
            }
        };

        /// \brief Result of the message of the day flow.
        class ORIGIN_PLUGIN_API MOTDFlowResult : public FlowResult
        {

        };

        /// \brief Result of the single login flow.
        class ORIGIN_PLUGIN_API SingleLoginFlowResult : public FlowResult
        {
        public:
            /// \brief The SingleLoginFlowResult constructor.
            /// \param _result The flow result.
            /// \param _offlineMode True if the user elected to log in using offline mode.
            SingleLoginFlowResult(Result _result, bool _offlineMode)
                : FlowResult(_result)
                , offlineMode(_offlineMode)
            {
            }

            bool offlineMode; ///< True if the user elected to log in using offline mode.
        };

        /// \brief Result of the play flow.
        class ORIGIN_PLUGIN_API PlayFlowResult : public FlowResult
        {
        public:
            QString productId; ///< Product ID of the game that is being played.
        };

        /// \brief Result of the play flow.
        class ORIGIN_PLUGIN_API InstallerFlowResult : public FlowResult
        {
        public:
            QString productId; ///< Product ID of the game that is being downloaded.
        };

		/// \brief Result of the play flow.
		class ORIGIN_PLUGIN_API CloudSyncFlowResult : public FlowResult
		{
		public:
			QString productId; ///< Product ID of the game that is being synced.
		};

        /// \brief Result of the backup restore flow.
        class ORIGIN_PLUGIN_API SaveBackupRestoreFlowResult : public FlowResult
        {
        public:
            QString productId; ///< Product ID of the game that is being synced.
        };

        /// \brief Result of the Entitle Free Game flow.
        class ORIGIN_PLUGIN_API EntitleFreeGameFlowResult : public FlowResult
        {
        public:
            QString productId; ///< Product ID of the game that is being entitled.
        };

        /// \brief Result of the RTP flow.
        class ORIGIN_PLUGIN_API RTPFlowResult : public FlowResult
        {
        };

        /// \brief Result of the SSO flow.
        class ORIGIN_PLUGIN_API SSOFlowResult: public FlowResult
        {
        public:
            /// \brief Enumeration of actions that are requested by SSO flow.
            enum Action
            {
                SSO_NONE, ///< SSO Flow reuquested no action.
                SSO_LOGIN, ///< SSO Flow requested that we log the user in.
                SSO_LOGOUT, ///< SSO Flow requested that we log the user out.
                SSO_LOGGEDIN_USERMATCHES, ///< SSO Flow requested authentication, user already logged in matches SSOtoken user
                SSO_LOGGEDIN_USERUNKNOWN ///< SSO failed, or unable to verify user logged in but A user is logged in  
            };

            Action action;

            /// \brief The SSOFlowResult constructor.
            SSOFlowResult()
                : FlowResult(FLOW_SUCCEEDED)
                , action(SSO_NONE)
            {
            }

            /// \brief The SSOFlowResult constructor.
            /// \param result The flow result
            /// \param act The action requested by the SSO Flow.
            SSOFlowResult(Result result, Action act)
                : FlowResult(result)
                , action(act)
            {
            }
        };

        /// \return String representation of SSOFlow action value
        QString ssoFlowActionString(const SSOFlowResult::Action action);

        /// \brief Result of the Non-Origin Game flow.
        class ORIGIN_PLUGIN_API NonOriginGameFlowResult : public FlowResult
        {
        };

        /// \brief Result of the Customize Boxart flow.
        class ORIGIN_PLUGIN_API CustomizeBoxartFlowResult : public FlowResult
        {
        };
    }
}

#endif //FLOW_RESULTS_H
