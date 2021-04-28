/**************************************************************************************************/
/*! 
    \file sessiondata.h
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef SESSIONDATA_H
#define SESSIONDATA_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

// Include files
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/dispatcher.h"
#include "BlazeSDK/component/framework/tdf/userdefines.h"
#include "BlazeSDK/component/authentication/tdf/authentication.h"

namespace Blaze
{

class BlazeHub;
namespace LoginManager
{
/*! ***********************************************************************************************/
/*! \struct SessionData
    
    \brief Stores data shared by the login manager and the login state machine for the duration of
           the user session.
***************************************************************************************************/
struct SessionData
{
    SessionData(BlazeHub *hub, uint32_t userIndex, MemoryGroupId memGroupId = Blaze::MEM_GROUP_LOGINMANAGER_TEMP)
    :   mBlazeHub(hub),
        mUserIndex(userIndex),
        mDirtySockUserIndex(userIndex),
        mAuthComponent(nullptr),
        mListenerDispatcher(hub->getScheduler())
    {
    }

    const UserManager::LocalUser *getLocalUser() const { return mBlazeHub->getUserManager()->getLocalUser(mUserIndex); }
    UserManager::LocalUser *getLocalUser() { return mBlazeHub->getUserManager()->getLocalUser(mUserIndex); }

    BlazeHub* mBlazeHub;
    uint32_t mUserIndex; 
    uint32_t mDirtySockUserIndex;
    Authentication::AuthenticationComponent* mAuthComponent;
    DelayedDispatcher<LoginManagerListener>  mListenerDispatcher;
};

} // LoginManager
} // Blaze

#endif // SESSIONDATA_H
