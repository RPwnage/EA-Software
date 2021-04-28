/*************************************************************************************************/
/*!
    \file messaging.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_MESSAGINGDISPATCHER_H
#define BLAZE_MESSAGINGDISPATCHER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blaze_eastl/fixed_vector.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/component/messaging/tdf/messagingtypes.h"

namespace Blaze
{

class API;

namespace UserManager
{
    class User;
}

namespace Messaging
{
    class BLAZESDK_API MessageDispatcher
    {
    public:
        virtual ~MessageDispatcher() {}
        virtual void dispatch(BlazeHub* hub, const ServerMessage* message) = 0;
    };
    
    class BLAZESDK_API GlobalDispatcher : public MessageDispatcher
    {
    public:
        typedef Functor1<const ServerMessage*> CallbackType;
        
        explicit GlobalDispatcher() : mCallbacks(MEM_GROUP_MESSAGING,"GlobalCbList"){}
        void dispatch(BlazeHub* hub, const ServerMessage* message);
        bool addCallback(const CallbackType& cb);
        bool removeCallback(const CallbackType& cb);
        
    private:
        typedef fixed_vector<CallbackType, 8, true> CallbackList;
        CallbackList mCallbacks;
    };
    
    class BLAZESDK_API UserDispatcher : public MessageDispatcher
    {
    public:
        typedef Functor2<const ServerMessage*, const UserManager::User*> CallbackType;
        
        explicit UserDispatcher() : mCallbacks(MEM_GROUP_MESSAGING,"UserCbList"){}
        void dispatch(BlazeHub* hub, const ServerMessage* message);
        bool addCallback(const CallbackType& cb);
        bool removeCallback(const CallbackType& cb);
        
    private:
        typedef fixed_vector<CallbackType, 8, true> CallbackList;
        CallbackList mCallbacks;
    };
    
    template <class GroupType>
    class GroupDispatcher : public MessageDispatcher
    {
    public:
        typedef Functor2<const ServerMessage*, GroupType*> CallbackType;
        
        explicit GroupDispatcher() : mCallbacks(MEM_GROUP_MESSAGING,"GroupCbList"){}
        void dispatch(BlazeHub* hub, const ServerMessage* message);
        bool addCallback(const CallbackType& cb);
        bool removeCallback(const CallbackType& cb);
        
    private:
        typedef fixed_vector<CallbackType, 8, true> CallbackList;
        CallbackList mCallbacks;
    };
    
    template <class GroupType>
    inline void GroupDispatcher<GroupType>::dispatch(BlazeHub* hub, const ServerMessage* message)
    {
        GroupType* group = nullptr;

        // Since we can send a message to multiple groups, here we need to get the correct 
        // group for the each user.
        for (TargetIdList::const_iterator iter = message->getPayload().getTargetIds().begin(),
            iterEnd = message->getPayload().getTargetIds().end(); iter != iterEnd ; ++iter)
        {
            EA::TDF::ObjectId target = EA::TDF::ObjectId(message->getPayload().getTargetType(), *iter);
            group = static_cast<GroupType*>(hub->getUserGroupById(target));

            if (group != nullptr)
                break;
        }

        for(typename CallbackList::const_iterator i = mCallbacks.begin(), e = mCallbacks.end(); i != e; ++i)
        {
            (*i)(message, group);
        }
    }
    
    template <class GroupType>
    inline bool GroupDispatcher<GroupType>::addCallback(const CallbackType& cb)
    {
        for(typename CallbackList::iterator i = mCallbacks.begin(), e = mCallbacks.end(); i != e; ++i)
        {
            if((*i) == cb)
            {
                return false;
            }
        }
        mCallbacks.push_back(cb);
        return true;
    }
    
    template <class GroupType>
    inline bool GroupDispatcher<GroupType>::removeCallback(const CallbackType& cb)
    {
        for(typename CallbackList::iterator i = mCallbacks.begin(), e = mCallbacks.end(); i != e; ++i)
        {
            if((*i) == cb)
            {
                mCallbacks.erase(i);
                return true;
            }
        }
        return false;
    }

} // namespace Messaging
} // namespace Blaze
#endif // BLAZE_MESSAGINGDISPATCHER_H
