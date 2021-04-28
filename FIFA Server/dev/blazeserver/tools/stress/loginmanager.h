/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_LOGINMANAGER_H
#define BLAZE_STRESS_LOGINMANAGER_H

/*** Include files *******************************************************************************/

#include "framework/callback.h"
#include "framework/usersessions/usersessionmanager.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave.h"
#include "stressconnection.h"

#include <EASTL/queue.h>
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConfigMap;

namespace Stress
{

class Login;
struct CmdLineArguments;

class LoginManager : private StressConnection::ConnectionHandler
{
    NON_COPYABLE(LoginManager);

public:
    LoginManager();
    ~LoginManager() override;

    Login* getLogin(StressConnection* connection, ClientPlatformType platformType, bool connectToCommonServer);
    bool initialize(const ConfigMap& config, const CmdLineArguments& arguments);
    bool retrieveServerAuthCode(const uint64_t id, const ClientPlatformType platformType, const bool connectToCommonServer, eastl::string& serverAuthCode);
    bool retrieveClientAccessToken(const uint64_t id, const ClientPlatformType platformType, const bool connectToCommonServer, eastl::string& clientAccessToken);

    const bool getUseStressLogin() const { return mUseStressLogin; }
    const bool getLoginByJwt() const { return mLoginByJwt; }
    const AccountId getStartNucleusId() const { return mStartNucleusId; }
    const uint64_t getStartIndex() const { return mStartIndex; }
    const char8_t* getPreAuthClientConfig() const { return mPreAuthClientConfig; }

    const char8_t* getServiceName(const ClientPlatformType platformType) { return mUseStressLogin ? mStressLoginByPlatformConfig[platformType].mServiceName : mNucleusLoginByPlatformConfig[platformType].mServiceName; }
    const char8_t* getEmailFormat(const ClientPlatformType platformType) { return mUseStressLogin ? mStressLoginByPlatformConfig[platformType].mEmailFormat : mNucleusLoginByPlatformConfig[platformType].mEmailFormat; }
    const char8_t* getPersonaFormat(const ClientPlatformType platformType) { return mUseStressLogin ? mStressLoginByPlatformConfig[platformType].mPersonaFormat : mNucleusLoginByPlatformConfig[platformType].mPersonaFormat; }

    const char8_t* getCachedJwtToken(const uint64_t id);
    void cacheJwtTokenIfAllowed(const uint64_t id, const char8_t* token);

    void adjustLoginCount(int32_t adj) { mLoginCount += adj; }

    typedef eastl::vector<uint64_t> PidList;
    
private:
    class StressHttpResult : public OutboundHttpResult
    {
    public:
        StressHttpResult();

        inline void setHttpError(BlazeRpcError err = ERR_SYSTEM) override { mErr = err; }
        void setAttributes(const char8_t* fullname, const XmlAttribute* attributes, size_t attributeCount) override { }
        void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) override;
        inline bool checkSetError(const char8_t* fullname, const XmlAttribute* attribuets, size_t attrCount) override { return false; }

        inline bool hasError() const { return mErr != ERR_OK; }
        inline const BlazeRpcError getError() const { return mErr; }
        inline const char8_t* getAccessToken() const { return mAccessToken.c_str(); }
        const char8_t* getCode(const char8_t* header);

    private:
        BlazeRpcError mErr;
        eastl::string mAuthCode;
        eastl::string mAccessToken;
    };

    void initializeHttpConnection();

    bool requestClientAuthCode(const uint64_t id, const ClientPlatformType platformType, const eastl::string& originAccessToken, eastl::string& clientAuthCode);
    bool requestServerAuthCode(const uint64_t id, const ClientPlatformType platformType, const bool connectToCommonServer, const eastl::string& accessToken, eastl::string& serverAuthCode);
    bool requestClientAccessToken(const uint64_t id, const ClientPlatformType platformType, const eastl::string& clientAuthCode, eastl::string& clientAccessToken);
    bool requestOriginAccessToken(const uint64_t id, const ClientPlatformType platformType, eastl::string& originAccessToken);

    void onDisconnected(StressConnection* conn) override;
    bool onUserSessionMigrated(StressConnection* conn) override { return true; }

private:
    typedef struct AccountPlatformConfig
    {
        char8_t mServiceName[256];
        char8_t mGrantType[256];
        char8_t mEmailFormat[256];
        char8_t mTokenFormat[256];
        char8_t mPassword[256];
        char8_t mPersonaFormat[256];
        char8_t mClientId[256];
        char8_t mClientSecret[256];
        char8_t mRedirectUri[256];
        char8_t mDisplay[256];
        char8_t mAuthSource[256];
        char8_t mServerClientId[256];
        char8_t mServerRedirectUri[256];
    } AccountByPlatformTypeConfig;
    typedef eastl::vector_map<ClientPlatformType, AccountPlatformConfig> AccountByPlatformTypeConfigMap;
    typedef eastl::hash_map<uint64_t, Login*> LoginsByInstanceId;
    typedef eastl::hash_map<uint64_t, eastl::string> JwtTokenById;
    bool initializeAccountByPlatformConfig(const ConfigMap* config, AccountByPlatformTypeConfigMap& accountByPlatformConfig);

private:
    LoginsByInstanceId mLogins;
    JwtTokenById mJwtTokenByIdCache;
    OutboundHttpConnectionManagerPtr mHttpConnectionManager;
    int32_t mLoginCount;

    //per-platform config
    AccountByPlatformTypeConfigMap mNucleusLoginByPlatformConfig;
    AccountByPlatformTypeConfigMap mStressLoginByPlatformConfig;

    //non-platform config
    bool mUseStressLogin;
    bool mLoginByJwt;
    bool mCacheJwtToken;
    AccountId mStartNucleusId;  //used for stress login
    uint64_t mStartIndex;
    uint16_t mPortNumber;
    char8_t mResponseType[256];
    char8_t mHeader[256];
    char8_t mHeaderType[256];
    char8_t mAuthenticationUri[256];
    char8_t mAuthCodePath[256];
    char8_t mAccessTokenPath[256];
    char8_t mPreAuthClientConfig[256];
    char8_t mCommonServerClientId[256];
    char8_t mCommonServerRedirectUri[256];
};

class Login
{
    NON_COPYABLE(Login);

public:
    typedef Functor2<BlazeRpcError, Login*> LoginCb;

    Login(StressConnection* connection, LoginManager* owner, uint64_t userId, ClientPlatformType platformType, bool connectToCommonServer);

    BlazeRpcError login();
    BlazeRpcError logout();

    LoginManager* getOwner() { return mOwner; }
    const Authentication::UserLoginInfo* getUserLoginInfo() const { return &mUserLoginInfo; }
    ClientPlatformType getPlatform() const { return mPlatformType; }
    const char8_t* getServiceName() { return mOwner->getServiceName(mPlatformType); }

private:
    LoginManager* mOwner;
    StressConnection* mStressConnection;
    Authentication::AuthenticationSlave mProxy;
    uint64_t mIdent;
    ClientPlatformType mPlatformType;
    char8_t mEmail[256];
    bool mConnectToCommonServer;

    Authentication::UserLoginInfo mUserLoginInfo;

    LoginCb mCallback;

    BlazeRpcError stressLogin(TimeValue& serverLoginTimeTaken);
    BlazeRpcError nucleusLogin(TimeValue& serverLoginTimeTaken);
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_LOGINMANAGER_H

