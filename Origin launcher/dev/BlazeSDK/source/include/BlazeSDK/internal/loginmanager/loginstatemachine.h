/**************************************************************************************************/
/*! 
    \file loginstatemachine.h
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef LOGINSTATEMACHINE_H
#define LOGINSTATEMACHINE_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

// Include files
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/internal/loginmanager/loginmanagerimpl.h"
#include "BlazeSDK/internal/statemachine.h"
#include "BlazeSDK/component/authentication/tdf/authentication.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/jobid.h"
#include "BlazeSDK/blaze_eastl/list.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/component/utilcomponent.h"

#include "DirtySDK/proto/protohttp.h"

#define PROTO_HTTP_BUFFER_SIZE (50000)

namespace Blaze
{

namespace Authentication
{
    class PersonaDetails;
}

namespace LoginManager
{

enum LoginStateId
{
    LOGIN_STATE_INIT,
    LOGIN_STATE_ACCOUNT_CREATION,
    LOGIN_STATE_AUTHENTICATED,
    NUM_LOGIN_STATES
};

enum DownloadTosDocumentStatus
{
    DOCUMENT_GET_TERMS_AND_CONDITIONS   = -5, 
    DOCUMENT_GET_PRIVACY_POLICY         = -4, 
    DOCUMENT_CREATE_GET_FAIL            = -3, 
    DOCUMENT_BUFFER_TOO_SMALL           = -2,    
    DOCUMENT_DOWNLOAD_FAILED            = -1, 
    DOCUMENT_DOWNLOAD_DONE              = 0,     
    DOCUMENT_CREATE_GET_SUCCESS         = 1, 
    DOCUMENT_RECEIVING                  = 2 
};

class LoginStateBase;

/*! ***********************************************************************************************/
/*! \class LoginStateMachine
    
    \brief State machine interface for login and account creation flow.
***************************************************************************************************/
class LoginStateMachine : public StateMachine<LoginStateBase, NUM_LOGIN_STATES>
{
public:
    static LoginStateMachine* create(LoginManagerImpl& loginManager, SessionData& session);
    
protected:
    // Abstract class.
    LoginStateMachine() {}
};

/*! ***********************************************************************************************/
/*! \class LoginStateBase
    
    \brief Abstract base class for login and account creation states.
***************************************************************************************************/
class LoginStateBase 
:   public State<LoginStateMachine>,
    public Idler
{
public:
    virtual ~LoginStateBase();
    
    virtual void onExit();

    typedef Functor1<BlazeError> UpdateUserOptionsCb;
    
    // LoginManager actions
    virtual void onStartLoginProcess(const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT); // DEPRECATED
    virtual void onStartGuestLoginProcess(LoginManager::GuestLoginCb cb = LoginManager::GuestLoginCb());
    virtual void onStartOriginLoginProcess(const char8_t *code, const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT);
    virtual void onStartAuthCodeLoginProcess(const char8_t *code, const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT);
    virtual void onStartAccessTokenLoginProcess(const char8_t *accessToken, const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT);
    virtual void onStartTrustedLoginProcess(const char8_t* accessToken, const char8_t* id, const char8_t* idType);
    virtual void onStartTrustedLoginProcess(const char8_t* certData , const char8_t* keyData, const char8_t* id, const char8_t* idType);
    virtual void onLogout(Functor1<Blaze::BlazeError> cb = Functor1<Blaze::BlazeError>());
    virtual void onCancelLogin(Functor1<Blaze::BlazeError> cb = Functor1<Blaze::BlazeError>());
    virtual void onUpdateAccessToken(const char8_t* accessToken, Functor1<Blaze::BlazeError> cb = Functor1<Blaze::BlazeError>());
    virtual void onConfirmMessage();
    virtual void onGetAccountInfo(const LoginManager::GetAccountInfoCb& resultCb);
    virtual void onAssociateAccount(const char8_t* emailOrOriginPersona, const char8_t* password);
    virtual void clearLoginInfo();
    virtual JobId onUpdateUserOptions(const Util::UserOptions& request, Functor1<Blaze::BlazeError> cb);
    
protected:
    // Abstract class    
    LoginStateBase(LoginStateMachine& loginStateMachine, LoginManagerImpl& loginManager, MemoryGroupId memGroupId, StateId id);
    virtual LoginData* getLoginData() { return mLoginManager.getLoginData(); }
    virtual void logoutCb(BlazeError error, JobId id,Functor1<Blaze::BlazeError> cb);
    virtual void invalidAction();
    virtual void doLogin(const Authentication::LoginRequest& request);
    virtual void loginCb(const Authentication::LoginResponse* response, BlazeError error, JobId id);

    const char8_t* getErrorName(BlazeError error) { return mSession.mBlazeHub->getErrorName(error, mSession.mUserIndex); }

    BlazeHub *getBlazeHub() { return mSession.mBlazeHub; }
    DelayedDispatcher<LoginManagerListener> *getDispatcher() { return &mSession.mListenerDispatcher; }

    LoginManagerImpl& mLoginManager;
    SessionData&      mSession; // referenced from LoginManager, share same copy, same as LoginData
    bool mLoginCancelled;
    char8_t mProductName[MAX_PRODUCTNAME_LENGTH];
    Authentication::CrossPlatformOptSetting mCrossPlatformOpt;

public:
    virtual JobId onGetTermsOfService(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb);
    virtual JobId onGetPrivacyPolicy(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb);

    virtual void onFreeTermsOfServiceBuffer(){}
    virtual void onFreePrivacyPolicyBuffer(){}

protected:
    virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime) { }

    // The protected LoginData modifying methods
    void clear() { getLoginData()->clear(); }
    void clearLegalDocs() { getLoginData()->clearLegalDocs(); }
    void setLoginFlowType(LoginFlowType loginFlowType) { getLoginData()->setLoginFlowType(loginFlowType); }
    void setPersonaDetailsList(const Authentication::PersonaDetailsList *list) { getLoginData()->setPersonaDetailsList(list); }
    void setAccountId(AccountId accountId) { getLoginData()->setAccountId(accountId); }
    void setIsoCountryCode(const char8_t* isoCountryCode) { getLoginData()->setIsoCountryCode(isoCountryCode); }
    void setTermsOfService(char8_t* termsOfService) { getLoginData()->setTermsOfService(termsOfService); }
    void setPrivacyPolicy(char8_t* privacyPolicy) { getLoginData()->setPrivacyPolicy(privacyPolicy); }
    void setTermsOfServiceVersion(const char8_t* termsOfServiceVersion) { getLoginData()->setTermsOfServiceVersion(termsOfServiceVersion); }
    void setPrivacyPolicyVersion(const char8_t* privacyPolicyVersion) { getLoginData()->setPrivacyPolicyVersion(privacyPolicyVersion); }  
    void setPassword(const char8_t* password) { getLoginData()->setPassword(password); } 
    void setPersonaName(const char8_t* personaName) { getLoginData()->setPersonaName(personaName); }
    void setToken(const char8_t* token) { getLoginData()->setToken(token); }
    void setLastLoginError(BlazeError error) { getLoginData()->setLastLoginError(error); }
    void setIsOfLegalContactAge(uint8_t isOfLegalContactAge) { getLoginData()->setIsOfLegalContactAge(isOfLegalContactAge); }
    void setIsUnderage(bool isUnderage) { getLoginData()->setIsUnderage(isUnderage); }
    void setIsUnderageSupported(bool isUnderageSupported) { getLoginData()->setIsUnderageSupported(isUnderageSupported); }
    void setIsAnonymous(bool isAnonymous) { getLoginData()->setIsAnonymous(isAnonymous); }
    void setSpammable(bool eaEmailAllowed, bool thirdPartyEmailAllowed) { getLoginData()->setSpammable(eaEmailAllowed, thirdPartyEmailAllowed); }
    void setNeedsLegalDoc(bool needsLegalDoc) { getLoginData()->setNeedsLegalDoc(needsLegalDoc); }
    void setSkipLegalDocumentDownload(bool skipLegalDocumentDownload) { getLoginData()->setSkipLegalDocumentDownload(skipLegalDocumentDownload); }

    NON_COPYABLE(LoginStateBase)
};

/*! ***********************************************************************************************/
/*! \class LoginStateInit
    
    \brief The initial default state of the login state machine.
***************************************************************************************************/
class LoginStateInit : public LoginStateBase
{
public:
    LoginStateInit(LoginStateMachine& loginStateMachine, LoginManagerImpl& loginManager, MemoryGroupId memGroupId = MEM_GROUP_LOGINMANAGER_TEMP)
    :   LoginStateBase(loginStateMachine, loginManager, memGroupId, LOGIN_STATE_INIT) {}

    virtual void onStartGuestLoginProcess(Functor2<Blaze::BlazeError, const Blaze::SessionInfo*> cb);
    virtual void onStartOriginLoginProcess(const char8_t *code, const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT);
    virtual void onStartAuthCodeLoginProcess(const char8_t *code, const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT);
    virtual void onStartAccessTokenLoginProcess(const char8_t *accessToken, const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT);

protected:
    virtual BlazeError setExternalLoginData(Authentication::LoginRequest &req) { return ERR_OK; }
    virtual void guestLoginCb(const Blaze::SessionInfo* info, Blaze::BlazeError error, Blaze::JobId jobId,
        Blaze::Functor2<Blaze::BlazeError, const Blaze::SessionInfo*> cb);
};

/*! ***********************************************************************************************/
/*! \class LoginStateInitS2S

\brief Extends LoginStateInit; adds support for trusted login
***************************************************************************************************/
class LoginStateInitS2S : public LoginStateInit, public S2SManager::S2SManagerListener
{
public:
    LoginStateInitS2S(LoginStateMachine& loginStateMachine, LoginManagerImpl& loginManager, MemoryGroupId memGroupId = MEM_GROUP_LOGINMANAGER_TEMP)
        : LoginStateInit(loginStateMachine, loginManager, memGroupId) {}

    virtual ~LoginStateInitS2S();

    virtual void onStartTrustedLoginProcess(const char8_t* accessToken, const char8_t* id, const char8_t* idType);
    virtual void onStartTrustedLoginProcess(const char8_t* certData, const char8_t* keyData, const char8_t* id, const char8_t* idType);

protected:
    virtual void trustedLoginCb(const Authentication::LoginResponse* response, BlazeError error, JobId id);
    virtual void onFetchedS2SToken(BlazeError err, const char8_t* token);

private:
    Authentication::TrustedLoginRequest mLoginRequest;
};

/*! ***********************************************************************************************/
/*! \class LoginStateAuthenticated
    
    \brief The final state of a successful login.
***************************************************************************************************/
class LoginStateAuthenticated : public LoginStateBase
{
public:
    LoginStateAuthenticated(LoginStateMachine& loginStateMachine, LoginManagerImpl& loginManager, MemoryGroupId memGroupId = MEM_GROUP_LOGINMANAGER_TEMP)
    :   LoginStateBase(loginStateMachine, loginManager, memGroupId, LOGIN_STATE_AUTHENTICATED),
    mTermsOfServiceBuffer(0),
    mTermsOfServiceBufferSize(0),
    mPrivacypolicyBuffer(0),
    mPrivacyPolicyBufferSize(0)
    {}
    
    virtual void onEntry();
    
    virtual void cancelLogin() { invalidAction(); }
    virtual void onGetAccountInfo(const LoginManager::GetAccountInfoCb& resultCb);
    virtual void onUpdateAccessToken(const char8_t* accessToken, Functor1<Blaze::BlazeError> cb = Functor1<Blaze::BlazeError>());
    virtual JobId onUpdateUserOptions(const Blaze::Util::UserOptions &request, Functor1<Blaze::BlazeError> cb);
    virtual JobId onGetTermsOfService(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb);
    virtual JobId onGetPrivacyPolicy(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb);
    virtual void onFreeTermsOfServiceBuffer();
    virtual void onFreePrivacyPolicyBuffer();
    virtual void onLogout(Functor1<Blaze::BlazeError> cb = Functor1<Blaze::BlazeError>());

protected:
    virtual void getAccountInfoCb(const Authentication::AccountInfo* accountInfo, BlazeError error, JobId jobId, const LoginManager::GetAccountInfoCb resultCb);
    virtual void getEmailOptInSettingsCb(const Authentication::GetEmailOptInSettingsResponse *response, BlazeError error, JobId id, Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb, bool getTermsOfService);
    virtual void getCountryCodeCb(const Authentication::AccountInfo* accountInfo, BlazeError error, JobId jobId, Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb, bool getTermsOfService);
    virtual void getLegalDocContentCb(const Authentication::GetLegalDocContentResponse* response, BlazeError error, JobId jobId, Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb, bool getTermsOfService);
    virtual void returnLegalDocContent(const Authentication::GetLegalDocContentResponse *response, bool getTermsOfService);
    virtual void updateAccessTokenCb(const Authentication::UpdateAccessTokenResponse* response, BlazeError error, JobId id, Functor1<Blaze::BlazeError> cb);

protected:
    Functor3<Blaze::BlazeError, const char8_t*, uint32_t> mCallBackGetTosInfo;

    char8_t *mTermsOfServiceBuffer;
    uint32_t mTermsOfServiceBufferSize;

    char8_t *mPrivacypolicyBuffer;
    uint32_t mPrivacyPolicyBufferSize;
};

/*! ***********************************************************************************************/
/*! \class LoginStateMachineImpl
    
    \brief Concrete implementation of LoginStateMachine.
***************************************************************************************************/
class LoginStateMachineImpl : public LoginStateMachine
{
public:
    // Note: It's safe to use *this in the initializer list here because the states don't use the
    //       references during construction.
#ifdef EA_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4355) // 'this' : used in base member initializer list
#endif
    LoginStateMachineImpl(LoginManagerImpl& loginManager, SessionData& session, MemoryGroupId memGroupId = MEM_GROUP_LOGINMANAGER_TEMP) :
        mLoginStateInit(*this, loginManager, memGroupId),
        mLoginStateAuthenticated(*this, loginManager, memGroupId)
    {
        mStates[LOGIN_STATE_INIT]              = &mLoginStateInit;
        mStates[LOGIN_STATE_AUTHENTICATED]     = &mLoginStateAuthenticated;
        
        changeState(LOGIN_STATE_INIT);
    }
#ifdef EA_COMPILER_MSVC
#pragma warning(pop)
#endif
    
protected:
    LoginStateInitS2S          mLoginStateInit;
    LoginStateAuthenticated    mLoginStateAuthenticated;
    
    NON_COPYABLE(LoginStateMachineImpl)
};

} // LoginManager
} // Blaze

#endif // LOGINSTATEMACHINE_H
