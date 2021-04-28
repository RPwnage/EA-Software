//  LoginFlow.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "flows/LoginFlow.h"
#include "flows/ITOFlow.h"
#include "engine/login/LoginController.h"
#include "Qt/originwindow.h"
#include "Qt/originmsgbox.h"
#include "Qt/originwindowmanager.h"
#include "Qt/originpushbutton.h"
#include "widgets/login/source/LoginViewController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "engine/social/SocialController.h"
#include "services/platform/PlatformService.h"
#include "common/source/OriginApplication.h"
#include "TelemetryAPIDLL.h"

using namespace Origin::Engine;
using namespace Origin::Services;

namespace Origin
{
    namespace Client
    {
        LoginFlow::LoginFlow(const StartupState& startupState, QString const& initialLoginErrorMessage, const Origin::UIToolkit::OriginBanner::IconType initialIcon) 
            : mLoginRequestInProcess (false)
            , mStartupState(startupState)
        {
            ORIGIN_VERIFY_CONNECT(
                LoginController::instance(), SIGNAL(loginComplete(Origin::Engine::LoginController::Error)),
                this, SLOT(onLoginComplete(Origin::Engine::LoginController::Error)));

            ORIGIN_VERIFY_CONNECT(
                LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)),
                this, SLOT(onUserLoggedIn(Origin::Engine::UserRef)));

            mInitialLoginErrorMessage = initialLoginErrorMessage;
            mInitialIcon = initialIcon;
        }

        bool LoginFlow::isLoginDialogShowing()
        {
            if (!mLoginViewController.isNull() && mLoginViewController->isLoginWindowVisible())
                return true;

            return false;
        }

        //this checks to see if we're in the middle of EADP login (not back end client login)
        bool LoginFlow::autoLoginInProgress()
        {
            if (!mLoginViewController.isNull() && mLoginViewController->autoLoginInProgress())
                return true;

            return false;
        }

        void LoginFlow::setLoginBanner(UIToolkit::OriginBanner::IconType type, const QString& message)
        {
            if (!mLoginViewController.isNull() && mLoginViewController->isLoginWindowVisible())
            {
                mLoginViewController->showBanner(type, message); // used for ITO, MainFlow.cpp(159)
            }
        }

        void LoginFlow::setShowAgeUpFlow (bool show)
        {
            if (!mLoginViewController.isNull())
            {
                mLoginViewController->setShowAgeUpFlow (show);
            }
        }

        void LoginFlow::onUserLoggedIn(Origin::Engine::UserRef)
        {
            QString oemTag;
            if(!mLoginViewController.isNull())
            {
                mLoginViewController->getOemInformation(oemTag);
                if(!oemTag.isEmpty())
                    GetTelemetryInterface()->Metric_LOGIN_OEM(oemTag.toUtf8().data());
                // Generate a random string for session id
                // send it in telemetry and record it to the log
                // This is going to be used later to match the teleemtry and the log
                QString sessionId(QUuid::createUuid().toString());
                ORIGIN_LOG_EVENT << "Session ID; " + sessionId;
                GetTelemetryInterface()->Metric_APP_SESSION_ID(sessionId.toStdString().c_str());
            }
        }

        void LoginFlow::start()
        {

            if (mLoginViewController.isNull())
            {
                //TODO not sure if we still need this with nuke 2.0
                // make sure we do single login check as we go back online
                Engine::UserRef user = Origin::Engine::LoginController::currentUser();
                if ( user )
                {
                    Origin::Services::Session::SessionService::setSingleLoginCkCompletedFlag(user->getSession(), false);
                }

                // check for token login
                QString token;

                QString authToken = Origin::Services::readSetting (Origin::Services::SETTING_CmdLineAuthToken).toString();
                if (!authToken.isEmpty())    //first check for just login with token
                {
                    token = Origin::Services::readSetting (Origin::Services::SETTING_CmdLineAuthToken).toString();
                    Origin::Services::writeSetting(Origin::Services::SETTING_CmdLineAuthToken, "");
                }
                else //check for SSO
                {
                    QString ssoToken = Origin::Services::readSetting(Origin::Services::SETTING_UrlSSOtoken).toString();
                    if (!ssoToken.isEmpty ())
                    {
                        token = ssoToken;
                        //consume it
                        Origin::Services::writeSetting (Origin::Services::SETTING_UrlSSOtoken, "");
                    }
                    else
                    {
                        //try and see if localhost SSOtoken exists
                        ssoToken = Origin::Services::readSetting(Origin::Services::SETTING_LocalhostSSOtoken).toString();
                        if (!ssoToken.isEmpty ())
                        {
                            token = ssoToken;
                            //consume it
                            Origin::Services::writeSetting (Origin::Services::SETTING_LocalhostSSOtoken, "");
                        }
                    }
                }
                mLoginViewController.reset(new LoginViewController(NULL, token));
                ORIGIN_VERIFY_CONNECT(mLoginViewController.data(), SIGNAL(typeOfLogin(const QString&, bool )), this, SLOT(onTypeOfLogin(const QString&, bool )));
                if (!mInitialLoginErrorMessage.isEmpty())
                {
                    Origin::UIToolkit::OriginBanner::IconType icon = UIToolkit::OriginBanner::Error;
                    if (mInitialIcon != UIToolkit::OriginBanner::IconUninitialized)
                        icon = mInitialIcon;
                    mLoginViewController->init(mStartupState, icon, mInitialLoginErrorMessage);
                }
                else if (OriginApplication::instance().isITE())
                {
                    mLoginViewController->init(mStartupState, UIToolkit::OriginBanner::Info, tr("ebisu_client_login_ito").arg(Origin::Client::ITOFlow::getGameTitleFromManifest()));
                }
                else
                {
                    mLoginViewController->init(mStartupState);
                }
                ORIGIN_VERIFY_CONNECT(mLoginViewController.data(), SIGNAL(loginError()), this, SIGNAL(loginError()));
                ORIGIN_VERIFY_CONNECT(mLoginViewController.data(), SIGNAL(userCancelled()), this, SLOT(onUserCancelled()));
                ORIGIN_VERIFY_CONNECT(mLoginViewController.data(), SIGNAL(loginComplete(const QString&, const QString&)), this, SLOT(onPortalLoginComplete(const QString&, const QString&)));
                ORIGIN_VERIFY_CONNECT(mLoginViewController.data(), SIGNAL(offlineLoginComplete(const QString&, const QString&)), this, SLOT(onOfflinePortalLoginComplete(const QString&, const QString&)));
                ORIGIN_VERIFY_CONNECT(mLoginViewController.data(), SIGNAL(ageUpComplete()), this, SIGNAL(ageUpFlowCompleted()));
                mLoginViewController->validateOfflineCacheAndShow();
            }
        }

        bool LoginFlow::restart(QString const& initialLoginErrorMessage)
        {
            //check and see if we've already requested actual login
            bool restartOk = true;
            if (!mLoginRequestInProcess)
            {
                if (!mLoginViewController.isNull())
                {
                    // HACK: MAC only. During an SSO login with the login window already showing, for some reason, the LoginViewController destructor
                    // never gets executed after the deleteLater() call. This prevents the login window from being shutdown, so explictly close it.
                    #ifdef ORIGIN_MAC
                        mLoginViewController->closeLoginWindow();                     
                    #endif
 
                    mLoginViewController.take()->deleteLater();
                }
                mInitialLoginErrorMessage = initialLoginErrorMessage;
                start();
            }
            else
            {
                restartOk = false;
                ORIGIN_LOG_EVENT << "Login Request In Process, unable to restart LoginFlow";
            }
            return restartOk;
        }

        void LoginFlow::onLoginComplete(LoginController::Error error)
        {
            mLoginRequestInProcess = false;
            switch (error)
            {
            case LoginController::LOGIN_SUCCEEDED:
                {
                    Engine::UserRef user = Origin::Engine::LoginController::currentUser();
                    if ( user && !mLoginViewController.isNull() )
                    {
                        Origin::Services::Session::SessionRef session = user->getSession();
                        if (!session.isNull())
                        {
                            GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::LoginToFriendsList);
                            GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::LoginToMainPage);
                            GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::LoginToMainPageLoaded);

                            Origin::Services::Session::AccessToken access_token = Origin::Services::Session::SessionService::accessToken(session);
                            //if access_token is empty, then we've been logged into offline mode at least wrt to authentication (single login check still to occur)
                            if (access_token.isEmpty())
                            {
                                //logged into offline mode
                                //if remember me is set, retrieve the associated userID
                                QString envname = Origin::Services::readSetting(Services::SETTING_EnvironmentName, Services::Session::SessionRef());
                                const Origin::Services::Setting &rememberMeSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_PROD : Services::SETTING_REMEMBER_ME_INT;
                                const Origin::Services::Setting &rememberMeUserIdSetting = envname == Services::SETTING_ENV_production ? Services::SETTING_REMEMBER_ME_USERID_PROD : Services::SETTING_REMEMBER_ME_USERID_INT;
                                
                                QString remembermeCookie = Origin::Services::readSetting (rememberMeSetting).toString();
                                if (!remembermeCookie.isEmpty())
                                {
                                    //retrive the associated userID
                                    QString rememberMeUserIdStr = Origin::Services::readSetting (rememberMeUserIdSetting).toString();
                                    qint64 rememberMeUserId = rememberMeUserIdStr.toLongLong();
                                    if (rememberMeUserId != user->userId())    //user logging into offline mode is not the same user as the one that holds remember me so clear it
                                    {
                                        mLoginViewController->clearRememberMeCookie();
                                        //should we clear out the PCUN cookie and associated setting too? (if we logged in offline from login page, pcun cookie isn't created)
                                    }
                                    else //otherwise if it matches, save out the cookie again in case in needs to updated for cases where we're in offline mode because of failed REST call
                                    {
                                        //need to call this to update the rememerUserId
                                        mLoginViewController->saveRememberMeCookie (rememberMeUserIdStr);
                                    }
                                }
                            }
                            else
                            {
                                //save off the rememeber me and userID an associated userId
                                //convert userId to string
                                QString userIdStr = QString("%1").arg (user->userId());
                                mLoginViewController->saveRememberMeCookie (userIdStr);
                            }

                        }

                        Origin::Services::Session::SessionService::setSingleLoginCkCompletedFlag(user->getSession(), false);
                    }


                    mLoginViewController.take()->deleteLater();

                    LoginFlowResult flowResult;
                    flowResult.result = FLOW_SUCCEEDED;
                    flowResult.transition = LoginFlowResult::DEFAULT;

                    emit finished(flowResult);
                    break;
                }

            //to get around some oddities with reloading the same page (we suspect), we need to restart thru the
            //the login flow again
            case LoginController::LOGIN_ERROR_FAILED_EXTRACT_CID: //unable to extract CID from idToken
            case LoginController::LOGIN_ERROR_OFFLINE_AUTHENTICATION_FAILED: //problem with .olc so unable to login to offline mode
            case LoginController::LOGIN_ERROR_NUCLEUS: // nucleus did not provide an authorization code
                {
                    mLoginViewController.take()->deleteLater();

                    LoginFlowResult flowResult;
                    flowResult.result = FLOW_FAILED;
                    flowResult.transition = LoginFlowResult::RETRY_LOGIN;
                    emit finished(flowResult);
                }
                break;

            default:
                {
                    mLoginViewController->showErrorMessage(tr("ebisu_client_login_error"));
                    break;
                }
            }
        }

        void LoginFlow::onPortalLoginComplete(const QString& authorizationCode, const QString& idToken)
        {
            mLoginRequestInProcess = true;
            LoginController::nucleus20LoginAsync(authorizationCode, idToken);
        }

        void LoginFlow::onOfflinePortalLoginComplete(const QString& primaryOfflineKey, const QString& backupOfflineKey)
        {
            mLoginRequestInProcess = true;
            LoginController::nucleus20OfflineLoginAsync(primaryOfflineKey, backupOfflineKey);
        }

        void LoginFlow::onUserCancelled()
        {
            //This disconnect is to prevent the login from attempting from complete in the event that the user quit but the LoginFlow was not destructed in time
            //Otherwise it would crash because LoginViewController was null.
            ORIGIN_VERIFY_DISCONNECT(
                LoginController::instance(), SIGNAL(loginComplete(Origin::Engine::LoginController::Error)),
                this, SLOT(onLoginComplete(Origin::Engine::LoginController::Error)));

            mLoginViewController.take()->deleteLater();

            LoginFlowResult flowResult;
            flowResult.result = FLOW_FAILED;
            flowResult.transition = LoginFlowResult::CANCELLED;

            //this LoginFlowResult type deletes the LoginFlow.
            emit finished(flowResult);
        }
        void LoginFlow::onTypeOfLogin(const QString& typeOfLogin, bool autoLogin)
        {
            LoginController::loginType(typeOfLogin, autoLogin);
        }
    }
}
