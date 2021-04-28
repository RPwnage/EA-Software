/*************************************************************************************************/
/*!
    \file messagingapi.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/messaging/messaging.h"
#include "BlazeSDK/messaging/messagingapi.h"
#include "BlazeSDK/util/utilapi.h"
#include "BlazeSDK/component/util/tdf/utiltypes.h"

namespace Blaze
{
namespace Messaging
{

void MessagingAPI::createAPI(BlazeHub &hub, EA::Allocator::ICoreAllocator* allocator)
{
    if (hub.getMessagingAPI(0) != nullptr) 
    {
        BLAZE_SDK_DEBUGF("[MessagingAPI] Warning: Ignoring attempt to recreate API\n");
        return;
    }
    MessagingComponent::createComponent(&hub);
    if (Blaze::Allocator::getAllocator(MEM_GROUP_MESSAGING) == nullptr)
        Blaze::Allocator::setAllocator(MEM_GROUP_MESSAGING, allocator!=nullptr? allocator : Blaze::Allocator::getAllocator());
    Blaze::Util::UtilAPI::createAPI(hub); 

    APIPtrVector *apiVector = BLAZE_NEW(MEM_GROUP_MESSAGING, "MessagingAPIArray")
        APIPtrVector(MEM_GROUP_MESSAGING, hub.getNumUsers(), MEM_NAME(MEM_GROUP_MESSAGING, "MessagingAPIArray")); 
    // create a separate instance for each userIndex
    for(uint32_t i=0; i < hub.getNumUsers(); i++)
    {
        MessagingAPI *api = BLAZE_NEW(MEM_GROUP_MESSAGING, "MessagingAPI") MessagingAPI(hub, i, MEM_GROUP_MESSAGING);
        (*apiVector)[i] = api; // Note: the Blaze framework will delete the API(s)
    }

    hub.createAPI(MESSAGING_API, apiVector);
}

MessagingAPI::MessagingAPI(BlazeHub &blazeHub, uint32_t userIndex, MemoryGroupId memGroupId)
    : MultiAPI(blazeHub, userIndex),
      mComponent(blazeHub.getComponentManager(userIndex)->getMessagingComponent()),
      mDispatcherByTypeMap(memGroupId, MEM_NAME(memGroupId, "MessagingAPI::mDispatcherByTypeMap")),
      mDispatcherByComponentMap(memGroupId, MEM_NAME(memGroupId, "MessagingAPI::mDispatcherByComponentMap")),
      mMemGroup(memGroupId)
{   
    mComponent->setNotifyMessageHandler(MessagingComponent::NotifyMessageCb(this, &MessagingAPI::onMessageNotification));
}


MessagingAPI::~MessagingAPI()
{
    mComponent->clearNotifyMessageHandler();

    // Remove any outstanding txns.  Canceling here can be dangerous here as it will still attempt
    // to call the callback.  Since the object is being deleted, we go with the remove.
    getBlazeHub()->getScheduler()->removeByAssociatedObject(this);
    // delete per-ObjectType dispatchers
    for(DispatcherByTypeMap::iterator i = mDispatcherByTypeMap.begin(), e = mDispatcherByTypeMap.end(); i != e; ++i)
    {
        BLAZE_DELETE(mMemGroup, i->second);
    }

    for(DispatcherByComponentMap::iterator i = mDispatcherByComponentMap.begin(), e = mDispatcherByComponentMap.end(); i != e; ++i)
    {
        BLAZE_DELETE(mMemGroup, i->second);
    }
}

JobId MessagingAPI::sendMessage(const SendMessageParameters& request, const SendMessageCb& cb)
{
    if(request.getFlags().getFilterProfanity())
    {        
        FilteredMessageSender *sender = BLAZE_NEW(mMemGroup, "FilteredMessageSender") FilteredMessageSender(this, mMemGroup);
        return sender->filterAndSend(request, cb);
    }
    else
    {
        //do normal unfiltered send
        return mComponent->sendMessage(request, cb);
    }
} 

JobId MessagingAPI::fetchMessages(const FetchMessageParameters& request, const FetchMessagesCb& cb)
{
    return mComponent->fetchMessages(request, cb);
}

JobId MessagingAPI::purgeMessages(const PurgeMessageParameters& request, const PurgeMessagesCb& cb)
{
    return mComponent->purgeMessages(request, cb);
}

JobId MessagingAPI::touchMessages(const TouchMessageParameters& request, const TouchMessagesCb& cb)
{
    return mComponent->touchMessages(request, cb);
}

// ================== notifications ======================
void MessagingAPI::onMessageNotification(const ServerMessage *message, uint32_t userIndex)
{
    BLAZE_SDK_SCOPE_TIMER("MessagingAPI::onMessageNotification");

    // convert TDF structure into a bit-shifted primitive value that represents a blaze object type
    const EA::TDF::ObjectType typeValue = message->getPayload().getTargetType();
   
    // if type-specific dispatcher installed, use it
    DispatcherByTypeMap::iterator it = mDispatcherByTypeMap.find(typeValue);
    if(it != mDispatcherByTypeMap.end())
    {
        it->second->dispatch(getBlazeHub(), message);
    }

    //If there's a component wide handler, send it a dispatch as well.
    DispatcherByComponentMap::iterator itByComponent = mDispatcherByComponentMap.find(typeValue.component);
    if(itByComponent != mDispatcherByComponentMap.end())
    {
        itByComponent->second->dispatch(getBlazeHub(), message);
    }

    // dispatch global callbacks, if any
    mGlobalDispatcher.dispatch(getBlazeHub(), message);
}

void MessagingAPI::logStartupParameters() const
{
    //  will think of something
}

bool MessagingAPI::registerCallback(const GlobalMessageCb& cb)
{
    return mGlobalDispatcher.addCallback(cb);
}

bool MessagingAPI::registerCallback(const UserMessageCb& cb)
{
    UserDispatcher* dispatcher = nullptr;
    // user messages are currently assumed to belong to the messaging component, this may change in the future
    const EA::TDF::ObjectType typeValue = ENTITY_TYPE_USER;
    // lookup dispatcher registered for this blaze object type (User)
    DispatcherByTypeMap::iterator it = mDispatcherByTypeMap.find(typeValue);
    if(it == mDispatcherByTypeMap.end())
    {
        dispatcher = BLAZE_NEW(mMemGroup, "UserDispatcher") UserDispatcher;
        mDispatcherByTypeMap.insert(eastl::make_pair(typeValue, dispatcher));
    }
    else
    {
        dispatcher = static_cast<UserDispatcher*>(it->second);
    }
    return dispatcher->addCallback(cb);
}

bool MessagingAPI::removeCallback(const GlobalMessageCb& cb)
{
    return mGlobalDispatcher.removeCallback(cb);
}

bool MessagingAPI::removeCallback(const UserMessageCb& cb)
{
    // user messages are currently assumed to belong to the messaging component, this may change in the future
    const EA::TDF::ObjectType typeValue = ENTITY_TYPE_USER;
    // lookup dispatcher registered for this blaze object type (User)
    DispatcherByTypeMap::iterator it = mDispatcherByTypeMap.find(typeValue);
    if(it != mDispatcherByTypeMap.end())
    {
        return static_cast<UserDispatcher*>(it->second)->removeCallback(cb);
    }
    return false;
}

MessagingAPI::FilteredMessageSender::FilteredMessageSender(MessagingAPI* api, MemoryGroupId memGroupId)
: mApi(api), mMessage(memGroupId), mMemGroup(memGroupId)
{
};

JobId MessagingAPI::FilteredMessageSender::filterAndSend(const SendMessageParameters& request, const MessagingAPI::SendMessageCb& cb)
{
    mCb = cb;
    //store a copy of the message
    request.copyInto(mMessage);

    Util::UserStringList textlist;
    Messaging::MessageAttrMap::const_iterator i = request.getAttrMap().begin(); // first filterable attribute
    Messaging::MessageAttrMap::const_iterator e = request.getAttrMap().upper_bound(PARAM_ATTR_MIN - 1); // last filterable attribute
    // iterate the attributes that need to be checked for profanity (includes custom attributes and blaze attributes)
    for(; i != e; ++i)
    {
        // check attribute for profanity
        Blaze::Util::UserText *t = textlist.getTextList().allocate_element();
        t->setText(i->second.c_str());
        textlist.getTextList().push_back(t);
    }
    // NOTE: Messaging does its own profanity filtering on the server side;
    // therefore, only client side filtering is performed when sending a message
    return mApi->getBlazeHub()->getUtilAPI()->filterUserText(textlist,
        Util::UtilAPI::FilterUserTextCb(this, &FilteredMessageSender::sendFilteredMessage), Util::FILTER_CLIENT_ONLY
    );
}

void MessagingAPI::FilteredMessageSender::sendFilteredMessage(BlazeError aError, JobId jobId, const Util::FilterUserTextResponse* response)
{
    if (aError != ERR_OK)
    {
        mCb(nullptr, aError, jobId);
    }
    else
    {
        // NOTE: response may be nullptr if there were no attributes to be filtered
        if(response != nullptr)
        {
            Util::FilterUserTextResponse::FilteredUserTextList::const_iterator si = response->getFilteredTextList().begin();
            Util::FilterUserTextResponse::FilteredUserTextList::const_iterator se = response->getFilteredTextList().end();

            Messaging::MessageAttrMap::iterator i = mMessage.getAttrMap().begin(); // first filterable attribute
            Messaging::MessageAttrMap::iterator e = mMessage.getAttrMap().upper_bound(PARAM_ATTR_MIN - 1); // last filterable attribute
            // iterate the attributes that need to be checked for profanity (includes custom attributes and blaze attributes)
            for(; i != e && si != se; ++i, ++si)
            {
                // check attribute for profanity
                if ((*si)->getResult() != Util::FILTER_RESULT_PASSED)
                {
                    i->second.set((*si)->getFilteredText());
                }
            }
        }
        //use the same job id to send the message
        mApi->getComponent()->sendMessage(mMessage, mCb, jobId);
    }
    BLAZE_DELETE(mMemGroup, this);
} /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

} // Messaging
} // Blaze
