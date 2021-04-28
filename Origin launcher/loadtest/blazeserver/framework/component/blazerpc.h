/**************************************************************************************************/
/*!
    \file 

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#ifndef BLAZE_BLAZERPC_H
#define BLAZE_BLAZERPC_H

/*** Include Files ********************************************************************************/
#include "EASTL/map.h"
#include "EASTL/hash_map.h"
#include "framework/component/blazepermissions.h"
#include "framework/protocol/shared/protocoltypes.h"
#include "framework/connection/inetaddress.h"
#include "framework/replication/replication.h"
#include "framework/system/fiber.h"
#include "framework/grpc/grpcutils.h"

#include <functional>
#include <EASTL/unique_ptr.h>

#include "EATDF/tdfobjectid.h"

/*** Defines/Macros/Constants/Typedefs ************************************************************/
namespace google
{
namespace protobuf
{
class Message;
}
}

namespace grpc
{
class Service;
class ServerCompletionQueue;
}

namespace Blaze
{
namespace Grpc
{
class GrpcEndpoint;
class InboundRpcBase;
}

class CollectionId;
class Component;
class Command;
class Message;
class UserSession;
struct Notification;

typedef Command* (*CommandCreateFunc)(Message* msg, EA::TDF::Tdf* request, Component* component);
typedef std::function<void(grpc::Service*, grpc::ServerCompletionQueue*, Blaze::Grpc::GrpcEndpoint*)> GrpcRpcCreateFunc;
typedef std::function<const EA::TDF::Tdf*(const google::protobuf::Message*)> ProtobufToTdfCast; 
typedef std::function<google::protobuf::Message*(const EA::TDF::Tdf*)> TdfToProtobufCast;


typedef std::function<bool(Blaze::Grpc::InboundRpcBase&, const EA::TDF::Tdf*)> GrpcSend_ResponseTdf; // Send a Grpc response when command succeeded and has a response tdf.
typedef std::function<bool(Blaze::Grpc::InboundRpcBase&)> GrpcSend_NoResponseTdf; // Send a Grpc response when command succeeded but does not have a response tdf.

typedef std::function<bool(Blaze::Grpc::InboundRpcBase&, BlazeRpcError, const EA::TDF::Tdf*)> GrpcSend_ErrorResponseTdf; // Send a Grpc response when command failed and has an error tdf.
typedef std::function<bool(Blaze::Grpc::InboundRpcBase&, BlazeRpcError)> GrpcSend_NoErrorResponseTdf; // Send a Grpc response when command failed but does not have an error tdf.

struct ComponentData;
struct CommandInfo
{
    const ComponentData* componentInfo;
    CommandId commandId;
    size_t index;
    const char8_t* name;
    const char8_t* context;
    const char8_t* loggingName;
    const char8_t* description;

    EA::TDF::ComponentId passthroughComponentId;
    CommandId passthroughCommandId;

    bool isBlocking; 
    Fiber::StackSize fiberStackSize;
    EA::TDF::TimeValue fiberTimeoutOverride;
    bool requiresAuthentication;
    bool requiresUserSession;
    bool setCurrentUserSession;
    bool allowGuestCall;
    bool isInternalOnly; 
    bool useShardKey;
    bool obfuscatePlatformInfo;

    Blaze::Grpc::RpcType rpcType;

    const EA::TDF::TypeDescriptionClass* requestTdfDesc;
    const EA::TDF::TypeDescriptionClass* responseTdfDesc;
    const EA::TDF::TypeDescriptionClass* errorTdfDesc;
    CommandCreateFunc commandCreateFunc;
    GrpcRpcCreateFunc grpcRpcCreateFunc;

    // Due to multiple inheritance, we can't go from google::protobuf::Message* to EA::TDF::Tdf* using static/reinterpret_cast.
    // dynamic_cast would do it but we don't have it. So we use below helper function to allow compiler to generate right offsets.
    ProtobufToTdfCast requestProtobufToTdf;

    // Unlike Legacy Rpc Framework, which has optional response and error response, Grpc always sends a response. Following helpers take care of that.
    GrpcSend_ResponseTdf sendGrpc_ResponseTdf;
    GrpcSend_NoResponseTdf sendGrpc_NoResponseTdf;

    GrpcSend_ErrorResponseTdf sendGrpc_ErrorResponseTdf;
    GrpcSend_NoErrorResponseTdf sendGrpc_NoErrorResponseTdf;

    const RestResourceInfo* restResourceInfo;

    bool sendGrpcResponse(Grpc::InboundRpcBase& rpc, const EA::TDF::Tdf* tdf, BlazeRpcError err) const;

    EA::TDF::TdfPtr createRequest(const char8_t* memName) const;
    EA::TDF::TdfPtr createResponse(const char8_t* memName) const;
    EA::TDF::TdfPtr createErrorResponse(const char8_t* memName) const;

    const RestResourceInfo* getRestResourceInfo(const char8_t* method, const char8_t* actualPath) const;
};

typedef std::function<void(Blaze::Grpc::InboundRpcBase&, Notification&)> GrpcSendNotification;
struct NotificationInfo
{
    EA::TDF::ComponentId componentId;
    NotificationId notificationId;
    size_t index;
    const char8_t* name;
    const char8_t* context;
    const char8_t* loggingName;
    const char8_t* description;

    bool isClientExport;
    bool obfuscatePlatformInfo;
    EA::TDF::ComponentId passthroughComponentId;
    CommandId passthroughNotificationId;

    const EA::TDF::TypeDescriptionClass* payloadTypeDesc;

    typedef void (Component::*NotificationDispatchFunc)(const EA::TDF::Tdf*, UserSession*) const;
    NotificationDispatchFunc dispatchFunc;

    GrpcSendNotification sendNotificationGrpc;
    bool is(const NotificationInfo& info) const { return ((info.componentId == componentId) && (info.notificationId == notificationId)); }
};

struct EventInfo
{
    EA::TDF::ComponentId componentId;
    EventId eventId;
    size_t index;
    const char8_t* name;

    const EA::TDF::TypeDescriptionClass* payloadTypeDesc;
};

struct ObjectTypeInfo
{
    EA::TDF::ComponentId componentId;
    EA::TDF::EntityType typeId;
    bool hasIdentity;    
    const char8_t* name;
};

struct PermissionInfo
{
    Authorization::Permission id;
    EA::TDF::ComponentId componentId;
    uint16_t subId;    
    const char8_t* name;
};


struct ErrorInfo
{
    BlazeRpcError id;
    EA::TDF::ComponentId componentId;
    uint16_t subId;
    const char8_t* name;
    const char8_t* description;
    HttpStatusCode httpStatusCode;
};

struct ReplicationInfo
{
    const ComponentData* componentInfo;
    const char8_t* name;

    const CollectionIdRange* range;
    
    const EA::TDF::TypeDescriptionClass* itemTypeDesc;
    const EA::TDF::TypeDescriptionClass* contextTypeDesc;
};

struct ReplicationNotificationInfo : NotificationInfo
{
    ReplicationNotificationInfo(EA::TDF::ComponentId _componentId, NotificationId _notificationId, const char8_t* _name, const EA::TDF::TypeDescriptionClass* _payloadTypeDesc)
    {
        componentId = _componentId;
        notificationId = _notificationId;
        name = _name;
        payloadTypeDesc = _payloadTypeDesc;

        index = (size_t)(-1);
        context = "Component::processNotification";
        loggingName = "processNotification";
        isClientExport = false;
        passthroughComponentId = RPC_COMPONENT_UNKNOWN;
        passthroughNotificationId = 0xFFFF;
        dispatchFunc = nullptr;
    }
};

struct ComponentBaseInfo;
class RpcProxyResolver;
struct ComponentData
{
    NON_COPYABLE(ComponentData);
public:
    typedef Component* (*LocalComponentFactoryFunc)();
    typedef Component* (*RemoteComponentFactoryFunc)(RpcProxyResolver& resolver, const InetAddress& externalAddress);
    typedef std::function<eastl::unique_ptr<grpc::Service>()> GrpcServiceCreateFunc;

    //This one needs a constructor to build the maps properly
    ComponentData(const ComponentBaseInfo& _baseInfo, EA::TDF::ComponentId _id, const char8_t* _name, const char8_t* _loggingName, const char8_t* description, bool hasMaster,
        bool requiresMasterComponent, bool hasMultipleInstances, 
        bool _hasReplication, bool _shouldAutoRegisterForMasterReplication,
        bool _hasNotifications, bool _shouldAutoRegisterForMasterNotifications,
        const CommandInfo* _commands[], size_t numCommands, 
        const NotificationInfo* _notifications[], size_t numNotifications,
        const EventInfo events[], size_t numEvents, 
        const ReplicationInfo replication[], size_t numReplication,
        LocalComponentFactoryFunc localCreateFunc, RemoteComponentFactoryFunc remoteCreateFunc, GrpcServiceCreateFunc grpcCreateFunc,
        const bool _proxyOnly);

    const ComponentBaseInfo& baseInfo;
    EA::TDF::ComponentId id;
    size_t index;
    const char8_t* name;
    const char8_t* loggingName;
    const char8_t* description;
    bool hasMaster;
    bool requiresMasterComponent;
    bool hasMultipleInstances;
    bool hasReplication;
    bool shouldAutoRegisterForMasterReplication;
    bool hasNotifications;
    bool shouldAutoRegisterForMasterNotifications;

    typedef eastl::hash_map<CommandId, const CommandInfo*> CommandInfoMap;
    CommandInfoMap commands;

    typedef eastl::hash_map<const char8_t*, const CommandInfo*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*>  > CommandInfoByNameMap;
    CommandInfoByNameMap commandsByName;

    typedef eastl::hash_map<NotificationId, const NotificationInfo*> NotificationInfoMap;
    NotificationInfoMap notifications;

    typedef eastl::hash_map<const char8_t*, const NotificationInfo*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*>  > NotificationInfoByNameMap;
    NotificationInfoByNameMap notificationsByName;

    typedef eastl::hash_map<EventId, const EventInfo*> EventInfoMap;
    EventInfoMap events;

    typedef eastl::hash_map<const char8_t*, const EventInfo*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*>  > EventInfoByNameMap;
    EventInfoByNameMap eventsByName;

    typedef eastl::map<CollectionIdRange, const ReplicationInfo*> ReplicationInfoMap;
    ReplicationInfoMap replicationInfo;

    typedef eastl::hash_map<const char8_t*, const ReplicationInfo*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*>  > ReplicationInfoByNameMap;
    ReplicationInfoByNameMap replicationInfoByName;

    LocalComponentFactoryFunc localComponentCreateFunc;
    RemoteComponentFactoryFunc remoteComponentCreateFunc;
    GrpcServiceCreateFunc grpcServiceCreateFunc;

    bool proxyOnly;

    Component* createLocalComponent() const { return localComponentCreateFunc != nullptr ? (*localComponentCreateFunc)() : nullptr; } 
    Component* createRemoteComponent(RpcProxyResolver& resolver) const { return (*remoteComponentCreateFunc)(resolver, InetAddress()); }
    Component* createRemoteComponent(RpcProxyResolver& resolver, InetAddress& addy) const { return (*remoteComponentCreateFunc)(resolver, addy); }

    const CommandInfo* getCommandInfo(CommandId commandId) const
    {
        CommandInfoMap::const_iterator itr = commands.find(commandId);
        return (itr != commands.end()) ? itr->second : nullptr;
    }

    const CommandInfo* getCommandInfo(const char8_t* commandName) const
    {
        CommandInfoByNameMap::const_iterator itr = commandsByName.find(commandName);
        return (itr != commandsByName.end()) ? itr->second : nullptr;
    }

    const NotificationInfo* getNotificationInfo(NotificationId notifId) const
    {
        NotificationInfoMap::const_iterator itr = notifications.find(notifId);
        return (itr != notifications.end()) ? itr->second : nullptr;
    }

    const NotificationInfo* getNotificationInfo(const char8_t* notifName) const
    {
        NotificationInfoByNameMap::const_iterator itr = notificationsByName.find(notifName);
        return (itr != notificationsByName.end()) ? itr->second : nullptr;
    }

    const EventInfo* getEventInfo(EventId eventId) const
    {
        EventInfoMap::const_iterator itr = events.find(eventId);
        return (itr != events.end()) ? itr->second : nullptr;
    }

    const EventInfo* getEventInfo(const char8_t* eventName) const
    {
        EventInfoByNameMap::const_iterator itr = eventsByName.find(eventName);
        return (itr != eventsByName.end()) ? itr->second : nullptr;
    }

    const ReplicationInfo* getReplicationInfo(const CollectionIdRange& range) const
    {
        ReplicationInfoMap::const_iterator itr = replicationInfo.find(range);
        return (itr != replicationInfo.end()) ? itr->second : nullptr;
    }

    const ReplicationInfo* getReplicationInfo(const CollectionId& riId) const;

    const ReplicationInfo* getReplicationInfo(const char8_t* riName) const
    {
        ReplicationInfoByNameMap::const_iterator itr = replicationInfoByName.find(riName);
        return (itr != replicationInfoByName.end()) ? itr->second : nullptr;
    }

    const RestResourceInfo* getRestResourceInfo(const char8_t* method, const char8_t* actualPath) const;

    static const ComponentData* getComponentData(EA::TDF::ComponentId cId)
    {
        ComponentDataMap::const_iterator itr = getInfoMap().find(cId);
        return (itr != getInfoMap().end()) ? itr->second : nullptr;
    }

    static const ComponentData* getComponentDataAtIndex(size_t index)
    {
        const ComponentData* result = nullptr;
        if (index < getComponentDataVector().size())
        {
            result = getComponentDataVector().at(index);
        }

        return result;
    }

    static const ComponentData* getComponentDataByName(const char8_t* cdName)
    {
        ComponentDataByNameMap::const_iterator itr = getInfoByNameMap().find(cdName);
        return (itr != getInfoByNameMap().end()) ? itr->second : nullptr;
    }

    static size_t getComponentCount()
    {
        return getComponentDataVector().size();
    }

    static const CommandInfo* getCommandInfo(EA::TDF::ComponentId componentId, CommandId commandId)
    {
        ComponentDataMap::const_iterator itr = getInfoMap().find(componentId);
        return (itr != getInfoMap().end()) ? itr->second->getCommandInfo(commandId) : nullptr;
    }

    static const NotificationInfo* getNotificationInfo(EA::TDF::ComponentId componentId, NotificationId notificationId)
    {
        ComponentDataMap::const_iterator itr = getInfoMap().find(componentId);
        return (itr != getInfoMap().end()) ? itr->second->getNotificationInfo(notificationId) : nullptr;
    }

    static const EventInfo* getEventInfo(EA::TDF::ComponentId componentId, EventId eventId)
    {
        ComponentDataMap::const_iterator itr = getInfoMap().find(componentId);
        return (itr != getInfoMap().end()) ? itr->second->getEventInfo(eventId) : nullptr;
    }

private:
    friend class BlazeRpcComponentDb;

    typedef eastl::hash_map<EA::TDF::ComponentId, const ComponentData*> ComponentDataMap;
    typedef eastl::hash_map<const char8_t*, const ComponentData*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*>  > ComponentDataByNameMap;
    typedef eastl::vector<const ComponentData*> ComponentDataVector;

    static ComponentDataMap& getInfoMap()
    {
        static ComponentDataMap sInfo(BlazeStlAllocator("ComponentDataMap"));
        return sInfo;
    }

    static ComponentDataByNameMap& getInfoByNameMap()
    {
        static ComponentDataByNameMap sInfo(BlazeStlAllocator("ComponentDataByNameMap"));
        return sInfo;
    }

    static ComponentDataVector& getComponentDataVector()
    {
        static ComponentDataVector sVector;
        return sVector;
    }

    ReplicationNotificationInfo mREPLICATION_MAP_CREATE_NOTIFICATION;
    ReplicationNotificationInfo mREPLICATION_MAP_DESTROY_NOTIFICATION;
    ReplicationNotificationInfo mREPLICATION_OBJECT_UPDATE_NOTIFICATION;
    ReplicationNotificationInfo mREPLICATION_OBJECT_INSERT_NOTIFICATION;
    ReplicationNotificationInfo mREPLICATION_OBJECT_ERASE_NOTIFICATION;
    ReplicationNotificationInfo mREPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION;
    ReplicationNotificationInfo mREPLICATION_SYNC_COMPLETE_NOTIFICATION;
    ReplicationNotificationInfo mREPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION;
};

struct ComponentBaseInfo
{
private:
    typedef eastl::hash_map<EA::TDF::ComponentId, const ComponentBaseInfo* > ComponentBaseInfoByIdMap;
    typedef eastl::hash_map<const char8_t*, const ComponentBaseInfo*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*>  > ComponentBaseInfoByNameMap;
    typedef eastl::vector<const ComponentBaseInfo*> ComponentBaseInfoVector;
    typedef eastl::hash_map<Authorization::Permission, const PermissionInfo*> PermissionInfoMap;
    typedef eastl::hash_map<BlazeRpcError, const ErrorInfo*> ErrorInfoMap;
    typedef eastl::hash_map<const char8_t*, const ErrorInfo*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*>  > ErrorInfoByNameMap;

public:
    ComponentBaseInfo(EA::TDF::ComponentId _id, const char8_t* _name,
        const ObjectTypeInfo _objectTypes[], size_t numObjectTypes,
        const PermissionInfo _permissions[], size_t numPermissions,
        const ErrorInfo _errors[], size_t numErrors,
        const EA::TDF::TypeDescriptionClass* _configTypeDesc,
        const EA::TDF::TypeDescriptionClass* _preconfigTypeDesc,
        const ComponentData* _slaveComponent,
        const ComponentData* _masterComponent,
        const char8_t* _allocGroupNames[], size_t numAllocGroups);

    EA::TDF::ComponentId id; //Same as the slave id, if there is one
    size_t index;
    MemoryGroupId memgroup;          // Base Memory Group - Additional groups must be tracked separately (created with MEM_GROUP_GET_SUB_ALLOC_GROUP)
    const char8_t** allocGroupNames;
    size_t allocGroups;
    const char8_t* name;
    const ComponentData* slaveInfo;
    const ComponentData* masterInfo;

    typedef eastl::hash_map<EA::TDF::EntityType, const ObjectTypeInfo*> ObjectTypeInfoMap;
    ObjectTypeInfoMap objectTypes;

    typedef eastl::hash_map<const char8_t*, const ObjectTypeInfo*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*>  > ObjectTypeInfoByNameMap;
    ObjectTypeInfoByNameMap objectTypesByName;

    const EA::TDF::TypeDescriptionClass* configTdfDesc;
    const EA::TDF::TypeDescriptionClass* preconfigTdfDesc;

    const ObjectTypeInfo* getObjectTypeInfo(EA::TDF::EntityType otId) const
    {
        ObjectTypeInfoMap::const_iterator itr = objectTypes.find(otId);
        return (itr != objectTypes.end()) ? itr->second : nullptr;
    }

    const ObjectTypeInfo* getObjectTypeInfo(const char8_t* otNme) const
    {
        ObjectTypeInfoByNameMap::const_iterator itr = objectTypesByName.find(otNme);
        return (itr != objectTypesByName.end()) ? itr->second : nullptr;
    }

    static const ComponentBaseInfo* getComponentBaseInfo(EA::TDF::ComponentId cid)
    {
        ComponentBaseInfoByIdMap::const_iterator itr = getInfoMap().find(cid);
        return (itr != getInfoMap().end()) ? itr->second : nullptr;
    }

    static const ComponentBaseInfo* getComponentBaseInfoAtIndex(size_t index)
    {
        const ComponentBaseInfo* result = nullptr;
        if (index < getComponentBaseInfoVector().size())
        {
            result = getComponentBaseInfoVector().at(index);
        }

        return result;
    }

    static const ComponentBaseInfo* getComponentBaseInfoByName(const char8_t* cbiName)
    {
        ComponentBaseInfoByNameMap::const_iterator itr = getInfoByNameMap().find(cbiName);
        return (itr != getInfoByNameMap().end()) ? itr->second : nullptr;
    }

    static size_t getComponentTypeCount()
    {
        return getComponentBaseInfoVector().size();
    }

    static const PermissionInfo* getPermissionInfo(Authorization::Permission piId) 
    {
        PermissionInfoMap::const_iterator itr = getPermissionInfoMap().find(piId);
        return (itr != getPermissionInfoMap().end()) ? itr->second : nullptr;
    }

    static const PermissionInfo* getPermissionInfo(const char8_t* piName)
    {
        PermissionInfoByNameMap::const_iterator itr = getPermissionInfoByNameMap().find(piName);
        return (itr != getPermissionInfoByNameMap().end()) ? itr->second : nullptr;
    }

    static const char8_t* getPermissionNameById(Authorization::Permission permission)
    {
        const PermissionInfo* info = getPermissionInfo(permission);
        return (info != nullptr) ? info->name : "UNKNOWN";
    }

    static Authorization::Permission getPermissionIdByName(const char8_t* permissionName)
    {
        const PermissionInfo* info = ComponentBaseInfo::getPermissionInfo(permissionName);
        return (info != nullptr) ? info->id : Authorization::PERMISSION_NONE;
    }

    typedef eastl::hash_map<const char8_t*, const PermissionInfo*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*>  > PermissionInfoByNameMap;
    static const PermissionInfoByNameMap& getAllPermissionsByName()
    {
        return getPermissionInfoByNameMap();
    }

    static const ErrorInfo* getErrorInfo(BlazeRpcError eiId) 
    {
        ErrorInfoMap::const_iterator itr = getErrorInfoMap().find(eiId);
        return (itr != getErrorInfoMap().end()) ? itr->second : nullptr;
    }

    static const ErrorInfo* getErrorInfo(const char8_t* eiName)
    {
        ErrorInfoByNameMap::const_iterator itr = getErrorInfoByNameMap().find(eiName);
        return (itr != getErrorInfoByNameMap().end()) ? itr->second : nullptr;
    }

    static void dumpComponentTypeIndexToLog();
private:
    friend struct SystemErrorBootStrap; //This class bootsraps our system errors into the map

    static ComponentBaseInfoByNameMap& getInfoByNameMap()
    {
        static ComponentBaseInfoByNameMap sInfo(BlazeStlAllocator("ComponentBaseInfoByNameMap"));
        return sInfo;
    }

    static ComponentBaseInfoByIdMap& getInfoMap()
    {
        static ComponentBaseInfoByIdMap sInfo(BlazeStlAllocator("ComponentBaseInfoByIdMap"));
        return sInfo;
    }

    static ComponentBaseInfoVector& getComponentBaseInfoVector()
    {
        static ComponentBaseInfoVector sVector(BlazeStlAllocator("ComponentBaseInfoVector"));
        return sVector;
    }

    static PermissionInfoMap& getPermissionInfoMap()
    {
        static PermissionInfoMap& sMap = getPermissionInfoMapInner();
        return sMap;
    }

    static PermissionInfoByNameMap& getPermissionInfoByNameMap()
    {
        static PermissionInfoByNameMap& sMap = getPermissionInfoByNameMapInner();
        return sMap;
    }

    static ErrorInfoMap& getErrorInfoMap()
    {
        static ErrorInfoMap sMap(BlazeStlAllocator("ErrorInfoMap"));
        return sMap;
    }

    static ErrorInfoByNameMap& getErrorInfoByNameMap()
    {
        static ErrorInfoByNameMap sMap(BlazeStlAllocator("ErrorInfoByNameMap"));
        return sMap;
    }

    static PermissionInfoMap& getPermissionInfoMapInner();
    static PermissionInfoByNameMap& getPermissionInfoByNameMapInner();
};



class BlazeRpcComponentDb
{
public:
    typedef eastl::map<uint16_t, const char8_t*> NamesById;
    
    static void initialize(EA::TDF::ComponentId componentId = 0);
    static size_t getTotalComponentCount();
    static size_t getTotalProxyComponentCount();

    static size_t getMemGroupCategoryIndex(EA::TDF::ComponentId componentId);
    static size_t getAllocGroups(size_t componentCategoryIndex);
    static size_t getComponentCategoryIndex(EA::TDF::ComponentId componentId);
    static size_t getComponentAllocGroups(size_t componentCategoryIndex);
    static size_t getProxyComponentCategoryIndex(EA::TDF::ComponentId componentId);
    static size_t getProxyComponentAllocGroups(size_t componentCategoryIndex);
    


    static bool isValidComponentId(EA::TDF::ComponentId componentId);
    static bool isNonProxyComponent(const char8_t* componentName);
    static EA::TDF::ComponentId getComponentIdByName(const char8_t* componentName);
    static bool getComponentIdsByBaseName(const char8_t* componentBaseName, EA::TDF::ComponentId &slaveId, EA::TDF::ComponentId &masterId);
    static CommandId getCommandIdByName(EA::TDF::ComponentId componentId, const char8_t* commandName);
    static NotificationId getNotificationIdByName(EA::TDF::ComponentId componentId, const char8_t* notificationName, bool* isFound = nullptr);
    static EventId getEventIdByName(EA::TDF::ComponentId componentId, const char8_t* eventName, bool* isFound = nullptr);
    static EA::TDF::EntityType getEntityTypeByName(EA::TDF::ComponentId componentId, const char8_t* entityTypeName);
    static EA::TDF::ObjectType getBlazeObjectTypeByName(const char8_t* qualifiedTypeName);
    static const char8_t* getBlazeObjectTypeNameById(EA::TDF::ObjectType type, bool* isKnown = nullptr);
    static bool hasIdentity(EA::TDF::ObjectType type);

    static const char8_t* getComponentNameById(EA::TDF::ComponentId componentId, bool* isKnown = nullptr);
    static const char8_t* getComponentBaseNameById(EA::TDF::ComponentId componentId, bool* isKnown = nullptr);
    static const char8_t* getCommandNameById(EA::TDF::ComponentId componentId, CommandId commandId, bool* isKnown = nullptr);
    static const char8_t* getNotificationNameById(EA::TDF::ComponentId componentId, NotificationId notificationId, bool* isKnown = nullptr);
    static const char8_t* getEventNameById(EA::TDF::ComponentId componentId, EventId eventId, bool* isKnown = nullptr);
    static const char8_t* getEntityTypeNameById(EA::TDF::ComponentId componentId, EA::TDF::EntityType entityTypeId, bool* isKnown = nullptr);
    static NamesById* getComponents();
    static NamesById* getCommandsByComponent(EA::TDF::ComponentId componentId);
    static NamesById* getNotificationsByComponent(EA::TDF::ComponentId componentId);
    static int32_t getComponentIndex(EA::TDF::ComponentId componentId);
    static EA::TDF::ComponentId getComponentIdFromIndex(int32_t componentIndex);
    
    static const RestResourceInfo* getRestResourceInfo(const char8_t* method, const char8_t* resourcePath); 
    static const RestResourceInfo* getRestResourceInfo(uint16_t componentId, uint16_t commandId);
    static bool hasDuplicateResourcePath();
    static EA::TDF::TdfPtr createConfigTdf(EA::TDF::ComponentId componentId);
    static EA::TDF::TdfPtr createPreconfigTdf(EA::TDF::ComponentId componentId);

    static void dumpIndexInfoToLog();
private:
    static void stripTemplateParam(const char8_t* method, const char8_t* resourcePath, char8_t* dest, uint32_t len);
};


} // Blaze


#endif // BLAZE_BLAZERPC_H

