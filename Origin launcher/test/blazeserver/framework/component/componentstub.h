/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COMPONENT_STUB_H
#define BLAZE_COMPONENT_STUB_H

/*** Include files *******************************************************************************/
#include "blazerpcerrors.h"
#include "framework/component/blazerpc.h"
#include "framework/tdf/slivermanagertypes_server.h"
#include "EASTL/vector_set.h"
#include "EASTL/set.h"
#include "framework/blazedefines.h"
#include "framework/connection/session.h"
#include "EATDF/time.h"
#include "EATDF/tdfobjectid.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Component;
class ComponentStatus;
struct CommandMetrics;
struct CommandMetricsHistory;
class ConfigureValidationErrors;
class ComponentMetricsResponse;
struct CommandInfo;
struct RpcCallOptions;
class InboundRpcConnection;
class PeerInfo;
namespace Metrics { class MetricsCollection; }

typedef eastl::set<ClientPlatformType> ClientPlatformSet;

class ComponentStub : protected SlaveSessionListener
{
    NON_COPYABLE(ComponentStub);
public:
 /*! The various states a component can be in. 
        IMPORTANT! Ensure changes to this enum
        are propagated to sStateNames[] array! 
        
        All the components go in the state in the order below except the controller component. Controller component first goes in RESOLVING state and
        then CONFIGURING. 
        */

    typedef enum
    {
        INIT = 0,               // Initial stage for any component
        CONFIGURING,            // <ComponentImpl>::onConfigure is called by framework code
        CONFIGURED,             // <ComponentImpl>::onConfigure has finished executing and control is back to the framework code
        RESOLVING,              // <ComponentImpl>::onResolve is called by framework code. The Master counterpart of the component is ACTIVE (if required by the component).
        RESOLVED,               // <ComponentImpl>::onResolve has finished executing and control is back to the framework code
        CONFIGURING_REPLICATION,// DEPRECATED Functionality
        CONFIGURED_REPLICATION, // DEPRECATED Functionality
        ACTIVATING,             // <ComponentImpl>::onActivate is called by framework code. 
        ACTIVE,                 // <ComponentImpl>::onActivate has finished executing. The component is active and ready to be used!
        SHUTTING_DOWN,          // <ComponentImpl>::onShutdown is called by framework code. Clean up time. Note that a component must handle drain functionality for safeguarding against 
                                // operations that rely on Network functionality instead of handing via onShutdown (Network stack is tore down by the time this state kicks in).
        SHUTDOWN,               // <ComponentImpl>::onActivate has finished executing. The component is active and ready to be used!
        ERROR_STATE,            // An error occured. Time to bail out.
        INVALID_STATE,          // Currently unused. 

        STATE_COUNT //sentinel value, do not use
    } State;

    typedef eastl::vector_set<uint16_t> ReplicatedMapTypes;

    ~ComponentStub() override;

    virtual Component& asComponent() = 0;
    virtual const Component& asComponent() const = 0;

    const ComponentData& getComponentData() const { return mComponentInfo; }

    //These are frequently called enough that its worth just exposing them here.
    EA::TDF::ComponentId getComponentId() const { return mComponentInfo.id; }
    const char8_t* getComponentName() const { return mComponentInfo.name; }

    BlazeRpcError executeLocalCommand(const CommandInfo& cmdInfo, const EA::TDF::Tdf* request, EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf,
        const RpcCallOptions& options, Message* origMsg, bool obfuscateResponse);

    /*! \brief Gets the current state of the component.  */
    State getState() const { return mState; }    

    /*! \brief Returns string name of a particular component state value.*/
    static const char8_t* getStateName(State state);

    BlazeRpcError waitForState(State state, const char8_t* context, EA::TDF::TimeValue absoluteTimeout = EA::TDF::TimeValue());

    /*! \brief Gets the metrics for this component.
     *  \param size The size of the array is returned in this parameter
     *  \return Array of metrics info (one per command supported by the component)
     */
    void tickMetrics(const PeerInfo* peerInfo, const CommandInfo& id, const EA::TDF::TimeValue& accumulatedTime, BlazeRpcError err);
    void tickRpcAuthorizationFailureCount(const PeerInfo& peerInfo, const CommandInfo& info);
    void gatherMetrics(ComponentMetricsResponse& metrics, CommandId specificCommand = RPC_COMMAND_UNKNOWN) const;

     /*! \brief Gets the interval-based historic metrics for this component.
     *  \param cmdID command ID to of RPC to get metrics history.
     *  \param bucketIndex Index representing which interval of historic metrics to get
     *  \return Historic metrics data for the specified interval.  nullptr if index out of range.
     */
    virtual CommandMetricsHistory* getCommandMetricsHistory(const uint32_t metricIndex, const uint32_t intervalIndex) const { return nullptr; }

    /*! \brief Populate the given status object with the component's current status.
     */
    virtual void getStatusInfo(ComponentStatus& status) const { };

        /*! \brief Returns schema version of a particular component 0 by default.*/
    virtual uint16_t getDbSchemaVersion() const { return 0; }

    /*! \brief Returns the content schema version of a particular component 0 by default.*/
    virtual uint32_t getContentDbSchemaVersion() const { return 0; }

    /*! This api is now deprecated in favor of getDbNameByPlatformTypeMap which works for both the shared as well as single platform deployments. 
    getDbName only works for the single platform deployments. */
    virtual const char8_t* getDbName() const { return ""; }
    
    /*! \brief Returns the name(s) of the dbs the component uses for each platform.
    Components should override this in such a way that it returns a valid
    value _before_ onConfigure().

    This api replaces deprecated getDbName() api.
    */
    virtual const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const { return nullptr; }

    /*! \brief Returns the map of dbIds per platform for a particular component
        (populated on component activation - after onPreconfigure() and before onConfigure() - using db names from getDbNameByPlatformTypeMap()).
    */
    const DbIdByPlatformTypeMap& getDbIdByPlatformTypeMap() const { return mDbIdByPlatformTypeMap; }
    
    /*! \brief Returns the dbId of a component on a particular platform
    */
    uint32_t getDbId(ClientPlatformType platform = INVALID) const;

    /*! \brief For shared platform deployments only.
 
        Returns true if the component requires an entry in getDbNameByPlatformTypeMap() for the given platform, if hosted.
        Components should override this in such a way that it returns a valid value _before_ onConfigure().

        This method is used to prevent components from starting up without valid dbconfigs for all required schemas.
        It is not necessary for components to implement their own validation of getDbNameByPlatformTypeMap() in onConfigure(),
        either on shared platform or single-cluster deployments.
    */
    virtual bool requiresDbForPlatform(ClientPlatformType platform) const { return platform == ClientPlatformType::common; }

    /*! \brief Tells component if it is enabled or not */
    virtual bool isEnabled() const { return mEnabled; }

    /*! \brief Override in components that don't want to handle RPCs until the instance is in service */
    virtual bool getHoldIncomingRpcsUntilReadyForService() const { return false; }

    /*! \brief Provides overrided return code to pass back when component disabled  */
    virtual BlazeRpcError getDisabledReturnCode() const { return mDisableErrorReturnCode; }

    /*! \brief For shared platform deployments only.

        This method is called to obfuscate 1st-party identifiers in notifications and RPC responses sent
        down to clients, for notifications and RPCs that set the obfuscate_platform_info option.

        Since only a small number of Tdf notification and RPC response classes include 1st-party identifiers,
        we handle them directly rather than use a generic TdfVisitor (which is inefficient).
    */
    virtual void obfuscatePlatformInfo(ClientPlatformType platform, EA::TDF::Tdf& tdf) const { }

    /*! \brief 
        For events configured with a non-empty eventConfig.messageTypesByComponentEvent.<componentname>.platforms list,
        this method is invoked by the EventManager to determine whether the given event should be submitted to the Notify service.

        \return true if the notification's platform is in the ClientPlatformSet (or if the notification should be submitted regardless of platform)
    */
    virtual bool shouldSendNotifyEvent(const ClientPlatformSet& platformSet, const Notification& notification) const { return true; }

    const EA::TDF::Tdf& getConfigTdf() const { return *mConfiguration; }
    const EA::TDF::Tdf& getPreconfigTdf() const { return *mPreconfiguration; }

    /*! \brief Returns true if this component is a reconfigurable component, i.e. its config is tdf-based. */
    bool isReconfigurable() const { return mConfiguration != nullptr; }

    void addNotificationSubscription(SlaveSession& session);
    const ReplicatedMapTypes& getPartialReplicationTypes() const { return mPartialReplicationTypes; }

    Metrics::MetricsCollection& getMetricsCollection() { return mMetricsCollection; }

protected:
    friend class Controller;

    ComponentStub(const ComponentData& componentInfo);

    //State change functions
    virtual void activateComponent();
    virtual void prepareForReconfigure();
    virtual void reconfigureComponent();
    virtual void shutdownComponent();

    //Startup sequence callbacks - intended for user override
    virtual bool loadConfig(EA::TDF::TdfPtr& newConfig, bool loadFromStaging);
    virtual bool loadPreconfig(EA::TDF::TdfPtr& newConfig);

    //caught by the stub, which has a specialized version to override for each component implementation
    virtual bool onPreconfigure() { return true; }
    virtual bool onValidatePreconfigTdf(EA::TDF::Tdf& preconfig, const EA::TDF::Tdf* referenceConfigPtr, ConfigureValidationErrors& validationErrors) { return true; }
    virtual bool onValidateConfigTdf(EA::TDF::Tdf& config, const EA::TDF::Tdf* referenceConfigPtr, ConfigureValidationErrors& validationErrors) { return true; }
    virtual bool onConfigure() { return true; }
    virtual bool onResolve() { return true; }
    virtual bool onConfigureReplication() { return true; }
    virtual bool onActivate() { return true; }
    virtual bool onCompleteActivation() { return true; }

    //Reconfigure callbacks
    virtual bool onPrepareForReconfigureComponent(const EA::TDF::Tdf& newConfig) { return true; }
    virtual bool onReconfigure() { return true; }
    
    /*! ***************************************************************************/
    /*!    \brief Overridable function to handle a new slave session connecting.
    
        \param session  The new session
    *******************************************************************************/
    virtual void onSlaveSessionAdded(SlaveSession& session) {}
    void onSlaveSessionRemoved(SlaveSession& session) override {}

    // Notification helpers
    void sendNotificationToSliver(SliverIdentity sliverIdentity, Notification& notification) const;
    void sendNotificationToSliver(const NotificationInfo& notificationInfo, SliverIdentity sliverIdentity, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const;

    void sendNotificationToSlaves(const NotificationInfo& notificationInfo, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts, bool notifyLocal);

    template<typename InputIterator, typename SessionFetchFunction>
    void sendNotificationToSlaveSessions(const NotificationInfo& notificationInfo, InputIterator first, InputIterator last, SessionFetchFunction fetchFunction, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const;
    void sendNotificationToSlaveSession(const NotificationInfo& notificationInfo, SlaveSession* slaveSession, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const;
    void sendNotificationToSlaveSession(SlaveSession* slaveSession, Notification& notification) const;

    template<typename InputIterator, typename SessionFetchFunction>
    void sendNotificationToUserSessions(const NotificationInfo& notificationInfo, InputIterator first, InputIterator last, SessionFetchFunction fetchFunction, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const;
    void sendNotificationToUserSession(const NotificationInfo& notificationInfo, UserSessionMaster* userSession, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const;
    void sendNotificationToUserSession(UserSessionMaster* userSession, Notification& notification) const;

    template<typename InputIterator, typename SessionIdFetchFunction>
    void sendNotificationToUserSessionsById(const NotificationInfo& notificationInfo, InputIterator first, InputIterator last, SessionIdFetchFunction fetchFunction, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const;
    void sendNotificationToUserSessionById(const NotificationInfo& notificationInfo, UserSessionId userSessionId, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const;
    void sendNotificationToUserSessionById(UserSessionId userSessionId, Notification& notification) const;

    template<typename InputIterator, typename InstanceIdFetchFunction>
    void sendNotificationToInstancesById(const NotificationInfo& notificationInfo, InputIterator first, InputIterator last, InstanceIdFetchFunction fetchFunction, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const;
    void sendNotificationToInstanceById(const NotificationInfo& notificationInfo, InstanceId instanceId, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const;
    void sendNotificationToInstanceById(InstanceId instanceId, Notification& notification) const;

    virtual void onShutdown() {}

    virtual void setEnabled(bool enable) { mEnabled = enable; }

    void setDisableErrorReturnCode(BlazeRpcError code) { mDisableErrorReturnCode = code; }
    void changeStateAndSignalWaiters(State state);
    void setPartialReplication(uint16_t replMapType) { mPartialReplicationTypes.insert(replMapType); }

    struct LocalCommandParams
    {
        const CommandInfo& mCmdInfo;
        const EA::TDF::Tdf* mRequest;
        EA::TDF::Tdf* mResponseTdf;
        EA::TDF::Tdf* mErrorTdf;
        const RpcCallOptions& mOptions;
        bool mObfuscateResponse;
        Message* mRpcContextMsg;
        UserSessionId mUserSessionId;
        bool mSuperUserPrivilege;

        LocalCommandParams(const CommandInfo& cmdInfo, const EA::TDF::Tdf* request, EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf,
                const RpcCallOptions& options, bool obfuscateResponse, Message* rpcContextMsg, UserSessionId userSessionId, bool superUserPrivilege)
            : mCmdInfo(cmdInfo)
            , mRequest(request)
            , mResponseTdf(responseTdf)
            , mErrorTdf(errorTdf)
            , mOptions(options)
            , mObfuscateResponse(obfuscateResponse)
            , mRpcContextMsg(rpcContextMsg)
            , mUserSessionId(userSessionId)
            , mSuperUserPrivilege(superUserPrivilege)
        {
        }
    };

    BlazeRpcError processLocalCommandInternal(LocalCommandParams* params);

    BlazeRpcError executeLocalCommandInternal(const CommandInfo& cmdInfo, const EA::TDF::Tdf* request, EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf,
        const RpcCallOptions& options, Message* origMsg, bool obfuscateResponse);

    const ComponentData& mComponentInfo;
    Blaze::Allocator& mAllocator;
    Component *mMaster;
    State mState;
    SlaveSessionList mSlaveList;
private:
    static void sendNotificationToUserSessionById(); /*const NotificationInfo& info, AsyncNotificationPtr& notification, UserSessionId userSessionId, const EA::TDF::Tdf* payload, int32_t logCategory, const NotificationSendOpts& opts);*/
    const char8_t* getStubBaseName() const { return mComponentInfo.baseInfo.name; }
    size_t getStubLogCategory() const { return mComponentInfo.baseInfo.index; }
    MemoryGroupId getStubMemGroupId() const { return mComponentInfo.baseInfo.memgroup; }
    Blaze::Allocator& getStubAllocator() const { return mAllocator; }
    bool isMasterStub() const { return RpcIsMasterComponent(mComponentInfo.id); }
    bool hasMasterComponentStub() const { return mComponentInfo.hasMaster; }
    bool requiresMasterComponent() const { return mComponentInfo.requiresMasterComponent; }
    bool hasComponentReplicationStub() const { return mComponentInfo.hasReplication; }
    bool hasComponentNotificationsStub() const { return mComponentInfo.hasNotifications; }
    static void logRpcPreamble(const CommandInfo& cmdInfo, const Message* origMsg, const EA::TDF::Tdf* request);
    static void logRpcPostamble(BlazeRpcError rc, const CommandInfo& cmdInfo, const Message* origMsg, const EA::TDF::Tdf* errorTdf);

    bool mEnabled;
    BlazeRpcError mDisableErrorReturnCode;

    EA::TDF::TdfPtr mPendingConfiguration; 
    EA::TDF::TdfPtr mConfiguration; 
    EA::TDF::TdfPtr mPreconfiguration;

    typedef eastl::vector<EA::TDF::TdfPtr> ConfigList;
    ConfigList mOldConfigs;
    ReplicatedMapTypes mPartialReplicationTypes;

    typedef eastl::map<State, Fiber::WaitList> WaitListByStateMap; // This must be a map, not a vector_map because Fiber::WaitList is *deliberately* non-copyable
    WaitListByStateMap mStateWaitLists;

    Metrics::MetricsCollection& mMetricsCollection;

    CommandMetrics** mMetricsAllTime;
    DbIdByPlatformTypeMap mDbIdByPlatformTypeMap;
};


template<typename InputIterator, typename SessionFetchFunction>
void ComponentStub::sendNotificationToSlaveSessions(const NotificationInfo& notificationInfo, InputIterator first, InputIterator last, 
                                                    SessionFetchFunction fetchFunction, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const
{
    Notification notification(notificationInfo, payload, opts);
    for (; first != last; ++first)
    {
        SlaveSession* slaveSession = fetchFunction(*first);
        sendNotificationToSlaveSession(slaveSession, notification);
    }
}

template<typename InputIterator, typename SessionFetchFunction>
void ComponentStub::sendNotificationToUserSessions(const NotificationInfo& notificationInfo, InputIterator first, InputIterator last, 
                                                   SessionFetchFunction fetchFunction, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const
{
    Notification notification(notificationInfo, payload, opts);
    for (; first != last; ++first)
    {
        UserSessionMaster* userSession = fetchFunction(*first);
        sendNotificationToUserSession(userSession, notification);
    }
}

template<typename InputIterator, typename SessionIdFetchFunction>
void ComponentStub::sendNotificationToUserSessionsById(const NotificationInfo& notificationInfo, InputIterator first, InputIterator last, 
                                                       SessionIdFetchFunction fetchFunction, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const
{
    Notification notification(notificationInfo, payload, opts);
    for (; first != last; ++first)
    {
        UserSessionId userSessionId = fetchFunction(*first);
        sendNotificationToUserSessionById(userSessionId, notification);
    }
}

template<typename InputIterator, typename InstanceIdFetchFunction>
void ComponentStub::sendNotificationToInstancesById(const NotificationInfo& notificationInfo, InputIterator first, InputIterator last, 
                                                    InstanceIdFetchFunction fetchFunction, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const
{
    Notification notification(notificationInfo, payload, opts);
    for (; first != last; ++first)
    {
        InstanceId instanceId = fetchFunction(*first);
        sendNotificationToInstanceById(instanceId, notification);
    }
}


} // Blaze

#endif // BLAZE_COMPONENT_STUB_H

