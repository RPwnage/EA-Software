/*************************************************************************************************/
/*!
    \file
        petitionablecontent.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "blazerpcerrors.h"
#include "framework/petitionablecontent/petitionablecontent.h"
#include "framework/controller/controller.h"
#include "framework/component/componentmanager.h"

namespace Blaze
{

// static
PetitionableContentSlave* PetitionableContentSlave::createImpl()
{
    return BLAZE_NEW_MGID(PetitionableContentManager::COMPONENT_MEMORY_GROUP, "Component") PetitionableContentManager();
}

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/

PetitionableContentManager::PetitionableContentManager() : mProviders(BlazeStlAllocator("PetitionableContentManager::mProviders"))
{
    EA_ASSERT(gPetitionableContentManager == nullptr);
    gPetitionableContentManager = this;
}

PetitionableContentManager::~PetitionableContentManager()
{
    gPetitionableContentManager = nullptr;
}

BlazeRpcError PetitionableContentManager::fetchContent(const EA::TDF::ObjectId& bobjId, Collections::AttributeMap& attributeMap, eastl::string& url)
{
    VERIFY_WORKER_FIBER();

    ProvidersByComponent::const_iterator iter = mProviders.find(bobjId.type.component);
    if (iter != mProviders.end())
    {
        return iter->second->fetchContent(bobjId, attributeMap, url);
    }
    else
    {
        // If we don't have a local provider, check if this component is remote, if so we'll make an RPC to the user set
        // on a remote slave, otherwise we fail
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(getRemotePetitionableContentSlaveInstanceId(bobjId.type.component));
        if (!opts.routeTo.isValid())
        {
            BLAZE_ERR_LOG(Log::USER, "[PetitionableContentManager].: cannot locate user set component");
            return ERR_SYSTEM;
        }

        FetchContentRequest request;
        request.setBlazeObjectId(bobjId);
        FetchContentResponse response;

        BlazeRpcError err = PetitionableContentSlave::fetchContent(request, response, opts);
        if (err != ERR_OK)
        {
            return err;
        }

        response.getAttributeMap().copyInto(attributeMap);
        url = response.getUrl();
    }
    return ERR_OK;
}

BlazeRpcError PetitionableContentManager::showContent(const EA::TDF::ObjectId& bobjId, bool visible)
{
    VERIFY_WORKER_FIBER();

    ProvidersByComponent::const_iterator iter = mProviders.find(bobjId.type.component);
    if (iter != mProviders.end())
    {
        return iter->second->showContent(bobjId, visible);
    }
    else
    {
        // If we don't have a local provider, check if this component is remote, if so we'll make an RPC to the user set
        // on a remote slave, otherwise we fail
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(getRemotePetitionableContentSlaveInstanceId(bobjId.type.component));
        if (!opts.routeTo.isValid())
        {
            BLAZE_ERR_LOG(Log::USER, "[PetitionableContentManager].: cannot locate user set component");
            return ERR_SYSTEM;
        }

        ShowContentRequest request;
        request.setBlazeObjectId(bobjId);
        request.setVisible(visible);

        return PetitionableContentSlave::showContent(request, opts);
    }
}

BlazeRpcError PetitionableContentManager::fetchLocalContent(const EA::TDF::ObjectId& bobjId, Collections::AttributeMap& attributeMap, eastl::string& url)
{
    ProvidersByComponent::const_iterator iter = mProviders.find(bobjId.type.component);
    if (iter != mProviders.end())
    {
        return iter->second->fetchContent(bobjId, attributeMap, url);
    }

    BLAZE_ERR_LOG(Log::USER, "[PetitionableContentManager].fetchContent: component " << bobjId.type.component << " not a local provider");

    return ERR_SYSTEM;
}

BlazeRpcError PetitionableContentManager::showLocalContent(const EA::TDF::ObjectId& bobjId, bool visible)
{
    ProvidersByComponent::const_iterator iter = mProviders.find(bobjId.type.component);
    if (iter != mProviders.end())
    {
        return iter->second->showContent(bobjId, visible);
    }

    BLAZE_ERR_LOG(Log::USER, "[PetitionableContentManager].showContent: component " << bobjId.type.component << " not a local provider");

    return ERR_SYSTEM;
}

// It may be required at some point to support Sliver based sharding...
// Right now, the Clubs component is the only one that is a PetitionableContentProvider, and
// it's implementation does not require sliver support.  So, for now, I'm leaving this as is.
InstanceId PetitionableContentManager::getRemotePetitionableContentSlaveInstanceId(ComponentId componentId) const
{
    Component* component = gController->getComponent(componentId, false, false, nullptr);
    if (component == nullptr)
    {
        BLAZE_ERR_LOG(Log::USER, "[PetitionableContentManager].getRemotePetitionableContentSlaveInstanceId: could not locate component:" << componentId);
        return INVALID_INSTANCE_ID;
    }
    if (component->isLocal())
    {
        BLAZE_ERR_LOG(Log::USER, "[PetitionableContentManager].getRemotePetitionableContentSlaveInstanceId: component: " << componentId << " is local but has not registered as a user set provider");
        return INVALID_INSTANCE_ID;
    }

    return component->selectInstanceId();
}

bool PetitionableContentManager::registerProvider(ComponentId compId, PetitionableContentProvider& provider)
{
    if(mProviders.find(compId) != mProviders.end())
    {
        BLAZE_ERR_LOG(Log::USER, "[PetitionableContentManager].registerProvider: PetitionableContent provider already registered with component " << compId);
        return false;
    }

    BLAZE_TRACE_LOG(Log::USER, "[PetitionableContentManager].registerProvider: added PetitionableContent provider for component(" << compId << "): " << &provider);

    mProviders[compId] = &provider;
    return true;
}

bool PetitionableContentManager::deregisterProvider(ComponentId compId)
{
    ProvidersByComponent::iterator it = mProviders.find(compId);
    if(it == mProviders.end())
    {
        BLAZE_TRACE_LOG(Log::USER, "[PetitionableContentManager].deregisterProvider: PetitionableContent provider not registered with for component " << compId);
        return false;
    }

    BLAZE_TRACE_LOG(Log::USER, "[PetitionableContentManager].registerProvider: removed PetitionableContent provider for component(" << compId);

    mProviders.erase(it);
    return true;
}

} //namespace Blaze
