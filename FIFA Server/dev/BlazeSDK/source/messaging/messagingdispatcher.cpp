/*************************************************************************************************/
/*!
    \file messaging.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/messaging/messagingdispatcher.h"

namespace Blaze
{
namespace Messaging
{


void GlobalDispatcher::dispatch(BlazeHub* hub, const ServerMessage* message)
{
    for(CallbackList::const_iterator i = mCallbacks.begin(), e = mCallbacks.end(); i != e; ++i)
    {
        (*i)(message);
    }
}

bool GlobalDispatcher::addCallback(const CallbackType& cb)
{
    for(CallbackList::iterator i = mCallbacks.begin(), e = mCallbacks.end(); i != e; ++i)
    {
        if((*i) == cb)
        {
            return false;
        }
    }
    mCallbacks.push_back(cb);
    return true;
}

bool GlobalDispatcher::removeCallback(const CallbackType& cb)
{
    for(CallbackList::iterator i = mCallbacks.begin(), e = mCallbacks.end(); i != e; ++i)
    {
        if((*i) == cb)
        {
            mCallbacks.erase(i);
            return true;
        }
    }
    return false;
}

void UserDispatcher::dispatch(BlazeHub* hub, const ServerMessage* message)
{
    for (TargetIdList::const_iterator iter = message->getPayload().getTargetIds().begin(),
           iterEnd = message->getPayload().getTargetIds().end(); iter != iterEnd ; ++iter)
    {
        const BlazeId blazeId = *iter;
        const UserManager::User* user = hub->getUserManager()->getUserById(blazeId);
        for(CallbackList::const_iterator i = mCallbacks.begin(), e = mCallbacks.end(); i != e; ++i)
        {
            (*i)(message, user);
        }
    }
}

bool UserDispatcher::addCallback(const CallbackType& cb)
{
    for(CallbackList::iterator i = mCallbacks.begin(), e = mCallbacks.end(); i != e; ++i)
    {
        if((*i) == cb)
        {
            return false;
        }
    }
    mCallbacks.push_back(cb);
    return true;
}

bool UserDispatcher::removeCallback(const CallbackType& cb)
{
    for(CallbackList::iterator i = mCallbacks.begin(), e = mCallbacks.end(); i != e; ++i)
    {
        if((*i) == cb)
        {
            mCallbacks.erase(i);
            return true;
        }
    }
    return false;
}

}
}
