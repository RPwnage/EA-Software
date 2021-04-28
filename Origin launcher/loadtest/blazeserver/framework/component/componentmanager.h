/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COMPONENT_MANAGER_H
#define BLAZE_COMPONENT_MANAGER_H

/*** Include files *******************************************************************************/

#include "EASTL/hash_map.h"
#include "EASTL/vector_map.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/component/component.h"
#include "framework/component/componentstub.h"
#include "framework/blazedefines.h"
#include "EATDF/tdfobjectid.h"

namespace Blaze
{

class Controller;
class RemoteServerInstance;
class RemoteMapCollection;

/*! ************************************************************************/
/*! \brief 
*****************************************************************************/
class ComponentManager
{
    NON_COPYABLE(ComponentManager)
public:
    typedef eastl::hash_map<EA::TDF::ComponentId, Component*> ComponentMap;
    typedef eastl::hash_map<EA::TDF::ComponentId, ComponentStub*> ComponentStubMap;
   
    void initialize(Controller& controller);
    void shutdown();
    Component* getComponent(EA::TDF::ComponentId componentId, bool requireLocal = false) const;
    Component* getLocalComponent(EA::TDF::ComponentId componentId) const
    { 
        ComponentStub *stub = getLocalComponentStub(componentId);
        return stub != nullptr ? &stub->asComponent() : nullptr;
    }

    ComponentStub* getLocalComponentStub(EA::TDF::ComponentId componentId) const
    {
        ComponentStubMap::const_iterator itr = mLocalComponentMap.find(componentId);
        return itr != mLocalComponentMap.end() ? itr->second : nullptr;
    }

    ComponentStubMap& getLocalComponents() { return mLocalComponentMap; }
    const ComponentStubMap& getLocalComponents() const { return mLocalComponentMap; }


    BlazeRpcError addRemoteComponentPlaceholder(EA::TDF::ComponentId compId, Component*& componentPlaceholder);
    
private:
    ComponentManager() :
        mLocalComponentMap(BlazeStlAllocator("mLocalComponentMap")),
        mComponentMap(BlazeStlAllocator("mComponentMap"))
    {}
    ~ComponentManager() {}
    
    ComponentStub* addLocalComponent(EA::TDF::ComponentId id);
    BlazeRpcError addRemoteComponent(EA::TDF::ComponentId compId, InstanceId instanceId);
    bool removeRemoteComponent(EA::TDF::ComponentId compId, InstanceId instanceId);
    
private:
    friend class Component;
    friend class Controller;
    friend class RemoteServerInstance;
    friend class RemoteMapCollection;
    
    ComponentStubMap mLocalComponentMap;
    ComponentMap mComponentMap;
    typedef eastl::vector<EA::TDF::ComponentId> ComponentIdList;
    ComponentIdList mComponentIdList;
};

} // Blaze

#endif // BLAZE_INSTANCE_MANAGER_H


