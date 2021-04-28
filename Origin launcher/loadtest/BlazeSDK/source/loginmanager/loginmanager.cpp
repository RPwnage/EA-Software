/**************************************************************************************************/
/*! 
    \file loginmanager.cpp
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

// Include files
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/internal/loginmanager/loginmanagerimpl.h"
#include "BlazeSDK/internal/loginmanager/loginstatemachine.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/component/authenticationcomponent.h"

#include "DirtySDK/dirtysock/netconn.h"

namespace Blaze
{
namespace LoginManager
{

using namespace Authentication;

LoginManager* LoginManager::create(BlazeHub* blazeHub, uint32_t userIndex)
{
    return BLAZE_NEW(MEM_GROUP_LOGINMANAGER, "LoginManagerImpl")
        LoginManagerImpl(blazeHub, userIndex, MEM_GROUP_LOGINMANAGER);
}

static uint32_t defaultLegalDocNormalizerHook(char8_t* dest, const char8_t* source, uint32_t size)
{
    memcpy(dest, source, size);
    dest[size] = '\0';
    return size;
}

void LoginManager::setLegalDocNormalizerHook(LegalDocNormalizerFunction hook)
{
    mLegalDocNormalizerFunction = hook != nullptr ? hook : defaultLegalDocNormalizerHook;
}

LoginManager::LoginManager()
:   isGetAccountInfoCalled(false),
    mLegalDocNormalizerFunction(defaultLegalDocNormalizerHook)
{
}

void LoginManagerImpl::setIgnoreLegalDocumentUpdate(bool ignoreLegalDocumentUpdate)
{        
    mStateMachine->setIgnoreLegalDocumentUpdate(ignoreLegalDocumentUpdate);
}

#if !defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_PS5) && !defined(EA_PLATFORM_XBSX)
void LoginManagerImpl::setUseExternalLoginFlow(bool useExternalLoginFlow)
{    
    mStateMachine->setUseExternalLoginFlow(useExternalLoginFlow);
}
#endif

bool LoginManagerImpl::getSkipLegalDocumentDownload() const
{
    return mSkipLegalDocumentDownload;
}

void LoginManagerImpl::setSkipLegalDocumentDownload(bool skipLegalDocumentDownload)
{
    mSkipLegalDocumentDownload = skipLegalDocumentDownload;
    if (mData != nullptr)
        mData->setSkipLegalDocumentDownload(mSkipLegalDocumentDownload);
}

LoginManagerImpl::LoginManagerImpl(BlazeHub* blazeHub, uint32_t userIndex, MemoryGroupId memGroupId)
  : mData(nullptr),
    mSession(blazeHub, userIndex, memGroupId),
    mSkipLegalDocumentDownload(false)
{
    getLoginData();

    if (mSession.mBlazeHub)
    {
        // listen to blaze hub connected events.
        mSession.mBlazeHub->addUserStateAPIEventHandler(this);

        ComponentManager* componentManager = mSession.mBlazeHub->getComponentManager(static_cast<uint32_t>(mSession.mUserIndex));
        if (componentManager != nullptr)
        {
            mSession.mAuthComponent = componentManager->getAuthenticationComponent();
        }
    }

    mStateMachine = LoginStateMachine::create(*this, mSession);

    BlazeAssert(mSession.mBlazeHub != nullptr);
    BlazeAssert(mSession.mAuthComponent != nullptr);
}

LoginManagerImpl::~LoginManagerImpl()
{
    if (mSession.mBlazeHub)
    {
        mSession.mBlazeHub->removeUserStateAPIEventHandler(this);
    }
    BLAZE_DELETE(MEM_GROUP_LOGINMANAGER, mStateMachine);
    freeLoginData();
}

/*! ***********************************************************************************************/
/*! \name Listener registration methods
***************************************************************************************************/

void LoginManagerImpl::addListener(LoginManagerListener* listener)
{
    mSession.mListenerDispatcher.addDispatchee(listener);
}

void LoginManagerImpl::removeListener(LoginManagerListener* listener)
{
    mSession.mListenerDispatcher.removeDispatchee(listener);
}

/*! ***********************************************************************************************/
/*! \name Primary control methods
***************************************************************************************************/

void LoginManagerImpl::startLoginProcess(const char8_t* productName/* = ""*/, Authentication::CrossPlatformOptSetting crossPlatformOpt /* = DEFAULT*/) const
{
    mStateMachine->getState()->onStartLoginProcess(productName, crossPlatformOpt);
}

void LoginManagerImpl::startGuestLoginProcess(LoginManager::GuestLoginCb cb) const
{
    mStateMachine->getState()->onStartGuestLoginProcess(cb);
}

void LoginManagerImpl::startOriginLoginProcess(const char8_t *code, const char8_t* productName/* = ""*/, Authentication::CrossPlatformOptSetting crossPlatformOpt /* = DEFAULT*/) const
{
    mStateMachine->getState()->onStartOriginLoginProcess(code, productName, crossPlatformOpt);
}

void LoginManagerImpl::startAuthCodeLoginProcess(const char8_t *code, const char8_t* productName/* = ""*/, Authentication::CrossPlatformOptSetting crossPlatformOpt /* = DEFAULT*/) const
{
    mStateMachine->getState()->onStartAuthCodeLoginProcess(code, productName, crossPlatformOpt);
}

void LoginManagerImpl::startTrustedLoginProcess(const char8_t* accessToken, const char8_t* id, const char8_t* idType) const
{
    mStateMachine->getState()->onStartTrustedLoginProcess(accessToken, id, idType);
}

void LoginManagerImpl::startTrustedLoginProcess(const char8_t* certData, const char8_t* keyData, const char8_t* id, const char8_t* idType) const
{
    mStateMachine->getState()->onStartTrustedLoginProcess(certData, keyData, id, idType);
}

void LoginManagerImpl::logout(Functor1<Blaze::BlazeError> cb) const
{
    mStateMachine->getState()->onLogout(cb);
}

void LoginManagerImpl::cancelLogin(Functor1<Blaze::BlazeError> cb) const
{
    mStateMachine->getState()->onCancelLogin(cb);
}

/*! ***********************************************************************************************/
/*! \name Account creation methods
***************************************************************************************************/
JobId LoginManagerImpl::getTermsOfService(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb) const
{
    return mStateMachine->getState()->onGetTermsOfService(cb);    
}

JobId LoginManagerImpl::getPrivacyPolicy(Functor3<Blaze::BlazeError, const char8_t*, uint32_t> cb) const
{
    return mStateMachine->getState()->onGetPrivacyPolicy(cb);
}

void LoginManagerImpl::freeTermsOfServiceBuffer() const
{
    mStateMachine->getState()->onFreeTermsOfServiceBuffer();    
}

void LoginManagerImpl::freePrivacyPolicyBuffer() const
{
    mStateMachine->getState()->onFreePrivacyPolicyBuffer();
}

/*! ***********************************************************************************************/
/*! \name Update util user options
***************************************************************************************************/
JobId LoginManagerImpl::updateUserOptions(const Util::UserOptions& resquest, Functor1<Blaze::BlazeError> cb) const
{
    return mStateMachine->getState()->onUpdateUserOptions(resquest, cb);
}

/*! ***********************************************************************************************/
/*! \name Account info methods
***************************************************************************************************/
void LoginManagerImpl::getAccountInfoInternal(const GetAccountInfoCb& resultCb) const
{
    mStateMachine->getState()->onGetAccountInfo(resultCb);
}

/*! ***********************************************************************************************/
/*! \name Console platform-specific methods
***************************************************************************************************/
uint32_t LoginManagerImpl::getConsoleUserIndex() const
{
    return getDirtySockUserIndex();
}

BlazeError LoginManagerImpl::setConsoleUserIndex(uint32_t consoleUserIndex)
{
    return setDirtySockUserIndex(consoleUserIndex);
}

uint32_t LoginManagerImpl::getDirtySockUserIndex() const
{
    return mSession.mDirtySockUserIndex;
}

BlazeError LoginManagerImpl::setDirtySockUserIndex(uint32_t dirtySockUserIndex)
{
    if (dirtySockUserIndex >= NETCONN_MAXLOCALUSERS)
    {
        BLAZE_SDK_DEBUGF("[LoginManagerImpl] Cannot set user index (%d) to dirty sock index (%d). It'll be out of bound. \n", mSession.mUserIndex, dirtySockUserIndex);
        return SDK_ERR_INVALID_USER_INDEX;
    }

    // Check that this user index isn't already being used by another user:
    for (uint32_t userIndex = 0; userIndex < mSession.mBlazeHub->getNumUsers(); ++userIndex)
    {
        if (mSession.mBlazeHub->getUserManager()->getLocalUser(userIndex) != nullptr && 
            mSession.mBlazeHub->getLoginManager(userIndex)->getDirtySockUserIndex() == dirtySockUserIndex)
        {
            // This indicates that there is already another logged in user on this console index. 
            BLAZE_SDK_DEBUGF("[LoginManagerImpl] Cannot set user index (%d) to dirty sock index (%d). Already in use by user (%d). \n", mSession.mUserIndex, dirtySockUserIndex, userIndex);
            return SDK_ERR_INVALID_STATE;
        }
    }

    mSession.mDirtySockUserIndex = dirtySockUserIndex;
    return ERR_OK;
}

/*! ***********************************************************************************************/
/*! \name Login data methods
***************************************************************************************************/

LoginData* LoginManagerImpl::getLoginData()
{
    if (mData == nullptr)
    {
        mData = BLAZE_NEW(MEM_GROUP_LOGINMANAGER, "LoginData") LoginData(MEM_GROUP_LOGINMANAGER);

        mData->setSkipLegalDocumentDownload(mSkipLegalDocumentDownload);
    }
    return mData;
}

const LoginData* LoginManagerImpl::getLoginData() const
{
    return mData;
}

void LoginManagerImpl::freeLoginData()
{
    if (mData != nullptr)
    {
        BLAZE_DELETE(MEM_GROUP_LOGINMANAGER, mData);
    }
    mData = nullptr;
}

/*! ***********************************************************************************************/
/*! \name BlazeStateEventHandler interface
***************************************************************************************************/
void LoginManagerImpl::onDisconnected(BlazeError errorCode)
{
    //reset state machine
    mStateMachine->getState()->clearLoginInfo();
    mStateMachine->changeState(LOGIN_STATE_INIT);
}

void LoginManagerImpl::onAuthenticated(uint32_t userIndex)
{
    if (mSession.mUserIndex == userIndex)
    {
        //we are authenticated, so update the state machine
        mStateMachine->changeState(LOGIN_STATE_AUTHENTICATED);
    }
}

void LoginManagerImpl::onDeAuthenticated(uint32_t userIndex)
{
    // logged out, so update the state machine
    if (mSession.mUserIndex == userIndex)
    {
        mStateMachine->changeState(LOGIN_STATE_INIT);
    }
}

} // LoginManager
} // Blaze
