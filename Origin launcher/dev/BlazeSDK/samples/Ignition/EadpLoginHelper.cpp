/**************************************************************************************************/
/*!
    \file EadpLoginHelper.cpp

    \attention
        DO NOT COPY CODE FROM THIS FILE INTO SHIPPING PRODUCT
        Code in this file is for Blaze Dev Team internal test purpose only. It may include hacks and
        shortcuts which are not suitable for shipping product.
        Code in this file is written referencing EadpAuthentication examples.
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#include "Ignition/EadpLoginHelper.h"
#include "Ignition/IgnitionClientId.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"

#include "eadp/foundation/Config.h"
#include "eadp/foundation/DirectorService.h"
#include "eadp/foundation/IdentityService.h"

#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_NX) || defined(EA_PLATFORM_STADIA)
#include "EAUser/EAUser.h"
#endif

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
#include "EAControllerUserPairing/EAControllerUserPairingServer.h"
#endif

#if defined(EA_PLATFORM_STADIA)
#include "ggp/id.h"
#include "ggp/player.h"
#endif

namespace Ignition
{

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
extern EA::Pairing::EAControllerUserPairingServer* gpEacup;
#endif

const uint32_t USER_INDEX = 0;

const char8_t* EAORIGIN_TITLE = "Blaze Ignition";
const char8_t* EAORIGIN_CONTENT_ID = "71314";
const char8_t* EAORIGIN_MULTIPLAYER_ID = "blaze_ignition_multiplayer_id";

const eadp::foundation::Callback::GroupId EAORIGIN_CALLBACK_GROUP_ID = 107;
const eadp::foundation::Callback::GroupId AUTH_CALLBACK_GROUP_ID = 108;

EadpLoginHelper::EadpLoginHelper(EadpLoginListener& owner) :
    mOwner(owner),
    mCrossPlatformOpt(Blaze::Authentication::CrossPlatformOptSetting::DEFAULT),
    mAuthOption(AUTH_CODE)
{
    mProductName[0] = '\0';
    mPersistenceObject = gEadpHub->getAllocator().makeShared<eadp::foundation::Callback::Persistence>();

    mId20AuthService = eadp::authentication::ID20AuthService::createService(gEadpHub);

#if defined(EA_PLATFORM_WINDOWS)
    mIsOriginClientConnected = false;
    mOriginService = ea::origin::IOriginService::createService(gEadpHub);
    mId20AuthService->initialize(IGNITION_CLIENT_ID, IGNITION_CLIENT_SECRET, mOriginService);
#else
    mBrowserProvider = eadp::browserprovider::SystemBrowserProvider::create(gEadpHub);
    mId20AuthService->initialize(IGNITION_CLIENT_ID, IGNITION_CLIENT_SECRET, mBrowserProvider);
#endif

    char8_t localeLanguage[16];
    char8_t localeCountry[16];
    uint32_t localeToken = gBlazeHub->getLocale();
    LocaleTokenCreateLanguageString(localeLanguage, localeToken);
    LocaleTokenCreateCountryString(localeCountry, localeToken);
    eastl::string locale(eastl::string::CtorSprintf(), "%s_%s", localeLanguage, localeCountry);
    
    mId20AuthService->setLocale(locale.c_str());
    mId20AuthService->setRedirectUrl(IGNITION_REDIRECT_URI);

#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(EA_PLATFORM_PS5)
    mId20AuthService->setPSEnterButtonO(true);
#endif
}

EadpLoginHelper::~EadpLoginHelper()
{
}

void EadpLoginHelper::onStartLoginProcess(const char8_t* productName, const Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt, const LoginAuthenticationOption authOption)
{
    blaze_strnzcpy(mProductName, productName, sizeof(mProductName));
    mCrossPlatformOpt = crossPlatformOpt;
    mAuthOption = authOption;

#if defined(EA_PLATFORM_WINDOWS)
    if (mIsOriginClientConnected)
    {
        requestAccessToken();
    }
    else
    {
        initOriginService();
    }
#else
    requestAccessToken();
#endif
}

void EadpLoginHelper::initOriginService()
{
#if defined(EA_PLATFORM_WINDOWS)
    auto connectCallback = [this](const eadp::foundation::ErrorPtr& error)
    {
        if (error)
        {
            BLAZE_SDK_DEBUGF("[OriginService::connectCallback] Failed to connect to the Origin client.");
            BLAZE_SDK_DEBUGF("[OriginService::connectCallback] Code: %d ", error->getCode());
            BLAZE_SDK_DEBUGF("[OriginService::connectCallback] Reason: %s ", error->getReason().c_str());
        }
        else
        {
            BLAZE_SDK_DEBUGF("[OriginService::connectCallback] Successfully connected to the Origin client.");
            mIsOriginClientConnected = true;
            requestAccessToken();
        }
    };

    auto disconnectCallback = [this](const eadp::foundation::ErrorPtr& error)
    {
        BLAZE_SDK_DEBUGF("[OriginService::disconnectCallback] Disconnected from the Origin client.");
        if (error)
        {
            BLAZE_SDK_DEBUGF("[OriginService::disconnectCallback] Code: %d ", error->getCode());
            BLAZE_SDK_DEBUGF("[OriginService::disconnectCallback] Reason: %s ", error->getReason().c_str());
        }
    };

    mOriginService->initialize(gEadpHub->getAllocator().make<eadp::foundation::String>(EAORIGIN_TITLE),
        gEadpHub->getAllocator().make<eadp::foundation::String>(EAORIGIN_CONTENT_ID),
        gEadpHub->getAllocator().make<eadp::foundation::String>(EAORIGIN_MULTIPLAYER_ID),
        ea::origin::IOriginService::ConnectCallback(connectCallback, mPersistenceObject, EAORIGIN_CALLBACK_GROUP_ID),
        ea::origin::IOriginService::ClientDisconnectCallback(disconnectCallback, mPersistenceObject, EAORIGIN_CALLBACK_GROUP_ID));
#endif
}

void EadpLoginHelper::requestAccessToken()
{
    if (mAuthOption == JWT_ACCESS_TOKEN)
    {
        gEadpHub->getDirectorService()->setConfigurationValue("eadp.authentication.useJwtToken", "true");
    }
    else
    {
        gEadpHub->getDirectorService()->setConfigurationValue("eadp.authentication.useJwtToken", "false");
    }

    auto request = gEadpHub->getAllocator().makeUnique<eadp::authentication::ID20AuthService::LoginRequest>(gEadpHub->getAllocator());
    request->setUserIndex(USER_INDEX);
    request->setScope("");
    request->setGuestMode(false);
    request->setUserInfo(retrieveUserInfo());

    auto callback = [this](eadp::foundation::UniquePtr<eadp::authentication::ID20AuthService::Response> response, const eadp::foundation::ErrorPtr& error)
    {
        if (response)
        {
            BLAZE_SDK_DEBUGF("[EadpLoginHelper::requestAccessToken] Received access token (may not display full string if token is too long like JWT): %s ", gEadpHub->getIdentityService()->getAccessToken().c_str());
            onAccessTokenReceived(gEadpHub->getIdentityService()->getAccessToken().c_str());
        }
        else if (error)
        {
            BLAZE_SDK_DEBUGF("[EadpLoginHelper::requestAccessToken] Failed with error: %s ", error->toString().c_str());
        }
    };

    mId20AuthService->login(std::move(request), eadp::authentication::ID20AuthService::Callback(callback, mPersistenceObject, AUTH_CALLBACK_GROUP_ID));
}

void EadpLoginHelper::onAccessTokenReceived(const char8_t* accessToken)
{
    if (mAuthOption == AUTH_CODE)
    {
        requestAuthCode();
    }
    else
    {
        mOwner.onEadpLoginByAccessToken(accessToken, mProductName, mCrossPlatformOpt);
    }
}

void EadpLoginHelper::requestAuthCode()
{
    auto request = gEadpHub->getAllocator().makeUnique<eadp::foundation::IIdentityService::GetAuthCodeRequest>(gEadpHub->getAllocator());
    request->setClientId(gBlazeHub->getConnectionManager()->getClientId());     //server client id
    request->setUserIndex(USER_INDEX);
    request->setScope("");
    request->setAuthenticationSource(IGNITION_AUTH_SOURCE);
    request->setRedirectUri(IGNITION_REDIRECT_URI);

    auto callback = [this](eadp::foundation::UniquePtr<eadp::foundation::IIdentityService::GetAuthCodeResponse> response, const eadp::foundation::ErrorPtr& error) {
        if (response)
        {
            BLAZE_SDK_DEBUGF("[EadpLoginHelper::requestAuthCode] Received auth code: %s ", response->getAuthCode().c_str());
            onAuthCodeReceived(response->getAuthCode().c_str());
        }
        else if (error)
        {
            BLAZE_SDK_DEBUGF("[EadpLoginHelper::requestAuthCode] Failed with error: %s ", error->toString().c_str());
        }
    };

    gEadpHub->getIdentityService()->getAuthCode(EADPSDK_MOVE(request), eadp::foundation::IIdentityService::GetAuthCodeCallback(callback, mPersistenceObject, AUTH_CALLBACK_GROUP_ID));
}

void EadpLoginHelper::onAuthCodeReceived(const char8_t* authCode)
{
    mOwner.onEadpLoginByAuthCode(authCode, mProductName, mCrossPlatformOpt);
}

void EadpLoginHelper::onLogout()
{
    if (!gEadpHub->getIdentityService()->isAuthenticated())
        return;

    auto callback = [](eadp::foundation::UniquePtr<eadp::authentication::ID20AuthService::Response> response, const eadp::foundation::ErrorPtr& error)
    {
        if (response)
        {
            BLAZE_SDK_DEBUGF("[EadpLoginHelper::onLogout] Response: %s ", (response->getSuccess() ? "success" : "failure"));
        }
        else if (error)
        {
            BLAZE_SDK_DEBUGF("[EadpLoginHelper::onLogout] Failed with error: %s ", error->toString().c_str());
        }
    };

    mId20AuthService->logout(USER_INDEX, eadp::authentication::ID20AuthService::Callback(callback, mPersistenceObject, AUTH_CALLBACK_GROUP_ID));
}

void EadpLoginHelper::updateAccessToken()
{
    //EADP SDK does not provide API to trigger token refreshment. It refreshes JWT access token automatically.
    //This function does not demonstrate the right way to refresh token, and is for testing server RPC updateAccessToken only.

    auto logoutCallback = [this](eadp::foundation::UniquePtr<eadp::authentication::ID20AuthService::Response> response, const eadp::foundation::ErrorPtr& error)
    {
        auto request = gEadpHub->getAllocator().makeUnique<eadp::authentication::ID20AuthService::LoginRequest>(gEadpHub->getAllocator());
        request->setUserIndex(USER_INDEX);
        request->setScope("");
        request->setGuestMode(false);
        request->setUserInfo(retrieveUserInfo());

        auto loginCallback = [this](eadp::foundation::UniquePtr<eadp::authentication::ID20AuthService::Response> response, const eadp::foundation::ErrorPtr& error)
        {
            if (response)
            {
                BLAZE_SDK_DEBUGF("[EadpLoginHelper::updateAccessToken] Received access token (may not display full string if token is too long like JWT): %s ", gEadpHub->getIdentityService()->getAccessToken().c_str());
                mOwner.onEadpUpdateAccessToken(gEadpHub->getIdentityService()->getAccessToken().c_str());
            }
            else if (error)
            {
                BLAZE_SDK_DEBUGF("[EadpLoginHelper::updateAccessToken] Failed with error: %s ", error->toString().c_str());
            }
        };

        mId20AuthService->login(std::move(request), eadp::authentication::ID20AuthService::Callback(loginCallback, mPersistenceObject, AUTH_CALLBACK_GROUP_ID));
    };

    mId20AuthService->logout(USER_INDEX, eadp::authentication::ID20AuthService::Callback(logoutCallback, mPersistenceObject, AUTH_CALLBACK_GROUP_ID));
}

eadp::foundation::PlatformUserPtr EadpLoginHelper::retrieveUserInfo()
{
    eadp::foundation::PlatformUserPtr userInfo = nullptr;

#if EADPSDK_USE_STD_STL
    EA::Allocator::ICoreAllocator* coreAlloc = EA::Allocator::ICoreAllocator::GetDefaultAllocator();
#else
    EA::Allocator::ICoreAllocator* coreAlloc = gEadpHub->getAllocator().getCoreAllocator();
#endif
    (void)coreAlloc;

    //retrieve user info based on platform
#if defined(EA_PLATFORM_WINDOWS)
    userInfo = nullptr;

#elif defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(EA_PLATFORM_PS5)
    SceUserServiceLoginUserIdList userIdList;
    memset(&userIdList, 0, sizeof(userIdList));
    if (sceUserServiceGetLoginUserIdList(&userIdList) == SCE_OK && userIdList.userId[USER_INDEX] != SCE_USER_SERVICE_USER_ID_INVALID)
    {
        EA::User::SonyUser* sonyUser = gEadpHub->getAllocator().makeRaw<EA::User::SonyUser>(userIdList.userId[USER_INDEX], coreAlloc);
        userInfo = eadp::foundation::PlatformUser::create(gEadpHub->getAllocator(), sonyUser);
    }

#elif defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
    EA::User::IEAUser* primaryUser = gpEacup->GetPrimaryUser();
    userInfo = eadp::foundation::PlatformUser::create(gEadpHub->getAllocator(), primaryUser);

#elif defined(EA_PLATFORM_NX)
    userInfo = eadp::foundation::PlatformUser::createForUserIndex(gEadpHub->getAllocator(), 0);

#elif defined(EA_PLATFORM_STADIA)
    ggp::PlayerId playerId = GgpGetPrimaryPlayerId();
    if (playerId != ggp::IdUint64Constants::kInvalidId)
    {
        userInfo = eadp::foundation::PlatformUser::create(gEadpHub->getAllocator(), playerId);
    }

#endif

    return userInfo;
}

}  //namespace Ignition
