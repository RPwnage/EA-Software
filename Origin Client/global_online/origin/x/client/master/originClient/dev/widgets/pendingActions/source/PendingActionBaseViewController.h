#ifndef PENDING_ACTION_BASE_VIEW_CONTROLLER_H
#define PENDING_ACTION_BASE_VIEW_CONTROLLER_H

#include <QObject>
#include <QUrlQuery>
#include "SSOFlow.h"
#include "PendingActionDefines.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API PendingActionBaseViewController : public QObject
        {
            Q_OBJECT

            public:
                PendingActionBaseViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId);
                virtual ~PendingActionBaseViewController(){};
                
                /// \brief initiates the sso process
                virtual void ssoLogin();

                /// \brief performs the main action designated by the PendingAction, e.g. triggering RTP for /game/launch action is done in this function
                virtual void performMainAction(){};

                /// \brief shows the requested popup
                virtual void showPopupWindow();

                /// \brief sets the window mode to be minimized, maxmized, normal
                virtual void setWindowMode();

                /// \brief returns a string identifying the startup tab
                virtual QString startupTab(){ return QString();}

                /// \brief returns the store url to show in the action
                virtual QUrl storeUrl(){ return QUrl();}

                /// \brief returns the state of the main client window
                virtual int windowsShowState(){return mWindowShowState;}

                /// \brief returns true if all the params in the url are valid. Parses the url params
                virtual bool parseAndValidateParams();

                /// \brief sends out telemetry pertient to the action
                virtual void sendTelemetry();

                /// \brief returns true if the action requires entitlements to be loaded first
                bool requiresEntitlements();

                /// \brief returns true if the action requires extra content entitlements (DLC) to be loaded first
                bool requiresExtraContent();

                /// \brief returns true if the action requires social(friends list) to be loaded first
                bool requiresSocial();

                /// \brief refreshes the entitlements if requested in the query params.
                /// returns true if the refresh was kicked off
                bool refreshEntitlementsIfRequested();

                /// \brief returns the id of the action (e.g. store/open, game/launch)
                QString lookupId() { return mLookupId; }

                /// \brief returns true if all the parts of this pending action have been called
                bool allComponentsExecuted();

                /// \brief returns true if the SSO token should be consumed (it depends on the action and if that action has forceOnline functionality)
                virtual bool shouldConsumeSSOToken();

                /// \brief set suppression state of the Promo Manager and Net Promoter Score survey based on onPromo flag 
                void setQuietMode();

            protected:
                /// \brief the query params extracted from the url in the constructor
                QUrlQuery mQueryParams;

                /// \brief the type of sso flow to intiate
                SSOFlow::SSOLoginType mSSOFlowType;

                /// \brief flag to dictate whether we wait for entitlements before performing the action 
                bool mRequiresEntitlements;

                /// \brief flag to dictate whether we wait for full initial entitlements before performing the action 
                bool mRequiresExtraContent;

                /// \brief flag to dictate whether we wait for social before performing the action 
                bool mRequiresSocial;

                /// \brief a user defined id to uniquely identify an action
                QString mUUID;

                /// \brief host + path of the url used to start the action (e.g. store/open, game/launch)
                QString mLookupId;

                /// \brief the flag used to determinie whether the action was triggered from a web browser
                bool mLaunchedFromBrowser;

                /// \brief holds the bit flags of the pending action components completed
                int mComponentsCompleted;

                /// \brief protocol command was issued from jumplist
                bool mFromJumpList;

                /// \called to make sure the client window is visible if we are not minimized;
                void ensureClientVisible();

            private:
                ///\brief checks to see if the popup types are valid
                bool parseAndValidatePopupWindowParam();

                ///\brief points to one show popup functions below, null if not valid
                void (PendingActionBaseViewController::*mShowPopup)();

                // popup window functions
                void showFriendChatPopup();
                void showFriendListPopup();
                void showAddGamePopup();
                void showRedeemGamePopup();
                void showAddFriendPopup();
                void showHelpPopup();
                void showChangeAvatarPopup();
                void showAboutPopup();

                ///\brief current window state -- holds one of these values: ClientWindow_SHOW_NORMAL, ClientWindow_SHOW_MINIMIZED, ClientWindow_SHOW_MINIMIZED_SYSTEMTRAY, ClientWindow_SHOW_MAXIMIZED, ClientWindow_NO_ACTION
                int mWindowShowState;

                /// \brief holds the username used if the popup type is friend chat window
                QString mUserName;

                /// \brief holds the requestor id used in conjunction with the redeem page.
                int mRequestorId;

                /// \brief holds the src id used in conjunction with the redeem page.
                int mRedeemSrcId;

                /// \brief holds the code key user in conjuction with the redeem page.
                QString mRedeemCode;
        };
    }
}

#endif
