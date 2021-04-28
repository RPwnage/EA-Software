///////////////////////////////////////////////////////////////////////////////
// PendingActionFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef PENDING_ACTION_FLOW_H
#define PENDING_ACTION_FLOW_H

#include "AbstractFlow.h"
#include "widgets/pendingActions/source/PendingActionDefines.h"
#include "services/plugin/PluginAPI.h"

#include <QUrl>
#include <QReadWriteLock>

namespace Origin
{
    namespace Client
    {
        class PendingActionBaseViewController;

        /// \brief Handles all high-level actions related to the main flow.
        class ORIGIN_PLUGIN_API PendingActionFlow : public AbstractFlow
        {
            Q_OBJECT

        public:
            enum PendingActionFlowStatus
            {
                Inactive,
                WaitingForLogin,
                WaitingForMyGames,
                ActionComplete,
                ErrorInvalidParams
            };

            /// \brief Creates a new PendingActionFlow object.
            static void create();

            /// \brief Destroys the current PendingActionFlow object.
            static void destroy();

            /// \brief Gets the current PendingActionFlow object.
            /// \return The current PendingActionFlow object.
            static PendingActionFlow* instance();
            
            /// \brief not used, but we have to define it as a part of deriving from abstract flow
            void start(){};

            /// \brief starts the pending action.
            /// \param url the origin2:// url we are parsing
            /// \param launchedFromBrowser does this request come from a browser
            /// \param uuid a user defined string used as a unique identifier
            Q_INVOKABLE bool startAction(QUrl url, bool launchedFromBrowser, QString uuid = QString());

            /// \brief returns true if we can start a new action
            /// \param url the url we are trying to start an action with
            /// \return true if can start a new action, false if we cannot
            bool canStartNewAction(const QUrl &url);

            /// \brief returns the startupTab from the current pending action
            QString startupTab();

            /// \brief returns the storeurl from the current pending action
            QUrl storeUrl();

            /// \brief returns the client window state (minimized/maxmized/etc) from the current pending action
            int windowsShowState();

            /// \brief returns the uuid from the current pending action
            QString activeUUID();

            /// \brief returns the current status of the current pending action
            PendingActionFlowStatus actionStatus() {return mPendingActionFlowStatus;}

            /// \brief returns true if we should show the primary window
            bool shouldShowPrimaryWindow();

            /// \brief sets a RTP launch request if the pending action is game launch
            void setPendingRTPLaunch();

            /// \brief returns if there is a current action running.
            bool isActionRunning() const {return mPendingAction != NULL;}

            /// \brief return suppression state of the Promo Manager and Net Promoter Score survey based on onPromo flag 
            bool suppressPromoState() { return mSuppressPromo; }

            /// \brief set suppression state of the Promo Manager and Net Promoter Score survey based on onPromo flag 
            void setSuppressPromoState(bool suppress) { mSuppressPromo = suppress; }

        protected slots:
            /// \brief triggered as a result of the ssologin, we call sso login in all cases, 
            /// so this is the starting point for performing the different parts of our requested actions
            void onSSOComplete(SSOFlowResult result, bool launchGame);

            /// \brief triggered when the clientflow is ready after login
            void onStartPendingAction();

            /// \brief triggered when the entitlements are ready
            void onEntitlementsUpdated();

            void onSocialRosterLoaded();
        protected:
            /// \brief The PendingActionFlow constructor.
            PendingActionFlow();

            /// \brief The PendingActionFlow destructor.
            virtual ~PendingActionFlow();

            /// \brief cleanup of the current action, including deleting it
            void resetAction(PendingActionFlowStatus status = ActionComplete);
            
            /// \brief pointer to the current pending action
            PendingActionBaseViewController *mPendingAction;

            /// \brief holds the current status of the action
            PendingActionFlowStatus mPendingActionFlowStatus;

            /// \brief holds the uuid of the last active action flow 
            /// (either current or the one that has finished if none are current)
            QString mUUID;

            ///\brief promo browser and net promoter dialog suppression state
            int mSuppressPromo;
        };


    }
}

#endif // COMMANDFLOW_H
