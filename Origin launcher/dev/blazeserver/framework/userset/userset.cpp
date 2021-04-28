/*************************************************************************************************/
/*!
    \file
        userset.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "blazerpcerrors.h"
#include "framework/userset/userset.h"
#include "framework/usersessions/usersession.h"
#include "framework/controller/controller.h"
#include "framework/component/componentmanager.h"

namespace Blaze
{

// static
UserSetSlave* UserSetSlave::createImpl()
{
    return BLAZE_NEW_MGID(UserSetManager::COMPONENT_MEMORY_GROUP, "Component") UserSetManager();
}

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/

bool UserSetProvider::hasSession(const EA::TDF::ObjectId& bobjId, BlazeRpcError& result)
{
    UserSessionIdList ids;
    result = getSessionIds(bobjId, ids);
    return (!ids.empty());
}

bool UserSetProvider::hasUser(const EA::TDF::ObjectId& bobjId, BlazeRpcError& result)
{
    BlazeIdList ids;
    result = getUserBlazeIds(bobjId, ids);
    return (!ids.empty());
}

bool UserSetProvider::containsUser(const EA::TDF::ObjectId& bobjId, const BlazeId& blazeId)
{
    BlazeIdList ids;
    BlazeRpcError result = getUserBlazeIds(bobjId, ids);

    if (result == ERR_OK)
    {
        for (BlazeIdList::const_iterator it = ids.begin(), itend = ids.end(); it != itend; ++it)
        {
            if (*it == blazeId)
            {
                return true;
            }
        }
    }

    return false;
}

void UserSetProvider::addSubscriber(UserSetSubscriber& subscriber)
{
    mUserSetDispatcher.addDispatchee(subscriber);    
}

void UserSetProvider::removeSubscriber(UserSetSubscriber& subscriber)
{   
    mUserSetDispatcher.removeDispatchee(subscriber);
}

void UserSetProvider::dispatchAddUser(EA::TDF::ObjectId compObjectId, BlazeId blazeId, const UserSession* userSession) 
{
    mUserSetDispatcher.dispatch<EA::TDF::ObjectId, BlazeId, const UserSession*>(&UserSetSubscriber::onAddSubscribe, compObjectId, blazeId, userSession);
}

void UserSetProvider::dispatchRemoveUser(EA::TDF::ObjectId compObjectId, BlazeId blazeId, const UserSession* userSession) 
{
    mUserSetDispatcher.dispatch<EA::TDF::ObjectId, BlazeId, const UserSession*>(&UserSetSubscriber::onRemoveSubscribe, compObjectId, blazeId, userSession);
}

UserSetManager::UserSetManager() :
    mProviders(BlazeStlAllocator("UserSetManager::mProviders"))
{
    EA_ASSERT(gUserSetManager == nullptr);
    gUserSetManager = this;
}

UserSetManager::~UserSetManager()
{
    gUserSetManager = nullptr;
}

BlazeRpcError UserSetManager::addSubscriberToProvider(ComponentId compId, UserSetSubscriber& subscriber)
{
    ProvidersByComponent::iterator it = mProviders.find(compId);
    if (it == mProviders.end())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSetManager].addSubscriberToProvider: UserSet provider not registered (locally) for component " << compId);
        return ERR_SYSTEM;
    }
    it->second->addSubscriber(subscriber); // TBD Should this not return a success/failure?
    return ERR_OK;
}

BlazeRpcError UserSetManager::removeSubscriberFromProvider(ComponentId compId, UserSetSubscriber& subscriber)
{
    ProvidersByComponent::iterator it = mProviders.find(compId);
    if (it == mProviders.end())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSetManager].removeSubscriberFromProvider: UserSet provider not registered (locally) for component " << compId);
        return ERR_SYSTEM;
    }
    it->second->removeSubscriber(subscriber); // TBD Should this not return a success/failure?
    return ERR_OK;
}

BlazeRpcError UserSetManager::countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& userSetSize)
{
    VERIFY_WORKER_FIBER();

    userSetSize = 0;

    RpcCallOptions opts;
    opts.routeTo = getProviderRoutingOptions(bobjId);

    UserSetRequest request;
    UserSetUserBlazeIdsResponse response;
    request.setBlazeObjectId(bobjId);

    // Might be more efficient to add a countUsers RPC and invoke that so that we don't need to transfer
    // the entire collection of user ids over here just to count them
    BlazeRpcError err = UserSetSlave::getUserBlazeIds(request, response, opts);
    if (err != ERR_OK)
    {
        return err;
    }
    userSetSize = static_cast<uint32_t>(response.getBlazeIdList().size());

    return ERR_OK;
}

bool UserSetManager::containsUser(const EA::TDF::ObjectId& bobjId, const BlazeId& blazeId)
{
    VERIFY_WORKER_FIBER_BOOL();

    RpcCallOptions opts;
    opts.routeTo = getProviderRoutingOptions(bobjId);

    UserSetRequest request;
    UserSetUserBlazeIdsResponse response;
    request.setBlazeObjectId(bobjId);

    BlazeRpcError err = UserSetSlave::getUserBlazeIds(request, response, opts);
    if (err != ERR_OK)
    {
        return false;
    }

    for (BlazeIdList::const_iterator it = response.getBlazeIdList().begin(), itend = response.getBlazeIdList().end(); it != itend; ++it)
    {
        if (*it == blazeId)
        {
            return true;
        }
    }

    return false;
}

BlazeRpcError UserSetManager::getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids)
{
    VERIFY_WORKER_FIBER();

    RpcCallOptions opts;
    opts.routeTo = getProviderRoutingOptions(bobjId);

    UserSetRequest request;
    UserSetSessionIdsResponse response;
    request.setBlazeObjectId(bobjId);
    response.setUserSessionIdList(ids);

    BlazeRpcError err = UserSetSlave::getSessionIds(request, response, opts);
    if (err != ERR_OK)
    {
        return err;
    }

    return ERR_OK;
}

BlazeRpcError UserSetManager::getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& ids)
{
    VERIFY_WORKER_FIBER();

    RpcCallOptions opts;
    opts.routeTo = getProviderRoutingOptions(bobjId);

    UserSetRequest request;
    UserSetUserIdsResponse response;
    request.setBlazeObjectId(bobjId);
    response.setUserIdList(ids);

    BlazeRpcError err = UserSetSlave::getUserIds(request, response, opts);
    if (err != ERR_OK)
    {
        return err;
    }

    return ERR_OK;
}

BlazeRpcError UserSetManager::getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& ids)
{
    VERIFY_WORKER_FIBER();

    RpcCallOptions opts;
    opts.routeTo = getProviderRoutingOptions(bobjId);

    UserSetRequest request;
    UserSetUserBlazeIdsResponse response;
    request.setBlazeObjectId(bobjId);
    response.setBlazeIdList(ids);

    BlazeRpcError err = UserSetSlave::getUserBlazeIds(request, response, opts);
    if (err != ERR_OK)
    {
        return err;
    }

    return ERR_OK;
}

BlazeRpcError UserSetManager::getLocalSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids)
{
    ProvidersByComponent::const_iterator iter = mProviders.find(bobjId.type.component);
    if (iter != mProviders.end())
    {
        return iter->second->getSessionIds(bobjId, ids);
    }

    BLAZE_ERR_LOG(Log::USER, "[UserSetManager].getLocalSessionIds: component " << bobjId.type.component << " not a local provider");

    return ERR_SYSTEM;
}

BlazeRpcError UserSetManager::getLocalUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& ids)
{
    ProvidersByComponent::const_iterator iter = mProviders.find(bobjId.type.component);
    if (iter != mProviders.end())
    {
        return iter->second->getUserBlazeIds(bobjId, ids);
    }

    BLAZE_ERR_LOG(Log::USER, "[UserSetManager].getLocalUserIds: component " << bobjId.type.component << " not a local provider");

    return ERR_SYSTEM;
}

BlazeRpcError UserSetManager::getLocalUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& ids)
{
    ProvidersByComponent::const_iterator iter = mProviders.find(bobjId.type.component);
    if (iter != mProviders.end())
    {
        return iter->second->getUserIds(bobjId, ids);
    }

    BLAZE_ERR_LOG(Log::USER, "[UserSetManager].getLocalUserIds: component " << bobjId.type.component << " not a local provider");

    return ERR_SYSTEM;
}

BlazeRpcError UserSetManager::countLocalSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count)
{
    ProvidersByComponent::const_iterator iter = mProviders.find(bobjId.type.component);
    if (iter != mProviders.end())
    {
        return iter->second->countSessions(bobjId, count);
    }

    BLAZE_ERR_LOG(Log::USER, "[UserSetManager].countLocalSessionIds: component " << bobjId.type.component << " not a local provider");

    return ERR_SYSTEM;
}

BlazeRpcError UserSetManager::countLocalUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count)
{
    ProvidersByComponent::const_iterator iter = mProviders.find(bobjId.type.component);
    if (iter != mProviders.end())
    {
        return iter->second->countUsers(bobjId, count);
    }

    BLAZE_ERR_LOG(Log::USER, "[UserSetManager].countLocalUserIds: component " << bobjId.type.component << " not a local provider");

    return ERR_SYSTEM;
}

RoutingOptions UserSetManager::getProviderRoutingOptions(const EA::TDF::ObjectId& bobjId, bool remoteRpcCheck)
{
    RoutingOptions routeTo;

    ProvidersByComponent::const_iterator iter = mProviders.find(bobjId.type.component);
    if (iter != mProviders.end())
    {
        UserSetProvider& provider = *iter->second;

        // Determine where the required provider is located.  The key could be sharded as an InstanceKey or a SliverKey.
        // These two must be handle differently.  The requiredCallOpts are configured with the appropriate routing info.
        routeTo = provider.getRoutingOptionsForObjectId(bobjId);
    }
    else
    {
        // If we don't have a local provider, check if this component is remote, if so we'll make an RPC to the user set
        // on a remote slave, otherwise we fail
        Component* component = gController->getComponent(bobjId.type.component, false, false, nullptr);
        if (component == nullptr)
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSetManager].getLocalProviderOrRemoteShardId: could not locate component:" << bobjId.type.component);
        }
        else if (component->isLocal())
        {
            BLAZE_ERR_LOG(Log::USER, "[UserSetManager].getLocalProviderOrRemoteShardId: component: " << bobjId.type.component << " is local but has not registered as a user set provider");
        }
        else
        {
            routeTo.setInstanceId(component->selectInstanceId());
        }

        // Get the ProviderRoutingOptions from someone that actually has the provider:
        if (remoteRpcCheck)
        {
            UserSetRequest request;
            RoutingOptionsResponse response;
            request.setBlazeObjectId(bobjId);

            RpcCallOptions opts;
            opts.routeTo = routeTo;
            UserSetSlave::getProviderRoutingOptions(request, response, opts);

            if (response.getId().getActiveMember() == RoutingOptionsId::MEMBER_INSTANCEID)
                routeTo.setInstanceId(response.getId().getInstanceId());
            else if (response.getId().getActiveMember() == RoutingOptionsId::MEMBER_SLIVERIDENTITY)
                routeTo.setSliverIdentity(response.getId().getSliverIdentity());
        }
    }

    return routeTo;
}

bool UserSetManager::registerProvider(ComponentId compId, UserSetProvider& provider)
{
    if(mProviders.find(compId) != mProviders.end())
    {
        BLAZE_ERR_LOG(Log::USER, "[UserSetManager].registerProvider: UserSet provider already registered with component " << compId);
        return false;
    }

    BLAZE_TRACE_LOG(Log::USER, "[UserSetManager].registerProvider: added UserSet provider for component(" << compId << "): " << &provider);

    mProviders[compId] = &provider;
    return true;
}

bool UserSetManager::deregisterProvider(ComponentId compId)
{
    ProvidersByComponent::iterator it = mProviders.find(compId);
    if(it == mProviders.end())
    {
        BLAZE_TRACE_LOG(Log::USER, "[UserSetManager].deregisterProvider: UserSet provider not registered with for component " << compId);
        return false;
    }

    BLAZE_TRACE_LOG(Log::USER, "[UserSetManager].registerProvider: removed UserSet provider for component(" << compId);

    mProviders.erase(it);
    return true;
}

} //namespace Blaze
