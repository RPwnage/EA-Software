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
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave.h"
#include "EASTL/hash_map.h"
#include "stressconnection.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/rpc/usersessions_defines.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConfigMap;

namespace Stress
{

class LoginManager;
struct CmdLineArguments;

typedef enum { AUTH_PC, AUTH_PS3, AUTH_XBOX360 } LoginAuthType;

class Login
{
    NON_COPYABLE(Login);

public:
    typedef Functor2<BlazeRpcError, Login*> LoginCb;

    Login(StressConnection* connection, LoginManager* owner,
            const char8_t* email, const char8_t* password, const char8_t *persona, bool8_t expressLogin, int32_t startIndex);
    
	Login(StressConnection* connection, LoginManager* owner, uint8_t* ps3Ticket, size_t len, int32_t startIndex);
    
	Login(StressConnection* connection, LoginManager* owner,
            const char8_t* gamertag, uint64_t xuid, int32_t startIndex);
    
	Login(StressConnection* connection, LoginManager* owner,
        const char8_t* email, const char8_t* persona, uint64_t accountId, int32_t startIndex);
	
	Login(StressConnection* connection, LoginManager* owner,
        const char8_t* token, int32_t startIndex);
	
	Login(StressConnection* connection, LoginManager* owner, int32_t startIndex);

	Login(StressConnection* connection, LoginManager* owner, LoginAuthType authType, const char8_t* token, int32_t startIndex);

    Blaze::BlazeRpcError								login();
    Blaze::BlazeRpcError								logout();
    bool												isLoggedIn() const					{ return mLoggedIn; }
	void												resetLoggedIn()						{ mLoggedIn = false; }
    bool												isStressLogin() const				{ return mUseStressLogin;}
    LoginManager*										getOwner()							{ return mOwner; }
    const Authentication::UserLoginInfo*				getUserLoginInfo() const			{ return &mUserLoginInfo; }

	// Getters for the various login request objects. In case they need to be overridden by the module.
    //Authentication::LoginRequest*						getLoginRequest() const             { return (Authentication::LoginRequest*)&mLoginRequest; }
    Authentication::ExpressLoginRequest*				getExpressLoginRequest() const      { return (Authentication::ExpressLoginRequest*)&mExpressLoginRequest; }
    Authentication::StressLoginRequest*					getStressLoginRequest() const       { return (Authentication::StressLoginRequest*)&mStressLoginRequest; }
	Authentication::LoginRequest*						getLoginRequest()const				{ return (Authentication::LoginRequest*)&mLoginRequest; }

    void												setClientType(const ClientType& clientType)	{ mClientType = clientType; }
	uint32_t											getMyStartIndex()							{ return mMyStartIndex; }
	uint32_t											getMyIdent()							    { return mIdent; }
	eastl::string										getMyPid()							        { return mPid; }
	void												incrementMyStartIndex();

private:
    LoginManager*										mOwner;
    LoginAuthType										mAuthType;
    BlazeId 											mBlazeUserId;
	char8_t 											mToken[256];

    Blaze::Authentication::ExpressLoginRequest			mExpressLoginRequest;
    Blaze::Authentication::StressLoginRequest			mStressLoginRequest;
	Blaze::Authentication::LoginRequest					mLoginRequest;
    Blaze::Authentication::LoginResponse				mLoginResponse;
    //Authentication::CreateAccountResponse				mLoginErrorResponse;
    //shisingh Authentication::ConsoleCreateAccountRequest			mCreateArgs;
	//shisingh Authentication::ConsoleCreateAccountResponse		mCreateResp;
    Authentication::UserLoginInfo						mUserLoginInfo;
    //Authentication::CreateAccountResponse				mCreateAccountResponse;
    Authentication::FieldValidateErrorList				mFieldValidateErrorList;
    Authentication::AuthenticationSlave					mProxy;
    bool												mLoggedIn;
    bool8_t												mUseExpressLogin;
    bool8_t												mUseStressLogin;
    uint32_t											mIdent;
	eastl::string										mPid;
	uint16_t											mRetryCount;
    LoginCb												mCallback;
    ClientType											mClientType;
	uint32_t											mMyStartIndex;

	BlazeRpcError										NucleusFlow();
	BlazeRpcError										doPCLogin();
	BlazeRpcError										finishConsoleLogin();
	BlazeRpcError										finishAuthLogin(BlazeRpcError err);
};

class StressHttpResult : public OutboundHttpResult
{

private:
	BlazeRpcError										mErr;
	char												mAuthCode[256];
	char												mServerAuthCode[256];
	char												mAccessToken[256];
	char												mCode[256];

public:
	StressHttpResult(){
		mErr = ERR_OK;
		memset(mAuthCode,'\0',256);
		memset(mAccessToken,'\0',256);
		memset(mCode,'\0',256);
		memset(mServerAuthCode, '\0', 256);
	}
    void setHttpError(BlazeRpcError err = ERR_SYSTEM)
    {
		mErr = err;
    }
	bool hasError()
	{
		if (mErr != ERR_OK)
			return true;
		return false;
	}

    void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes,
            size_t attributeCount)
    {
		//Do nothing here
    }

	void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen)
	{
		BLAZE_TRACE_LOG(BlazeRpcLog::usersessions, "[StressHttpResult::setValue]:Received data = " << data);
		const char* token = strstr(data,"access_token");
		if(token!=NULL)
		{				
			const char* startPos = token+strlen("access_token")+2;
			char accessToken[256] = {'\0'};
			bool foundStart = false;
			for(int index = 0, resindex = 0;index < (int)(strlen(data));index++)
			{
				if(foundStart && startPos[index] == '"')
				{
					break;
				}
				if(!foundStart && startPos[index] == '"')
				{
					foundStart = true;
					continue;
					
				}
				if(foundStart)
				{
					accessToken[resindex] = startPos[index];
					resindex++;
				}
			}
			strncpy(mAccessToken,accessToken,strlen(accessToken)+1);
			return;
		} // if token is NULL is token careoff
		else
		{
			BLAZE_ERR(BlazeRpcLog::usersessions, "[StressHttpResult::setValue]: token is null - data value %s ",data);
		}
	}

    bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attribuets,
            size_t attrCount)
    {
        return false;
    }
	
	char* getAccessToken()
	{
		if(strlen(mAccessToken) ==  0 )
		{
			return NULL;
		}
		
		char* accesstoken = new char[strlen(mAccessToken)+1];				
		memset(accesstoken,'\0',strlen(mAccessToken)+1);	
		strcpy(accesstoken,mAccessToken);
		
		return accesstoken;
	}
	char* getCode(char* header)
	{
		char* code = NULL;
		HttpHeaderMap::const_iterator it = getHeaderMap().find(header);
		if(it!=getHeaderMap().end())
		{
			BLAZE_INFO(BlazeRpcLog::usersessions, "[StressHttpResult::getCode]: %s : %s",it->first.c_str(), it->second.c_str());
			//check for fid
			{
				bool foundStart = false;
				char codeName[16] = { '\0' };
				const char* address = it->second.c_str();
				int index = 0;
				for (int i = 0; i < (int)(strlen(address)); i++)
				{
					if (address[i] == '?')
					{
						index = i + 1;
						foundStart = true;
						break;
					}
				}
				if (foundStart)
				{
					for (int i = index, resindex = 0; i < (int)(strlen(address)); i++)
					{
						if (address[i] == '=')
						{
							break;
						}
						codeName[resindex] = address[i];
						resindex++;
					}
					if (blaze_stricmp("fid", codeName) == 0)
					{
						BLAZE_ERR(BlazeRpcLog::usersessions, "[StressHttpResult::getCode]: Fake Id generated");
						return NULL;
					}
					else
					{
						BLAZE_TRACE(BlazeRpcLog::usersessions, "[StressHttpResult::getCode]: codeName %s ", codeName);
					}
				}
			}
			//check for authcode 
			{
				const char* address = it->second.c_str();
				int index = 0;
				for (int i = 0; i < (int)(strlen(address)); i++)
				{
					if (address[i] == '=')
					{
						index = i + 1;
						break;
					}
				}
				code = new char[strlen(address + index) + 1];
				memset(code, '\0', strlen(address + index) + 1);
				strcpy(code, address + index);
				return code;
			}
		}
		else
		{
			return NULL;
		}
	}
};


class LoginManager : private StressConnection::ConnectionHandler
{
    NON_COPYABLE(LoginManager);

public:
    struct Xbox360AccountInfo
    {
        char8_t gamerTag[32];
        uint64_t xuid;
    };

    struct Ps3AccountInfo
    {
        uint8_t* ticket;
        size_t len;
    };

    LoginManager();
    ~LoginManager();
	typedef eastl::vector<eastl::string>			pidVector;
	typedef eastl::vector<Blaze::Authentication::StressLoginRequest>stressLoginRequestVector;
    bool											initialize(const ConfigMap& config, const CmdLineArguments& arguments);
	
	void											IntializeHttpConnection(); //initialize http conneciton
	const	pidVector&								getPidMem() const						{ return mPidMemVec; }
	const	stressLoginRequestVector				getStressLoginVector() const			{ return mStressLoginRequestVector; }
	bool											sendPCNucleusLoginRequest(eastl::string pid);  // Nucleus login
	bool											sendConsoleNucleusLoginRequest(const char* token);
	bool											readLoginDetailsFromFile();
	bool											readPidsFromFile();

    Login*											getLogin(StressConnection* connection, bool8_t useStressLogin, uint32_t startIndex = 0);

    typedef Functor1<uint32_t>						CreateAccountsCb;
    bool											createAccounts(StressConnection* connection, uint32_t count, const CreateAccountsCb& callback);

	const  Xbox360AccountInfo&						getXbox360AccountInfo(size_t index) const{ return mXbox360Accounts[index]; }

		   size_t									getNumXbox360Accounts() const			{ return mXbox360Accounts.size(); }

    void											adjustLoginCount(int32_t adj)			{ mLoginCount += adj; }
		  int32_t									getLoginCount() const					{ return mLoginCount; }
	const char8_t*									getPersonaFormat() const				{ return mPersonaFormat;}
    const char8_t*									getServiceName() const					{ return mServiceName; }
	const char8_t*									getPreAuthClientConfig() const			{ return mPreAuthClientConfig; }
		  uint32_t									getStartIndex() const					{ return mStartIndex; }
		  uint32_t									getPsu() const							{ return mPsu; }

	const char8_t*									getNucleusCreateAccEmailFormat() const	{ return mNucleusCreateAccEmailFormat; }
	const char8_t*									getNucleusCreateAccPassword() const		{ return mNucleusCreateAccPassword; }
	const char8_t*									getAuthCode()							{ return mAuthCode; }
	const char8_t* getServerAuthCode() { return mServerAuthCode; }
	const char8_t*									getAccessToken()						{ return mAccessToken; }
	const char8_t*									getServerPidPC()						{ return mServerPidPC; }
	uint16_t										getNumberOfRetrys()						{ return mNumberOfRetrys; }
	uint32_t										getMinStartIndex()						{ return mMinStartIndex; }
	uint32_t										getMaxStartIndex()						{ return mMaxStartIndex; }
	// get accesstoken
	bool											requestAccessToken(eastl::string pid);
	// get auth code
	bool											requestAuthCode(const char*token = NULL);
	// get authcode using server client id for console
	bool											requestServerAuthCode();
	bool											getUseNuceIdForStressLogin()			{ return doUseNucleusAccountsForStressLogin; }
    const char8_t*                                  getTokenFormat()                        { return mTokenFormat;}
	char8_t											mEslAuthCode[256];

private:
    typedef eastl::hash_map<uint32_t,Login*>		LoginsByInstanceId;
    typedef eastl::vector<Xbox360AccountInfo>		Xbox360AccountList;
    typedef eastl::vector<Ps3AccountInfo>			PS3AccountList;
	
	uint32_t										mStartIndex;
	uint32_t										mPsu;
    AccountId										mStartAccountId;
    char8_t											mEmailFormat[256];
    char8_t											mPassword[256];
    char8_t											mPersonaFormat[256];
    char8_t											mServiceName[256];
    char8_t											mPreAuthClientConfig[256];
	char8_t											mAuthenticationUri[256];
	char8_t											mClientId[256];
	char8_t											mTestClientId[256];
	char8_t											mClientSecret[256];
	char8_t											mTokenFormat[256];
	char8_t											mHeader[64];
	char8_t											mHeaderType[64];
	char8_t											mResponseType[64];
	char8_t											mRedirectUri[64];
	char8_t											mAuthCodePath[64];
	char8_t											mAccessTokenPath[64];
	char8_t											mPidFilePath[64];
	char8_t											mLoginDetailsFilePath[64];
	bool											doUseNucleusAccountsForStressLogin;
	char8_t											mServerPidPC[32];
	uint32_t										mNumberofPidtoRead;
    LoginAuthType									mAuthType;
    Xbox360AccountList								mXbox360Accounts;
    PS3AccountList									mPS3Accounts;
	char8_t											mNucleusCreateAccEmailFormat[256];
	char8_t											mNucleusCreateAccPassword[256];
	char8_t											mAuthCode[256] ;
	char8_t											mAccessToken[256];
	char8_t											mServerAuthCode[256];
	char8_t											mServerClientId[256];
	char8_t 										mTestClientSecret[256];
	char8_t 										mNucleusAccPassword[256];
	
    LoginsByInstanceId								mLogins;

    uint32_t										mCreateCount;
    uint32_t										mCreatedCount;
    CreateAccountsCb								mCreateCallback;
    StressConnection*								mCreateConnection;
    Authentication::AuthenticationSlave*			mAuthProxy;
	OutboundHttpConnectionManager*					mHttpConnectionManager;
	InetAddress*									mInetAddress;
	uint16_t										mPortNumber;
	uint16_t										mNumberOfRetrys;
	bool8_t											mIsSecure;

    bool8_t											mUseExpressLogin;
    int32_t											mLoginCount;
	pidVector										mPidMemVec;
	stressLoginRequestVector						mStressLoginRequestVector;

	uint32_t										mMaxStartIndex;
	uint32_t										mMinStartIndex;

    void											createAccount();
    void											createFinished();

    virtual void onDisconnected(StressConnection* conn, bool resumeSession);
    virtual void onSessionResumed(StressConnection* conn, bool success) {}
    virtual void onUserSessionMigrated(StressConnection* conn) {}
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_LOGINMANAGER_H

