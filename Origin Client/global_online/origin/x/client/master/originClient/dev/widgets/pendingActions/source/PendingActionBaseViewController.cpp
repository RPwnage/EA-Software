#include "PendingActionBaseViewController.h"
#include "services/settings/SettingsManager.h"
#include "MainFlow.h"
#include "ClientFlow.h"
#include "PendingActionFlow.h"
#include "services/debug/DebugService.h"
#include "originwindow.h"
#include "engine/login/User.h"
#include "chat/OriginConnection.h"
#include "chat/Roster.h"
#include "chat/RemoteUser.h"
#include "engine/social/SocialController.h"
#include "engine/social/ConversationManager.h"
#include "SocialViewController.h"
#include "RedeemBrowser.h"
#include "TelemetryAPIDLL.h"

#include <QUuid>

namespace Origin
{
    namespace Client
    {
        using namespace PendingAction;

        PendingActionBaseViewController::PendingActionBaseViewController(const QUrlQuery &queryParams, const bool launchedFromBrowser, const QString &uuid, const QString &lookupId)
            :mQueryParams(queryParams)
            ,mSSOFlowType(Origin::Client::SSOFlow::SSO_ORIGINPROTOCOL)
            ,mRequiresEntitlements(false)
            ,mRequiresExtraContent(false)
            ,mRequiresSocial(false)
            ,mUUID(uuid)
            ,mLookupId(lookupId)
            ,mLaunchedFromBrowser(launchedFromBrowser)
            ,mComponentsCompleted(NoComponents)
            ,mFromJumpList(false)
            ,mShowPopup(NULL)
            ,mWindowShowState(ClientWindow_NO_ACTION)
            ,mRequestorId(RedeemBrowser::OriginCodeClient)
            ,mRedeemSrcId(RedeemBrowser::Origin)
        {
            QString popupWindow = mQueryParams.queryItemValue(ParamPopupWindowId);
            if(popupWindow.compare(PopupWindowFriendChatId, Qt::CaseInsensitive)==0)
            {
                mRequiresSocial = true;
            }

            QString fromJump = mQueryParams.queryItemValue(ParamInternalJumpId);
            mFromJumpList = (fromJump == "true");
        }

        void PendingActionBaseViewController::showPopupWindow()
        {
            if(mShowPopup)
                (this->*mShowPopup)();

            mComponentsCompleted |= PopupWindow;
        }

        void PendingActionBaseViewController::ssoLogin()
        {
            QString authCode;
            QString authToken;
            QString authRedirectUrl;

            //clear all the params in case
            Origin::Services::writeSetting(Origin::Services::SETTING_UrlSSOtoken, "");
            Origin::Services::writeSetting(Origin::Services::SETTING_UrlSSOauthCode, "");
            Origin::Services::writeSetting(Origin::Services::SETTING_UrlSSOauthRedirectUrl, "");

            //check and see if there's a authcode
            authCode = mQueryParams.queryItemValue(ParamAuthCodeId);
            if(authCode.isEmpty())
            {
                //if not lets try to grab the token
                authToken = mQueryParams.queryItemValue(ParamAuthTokenId);
                if(!authToken.isEmpty())
                    Origin::Services::writeSetting(Origin::Services::SETTING_UrlSSOtoken, authToken);

                //Detect if we are received out a literal "null" auth token from SSO
                if (authToken.compare("null", Qt::CaseInsensitive) == 0)
                {
                    GetTelemetryInterface()->Metric_ERROR_NOTIFY("LiteralNullToken", "ReceivedFromSSO");
                }
            }
            else
            {
                //if we do have an authcode we need the redirect url
                authRedirectUrl = mQueryParams.queryItemValue(ParamAuthRedirectUrlId);
                Origin::Services::writeSetting(Origin::Services::SETTING_UrlSSOauthCode, authCode);
                Origin::Services::writeSetting(Origin::Services::SETTING_UrlSSOauthRedirectUrl, authRedirectUrl);
            }

            setQuietMode(); // set the quiet mode here before NPS or Promo managers are run

            //start the sso flow
            Origin::Client::MainFlow::instance()->startSSOFlow(mSSOFlowType);
        }

        void PendingActionBaseViewController::setWindowMode()
        {
            if(mWindowShowState != ClientWindow_NO_ACTION)
            {
                ClientFlow *clientFlow = ClientFlow::instance();
                clientFlow->view()->show(static_cast<ClientViewWindowShowState>(mWindowShowState));
                if((mWindowShowState == ClientWindow_SHOW_NORMAL) || (mWindowShowState == ClientWindow_SHOW_MAXIMIZED))
                {
                    //if we have a set windows mode command and its not a minimize one lets make sure the window is on top
                    clientFlow->view()->window()->raise();
                    clientFlow->view()->window()->activateWindow();
                }
            }
            mComponentsCompleted |= WindowMode;
        }

        void PendingActionBaseViewController::setQuietMode()
        {
            QString no_promo = mQueryParams.queryItemValue(ParamNoPromo);
            PendingActionFlow *actionFlow = PendingActionFlow::instance();

            if (!no_promo.isEmpty() && actionFlow)
            {

                if (no_promo == QString("1"))
                {
                    // suppress Promo pop-ups and net promoter score surveys
                    actionFlow->setSuppressPromoState(true);
                }
                else
                {
                    // turn off suppression of Promo pop-ups and net promoter score surveys
                    actionFlow->setSuppressPromoState(false);
                }
            }
        }

        bool PendingActionBaseViewController::requiresEntitlements() 
        {
            return mRequiresEntitlements;
        }

        bool PendingActionBaseViewController::requiresExtraContent()
        {
            return mRequiresExtraContent;
        }

        bool PendingActionBaseViewController::requiresSocial() 
        {
            return mRequiresSocial;
        }

        bool PendingActionBaseViewController::refreshEntitlementsIfRequested()
        {
            bool refreshEntitlements = (QString(mQueryParams.queryItemValue(ParamRefreshId)).compare("true", Qt::CaseInsensitive) == 0);
            if(refreshEntitlements)
            {
                Origin::Engine::Content::ContentController::currentUserContentController()->refreshUserGameLibrary(Origin::Engine::Content::ContentUpdates);
            }

            return refreshEntitlements;
        }

        bool PendingActionBaseViewController::allComponentsExecuted()
        {
            return (mComponentsCompleted == AllComponents);
        }
        
        bool PendingActionBaseViewController::parseAndValidatePopupWindowParam()
        {
            bool valid = true;

            //lets see if we have a popup window set
            QString popupWindow = mQueryParams.queryItemValue(ParamPopupWindowId);
            if(!popupWindow.isEmpty())
            {
                if(popupWindow.compare(PopupWindowFriendChatId, Qt::CaseInsensitive)==0)
                {
                    mUserName = mQueryParams.queryItemValue(ParamPersonaId);
                    mShowPopup = &PendingActionBaseViewController::showFriendChatPopup;
                }
                else if(popupWindow.compare(PopupWindowFriendListId, Qt::CaseInsensitive) == 0)
                {
                    mShowPopup = &PendingActionBaseViewController::showFriendListPopup;
                }
                else if(popupWindow.compare(PopupWindowAddGameId, Qt::CaseInsensitive) == 0)
                {
                    mShowPopup = &PendingActionBaseViewController::showAddGamePopup;
                }
                else if(popupWindow.compare(PopupWindowRedeemGameId, Qt::CaseInsensitive) == 0)
                {
                    QString id = mQueryParams.queryItemValue(ParamRequestorId);
                    if(!id.isEmpty())
                    {
                        if(id.compare(RequestorIdClient, Qt::CaseInsensitive) == 0)
                        {
                            mRedeemSrcId = RedeemBrowser::Origin;
                            mRequestorId = RedeemBrowser::OriginCodeClient;
                        }
                        else if(id.compare(RequestorIdITO, Qt::CaseInsensitive) == 0)
                        {
                            mRedeemSrcId = RedeemBrowser::ITE;
                            mRequestorId = RedeemBrowser::OriginCodeITE;
                        }
                        else if(id.compare(RequestorIdRTP, Qt::CaseInsensitive) == 0)
                        {
                            mRedeemSrcId = RedeemBrowser::RTP;
                            mRequestorId = RedeemBrowser::OriginCodeRTP;
                        }
                        else if(id.compare(RequestorIdWeb, Qt::CaseInsensitive) == 0)
                        {
                            mRedeemSrcId = RedeemBrowser::Origin;
                            mRequestorId = RedeemBrowser::OriginCodeWeb;
                        }
                        else
                        {
                            valid = false;
                        }
                    }

                    mRedeemCode = mQueryParams.queryItemValue(ParamRedeemCodeId);

                    mShowPopup = &PendingActionBaseViewController::showRedeemGamePopup;
                }
                else if(popupWindow.compare(PopupWindowAddFriendId, Qt::CaseInsensitive) == 0)
                {
                    mShowPopup = &PendingActionBaseViewController::showAddFriendPopup;
                }
                else if(popupWindow.compare(PopupWindowHelpId, Qt::CaseInsensitive) == 0)
                {
                    mShowPopup = &PendingActionBaseViewController::showHelpPopup;
                }
                else if(popupWindow.compare(PopupWindowChangeAvatarId, Qt::CaseInsensitive) == 0)
                {
                    mShowPopup = &PendingActionBaseViewController::showChangeAvatarPopup;
                }
                else if(popupWindow.compare(PopupWindowAboutId, Qt::CaseInsensitive) == 0)
                {
                    mShowPopup = &PendingActionBaseViewController::showAboutPopup;
                }
                else
                {
                    //invalid popup string passed in
                    valid = false;
                }
            }
            return valid;
        }

        bool PendingActionBaseViewController::parseAndValidateParams()
        {
            bool popupParamValid = parseAndValidatePopupWindowParam();

            bool windowModeValid = true;

            QString windowMode = mQueryParams.queryItemValue(ParamWindowModeId);
            if(!windowMode.isEmpty())
            {
                if(windowMode.compare(ClientWindowMinimizedToTrayId, Qt::CaseInsensitive)==0)
                {
                    mWindowShowState = ClientWindow_SHOW_MINIMIZED_SYSTEMTRAY;
                }
                else if(windowMode.compare(ClientWindowMinimizedToTaskbarId, Qt::CaseInsensitive) == 0)
                {
                    mWindowShowState = ClientWindow_SHOW_MINIMIZED;
                }
                else if(windowMode.compare(ClientWindowMaximizedId, Qt::CaseInsensitive) == 0)
                {
                    mWindowShowState = ClientWindow_SHOW_MAXIMIZED;
                }
                else if(windowMode.compare(ClientWindowNormalId, Qt::CaseInsensitive) == 0)
                {
                    mWindowShowState = ClientWindow_SHOW_NORMAL;

                }
                else
                {
                    windowModeValid = false;
                    mWindowShowState = ClientWindow_NO_ACTION;
                }
            }


            return popupParamValid && windowModeValid;
        }

        void PendingActionBaseViewController::sendTelemetry ()
        {
            QString popupWindow = mQueryParams.queryItemValue(ParamPopupWindowId);
            if(!popupWindow.isEmpty())
            {
                const char *telemetryAction = NULL;

                if(popupWindow.compare(PopupWindowFriendListId, Qt::CaseInsensitive)==0)
                {
                    telemetryAction = TelemetryFriends;
                }
                else if(popupWindow.compare(PopupWindowAddGameId, Qt::CaseInsensitive) == 0)
                {
                    telemetryAction = TelemetryAddGame;
                }
                else if(popupWindow.compare(PopupWindowRedeemGameId, Qt::CaseInsensitive) == 0)
                {
                    telemetryAction = TelemetryRedeem;
                }
                else if(popupWindow.compare(PopupWindowAddFriendId, Qt::CaseInsensitive) == 0)
                {
                    telemetryAction = TelemetryAddFriend;
                }
                else if(popupWindow.compare(PopupWindowHelpId, Qt::CaseInsensitive) == 0)
                {
                    telemetryAction = TelemetryHelp;
                }

                if (telemetryAction != NULL)
                {
                    //for now, send telemetry regardless of mFromJumpList because before refactor, we were assuming that origin://settings was only coming from jumplist
                    //and origin:// doesn't differentiate the source; should refactor PlatformJumpList to use origin2:// and then we can pass in the jump parameter
        //            if (mFromJumpList)
                    {
                        //  TELEMETRY:  report to telemetry that the user selected this from the jumplist
                        GetTelemetryInterface()->Metric_JUMPLIST_ACTION(telemetryAction, "jumplist", "" );
                    }
                }
            }
        }

        void PendingActionBaseViewController::showFriendChatPopup()
        {
            Origin::Engine::UserRef user = Origin::Engine::LoginController::currentUser();
            if (!user.isNull())
            {
                //we need to find the remote user associated with this persona name
                QSet<Origin::Chat::RemoteUser*> contacts = user->socialControllerInstance()->chatConnection()->currentUser()->roster()->contacts();

                for(QSet<Origin::Chat::RemoteUser*>::ConstIterator it = contacts.constBegin(); it != contacts.constEnd(); it++)
                {
                    Origin::Chat::RemoteUser* user = *(it);
                    QString originId = user->originId();
                    if (originId.compare(mUserName, Qt::CaseInsensitive) == 0)
                    {
                        //found lets popup that window for the requested user
                        Origin::Client::ClientFlow::instance()->socialViewController()->chatWindowManager()->startConversation(user, Origin::Engine::Social::Conversation::InternalRequest, Engine::IIGOCommandController::CallOrigin_CLIENT);
                        break;
                    }
                }
            }
        }

        void PendingActionBaseViewController::showFriendListPopup()
        {
            ClientFlow::instance()->showMyFriends();
        }

        void PendingActionBaseViewController::showAddGamePopup()
        {
            MainFlow::instance()->showAddGameWindow();
        }

        void PendingActionBaseViewController::showRedeemGamePopup()
        {
            ClientFlow::instance()->showRedemptionPage((RedeemBrowser::SourceType) mRedeemSrcId, (RedeemBrowser::RequestorID) mRequestorId, mRedeemCode);
        }

        void PendingActionBaseViewController::showAddFriendPopup()
        {
            ClientFlow::instance()->showFriendSearchDialog();
        }

        void PendingActionBaseViewController::showHelpPopup()
        {
            ClientFlow::instance()->showHelp();
        }

        void PendingActionBaseViewController::showChangeAvatarPopup()
        {
            ClientFlow::instance()->showSelectAvatar();
        }

        void PendingActionBaseViewController::showAboutPopup()
        {
            ClientFlow::instance()->showAbout();
        }

        void PendingActionBaseViewController::ensureClientVisible()
        {
            ClientFlow *clientFlow = ClientFlow::instance();
            clientFlow->view()->show(static_cast<ClientViewWindowShowState>(windowsShowState()));
            if((windowsShowState() != ClientWindow_SHOW_MINIMIZED) && (windowsShowState() != ClientWindow_SHOW_MINIMIZED_SYSTEMTRAY))
            {
                //if we have a set windows mode command and its not a minimize one lets make sure the window is on top
                clientFlow->view()->window()->raise();
                clientFlow->view()->window()->activateWindow();
            }           
        }

        bool PendingActionBaseViewController::shouldConsumeSSOToken()
        {
            return Engine::LoginController::isUserLoggedIn();
        }

    }
}
