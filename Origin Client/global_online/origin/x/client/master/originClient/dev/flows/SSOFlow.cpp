//  SSOFlow.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "SSOFlow.h"
#include "FlowResult.h"

#include <QDomDocument>
#include <QDomElement>

#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/network/NetworkAccessManager.h"
#include "services/rest/AuthPortalServiceClient.h"
#include "services/log/LogService.h"
#include "services/session/SessionService.h"

#include "TelemetryAPIDLL.h"
#include "MainFlow.h"
#include "RTPFlow.h"
#include "ClientFlow.h"
#include "CommandFlow.h" // :TODO: remove dependency of CommandFlow from SSOFlow

namespace
{
    /// \return String list of offer IDs associated with the current command flow.
    /// \todo :TODO: We're referencing CommandFlow in order to get access to the
    /// "action URL" containing the offer IDs associated with the SSOFlow. This
    /// dependency should be removed. Alternative approaches, such as perhaps 
    /// the MainFlow setting an "offer IDs" member variable of SSOFlow, or SSOFlow
    /// reporting a "SSO flow error" event that gets picked up and reported to 
    /// telemetry elsewhere (with access to the offer IDs), etc.
    QString commandFlowOfferIds()
    {
        QString offerIds;

        if (Origin::Client::CommandFlow::instance())
        {
            offerIds = QUrlQuery(Origin::Client::CommandFlow::instance()->getActionUrl()).queryItemValue("offerIds", QUrl::FullyDecoded);
        }

        return offerIds;
    }
}

namespace Origin
{
    namespace Client
    {
        /// \return String representation of SSOLoginType value
        QString ssoFlowLoginTypeString(const Origin::Client::SSOFlow::SSOLoginType type)
        {
            using namespace Origin::Client;

#define RETURN_LOGIN_TYPE_STRING(loginType) if (type == SSOFlow::loginType) { return #loginType; }

            RETURN_LOGIN_TYPE_STRING(SSO_LAUNCHGAME);
            RETURN_LOGIN_TYPE_STRING(SSO_ORIGINPROTOCOL);
            RETURN_LOGIN_TYPE_STRING(SSO_LOCALHOST);

#undef RETURN_LOGIN_TYPE_STRING

            // Sorry to bother you, but a new enum value has been added to 
            // SSOLoginType that this function can't convert to. Please update 
            // this function accordingly.
            ORIGIN_ASSERT( 0 );

            return "SSO_UNKNOWN";
        }

        SSOFlow::SSOFlow()
            :mState(SSO_IDLE)
            ,mAuthToken(QString())
            ,mAuthCode(QString())
            ,mAuthRedirectUrl(QString())
            ,mIdentifyReplyPtr(0)
            ,mTokenResponse(0)
            ,mLoginType(SSO_LAUNCHGAME)
            ,mActive (false)
        {
        }

        SSOFlow::~SSOFlow()
        {
        }


        void SSOFlow::start()
        {
            QString authToken;
            QString authCode;
            QString authRedirectUrl;

            mActive = true;

            if (mLoginType == SSO_LAUNCHGAME || mLoginType == SSO_ORIGINPROTOCOL)
            {
                authToken = Origin::Services::readSetting(Origin::Services::SETTING_UrlSSOtoken).toString();
                authCode = Origin::Services::readSetting(Origin::Services::SETTING_UrlSSOauthCode).toString();
                authRedirectUrl = Origin::Services::readSetting(Origin::Services::SETTING_UrlSSOauthRedirectUrl).toString();
            }
            else
            {
                authToken = Origin::Services::readSetting(Origin::Services::SETTING_LocalhostSSOtoken).toString();
            }

            QString oldAuthToken = mAuthToken;
            QString oldAuthCode = mAuthCode;

            mAuthToken = authToken;
            mAuthCode = authCode;
            mAuthRedirectUrl = authRedirectUrl;

            static const int TOKEN_SUBSTRING_LENGTH = 10;

            GetTelemetryInterface()->Metric_SSO_FLOW_START(
                mAuthCode.right(TOKEN_SUBSTRING_LENGTH).toUtf8().constData(),
                mAuthToken.right(TOKEN_SUBSTRING_LENGTH).toUtf8().constData(),
                ssoFlowLoginTypeString(mLoginType).toUtf8().constData(),
                Engine::LoginController::isUserLoggedIn());

            // We only need to reidentify if we're underway and we'll be
            // logging in a potentially different person than we've been identifying.
            if (mState != SSO_IDLE && 
                ((!authToken.isEmpty() && authToken.compare(oldAuthToken) != 0) ||
                    (!authCode.isEmpty() && authCode.compare(oldAuthCode) != 0)))
            {
                if (!authCode.isEmpty() && authCode.compare(oldAuthCode) != 0)
                {
                    mState = SSO_RERETRIEVING_TOKEN;
                }
                else
                {
                    mState = SSO_REIDENTIFYING;
                }
            }
            else if (mState == SSO_IDLE)
            {
                if ((authToken.isEmpty() && (authCode.isEmpty())))
                {
                    //no authTOken or authCode, so we should be prompting for login if user isn't logged in yet
                    if (Engine::LoginController::isUserLoggedIn())
                    {
                        launch();
                    }
                    else
                    {
                        logIn();
                    }
                }
                else if (!authCode.isEmpty())
                {
                    retrieveToken ();
                }
                else 
                {
                    identify();
                }
            }
        }

        void SSOFlow::stop()
        {
            mState = SSO_IDLE;
            mAuthToken = QString();
            mAuthCode = QString();

            if (mIdentifyReplyPtr)
            {
                ORIGIN_VERIFY_DISCONNECT(mIdentifyReplyPtr, SIGNAL(finished()), this, SLOT(onIdentifyFinished()));
                mIdentifyReplyPtr->deleteLater();
            }
            mIdentifyReplyPtr = 0;

            if (mTokenResponse)
            {
                ORIGIN_VERIFY_DISCONNECT(mTokenResponse, SIGNAL(finished()), this, SLOT(onTokenRetrived()));
                mTokenResponse->deleteLater();
            }
            mTokenResponse = NULL;
            mActive = false;
        }

        void SSOFlow::retrieveToken ()
        {
            if (mAuthCode.isEmpty())
            {
                // Note: current code flow shouldn't hit this condition.
                GetTelemetryInterface()->Metric_SSO_FLOW_INFO("RetrieveTokenNoAuthCode");
                identify();
            }
            else
            {
                if (mTokenResponse)
                {
                    // Note: current code flow shouldn't hit this condition.
                    GetTelemetryInterface()->Metric_SSO_FLOW_INFO("RetrieveTokenReplyPending");

                    ORIGIN_VERIFY_DISCONNECT(mTokenResponse, SIGNAL(finished()), this, SLOT(onTokenRetrieved()));
                    mRequestTimeoutTimer.stop();
                    mTokenResponse->deleteLater();
                    mTokenResponse = 0;
                }

                mTokenResponse = Origin::Services::AuthPortalServiceClient::retrieveTokens(mAuthCode, mAuthRedirectUrl);
                if (mTokenResponse)
                {
                    const int NETWORK_TIMEOUT_MSECS = 30000;

                    ORIGIN_VERIFY_CONNECT(mTokenResponse, SIGNAL(finished()), this, SLOT(onTokenRetrieved()));
                    ORIGIN_VERIFY_CONNECT(&mRequestTimeoutTimer, SIGNAL (timeout()), mTokenResponse, SLOT(abort())); 

                    //don't want to use QTimer::singleShot(NETWORK_TIMEOUT_MSECS, response, SLOT(abort()));
                    //because we need to be able to stop the timer when stopping the flow
                    mRequestTimeoutTimer.setSingleShot(true);
                    mRequestTimeoutTimer.setInterval (NETWORK_TIMEOUT_MSECS);
                    mRequestTimeoutTimer.start();

                    mState = SSO_RETRIEVING_TOKEN;
                }
            }
        }

        void SSOFlow::identify()
        {
            if (!mAuthToken.isEmpty())
            {
                //we'd only want to do this if we're already logged in since all we're doing it checking to see if we're the same user
                if (Engine::LoginController::isUserLoggedIn())
                {
                    if (mIdentifyReplyPtr)
                    {
                        // Note: current code flow shouldn't hit this condition.
                        GetTelemetryInterface()->Metric_SSO_FLOW_INFO("IdentifyReplyPending");

                        ORIGIN_VERIFY_DISCONNECT(mIdentifyReplyPtr, SIGNAL(finished()), this, SLOT(onIdentifyFinished()));
                        mIdentifyReplyPtr->deleteLater();
                        mIdentifyReplyPtr = 0;
                    }

                    mIdentifyReplyPtr = Origin::Services::AuthPortalServiceClient::retrieveAccessTokenInfo (mAuthToken);
                    if (mIdentifyReplyPtr)
                    {
                        ORIGIN_VERIFY_CONNECT(mIdentifyReplyPtr, SIGNAL(finished()), this, SLOT(onIdentifyFinished()));
                        mState = SSO_IDENTIFYING;
                    }
                    else
                    {
                        // If unable to create the identifyreplyptr (we're out of memory) then just proceed to onIdentifyFinished
                        GetTelemetryInterface()->Metric_SSO_FLOW_INFO("IdentifyCreateTokenInfoResponseFail");

                        mState = SSO_IDENTIFYING;
                        onIdentifyFinished();
                    }
                }
                else
                {
                    mState = SSO_IDENTIFYING;
                    onIdentifyFinished();
                }
            }
            else
            {
                flowFailed();
            }
        }

        void SSOFlow::logOut()
        {
            // If no-one is logged in we don't need to log out, we can skip straight to
            // SSO logging in the new user.
            Engine::UserRef user = Engine::LoginController::currentUser();
            bool isLoggedIn = !user.isNull();
            if (isLoggedIn)
            {
                mState = SSO_LOGGING_OUT;
                stop();

                emit (finished(SSOFlowResult(FLOW_SUCCEEDED, Origin::Client::SSOFlowResult::SSO_LOGOUT)));
            }
            else
            {
                logIn();
            }
        }

        void SSOFlow::logIn()
        {
            // We can't log in unless we have an auth token.
            if (!mAuthToken.isEmpty())
            {
                mState = SSO_LOGGING_IN;
                stop();

                emit (finished(SSOFlowResult(FLOW_SUCCEEDED, Origin::Client::SSOFlowResult::SSO_LOGIN)));
            }
            else
            {
                flowFailed();
            }
        }

        void SSOFlow::launch()
        {
            stop();

            emit (finished(SSOFlowResult(FLOW_SUCCEEDED, Origin::Client::SSOFlowResult::SSO_LOGGEDIN_USERMATCHES)));
        }

        void SSOFlow::onIdentifyFinished()
        {
            bool sameUser = false;

            if (mIdentifyReplyPtr)
            {
                if (mIdentifyReplyPtr->error() == Origin::Services::restErrorSuccess)
                {
                    QString userId = mIdentifyReplyPtr->userId();
                    //if we're already logged in check and see if it's the same user
                    if (Engine::LoginController::isUserLoggedIn())
                    {
                        QString nucleusID = QString::number(Origin::Engine::LoginController::currentUser()->userId());
                        if (!userId.isEmpty() && userId.compare(nucleusID) == 0)
                        {
                            sameUser = true;
                        }
                    }
                }
                else
                {
                    ORIGIN_LOG_EVENT << "Unable to identify from SSOtoken";

                    GetTelemetryInterface()->Metric_SSO_FLOW_ERROR(
                        "IdentifyFail",
                        Services::getLoginErrorString(mIdentifyReplyPtr->error()).toUtf8().constData(),
                        static_cast< int32_t >(mIdentifyReplyPtr->error()),
                        static_cast< uint32_t >(mIdentifyReplyPtr->httpStatus()),
                        commandFlowOfferIds().toUtf8().constData());

                    //if the user IS NOT already logged in, then this will attempt a login with bad sso token and that will fail
                    //and the user will be asked for credentials
                    //if the user IS already logged in, then assume that it will be the same user
                    emit ssoIdentifyError();
                    sameUser = true;
                    if (mState == SSO_IDENTIFYING)  //we don't want to blow it away if we got anothe request while we were waiting for the first one to finish
                        mAuthToken.clear();
                }


                ORIGIN_VERIFY_DISCONNECT(mIdentifyReplyPtr, SIGNAL(finished()), this, SLOT(onIdentifyFinished()));
                mIdentifyReplyPtr->deleteLater();
                mIdentifyReplyPtr = 0;
            }

            if (mState == SSO_REIDENTIFYING)
            {
                // Note: reretrieving state should never be reached. SSOFlow needs some minor clean up.
                GetTelemetryInterface()->Metric_SSO_FLOW_INFO("OnIdentifyFinished_REIDENTIFYING");

                identify();
            }
            else if (mState == SSO_IDENTIFYING)
            {
                if (!Engine::LoginController::isUserLoggedIn())
                {
                    logIn();
                }
                else if (mAuthToken.isEmpty() || sameUser)
                {
                    launch();
                }
                else
                {
                    logOut();
                }
            }
            else
            {
                flowFailed();
            }
        }

        void SSOFlow::onTokenRetrieved ()
        {
            mRequestTimeoutTimer.stop();
            if (mTokenResponse)
            {
                ORIGIN_VERIFY_DISCONNECT(&mRequestTimeoutTimer, SIGNAL (timeout()), mTokenResponse, SLOT(abort())); 
            }

            if (mTokenResponse == NULL)
            {
                //the response may have timed out, so just pass it thru to identify
                GetTelemetryInterface()->Metric_SSO_FLOW_INFO("onTokenRetrieved_noresponse");
                ORIGIN_LOG_EVENT << "mTokenResponse = NULL";
            }
            else
            {
                ORIGIN_VERIFY_DISCONNECT(mTokenResponse, SIGNAL(finished()), this, SLOT(onTokenRetrieved()));

                if (mTokenResponse->error() == Origin::Services::restErrorSuccess)
                {
                    mAuthToken = mTokenResponse->tokens().mAccessToken;
                    //save it out to settings
    			    Origin::Services::writeSetting (Origin::Services::SETTING_UrlSSOtoken, mAuthToken);
                }
                else
                {
                    //got an error, just pass it thru to identify
                    mAuthToken.clear();

                    GetTelemetryInterface()->Metric_SSO_FLOW_ERROR(
                        "TokenRetrievedError",
                        Services::getLoginErrorString(mTokenResponse->error()).toUtf8().constData(),
                        static_cast< int32_t >(mTokenResponse->error()),
                        static_cast< uint32_t >(mTokenResponse->httpStatus()),
                        commandFlowOfferIds().toUtf8().constData());

                    QString errorStr = QString("onTokenRetrieved_error:%1 http=%2").arg(mTokenResponse->error()).arg(mTokenResponse->httpStatus());
                    ORIGIN_LOG_EVENT << errorStr;
                }
            }
            mTokenResponse->deleteLater();
            mTokenResponse = NULL;

            if (mState == SSO_RERETRIEVING_TOKEN)
            {
                // Note: reretrieving state should never be reached. SSOFlow needs some minor clean up.
                GetTelemetryInterface()->Metric_SSO_FLOW_INFO("OnRetrieveTokenFinished_RERETRIEVING");

                retrieveToken();
            }
            else if (mState == SSO_RETRIEVING_TOKEN)
            {
                mAuthCode.clear();      //consume it
                //consume it
			    Origin::Services::writeSetting (Origin::Services::SETTING_UrlSSOauthCode, "");
			    Origin::Services::writeSetting (Origin::Services::SETTING_UrlSSOauthRedirectUrl, "");

                identify();
            }
            else    //shouldn't be here in any other state, just fail
            {
                flowFailed();
            }
        }

        void SSOFlow::flowFailed()
        {
            stop();

            //if this is an RTP launch, then we want to prompt the user to login or if they're already logged in, then allow them thru
            if (mLoginType == SSO_LAUNCHGAME || mLoginType == SSO_ORIGINPROTOCOL )
            {
                if (!Engine::LoginController::isUserLoggedIn())
                {
                    finished(SSOFlowResult(FLOW_FAILED, Origin::Client::SSOFlowResult::SSO_LOGIN));
                }
                else
                {
                    finished(SSOFlowResult(FLOW_FAILED, Origin::Client::SSOFlowResult::SSO_LOGGEDIN_USERUNKNOWN));
                }
            }
            else
                emit (finished(SSOFlowResult(FLOW_FAILED, Origin::Client::SSOFlowResult::SSO_NONE)));
        }
    } //namespace Client
} //namespace Origin
