/**************************************************************************************************/
/*!
    \file 

    \attention
        (c) Electronic Arts. All Rights Reserved.
    
*/
/**************************************************************************************************/


/*** Include Files ********************************************************************************/

#include "framework/blaze.h"
#include "framework/component/component.h"
#include "framework/component/blazerpc.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{

/*** Public Methods ******************************************************************************/



const RestResourceInfo* CommandInfo::getRestResourceInfo(const char8_t* method, const char8_t* actualPath) const
{
    if (restResourceInfo != nullptr)
    {
        if (restResourceInfo->equals(method, actualPath))
            return restResourceInfo;
    }

    return nullptr;
}

bool CommandInfo::sendGrpcResponse(Grpc::InboundRpcBase& rpc, const EA::TDF::Tdf* tdf, BlazeRpcError err) const
{
    rpc.logRpcPostamble(tdf, Grpc::getGrpcStatusFromBlazeError(err));
    if (err == ERR_OK) 
    {
        if (sendGrpc_ResponseTdf)
            return sendGrpc_ResponseTdf(rpc, tdf);
        else
            return sendGrpc_NoResponseTdf(rpc);
    }
    else
    {
        if (sendGrpc_ErrorResponseTdf)
            return sendGrpc_ErrorResponseTdf(rpc, err, tdf);
        else
            return sendGrpc_NoErrorResponseTdf(rpc, err);
    }
}

EA::TDF::TdfPtr CommandInfo::createRequest(const char8_t* memName) const
{ 
    return requestTdfDesc != nullptr ? requestTdfDesc->createInstance(*Allocator::getAllocator(componentInfo->baseInfo.memgroup), memName) : nullptr;

}

EA::TDF::TdfPtr CommandInfo::createResponse(const char8_t* memName) const 
{ 
    return responseTdfDesc != nullptr ? responseTdfDesc->createInstance(*Allocator::getAllocator(componentInfo->baseInfo.memgroup), memName) : nullptr;
}

EA::TDF::TdfPtr CommandInfo::createErrorResponse(const char8_t* memName) const 
{ 
    return errorTdfDesc != nullptr ? errorTdfDesc->createInstance(*Allocator::getAllocator(componentInfo->baseInfo.memgroup), memName) : nullptr; 
}


ComponentData::ComponentData(const ComponentBaseInfo& _baseInfo, ComponentId _id, const char8_t* _name, const char8_t* _loggingName, const char8_t* _description, bool _hasMaster,
    bool _requiresMasterComponent, bool _hasMultipleInstances, 
    bool _hasReplication, bool _shouldAutoRegisterForMasterReplication,
    bool _hasNotifications, bool _shouldAutoRegisterForMasterNotifications,
    const CommandInfo* _commands[], size_t numCommands, 
    const NotificationInfo* _notifications[], size_t numNotifications,
    const EventInfo _events[], size_t numEvents,   
    const ReplicationInfo _replicationInfo[], size_t numReplication,
    ComponentData::LocalComponentFactoryFunc _localCreateFunc, ComponentData::RemoteComponentFactoryFunc _remoteCreateFunc, ComponentData::GrpcServiceCreateFunc _grpcCreateFunc,
    const bool _proxyOnly) :
    baseInfo(_baseInfo),
    id(_id), 
    name(_name),
    loggingName(_loggingName),
    description(_description),
    hasMaster(_hasMaster),
    requiresMasterComponent(_requiresMasterComponent),
    hasMultipleInstances(_hasMultipleInstances),
    hasReplication(_hasReplication),
    shouldAutoRegisterForMasterReplication(_shouldAutoRegisterForMasterReplication),
    hasNotifications(_hasNotifications),
    shouldAutoRegisterForMasterNotifications(_shouldAutoRegisterForMasterNotifications),
    commands(BlazeStlAllocator("ComponentData::commands")),
    commandsByName(BlazeStlAllocator("ComponentData::commandsByName")),
    notifications(BlazeStlAllocator("ComponentData::notifications")),
    notificationsByName(BlazeStlAllocator("ComponentData::notificationsByName")),
    events(BlazeStlAllocator("ComponentData::events")),
    eventsByName(BlazeStlAllocator("ComponentData::eventsByName")),
    replicationInfo(BlazeStlAllocator("ComponentData::replicationInfo")),
    replicationInfoByName(BlazeStlAllocator("ComponentData::replicationInfoByName")),
    localComponentCreateFunc(_localCreateFunc),
    remoteComponentCreateFunc(_remoteCreateFunc),
    grpcServiceCreateFunc(_grpcCreateFunc),
    proxyOnly(_proxyOnly),
    mREPLICATION_MAP_CREATE_NOTIFICATION(_id, Component::REPLICATION_MAP_CREATE_NOTIFICATION_ID, "REPLICATION_MAP_CREATE_NOTIFICATION", &EA::TDF::TypeDescriptionSelector<ReplicationMapCreationUpdate>::get()),
    mREPLICATION_MAP_DESTROY_NOTIFICATION(_id, Component::REPLICATION_MAP_DESTROY_NOTIFICATION_ID, "REPLICATION_MAP_DESTROY_NOTIFICATION", &EA::TDF::TypeDescriptionSelector<ReplicationMapDeletionUpdate>::get()),
    mREPLICATION_OBJECT_UPDATE_NOTIFICATION(_id, Component::REPLICATION_OBJECT_UPDATE_NOTIFICATION_ID, "REPLICATION_OBJECT_UPDATE_NOTIFICATION", &EA::TDF::TypeDescriptionSelector<ReplicationUpdate>::get()),
    mREPLICATION_OBJECT_INSERT_NOTIFICATION(_id, Component::REPLICATION_OBJECT_INSERT_NOTIFICATION_ID, "REPLICATION_OBJECT_INSERT_NOTIFICATION", &EA::TDF::TypeDescriptionSelector<ReplicationUpdate>::get()),
    mREPLICATION_OBJECT_ERASE_NOTIFICATION(_id, Component::REPLICATION_OBJECT_ERASE_NOTIFICATION_ID, "REPLICATION_OBJECT_ERASE_NOTIFICATION", &EA::TDF::TypeDescriptionSelector<ReplicationDeletionUpdate>::get()),
    mREPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION(_id, Component::REPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION_ID, "REPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION", &EA::TDF::TypeDescriptionSelector<ReplicationObjectSubscribe>::get()),
    mREPLICATION_SYNC_COMPLETE_NOTIFICATION(_id, Component::REPLICATION_SYNC_COMPLETE_NOTIFICATION_ID, "REPLICATION_SYNC_COMPLETE_NOTIFICATION", &EA::TDF::TypeDescriptionSelector<ReplicationSynchronizationComplete>::get()),
    mREPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION(_id, Component::REPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION_ID, "REPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION", &EA::TDF::TypeDescriptionSelector<ReplicationMapSynchronizationComplete>::get())
{
    for (size_t counter = 0; counter < numCommands; ++counter)
    {
        EA_ASSERT_FORMATTED(commands.count(_commands[counter]->commandId) == 0, ("Command '%s' uses overlapping id (%hu) with another command.", _commands[counter]->name, _commands[counter]->commandId));
        commands[_commands[counter]->commandId] = _commands[counter];
        commandsByName[_commands[counter]->name] = _commands[counter];
    }

    for (size_t counter = 0; counter < numNotifications; ++counter)
    {
        EA_ASSERT_FORMATTED(notifications.count(_notifications[counter]->notificationId) == 0, ("Notification '%s' uses overlapping id (%hu) with another notification.", _notifications[counter]->name, _notifications[counter]->notificationId));
        notifications[_notifications[counter]->notificationId] = _notifications[counter];
        notificationsByName[_notifications[counter]->name] = _notifications[counter];
    }

    {
        notifications[mREPLICATION_MAP_CREATE_NOTIFICATION.notificationId] = &mREPLICATION_MAP_CREATE_NOTIFICATION;
        notifications[mREPLICATION_MAP_DESTROY_NOTIFICATION.notificationId] = &mREPLICATION_MAP_DESTROY_NOTIFICATION;
        notifications[mREPLICATION_OBJECT_UPDATE_NOTIFICATION.notificationId] = &mREPLICATION_OBJECT_UPDATE_NOTIFICATION;
        notifications[mREPLICATION_OBJECT_INSERT_NOTIFICATION.notificationId] = &mREPLICATION_OBJECT_INSERT_NOTIFICATION;
        notifications[mREPLICATION_OBJECT_ERASE_NOTIFICATION.notificationId] = &mREPLICATION_OBJECT_ERASE_NOTIFICATION;
        notifications[mREPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION.notificationId] = &mREPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION;
        notifications[mREPLICATION_SYNC_COMPLETE_NOTIFICATION.notificationId] = &mREPLICATION_SYNC_COMPLETE_NOTIFICATION;
        notifications[mREPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION.notificationId] = &mREPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION;

        notificationsByName[mREPLICATION_MAP_CREATE_NOTIFICATION.name] = &mREPLICATION_MAP_CREATE_NOTIFICATION;
        notificationsByName[mREPLICATION_MAP_DESTROY_NOTIFICATION.name] = &mREPLICATION_MAP_DESTROY_NOTIFICATION;
        notificationsByName[mREPLICATION_OBJECT_UPDATE_NOTIFICATION.name] = &mREPLICATION_OBJECT_UPDATE_NOTIFICATION;
        notificationsByName[mREPLICATION_OBJECT_INSERT_NOTIFICATION.name] = &mREPLICATION_OBJECT_INSERT_NOTIFICATION;
        notificationsByName[mREPLICATION_OBJECT_ERASE_NOTIFICATION.name] = &mREPLICATION_OBJECT_ERASE_NOTIFICATION;
        notificationsByName[mREPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION.name] = &mREPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION;
        notificationsByName[mREPLICATION_SYNC_COMPLETE_NOTIFICATION.name] = &mREPLICATION_SYNC_COMPLETE_NOTIFICATION;
        notificationsByName[mREPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION.name] = &mREPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION;
    }

    for (size_t counter = 0; counter < numEvents; ++counter)
    {
        EA_ASSERT_FORMATTED(events.count(_events[counter].eventId) == 0, ("Event '%s' uses overlapping id (%hu) with another event.", _events[counter].name, _events[counter].eventId));
        events[_events[counter].eventId] = &_events[counter];
        eventsByName[_events[counter].name] = &_events[counter];
    }

    for (size_t counter = 0; counter < numReplication; ++counter)
    {
        EA_ASSERT_FORMATTED(replicationInfo.count(*_replicationInfo[counter].range) == 0, ("Replication info '%s' overlaps with range of another info.", _replicationInfo[counter].name));
        replicationInfo[*_replicationInfo[counter].range] = &_replicationInfo[counter];
        replicationInfoByName[_replicationInfo[counter].name] = &_replicationInfo[counter];
    }


    getComponentDataVector().push_back(this);
    index = getComponentDataVector().size() - 1;

    EA_ASSERT_FORMATTED(getInfoMap().count(id) == 0, ("Component '%s' uses overlapping id (0x%hx) with component '%s'.  Make sure components have unique id, and that the Arson component is not included.", name, id, getInfoMap()[id]->name));
    getInfoMap()[id] = this;   
    getInfoByNameMap()[name] = this;
}

const RestResourceInfo* ComponentData::getRestResourceInfo(const char8_t* method, const char8_t* actualPath) const
{
    const RestResourceInfo* result = nullptr;
    for (CommandInfoMap::const_iterator itr = commands.begin(), end = commands.end(); result == nullptr && itr != end; ++itr)
    {
        result = itr->second->getRestResourceInfo(method, actualPath);
    }

    return result;
}

const ReplicationInfo* ComponentData::getReplicationInfo(const CollectionId& riId) const
{
    const ReplicationInfo* result = nullptr;
    for (ReplicationInfoMap::const_iterator itr = replicationInfo.begin(), end = replicationInfo.end(); itr != end; ++itr)
    {
        if (itr->first.isInBounds(riId))
        {
            result = itr->second;
            break;
        }
    }

    return result;
}

ComponentBaseInfo::ComponentBaseInfo(ComponentId _id, const char8_t* _name,
    const ObjectTypeInfo _objectTypes[], size_t numObjectTypes,
    const PermissionInfo _permissions[], size_t numPermissions,
    const ErrorInfo _errors[], size_t numErrors,
    const EA::TDF::TypeDescriptionClass* _configTdfDesc, 
    const EA::TDF::TypeDescriptionClass* _preconfigTdfDesc, 
    const ComponentData* _slaveInfo, const ComponentData* _masterInfo,
    const char8_t* _allocGroupNames[], size_t numAllocGroups) :
    id(_id), 
    allocGroupNames(_allocGroupNames),
    allocGroups(numAllocGroups),
    name(_name),    
    slaveInfo(_slaveInfo),
    masterInfo(_masterInfo),
    objectTypes(BlazeStlAllocator("ComponentBaseInfo::objectTypes")),
    objectTypesByName(BlazeStlAllocator("ComponentBaseInfo::objectTypesByName")),
    configTdfDesc(_configTdfDesc),
    preconfigTdfDesc(_preconfigTdfDesc)
{   
    for (size_t counter = 0; counter < numObjectTypes; ++counter)
    {
        objectTypes[_objectTypes[counter].typeId] = &_objectTypes[counter];
        objectTypesByName[_objectTypes[counter].name] = &_objectTypes[counter];
    }

    for (size_t counter = 0; counter < numPermissions; ++counter)
    {
        getPermissionInfoMap()[_permissions[counter].id] = &_permissions[counter];
        getPermissionInfoByNameMap()[_permissions[counter].name] = &_permissions[counter];
    }

    for (size_t counter = 0; counter < numErrors; ++counter)
    {
        getErrorInfoMap()[_errors[counter].id] = &_errors[counter];
        getErrorInfoByNameMap()[_errors[counter].name] = &_errors[counter];
    }

    getComponentBaseInfoVector().push_back(this);
    index = getComponentBaseInfoVector().size() - 1;
    memgroup = MEM_GROUP_FROM_COMPONENT_CATEGORY(BlazeRpcComponentDb::getMemGroupCategoryIndex(id));
    getInfoMap()[id] = this;
    getInfoByNameMap()[name] = this;

    Logger::registerCategory(index, name);
}



static const PermissionInfo& getPermissionInfoNone()
{
    static PermissionInfo result = { Authorization::PERMISSION_NONE, Component::INVALID_COMPONENT_ID, (uint16_t) Authorization::PERMISSION_NONE, "PERMISSION_NONE" };
    return result;
}

static const PermissionInfo& getPermissionInfoAll()
{
    Authorization::Permission permAll = Authorization::PERMISSION_ALL;
    static PermissionInfo result = { Authorization::PERMISSION_ALL, Component::INVALID_COMPONENT_ID, (uint16_t)permAll, "PERMISSION_ALL" };
    return result;
}

ComponentBaseInfo::PermissionInfoMap& ComponentBaseInfo::getPermissionInfoMapInner()
{
    static ComponentBaseInfo::PermissionInfoMap sMap(BlazeStlAllocator("getPermissionInfoMapInner"));
    sMap[getPermissionInfoNone().id] = &getPermissionInfoNone();
    sMap[getPermissionInfoAll().id] = &getPermissionInfoAll();
    return sMap;
}

ComponentBaseInfo::PermissionInfoByNameMap& ComponentBaseInfo::getPermissionInfoByNameMapInner()
{
    static ComponentBaseInfo::PermissionInfoByNameMap sMap(BlazeStlAllocator("getPermissionInfoByNameMapInner"));
    sMap[getPermissionInfoNone().name] = &getPermissionInfoNone();
    sMap[getPermissionInfoAll().name] = &getPermissionInfoAll();
    return sMap;
}




ComponentId BlazeRpcComponentDb::getComponentIdByName(const char8_t* componentName)
{
    ComponentId result = Component::INVALID_COMPONENT_ID;
    const ComponentData* data = ComponentData::getComponentDataByName(componentName);
    if (data != nullptr)
    {
        result = data->id;
    }

    return result;
}

bool BlazeRpcComponentDb::getComponentIdsByBaseName(const char8_t* componentName, ComponentId &slaveId, ComponentId &masterId)
{
    slaveId = masterId = Component::INVALID_COMPONENT_ID;
    const ComponentBaseInfo* info = ComponentBaseInfo::getComponentBaseInfoByName(componentName);
    if (info != nullptr)
    {
        slaveId = info->slaveInfo != nullptr ? info->slaveInfo->id : Component::INVALID_COMPONENT_ID;
        masterId = info->masterInfo != nullptr ? info->masterInfo->id : Component::INVALID_COMPONENT_ID;
    }

    return (slaveId != Component::INVALID_COMPONENT_ID || masterId != Component::INVALID_COMPONENT_ID);
}


CommandId BlazeRpcComponentDb::getCommandIdByName(ComponentId componentId, const char8_t* commandName)
{
    const ComponentData* compInfo = ComponentData::getComponentData(componentId);
    if (compInfo != nullptr)
    {
        const CommandInfo* commInfo = compInfo->getCommandInfo(commandName);
        if (commInfo != nullptr)
        {
            return commInfo->commandId;
        }
    }

    return Component::INVALID_COMMAND_ID;
}

NotificationId BlazeRpcComponentDb::getNotificationIdByName(ComponentId componentId, const char8_t* notificationName, bool *isFound)
{
    const ComponentData* compInfo = ComponentData::getComponentData(componentId);
    if (compInfo != nullptr)
    {
        const NotificationInfo* notifInfo = compInfo->getNotificationInfo(notificationName);
        if (notifInfo != nullptr)
        {
            if (isFound != nullptr)
                *isFound = true;
            return notifInfo->notificationId;
        }
    }

    if (isFound != nullptr)
        *isFound = false;

    return Component::INVALID_NOTIFICATION_ID;
}

EventId BlazeRpcComponentDb::getEventIdByName(ComponentId componentId, const char8_t* eventName, bool *isFound)
{
    if (isFound != nullptr)
        *isFound = false;

    const ComponentData* compInfo = ComponentData::getComponentData(componentId);
    if (compInfo != nullptr)
    {
        const EventInfo* eventInfo = compInfo->getEventInfo(eventName);
        if (eventInfo != nullptr)
        {
            if (isFound != nullptr)
                *isFound = true;
            return eventInfo->eventId;
        }
    }

    return 0;
}

EA::TDF::EntityType BlazeRpcComponentDb::getEntityTypeByName(ComponentId componentId, const char8_t* entityTypeName)
{
    EA::TDF::EntityType result = 0;

    const ComponentBaseInfo* info = ComponentBaseInfo::getComponentBaseInfo(RpcMakeSlaveId(componentId));
    if (info != nullptr)
    {
        const ObjectTypeInfo* typeInfo = info->getObjectTypeInfo(entityTypeName);
        if (typeInfo != nullptr)
        {
            result = typeInfo->typeId;
        }            
    }

    return result;
}

 EA::TDF::ObjectType BlazeRpcComponentDb::getBlazeObjectTypeByName(const char8_t* qualifiedTypeName)
{
    //Unfortunately we have two competing formats for object id names - one using "/" and the other using "." for
    //separators.  Try "." as a backup in case "/" doesn't work.
     EA::TDF::ObjectType result =  EA::TDF::ObjectType::parseString(qualifiedTypeName);
    if (result == EA::TDF::OBJECT_TYPE_INVALID)
    {
        result =  EA::TDF::ObjectType::parseString(qualifiedTypeName, '.');
    }

    return result;
}

const char8_t* BlazeRpcComponentDb::getBlazeObjectTypeNameById( EA::TDF::ObjectType type, bool* isKnown)
{
    const char8_t* name = "";

    const ComponentBaseInfo* info = ComponentBaseInfo::getComponentBaseInfo(RpcMakeSlaveId(type.component));
    if (info != nullptr)
    {
        const ObjectTypeInfo* typeInfo = info->getObjectTypeInfo(type.type);
        if (typeInfo != nullptr)
        {
            name = typeInfo->name;
        }            
    }

    if (isKnown != nullptr)
        *isKnown = (name[0] != '\0');

    return name;
}

bool BlazeRpcComponentDb::hasIdentity( EA::TDF::ObjectType type)
{
    bool result = false;

    const ComponentBaseInfo* info = ComponentBaseInfo::getComponentBaseInfo(RpcMakeSlaveId(type.component));
    if (info != nullptr)
    {
        const ObjectTypeInfo* typeInfo = info->getObjectTypeInfo(type.type);
        if (typeInfo != nullptr)
        {
            result = typeInfo->hasIdentity;
        }            
    }

    return result;
}

const RestResourceInfo* BlazeRpcComponentDb::getRestResourceInfo(const char8_t* method, const char8_t* path)
{
    if (path == nullptr || *path == '\0' || method == nullptr || *method == '\0')
    {
        return nullptr;
    }

    const ComponentData::ComponentDataMap& map = ComponentData::getInfoMap();
    for (ComponentData::ComponentDataMap::const_iterator itr = map.begin(), end = map.end(); itr != end; ++itr)
    {
        const ComponentData* info = itr->second;
        if (info != nullptr)
        {
            const RestResourceInfo* restResourceInfo = info->getRestResourceInfo(method, path);
            if (restResourceInfo != nullptr)
                return restResourceInfo;
        }
    }

    return nullptr;
}

const RestResourceInfo* BlazeRpcComponentDb::getRestResourceInfo(uint16_t componentId, uint16_t commandId)
{
    if (componentId == 0 || commandId == 0)
    {
        return nullptr;
    }

    const CommandInfo* info = ComponentData::getCommandInfo(componentId, commandId);
    if (info != nullptr)
    {
        return info->restResourceInfo;
    }

    return nullptr;
}

bool BlazeRpcComponentDb::hasDuplicateResourcePath()
{
    eastl::set<eastl::string> restResourceInfoSet;

    const ComponentData::ComponentDataMap& map = ComponentData::getInfoMap();
    for (ComponentData::ComponentDataMap::const_iterator cdItr = map.begin(), cdEnd = map.end(); cdItr != cdEnd; ++cdItr)
    {
        const ComponentData* info = cdItr->second;
        if (info == nullptr)
            continue;

        if (info->proxyOnly == true)
            continue;

        for (ComponentData::CommandInfoMap::const_iterator ciItr = info->commands.begin(), ciEnd = info->commands.end(); ciItr != ciEnd; ++ciItr)
        {
            if (ciItr->second->restResourceInfo == nullptr)
                continue;

            char8_t path[256];
            stripTemplateParam(ciItr->second->restResourceInfo->methodName, ciItr->second->restResourceInfo->resourcePath, path, sizeof(path));
            eastl::string resourceStr(path);

            if (restResourceInfoSet.find(resourceStr) != restResourceInfoSet.end())
            {
                //Http "resource" is defined in the http field blocks in rpc definition.
                //Each resource corresponds to a distinct URL for the http service.  As a result it does not make sense for them to be duplicated.
                BLAZE_ERR_LOG(Log::SYSTEM, "[BlazeRpcComponentDb::hasDuplicateResourcePath: duplicate http resource: " << resourceStr);

                restResourceInfoSet.clear();
                return true;
            }
            restResourceInfoSet.insert(resourceStr);
        }
    }

    restResourceInfoSet.clear();
    return false;
}

const char8_t* BlazeRpcComponentDb::getComponentNameById(ComponentId componentId, bool* isKnown)
{
    const char8_t* name = nullptr;
    const ComponentData* info = ComponentData::getComponentData(componentId);
    if (info != nullptr)
    {
        name = info->name;
    }
    
    if (isKnown != nullptr)
        *isKnown = (name != nullptr);
    
    if (name == nullptr)
        return "<UNKNOWN>";
    
    return name;
}

const char8_t* BlazeRpcComponentDb::getComponentBaseNameById(ComponentId componentId, bool* isKnown)
{
    const char8_t* name = nullptr;
    const ComponentData* info = ComponentData::getComponentData(componentId);
    if (info != nullptr)
    {
        name = info->baseInfo.name;
    }

    if (isKnown != nullptr)
        *isKnown = (name != nullptr);

    if (name == nullptr)
        return "<UNKNOWN>";

    return name;
}

size_t BlazeRpcComponentDb::getMemGroupCategoryIndex(ComponentId componentId)
{
    size_t catIndex = getComponentCategoryIndex(componentId);
    if (catIndex != (size_t)-1)
        return catIndex;

    // Proxy components come after normal comps
    catIndex = getProxyComponentCategoryIndex(componentId);
    if (catIndex != (size_t)-1)
        catIndex += getTotalComponentCount();

    return catIndex;
}

size_t BlazeRpcComponentDb::getAllocGroups(size_t componentCategoryIndex)
{
    size_t allocGroups = getComponentAllocGroups(componentCategoryIndex);
    if (allocGroups != (size_t)-1)
        return allocGroups;

    // Proxy components come after normal comps
    return getProxyComponentAllocGroups(componentCategoryIndex - getTotalComponentCount());
}


bool BlazeRpcComponentDb::isValidComponentId(ComponentId componentId)
{
    return ComponentData::getComponentData(componentId) != nullptr;
}

bool BlazeRpcComponentDb::isNonProxyComponent(const char8_t* componentName)
{
    const ComponentData* data = ComponentData::getComponentDataByName(componentName);
    if (data == nullptr)
        return false;

    return !(data->proxyOnly);
}

const char8_t* BlazeRpcComponentDb::getCommandNameById(ComponentId componentId, CommandId commandId, bool* isKnown)
{
    const CommandInfo* info = ComponentData::getCommandInfo(componentId, commandId);
    if (isKnown != nullptr)    
        *isKnown = (info != nullptr);

    return (info != nullptr) ? info->name : "<UNKNOWN>";
}

const char8_t* BlazeRpcComponentDb::getNotificationNameById(ComponentId componentId, NotificationId notificationId, bool* isKnown)
{
    const NotificationInfo* info = ComponentData::getNotificationInfo(componentId, notificationId);
    if (isKnown != nullptr)
        *isKnown = (info != nullptr);

    return (info != nullptr) ? info->name : "<UNKNOWN>";
}

const char8_t* BlazeRpcComponentDb::getEventNameById(ComponentId componentId, EventId eventId, bool* isKnown)
{
    const EventInfo* info = ComponentData::getEventInfo(componentId, eventId);
    if (isKnown != nullptr)
        *isKnown = (info != nullptr);

    return (info != nullptr) ? info->name : "<UNKNOWN>";
}

const char8_t* BlazeRpcComponentDb::getEntityTypeNameById(ComponentId componentId, EA::TDF::EntityType entityTypeId, bool* isKnown)
{
    const ObjectTypeInfo* info = nullptr;
    const ComponentBaseInfo* baseInfo = ComponentBaseInfo::getComponentBaseInfo(componentId);
    if (baseInfo != nullptr)
    {
        info = baseInfo->getObjectTypeInfo(entityTypeId);
    }

    if (isKnown != nullptr)
        *isKnown = (info != nullptr);
    
    return (info != nullptr) ? info->name : "<UNKNOWN>";
}

BlazeRpcComponentDb::NamesById* BlazeRpcComponentDb::getComponents()
{
    NamesById* components = BLAZE_NEW NamesById;
    
    const ComponentData::ComponentDataMap& map = ComponentData::getInfoMap();
    for (ComponentData::ComponentDataMap::const_iterator itr = map.begin(), end = map.end(); itr != end; ++itr)
    {
        (*components)[itr->first] = itr->second->name;
    }

    return components;
}

BlazeRpcComponentDb::NamesById* BlazeRpcComponentDb::getCommandsByComponent(ComponentId componentId)
{
    NamesById* result = BLAZE_NEW NamesById;
    
    const ComponentData* info = ComponentData::getComponentData(componentId);
    if (info != nullptr)
    {
        for (ComponentData::CommandInfoMap::const_iterator itr = info->commands.begin(), end = info->commands.end(); itr != end; ++itr)
        {
            (*result)[itr->first] = itr->second->name;
        }
    }

    return result;
}

BlazeRpcComponentDb::NamesById* BlazeRpcComponentDb::getNotificationsByComponent(ComponentId componentId)
{
    NamesById* result = BLAZE_NEW NamesById;

    const ComponentData* info = ComponentData::getComponentData(componentId);
    if (info != nullptr)
    {
        for (ComponentData::NotificationInfoMap::const_iterator itr = info->notifications.begin(), end = info->notifications.end(); itr != end; ++itr)
        {
            (*result)[itr->first] = itr->second->name;
        }
    }

    return result;
}

int32_t BlazeRpcComponentDb::getComponentIndex(ComponentId componentId)
{
    const ComponentData* info = ComponentData::getComponentData(componentId);
    if (info != nullptr)
    {
        return (int32_t) info->index;
    }

    return -1;
}

ComponentId BlazeRpcComponentDb::getComponentIdFromIndex(int32_t index)
{
    const ComponentData* info = ComponentData::getComponentDataAtIndex((size_t) index);
    if (info != nullptr)
        return info->id;    
    return Component::INVALID_COMPONENT_ID;
}


EA::TDF::TdfPtr BlazeRpcComponentDb::createConfigTdf(ComponentId componentId)
{
    EA::TDF::TdfPtr result = nullptr;
    const ComponentData* data = ComponentData::getComponentData(componentId);
    if (data != nullptr)
    {
        result = data->baseInfo.configTdfDesc != nullptr ? data->baseInfo.configTdfDesc->createInstance(*Allocator::getAllocator(data->baseInfo.memgroup), "ConfigTdf") : nullptr; 
    }

    return result;
}

EA::TDF::TdfPtr BlazeRpcComponentDb::createPreconfigTdf(ComponentId componentId)
{
    EA::TDF::TdfPtr result = nullptr;
    const ComponentData* data = ComponentData::getComponentData(componentId);
    if (data != nullptr)
    {
        result = data->baseInfo.preconfigTdfDesc != nullptr ? data->baseInfo.preconfigTdfDesc->createInstance(*Allocator::getAllocator(data->baseInfo.memgroup), "PreconfigTdf") : nullptr; 
    }

    return result;
}

void BlazeRpcComponentDb::stripTemplateParam(const char8_t* method, const char8_t* resourcePath, char8_t* dest, uint32_t len)
{
    dest[0] = '\0';

    uint32_t i = 0;
    uint32_t j = blaze_strnzcat(dest, method, len);
    bool stripChar = false;

    while (resourcePath[i] != '\0' && j < len)
    {
        if (stripChar == false && resourcePath[i] == '{')
        {
            stripChar = true;
        }

        if (stripChar == false)
            dest[j++] = resourcePath[i];

        if (resourcePath[i] == '}')
        {
            stripChar = false;
        }

        i++;
    }

    dest[j] = '\0';
}

void ComponentBaseInfo::dumpComponentTypeIndexToLog()
{
    BLAZE_INFO_LOG(Log::SYSTEM, "Begin dump of component type indices:");
    ComponentBaseInfo::ComponentBaseInfoVector& infos = ComponentBaseInfo::getComponentBaseInfoVector();
    for (ComponentBaseInfo::ComponentBaseInfoVector::const_iterator itr = infos.begin(), end = infos.end(); itr != end; ++itr)
    {
        BLAZE_INFO_LOG(Log::SYSTEM, (*itr)->index << ": " << (*itr)->name);
    }
    BLAZE_INFO_LOG(Log::SYSTEM, "End dump of component type indices.");
}

void BlazeRpcComponentDb::dumpIndexInfoToLog()
{
    ComponentBaseInfo::dumpComponentTypeIndexToLog();
}


struct EATDFBlazeObjectIdBootstrapper 
{
    EATDFBlazeObjectIdBootstrapper() 
    {
        //Init EATDF before any config action
         EA::TDF::ObjectType::setNameConversionFuncs(&BlazeRpcComponentDb::getComponentNameById, &BlazeRpcComponentDb::getEntityTypeNameById,
            &BlazeRpcComponentDb::getComponentIdByName, &BlazeRpcComponentDb::getEntityTypeByName);    
    }
};

static EATDFBlazeObjectIdBootstrapper sEATDFBlazeObjectIdBootstrapper;

} // Blaze
