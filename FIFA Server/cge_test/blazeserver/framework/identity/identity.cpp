/*************************************************************************************************/
/*!
    \file
        identity.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "blazerpcerrors.h"
#include "framework/identity/identity.h"
#include "framework/usersessions/usersession.h"
#include "framework/controller/controller.h"
#include "framework/component/componentmanager.h"

namespace Blaze
{

// static
IdentitySlave* IdentitySlave::createImpl()
{
    return BLAZE_NEW_MGID(IdentityManager::COMPONENT_MEMORY_GROUP, "Component") IdentityManager();
}

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/

IdentityManager::IdentityManager() :
    mProviders(BlazeStlAllocator("IdentityManager::mProviders"))
{
    EA_ASSERT(gIdentityManager == nullptr);
    gIdentityManager = this;
}

IdentityManager::~IdentityManager()
{
    gIdentityManager = nullptr;
}

BlazeRpcError IdentityManager::getIdentity(EA::TDF::ObjectType bobjType, EntityId key, IdentityInfo& result)
{
    VERIFY_WORKER_FIBER();

    EntityIdList keys;
    keys.push_back(key);
    IdentityInfoByEntityIdMap identityMap;

    BlazeRpcError err = getIdentities(bobjType, keys, identityMap);
    if (err == ERR_OK)
    {
        IdentityInfoByEntityIdMap::const_iterator iter = identityMap.find(key);
        if (iter == identityMap.end())
        {
            BLAZE_ERR_LOG(Log::USER, "[IdentityManager].getIdentity: "
                "call to getIdentities returned successfully without populating results for entity id: " << key);
            return ERR_SYSTEM;
        }

        iter->second->copyInto(result);
    }
    return err;
}

BlazeRpcError IdentityManager::getIdentities(EA::TDF::ObjectType bobjType, const EntityIdList& keys, IdentityInfoByEntityIdMap& results)
{
    VERIFY_WORKER_FIBER();

    ProvidersByComponent::const_iterator iter = mProviders.find(bobjType.component);
    if (iter != mProviders.end())
    {
        return iter->second->getIdentities(bobjType, keys, results);
    }
    else
    {
        // If we don't have a local provider, check if this component is remote, if so we'll make an RPC to the user set
        // on a remote slave, otherwise we fail
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(getRemoteIdentitySlaveInstanceId(bobjType.component));

        if (!opts.routeTo.isValid())
        {
            return ERR_SYSTEM;
        }

        IdentitiesRequest request;
        IdentitiesResponse response;
        request.setBlazeObjectType(bobjType);
        EntityIds& entityIds = request.getEntityIds();
        for (EntityIdList::const_iterator eidIter = keys.begin(); eidIter != keys.end(); ++eidIter)
            entityIds.push_back(*eidIter);

        BlazeRpcError err = IdentitySlave::getIdentities(request, response, opts);
        if (err != ERR_OK)
        {
            return err;
        }

        response.getIdentityInfoByEntityIdMap().copyInto(results); // TBD can this be a swap instead???
    }
    return ERR_OK;
}

BlazeRpcError IdentityManager::getIdentityByName(EA::TDF::ObjectType bobjType, const EntityName& name, IdentityInfo& result)
{
    VERIFY_WORKER_FIBER();

    ProvidersByComponent::const_iterator iter = mProviders.find(bobjType.component);
    if (iter != mProviders.end())
    {
        return iter->second->getIdentityByName(bobjType, name, result);
    }
    else
    {
        // If we don't have a local provider, check if this component is remote, if so we'll make an RPC to the user set
        // on a remote slave, otherwise we fail
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(getRemoteIdentitySlaveInstanceId(bobjType.component));

        if (!opts.routeTo.isValid())
        {
            return ERR_SYSTEM;
        }

        IdentityByNameRequest request;
        request.setBlazeObjectType(bobjType);
        request.setEntityName(name);

        BlazeRpcError err = IdentitySlave::getIdentityByName(request, result, opts);
        if (err != ERR_OK)
        {
            return err;
        }
    }
    return ERR_OK;
}

BlazeRpcError IdentityManager::getLocalIdentities(EA::TDF::ObjectType bobjType, const EntityIdList& keys, IdentityInfoByEntityIdMap& results)
{
    ProvidersByComponent::const_iterator iter = mProviders.find(bobjType.component);
    if (iter != mProviders.end())
    {
        return iter->second->getIdentities(bobjType, keys, results);
    }

    BLAZE_ERR_LOG(Log::USER, "[IdentityManager].getIdentities: component " << bobjType.component << " not a local provider");

    return ERR_SYSTEM;
}

BlazeRpcError IdentityManager::getLocalIdentityByName(EA::TDF::ObjectType bobjType, const EntityName& name, IdentityInfo& result)
{
    ProvidersByComponent::const_iterator iter = mProviders.find(bobjType.component);
    if (iter != mProviders.end())
    {
        return iter->second->getIdentityByName(bobjType, name, result);
    }

    BLAZE_ERR_LOG(Log::USER, "[IdentityManager].getIdentityByName: component " << bobjType.component << " not a local provider");

    return ERR_SYSTEM;
}

InstanceId IdentityManager::getRemoteIdentitySlaveInstanceId(ComponentId componentId) const
{
    Component* component = gController->getComponent(componentId, false, false, nullptr);
    if (component == nullptr)
    {
        BLAZE_ERR_LOG(Log::USER, "[IdentityManager].getRemoteIdentitySlaveInstanceId: could not locate component:" << componentId);
        return INVALID_INSTANCE_ID;
    }
    if (component->isLocal())
    {
        BLAZE_ERR_LOG(Log::USER, "[IdentityManager].getRemoteIdentitySlaveInstanceId: component: " << componentId << " is local but has not registered as a user set provider");
        return INVALID_INSTANCE_ID;
    }

    return component->selectInstanceId();
}

bool IdentityManager::registerProvider(ComponentId compId, IdentityProvider& provider)
{
    if(mProviders.find(compId) != mProviders.end())
    {
        BLAZE_ERR_LOG(Log::USER, "[IdentityManager].registerProvider: Identity provider already registered with component " << compId);
        return false;
    }

    BLAZE_TRACE_LOG(Log::USER, "[IdentityManager].registerProvider: added Identity provider for component(" << compId << "): " << &provider);

    mProviders[compId] = &provider;
    return true;
}

bool IdentityManager::deregisterProvider(ComponentId compId)
{
    ProvidersByComponent::iterator it = mProviders.find(compId);
    if(it == mProviders.end())
    {
        BLAZE_TRACE_LOG(Log::USER, "[IdentityManager].deregisterProvider: Identity provider not registered with for component " << compId);
        return false;
    }

    BLAZE_TRACE_LOG(Log::USER, "[IdentityManager].registerProvider: removed Identity provider for component(" << compId);

    mProviders.erase(it);
    return true;
}

} //namespace Blaze
