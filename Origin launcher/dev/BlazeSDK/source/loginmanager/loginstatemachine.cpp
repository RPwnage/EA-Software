/**************************************************************************************************/
/*! 
    \file loginstatemachine.cpp
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

// Include files
#include "BlazeSDK/component/authentication/tdf/authentication.h"
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/internal/loginmanager/loginstatemachine.h"
#include "BlazeSDK/internal/loginmanager/loginmanagerimpl.h"
#include "BlazeSDK/internal/loginmanager/loginstatemachineconsole.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/component/authenticationcomponent.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"
#include "BlazeSDK/util/utilapi.h"
#include "BlazeSDK/blazesdkdefs.h"

#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protohttp.h"

namespace Blaze
{
namespace LoginManager
{

using namespace Authentication;

/*! ***********************************************************************************************/
/*! \name LoginStateMachine methods
***************************************************************************************************/

LoginStateMachine* LoginStateMachine::create(LoginManagerImpl& loginManager, SessionData& session)
{
#ifdef BLAZESDK_CONSOLE_PLATFORM
    return BLAZE_NEW(MEM_GROUP_LOGINMANAGER, "LoginStateMachineImpl") LoginStateMachineImplConsole(
        loginManager, session, MEM_GROUP_LOGINMANAGER);
#else
    return BLAZE_NEW(MEM_GROUP_LOGINMANAGER, "LoginStateMachineImpl") LoginStateMachineImpl(
        loginManager, session, MEM_GROUP_LOGINMANAGER);
#endif
}

/*! ***********************************************************************************************/
/*! \name LoginStateBase methods
***************************************************************************************************/

LoginStateBase::LoginStateBase(LoginStateMachine& loginStateMachine, LoginManagerImpl& loginManager, MemoryGroupId memGroupId, StateId id)
:   State<LoginStateMachine>(loginStateMachine, id),
    mLoginManager(loginManager),
    mSession(loginManager.getSessionData()),
    mLoginCancelled(false)
{
    mProductName[0] = '\0';
}

LoginStateBase::~LoginStateBase()
{
    // Remove pending requests to avoid callbacks after destruction.
    getBlazeHub()->getScheduler()->removeByAssociatedObject(this);
}

void LoginStateBase::onExit()
{
    // Remove pending requests to avoid callbacks after leaving this state.
    getBlazeHub()->getScheduler()->removeByAssociatedObject(this);
}

void LoginStateBase::onLogout(Functor1<Blaze::BlazeError> cb)
{
    getBlazeHub()->getScheduler()->removeByAssociatedObject(this);
    UserManager::LocalUser* user = mSession.getLocalUser();
    auto userIndex = 0;
    if (user != nullptr)
    {
        userIndex = user->getUserIndex();
        if (user->isUserDeauthenticating())
            return;

        user->setUserDeauthenticating();
    }

    mSession.mAuthComponent->logout(MakeFunctor(this,
        &LoginStateBase::logoutCb),cb);

    ConnectionManager::ConnectionManager* connManager = getBlazeHub()->getConnectionManager();
    if (connManager != nullptr)
    {
        // connManager->clearCachedSessionKeyByUserIndex(userIndex);
        connManager->resetRetriveUpnpFlag();
    }
}

void LoginStateBase::logoutCb(BlazeError error, JobId id,Functor1<Blaze::BlazeError> cb)
{
    clearLoginInfo();

    // Return to initial state.
    mStateMachine.changeState(LOGIN_STATE_INIT);
    if(cb.isValid())
        cb(error);
}

void LoginStateBase::onCancelLogin(Functor1<Blaze::BlazeError> cb)
{
    onLogout(cb);
    mLoginCancelled = true;
}

void LoginStateBase::invalidAction()
{
    getDispatcher()->delayedDispatch("onSdkError", &LoginManagerListener::onSdkError, SDK_ERR_INVALID_LOGIN_ACTION);
    return;
}

void LoginStateBase::doLogin(const Authentication::LoginRequest& request)
{
    mSession.mAuthComponent->login(request, AuthenticationComponent::LoginCb(this, &LoginStateBase::loginCb));
}

void LoginStateBase::loginCb(const Authentication::LoginResponse* response, BlazeError error, JobId id)
{
    setLastLoginError(error);

    if (error == ERR_OK)
    {
        setIsUnderage(response->getIsUnderage());
        setIsAnonymous(response->getIsAnonymous());
        setIsOfLegalContactAge((uint8_t)response->getIsOfLegalContactAge());
    }
    else
    {
        getDispatcher()->dispatch("onLoginFailure", &LoginManagerListener::onLoginFailure, error, "");
    }
}

// reset all login info user disconnected/logged out
void LoginStateBase::clearLoginInfo()
{
    LoginData* data = getLoginData();

    // Clear login data.
    data->clear();
}

// Each LoginStateBase subclass implements the permitted actions for its state.
void LoginStateBase::onStartLoginProcess(const char8_t* productName/* = ""*/, CrossPlatformOptSetting crossPlatformOpt/* = DEFAULT*/)   { invalidAction(); }
void LoginStateBase::onStartGuestLoginProcess(LoginManager::GuestLoginCb cb)    { invalidAction(); }
void LoginStateBase::onStartTrustedLoginProcess(const char8_t* accessToken, const char8_t* id, const char8_t* idType)
                                                                                { invalidAction(); }
void LoginStateBase::onStartTrustedLoginProcess(const char8_t* certData, const char8_t* keyData, const char8_t* id, const char8_t* idType)
                                                                                { invalidAction(); }
void LoginStateBase::onStartOriginLoginProcess(const char8_t *code, const char8_t* productName/* = ""*/, CrossPlatformOptSetting crossPlatformOpt/* = DEFAULT*/)
                                                                                { invalidAction(); }
void LoginStateBase::onStartAuthCodeLoginProcess(const char8_t *code, const char8_t* productName/* = ""*/, CrossPlatformOptSetting crossPlatformOpt/* = DEFAULT*/)
{
    invalidAction();
}

void LoginStateBase::onStartAccessTokenLoginProcess(const char8_t *accessToken, const char8_t* productName/* = ""*/, CrossPlatformOptSetting crossPlatformOpt/* = DEFAULT*/)
{
    invalidAction();
}

void LoginStateBase::onConfirmMessage()                                         { invalidAction(); }
void LoginStateBase::onGetAccountInfo(const LoginManager::GetAccountInfoCb& resultCb)
                                                                                { invalidAction(); }
void LoginStateBase::onAssociateAccount(const char8_t* emailOrOriginPersona, const char8_t* password)
                                                                                { invalidAction(); }
void LoginStateBase::onUpdateAccessToken(const char8_t* accessToken, Functor1<Blaze::BlazeError> cb)
{
    invalidAction();
}
JobId LoginStateBase::onUpdateUserOptions(const Util::UserOptions& request, Functor1<Blaze::BlazeError> cb)
{
    invalidAction();
    return INVALID_JOB_ID;
}
JobId LoginStateBase::onGetTermsOfService(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb)
{
    if (cb.isValid())
    {
        if (nullptr != getLoginData() && nullptr != getLoginData()->getTermsOfService())
        {
            cb(ERR_OK, getLoginData()->getTermsOfService(), (uint32_t)(strlen(getLoginData()->getTermsOfService()) + 1));
        }
        else
        {
            cb(ERR_SYSTEM, nullptr, 0);
        }
    }

    return INVALID_JOB_ID; 
}
JobId LoginStateBase::onGetPrivacyPolicy(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb)
{
    if (cb.isValid())
    {
        if (nullptr != getLoginData() && nullptr != getLoginData()->getPrivacyPolicy())
        {
            cb(ERR_OK, getLoginData()->getPrivacyPolicy(), (uint32_t)(strlen(getLoginData()->getPrivacyPolicy()) + 1));
        }
        else
        {
            cb(ERR_SYSTEM, nullptr, 0);
        }
    }

    return INVALID_JOB_ID;
}

/*! ***********************************************************************************************/
/*! \name LoginStateInit methods
***************************************************************************************************/

void LoginStateInit::onStartGuestLoginProcess(Blaze::Functor2<Blaze::BlazeError, const Blaze::SessionInfo*> cb)
{
    mLoginCancelled = false;

    if (!getBlazeHub()->getConnectionManager()->isConnected())
    {
        getDispatcher()->delayedDispatch("onSdkError", &LoginManagerListener::onSdkError, SDK_ERR_NOT_CONNECTED);
        return;
    }

    if (getBlazeHub()->getUserManager()->getPrimaryLocalUser() == nullptr)
    {
        BLAZE_SDK_DEBUGF("[LoginStateInit::onStartGuestLoginProcess]: Unable to do guest login when no primary user logged in.\n");
        getDispatcher()->delayedDispatch("onSdkError", &LoginManagerListener::onSdkError, SDK_ERR_NO_PRIMARY_USER);
        return;
    }

    setLoginFlowType(LOGIN_FLOW_STANDARD);

    JobId jobId = mSession.mAuthComponent->guestLogin(Blaze::MakeFunctor(this, &LoginStateInit::guestLoginCb), cb);
    Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, cb);
}

void LoginStateInit::guestLoginCb(const Blaze::SessionInfo* info, Blaze::BlazeError error, Blaze::JobId jobId,
                                  Blaze::Functor2<Blaze::BlazeError, const Blaze::SessionInfo*> cb)
{
    if (cb.isValid())
        cb(error, info);
}

void LoginStateInit::onStartOriginLoginProcess(const char8_t *code, const char8_t* productName/* = ""*/, CrossPlatformOptSetting crossPlatformOpt /* = DEFAULT*/)
{
    onStartAuthCodeLoginProcess(code, productName, crossPlatformOpt);
}

void LoginStateInit::onStartAuthCodeLoginProcess(const char8_t *code, const char8_t* productName/* = ""*/, CrossPlatformOptSetting crossPlatformOpt /* = DEFAULT*/)
{
    mLoginCancelled = false;

    if (!getBlazeHub()->getConnectionManager()->isConnected())
    {
        getDispatcher()->delayedDispatch("onSdkError", &LoginManagerListener::onSdkError, SDK_ERR_NOT_CONNECTED);
        return;
    }
    
    getLoginData()->setToken(code);

    if ((code != nullptr) && (code[0] != '\0'))
    {
        Authentication::LoginRequest request;

        request.getPlatformInfo().setClientPlatform(getBlazeHub()->getClientPlatformType());

        request.setAuthCode(code);
        request.setProductName(productName);
        request.setCrossPlatformOpt(crossPlatformOpt);

        BlazeError err;
        if ((err = setExternalLoginData(request)) != ERR_OK)
            getDispatcher()->dispatch<BlazeError, const char8_t*>("onLoginFailure", &LoginManagerListener::onLoginFailure, err, "");
        else
            doLogin(request);
    }
    else
    {
        BLAZE_SDK_DEBUGF("[LoginStateInit:%p].onStartAuthCodeLoginProcess: Code is empty.", this);
        getDispatcher()->dispatch("onLoginFailure", &LoginManagerListener::onLoginFailure, AUTH_ERR_INVALID_TOKEN, "");
    }
}

void LoginStateInit::onStartAccessTokenLoginProcess(const char8_t *accessToken, const char8_t* productName/* = ""*/, CrossPlatformOptSetting crossPlatformOpt /* = DEFAULT*/)
{
    mLoginCancelled = false;

    if (!getBlazeHub()->getConnectionManager()->isConnected())
    {
        getDispatcher()->delayedDispatch("onSdkError", &LoginManagerListener::onSdkError, SDK_ERR_NOT_CONNECTED);
        return;
    }

    getLoginData()->setToken(accessToken);

    if ((accessToken != nullptr) && (accessToken[0] != '\0'))
    {
        Authentication::LoginRequest request;

        request.getPlatformInfo().setClientPlatform(getBlazeHub()->getClientPlatformType());

        request.setAccessToken(accessToken);
        request.setProductName(productName);
        request.setCrossPlatformOpt(crossPlatformOpt);

        BlazeError err;
        if ((err = setExternalLoginData(request)) != ERR_OK)
            getDispatcher()->dispatch<BlazeError, const char8_t*>("onLoginFailure", &LoginManagerListener::onLoginFailure, err, "");
        else
            doLogin(request);
    }
    else
    {
        BLAZE_SDK_DEBUGF("[LoginStateInit:%p].onStartAccessTokenLoginProcess: Access token is empty.", this);
        getDispatcher()->dispatch("onLoginFailure", &LoginManagerListener::onLoginFailure, AUTH_ERR_INVALID_TOKEN, "");
    }
}

/*! ***********************************************************************************************/
/*! \name LoginStateInitS2S methods
***************************************************************************************************/

LoginStateInitS2S::~LoginStateInitS2S()
{
    getBlazeHub()->getS2SManager()->removeListener(this);
}

void LoginStateInitS2S::onStartTrustedLoginProcess(const char8_t* accessToken, const char8_t* id, const char8_t* idType)
{
    Blaze::ConnectionManager::ConnectionManager* connMgr = getBlazeHub()->getConnectionManager();

    if (connMgr == nullptr)
    {
        getDispatcher()->delayedDispatch("onSdkError", &LoginManagerListener::onSdkError, SDK_ERR_NOT_CONNECTED);
        return;
    }

    if (accessToken == nullptr || *accessToken == '\0')
    {
        BLAZE_SDK_DEBUGF("[LoginStateInitS2S:%p].onStartTrustedLoginProcess: Access token not specified.\n", this);
        getDispatcher()->dispatch("onLoginFailure", &LoginManagerListener::onLoginFailure, ERR_SYSTEM, "");
        return;
    }

    mLoginManager.setAccessToken(accessToken);
    mLoginRequest.setAccessToken(accessToken);
    mLoginRequest.setId(id);
    mLoginRequest.setIdType(idType);

    mSession.mAuthComponent->trustedLogin(mLoginRequest, AuthenticationComponent::TrustedLoginCb(this, &LoginStateInitS2S::trustedLoginCb));
}

void LoginStateInitS2S::onStartTrustedLoginProcess(const char8_t* certData, const char8_t* keyData, const char8_t* id, const char8_t* idType)
{
    S2SManager::S2SManager* s2sManager = getBlazeHub()->getS2SManager();
    BlazeError err = s2sManager->initialize(certData, keyData, "LoginStateMachine");

    if (err == ERR_OK)
    {
        s2sManager->addListener(this);
        mLoginRequest.setId(id);
        mLoginRequest.setIdType(idType);
    }
    else if (err == SDK_ERR_NOT_CONNECTED)
    {
        getDispatcher()->delayedDispatch("onSdkError", &LoginManagerListener::onSdkError, SDK_ERR_NOT_CONNECTED);
    }
    else
    {
        BLAZE_SDK_DEBUGF("[LoginStateInitS2S:%p].onStartTrustedLoginProcess: S2S initialization failed with error: %s\n", this, getBlazeHub()->getErrorName(err));
        getDispatcher()->dispatch("onLoginFailure", &LoginManagerListener::onLoginFailure, ERR_SYSTEM, "");
    }
}

void LoginStateInitS2S::onFetchedS2SToken(BlazeError err, const char8_t* token)
{
    getBlazeHub()->getS2SManager()->removeListener(this);
    if (err != ERR_OK)
    {
        BLAZE_SDK_DEBUGF("[LoginStateInitS2S:%p].onFetchedS2SToken: Failed to obtain S2S access token; error: %s\n", this, getBlazeHub()->getErrorName(err));
        getDispatcher()->dispatch("onLoginFailure", &LoginManagerListener::onLoginFailure, err, "");
    }
    else
    {
        mLoginManager.setAccessToken(token);
        mLoginRequest.setAccessToken(token);
        mSession.mAuthComponent->trustedLogin(mLoginRequest, AuthenticationComponent::TrustedLoginCb(this, &LoginStateInitS2S::trustedLoginCb));
    }
}

void LoginStateInitS2S::trustedLoginCb(const Authentication::LoginResponse* response, BlazeError error, JobId id)
{
    setLastLoginError(error);

    if (error != ERR_OK)
    {
        getDispatcher()->dispatch("onLoginFailure", &LoginManagerListener::onLoginFailure, error, "");
    }
}

/*! ***********************************************************************************************/
/*! \name LoginStateAuthenticated methods
***************************************************************************************************/

void LoginStateAuthenticated::onEntry()
{
    // at this point, lets keep the simple login data values,
    // but delete the Terms Of service and Privacy Policy responseContent to free up mem
    clearLegalDocs();

    onFreeTermsOfServiceBuffer();
    onFreePrivacyPolicyBuffer();
}

void LoginStateAuthenticated::onGetAccountInfo(const LoginManager::GetAccountInfoCb& resultCb)
{
    JobId jobId = mSession.mAuthComponent->getAccount(MakeFunctor(this, &LoginStateAuthenticated::getAccountInfoCb), resultCb);
    if (resultCb.isValid())
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, resultCb);
}

void LoginStateAuthenticated::getAccountInfoCb(const Authentication::AccountInfo* accountInfo, BlazeError error, JobId jobId, const LoginManager::GetAccountInfoCb resultCb)
{
        if (error == ERR_OK)
        {
            bool globalOptin = (accountInfo->getGlobalOptin() == 1)? true : false;
            bool thirdPartyOptin = (accountInfo->getThirdPartyOptin() == 1)? true : false;
            setSpammable(globalOptin, thirdPartyOptin);
            
            resultCb(error, jobId, globalOptin, thirdPartyOptin);
        }
        else
        {
            resultCb(error, jobId, false, false);
            BlazeAssertMsg(false,  "fetch region count failed.");
        }
}

void LoginStateAuthenticated::onUpdateAccessToken(const char8_t* accessToken, Functor1<Blaze::BlazeError> cb)
{
    if (accessToken == nullptr || accessToken[0] == '\0')
    {
        BLAZE_SDK_DEBUGF("[LoginStateAuthenticated:%p].onUpdateAccessToken: Access token is empty.", this);
        updateAccessTokenCb(nullptr, AUTH_ERR_INVALID_TOKEN, INVALID_JOB_ID, cb);
        return;
    }

    Authentication::UpdateAccessTokenRequest request;
    request.setAccessToken(accessToken);

    mSession.mAuthComponent->updateAccessToken(request, MakeFunctor(this, &LoginStateAuthenticated::updateAccessTokenCb), cb);
}

void LoginStateAuthenticated::updateAccessTokenCb(const Authentication::UpdateAccessTokenResponse* response, BlazeError error, JobId id, Functor1<Blaze::BlazeError> cb)
{
    if (cb.isValid())
        cb(error);
}

JobId LoginStateAuthenticated::onUpdateUserOptions(const Blaze::Util::UserOptions &request,  Functor1<Blaze::BlazeError> cb)
{
    JobId jobId = getBlazeHub()->getUserManager()->getLocalUser(mSession.mUserIndex)->updateUserOptions(request, cb);
    return jobId;
}

void LoginStateAuthenticated::returnLegalDocContent(const Authentication::GetLegalDocContentResponse *response, bool getTermsOfService)
{
    Blaze::BlazeError ret = DOCUMENT_DOWNLOAD_FAILED;
    uint32_t bufferSize = response->getLegalDocContentLength();
    const char8_t* content = response->getLegalDocContent();

    if (getTermsOfService)
    {
        onFreeTermsOfServiceBuffer();

        mTermsOfServiceBuffer = (char8_t *)BLAZE_ALLOC(bufferSize + 1, MEM_GROUP_LOGINMANAGER, "LoginStateAuthenticated::mTermsOfServiceBuffer");
        if(!mTermsOfServiceBuffer)
        {
            mTermsOfServiceBuffer = nullptr;
            mTermsOfServiceBufferSize = 0;
            ret = DOCUMENT_DOWNLOAD_FAILED;
        }
        else
        {
            mTermsOfServiceBufferSize = mLoginManager.getLegalDocNormalizerHook()(mTermsOfServiceBuffer, content, bufferSize) + 1;
            ret = DOCUMENT_DOWNLOAD_DONE;
        }

        if (mCallBackGetTosInfo.isValid())
        {
            mCallBackGetTosInfo(ret, mTermsOfServiceBuffer, mTermsOfServiceBufferSize);
            mCallBackGetTosInfo.clear();
        }
    }
    else
    {
        onFreePrivacyPolicyBuffer();

        mPrivacypolicyBuffer = (char8_t *)BLAZE_ALLOC(bufferSize + 1, MEM_GROUP_LOGINMANAGER, "LoginStateAuthenticated::mPrivacypolicyBuffer");
        if(!mPrivacypolicyBuffer)
        {
            mPrivacypolicyBuffer = nullptr;
            mPrivacyPolicyBufferSize = 0;
            ret = DOCUMENT_DOWNLOAD_FAILED;
        }
        else
        {
            mPrivacyPolicyBufferSize = mLoginManager.getLegalDocNormalizerHook()(mPrivacypolicyBuffer, content, bufferSize) + 1;
            ret = DOCUMENT_DOWNLOAD_DONE;
        }

        if (mCallBackGetTosInfo.isValid())
        {
            mCallBackGetTosInfo(ret, mPrivacypolicyBuffer, mPrivacyPolicyBufferSize);
            mCallBackGetTosInfo.clear();
        }
    }
}

void LoginStateAuthenticated::getLegalDocContentCb(const Authentication::GetLegalDocContentResponse *response, BlazeError error, JobId id,
                                                   Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb, bool getTermsOfService)
{
    if (error == ERR_OK)
    {
        mCallBackGetTosInfo = cb;

        returnLegalDocContent(response, getTermsOfService);
    }

    if (error != ERR_OK)
    {
        if (cb.isValid())
            cb(error, nullptr, 0);
    }
}

void LoginStateAuthenticated::getEmailOptInSettingsCb(const Authentication::GetEmailOptInSettingsResponse *response, BlazeError error, JobId id,
                                                      Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb, bool getTermsOfService)
{
    if (error == ERR_OK)
    {
        Authentication::GetLegalDocContentRequest request;
        request.setIsoCountryCode(getLoginData()->getIsoCountryCode());

        if (getBlazeHub()->getConnectionManager())
        {
           request.setPlatform(getBlazeHub()->getConnectionManager()->getClientPlatformType());
        }

        if (getBlazeHub()->getInitParams().UsePlainTextTOS)
        {
            request.setContentType(Authentication::GetLegalDocContentRequest::PLAIN);
        }
        else
        {
            request.setContentType(Authentication::GetLegalDocContentRequest::HTML);
        }


        JobId jobId = 0;

        if (getTermsOfService)
        {
            jobId = mSession.mAuthComponent->getTermsOfServiceContent(request, MakeFunctor(this,&LoginStateAuthenticated::getLegalDocContentCb), cb, getTermsOfService);

        }
        else
        {
            jobId = mSession.mAuthComponent->getPrivacyPolicyContent(request, MakeFunctor(this,&LoginStateAuthenticated::getLegalDocContentCb), cb, getTermsOfService);
        }

        if (cb.isValid())
            Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, cb);
        else
            error = ERR_SYSTEM;
    }

    if (error != ERR_OK)
    {
        if (cb.isValid())
            cb(error, nullptr, 0);
    }
}

void LoginStateAuthenticated::getCountryCodeCb(const Authentication::AccountInfo* accountInfo, BlazeError error, JobId jobId,
                                               Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb, bool getTermsOfService)
{
    if (error == ERR_OK)
    {
        Authentication::GetEmailOptInSettingsRequest request;

        request.setIsoCountryCode(accountInfo->getCountry());
        setIsoCountryCode(accountInfo->getCountry());        
        
        if (getBlazeHub()->getConnectionManager())
        {
           request.setPlatform(getBlazeHub()->getConnectionManager()->getClientPlatformType());
        }

        JobId jobIdTemp = mSession.mAuthComponent->getEmailOptInSettings(request, MakeFunctor(this,&LoginStateAuthenticated::getEmailOptInSettingsCb), cb, getTermsOfService);

        if (cb.isValid())
            Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobIdTemp, cb);
    }
    else
    {
        if (cb.isValid())
            cb(error, nullptr, 0);
    }
}

JobId LoginStateAuthenticated::onGetTermsOfService(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb)
{ 
    if(mTermsOfServiceBuffer != nullptr)
    {
        if (cb.isValid())
        {
            cb(ERR_OK, mTermsOfServiceBuffer, mTermsOfServiceBufferSize);
        }

        return 0;
    }

    JobId jobId = mSession.mAuthComponent->getAccount(MakeFunctor(this, &LoginStateAuthenticated::getCountryCodeCb), cb, true);

    if (cb.isValid())
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, cb);

    return jobId;
}

JobId LoginStateAuthenticated::onGetPrivacyPolicy(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb)
{ 
    if(mPrivacypolicyBuffer != nullptr)
    {
        if (cb.isValid())
        {
            cb(ERR_OK, mPrivacypolicyBuffer, mPrivacyPolicyBufferSize);
        }

        return 0;
    }

    JobId jobId = mSession.mAuthComponent->getAccount(MakeFunctor(this, &LoginStateAuthenticated::getCountryCodeCb), cb, false);

    if (cb.isValid())
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, cb);

    return jobId;
}

void LoginStateAuthenticated::onFreeTermsOfServiceBuffer()
{ 
    if (mTermsOfServiceBuffer != nullptr)
    {
        BLAZE_FREE(MEM_GROUP_LOGINMANAGER, mTermsOfServiceBuffer);
        mTermsOfServiceBuffer = nullptr;
        mTermsOfServiceBufferSize = 0;
    }
}

void LoginStateAuthenticated::onFreePrivacyPolicyBuffer()
{
    if (mPrivacypolicyBuffer != nullptr)
    {
        BLAZE_FREE(MEM_GROUP_LOGINMANAGER, mPrivacypolicyBuffer);
        mPrivacypolicyBuffer = nullptr;
        mPrivacyPolicyBufferSize = 0;
    }
}

void LoginStateAuthenticated::onLogout(Functor1<Blaze::BlazeError> cb)
{
    onFreeTermsOfServiceBuffer();
    onFreePrivacyPolicyBuffer();

    LoginStateBase::onLogout(cb);
}

} // LoginManager
} // Blaze
