#include "framework/blaze.h"
#include "framework/system/fibermanager.h"
#include "framework/connection/selector.h"
#include "framework/component/component.h"
#include "framework/component/componentstub.h"
#include "framework/component/commandmetrics.h"
#include "framework/component/inboundrpccomponentglue.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/component/componentmanager.h"
#include "framework/config/configmerger.h"
#include "framework/replication/replicator.h"
#include "framework/tdf/replicationtypes_server.h"
#include "framework/connection/inboundrpcconnection.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "framework/util/random.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/slivers/slivermanager.h"
#include "framework/vault/vaultmanager.h"
#include "framework/vault/vaultlookuplocater.h"
#include "framework/database/dbscheduler.h"
#include "EASTL/string.h"


namespace Blaze
{

static const char8_t* sStateNames[] = {
    "INIT",
    "CONFIGURING",
    "CONFIGURED",
    "RESOLVING",
    "RESOLVED",
    "CONFIGURING_REPLICATION",
    "CONFIGURED_REPLICATION",
    "ACTIVATING",
    "ACTIVE",
    "SHUTTING_DOWN",
    "SHUTDOWN",
    "ERROR",
    "INVALID"
};

static_assert(ComponentStub::STATE_COUNT == EAArrayCount(sStateNames), "If you see this compile error, make sure ComponentStub::State and sStateNames are in sync.");

// static 
const char8_t* ComponentStub::getStateName(State state)
{
    return sStateNames[state];
}

ComponentStub::ComponentStub(const ComponentData& info) 
: mComponentInfo(info),
  mAllocator(*Allocator::getAllocator(getStubMemGroupId())),
  mMaster(nullptr),
  mState(INIT),
  mEnabled(true),
  mDisableErrorReturnCode(ERR_COMPONENT_NOT_FOUND),  
  mPendingConfiguration(nullptr),
  mConfiguration(nullptr),
  mPreconfiguration(nullptr),
  mMetricsCollection(Metrics::gMetricsCollection->getCollection(Metrics::Tag::component, info.name, true))
{
    //Initialize the metrics data
    mMetricsAllTime = BLAZE_NEW_ARRAY_MGID(CommandMetrics*, mComponentInfo.commands.size(), getStubMemGroupId(), "Component AllTime Metrics");

    //This stuff never changes so do it once and be done with it
    for (ComponentData::CommandInfoMap::const_iterator itr = mComponentInfo.commands.begin(), end = mComponentInfo.commands.end(); itr != end; ++itr)
    {
        mMetricsAllTime[itr->second->index] = BLAZE_NEW CommandMetrics(*itr->second);
    }
}

ComponentStub::~ComponentStub()
{
    for(size_t idx = 0; idx < mComponentInfo.commands.size(); idx++)
        delete mMetricsAllTime[idx];
    delete[] mMetricsAllTime;
}

void ComponentStub::changeStateAndSignalWaiters(State state)
{
    if (mState != state)
    {
        mState = state;

        BlazeRpcError err = ((mState == SHUTDOWN) || (mState == ERROR_STATE) ? ERR_CANCELED : ERR_OK);
        for (WaitListByStateMap::iterator it = mStateWaitLists.begin(), end = mStateWaitLists.end(); it != end; ++it)
        {
            if ((it->first <= mState) || (err == ERR_CANCELED))
            {
                it->second.signal(err);
            }
        }
    }
}

void ComponentStub::activateComponent()
{
    BlazeRpcError rc = ERR_OK;
    changeStateAndSignalWaiters(CONFIGURING);

    if (!loadPreconfig(mPreconfiguration))
    {
        BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure fetching preconfig for component(" << getComponentName() << ").");
        changeStateAndSignalWaiters(ERROR_STATE);
        return;
    }

    ConfigureValidationErrors validationErrors;
    if (mPreconfiguration != nullptr && !onValidatePreconfigTdf(*mPreconfiguration, nullptr, validationErrors))
    {
        const ConfigureValidationErrors::StringValidationErrorMessageList &errorList =  validationErrors.getErrorMessages();
        for (ConfigureValidationErrors::StringValidationErrorMessageList::const_iterator cit = errorList.begin(); cit != errorList.end(); ++cit)
        {
            BLAZE_FATAL(getStubLogCategory(), cit->c_str());
        }

        BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Validation failed for component " << getComponentName() << " preconfig.");
        changeStateAndSignalWaiters(ERROR_STATE);
        return;
    }

    //Now let the component get preconfiged
    if (!onPreconfigure())
    {
        changeStateAndSignalWaiters(ERROR_STATE);
        BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure preconfiguring component(" << getComponentName() << ").");
        return;
    }

    if (!loadConfig(mConfiguration, false))
    {
        BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure fetching config for component(" << getComponentName() << ").");
        changeStateAndSignalWaiters(ERROR_STATE);
        return;
    }

    // Anything that uses the dbmig upgrade system will have a valid db schema version.  If this is 0, we 
    // aren't using dbmig for this component. The component itself however may still rely on us to create db Ids by implementing
    // getDbNameByPlatformTypeMap() (such as gdpr).
    bool usesDbMig = (getDbSchemaVersion() != 0) || (getContentDbSchemaVersion() != 0);

    // Build the DbNameByPlatformTypeMap
    DbNameByPlatformTypeMap dbNamesByPlatform;
    if (getDbNameByPlatformTypeMap() != nullptr)
    {
        getDbNameByPlatformTypeMap()->copyInto(dbNamesByPlatform);
    }
    else if (!gController->isSharedCluster()) //backward compatibility path for the single platform cluster
    {
        if (getDbName() != nullptr && getDbName()[0] != '\0')
        {
            dbNamesByPlatform[gController->getDefaultClientPlatform()] = getDbName();
        }
        else
        {
            //looks like we didn't have a good name for this component.  There is a legacy method of looking up the
            //db name directly from the config file if the component failed to override the "Component::getDbName()" 
            //method, so try that method now:

            EA::TDF::TdfGenericReference dbNameRef;
            if (mConfiguration != nullptr && mConfiguration->getValueByName("dbName", dbNameRef) && dbNameRef.isTypeString())
            {
                const char8_t* dbName = dbNameRef.asString().c_str();
                if (dbName != nullptr && dbName[0] != '\0')
                    dbNamesByPlatform[gController->getDefaultClientPlatform()] = dbName;
            }
        }
    }

    if (!dbNamesByPlatform.empty())
    {
        ClientPlatformType missingPlatform = ClientPlatformType::INVALID;
        if (gController->isSharedCluster() && requiresDbForPlatform(ClientPlatformType::common) && dbNamesByPlatform.find(ClientPlatformType::common) == dbNamesByPlatform.end())
            missingPlatform = ClientPlatformType::common;

        for (ClientPlatformSet::const_iterator itr = gController->getHostedPlatforms().begin(); missingPlatform == ClientPlatformType::INVALID && itr != gController->getHostedPlatforms().end(); ++itr)
        {
            if ((!gController->isSharedCluster() || requiresDbForPlatform(*itr)) && dbNamesByPlatform.find(*itr) == dbNamesByPlatform.end())
                missingPlatform = *itr;
        }
        if (missingPlatform != ClientPlatformType::INVALID)
        {
            BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Component(" << getComponentName() << ") is missing a DbNameByPlatformTypeMap entry for platform '" << ClientPlatformTypeToString(missingPlatform) << "'");
            changeStateAndSignalWaiters(ERROR_STATE);
            return;
        }

        if (!gDbScheduler->parseDbNameByPlatformTypeMap(getComponentName(), dbNamesByPlatform, mDbIdByPlatformTypeMap))
        {
            BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure parsing DbNameByPlatformTypeMap for component(" << getComponentName() << ").");
            changeStateAndSignalWaiters(ERROR_STATE);
            return;
        }

        if (usesDbMig && !gController->upgradeSchemaWithDbMig(getDbSchemaVersion(), getContentDbSchemaVersion(), getStubBaseName(), mDbIdByPlatformTypeMap))
        {
            BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure updating db schema for component(" << getComponentName() << ").  Check configmaster logs for more info.");
            changeStateAndSignalWaiters(ERROR_STATE);
            return;
        }
    }
    else if (usesDbMig)
    {
        if (gController->isSharedCluster())
        {
            BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Component(" << getComponentName() << ") using db schema version " << getDbSchemaVersion() << " and db content version " << getContentDbSchemaVersion() << " does not implement getDbNameByPlatformTypeMap() in a shared cluster.");
        }
        else
        {
            BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Component(" << getComponentName() << ") using db schema version " << getDbSchemaVersion() << " and db content version " << getContentDbSchemaVersion() << " does not implement getDbNameByPlatformTypeMap() or getDbName() or does not have a dbName configuration member in a single platform cluster.");
        }
        changeStateAndSignalWaiters(ERROR_STATE);
        return;
    }

    //  validate configuration and print out errors if validation fails.
    if (mConfiguration != nullptr)
    {
        // Dynamically update any VaultLookup entries in the configuration. This will make a blocking call to the gVaultManager.
        if (!gVaultManager->updateTdfWithSecrets(*mConfiguration, validationErrors) || 
            !onValidateConfigTdf(*mConfiguration, nullptr, validationErrors))
        {
            const ConfigureValidationErrors::StringValidationErrorMessageList &errorList = validationErrors.getErrorMessages();
            for (ConfigureValidationErrors::StringValidationErrorMessageList::const_iterator cit = errorList.begin(); cit != errorList.end(); ++cit)
            {
                BLAZE_FATAL(getStubLogCategory(), cit->c_str());
            }

            BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Validation failed for component " << getComponentName() << ".");
            changeStateAndSignalWaiters(ERROR_STATE);
            return;
        }
    }

    //Now let the component get setup
    if (!onConfigure())
    {
        changeStateAndSignalWaiters(ERROR_STATE);
        BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure configuring component(" << getComponentName() << ").");
        return;
    }

    changeStateAndSignalWaiters(CONFIGURED);
    changeStateAndSignalWaiters(RESOLVING);

    if (hasMasterComponentStub())
    {
        mMaster = gController->getComponentManager().getLocalComponent(RpcMakeMasterId(getComponentId()));
        if(mMaster == nullptr || mMaster->asStub()->getState() != ACTIVE)
        {
            if(requiresMasterComponent())
            {
                mMaster = gController->getComponent(RpcMakeMasterId(getComponentId()), false, true, &rc);
            }
            else
            {
                // we don't need to wait for a master component if this component is sharded, so add it as a remote component.
                BlazeRpcError rcTemp = gController->getComponentManager().addRemoteComponentPlaceholder(RpcMakeMasterId(getComponentId()), mMaster);
                if(rcTemp != ERR_OK || mMaster == nullptr)
                {
                    // a similar check is done below, but this is to ensure we log the specific reason resolution failed, and protects against pressing on if subsequent steps change
                    BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure adding non-required master placeholder for component (" << getComponentName() << "_master).");
                    changeStateAndSignalWaiters(ERROR_STATE);         
                    return;
                }
            }

            if (rc != ERR_OK || mMaster == nullptr)
            {
                BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure resolving component(" << getComponentName() << "_master).");
                changeStateAndSignalWaiters(ERROR_STATE);
                return;
            }
            
        }
        
    }

    if (!onResolve())
    {
        BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure resolving component(" << getComponentName() << ").");
        changeStateAndSignalWaiters(ERROR_STATE);
        return;
    }

    changeStateAndSignalWaiters(RESOLVED);

    changeStateAndSignalWaiters(CONFIGURING_REPLICATION);

    if (hasMasterComponentStub())
    {
        // sync master/slave replication
        if (mMaster->hasComponentReplication() && mComponentInfo.shouldAutoRegisterForMasterReplication)
        {
            bool waitForSync = requiresMasterComponent() || (mMaster->getComponentInstanceCount() != 0);

            // even if we don't require the master, we want to sync if there are some we're already aware of
            rc = gReplicator->synchronizeRemoteCollection(*mMaster, waitForSync);
            if (rc != ERR_OK)
            {
                if (requiresMasterComponent())
                {
                    // We require the master component subscription, but it failed
                    // Nothing we can do but bail
                    BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: "
                        "Failure synchronizing replication from component(" << mMaster->getName() << ").");
                    changeStateAndSignalWaiters(ERROR_STATE);
                    return;
                }
                else
                {
                    // Failure to subscribe isn't catastrophic. We'll subscribe to the remote instance
                    // when it comes back up via addRemoteComponent
                    BLAZE_WARN_LOG(Log::SYSTEM, "[Component].activateComponent: "
                        "Failure synchronizing replication from component(" << mMaster->getName() << ").");
                }
            }
            
        }
    }

    if(!isMasterStub())
    {
        // sync slave/slave replication
        if (hasComponentReplicationStub())
        {
            rc = gReplicator->synchronizeRemoteCollection(asComponent());
            if (rc != ERR_OK)
            {
                // Failure to subscribe isn't catastrophic. We'll subscribe to the remote instance
                // when it comes back up via addRemoteComponent
                BLAZE_WARN_LOG(Log::SYSTEM, "[Component].activateComponent: "
                    "Failure synchronizing replication from component(" << getComponentName() << ").");
            }
        }
    }

    if (!onConfigureReplication())
    {
        changeStateAndSignalWaiters(ERROR_STATE);
        BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure configuring replication for component(" << getComponentName() << ").");
        return;
    }

    changeStateAndSignalWaiters(CONFIGURED_REPLICATION);

    changeStateAndSignalWaiters(ACTIVATING);

    if (!onActivate())
    {
        changeStateAndSignalWaiters(ERROR_STATE);
        BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure activating component(" << getComponentName() << ").");
        return;
    }

    if (hasMasterComponentStub())
    {
        if (mMaster->hasComponentNotifications() && mComponentInfo.shouldAutoRegisterForMasterNotifications)
        {
            // subscribe for master/slave notifications
            rc = mMaster->notificationSubscribe();
            if (rc != ERR_OK)
            {
                if (requiresMasterComponent())
                {
                    // We require the master component subscription, but it failed
                    // Nothing we can do but bail
                    changeStateAndSignalWaiters(ERROR_STATE);
                    BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure subscribing to notifications from component(" << mMaster->getName() << ")");
                    return;
                }
                else
                {
                    // Failure to subscribe isn't catastrophic. We'll subscribe to the remote instance
                    // when it comes back up via addRemoteComponent
                    BLAZE_WARN_LOG(Log::SYSTEM, "[Component].activateComponent: Failure subscribing to notifications from component(" << mMaster->getName() << ")");
                }
            }
        }
    }

    if (!isMasterStub())
    {
        if (hasComponentNotificationsStub())
        {
            // subscribe for slave/slave notifications
            rc = asComponent().notificationSubscribe();
            if (rc != ERR_OK)
            {
                // Failure to subscribe isn't catastrophic. We'll subscribe to the remote instance
                // when it comes back up via addRemoteComponent
                BLAZE_WARN_LOG(Log::SYSTEM, "[Component].activateComponent: "
                    "Failure subscribing to notifications from component(" << getComponentName() << ").");
            }
        }
    }

    if (!onCompleteActivation())
    {
        changeStateAndSignalWaiters(ERROR_STATE);
        BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].activateComponent: Failure completing activation for component(" << getComponentName() << ").");
        return;
    }

    changeStateAndSignalWaiters(ACTIVE);
}

void ComponentStub::prepareForReconfigure()
{
    ConfigureValidationErrors validationErrors;

    if (!isReconfigurable())
    {
        validationErrors.getErrorMessages().push_back("Needs TDF based config");
        gController->setConfigureValidationErrorsForFeature(getComponentName(), validationErrors);
        return;
    }


    EA::TDF::TdfPtr sourcePreconfig = nullptr;
    if (!loadPreconfig(sourcePreconfig))
    {
        BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].prepareForReconfigure: Failure fetching preconfig for component(" << getComponentName() << ").");
        changeStateAndSignalWaiters(ERROR_STATE);     
        return;
    }

    if (sourcePreconfig != nullptr)
    {
        if (!onValidatePreconfigTdf(*sourcePreconfig, mPreconfiguration, validationErrors))
        {
            const ConfigureValidationErrors::StringValidationErrorMessageList &errorList =  validationErrors.getErrorMessages();
            for (ConfigureValidationErrors::StringValidationErrorMessageList::const_iterator cit = errorList.begin(); cit != errorList.end(); ++cit)
            {
                BLAZE_FATAL(getStubLogCategory(), cit->c_str());
            }
        
            BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].prepareForReconfigure: Validation failed for component " << getComponentName() << " preconfig.");
            changeStateAndSignalWaiters(ERROR_STATE);
            return;
        }

        // Note: We have to call onPreconfigure() because it's needed for onValidateConfigTdf to work. 
        //       There's not a great way to undo this, but that's okay because preconfig is only additive, not subtractive.
        sourcePreconfig->copyInto(*mPreconfiguration);
        if (!onPreconfigure())
        {
            changeStateAndSignalWaiters(ERROR_STATE);
            BLAZE_FATAL_LOG(Log::SYSTEM, "[Component].prepareForReconfigure: Failure preconfiguring component(" << getComponentName() << ").");
            return;
        }
    }


    mPendingConfiguration = nullptr;
    
    EA::TDF::TdfPtr sourceConfig = nullptr;
    if (loadConfig(sourceConfig, true))
    {
        // This is essentially a (mPendingConfiguration = sourceConfig->clone()) call, but it needs to set copier(true) so that the change tracking is preserved.
        mPendingConfiguration = mConfiguration->getClassInfo().createInstance(DEFAULT_BLAZE_ALLOCATOR, "ComponentStub::prepareForReconfigure");
        
        EA::TDF::MemberVisitOptions copyOpts;
        copyOpts.onlyIfSet = true;
        mConfiguration->copyInto(*mPendingConfiguration, copyOpts);
        ConfigMerger merger;
        if(merger.merge(*mPendingConfiguration, *sourceConfig))
        {
            // Dynamically update any VaultLookup entries in the configuration. This will make a blocking call to the gVaultManager.
            if (!gVaultManager->updateTdfWithSecrets(*mPendingConfiguration, validationErrors) ||
                !onValidateConfigTdf(*mPendingConfiguration, mConfiguration, validationErrors))
            {
                validationErrors.getErrorMessages().push_back("Component validation failed.");
            }
        }
        else
        {
            validationErrors.getErrorMessages().push_back("Failed to merge existing configuration.");
        }
    }
    else
    {
        validationErrors.getErrorMessages().push_back("Failed to load configuration TDF.");
    }

    //If there were any errors, report them.
    if (!validationErrors.getErrorMessages().empty())
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Component].prepareForReconfigure() : Component (" << getComponentName() << ") failed validation.");
        gController->setConfigureValidationErrorsForFeature(getComponentName(), validationErrors);
        return;
    }

    if (mPendingConfiguration != nullptr && !onPrepareForReconfigureComponent(*mPendingConfiguration))
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Component].prepareForReconfigure() : Component (" << getComponentName() << ") failed to prepare for reconfiguration.");
        validationErrors.getErrorMessages().push_back("Failed to prepare for reconfiguration");
        gController->setConfigureValidationErrorsForFeature(getComponentName(), validationErrors);
        return;
    }
}

void ComponentStub::reconfigureComponent()
{
    //Make the pending config our new config
    if (mConfiguration != nullptr)
    {
        //clear out the old configuration.  We don't actually delete it, just store it off 
        //in case any bad behaved code is keeping pointers to it.  This gets cleaned up on component
        //shutdown.
        mOldConfigs.push_back(mConfiguration);
    }
    mConfiguration = mPendingConfiguration;
    mPendingConfiguration = nullptr;

    //Give the component a chance to rearrange itself
    if (!onReconfigure())
    {
        //There's not much to do on an error - this shouldn't happen unless the component 
        //is poorly written and somehow got past validations.  The transition occurs error or not. 
        BLAZE_WARN_LOG(Log::CONTROLLER, "[Component].reconfigureComponent: component " << getComponentName() << " reconfigure call returned an error.  This should not be possible.");
    }
}

void ComponentStub::shutdownComponent()
{
    changeStateAndSignalWaiters(SHUTTING_DOWN);
    onShutdown();
    changeStateAndSignalWaiters(SHUTDOWN);       
}

BlazeRpcError ComponentStub::waitForState(State state, const char8_t* context, TimeValue absoluteTimeout)
{
    if (mState >= state)
        return ERR_OK;

    if ((mState == SHUTDOWN) || (mState == ERROR_STATE))
        return ERR_CANCELED;

    return mStateWaitLists[state].wait(context, absoluteTimeout);
}

BlazeRpcError ComponentStub::executeLocalCommand(const CommandInfo& cmdInfo, const EA::TDF::Tdf* request, EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf,
    const RpcCallOptions& options, Message* origMsg, bool obfuscateResponse)
{
    // Schedule intra-instance RPCs on a separate fiber for proper metrics tracking:
    // (origMsg is nullptr for intra-instance RPCs)
    if (origMsg != nullptr)
    {
        return executeLocalCommandInternal(cmdInfo, request, responseTdf, errorTdf, options, origMsg, obfuscateResponse);
    }
    else
    {
        BlazeRpcError rc = ERR_SYSTEM;

        Message* rpcContextMsg = (gRpcContext != nullptr) ? &gRpcContext->mMessage : nullptr;

        LocalCommandParams commandParams(cmdInfo, request, responseTdf, errorTdf, options, obfuscateResponse,
            rpcContextMsg, UserSession::getCurrentUserSessionId(), UserSession::hasSuperUserPrivilege());

        // getCurrentTimeout returns the absolute timeout; convert to relative timeout
        EA::TDF::TimeValue commandTimeout = Fiber::getCurrentTimeout();
        if (commandTimeout > 0)
            commandTimeout -= EA::TDF::TimeValue::getTimeOfDay();

        Fiber::CreateParams fiberParams = InboundRpcComponentGlue::getCommandFiberParams(
            true, cmdInfo, commandTimeout, true, Fiber::allocateFiberGroupId());

        gFiberManager->scheduleCall<ComponentStub, BlazeRpcError, LocalCommandParams*>(this, &ComponentStub::processLocalCommandInternal, &commandParams, &rc, fiberParams);

        return rc;
    }
}

BlazeRpcError ComponentStub::processLocalCommandInternal(ComponentStub::LocalCommandParams* params)
{
    CommandFiberTracker commandFiberTracker;

    gLogContext.copyFromCallingFiber();

    InboundRpcConnection::initLocalRpcContext(params->mUserSessionId, params->mSuperUserPrivilege);

    Message* rpcContextMsg = params->mRpcContextMsg;
    if (rpcContextMsg == nullptr)
    {
        // rpcContextMsg will be null for commands called from scheduled fibers and component "on" callbacks (e.g. onValidateConfig);
        // Proceed without setting gRpcContext
        return executeLocalCommandInternal(params->mCmdInfo, params->mRequest, params->mResponseTdf, params->mErrorTdf, params->mOptions, nullptr, params->mObfuscateResponse);
    }
    else
    {
        // Need to make a copy of the rpcContextMsg object because the RpcContext destructor deallocates the mMessage ref.
        Message* copyMsg = BLAZE_NEW Message(rpcContextMsg->getSlaveSessionId(), rpcContextMsg->getUserSessionId(), rpcContextMsg->getPeerInfo(), rpcContextMsg->getFrame(), nullptr, nullptr);

        // Initialize gRpcContext to a local rpcContext instance;
        // rpcContext must remain in scope until executeLocalCommandInternal finishes
        InboundRpcConnectionRpcContext rpcContext(params->mCmdInfo, *copyMsg);

        return executeLocalCommandInternal(params->mCmdInfo, params->mRequest, params->mResponseTdf, params->mErrorTdf, params->mOptions, nullptr, params->mObfuscateResponse);
    }
}

BlazeRpcError ComponentStub::executeLocalCommandInternal(const CommandInfo& cmdInfo, const EA::TDF::Tdf* request, EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf,
    const RpcCallOptions& options, Message* origMsg, bool obfuscateResponse)
{
    Fiber::BlockingSuppressionAutoPtr blockingSuppression(!cmdInfo.isBlocking); // disable blocking now for this command if needed

    BlazeRpcError rc = ERR_OK;
    if (isEnabled()) 
    {
        logRpcPreamble(cmdInfo, origMsg, request);

        if ((rc == ERR_OK) &&
            cmdInfo.requiresAuthentication)
        {
            // This command requirement means that, the command should only be executed as long as there is some meaningful/authenticated
            // context under which the command will run.  This can either be a UserSession or if the request has "super user" privilege.
            // Note that, if this command options simply requires that one of these contexts exists in the system, but any immediate access
            // to further information about the context (e.g. a UserSession) won't be needed while executing the command.

            if (!UserSession::isValidUserSessionId(UserSession::getCurrentUserSessionId()) && !UserSession::hasSuperUserPrivilege())
            {
                rc = ERR_AUTHENTICATION_REQUIRED;
            }
        }

        if ((rc == ERR_OK) &&
            cmdInfo.requiresUserSession)
        {
            // This command requirement means that, for the command to execute, it needs at least the current UserSessionId.  Furthermore,
            // "super-user" privilege is *not* enough for this command to execute sucessfully; it needs a real UserSessionId!
            // Note that, a UserSession only exists *if* it is authenticated, so this option superceedes, and is splightly more specific, than
            // the requiresAuthentication option above.

            if (!UserSession::isValidUserSessionId(UserSession::getCurrentUserSessionId()))
            {
                rc = ERR_AUTHENTICATION_REQUIRED;
            }
        }

        if ((rc == ERR_OK) &&
            cmdInfo.setCurrentUserSession)
        {
            // This command requirement means that, the command expects the gCurrentUserSession to be available.  Furthermore,
            // "super-user" privilege is *not* enough for this command to execute sucessfully; it needs a real UserSession!
            // This requirement implies the previous two requirements above.

            if (gCurrentUserSession == nullptr)
            {
                //If the command requires a set user session, no ifs ands or buts, lets bail
                //generally commands that set this won't have a check for gCurrentUserSession NOT being nullptr
                //so either it a) doesn't really need this flag, or b) it does and is headed for a crash
                //if we let it continue.  

                rc = ERR_AUTHENTICATION_REQUIRED;
            }
        }

        if ((rc == ERR_OK) &&
            (!cmdInfo.allowGuestCall && ((gCurrentUserSession != nullptr) && gCurrentUserSession->isGuestSession())))
        {
            rc = ERR_GUEST_SESSION_NOT_ALLOWED;
        }

        if (rc != ERR_OK)
        {
            logRpcPostamble(rc, cmdInfo, origMsg, errorTdf);
            return rc;
        }

        //do local execution
        Command* command = (*cmdInfo.commandCreateFunc)(origMsg, const_cast<EA::TDF::Tdf*>(request), &asComponent());
        if (command != nullptr)
        {
            // Obfuscate PlatformInfo in the RPC response if:
            // (1) The response will be sent directly to the client (obfuscateResponse = true)
            // (2) The RPC command is defined with obfuscatePlatformInfo = true, and
            // (3) The server is a shared cluster
            bool shouldObfuscate = obfuscateResponse && cmdInfo.obfuscatePlatformInfo && gController->isSharedCluster();
            ClientPlatformType callerPlatform = shouldObfuscate ? INVALID : common;  // setting callerPlatform to 'common' disables PlatformInfo obfuscation
            if (callerPlatform == INVALID)
            {
                if (gCurrentUserSession != nullptr)
                {
                    callerPlatform = gCurrentUserSession->getClientPlatform();
                }
                else if (origMsg != nullptr)
                {
                    callerPlatform = gController->getServicePlatform(origMsg->getPeerInfo().getServiceName());
                }
                else
                {
                    // This shouldn't happen - origMsg should only be nullptr in cases where a component calls another "local" component's RPC
                    // (in which case obfuscateResponse should be false)
                    BLAZE_WARN_LOG(getStubLogCategory(), "[ComponentStub].executeLocalCommand: cannot obfuscate PlatformInfo for local command " << cmdInfo.loggingName << " for component " << mComponentInfo.loggingName << " - no Message* or gCurrentUserSession is set.");
                    callerPlatform = common;
                }
            }

            rc = command->executeCommand(callerPlatform);
            if (rc == ERR_OK && command->getResponse() != nullptr && responseTdf != nullptr) 
            {
                command->getResponse()->copyInto(*responseTdf);
            }
            else if (rc != ERR_OK && command->getErrorResponse() != nullptr && errorTdf != nullptr)
            {
                command->getErrorResponse()->copyInto(*errorTdf);
            }

            delete command;   
        }
        else
        {
            EA_FAIL_MESSAGE("ComponentStub::executeLocalCommand: Cannot create command!");
            BLAZE_WARN_LOG(getStubLogCategory(), "[ComponentStub].executeLocalCommand: cannot create local command " << cmdInfo.loggingName << " for component " << mComponentInfo.loggingName);
            rc = ERR_SYSTEM;
        }
    }
    else
    {
        BLAZE_TRACE_LOG(getStubLogCategory(), "[ComponentStub].executeLocalCommand: cannot execute local command " << cmdInfo.loggingName << " because component " << mComponentInfo.loggingName << " is disabled.");
        rc = getDisabledReturnCode();
    }

    return rc;
}

void ComponentStub::addNotificationSubscription(SlaveSession& session)
{

    if (!mSlaveList.contains(session))
    {
        mSlaveList.add(session);
        session.addSessionListener(*this);
        onSlaveSessionAdded(session);
    }
    else
    {
        BLAZE_INFO_LOG(getStubLogCategory(), "ComponentStub].addNotificationSubscription: SlaveSession(" << 
            SbFormats::HexLower(session.getId()) << ") already exists in list, skipping add.");
    }
}

bool ComponentStub::loadConfig(EA::TDF::TdfPtr& newConfig, bool loadFromStaging)
{
    newConfig = mComponentInfo.baseInfo.configTdfDesc != nullptr ? mComponentInfo.baseInfo.configTdfDesc->createInstance(mAllocator, "Component Configuration") : nullptr;

    if (newConfig != nullptr)
    {
        BlazeRpcError rc = gController->getConfigTdf(getStubBaseName(), loadFromStaging, *newConfig, true);    
        if (rc != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(getStubLogCategory(), "[ComponentStub].loadConfig: Got error " << ErrorHelp::getErrorName(rc) << " fetching config for component " << mComponentInfo.loggingName);
            return false;    
        }
    }

    return true;
}

bool ComponentStub::loadPreconfig(EA::TDF::TdfPtr& newPreconfig)
{
    newPreconfig = mComponentInfo.baseInfo.preconfigTdfDesc != nullptr ? mComponentInfo.baseInfo.preconfigTdfDesc->createInstance(mAllocator, "Component Configuration") : nullptr;

    if (newPreconfig != nullptr)
    {
        BlazeRpcError rc = gController->getConfigTdf(getStubBaseName(), false, *newPreconfig, true, true);    
        if (rc != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(getStubLogCategory(), "[ComponentStub].loadConfig: Got error " << ErrorHelp::getErrorName(rc) << " fetching preconfig for component " << mComponentInfo.loggingName);
            return false;    
        }
    }

    return true;
}

void ComponentStub::sendNotificationToSliver(SliverIdentity sliverIdentity, Notification& notification) const
{
    SliverPtr sliver = gSliverManager->getSliver(sliverIdentity);
    if (sliver != nullptr)
        sliver->sendNotification(notification);
    else
    {
        BLAZE_ASSERT_LOG(getStubLogCategory(), "ComponentStub.sendNotificationToSliver: Failed to get a valid sliver with identity(" << LOG_SIDENT(sliverIdentity) << ")");
    }
}

void ComponentStub::sendNotificationToSliver(const NotificationInfo& notificationInfo, SliverIdentity sliverIdentity, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const
{
    Notification notification(notificationInfo, payload, opts);
    sendNotificationToSliver(sliverIdentity, notification);
}

void ComponentStub::sendNotificationToInstanceById(InstanceId instanceId, Notification& notification) const
{
    if (instanceId == gController->getInstanceId())
        notification.executeLocal();
    else
    {
        SlaveSession* slaveSession = mSlaveList.getSlaveSessionByInstanceId(instanceId);
        if (slaveSession != nullptr)
            slaveSession->sendNotification(notification);
    }
}

void ComponentStub::sendNotificationToInstanceById(const NotificationInfo& notificationInfo, InstanceId instanceId, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const
{
    Notification notification(notificationInfo, payload, opts);
    sendNotificationToInstanceById(instanceId, notification);
}

void ComponentStub::sendNotificationToSlaves(const NotificationInfo& notificationInfo, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts, bool notifyLocal)
{
    Notification notification(notificationInfo, payload, opts);

    if (notifyLocal)
        notification.executeLocal();

    const int32_t count = (int32_t) mSlaveList.size();
    if (count > 0)
    {
        typedef eastl::vector<SlaveSessionId> SlaveSessionIdList;
        SlaveSessionIdList sessions(mSlaveList.size(), BuildInstanceKey64(INVALID_INSTANCE_ID, 0));
        const int32_t offset = (count > 1) ? Random::getRandomNumber(count) : 0;
        SlaveSessionList::iterator it = mSlaveList.begin();
        for (int32_t idx = 0; idx < count; ++idx, ++it)
        {
            int32_t pos = (offset + idx) % count;
            sessions[pos] = it->first;
        }

        for (size_t idx = 0; idx < sessions.size(); ++idx)
        {
            SlaveSession* slaveSession = mSlaveList.getSlaveSession(sessions[idx]);
            if (slaveSession != nullptr)
                slaveSession->sendNotification(notification);
        }
    }
}

void ComponentStub::sendNotificationToSlaveSession(SlaveSession* slaveSession, Notification& notification) const
{
    if (slaveSession != nullptr)
        slaveSession->sendNotification(notification);
}

void ComponentStub::sendNotificationToSlaveSession(const NotificationInfo& notificationInfo, SlaveSession* slaveSession, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const
{
    Notification notification(notificationInfo, payload, opts);
    sendNotificationToSlaveSession(slaveSession, notification);
}

void ComponentStub::sendNotificationToUserSession(UserSessionMaster* userSession, Notification& notification) const
{
    if (userSession != nullptr)
        userSession->sendNotification(notification);
}

void ComponentStub::sendNotificationToUserSession(const NotificationInfo& notificationInfo, UserSessionMaster* userSession, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const
{
    Notification notification(notificationInfo, payload, opts);
    sendNotificationToUserSession(userSession, notification);
}

void ComponentStub::sendNotificationToUserSessionById(UserSessionId userSessionId, Notification& notification) const
{
    if (UserSession::isValidUserSessionId(userSessionId))
    {
        notification.setUserSessionId(userSessionId);
        sendNotificationToSliver(UserSession::makeSliverIdentity(userSessionId), notification);
    }
}

void ComponentStub::sendNotificationToUserSessionById(const NotificationInfo& notificationInfo, UserSessionId userSessionId, const EA::TDF::Tdf* payload, const NotificationSendOpts& opts) const
{
    Notification notification(notificationInfo, payload, opts);
    sendNotificationToUserSessionById(userSessionId, notification);
}

void ComponentStub::logRpcPreamble(const CommandInfo& cmdInfo, const Message* origMsg, const EA::TDF::Tdf* request)
{
    StringBuilder sb;
    if (BLAZE_IS_LOGGING_ENABLED(cmdInfo.componentInfo->baseInfo.index, Logging::T_RPC))
    {
        if (origMsg != nullptr)
            sb.append("%" PRIu32, origMsg->getMsgNum());
        else
            sb.append("%s", "N/A");
    }

    BLAZE_TRACE_RPC_LOG(cmdInfo.componentInfo->baseInfo.index, "<- start [component=" << cmdInfo.componentInfo->loggingName << 
        " command=" << cmdInfo.loggingName << " seqno=" << sb.get() << "]" << 
        ((request != nullptr) ? "\n" : "") << request);
}

void ComponentStub::logRpcPostamble(BlazeRpcError rc, const CommandInfo& cmdInfo, const Message* origMsg, const EA::TDF::Tdf* errorTdf)
{
    StringBuilder sb;
    if (BLAZE_IS_LOGGING_ENABLED(cmdInfo.componentInfo->baseInfo.index, Logging::T_RPC))
    {
        if (origMsg != nullptr)
            sb.append("%" PRIu32, origMsg->getMsgNum());
        else
            sb.append("%s", "N/A");
    }

    BLAZE_TRACE_RPC_LOG(cmdInfo.componentInfo->baseInfo.index, "-> finish [component=" << cmdInfo.componentInfo->loggingName << 
        " command=" << cmdInfo.loggingName << " seqno=" << sb.get() << " ec=" << 
        ErrorHelp::getErrorName(rc) << "(" << SbFormats::HexLower(rc) << ")]" <<
        ((errorTdf != nullptr) ? "\n" : "") << errorTdf);
}

void ComponentStub::tickMetrics(const PeerInfo* peerInfo, const CommandInfo& info, const TimeValue& accumulatedTime, BlazeRpcError err)
{
    if (info.index < mComponentInfo.commands.size())
    {
        ClientType clientType = peerInfo != nullptr ? peerInfo->getClientType() : CLIENT_TYPE_INVALID;
        const char8_t* endpoint = peerInfo != nullptr ? peerInfo->getEndpointName() : "none";

        CommandMetrics* metrics = mMetricsAllTime[info.index];

        metrics->timer.record(accumulatedTime, endpoint, clientType);
        
        if (err == Blaze::ERR_OK)
        {
            metrics->successCount.increment(1, endpoint, clientType);
        }
        else
        {
            metrics->failCount.increment(1, err, endpoint, clientType);
        }

        gDbMetrics->iterateDbOpCounts([metrics, endpoint, clientType](DbConnMetrics::DbOp op, uint64_t value) { metrics->dbOps.increment(value, op, endpoint, clientType); });

        gDbMetrics->iterateDbOnThreadTimes([metrics, endpoint, clientType](DbConnMetrics::DbThreadStage stage, const TimeValue& time) { metrics->dbOnThreadTime.increment(time.getMicroSeconds(), stage, endpoint, clientType); });


        metrics->dbQueryTime.increment(gDbMetrics->getQueryTime().getMicroSeconds(), endpoint, clientType);

        metrics->fiberTime.increment(Fiber::getCurrentFiberTime().getMicroSeconds(), endpoint, clientType);
        if (gRpcContext != nullptr)
        {
            for(auto& waitInfo : gRpcContext->mWaitTimes)
            {
                metrics->waitTime.increment(waitInfo.second.first.getMicroSeconds(), waitInfo.first, endpoint, clientType);
                metrics->waitCount.increment(waitInfo.second.second, waitInfo.first, endpoint, clientType);
            }
        }
    }
}

void ComponentStub::tickRpcAuthorizationFailureCount(const PeerInfo& peerInfo, const CommandInfo& info)
{
    if (info.index < mComponentInfo.commands.size())
    {
        CommandMetrics* metrics = mMetricsAllTime[info.index];
        ClientType clientType = peerInfo.getClientType();
        const char8_t* endpoint = peerInfo.getEndpointName();
        metrics->timer.record(0, endpoint, clientType);
        metrics->failCount.increment(1, ERR_AUTHORIZATION_REQUIRED, endpoint, clientType);
    }
}

void ComponentStub::gatherMetrics(ComponentMetricsResponse& metrics, CommandId commandId) const
{
    if (commandId == RPC_COMMAND_UNKNOWN)
    {
        for(auto& cmdItr : mComponentInfo.commands)
        {
            CommandMetricInfo* m = metrics.getMetrics().pull_back();
            mMetricsAllTime[cmdItr.second->index]->fillOutMetricsInfo(*m, *cmdItr.second);
        }
    }
    else
    {
        auto cmdItr = mComponentInfo.commands.find(commandId);
        if (cmdItr != mComponentInfo.commands.end())
        {
            CommandMetricInfo* m = metrics.getMetrics().pull_back();
            mMetricsAllTime[cmdItr->second->index]->fillOutMetricsInfo(*m, *cmdItr->second);
        }
    }
}


uint32_t ComponentStub::getDbId(ClientPlatformType platform) const
{
    if (platform == INVALID)
        platform = gController->getDefaultClientPlatform();

    DbIdByPlatformTypeMap::const_iterator itr = mDbIdByPlatformTypeMap.find(platform);
    if (itr == mDbIdByPlatformTypeMap.end())
        return DbScheduler::INVALID_DB_ID;

    return itr->second;
}


} //namespace
