/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#include "framework/blaze.h"
#include "loginmanager.h"
#include "stressinstance.h"
#include "stress.h"
#include "framework/connection/selector.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"

using namespace std;

//#include "authentication/nucleushandler/nucleus.h"
#include "authentication/tdf/authentication.h"

namespace Blaze
{
namespace Stress
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

static const uint64_t STRESS_ACCOUNT_ID_BASE_DEFAULT = 1000000000000LL;

/*** Public Methods ******************************************************************************/

Login::Login(StressConnection* connection, LoginManager* owner, const char8_t* email, const char8_t* password, const char8_t *persona, bool8_t expressLogin, int32_t startIndex)
  : mOwner(owner),
    mAuthType(AUTH_PC),
    mBlazeUserId(0),
    mProxy(*connection),
    mLoggedIn(false),
    mUseExpressLogin(expressLogin),
    mUseStressLogin(false),
    mIdent(connection->getIdent()),
	mMyStartIndex(startIndex)
{
    mExpressLoginRequest.setEmail(email);
    mExpressLoginRequest.setPassword(password);
    mExpressLoginRequest.setPersonaName(persona);
}

Login::Login(StressConnection* connection, LoginManager* owner, uint8_t* ps3Ticket, size_t len, int32_t startIndex)
    : mOwner(owner),
      mAuthType(AUTH_PS3),
      mBlazeUserId(0),
      mProxy(*connection),
      mLoggedIn(false),
      mUseExpressLogin(false),
      mUseStressLogin(false),
      mIdent(connection->getIdent()),
	  mMyStartIndex(startIndex)
{

}

Login::Login(StressConnection* connection, LoginManager* owner,
        const char8_t* gamertag, uint64_t xuid, int32_t startIndex)
    : mOwner(owner),
      mAuthType(AUTH_XBOX360),
      mBlazeUserId(0),
      mProxy(*connection),
      mLoggedIn(false),
      mUseExpressLogin(false),
      mUseStressLogin(false),
      mIdent(connection->getIdent()),
	  mMyStartIndex(startIndex)
{
}
Login::Login(StressConnection* connection, LoginManager* owner,
        const char8_t* token, int32_t startIndex)
    : mOwner(owner),
	  mAuthType(AUTH_XBOX360),
      mBlazeUserId(0),
	  mProxy(*connection),
      mLoggedIn(false),
      mUseExpressLogin(false),
      mUseStressLogin(false),
      mIdent(connection->getIdent()),
	  mMyStartIndex(startIndex)
{
	blaze_strnzcpy(mToken, token, sizeof(mToken));
	mRetryCount = mOwner->getNumberOfRetrys();
}

Login::Login(StressConnection* connection, LoginManager* owner, LoginAuthType authType, const char8_t* token, int32_t startIndex)
	: mOwner(owner),
	mAuthType(authType),
	mBlazeUserId(0),
	mProxy(*connection),
	mLoggedIn(false),
	mUseExpressLogin(false),
	mUseStressLogin(false),
	mIdent(connection->getIdent()),
	mMyStartIndex(startIndex)
{
	blaze_strnzcpy(mToken, token, sizeof(mToken));
	mRetryCount = mOwner->getNumberOfRetrys();
	//BLAZE_INFO_LOG("Login mAuthType  with mAuthType " << mAuthType);
}

Login::Login(StressConnection* connection, LoginManager* owner,
        const char8_t* email, const char8_t* persona, uint64_t accountId, int32_t startIndex)
    : mOwner(owner),
      mAuthType(AUTH_PC),
      mBlazeUserId(0),
      mProxy(*connection),
      mLoggedIn(false),
      mUseExpressLogin(false),
      mUseStressLogin(true),
      mIdent(connection->getIdent()),
	  mMyStartIndex(startIndex)
{
	if(mOwner->getUseNuceIdForStressLogin())
	{
		if (mIdent < mOwner->getStressLoginVector().size())
		{
			mStressLoginRequest = mOwner->getStressLoginVector().at(mIdent);
		}
	}
	else
	{
		mStressLoginRequest.setEmail(email);
		mStressLoginRequest.setPersonaName(persona);
		mStressLoginRequest.setAccountId(accountId);
	}
   
}

Login::Login(StressConnection* connection, LoginManager* owner, int32_t startIndex)
    : mOwner(owner),
	  mAuthType(AUTH_PC),
      mBlazeUserId(0),
	  mProxy(*connection),
      mLoggedIn(false),
      mUseExpressLogin(false),
      mUseStressLogin(false),
	  mIdent(connection->getIdent()),
	  mMyStartIndex(startIndex)
{
	mRetryCount = mOwner->getNumberOfRetrys();
    mClientType = CLIENT_TYPE_GAMEPLAY_USER;
}

BlazeRpcError Login::login()
{
    if (mLoggedIn)
    {
        return ERR_OK;
    }
	BlazeRpcError result = ERR_OK;

    if (mUseStressLogin)
    {
        result = mProxy.stressLogin(mStressLoginRequest, mLoginResponse);
    }
	else
	{
		result = NucleusFlow();
	}
    	
	if (result != ERR_OK)
	{
		mLoggedIn = false;
		return result;
	}

	if (mUseStressLogin)
	{
		mLoginResponse.getUserLoginInfo().copyInto(mUserLoginInfo);
		BLAZE_INFO_LOG(Log::SYSTEM, "[" << mIdent << "] " << "User logged in: " << mUserLoginInfo.getPersonaDetails().getDisplayName());
	}
	mLoggedIn = true;
	mOwner->adjustLoginCount(1);
	
    return result;
}

BlazeRpcError Login::NucleusFlow()
{
	BlazeRpcError result = ERR_OK;
	BLAZE_INFO_LOG(Log::SYSTEM, "[Login::nucleusTwoflow]: selected auth2 login");
    switch (mAuthType)
    {
        case AUTH_PC:
            return doPCLogin();
            break;
        case AUTH_PS3:
		case AUTH_XBOX360:
			return finishConsoleLogin();
            break;
	}
	return result;
}

BlazeRpcError Login::doPCLogin()
{
    BlazeRpcError result = ERR_OK;
    for(int tryCount=0; tryCount < mRetryCount; tryCount++)
    {
		switch (mAuthType)
		{
		case AUTH_PC:
			if (!mOwner->sendPCNucleusLoginRequest(mToken))
			{
				BLAZE_ERR_LOG(Log::SYSTEM, "Failed to get Authcode");
				return ERR_SYSTEM;
			}
			mLoginRequest.getPlatformInfo().setClientPlatform(Blaze::pc);
			break;
		case AUTH_PS3:
		case AUTH_XBOX360:
			break;
		}

		mLoginRequest.setAuthCode(mOwner->getAuthCode());

		BLAZE_TRACE_LOG(Log::SYSTEM, "[" << mIdent << "] " << "Login Request = " << mLoginRequest);
		result = finishAuthLogin(mProxy.login(mLoginRequest, mLoginResponse));
		BLAZE_TRACE_LOG(Log::SYSTEM, "[" << mIdent << "] " << "Login Response = " << mLoginResponse);
		if (result == ERR_OK)
		{
			mLoginResponse.getUserLoginInfo().copyInto(mUserLoginInfo);
			mLoggedIn = true;
			mOwner->adjustLoginCount(1);
			BLAZE_INFO_LOG(Log::SYSTEM, "[" << mIdent << "] " << "PC Auth2 User logged in: " << mUserLoginInfo.getPersonaDetails().getDisplayName());
			break;
		}
		else
		{
			mLoggedIn = false;
			BLAZE_ERR_LOG(Log::SYSTEM, "[" << mIdent << "] " << "Login Failed with error = " << Blaze::ErrorHelp::getErrorName(result));
		}
    }
    if( ERR_OK == result)
    {
		mLoginResponse.getUserLoginInfo().copyInto(mUserLoginInfo);
        mLoggedIn = true;
        mOwner->adjustLoginCount(1);
        BLAZE_INFO_LOG(Log::SYSTEM, "[" << mIdent << "] " << "PC Auth2 User logged in: " << mUserLoginInfo.getPersonaDetails().getDisplayName());
    }
    else
    {
        mLoggedIn = false;
    }

    return result;
}

BlazeRpcError Login::finishConsoleLogin()
{
	BlazeRpcError result = ERR_OK;
	for(int tryCount=0; tryCount<mRetryCount;tryCount++)
	{
		switch (mAuthType)
		{
			case AUTH_PC:  
			case AUTH_XBOX360:
				if(!mOwner->sendConsoleNucleusLoginRequest(mToken))
				{
					BLAZE_ERR_LOG(Log::SYSTEM, "Failed to get Authcode");
					return ERR_SYSTEM;
				}
				mLoginRequest.getPlatformInfo().setClientPlatform(Blaze::ps4);
				break;
			case AUTH_PS3:
				if(!mOwner->sendConsoleNucleusLoginRequest(mToken))
				{
					BLAZE_ERR_LOG(Log::SYSTEM, "Failed to get Authcode");
					return ERR_SYSTEM;
				}
				mLoginRequest.getPlatformInfo().setClientPlatform(Blaze::xone);
				break;
		}
		//mLoginRequest.setAuthCode(mOwner->getAuthCode());
		mLoginRequest.setAuthCode(mOwner->getServerAuthCode());
		//char8_t buf[20240];
		//BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "LoginRequest RPC : \n" << mLoginRequest.print(buf, sizeof(buf)));
		result = finishAuthLogin(mProxy.login(mLoginRequest,mLoginResponse));

		if(result != AUTH_ERR_INVALID_TOKEN)
		{
			break;
		}
	}

	return result;
}
BlazeRpcError Login::finishAuthLogin(BlazeRpcError err)
{
	//TODO : Need to Handle Create Account 
	if (err != Blaze::ERR_OK)
    {
		mLoggedIn = false;
        return err;
    }
	else
    {
        // Save off the response
		mLoginResponse.getUserLoginInfo().copyInto(mUserLoginInfo);
	}
	
	mLoggedIn = true;
    mOwner->adjustLoginCount(1);

	BLAZE_INFO_LOG(Log::SYSTEM, "[" << mIdent << "] " << "Xbl2/Ps3 Auth2 User logged in: " << mUserLoginInfo.getPersonaDetails().getDisplayName() << " with start index " << getMyStartIndex());

    return ERR_OK;
}

BlazeRpcError Login::logout()
{
    BlazeRpcError result = ERR_OK;

    if (!mLoggedIn)
    {
        return result;
    }
    BLAZE_INFO_LOG(Log::SYSTEM, "[" << mIdent << "] " << "Logging out Persona " << mUserLoginInfo.getPersonaDetails().getDisplayName());

    result = mProxy.logout();
    if(result != ERR_OK){
        return result;
    }

    mLoggedIn = false;
    mOwner->adjustLoginCount(-1);

    return result;
}

// This function increments the start index for the player by PSU.
// incrementing the start index by PSU makes sure that no two players start index
// conflict with each other.
void Login::incrementMyStartIndex()
{
	mMyStartIndex = mMyStartIndex + mOwner->getPsu();
	if (mMyStartIndex >= mOwner->getMaxStartIndex())
	{
		// in case players start index reached max , reset it to starting value
		mMyStartIndex = getOwner()->getStartIndex();
	}
}

LoginManager::LoginManager()
    : mPsu(0),
	  mAuthType(AUTH_PC),
      mCreatedCount(0),
      mCreateConnection(NULL),
      mAuthProxy(NULL),
      mUseExpressLogin(false),
      mLoginCount(0),
	  mMaxStartIndex(0),
	  mMinStartIndex(0)
{
    mEmailFormat[0] = '\0';
    mPassword[0] = '\0';
    mPersonaFormat[0] = '\0';
	mAccessToken[0] = '\0';
	mServiceName[0] = '\0';
	mPreAuthClientConfig[0] = '\0';
	mAuthenticationUri[0] = '\0';
	mClientId[0] = '\0';
	mTestClientId[0] = '\0';
	mTestClientSecret[0] = '\0';
	mServerClientId[0] = '\0';
	mNucleusAccPassword[0] = '\0';
	mClientSecret[0] = '\0';
	mTokenFormat[0] = '\0';
	mHeader[0] = '\0';
	mHeaderType[0] = '\0';
	mResponseType[0] = '\0';
	mRedirectUri[0] = '\0';
	mAuthCodePath[0] = '\0';
	mAccessTokenPath[0] = '\0';
	mPidFilePath[0] = '\0';
	mLoginDetailsFilePath[0] = '\0';
    mServerPidPC[0] = '\0';
}

LoginManager::~LoginManager()
{
    LoginsByInstanceId::iterator i = mLogins.begin();
    LoginsByInstanceId::iterator e = mLogins.end();
    for(; i != e; ++i)
        delete i->second;

    delete mAuthProxy;
	delete mInetAddress;
}

bool LoginManager::initialize(const ConfigMap& config, const CmdLineArguments& arguments)
{
    const char8_t* emailFormat = config.getString("email-format", "stress%06x@blaze.ea.com");
    const char8_t* password = config.getString("password", "password");
    const char8_t* personaFormat = config.getString("persona-format", "stress%06x");
    //const char8_t* tos = config.getString("tos", "2.0");
    const char8_t* authType = config.getString("type", "pc");
    const char8_t* serviceName = config.getString("serviceName", "");
	const char8_t*  authUri = config.getString("authenticationUri","https://accounts.lt.internal.ea.com");
	const char8_t* tokenFormat = config.getString("token-format","nhllt%6d");
	//const char8_t* nucleusAuthVersion = config.getString("nucleusAuthVersion", "0");  // default 0, to support old scripts/cardhouse
	const char8_t* clientid = config.getString("client-id","NHL_2015_SERVER_CAPILANO");
	const char8_t* testclientid = config.getString("testclient-id","test_token_issuer");//this is for getting access token
	const char8_t* clientsecret = config.getString("client-secret","NHL_2015_SERVER_CAPILANO_SECRET");
	const char8_t* serverPidPC = config.getString("server-pid","");
	const char8_t* header		= config.getString("header","Location");
	const char8_t* headertype	= config.getString("header-type","Content-Type:application/x-www-form-urlencoded");
	const char8_t* responsetype	= config.getString("response-type","code");
	const char8_t* redirecturi	= config.getString("redirecturi","http://127.0.0.1/login_successful.html");
	const char8_t* authcodepath	= config.getString("authcodepath","/connect/auth");
	const char8_t* accesstokenpath	= config.getString("accesstokenpath","/connect/token");
	const char8_t* pidfilepath	=  config.getString("pidfilepath","pid.txt");
	const char8_t* loginDetailsFilePath = config.getString("loginDetailsFilePath","loginDetails.csv");
	const char8_t* testclientidSecret = config.getString("testclient-secret", "test_token_issuer_secret"); //this is for getting access token
	const char8_t* nucleusPassword = config.getString("nucleus-password", "loadtest1234");
	const char8_t* serverclientid = config.getString("serverclient-id", "FIFA21_PS4_BLZ_SERVER");

				   
	doUseNucleusAccountsForStressLogin = config.getBool("useNuceIdForStressLogin", false);
	
	mMaxStartIndex = config.getUInt32("maxStartIndex", 100000);
	mMinStartIndex = config.getUInt32("minStartIndex", 0);
	
	mNumberofPidtoRead = config.getInt32("numberofpidtoread",5000);
	mPortNumber = config.getInt16("portNumber",443);
	mIsSecure = config.getBool("isSecure",true);
	mNumberOfRetrys = config.getInt16("numberOfRetrys",1);
	if (blaze_stricmp(authType, "xbox360") == 0)
	{
		blaze_strnzcpy(mTokenFormat, "xonetoken: xone_", sizeof(mTokenFormat));
	}
	else if(blaze_stricmp(authType, "ps3") == 0)
	{
		 blaze_strnzcpy(mTokenFormat, "ps4_ticket: ps4_", sizeof(mTokenFormat));
	}
	
    mUseExpressLogin = config.getBool("express-pc-login", false);

    mStartIndex = arguments.startIndex != 0 ? arguments.startIndex : config.getUInt32("start-index", 0);
	mPsu = arguments.psu;
    mStartAccountId = config.getInt64("start-nucleus-id", STRESS_ACCOUNT_ID_BASE_DEFAULT);
	blaze_strnzcpy(mAuthenticationUri, authUri, sizeof(mAuthenticationUri));
    blaze_strnzcpy(mEmailFormat, emailFormat, sizeof(mEmailFormat));
    blaze_strnzcpy(mPassword, password, sizeof(mPassword));
    blaze_strnzcpy(mPersonaFormat, personaFormat, sizeof(mPersonaFormat));
    blaze_strnzcpy(mServiceName, serviceName, sizeof(mServiceName));
	blaze_strnzcat(mTokenFormat, tokenFormat, sizeof(mTokenFormat));
	blaze_strnzcpy(mClientId, clientid, sizeof(mClientId));
	blaze_strnzcpy(mTestClientId, testclientid, sizeof(mTestClientId));
	blaze_strnzcpy(mHeader, header, sizeof(mHeader));
	blaze_strnzcpy(mHeaderType,headertype,sizeof(mHeaderType));
	blaze_strnzcpy(mResponseType,responsetype,sizeof(mResponseType));
	blaze_strnzcpy(mRedirectUri,redirecturi,sizeof(mRedirectUri));
	blaze_strnzcpy(mAuthCodePath,authcodepath,sizeof(mAuthCodePath));
	blaze_strnzcpy(mAccessTokenPath,accesstokenpath,sizeof(mAccessTokenPath));
	blaze_strnzcpy(mPidFilePath,pidfilepath,sizeof(mPidFilePath));
	blaze_strnzcpy(mLoginDetailsFilePath,loginDetailsFilePath,sizeof(mLoginDetailsFilePath));
	blaze_strnzcpy(mServerPidPC,serverPidPC,sizeof(mServerPidPC));
	blaze_strnzcpy(mClientSecret, clientsecret, sizeof(mClientSecret));
	blaze_strnzcpy(mPreAuthClientConfig, config.getString("preAuthClientConfig", "BlazeSDK"), sizeof(mPreAuthClientConfig));
	blaze_strnzcpy(mTestClientSecret, testclientidSecret, sizeof(mTestClientSecret));
	blaze_strnzcpy(mServerClientId, serverclientid, sizeof(mServerClientId));
	blaze_strnzcpy(mNucleusAccPassword, nucleusPassword, sizeof(mNucleusAccPassword));

    if (blaze_stricmp(authType, "pc") == 0)
	{
        mAuthType = AUTH_PC;
	}
    else if (blaze_stricmp(authType, "ps3") == 0)
    {
        mAuthType = AUTH_PS3;
     }
    else if (blaze_stricmp(authType, "xbox360") == 0)
	{
        mAuthType = AUTH_XBOX360;
	}
    else
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "Invalid authentication type: " << authType << ".");
        return false;
    }
	
	IntializeHttpConnection(); 
	if(mAuthType == AUTH_PC)
	{
		if(readPidsFromFile())
		{
			BLAZE_INFO_LOG(Log::SYSTEM,"Pids successfully read from the file");
		}
		else
		{
			BLAZE_ERR_LOG(Log::SYSTEM, "Issues with reading pid.txt ");
			return false;
		}
	}

	if(doUseNucleusAccountsForStressLogin)
	{
		if(readLoginDetailsFromFile())
		{
			BLAZE_INFO_LOG(Log::SYSTEM,"Login details successfully read from the file");
		}
		else
		{
			BLAZE_ERR_LOG(Log::SYSTEM, "Issues with reading login details file");
			return false;
		}

	}

    return true;
}
bool LoginManager::requestAccessToken(eastl::string pid)
{
	bool retValue = false;
	StressHttpResult httpResult;
	HttpParam params[8];

	if (pid.size() <= 0 && (mAuthType == AUTH_PC))
	{
		BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager::requestAccessToken]: received empty pid");
		return retValue;
	}
	
	BLAZE_INFO(BlazeRpcLog::usersessions, "[LoginManager::sendConsoleNucleusLoginRequest]: getting code for login");

    // set http header to  x-www-form-urlencoded
	eastl::string strHttpHeader;
	//strHttpHeader.sprintf("Content-Type:application/x-www-form-urlencoded");
	strHttpHeader.sprintf(mHeaderType);
	const char8_t *httpHeaders[1];
	httpHeaders[0] = strHttpHeader.c_str();
	BlazeRpcError err = ERR_SYSTEM;

	// Fill the request here
	uint32_t paramCount = 0;

	/*params[paramCount].name = "client_id";
    params[paramCount].value = mTestClientId;
    params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "client_secret";
    params[paramCount].value = mClientSecret;
    params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "pidId";
    params[paramCount].value = pid.c_str();
    params[paramCount].encodeValue = true;
	paramCount++;
	
	params[paramCount].name = "grant_type";
    params[paramCount].value = "client_credentials";
    params[paramCount].encodeValue = true;
	paramCount++;
	
	err = mHttpConnectionManager->sendRequest(HttpProtocolUtil::HTTP_POST,
        mAccessTokenPath,
        params,
        paramCount,
        httpHeaders,
        sizeof(httpHeaders)/sizeof(char8_t*),
        &httpResult);
		
	if (ERR_OK != err)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager::requestAccessToken]: failed to fetch Access Token.");
		return retValue;
			
    }
	if(httpResult.hasError())
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[LoginManager::requestAuthCode]: Nucleus returned error");
		return retValue;	
	}
	char *acesstoken = httpResult.getAccessToken();
	if(acesstoken != NULL)
	{
		blaze_strnzcpy(mAccessToken, acesstoken, sizeof(mAccessToken));
		BLAZE_INFO_LOG(Log::SYSTEM, "Access Token for: ["<< pid << "] generated success: " << mAccessToken << ".");
		retValue = true;
		delete[] acesstoken;  //delete temporary acesstoken created on Heap
	}
	else
	{
		BLAZE_ERR_LOG(Log::SYSTEM, "Access Token for: ["<< pid << "] generated Failed ");
		return retValue;
	}
	return retValue;*/

	params[paramCount].name = "client_id";
	params[paramCount].value = mClientId;
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "client_secret";
	params[paramCount].value = mClientSecret;
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "scope";
	params[paramCount].value = "signin";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "code";
	params[paramCount].value = getAuthCode();
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "pidId";
	params[paramCount].value = pid.c_str();
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "grant_type";
	params[paramCount].value = "authorization_code";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "redirect_uri";
	params[paramCount].value = mRedirectUri;
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "release_type";
	params[paramCount].value = "lt";
	params[paramCount].encodeValue = true;
	paramCount++;

	err = mHttpConnectionManager->sendRequest(HttpProtocolUtil::HTTP_POST,
		mAccessTokenPath,
		params,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);

if (ERR_OK != err)
{
	BLAZE_ERR_LOG(Log::SYSTEM, "[LoginManager::requestAccessToken]: failed to fetch Access Token.");
	return retValue;

}
BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[LoginManager::requestAuthCode]: Status Codet" << httpResult.getHttpStatusCode());

/*    if (httpResult.hasError() == true)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[LoginManager::requestAuthCode]: Nucleus returned error");
		return retValue;
	}
*/
char* acesstoken = httpResult.getAccessToken();

if (acesstoken != NULL)
{
	blaze_strnzcpy(mAccessToken, acesstoken, sizeof(mAccessToken));
	BLAZE_INFO_LOG(Log::SYSTEM, "Access Token for: [" << pid << "] generated success: " << mAccessToken << ".");
	retValue = true;
	delete[] acesstoken;  //delete temporary acesstoken created on Heap
}
else
{
	BLAZE_ERR_LOG(Log::SYSTEM, "Access Token for: [" << pid << "] generated Failed ");
	return retValue;
}

return retValue;
}

bool LoginManager::requestServerAuthCode()
{
	BlazeRpcError err = ERR_SYSTEM;
	bool retValue = false;
	StressHttpResult httpResult;
	HttpParam params[4];
	// Fill the request here
	uint32_t paramCount = 0;
	eastl::string strHttpHeader = "access_token:";
	strHttpHeader.append_sprintf(getAccessToken());
	const char8_t* httpHeaders[1];
	httpHeaders[0] = strHttpHeader.c_str();
	params[paramCount].name = "response_type";
	params[paramCount].value = mResponseType;
	params[paramCount].encodeValue = true;
	paramCount++;
	params[paramCount].name = "client_id";
	params[paramCount].value = mServerClientId;
	params[paramCount].encodeValue = true;
	paramCount++;
	params[paramCount].name = "release_type";
	params[paramCount].value = "lt";
	params[paramCount].encodeValue = true;
	paramCount++;
	params[paramCount].name = "redirect_uri";
	params[paramCount].value = mRedirectUri;
	params[paramCount].encodeValue = true;
	paramCount++;
	err = mHttpConnectionManager->sendRequest(HttpProtocolUtil::HTTP_GET,
		mAuthCodePath,
		params,
		paramCount,
		httpHeaders,
		sizeof(httpHeaders) / sizeof(char8_t*),
		&httpResult);
	if (ERR_OK != err)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[LoginManager::requestServerAuthCode]: failed send the request");
		return retValue;
	}
	BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "[LoginManager::requestServerAuthCode]: Status Codet" << httpResult.getHttpStatusCode());
	if (httpResult.hasError() == true)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[LoginManager::requestServerAuthCode]: Nucleus returned error");
		return retValue;
	}
	char8_t* authCode = httpResult.getCode(mHeader);
	if (authCode != NULL)
	{
		blaze_strnzcpy(mServerAuthCode, authCode, sizeof(mServerAuthCode));
		BLAZE_INFO_LOG(Log::SYSTEM, "SERVER Auth code generated success: " << authCode);
		retValue = true;
		delete[] authCode;   //delete temporary authCode created on Heap
	}
	else
	{
		BLAZE_ERR_LOG(Log::SYSTEM, "Server Auth Code for generated Failed.");
		retValue = false;
	}
	return retValue;
}

bool LoginManager::requestAuthCode(const char*token )
{
	if (mAuthType == AUTH_PC)
	{
		return true;
	}
	BlazeRpcError err = ERR_SYSTEM;
	bool retValue = false;
	StressHttpResult httpResult;
	HttpParam params[7];
    // Fill the request here
	uint32_t paramCount = 0;

	params[paramCount].name = "response_type";
    params[paramCount].value = mResponseType;
    params[paramCount].encodeValue = true;
	paramCount++;
	
	params[paramCount].name = "client_id";
    params[paramCount].value = mClientId;
    params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "release_type";
	params[paramCount].value = "lt";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "redirect_uri";
	params[paramCount].value = mRedirectUri;
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "guest_mode";
	params[paramCount].value = "false";
	params[paramCount].encodeValue = true;
	paramCount++;

	params[paramCount].name = "ps4_env";
	params[paramCount].value = "lt";
	params[paramCount].encodeValue = true;
	paramCount++;
	if(mAuthType == AUTH_PC)
	{
		params[paramCount].name = "access_token";
		params[paramCount].value = getAccessToken();
		params[paramCount].encodeValue = true;
		paramCount++;

		params[paramCount].name = "redirect_uri";
		params[paramCount].value = mRedirectUri;
		params[paramCount].encodeValue = true;
		paramCount++;
		err = mHttpConnectionManager->sendRequest(HttpProtocolUtil::HTTP_GET,
        mAuthCodePath,
        params,
        paramCount,
        NULL,
        0,
        &httpResult);
	}
	else
	{
		eastl::string strHttpHeader;
		strHttpHeader.sprintf(token);
		const char8_t *httpHeaders[1];
		httpHeaders[0] = strHttpHeader.c_str();
		BLAZE_INFO_LOG(BlazeRpcLog::usersessions, "token is " << httpHeaders[0]);
		params[paramCount].name = "display";
		params[paramCount].value = "console2/welcome";
		params[paramCount].encodeValue = true;
		paramCount++;

		err = mHttpConnectionManager->sendRequest(HttpProtocolUtil::HTTP_GET,
        mAuthCodePath,
        params,
        paramCount,
        httpHeaders,
        sizeof(httpHeaders)/sizeof(char8_t*),
        &httpResult);
	}
	
	if (ERR_OK != err)
    {
        BLAZE_ERR(BlazeRpcLog::usersessions, "[LoginManager::requestAuthCode]: failed send the request");
		return retValue;
    }
	if(httpResult.hasError() && httpResult.getHttpStatusCode() != 302)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[LoginManager::requestAuthCode]: Nucleus returned error");
		return retValue;	
	}
	char8_t *authCode = httpResult.getCode(mHeader);

	if(authCode != NULL)
	{
		blaze_strnzcpy(mAuthCode, authCode, sizeof(mAuthCode));
		blaze_strnzcpy(mEslAuthCode, authCode, sizeof(mEslAuthCode));
		BLAZE_INFO_LOG(Log::SYSTEM, "Auth code generated success: " << authCode );
		retValue = true;
		delete[] authCode;   //delete temporary authCode created on Heap
	}
	else
	{
		BLAZE_ERR_LOG(Log::SYSTEM, "Auth Code for generated Failed.");
		retValue = false;
	}
	return retValue;
}
bool LoginManager::sendPCNucleusLoginRequest(eastl::string pid)
{
	bool retValue = false;
	// get accesstoken
	if(requestAccessToken(pid))
	{
		// get auth code
		if(requestServerAuthCode())
		{
			retValue = true;
		}
		else
		{
			BLAZE_ERR_LOG(Log::SYSTEM, " Auth Code for [" << pid << "] generation Failed.");
		}
	}
	else
	{
		BLAZE_ERR_LOG(Log::SYSTEM, " Access Token for [" << pid << "] generation Failed.");
	}

	return retValue;
        
}
bool LoginManager::sendConsoleNucleusLoginRequest(const char* token)
{
	if (token == NULL)
	{
		BLAZE_INFO(BlazeRpcLog::usersessions,"[LoginManager::sendConsoleNucleusLoginRequest]: received empty accessToken");
		return false;
	}
	
	BLAZE_INFO(BlazeRpcLog::usersessions, "[LoginManager::sendConsoleNucleusLoginRequest]: getting code for login");
	eastl::string pid;

	if(requestAuthCode(token))
	{
		if (requestAccessToken(pid))
		{
			if (requestServerAuthCode())
			{
				return true;
			}
		}
	}
	else
	{
		BLAZE_ERR_LOG(Log::SYSTEM, "Auth Code generation Failed.");
		return false;
	}
	return false;
}
void LoginManager::IntializeHttpConnection()
{
	BLAZE_INFO(BlazeRpcLog::usersessions, "[LoginManager::IntializeHttpConnection]: Initializing the HttpConnectionManager");
	mHttpConnectionManager = BLAZE_NEW OutboundHttpConnectionManager("StressLoginManager");
		mHttpConnectionManager = BLAZE_NEW OutboundHttpConnectionManager("framework"); //hostname); //mServiceName);

	if(mHttpConnectionManager == NULL)
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[LoginManager::IntializeHttpConnection]: Failed to Initialize the HttpConnectionManager");
		return;
	}
	const char8_t* serviceHostname = NULL;
	bool serviceSecure = false;
	HttpProtocolUtil::getHostnameFromConfig(mAuthenticationUri, serviceHostname, serviceSecure);
	mInetAddress = BLAZE_NEW InetAddress(serviceHostname,mPortNumber);
	if( mInetAddress != NULL)
	{
		mHttpConnectionManager->initialize(*mInetAddress, 4, mIsSecure, false);
	}
	else
	{
		BLAZE_ERR(BlazeRpcLog::usersessions, "[LoginManager::IntializeHttpConnection]: Failed to Create InetAddress with given hostname");
	}
}

void seekToLine(std::ifstream& file, int lineNumber)
{
	file.seekg(std::ios::beg);
	for(int i=0; i<lineNumber; i++)
	{
		file.ignore(eastl::numeric_limits<std::streamsize>::max(), '\n');
	}
}

bool LoginManager::readLoginDetailsFromFile()
{
	std::ifstream			 loginDetailsFile(mLoginDetailsFilePath);
	std::vector<std::string> loginDetails;
	std::vector<std::string> row;
	std::string				 line;
    std::string				 cell;
	
	if (loginDetailsFile)
	{
		loginDetailsFile.clear();
		uint32_t counter = 0;
		seekToLine(loginDetailsFile, mStartIndex);
		BLAZE_INFO_LOG(Log::SYSTEM, "Reading Login details form the position : " << mStartIndex << ".");
		if(mNumberofPidtoRead <= 0)
		{
			mNumberofPidtoRead = 5100;
		}
	
		while (loginDetailsFile && counter++ < mNumberofPidtoRead)
		{
			std::getline(loginDetailsFile,line);
			std::stringstream lineStream(line);
			row.clear();
        
			while (std::getline(lineStream, cell, ','))
			{
				row.push_back(cell);
			}

			if (!row.empty())
			{
				Blaze::Authentication::StressLoginRequest stressLoginRequest;
				stressLoginRequest.setEmail(row[0].c_str());
				stressLoginRequest.setPersonaName(row[2].c_str());
				uint64_t nucId = stoull(row[3], NULL);
				stressLoginRequest.setAccountId(nucId);
                    
				mStressLoginRequestVector.push_back(stressLoginRequest);
			}
		}
	}
	else
	{
		BLAZE_ERR_LOG(Log::SYSTEM, "not able to open loginDetailsFile ");
		return false;
	}

	return true;
}

bool LoginManager::readPidsFromFile()
{
	//std::ifstream pidFile("pid.txt");
	std::ifstream pidFile(mPidFilePath);
	
	if (pidFile.is_open())
	{
		std::string line;
		while(std::getline(pidFile,line))
		{
			eastl::string str = line.c_str();
			if (str.length() <= 0)
				continue;
			mPidMemVec.push_back(str);
			line.clear();
		}
		BLAZE_INFO_LOG(Log::SYSTEM, "Pids in the cache : " << mPidMemVec.size() << ".");
		return true;
	}
	else
	{
		BLAZE_ERR_LOG(Log::SYSTEM, "Unable to open file pid.txt ");
		return false;
	}

}

//creates a login object for a player. when startIndex argument is 0 it will use the start index given to stress exe.
Login* LoginManager::getLogin(StressConnection* connection, bool8_t useStressLogin, uint32_t startIndex  /*= 0 if its a new login, not a renew player*/)
{
    uint32_t id = connection->getIdent();
	uint32_t index;
	(startIndex)? index = startIndex : index = mStartIndex;
    LoginsByInstanceId::iterator find = mLogins.find(id);
    Login* login = NULL;

	//a non-zero startIndex will be given if we want to renew this player(with same ident) hence we'll have to create a new login
	//object even if this id is already there in login map.
    if ((startIndex == 0) && find != mLogins.end())
    {
        login = find->second;
		return login;
    }
    else if (useStressLogin)
    {
        char8_t emailBuf[256];
        char8_t personaBuf[256];
        blaze_snzprintf(emailBuf, sizeof(emailBuf), mEmailFormat, index + id);
        blaze_snzprintf(personaBuf, sizeof(personaBuf), mPersonaFormat, index + id);
        uint64_t accountId = mStartAccountId + index + id;

        login = BLAZE_NEW Login(connection, this, emailBuf, personaBuf, accountId, index);
    }
    else
    {
        char8_t emailBuf[256];
        char8_t personaBuf[256];
        blaze_snzprintf(emailBuf, sizeof(emailBuf), mEmailFormat, index + id);
        blaze_snzprintf(personaBuf, sizeof(personaBuf), mPersonaFormat, index + id);

		char8_t tokenName[256];
		switch (mAuthType)
		{
			case AUTH_PC:
				login = BLAZE_NEW Login(connection, this, index);
				break;
			case AUTH_PS3:
				blaze_snzprintf(tokenName, sizeof(tokenName), mTokenFormat, index + id);
				login = BLAZE_NEW Login(connection, this, mAuthType, tokenName, index);
				break;
			case AUTH_XBOX360:
				blaze_snzprintf(tokenName, sizeof(tokenName), mTokenFormat, index + id);
				login = BLAZE_NEW Login(connection, this, mAuthType, tokenName, index);
				break;
		}
    }
	mLogins[id] = login;
    return login;
}

bool LoginManager::createAccounts(
        StressConnection* connection, uint32_t count, const CreateAccountsCb& callback)
{
    if (count == 0)
    {
        callback(0);
        return true;
    }

    mCreateConnection = connection;
    mAuthProxy = BLAZE_NEW Authentication::AuthenticationSlave(*connection);
    mCreateCallback = callback;
    mCreateCount = count;

    mCreateConnection->setConnectionHandler(this);
    gSelector->scheduleFiberCall(this, &LoginManager::createAccount, "LoginManager::createAccount");

    return true;
}

/*** Private Methods *****************************************************************************/


void LoginManager::onDisconnected(StressConnection* conn, bool resumeSession)
{
    BLAZE_ERR_LOG(Log::SYSTEM, "[" << conn->getIdent() << "] " << "Lost connection to server.");
}

void LoginManager::createFinished()
{
    mCreateCallback(mCreatedCount);
}

void LoginManager::createAccount()
{
    if (!mCreateConnection->connect())
        return;

    uint64_t index = mStartIndex - 1;
    while (mCreateConnection->connected() && mCreatedCount < mCreateCount)
    {
        index++;
        //Blaze::Authentication::CreateAccountParameters createAccountRequest;
        //Blaze::Authentication::CreateAccountResponse createAccountResponse;
        Blaze::Authentication::FieldValidateErrorList acctFieldValidateErrorList;

        char8_t emailBuf[256];
        blaze_snzprintf(emailBuf, sizeof(emailBuf), mEmailFormat, index);
        char personaNameBuf[256];
        blaze_snzprintf(personaNameBuf, sizeof(personaNameBuf), mPersonaFormat, index);

        /*createAccountRequest.setEmail(emailBuf);
        createAccountRequest.setPassword(mPassword);
        createAccountRequest.setPersonaName(personaNameBuf);
        createAccountRequest.setBirthDay(24);
        createAccountRequest.setBirthMonth(2);
        createAccountRequest.setBirthYear(1955);
        createAccountRequest.setIsoCountryCode("US");
        createAccountRequest.setIsoLanguageCode("en");
        createAccountRequest.setTermsOfServiceUri("1");
        createAccountRequest.setPrivacyPolicyUri("1");
        createAccountRequest.setEaEmailAllowed(0);
        createAccountRequest.setThirdPartyEmailAllowed(0);*/

		BlazeRpcError errCode = ERR_OK; // mAuthProxy->expressCreateAccount(createAccountRequest, createAccountResponse, acctFieldValidateErrorList);

        if ((errCode != Blaze::ERR_OK) && (errCode != Blaze::AUTH_ERR_EXISTS))
        {
            /*BLAZE_WARN(Log::SYSTEM, "Account '%s' could not be created: %s (%s)",
                    createAccountRequest.getEmail(), ErrorHelp::getErrorDescription(errCode),
                    ErrorHelp::getErrorName(errCode));*/

            // Skip persona creation but continue trying to create accounts
            continue;
        }
        else
        {
            mCreatedCount++;
            //BLAZE_INFO(Log::SYSTEM, "Account '%s' created with persona '%s'.", createAccountRequest.getEmail(), createAccountRequest.getPersonaName());
        }
    }

    gSelector->scheduleCall(this, &LoginManager::createFinished);
}


} // namespace Stress
} // namespace Blaze

