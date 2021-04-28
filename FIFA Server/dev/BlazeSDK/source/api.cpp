/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
**************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/blazehub.h"


namespace Blaze
{
    API::API(BlazeHub &blazeHub) :
        mBlazeHub(&blazeHub), mIsActive(true)
    {
        if( mBlazeHub )
        {
            mBlazeHub->addUserStateAPIEventHandler(this);    
        }
    }

    API::~API()
    {
        if( mBlazeHub != nullptr )
        {
            mBlazeHub->removeUserStateAPIEventHandler(this);
        }
    }

    SingletonAPI::SingletonAPI(BlazeHub &blazeHub)
        : API(blazeHub)
    {
    }

    MultiAPI::MultiAPI(BlazeHub &blazeHub, uint32_t userIndex)
        : API(blazeHub),
        mUserIndex(userIndex)
    {
    }



}

