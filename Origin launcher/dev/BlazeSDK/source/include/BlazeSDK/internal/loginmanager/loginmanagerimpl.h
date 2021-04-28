/**************************************************************************************************/
/*! 
    \file loginmanagerimpl.h
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef LOGINMANAGERIMPL_H
#define LOGINMANAGERIMPL_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

// Include files
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/internal/loginmanager/logindata.h"
#include "BlazeSDK/internal/loginmanager/sessiondata.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/component/authenticationcomponent.h"
#include "BlazeSDK/component/authentication/tdf/authentication.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/dispatcher.h"

namespace Blaze
{

class BlazeHub;

namespace LoginManager
{
typedef Functor1<bool> CanCreateUnderageAccountCb;

class LoginStateMachine;

class LoginManagerImpl : public LoginManager, protected BlazeStateEventHandler
{
public:

    LoginManagerImpl(BlazeHub* blazeHub, uint32_t userIndex, MemoryGroupId memGroupId = Blaze::MEM_GROUP_LOGINMANAGER_TEMP);
    virtual ~LoginManagerImpl();

    // Listener registration methods
    virtual void addListener(LoginManagerListener* listener);
    virtual void removeListener(LoginManagerListener* listener);

    virtual void addAuthListener(UserManagerStateListener* listener) { mStateDispatcher.addDispatchee(listener); }
    virtual void removeAuthListener(UserManagerStateListener* listener) { mStateDispatcher.removeDispatchee(listener); }

    // Primary control methods
    virtual void startLoginProcess(const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT) const;
    virtual void startGuestLoginProcess(GuestLoginCb cb = GuestLoginCb()) const;
    virtual void startOriginLoginProcess(const char8_t *code, const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT) const;
    virtual void startAuthCodeLoginProcess(const char8_t *code, const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT) const;
    virtual void startAccessTokenLoginProcess(const char8_t *accessToken, const char8_t* productName = "", Authentication::CrossPlatformOptSetting crossPlatformOpt = Authentication::CrossPlatformOptSetting::DEFAULT) const;
    virtual void startTrustedLoginProcess(const char8_t* certData, const char8_t* keyData, const char8_t* id, const char8_t* idType) const;
    virtual void startTrustedLoginProcess(const char8_t* accessToken, const char8_t* id, const char8_t* idType) const;
    virtual void logout(Functor1<Blaze::BlazeError> cb = Functor1<Blaze::BlazeError>()) const;
    virtual void cancelLogin(Functor1<Blaze::BlazeError> cb = Functor1<Blaze::BlazeError>()) const;
    virtual void updateAccessToken(const char8_t* accessToken, Functor1<Blaze::BlazeError> cb = Functor1<Blaze::BlazeError>()) const;

    //Get legal Document methods
    virtual JobId getPrivacyPolicy(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb) const;
    virtual JobId getTermsOfService(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb) const;

    //Free legal Document buffer
    virtual void  freePrivacyPolicyBuffer() const;
    virtual void  freeTermsOfServiceBuffer() const;

    // Update util user options
    virtual JobId updateUserOptions(const Util::UserOptions& request, Functor1<Blaze::BlazeError> cb = Functor1<Blaze::BlazeError>()) const;

    // DEPRECATED - These function originally converted a console user index (ex. Controller index on x360) into a DS user index. 
    //  Now DS uses IEAUser and NetConnAddLocalUser to associate users, and does not track the orignal conversion. 
    //  These functions now just pass-through to the DirtySockUserIndex versions. 
    virtual uint32_t getConsoleUserIndex() const;
    virtual BlazeError setConsoleUserIndex(uint32_t consoleUserIndex);

    // Console platform-specific methods
    virtual uint32_t getDirtySockUserIndex() const;
    virtual BlazeError setDirtySockUserIndex(uint32_t dirtySockIndex);

    // Session information methods
    // These are all passthrough accessors to the actual LocalUser associated with this LoginManager
    virtual AccountId getAccountId() const { return (mSession.getLocalUser() ? mSession.getLocalUser()->getUser()->getAccountId() : getLoginData()->getAccountId()); }
    virtual BlazeId getBlazeId() const { return (mSession.getLocalUser() ? mSession.getLocalUser()->getId() : INVALID_BLAZE_ID); }
    virtual PersonaId getPersonaId() const { return (mSession.getLocalUser() ? mSession.getLocalUser()->getId() : INVALID_BLAZE_ID); }
    virtual uint32_t getLastLoginTime() const { return (mSession.getLocalUser() ? mSession.getLocalUser()->getSessionInfo()->getLastAuthenticated() : 0); }
    virtual int64_t getTitleLastLoginTime() const { return (mSession.getLocalUser() ? mSession.getLocalUser()->getSessionInfo()->getLastLoginDateTime() : 0); }
    virtual const char8_t *getPersonaName() const { return (mSession.getLocalUser() ? mSession.getLocalUser()->getSessionInfo()->getDisplayName() : ""); }
    virtual const char8_t* getSessionKey() const { return (mSession.getLocalUser() ? mSession.getLocalUser()->getSessionInfo()->getSessionKey() : ""); }
    virtual bool isFirstLogin() const { return (mSession.getLocalUser() ? mSession.getLocalUser()->getSessionInfo()->getIsFirstLogin() : false); }

    virtual const Util::GetTelemetryServerResponse *getTelemetryServer() const { return (mSession.getLocalUser() ? mSession.getLocalUser()->getTelemetryServer() : nullptr); }
    virtual const Util::GetTickerServerResponse *getTickerServer() const { return (mSession.getLocalUser() ? mSession.getLocalUser()->getTickerServer() : nullptr); }
    virtual const Util::UserOptions *getUserOptions() { return (mSession.getLocalUser() ? mSession.getLocalUser()->getUserOptions() : nullptr); }

    // LoginManager specific utility methods and accessors
    virtual bool getIsUnderage() { return getLoginData()->getIsUnderage(); }
    virtual bool getIsAnonymous() { return getLoginData()->getIsAnonymous(); }
    virtual BlazeError getLastLoginError() { return getLoginData()->getLastLoginError(); }

    virtual uint8_t isOfLegalContactAge() { return getLoginData()->getIsOfLegalContactAge(); };
    virtual bool getCanAgeUp() { return getIsAnonymous() && !getIsUnderage(); }
    virtual const char8_t *getPrivacyPolicy() const { return getLoginData()->getPrivacyPolicy(); }

    virtual const char8_t* getAccessToken() const { return (mAccessToken.empty() ? nullptr : mAccessToken.c_str()); }

    // Shared data methods
    virtual const LoginData* getLoginData() const;
    virtual LoginData* getLoginData();

    virtual void freeLoginData();
    virtual SessionData& getSessionData() { return mSession; }

#if !defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_PS5) && !defined(EA_PLATFORM_XBSX)
    virtual void setUseExternalLoginFlow(bool useExternalLoginFlow);
#endif
    virtual void setIgnoreLegalDocumentUpdate(bool ignoreLegalDocumentUpdate);

    virtual bool getSkipLegalDocumentDownload() const;
    virtual void setSkipLegalDocumentDownload(bool skipLegalDocumentDownload);
    virtual bool canCreateUnderageAccount() { return getLoginData()->getIsUnderageSupported(); }

    // The protected LoginData modifying methods
    void clear() { getLoginData()->clear(); }
    void clearLegalDocs() { getLoginData()->clearLegalDocs(); }
    void setLoginFlowType(LoginFlowType loginFlowType) { getLoginData()->setLoginFlowType(loginFlowType); }
    void addPersona(const Authentication::PersonaDetails *persona) { getLoginData()->addPersona(persona); }
    void removePersona(PersonaId id) { getLoginData()->removePersona(id); }
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
    void setSpammable(bool eaEmailAllowed, bool thirdPartyEmailAllowed) { getLoginData()->setSpammable(eaEmailAllowed, thirdPartyEmailAllowed); }
    void setNeedsLegalDoc(bool needsLegalDoc) { getLoginData()->setNeedsLegalDoc(needsLegalDoc); }
    void setIsUnderage(bool isUnderage) { getLoginData()->setIsUnderage(isUnderage); }

    void setCanTransitionToAuthenticated(bool canTransitionToAuthenticated)
    {
        mSession.mBlazeHub->getUserManager()->setLocalUserCanTransitionToAuthenticated(mSession.mUserIndex, canTransitionToAuthenticated);
    }

    void setAccessToken(const char8_t* accessToken) { mAccessToken = accessToken; }

protected:

    // BlazeStateEventHandler interface
    virtual void onDisconnected(BlazeError errorCode);
    virtual void onAuthenticated(uint32_t userIndex);
    virtual void onDeAuthenticated(uint32_t userIndex);

private:

    virtual void getSpammableInternal(bool& eaEmailAllowed, bool& thirdPartyEmailAllowed) { getLoginData()->getSpammable(eaEmailAllowed, thirdPartyEmailAllowed);}
    virtual void getAccountInfoInternal(const GetAccountInfoCb& resultCb) const;
    LoginData*         mData;
    SessionData        mSession;
    LoginStateMachine* mStateMachine;

    bool mSkipLegalDocumentDownload;

    eastl::string mAccessToken;

    Dispatcher<UserManagerStateListener> mStateDispatcher;
    NON_COPYABLE(LoginManagerImpl)
};

} // LoginManager
} // Blaze

#endif // LOGINMANAGERIMPL_H
