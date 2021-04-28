/*************************************************************************************************/
/*!
    \file 


    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/config/config_file.h"
#include "framework/config/config_boot_util.h"
#include "framework/component/blazerpc.h"
#include "blazerpcerrors.h"

#include "eathread/eathread_storage.h"
#include "EAIO/EAFileUtil.h"

#include "framework/controller/mastercontroller.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"

#include "framework/connection/inboundrpcconnection.h"

#include "framework/tdf/controllertypes_server.h"

#include "framework/connection/socketchannel.h"

#include "framework/database/dbscheduler.h"

#include "framework/replication/replicatedmap.h"
#include "framework/usersessions/usersession.h"

#include "framework/util/expression.h"
#include "framework/system/shellexec.h"


namespace Blaze
{

BlazeControllerMaster* BlazeControllerMaster::createImpl()
{
     return BLAZE_NEW_NAMED("MasterController") MasterController();
}


MasterController::MasterController()
    : mReconfigState(MasterController::RECONFIGURE_STATE_READY),
      mBootUtil(gProcessController->getBootConfigUtil()),
      mReconfigureValidateOnly(false)
{
    mState = ACTIVE;
    
    // we add the local controller here in order to enable local reconfigure
    ControllerInfo& info = *(BLAZE_NEW ControllerInfo(gController->getInstanceSessionId()));
    mSlaveControllers.push_back(info);
    char8_t buf[256];
    BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController].ctor: added local controller: " 
        << info.toString(buf, sizeof(buf)));

    initialize();
}

MasterController::~MasterController()
{
    for (FeatureConfigMap::iterator i = mFeaturePreconfigMap.begin(), e = mFeaturePreconfigMap.end(); i != e; ++i)
    {
        delete i->second;
    }

    for (FeatureConfigMap::iterator i = mFeatureConfigMap.begin(), e = mFeatureConfigMap.end(); i != e; ++i)
    {
        delete i->second;
    }

    for (FeatureConfigMap::iterator i = mStagingFeatureConfigMap.begin(), e = mStagingFeatureConfigMap.end(); i != e; ++i)
    {
        delete i->second;
    }
    
    while (!mSlaveControllers.empty())
    {
        ControllerInfo& info = mSlaveControllers.front();
        mSlaveControllers.pop_front();
        delete &info;
    }
}

void MasterController::initialize()
{
    mFeatureNameSet.insert("framework");
    mFeatureNameSet.insert("logging");
    mFeatureNameSet.insert("ssl");
    mFeatureNameSet.insert("vault");

    BootConfig::ComponentGroupsMap::const_iterator it = mBootUtil.getConfigTdf().getComponentGroups().begin();
    BootConfig::ComponentGroupsMap::const_iterator end = mBootUtil.getConfigTdf().getComponentGroups().end();
    for ( ; it != end; ++it)
    {
        const BootConfig::ComponentGroup* group = it->second;
        BootConfig::ComponentGroup::StringComponentsList::const_iterator compIt = group->getComponents().begin();
        BootConfig::ComponentGroup::StringComponentsList::const_iterator compEnd = group->getComponents().end();
        for ( ; compIt != compEnd; ++compIt)
        {
            ComponentId id = BlazeRpcComponentDb::getComponentIdByName(*compIt);

            // Unknown components should already be flagged as fatal errors in Controller::controllerBootstrap,
            // no need to spam additional errors here
            if (id == RPC_COMPONENT_UNKNOWN)
                continue;

            // The slave component name is synonymous with the feature name, don't need master names
            if (!RpcIsMasterComponent(id))
                mFeatureNameSet.insert(*compIt);
        }
    }
}

GetServerConfigMasterError::Error MasterController::processGetServerConfigMaster(
        const GetServerConfigRequest& request, 
        GetServerConfigResponse &response,
        const Message* message)
{
    BootConfig::ServerConfigsMap::const_iterator svr = mBootUtil.getConfigTdf().getServerConfigs().find(request.getBaseName());
    if (svr == mBootUtil.getConfigTdf().getServerConfigs().end())
    {
        BLAZE_WARN_LOG(Log::CONTROLLER,  "[MasterController].processGetServerConfigMaster: Invalid base instance name: " << request.getBaseName() 
            << ", no such instance defined in the config master's boot file.");
        return GetServerConfigMasterError::CTRL_ERROR_SERVER_INSTANCE_BASE_NAME_INVALID;
    }

    // NOTE: It's safe to const cast here, because the response is NEVER written to
    response.setMasterBootConfig(const_cast<BootConfig&>(mBootUtil.getConfigTdf()));

    return GetServerConfigMasterError::ERR_OK;
}

RunDbMigError::Error MasterController::processRunDbMig(const Blaze::RunDbMigInfo &request, const Message *message)
{
    RunDbMigError::Error err = RunDbMigError::ERR_OK;
    for (DbIdByPlatformTypeMap::const_iterator itr = request.getDbIdByPlatformType().begin(); itr != request.getDbIdByPlatformType().end(); ++itr)
    {
        err = runDbMigOnDb(itr->second, request, itr->first);
        if (err != RunDbMigError::ERR_OK)
            return err;
    }

    return err;
}

RunDbMigError::Error MasterController::runDbMigOnDb(uint32_t dbId, const Blaze::RunDbMigInfo &request, ClientPlatformType platform)
{
    FiberAutoMutex autoMutex(mDbMigFiberMutex);
    if (!autoMutex.hasLock())
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[MasterController::processRunDbMig] Failed to acquire fiber mutex.");
        return RunDbMigError::CTRL_ERROR_DBMIG_FAILED;
    }

    const char8_t* componentName = request.getComponentName();
    const char8_t* platformStr = ClientPlatformTypeToString(platform);

    //Check if we can early out here (schema version)
    bool runSchemaDbMig = true;
    uint32_t schemaVersion = gDbScheduler->getComponentVersion(dbId, componentName);
    if (schemaVersion >= request.getVersion())
    {
        if (schemaVersion == request.getVersion())
        {
            BLAZE_TRACE_LOG(Log::CONTROLLER, "[MasterController::processRunDbMig] Component " << componentName << " for platform '" << platformStr << "' already at version " << request.getVersion() << ". Nothing to do.");
        }
        else
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[MasterController::processRunDbMig] Component " << componentName << " for platform '" << platformStr << "' will not be changed; requested version (" << request.getVersion() << ") is lower than current version (" << schemaVersion << ")");
        }
        runSchemaDbMig = false;
    }
    
    //Check if we can early out here (content version)
    eastl::string contentName(componentName);
    bool runContentDbMig = true;
    contentName.append("_content");
    uint32_t contentVersion = gDbScheduler->getComponentVersion(dbId, contentName.c_str());
    if (contentVersion >= request.getContentVersion())
    {
        if (contentVersion == request.getContentVersion())
        {
            BLAZE_TRACE_LOG(Log::CONTROLLER, "[MasterController::processRunDbMig] Component " << contentName << " for platform '" << platformStr << "' already at version " << request.getContentVersion() << ". Nothing to do.");
        }
        else
        {
            BLAZE_WARN_LOG(Log::CONTROLLER, "[MasterController::processRunDbMig] Component " << contentName << " for platform '" << platformStr << "' will not be changed; requested version (" << request.getContentVersion() << ") is lower than current version (" << contentVersion << ")");
        }
        runContentDbMig = false;
    }

    //If neither the schema nor content need to be upgraded, we can early out
    if(!runSchemaDbMig && !runContentDbMig)
    {
        return RunDbMigError::ERR_OK;
    }

    //Are we actually allowed to do anything?  
    if (!gController->isDBDestructiveAllowed() && !request.getDbDestructive())
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[MasterController::processRunDbMig].upgradeSchemaWithDbMig: Schema change required, but --dbdestructive not set."
            " run blazeserver with --dbdestructive or manually upgrade component " << componentName << " for platform '" << platformStr << "'");
        return RunDbMigError::CTRL_ERROR_DBMIG_FAILED;
    }

    // Determine the component type
    const ComponentId componentId = BlazeRpcComponentDb::getComponentIdByName(componentName);
    const char8_t* componentType;

    if (RpcIsFrameworkComponent(componentId) || (componentId == RPC_COMPONENT_UNKNOWN))
    {
        componentType = "framework";
    }
    else
    {
        //Check if its a custom component or not
        char8_t path[512];
        blaze_snzprintf(path, sizeof(path), "%s/customcomponent/%s/", gDbScheduler->getDbMigConfig().getComponentPath(), componentName);
        if (EA::IO::Directory::Exists(path))
        {
            componentType = "customcomponent";
        }
        else
        {
            componentType = "component";
        }
    }

    const DbConfig* config = gDbScheduler->getConfig(dbId);
    const eastl::string pythonPath(gDbScheduler->getDbMigConfig().getPythonPath());

    //Set up the python arguments
    eastl::string tmp;
    eastl::string cen;
    Blaze::ShellExecArgList dbmigArgs;
    dbmigArgs.reserve(21);
    dbmigArgs.push_back(gDbScheduler->getDbMigConfig().getDbmigPath());
    dbmigArgs.push_back("upgrade");
    dbmigArgs.push_back("-c");
    dbmigArgs.push_back(componentName);
    dbmigArgs.push_back("-d");
    dbmigArgs.push_back("mysql");
    dbmigArgs.push_back("-r");
    dbmigArgs.push_back(gDbScheduler->getDbMigConfig().getComponentPath());
    dbmigArgs.push_back("-V");
    dbmigArgs.push_back(tmp.sprintf("%u",request.getVersion()).c_str());
    dbmigArgs.push_back("-v");
    dbmigArgs.push_back(tmp.sprintf("%u",request.getContentVersion()).c_str());
    dbmigArgs.push_back("-o");
    // censor argument's password here
    // Wrap options in quotes to prevent issues with characters that must be escaped in the shell, but
    // cannot be escaped in Blaze configs because they're also passed directly to MySQL C API.
#ifdef EA_PLATFORM_LINUX
    char8_t quoteChar = '\'';
#else
    char8_t quoteChar = '\"';
#endif
    tmp.sprintf("%chost=%s,port=%d,user=%s,db=%s,", quoteChar, config->getHost(), config->getPort(), config->getUser(), config->getDatabase());
    cen.sprintf("%spasswd=<CENSORED>%c", tmp.c_str(), quoteChar);
    tmp.append_sprintf("passwd=%s%c", config->getPassword(), quoteChar);
    dbmigArgs.push_back(ShellExecArg(tmp.c_str(), cen.c_str()));

    dbmigArgs.push_back("--cpath");
    dbmigArgs.push_back(tmp.sprintf("%s/%s/", componentType, componentName).c_str());
    if (gDbScheduler->getDbMigConfig().getContentEnvironment()[0] != '\0')
    {
        dbmigArgs.push_back("--contentenv");
        dbmigArgs.push_back(gDbScheduler->getDbMigConfig().getContentEnvironment());
    }
    dbmigArgs.push_back("-p");
    dbmigArgs.push_back(platformStr);
    if (!gController->isSharedCluster())
        dbmigArgs.push_back("--singleplatform");
    dbmigArgs.push_back("2>&1");

    //Run db mig using the shell
    BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController::processRunDbMig] (shell exec) ...");
    int32_t status;
    eastl::string dbmigOutput;
    eastl::string dbmigCommandString;
    eastl::string dbmigCensoredCommandString;
    ShellRunner::ShellJobId jobId;
    BlazeRpcError ret = gShellRunner->shellExec(Blaze::SHELLEXEC_PYTHON_DBMIG, pythonPath, dbmigArgs, &status, dbmigOutput, dbmigCommandString, dbmigCensoredCommandString, jobId);
    if(ret == Blaze::ERR_OK)
    {
        if (WIFEXITED(status) != 0) // child process exited normally/on it's own 
        {
            const int32_t exitStatus = WEXITSTATUS(status); 
            if (exitStatus == 0) // is the process indicating a success?
            {
                BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController::processRunDbMig] -->\n" 
                    << dbmigOutput.c_str() << "<-- DB update SUCCEEDED.");

                gDbScheduler->updateComponentVersion(dbId, componentName, request.getVersion());      
                gDbScheduler->updateComponentVersion(dbId, contentName.c_str(), request.getContentVersion());                
            }
            else
            {
                BLAZE_ERR_LOG(Log::CONTROLLER,  "[MasterController::processRunDbMig] -->\n" 
                    << dbmigOutput.c_str() << "<-- DB update FAILED, err(" << exitStatus << ") \n Command:(" 
                    << dbmigCensoredCommandString.c_str() << ")");                    
                return RunDbMigError::CTRL_ERROR_DBMIG_FAILED;
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[MasterController::processRunDbMig] -->\n" 
                << dbmigOutput.c_str() << "<-- DB update FAILED, err(process terminated abnormally) \n Command:(" 
                << dbmigCensoredCommandString.c_str() << ")");
            return RunDbMigError::CTRL_ERROR_DBMIG_FAILED;
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[MasterController::processRunDbMig] "
            "DB update FAILED, err(" << ErrorHelp::getErrorName(ret) << ") \n Command:("
            << dbmigCensoredCommandString.c_str() << ")");
        return RunDbMigError::CTRL_ERROR_DBMIG_FAILED;
    }

    return RunDbMigError::ERR_OK;
}

ReconfigureMasterError::Error MasterController::processReconfigureMaster(
    const ReconfigureRequest &request,
    const Message *message)
{
    // check if the current user has the permission to reload config
    if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_RELOAD_CONFIG))
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "Attempted to reconfigure, no permission!");
        return ReconfigureMasterError::ERR_AUTHORIZATION_REQUIRED;
    }

    if (mReconfigState != MasterController::RECONFIGURE_STATE_READY)
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController].processReconfigureMaster: Reconfigure is already in progress. Current state is " << mReconfigState);

        uint32_t numNoAck = 0;
        StringBuilder noAckIds(nullptr);
        for (ControllerList::const_iterator i = mSlaveControllers.begin(), e = mSlaveControllers.end(); i != e; ++i)
        {
            if ((mReconfiguringSlaveSessionIds.find(i->getSlaveSessionId()) != mReconfiguringSlaveSessionIds.end())
                && (i->getReconfigState() == MasterController::RECONFIGURE_STATE_PREPARING || i->getReconfigState() == MasterController::RECONFIGURE_STATE_RECONFIGURING))
            {
                numNoAck++;
                noAckIds << GetInstanceIdFromInstanceKey64(i->getSlaveSessionId());
                noAckIds << ",";
            }
        }
        if (numNoAck > 0)
        {
            noAckIds.trim(1); // trim last comma
            BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController].processReconfigureMaster: waiting on " << numNoAck << " controller(s): " << noAckIds.get());
        }
        return ReconfigureMasterError::CTRL_ERROR_RECONFIGURE_IN_PROGRESS;
    }

    mReconfigureValidateOnly = request.getValidateOnly();
    
    ConfigFeatureList allFeaturesList;
    if (request.getFeatures().empty())
    {
        // reconfigure all is not allowed
        BLAZE_INFO_LOG(Log::CONTROLLER, 
            "[MasterController].processReconfigureMaster: Attempt to reconfigure all features has been denied. "
            "Reconfigure all is not allowed. Please supply list of features to reconfigure.");
        return ReconfigureMasterError::CTRL_ERROR_RECONFIGURE_ALL_NOT_ALLOWED;
    }

    const ConfigFeatureList &featureList = request.getFeatures();

    mReconfigureFeatureList.clear();

    for (ConfigFeatureList::const_iterator i = featureList.begin(), e = featureList.end(); i != e; ++i)
    {
        const char8_t* featureName = *i;

        //  make sure the feature we're reconfiguring exists with a valid configuration.
        FeatureConfigInfo* featureConfigInfo = loadConfigInternal(featureName, true);
        if (featureConfigInfo == nullptr)
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[MasterController].processReconfigureMaster: Failed to reload config file for feature: " << featureName << ".");
            return ReconfigureMasterError::ERR_SYSTEM;
        }
            
        mReconfigureFeatureList.push_back(featureName);

    }

    PrepareForReconfigureNotification data;
    mReconfigureFeatureList.copyInto(data.getFeatures());

    if (!data.getFeatures().empty())
    {
        eastl::string attempt;
        for (ConfigFeatureList::const_iterator 
            i = data.getFeatures().begin(), 
            e = data.getFeatures().end(); i != e; ++i)
        {
            attempt += i->c_str();
            attempt += ",";
        }
        // trim last comma
        attempt.pop_back();

        BLAZE_INFO_LOG(Log::CONTROLLER,
            "[MasterController].processReconfigureMaster: "
            "Reconfiguring feature list [" << attempt.c_str() << "].");

        mReconfiguringSlaveSessionIds.clear();
        mReconfigState = MasterController::RECONFIGURE_STATE_PREPARING;
        for (ControllerList::iterator i = mSlaveControllers.begin(), e = mSlaveControllers.end(); i != e; ++i)
        {
            i->setReconfigState(MasterController::RECONFIGURE_STATE_PREPARING);
            mReconfiguringSlaveSessionIds.insert(i->getSlaveSessionId());
            BLAZE_TRACE_LOG(Log::CONTROLLER, "[MasterController].processReconfigureMaster: inserting sessionId[" << i->getSlaveSessionId() << "] into reconfiguring map");
        }
        
        // tell all connected instances to begin validating the specified features
        sendNotifyPrepareForReconfigureToSlaves(&data);     
    }
    else
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[MasterController].processReconfigureMaster:  No features will be reconfigured.");
        return ReconfigureMasterError::CTRL_ERROR_RECONFIGURE_SKIPPED;
    }
    
    if (gCurrentUserSession != nullptr)
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "User [" << gCurrentUserSession->getBlazeId() << "] reconfigured, had permission.");
    }
    return ReconfigureMasterError::ERR_OK;
}


PreparedForReconfigureMasterError::Error MasterController::processPreparedForReconfigureMaster(const Blaze::PreparedForReconfigureRequest &request, const Message* message)
{
    return processPreparedForReconfigureMaster(request, message != nullptr ? message->getSlaveSessionId() : gController->getInstanceSessionId());
}

PreparedForReconfigureMasterError::Error MasterController::processPreparedForReconfigureMaster(const Blaze::PreparedForReconfigureRequest &request, SlaveSessionId incomingSessionId)
{
    if (mReconfigState != MasterController::RECONFIGURE_STATE_PREPARING)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[MasterController].processPreparedForReconfigureMaster:  In an unexpected reconfiguration state (" << mReconfigState << ").  Expecting PREPARING.");
        return PreparedForReconfigureMasterError::ERR_SYSTEM;
    }

    //  Our stored feature list shouldn't ever be empty by this point.
    //  Check if any feature has failed validation by checking the received validation error map.  If there's an entry for this feature, then
    //  validation has failed and send notification to slaves that we're done.
    const ConfigureValidationFailureMap& validationFailures = request.getValidationErrorMap();
    for (ConfigFeatureList::const_iterator i = mReconfigureFeatureList.begin(), e = mReconfigureFeatureList.end(); i != e; ++i)
    {
        const char8_t* featureName = *i;
        ConfigureValidationFailureMap::const_iterator it = validationFailures.find(featureName);
        if (it != validationFailures.end())
        {
            //  prevent duplicates in our stored validation map.
            if (mReconfigureValidationFailureMap.find(featureName) == mReconfigureValidationFailureMap.end())
            {
                if (mReconfigureValidationFailureMap.empty())
                {
                    //  reserve enough space for the expected number of validation failures.
                    mReconfigureValidationFailureMap.reserve(validationFailures.size());
                }
                //  validation has failed.  build failure map.
                ConfigureValidationErrors *errors = mReconfigureValidationFailureMap.allocate_element();
                it->second->copyInto(*errors);
                mReconfigureValidationFailureMap[it->first] = errors;
            }
        }
    }

    //  Decide whether to send validation notification to slaves now (if all slaves reported in)
    //  Or wait.
    bool isFinishedValidating = true;
    for (ControllerList::iterator i = mSlaveControllers.begin(), e = mSlaveControllers.end(); i != e; ++i)
    {  
        if (i->getSlaveSessionId() == incomingSessionId)
        {
            i->setReconfigState(MasterController::RECONFIGURE_STATE_PREPARED);
        }

        if ((mReconfiguringSlaveSessionIds.find(i->getSlaveSessionId()) != mReconfiguringSlaveSessionIds.end()) && (i->getReconfigState() != MasterController::RECONFIGURE_STATE_PREPARED))
        {
            BLAZE_TRACE_LOG(Log::CONTROLLER, "[MasterController].processPreparedForReconfigureMaster: waiting for sessionId["
                << i->getSlaveSessionId() << "] to change to state RECONFIGURE_STATE_PREPARED");
            isFinishedValidating = false;
        }
    }

    char8_t buf[256];
    ControllerInfo* info = getControllerInfoBySessionId(incomingSessionId);
    if (info != nullptr)
        info->toString(buf, sizeof(buf));
    else
        buf[0] = '\0';
        
    BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController].processPreparedForReconfigureMaster : finished controller(" << buf 
                   << ") configuration validation step with a running total of " << (size_t)mReconfigureValidationFailureMap.size() << " errors found.");

    if (isFinishedValidating)
    {
        PrepareForReconfigureCompleteNotification notify;
        if (!mReconfigureValidationFailureMap.empty())
        {
            ConfigureValidationFailureMap& notifyValidationFailures = notify.getValidationErrorMap();
            mReconfigureValidationFailureMap.copyInto(notifyValidationFailures);
            mReconfigureValidationFailureMap.release();
        }

        BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController].processPreparedForReconfigureMaster : completed configuration validation step with " << notify.getValidationErrorMap().size() 
                      << " errors found.  Sending notification to slaves.");

        bool reconfigureSlaves = !mReconfigureValidateOnly && notify.getValidationErrorMap().empty();
        notify.setRestoreToActive(!reconfigureSlaves);
        sendNotifyPrepareForReconfigureCompleteToSlaves(&notify);
        
        if (reconfigureSlaves)
        {
            // This means that all the controllers that were tagged for preparation have ACKed their
            // success(or perhaps were removed out of the controller list due to session disconnect).

            //We're good to go - replace the old configs with the new ones in the feature config map.
            mReconfigState = MasterController::RECONFIGURE_STATE_RECONFIGURING;  
            for (ControllerList::iterator i = mSlaveControllers.begin(), e = mSlaveControllers.end(); i != e; ++i)
            {
                if (mReconfiguringSlaveSessionIds.find(i->getSlaveSessionId()) != mReconfiguringSlaveSessionIds.end())
                {
                    BLAZE_TRACE_LOG(Log::CONTROLLER, "[MasterController].processPreparedForReconfigureMaster: sessionId["
                        << i->getSlaveSessionId() << "] setting state RECONFIGURE_STATE_RECONFIGURING");
                    i->setReconfigState(MasterController::RECONFIGURE_STATE_RECONFIGURING);
                }
            }

            ReconfigureNotification reconNotification;
            for (ConfigFeatureList::const_iterator i = mReconfigureFeatureList.begin(), e = mReconfigureFeatureList.end(); i != e; ++i)
            {
                FeatureConfigMap::iterator featureIt = mFeatureConfigMap.find((*i).c_str());
                FeatureConfigMap::iterator stagingIt = mStagingFeatureConfigMap.find((*i).c_str());
                if (stagingIt == mStagingFeatureConfigMap.end())
                {
                    // this really shouldn't happen either since we can only get here if there were no validation errors reported
                    // which means all of the features that we want to reconfigure should have their configurations validated and loaded
                    // in mStagingFeatureConfigMap
                    BLAZE_WARN_LOG(Log::CONTROLLER, "[MasterController].processPreparedForReconfigureMaster : validated config not found for feature(" 
                        << (*i).c_str() << "), even though there were no validation errors reported!"); 
                    continue;
                }

                if (featureIt != mFeatureConfigMap.end())
                {
                    delete featureIt->second;
                }

                mFeatureConfigMap[(*i).c_str()] = stagingIt->second;

                //add it to the list
                reconNotification.getFeatures().push_back((*i).c_str());

                //remove it from staging feature config map
                mStagingFeatureConfigMap.erase(stagingIt);
            }

            //  clear out whatever remains in the reload configs map
            for (FeatureConfigMap::iterator i = mStagingFeatureConfigMap.begin(), e = mStagingFeatureConfigMap.end(); i != e; ++i)
            {
                BLAZE_WARN_LOG(Log::CONTROLLER, "[MasterController].processPreparedForReconfigureMaster : unexpected validated config found for feature(" 
                    << i->first.c_str() << "). This feature was not in the list of features to be reconfigured.  Please audit and remove code that causes this feature to be added to mStagingFeatureConfigMap"); 
                delete i->second;
            }
            mStagingFeatureConfigMap.clear();

            // tell all connected instances to begin reconfiguring the specified features
            sendNotifyReconfigureToSlaves(&reconNotification);
        }
        else
        {
            // reset to READY as reconfiguration process is now done.
            mReconfigState = MasterController::RECONFIGURE_STATE_READY;   
            for (ControllerList::iterator i = mSlaveControllers.begin(), e = mSlaveControllers.end(); i != e; ++i)
            {  
                i->setReconfigState(MasterController::RECONFIGURE_STATE_READY);
            }
            mReconfiguringSlaveSessionIds.clear();

            //  clear out reload configs map now since master controller will not continue the reconfiguration process
            for (FeatureConfigMap::iterator i = mStagingFeatureConfigMap.begin(), e = mStagingFeatureConfigMap.end(); i != e; ++i)
            {
                delete i->second;
            }
            mStagingFeatureConfigMap.clear();
        }

        //  done with the feature list that was constructed at start of validation.
        mReconfigureFeatureList.clear();
    }

    return PreparedForReconfigureMasterError::ERR_OK;
}

GetConfigFeatureListMasterError::Error MasterController::processGetConfigFeatureListMaster(Blaze::ReconfigurableFeatures &response, const Message* message)
{
    FeatureNameSet::const_iterator it = mFeatureNameSet.begin();
    FeatureNameSet::const_iterator end = mFeatureNameSet.end();
    for (; it != end; ++it)
    {
        response.getFeatures().push_back(*it);
    }

    return GetConfigFeatureListMasterError::ERR_OK;
}

ReconfiguredMasterError::Error MasterController::processReconfiguredMaster(const Message* message)
{
    return processReconfigured(message != nullptr ? message->getSlaveSessionId() : gController->getInstanceSessionId());
}

ReconfiguredMasterError::Error MasterController::processReconfigured(SlaveSessionId sessionId)
{
    BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController].processReconfigured: start for slave session(" << sessionId << ")");

    if (mReconfigState != MasterController::RECONFIGURE_STATE_RECONFIGURING)
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[MasterController].processReconfigured:  Reconfigure is *not* in progress.");
        return ReconfiguredMasterError::ERR_OK;
    }

    ControllerInfo* controller = getControllerInfoBySessionId(sessionId);
    if (controller == nullptr)
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[MasterController].processReconfigured:  No controller found for slave session: " << sessionId << ".");
        return ReconfiguredMasterError::ERR_SYSTEM;
    }

    if (controller->getReconfigState() != MasterController::RECONFIGURE_STATE_RECONFIGURING)
    {
        // We check the state in case a new controller came up and responded to 
        // the onReconfigure() notification while MasterController is performing
        // a reconfiguration. The new controller it would be in MasterController::READY
        // state, but should not hold up reconfiguration of existing controllers.
        char8_t buf[256];
        controller->toString(buf, sizeof(buf));

        BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController].processReconfigured:  Controller (" << buf << ") is not in state RECONFIGURE_STATE_RECONFIGURING.");
        return ReconfiguredMasterError::ERR_OK;
    }

    controller->setReconfigState(MasterController::RECONFIGURE_STATE_RECONFIGURED);

    for (ControllerList::const_iterator i = mSlaveControllers.begin(), e = mSlaveControllers.end(); i != e; ++i)
    {
        if ((mReconfiguringSlaveSessionIds.find(i->getSlaveSessionId()) != mReconfiguringSlaveSessionIds.end()) && (i->getReconfigState() == MasterController::RECONFIGURE_STATE_RECONFIGURING))
        {
            char8_t buf[256];
            i->toString(buf, sizeof(buf));

            // we did not receive all reconfigured ACKs from all controllers
            BLAZE_TRACE_LOG(Log::CONTROLLER, "[MasterController].processReconfigured:  We did not receive all reconfigured ACKs from all controllers including (" << buf << ").");
            return ReconfiguredMasterError::ERR_OK;
        }
    }


    // Advance relevant controllers to the next state
    mReconfigState = MasterController::RECONFIGURE_STATE_READY;
    for (ControllerList::iterator i = mSlaveControllers.begin(), e = mSlaveControllers.end(); i != e; ++i)
    {
        ReconfiguringSlaveSessionIdSet::iterator iter = mReconfiguringSlaveSessionIds.find(i->getSlaveSessionId());
        if ((iter != mReconfiguringSlaveSessionIds.end()) && (i->getReconfigState() == MasterController::RECONFIGURE_STATE_RECONFIGURED))
        {
            BLAZE_TRACE_LOG(Log::CONTROLLER, "[MasterController].processReconfigured: sessionId["
                << i->getSlaveSessionId() << "] setting state RECONFIGURE_STATE_READY");
            i->setReconfigState(MasterController::RECONFIGURE_STATE_READY);
        }
    }

    // this is the final notification
    ReconfigureNotification data;
    data.setFinal(true);
    
    // tell all connected instances to begin (or finish) reconfiguring the specified features
    sendNotifyReconfigureToSlaves(&data);
    mReconfiguringSlaveSessionIds.clear();

    BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController].processReconfigured:  All controllers reconfigured.");

    return ReconfiguredMasterError::ERR_OK;
}

SetComponentStateMasterError::Error MasterController::processSetComponentStateMaster(
            const Blaze::SetComponentStateRequest &request, 
            const Message *message)
{
    BlazeId blazeId = INVALID_BLAZE_ID;
    if (gCurrentUserSession != nullptr)
    {
        blazeId = gCurrentUserSession->getBlazeId();
    }

    if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_RELOAD_CONFIG))
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "User [" << blazeId << "] attempted to set component state, no permission!");
        return SetComponentStateMasterError::ERR_AUTHORIZATION_REQUIRED;
    }

    sendSetComponentStateToSlaves(&request);
 
    BLAZE_INFO_LOG(Log::CONTROLLER, "User [" << blazeId << "] set component state, had permission.");
    return SetComponentStateMasterError::ERR_OK;
}

GetConfigMasterError::Error MasterController::processGetConfigMaster(
    const Blaze::ConfigRequest &request, 
    Blaze::ConfigResponse &response, 
    const Message*)
{
    GetConfigMasterError::Error rc = GetConfigMasterError::ERR_SYSTEM;
    FeatureConfigInfo* info = nullptr;

    if (request.getLoadFromStaging())
    {
        FeatureConfigMap::const_iterator it = mStagingFeatureConfigMap.find(request.getFeature());
        if (it != mStagingFeatureConfigMap.end())
        {
            info = it->second;
        }
        else
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[MasterController].processGetConfigMaster: No staging config found for feature(" 
                    << request.getFeature() << "). This feature was not in the list of features to be reconfigured."); 
        }
    }
    else
    {
        info = loadConfigInternal(request.getFeature(), false, request.getLoadPreconfig());
    }
    if (info != nullptr)
    {
        response.setConfigData(info->mConfigData->getRawConfigData().c_str());
        response.setOverrideConfigData(info->mConfigData->getOverrideFile() != nullptr ? 
            info->mConfigData->getOverrideFile()->getRawConfigData().c_str() : "");        
        rc = GetConfigMasterError::ERR_OK;
    }

    return rc;
}

/*******************************************************************************************************/
/*!
    \brief loadConfigInternal

    Load feature configuration, and optionally reload the configuration tdf into the staging feature
    configuration map.

    \param[in]  reload - if true, the staging feature configuration map is reloaded from file.
                         Should not be set to true unless the caller is processReconfigureMaster
*/
/*******************************************************************************************************/
MasterController::FeatureConfigInfo* MasterController::loadConfigInternal(const char8_t* feature, bool reload, bool preconfig)
{
    const bool isComponent = BlazeRpcComponentDb::isNonProxyComponent(feature);
    eastl::pair<FeatureConfigMap::iterator, bool> result;
  
    if (reload)
    {
        result = mStagingFeatureConfigMap.insert(eastl::make_pair(feature, (FeatureConfigInfo*)nullptr));
    }
    else if (preconfig)
    {
        result = mFeaturePreconfigMap.insert(eastl::make_pair(feature, (FeatureConfigInfo*)nullptr));
    }
    else
    {
        result = mFeatureConfigMap.insert(eastl::make_pair(feature, (FeatureConfigInfo*)nullptr));
    }

    FeatureConfigMap::iterator it = result.first;
    if (result.second)
    {
        uint32_t flags = ConfigBootUtil::CONFIG_CREATE_DUMMY;
        
        if (isComponent)
            flags |= ConfigBootUtil::CONFIG_IS_COMPONENT;

        if (reload)
            flags |= ConfigBootUtil::CONFIG_RELOAD;

        if (preconfig)
            flags |= ConfigBootUtil::CONFIG_LOAD_PRECONFIG;

        ConfigFile* file = mBootUtil.createFrom(feature, flags);
        if (file == nullptr)
        {
            if (reload)
            {
                mStagingFeatureConfigMap.erase(it);
            }
            else if (preconfig)
            {
                mFeaturePreconfigMap.erase(it);
            }
            else
            {
                mFeatureConfigMap.erase(it);
                mFailedFeatureList.push_back(feature);
            }
            return nullptr;
        }
        it->second = BLAZE_NEW FeatureConfigInfo(file);
    }
    
    return it->second;
}

void MasterController::onSlaveSessionAdded(SlaveSession& session)
{
    if (session.getConnection() == nullptr)
    {
        BLAZE_ASSERT_LOG(Log::CONTROLLER, "[MasterController].onSlaveSessionAdded: The SlaveSession that was added has no connection. Session id(" << session.getId()<< ")");
        return;
    }

    char8_t buf[256];
    const ControllerInfo* existing = getControllerInfoBySessionId(session.getId());
    if (existing != nullptr)
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[MasterController].onSlaveSessionAdded: Duplicate session id, "
            << "can't subscribe new instance(" << session.getId()<< ") because session " 
            << existing->toString(buf, sizeof(buf)) << " is already registered.");
        return;
    }
    ControllerInfo& info = *(BLAZE_NEW ControllerInfo(session.getId()));
    info.setAddress(session.getConnection()->getPeerAddress());

    mSlaveControllers.push_back(info);
    BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController].onSlaveSessionAdded: added remote controller: " << info.toString(buf, sizeof(buf)));
}

void MasterController::onSlaveSessionRemoved(SlaveSession& session)
{
    for (ControllerList::iterator i = mSlaveControllers.begin(), e = mSlaveControllers.end(); i != e; ++i)
    {
        ControllerInfo& info = *i;
        if (info.getSlaveSessionId() != session.getId())
            continue;
            
        char8_t buf[256];
        BLAZE_INFO_LOG(Log::CONTROLLER, "[MasterController].onSlaveSessionRemoved: removed remote controller: " 
            << info.toString(buf, sizeof(buf)));

        // We may need to generate an outstanding notification, because MasterController
        // waits for all outstanding ACKs to be received from all controllers before
        // it can transition to the next state. Thus when a controller is disconnected
        // the MasterController must auto-transition to the next state.
        if (info.getReconfigState() == MasterController::RECONFIGURE_STATE_PREPARING)
        {
            PreparedForReconfigureRequest req;
            processPreparedForReconfigureMaster(req, info.getSlaveSessionId());
        }
        if (info.getReconfigState() == MasterController::RECONFIGURE_STATE_RECONFIGURING)
        {
            processReconfigured(info.getSlaveSessionId());
        }
                
        // remove this controller from the list
        mSlaveControllers.remove(info);
        mReconfiguringSlaveSessionIds.erase(info.getSlaveSessionId());
        
        // we own this pointer, kill it
        delete &info;
        break;
    }
}

MasterController::ControllerInfo* MasterController::getControllerInfoBySessionId(SlaveSessionId id) 
{
    ControllerList::iterator itr = mSlaveControllers.begin();
    ControllerList::iterator end = mSlaveControllers.end();
    for(; itr != end; ++itr)
    {
        ControllerInfo& info = *itr;
        if (info.getSlaveSessionId() == id)
        {
            return &info;
        }
    }

    return nullptr;
}

// ControllerInfo

MasterController::ControllerInfo::ControllerInfo(SlaveSessionId sessionId) :
    mSlaveSessionId(sessionId),
    mReconfigState(MasterController::RECONFIGURE_STATE_READY)
{
}

void MasterController::ControllerInfo::setAddress(const InetAddress& addr)
{
    mAddress = addr;
}

const char8_t* MasterController::ControllerInfo::toString(char8_t* buf, size_t len) const
{
    char8_t addrStr[128];
    blaze_snzprintf(buf, len, "%" PRIu16 " @ %s",
        GetInstanceIdFromInstanceKey64(mSlaveSessionId), mAddress.toString(addrStr, sizeof(addrStr)));
    return buf;
}


// FeatureConfigInfo

MasterController::FeatureConfigInfo::FeatureConfigInfo(ConfigFile* file) 
: mConfigData(file)
{

}

MasterController::FeatureConfigInfo::~FeatureConfigInfo()
{
    delete mConfigData;
}

} // Blaze

