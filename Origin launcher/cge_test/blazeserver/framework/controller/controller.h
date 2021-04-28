/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CONTROLLER_H
#define BLAZE_CONTROLLER_H

/*** Include files *******************************************************************************/

#include "EASTL/vector.h"
#include "EASTL/vector_set.h"
#include "EASTL/list.h"
#include "EASTL/string.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/rpc/blazecontrollerslave_stub.h"
#include "framework/rpc/blazecontrollermaster.h"
#include "framework/system/fibersemaphore.h"
#include "framework/system/fiber.h"
#include "framework/metrics/metrics.h"
#include "framework/metrics/externalservicemetrics.h"

// remote instance & pasutil are used extensively by components, maybe easier to just include here
// now that they are stripped down of other includes.
#include "framework/controller/remoteinstance.h"
#include "framework/util/pasutil.h"

#include <eathread/eathread_mutex.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ComponentManager;
class InetAddress;
class ConfigFile;
class Endpoint;
class ConnectionManager;
class OutboundConnectionManager;
class MasterController;
class Selector;
class DebugSystem;
struct RemoteInstanceInfo;
class InstanceInfoController;

namespace Grpc
{
class InboundGrpcManager;
class OutboundGrpcManager;
}

class DrainListener;
class DrainStatusCheckHandler;
class RemoteServerInstanceListener;

class Controller :
    public BlazeControllerSlaveStub,
    private RemoteServerInstanceHandler,
    private BlazeControllerMasterListener
{
public:
    class StateChangeListener
    {
    public:
        virtual void onControllerStateChange(ComponentStub::State oldState, ComponentStub::State newState) = 0;
        virtual ~StateChangeListener() { }
    };

    class ReadyForServiceListener
    {
    public:
        virtual void onControllerReadyForService() = 0;
        virtual ~ReadyForServiceListener() { }
    };

    typedef Blaze::DrainListener DrainListener;
    typedef Blaze::DrainStatusCheckHandler DrainStatusCheckHandler;
    typedef Blaze::RemoteServerInstanceListener RemoteServerInstanceListener;

    Controller(Selector &selector, const char8_t* baseName, const char8_t* instanceName, uint16_t basePort);
    ~Controller() override;

    bool startController();
    void shutdownController(ShutdownReason shutdownReason);
    void reconfigureController();
    
    Component* getComponent(ComponentId id, bool reqLocal = true, bool reqActive = true, BlazeRpcError* waitResult = nullptr);
    ComponentManager& getComponentManager() const { return mComponentManager; }
    Grpc::InboundGrpcManager* getInboundGrpcManager() const { return mInboundGrpcManager.get(); }
    Grpc::OutboundGrpcManager* getOutboundGrpcManager() const { return mOutboundGrpcManager.get(); }
    const FrameworkConfig& getFrameworkConfigTdf() const { return mFrameworkConfigTdf; }

    bool isInstanceTypeConfigMaster() const;
    bool isInstanceTypeMaster() const;

    //+ Backward compatible methods
    bool isMasterController() const { return isInstanceTypeConfigMaster(); }
    bool isMasterInstance() const { return isInstanceTypeMaster(); }
    //-

    /*! \brief Gets the global instance id of this server */
    InstanceId getInstanceId() const { return mInstanceId; }

    /*! \brief Gets the global instance name of this server */
    const char8_t* getInstanceName() const { return mInstanceName; }

    PasUtil& getPasUtil() { return mPasUtil; }

    /*! \brief Gets the instance base name of this server */
    const char8_t* getBaseName() const { return mBaseName; }

    /*! \brief Gets the base port of this this server instance */
    uint16_t getBasePort() const { return mBasePort; }

    /*! \brief Gets the global instance type of this server, only available after connected to configMaster */
    ServerConfig::InstanceType getInstanceType() const { return mServerConfigTdf.getInstanceType(); }

    /*! \brief Gets the global instance id of this server in the form of a session id */
    SlaveSessionId getInstanceSessionId() const { return mInstanceSessionId; }

    /*! \brief Gets the type index of this server instance. aux slave > 0, all other types == 0 */
    uint8_t getInstanceTypeIndex() const { return mInstanceTypeIndex; }

    /*! \brief Gets the max number of slave types indexed by the instance type index */
    uint8_t getSlaveTypeCount() const { return mSlaveTypeCount; }

    /*! \brief Gets the list of ClientTypes this server is dedicated to. An empty list indicates all ClientTypes are supported. */
    const ClientTypeList& getClientTypes() const { return mServerConfigTdf.getClientTypes(); }

    /*! \brief Whether this instance subscribes to partial usersession replication, only valid after connected to configMaster */
    bool getPartialUserReplicationEnabled() const { return mServerConfigTdf.getPartialUserReplication(); }

    /*! \brief Whether this instance subscribes to receive immediate partial local usersession replication, only valid after connected to configMaster */
    bool getImmediatePartialLocalUserReplicationEnabled() const { return mServerConfigTdf.getImmediatePartialLocalUserReplication(); }

    const Endpoint* getInternalEndpoint() const;
    uint32_t getSchemaVersion(uint32_t dbId, const char8_t* componentName) const;
    
    const char8_t* getBlazeServerNucleusClientId() const { return mBootConfigTdf.getBlazeServerNucleusClientId(); }

    //+ In a split platform cluster scenario, some services defined in the configuration may not be active. 
    // The apis below will only return valid info for service names that are hosted by this blaze server. 
    const ServiceNameInfo* getServiceNameInfo(const char8_t* serviceName) const;
    const char8_t* getServiceProductName(const char8_t* serviceName) const;
    ClientPlatformType getServicePlatform(const char8_t* serviceName) const;
    [[deprecated("Change to gController->getNamespaceFromPlatform")]] const char8_t* getServicePersonaNamespace(const char8_t* serviceName) const;
    [[deprecated("Change to gController->getBlazeServerNucleusClientId")]] const char8_t* getBlazeServerClientId(const char8_t* serviceName) const { return getBlazeServerNucleusClientId(); }
    [[deprecated("Change to gUserSessionManager->getProductReleaseType")]] const char8_t* getReleaseType(const char8_t* serviceName) const;
    //-


    bool isPassthroughNotificationSource() const { return mServerConfigTdf.getInstanceType() == ServerConfig::AUX_SLAVE; }
    bool isDBDestructiveAllowed() const;
    bool isShutdownPending() const { return mShutdownPending; }
    bool isShuttingDown() const { return mShuttingDown; }
    bool isDraining() const { return mIsDraining; }
    bool isCoreInstance() const { return mInstanceTypeIndex == 0; }
    bool isInService() const { return mInService; }
    bool isReadyForService() const { return mIsReadyForService; }
    void setServiceState(bool inService);

    const EA::TDF::TimeValue& getStartupTime() const { return mStartupTime; }

    // Heat1 encoder back compat check.  Affects how list<>, map<> encoder their key/value types.
    bool isHeat1BackCompatEnabled() const { return mBootConfigTdf.getEnableHeat1BackCompat(); }


    // Validates if the service or global mock services used are allowed:
    bool validateConfiguredMockServices(bool mockServiceEnabled);
    bool getUseMockConsoleServices() const { return mBootConfigTdf.getUseMockConsoleServices(); }

    bool getXboxOneRequiresXboxClientAddress() const { return mBootConfigTdf.getXboxOneRequiresXboxClientAddress(); }

    const ServiceNameInfoMap& getHostedServicesInfo() const { return mHostedServicesInfo; }
    const ClientPlatformSet& getHostedPlatforms() const; // The platforms are specified as a list in the config file but we return it as a set here to make C++ side of things easier.
    const ClientPlatformSet& getUsedPlatforms() const; // On single-platform deployments, this is equivalent to getHostedPlatforms(). On shared cluster deployments, 'common' is added to the set of platforms returned.

    bool isSharedCluster() const;
    
    bool isPlatformHosted(ClientPlatformType platform) const; // Note - "common" is NOT a hosted platform. 
    bool isPlatformUsed(ClientPlatformType platform) const { return isPlatformHosted(platform) || (isSharedCluster() && platform == common); } // Note - "common" is a 'used' platform in shared cluster but not 'hosted'. 
    bool isServiceHosted(const char8_t* serviceName) const;

    const char8_t* getServiceEnvironment() const  { return mBootConfigTdf.getServiceEnvironment(); }
    const char8_t* getGrpcUserAgent() const { return mGrpcUserAgent.c_str(); }
    bool usesExternalString(ClientPlatformType platform) const { return platform == ClientPlatformType::nx; }

    const char8_t* getDefaultServiceName() const; // The instance always hosts a default service name in both shared as well as per platform deployment.
    const char8_t* getDefaultServiceName(ClientPlatformType platform) const; // The default service name look up by platform will return valid value only if the platform is actually hosted.
    const ServiceNameInfo* getDefaultServiceNameInfo(ClientPlatformType platform) const;
    ClientPlatformType getDefaultClientPlatform() const;
    const char8_t* getDefaultProductName() const;

    const char8_t* getNamespaceFromPlatform(ClientPlatformType platform) const;
    const char8_t* getDefaultProductNameFromPlatform(ClientPlatformType platform) const;

    typedef eastl::set<eastl::string> NamespacesList;
    const NamespacesList& getAllSupportedNamespaces() const;        

    // returns default servicename if the passed-in service name isn't active
    const char8_t* validateServiceName(const char8_t* requestedServiceName) const;
    // In shared platform scenario, verify that the service is hosted without any fallback. 
    // In single platform scenario, fall back to the default service if service name is not valid. 
    // This function should eventually replace the validateServiceName api
    const char8_t* verifyServiceName(const char8_t* requestedServiceName) const;

    OutboundConnectionManager& getOutboundConnectionManager() const { return mOutboundConnectionManager; }
    ConnectionManager& getConnectionManager() const { return mConnectionManager; }

    BlazeRpcError DEFINE_ASYNC_RET(getConfig(const char8_t* featureName, bool loadFromStaging, const ConfigFile*& config, EA::TDF::Tdf* configTdf = nullptr, bool strictParsing = true, bool preconfig = false) const);
    BlazeRpcError DEFINE_ASYNC_RET(getConfigTdf(const char8_t* featureName, bool loadFromStaging, EA::TDF::Tdf& configTdf, bool strictParsing = true, bool preconfig = false) const);

    bool upgradeSchemaWithDbMig(uint32_t componentVersion, uint32_t contentVersion, const char8_t* componentName, const DbIdByPlatformTypeMap& dbIdByPlatformTypeMap) const;
    void addStateChangeListener(StateChangeListener& listener) { mStateChangeDispatcher.addDispatchee(listener); }
    void removeStateChangeListener(StateChangeListener& listener) { mStateChangeDispatcher.removeDispatchee(listener); }

    void addReadyForServiceListener(ReadyForServiceListener& listener) { mReadyForServiceDispatcher.addDispatchee(listener); }
    void removeReadyForServiceListener(ReadyForServiceListener& listener) { mReadyForServiceDispatcher.removeDispatchee(listener); }
    void executeShutdown(bool restartAfterShutdown, ShutdownReason reason);
    void addDrainListener(DrainListener& listener) { mDrainDispatcher.addDispatchee(listener); }
    void removeDrainListener(DrainListener& listener) { mDrainDispatcher.removeDispatchee(listener); }
    void registerDrainStatusCheckHandler(const char8_t* name, DrainStatusCheckHandler& handler);
    void deregisterDrainStatusCheckHandler(const char8_t* name);

    void connectAndActivateRemoteInstances(const eastl::vector<InetAddress>& remoteAddrList);
    void handleRemoteInstanceInfo(const RemoteInstanceInfo& remoteInstanceInfo, bool isRedirectorUpdate);
    BlazeRpcError checkClusterMembershipOnRemoteInstance(InstanceId remoteInstanceId);

    void addRemoteInstanceListener(RemoteServerInstanceListener& listener) { mRemoteInstanceDispatcher.addDispatchee(listener); }
    void removeRemoteInstanceListener(RemoteServerInstanceListener& listener) { mRemoteInstanceDispatcher.removeDispatchee(listener); }
    void syncRemoteInstanceListener(RemoteServerInstanceListener& listener) const;
    const RemoteServerInstance* getRemoteServerInstance(InstanceId instanceId) const;

    uint64_t getMachineInstanceCount() const;

    BlazeRpcError waitUntilReadyForService();
    BlazeRpcError waitForReconfigureValidation(ConfigureValidationFailureMap& validationErrorMap);
    void setConfigureValidationErrorsForFeature(const char8_t *featureName, ConfigureValidationErrors& validationErrorMap);

    /* \brief Get the debug system control instance */
    DebugSystem* getDebugSys() const { return mDebugSys; }

    const char8_t* getCurrentWorkingDirectory() const { return mCwd; }
    const char8_t* getHostCurrentWorkingDirectory() const { return mHostCwd.empty() ? mCwd : mHostCwd.c_str(); }

    const EA::TDF::TimeValue& getInboundGracefulClosureAckTimeout() const { return mInboundGracefulClosureAckTimeout; }

    uint16_t getConfiguredInstanceTypeCount(const char8_t* typeName) const;
    uint16_t getConfiguredInstanceTypeCount() const { return getConfiguredInstanceTypeCount(getBaseName()); }

    void tickDeprecatedRPCMetric(const char8_t* component, const char8_t* command, const Message* message);

    ExternalServiceMetrics& getExternalServiceMetrics() { return mExternalServiceMetrics; }

    BlazeRpcError enableAuditLogging(const UpdateAuditLoggingRequest &request, bool unrestricted = true);
    BlazeRpcError disableAuditLogging(const UpdateAuditLoggingRequest &request);

    bool isNucleusClientIdInUse(const char8_t* clientId) const;

protected:
    void onSlaveSessionAdded(SlaveSession& session) override;
    void onSlaveSessionRemoved(SlaveSession& session) override;

private:
    enum ComponentStage
    {
        COMPONENT_STAGE_FRAMEWORK = 0,
        COMPONENT_STAGE_MASTER,
        COMPONENT_STAGE_SLAVE,
        COMPONENT_STAGE_MAX // SENTINEL, DO NOT USE
    };

    void pushControllerStateTransition(Job&);
    void popControllerStateTransition();

    void controllerBootstrap();
    
    bool reserveEndpointPorts(uint16_t& basePort); 
    bool initializeConnectionManager(uint16_t basePort);

    bool initializeInboundGrpcManager(uint16_t basePort);
    bool initializeOutboundGrpcManager();
    
    void configureHostedServices();
    void validateHostedServicesConfig(ConfigureValidationErrors& validationErrors) const;
    void validateServiceNameStandard(ConfigureValidationErrors& validationErrors) const;

    //Blaze master notification listener interface
    void onNotifyPrepareForReconfigure(const Blaze::PrepareForReconfigureNotification& data, UserSession*) override;
    void onNotifyPrepareForReconfigureComplete(const Blaze::PrepareForReconfigureCompleteNotification& data, UserSession*) override;
    void onNotifyReconfigure(const ReconfigureNotification& data, UserSession*) override;
    void onSetComponentState(const SetComponentStateRequest& data, UserSession*) override;

    void doOnShutdownBroadcasted(ShutdownRequestPtr data);
    void onShutdownBroadcasted(const ShutdownRequest& data, UserSession*) override;
    void onNotifyControllerStateChange(const ControllerStateChangeNotification& data, UserSession*) override;
    void onNotifyLoadChange(const InstanceLoadChangeNotification& data, UserSession*) override;

    void internalPrepareForReconfigure();
    void internalReconfigure(bool allowBlocking);
    void reportReconfigureFinished();
    void internalReconfigureController();
    void configureShutdownSettings();

    void shutdownControllerInternal(ShutdownReason shutdownReason);
    void shutdownComponents();
    void destroyComponents();

    typedef eastl::vector<ComponentId> ComponentIdList;
    bool executeComponentStateChange(const ComponentIdList& components, ComponentStub::State desiredState, void (ComponentStub::*func)(), const char8_t *methodName, const EA::TDF::TimeValue& timeoutTime = EA::TDF::TimeValue());
    bool executeBlockingComponentMethod(const ComponentIdList& components, void (ComponentStub::*func)(), const char8_t* methodName, 
        ComponentStage stageFilter = COMPONENT_STAGE_MAX, const EA::TDF::TimeValue& timeoutTime = EA::TDF::TimeValue());
    void scheduleComponentStateChangeTimeout(ComponentStage stage, ComponentStub::State desiredState, TimerId& outTimerId);
    void executeComponentStateChangeTimeout(ComponentStage stage, ComponentStub::State desiredState, TimerId& outTimerId);
    
    void transitionControllerState(ComponentStub::State desiredState);

    // RemoteServerInstanceHandler interface
    void onRemoteInstanceStateChanged(RemoteServerInstance& instance) override;

    // Remote instances management
    bool isOurInternalEndpointAddress(const InetAddress& addr);
    void connectAndActivateInstance(const InetAddress& addr);
    void connectAndActivateInstanceInternal(InetAddress addr);
    void shutdownAndRemoveInstance(InetAddress addr, bool removeConnected);

    void updateInServiceStatus();
    void getStatusInfo(ComponentStatus& status) const override;

    GetInstanceInfoError::Error processGetInstanceInfo(GetInstanceInfoResponse &response, const Message *message) override;
    CheckClusterMembershipError::Error processCheckClusterMembership(const CheckClusterMembershipRequest &req, const Message *message) override;
    GetStatusError::Error processGetStatus(ServerStatus &response, const Message *message) override;
    GetComponentStatusError::Error processGetComponentStatus(const ComponentStatusRequest &request, ComponentStatus &response, const Message *message) override;
    GetConnMetricsError::Error processGetConnMetrics(const GetConnMetricsRequest &request, GetConnMetricsResponse &response, const Message* message) override;
    GetComponentMetricsError::Error processGetComponentMetrics(
        const ComponentMetricsRequest& request, ComponentMetricsResponse &response, const Message*) override;
    ConfigureMemoryMetricsError::Error processConfigureMemoryMetrics(
        const Blaze::ConfigureMemoryMetricsRequest &request, Blaze::ConfigureMemoryMetricsResponse &response, const Message* message) override;
    GetMemoryMetricsError::Error processGetMemoryMetrics(
        const Blaze::GetMemoryMetricsRequest &request, Blaze::GetMemoryMetricsResponse &response, const Message* message) override;
    SetServiceStateError::Error processSetServiceState(
        const Blaze::SetServiceStateRequest &request, const Message*) override;
    DumpCoreFileError::Error processDumpCoreFile(
        const Blaze::DumpCoreFileRequest& request, const Message*) override;    
    GetFiberTimingsError::Error processGetFiberTimings(Blaze::FiberTimings &response, const Message*) override;
    GetDbQueryMetricsError::Error processGetDbQueryMetrics(Blaze::DbQueryMetrics &response, const Message*) override;
    ShutdownError::Error processShutdown(const Blaze::ShutdownRequest &request, const Message* message) override;
    GetOutboundMetricsError::Error processGetOutboundMetrics(Blaze::OutboundMetrics &response, const Message* message) override;
    GetDrainStatusError::Error processGetDrainStatus(Blaze::DrainStatus &response, const Message* message) override;
    GetPSUByClientTypeError::Error processGetPSUByClientType(const Blaze::GetMetricsByGeoIPDataRequest &request, Blaze::GetPSUResponse &response, const Message* message) override;
    GetMetricsError::Error processGetMetrics(const Blaze::GetMetricsRequest &request, Blaze::GetMetricsResponse &response, const Message* message) override;
    GetMetricsSchemaError::Error processGetMetricsSchema(Blaze::GetMetricsSchemaResponse &response, const Message* message) override;

    // Census data
    GetCensusDataProvidersError::Error processGetCensusDataProviders(GetCensusDataProvidersResponse &response, const Message *message) override;

    static ComponentStage getStage(ComponentId componentId);
    static const char8_t *getStageName(ComponentStage stage);

    void addPendingConfigFeatures(const ConfigFeatureList& list);
    virtual bool onValidateConfig(FrameworkConfig& config, const FrameworkConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const;
    
    void updateRemoteInstanceLatencies();
    void clearFiberTimingData();
    void checkCpuBudget();

    void checkClientTypeBindings();

    void transactionContextCleanSweep();
    void logAndExportMetrics();
    void exportOIMetrics();
    void scheduleMetricsTimer();
    void scheduleOIMetricsTimer();
    void certRefresh();
    void scheduleCertRefreshTimer();

    bool validateTrialNameRemaps(const RedirectorSettingsConfig &config, ConfigureValidationErrors &validationErrors) const;

    void checkDrainStatus(bool restartAfterShutdown);

    void onNotifyUpdateAuditState(UserSession*) override;
    void fetchAndUpdateAuditState();
    void deleteExpiredAuditEntries();
    void scheduleExpireAuditEntries(const EA::TDF::TimeValue& deadline);

    void executeDrainShutdown(bool restartAfterShutdown);

private:
    friend class MasterController;
    friend class Component;
    friend class ServerRunnerThread;

    // Number of milliseconds between state change logs
    static const uint32_t STATE_CHANGE_TIMEOUT = 15000;
    // Number of milliseconds to wait and allow components to gracefully transition to the SHUTDOWN state
    // will ignore component states and proceed with forceful shutdown if this timeout is expired
    static const uint32_t SHUTDOWN_STATE_CHANGE_TIMEOUT = 30000;
    // Number of milliseconds to wait for remote instance cleanup
    static const uint32_t REMOTE_INSTANCE_SHUTDOWN_TIMEOUT = 100;

    // Number of milliseconds between service status updates
    static const uint32_t UPDATE_SERVICE_STATUS_INTERVAL = 1000;
    // Number of milliseconds between OI metrics exports
    static const uint32_t EXPORT_OI_METRICS_INTERVAL = 1000;

    static const uint32_t REMOTE_SERVER_INSTANCE_MAP_SIZE = 127;
    typedef eastl::intrusive_hash_map<InstanceId, RemoteServerInstance, REMOTE_SERVER_INSTANCE_MAP_SIZE> RemoteInstanceByIdMap;
    typedef eastl::map<InetAddress, RemoteServerInstance*> RemoteInstanceByAddrMap;
    
    typedef eastl::vector_set<eastl::string> ConfigFeatureSet;
    typedef eastl::vector<Fiber::EventHandle> EventHandleList;
    typedef eastl::hash_map< const char8_t*, uint16_t, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > UInt16ByString;

    UInt16ByString mConfiguredCountByInstanceType;
    ComponentManager& mComponentManager; // possibly move to ServerThread
    eastl::unique_ptr<Grpc::InboundGrpcManager> mInboundGrpcManager;
    eastl::unique_ptr<Grpc::OutboundGrpcManager> mOutboundGrpcManager;

    RemoteInstanceByIdMap mRemoteInstancesById;
    RemoteInstanceByAddrMap mRemoteInstancesByAddr;
    
    ConfigFeatureSet mPendingFeatures;

    const BootConfig& mBootConfigTdf;
    ServerConfig mServerConfigTdf;
    FrameworkConfig mFrameworkConfigTdf;
    FrameworkConfigPtr mPendingFrameworkConfigTdf;
    LoggingConfig mLoggingConfigTdf;
    LoggingConfig mPendingLoggingConfigTdf;

    // the configuration potentially contains more service names than will be enabled 
    // for this Blaze cluster- we cache the set of enabled ones here so the set doesn't have to be reconstructed on every call
    // to look up service names
    ServiceNameInfoMap mHostedServicesInfo;
    ClientPlatformSet mHostedPlatforms;

    InstanceInfoController& mInstanceInfoController;
    ConnectionManager& mConnectionManager;
    OutboundConnectionManager& mOutboundConnectionManager;

    PasUtil mPasUtil;
    uint16_t mBasePort;
    char8_t mBaseName[64];
    char8_t mInstanceName[64];
    InstanceId mInstanceId;
    SlaveSessionId mInstanceSessionId;
    //0 for all instances other than AUX_SLAVE. All AUX_SLAVE should have the same value for their instance type across the cluster.
    uint8_t mInstanceTypeIndex; 
    uint8_t mSlaveTypeCount;

    NamespacesList mSupportedNamespaceList;

    Fiber::FiberGroupId mTaskGroupId;
    Fiber::FiberGroupId mChangeGroupId;

    bool mBootstrapComplete;
    bool mIsDraining;
    bool mShutdownPending;
    bool mShuttingDown;
    bool mInService;
    bool mIsReadyForService;
    bool mIsStartInService;
    uint32_t mInServiceCheckCounter;
    Fiber::WaitList mReadyForServiceWaiters;

    Selector& mSelector;

    EA::TDF::TimeValue mStartupTime;
    EA::TDF::TimeValue mConfigReloadTime;
    EA::TDF::TimeValue mDrainStartTime;

    Metrics::PolledGauge mUptimeMetric;
    Metrics::PolledGauge mConfigUptimeMetric;
    Metrics::PolledGauge mInServiceMetric; // isInService()
    Metrics::PolledGauge mIsDrainingMetric; // isDraining()

    Metrics::TaggedCounter<const char8_t*, const char8_t*, ClientType, const char8_t*, InetAddress> mDeprecatedRPCs;

    Metrics::PolledGauge mProcessResidentMemory;

    typedef eastl::list<Job*> StateTransitionJobQueue;
    StateTransitionJobQueue mStateTransitionJobQueue;
    bool mExecutingStateTransition;
    EA::Thread::Mutex mStateTransitionMutex;
    Dispatcher<StateChangeListener> mStateChangeDispatcher;
    Dispatcher<ReadyForServiceListener> mReadyForServiceDispatcher;
    Dispatcher<DrainListener> mDrainDispatcher;
    Dispatcher<RemoteServerInstanceListener> mRemoteInstanceDispatcher;

    struct ReconfigureSlaveState
    {
        SemaphoreId mSemaphoreId;
        bool mReconfigureInProgress;
        ConfigureValidationFailureMap mValidationErrors;

        ReconfigureSlaveState(): mSemaphoreId(0), mReconfigureInProgress(false) {}
    };
    ReconfigureSlaveState mReconfigureState;

    ServerEvent mServerEventData;

    TimerId mAuditEntryExpiryTimerId;

    TimerId mMetricsTimerId;
    TimerId mOIMetricsTimerId;
    EA::TDF::TimeValue mNextMetricsLogTime;
    EA::TDF::TimeValue mNextMetricsExportTime;
    TimerId mCertRefreshTimerId;
    DebugSystem *mDebugSys;

    char8_t mCwd[256];
    char8_t mProcessOwner[64];
    eastl::string mHostCwd;
    eastl::string mGrpcUserAgent;

    typedef eastl::hash_map<eastl::string, DrainStatusCheckHandler*> DrainStatusCheckHandlersByName;
    DrainStatusCheckHandlersByName mDrainStatusCheckHandlers;
    TimerId mDrainTimerId;
    bool mDrainComplete;
    uint32_t mLastBroadcastLoad;
    uint32_t mLastBroadcastSystemLoad;

    EA::TDF::TimeValue mInboundGracefulClosureAckTimeout;

    Fiber::EventHandle mDelayedDrainEventHandle;
    FiberJobQueue mUpdateAuditStateQueue;
    ExternalServiceMetrics mExternalServiceMetrics;
};

extern EA_THREAD_LOCAL Controller* gController;

} // Blaze

#endif // BLAZE_CONTROLLER_H
