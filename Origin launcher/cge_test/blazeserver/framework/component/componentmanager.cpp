/*************************************************************************************************/
/*!
    \file 
  

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/component/blazerpc.h"
#include "blazerpcerrors.h"
#include "framework/component/componentmanager.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/connection/outboundconnectionmanager.h"

namespace Blaze
{

void ComponentManager::initialize(Controller& controller)
{
    mComponentMap[controller.getId()] = &controller;
    mLocalComponentMap[controller.getId()] = &controller;
    mComponentIdList.push_back(controller.getId());
}

void ComponentManager::shutdown()
{
    // Avoid deleting the controller component
    mComponentMap.erase(gController->getId());

    // Intentionally iterate in reverse order of construction to ensure that framework components are destroyed last
    for (ComponentIdList::reverse_iterator itr = mComponentIdList.rbegin(), end = mComponentIdList.rend(); itr != end; ++itr)
    {
        ComponentMap::iterator mitr = mComponentMap.find(*itr);
        if (mitr != mComponentMap.end())
        {
            delete mitr->second;
        }
    }

    mComponentMap.clear();
    mLocalComponentMap.clear();
    mComponentIdList.clear();
}

ComponentStub* ComponentManager::addLocalComponent(ComponentId id)
{
    const ComponentData* info = ComponentData::getComponentData(id);
    if (info == nullptr)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[ComponentManager].addLocalComponent: unknown component in configuration (" << id << "); aborting.");
        return nullptr;
    }

    if (mComponentMap.find(id) != mComponentMap.end())
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[ComponentManager].addLocalComponent: duplicate local component in configuration (" << info->name << "); aborting.");
        return nullptr;
    }

    // Ok lets make it
    Component* component = info->createLocalComponent();

    BLAZE_SPAM_LOG(Log::SYSTEM, "[ComponentManager].addLocalComponent: "
        << " component(" << info->name << ") " 
        << " result: " << (component != nullptr ? "success" : "fail"));

    if (component == nullptr)
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[ComponentManager].addLocalComponent: a non-local (proxy) component found in configuration (" << info->name << "); aborting.");
        return nullptr;
    }

    component->addInstance(gController->getLocalInstanceId());
    mLocalComponentMap[id] = component->asStub();
    mComponentMap[id] = component;
    mComponentIdList.push_back(id);

    return component->asStub();
}

/*! ***************************************************************************/
/*! \brief  Registers a remote component placeholder so components with a non-required master 
            can activate before a remote master instance does so
*******************************************************************************/
BlazeRpcError ComponentManager::addRemoteComponentPlaceholder(ComponentId compId, Component*& componentPlaceholder)
{
    componentPlaceholder = nullptr;
    const ComponentData* info = ComponentData::getComponentData(compId);
    if (info == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[ComponentManager].addRemoteComponentPlaceholder: unknown remote component id (" << compId << "); cannot create.");
        return ERR_SYSTEM;
    }

    BlazeRpcError rc = addRemoteComponent(compId, INVALID_INSTANCE_ID);
    if(rc == ERR_OK)
    {
       componentPlaceholder = mComponentMap[compId];
    }

    return rc;
}

/*! ***************************************************************************/
/*! \brief  Registering the same component twice is a NOOP that returns true, 
            whereas registering a different component when one is already 
            registered is a NOOP that returns false.
*******************************************************************************/
BlazeRpcError ComponentManager::addRemoteComponent(ComponentId compId, InstanceId instanceId)
{   
    const ComponentData* info = ComponentData::getComponentData(compId);
    if (info == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[ComponentManager].addRemoteComponent: unknown remote component id (" << compId << "); cannot create.");
        return ERR_SYSTEM;
    }

    // Do we have a component for this?  If not, create one.
    Component* component = nullptr;
    ComponentMap::iterator itr = mComponentMap.find(compId);
    if (itr != mComponentMap.end())
    {
        component = itr->second;
    }
    else
    {
        //Its a new remote component, create it and put it in the map        
        component = info->createRemoteComponent(gController->getOutboundConnectionManager());
        mComponentMap[compId] = component;
        mComponentIdList.push_back(compId);
    }

    BlazeRpcError rc = ERR_OK;

    if(instanceId != INVALID_INSTANCE_ID)
    {
        // we pre-register the masters of components that can run without their master, but don't add INVALID_INSTANCE_ID to the instance map
        rc = component->addInstance(instanceId);
    }

    BLAZE_SPAM_LOG(Log::SYSTEM, "[ComponentManager].registerComponent: "
        << " component(" << BlazeRpcComponentDb::getComponentNameById(compId) << ") by instanceId: " 
        << instanceId << " result: " << rc);
        
    return rc;
}

/*! ***************************************************************************/
/*! \brief  Deregistering an unregistered component is a NOOP that returns false.
*******************************************************************************/
bool ComponentManager::removeRemoteComponent(ComponentId compId, InstanceId instanceId)
{
    bool result = false;
        
    //Try to deregister

    ComponentMap::iterator mitr = mComponentMap.find(compId);
    if (mitr != mComponentMap.end())
    {
        result = mitr->second->removeInstance(instanceId);
    }

    BLAZE_SPAM_LOG(Log::SYSTEM, "[ComponentManager].deregisterComponent: "        
        << " component(" << BlazeRpcComponentDb::getComponentNameById(compId) << ") by instanceId: " 
        << instanceId << " result: " << (result ? "success" : "fail"));
   
    return result;
}

Component* ComponentManager::getComponent(ComponentId componentId, bool requireLocal /*= false*/) const
{
    ComponentMap::const_iterator it = mComponentMap.find(componentId);
    return (it != mComponentMap.end() && (!requireLocal || it->second->isLocal())) ? it->second : nullptr;
}

} // Blaze

