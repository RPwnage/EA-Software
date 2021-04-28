/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MASTER_CONTROLLER_H
#define BLAZE_MASTER_CONTROLLER_H

/*** Include files *******************************************************************************/

#include "framework/rpc/blazecontrollermaster_stub.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/util/expression.h"
#include "framework/tdf/frameworkconfigtypes_server.h"

#include "EASTL/intrusive_list.h"
#include "EASTL/vector_map.h"
#include "EASTL/string.h"
#include "EASTL/set.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConfigBootUtil;
class ConfigFile;
class Selector;

class ComponentInfo;
class ResolveRequest;

class MasterController
    : public BlazeControllerMasterStub
{
public:
    enum ReconfigState 
    { 
        RECONFIGURE_STATE_READY = 0,
        RECONFIGURE_STATE_PREPARING,
        RECONFIGURE_STATE_PREPARED,
        RECONFIGURE_STATE_RECONFIGURING, 
        RECONFIGURE_STATE_RECONFIGURED
    };
    
    MasterController();
    ~MasterController() override;

    void initialize();

private:
    GetServerConfigMasterError::Error processGetServerConfigMaster(const GetServerConfigRequest& request, GetServerConfigResponse &response, const Message*) override;

    ReconfigureMasterError::Error processReconfigureMaster(const Blaze::ReconfigureRequest &request, const Message*) override;
       
    ReconfiguredMasterError::Error processReconfiguredMaster(const Message*) override;
    
    SetComponentStateMasterError::Error processSetComponentStateMaster(const Blaze::SetComponentStateRequest &request, const Message*) override;

    RunDbMigError::Error processRunDbMig(const Blaze::RunDbMigInfo &request, const Message*) override;

    GetConfigMasterError::Error processGetConfigMaster(const Blaze::ConfigRequest &request, Blaze::ConfigResponse &response, const Message*) override;

    PreparedForReconfigureMasterError::Error processPreparedForReconfigureMaster(const Blaze::PreparedForReconfigureRequest &request, const Message* ) override;

    GetConfigFeatureListMasterError::Error processGetConfigFeatureListMaster(Blaze::ReconfigurableFeatures &response, const Message* message) override;

    RunDbMigError::Error runDbMigOnDb(uint32_t dbId, const Blaze::RunDbMigInfo &request, ClientPlatformType platform);

    void onSlaveSessionAdded(SlaveSession& session) override;
    void onSlaveSessionRemoved(SlaveSession& session) override;

private:
    class ControllerInfo : public eastl::intrusive_list_node
    {
        NON_COPYABLE(ControllerInfo);
    public:
        ControllerInfo(SlaveSessionId slaveSessionId);
        void setAddress(const InetAddress& addr);
        SlaveSessionId getSlaveSessionId() const { return mSlaveSessionId; }
        const char8_t* toString(char8_t* buf, size_t len) const;
        void setReconfigState(MasterController::ReconfigState state) { mReconfigState = state; }
        MasterController::ReconfigState getReconfigState() const { return mReconfigState; }

    private:
        SlaveSessionId mSlaveSessionId;
        InetAddress mAddress;
        MasterController::ReconfigState mReconfigState;
    };
    
    struct FeatureConfigInfo
    {
        FeatureConfigInfo(ConfigFile* file);
        ~FeatureConfigInfo();

        ConfigFile* mConfigData;
    };
    
    PreparedForReconfigureMasterError::Error processPreparedForReconfigureMaster(const Blaze::PreparedForReconfigureRequest &request, SlaveSessionId incomingSessionId);
    ReconfiguredMasterError::Error processReconfigured(SlaveSessionId slaveSessionId);
    FeatureConfigInfo* loadConfigInternal(const char8_t* feature, bool reload=false, bool preconfig=false);
    ControllerInfo *getControllerInfoBySessionId(SlaveSessionId id);

private:
    typedef eastl::vector_map<eastl::string, FeatureConfigInfo*> FeatureConfigMap;
    typedef eastl::vector<eastl::string> FeatureConfigList;
    typedef eastl::hash_set<SlaveSessionId> ReconfiguringSlaveSessionIdSet;
    
    ReconfigState mReconfigState;

    const ConfigBootUtil& mBootUtil; // not owned
    FeatureConfigMap mFeatureConfigMap;
    FeatureConfigMap mFeaturePreconfigMap;
    FeatureConfigMap mStagingFeatureConfigMap;
    FeatureConfigList mFailedFeatureList;

    ReconfiguringSlaveSessionIdSet mReconfiguringSlaveSessionIds;

    ConfigFeatureList mReconfigureFeatureList;
    ConfigureValidationFailureMap mReconfigureValidationFailureMap;
    bool mReconfigureValidateOnly;
    
    typedef eastl::intrusive_list<ControllerInfo> ControllerList;
    ControllerList mSlaveControllers;

    typedef eastl::set<const char8_t*, eastl::str_less<const char8_t*> > FeatureNameSet;
    FeatureNameSet mFeatureNameSet;

    FiberMutex mDbMigFiberMutex;
};


} // Blaze

#endif // BLAZE_MASTER_CONTROLLER_H
