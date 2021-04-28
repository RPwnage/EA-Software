/*************************************************************************************************/
/*!
    \file 
  

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/component/component.h"
#include "framework/component/componentmanager.h"
#include "framework/connection/connection.h"
#include "framework/connection/connectionmanager.h"
#include "framework/connection/outboundconnectionmanager.h"
#include "framework/component/blazerpc.h"
#include "blazerpcerrors.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/controller/remoteinstance.h"
#include "framework/replication/replicator.h"

namespace Blaze
{

RemoteServerInstance::RemoteServerInstance(const InetAddress& addr)
:   mInstanceId(INVALID_INSTANCE_ID),
    mRegistrationId(0),
    mInternalAddress(addr),
    mConnection(nullptr),
    mIsActive(false),
    mIsDraining(false),
    mIsShutdown(false),
    mIsMutuallyConnected(false),
    mImmediatePartialLocalUserReplication(false),
    mInstanceTypeIndex(0),
    mTotalLoad(0),
    mSystemLoad(0),
    mInstanceType(ServerConfig::UNKNOWN),
    mServiceImpact(ServerConfig::TRANSIENT),
    mHandler(nullptr),
    mFiberGroupId(Fiber::allocateFiberGroupId())
{
    blaze_strnzcpy(mInstanceName, "n/a", sizeof(mInstanceName));
    blaze_strnzcpy(mInstanceTypeName, "n/a", sizeof(mInstanceTypeName));
}

RemoteServerInstance::~RemoteServerInstance()
{
}

/*! ***************************************************************************/
/*! \brief  connect
            
            Performs a blocking connect call to the instance. If connection
            cannot be established immediately, blocks using CONNECT_RETRY_TIME
            until the connection can be established. In case the server is
            shut down, returns ERR_CANCELED.
            
    \return Blaze::BlazeRpcError 
*******************************************************************************/
BlazeRpcError RemoteServerInstance::connect()
{
    BlazeRpcError rc = ERR_OK;

    Fiber::CreateParams params;
    params.blocking = true;
    params.blocksCaller = true;
    params.groupId = mFiberGroupId;
    params.namedContext = "RemoteServerInstance::connectInternal";
    params.stackSize = Fiber::STACK_LARGE;
    gFiberManager->scheduleCall(this, &RemoteServerInstance::connectInternal, &rc, params);

    return rc;
    
}

BlazeRpcError RemoteServerInstance::connectInternal()
{
    char8_t addr[256];
    BlazeRpcError rc = Blaze::ERR_OK;
    OutboundRpcConnection* connection = nullptr;

    while (connection == nullptr)
    {
        // No connection object yet.  Create one and register to get connection updates
        connection = gController->getOutboundConnectionManager().getConnection(mInternalAddress);

        if(connection == nullptr)
        {
            connection = gController->getOutboundConnectionManager().createConnection(mInternalAddress);
        }
        else if (!connection->isConnected())
        {
            //If we have a connection and its not connected someone else is trying to connect.  Do 
            //another cycle of the loop and just rely on them to finish the job.  We'll check back in 
            //another iteration and see if its finished connecting, or try ourselves.
            connection = nullptr;            
        }

        if (!gController->getOutboundConnectionManager().isActive())
        {
            rc = ERR_CANCELED;
            break;
        }

        if (connection != nullptr)
        {
            BLAZE_TRACE_LOG(Log::CONTROLLER, "[RemoteServerInstance].connect: Connected to server @ "
                << mInternalAddress.toString(addr, sizeof(addr)));
            mConnection = connection;
            mConnection->addListener(*this);
            break;
        }

        BLAZE_INFO_LOG(Log::CONTROLLER, "[RemoteServerInstance].connect: will retry server @ "
            << mInternalAddress.toString(addr, sizeof(addr))
            << " in " << CONNECT_RETRY_TIME/1000 << "s.");

        rc = Fiber::sleep(CONNECT_RETRY_TIME * 1000, "RemoteInstance::connect sleep while connecting");
        
        if (rc != Blaze::ERR_OK)
            break;
    }
    
    return rc; 
}

/*! ***************************************************************************/
/*! \brief  disconnect

            Detaches RemoteServerInstance from the Connection object and 
            synchronously signals the listener. (NO-OP if already disconnected)
            
    \return void 
*******************************************************************************/
void RemoteServerInstance::disconnect()
{
    mIsActive = false;
    
    if (mConnection != nullptr)
    {
        mConnection->removeListener(*this);
        
        // safe to nullptr out, ConnectionManager owns it, and will clean it up
        mConnection = nullptr;

        // notify listener about disconnection
        if (mHandler != nullptr)
            mHandler->onRemoteInstanceStateChanged(*this);
    }
}

/*! ***************************************************************************/
/*! \brief  initialize
            
            Performs an RPC call to the instance that fetches relevant information
            stored within it (instance id, name, type, components, etc.)
            
    \return Blaze::BlazeRpcError 
*******************************************************************************/
BlazeRpcError RemoteServerInstance::initialize()
{
    if (!isConnected())
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[RemoteServerInstance].initialize: Cannot initialize a disconnected instance");
        return ERR_SYSTEM;
    }


    BlazeRpcError rc = ERR_OK;

    Fiber::CreateParams params;
    params.blocking = true;
    params.blocksCaller = true;
    params.groupId = mFiberGroupId;
    params.namedContext = "RemoteServerInstance::initializeInternal";
    params.stackSize = Fiber::STACK_LARGE;
    gFiberManager->scheduleCall(this, &RemoteServerInstance::initializeInternal, &rc, params);

    return rc;
}

BlazeRpcError RemoteServerInstance::initializeInternal()
{      
    //Here's an interesting problem - we don't have the info we need to create a proxy, but we need to send a message to get
    //that proxy.  So we do this by directly calling the RPC on the connection.
    InstanceId movedTo = INVALID_INSTANCE_ID;
    GetInstanceInfoResponse resp;

    mConnection->setContextOverride(gController->getInstanceSessionId());

    // Note, the call to sendRequest() can actually end up setting mConnection to nullptr if the send attempt results in ERR_DISCONNECTED.
    BlazeRpcError rc = mConnection->sendRequest(Controller::COMPONENT_ID, Controller::CMD_GETINSTANCEINFO, nullptr, &resp, nullptr, movedTo);
    if (rc == ERR_OK)
    {
        ServerInstanceInfo::EndpointInfo& ep = resp.getInstanceInfo().getInternalEndpoint();
        InetAddress remoteInternalAddr(ep.getIntAddress(), ep.getPort(), InetAddress::NET);
        if (mInternalAddress == remoteInternalAddr)
        {
            setInstanceId(resp.getInstanceInfo().getInstanceId());
            setInstanceName(resp.getInstanceInfo().getInstanceName());
            setInstanceTypeName(resp.getInstanceInfo().getInstanceType());
            setInstanceTypeIndex(resp.getInstanceInfo().getInstanceTypeIndex());
            setRegistrationId(resp.getInstanceInfo().getRegistrationId());
            setServiceImpact(resp.getInstanceInfo().getServiceImpact());
            setImmediatePartialLocalUserReplication(resp.getInstanceInfo().getImmediatePartialLocalUserReplication());
            mStartupTime = resp.getInstanceInfo().getStartupTime();

            // Set the instance id for our connection now that we have it.  From now on it can be looked up by the id.
            gController->getOutboundConnectionManager().bindConnectionToInstanceId(*mConnection, getInstanceId());
            mConnection->setPeerName(getInstanceName());
            setDraining(resp.getInstanceInfo().getIsDraining());

            resp.getInstanceInfo().getEndpointInfoList().copyInto(mEndpointList);

            // Set peer name if the corresponding inbound connection has been done already
            SlaveSessionPtr slaveSession = gController->getConnectionManager().getSlaveSessionByInstanceId(getInstanceId());
            if ((slaveSession != nullptr) && (slaveSession->getConnection() != nullptr))
                slaveSession->getConnection()->setPeerName(getInstanceName());

            if (mInstanceId != gController->getInstanceId())
            {
                for (ServerInstanceInfo::ComponentMap::const_iterator 
                    i = resp.getInstanceInfo().getComponents().begin(),
                    e = resp.getInstanceInfo().getComponents().end(); 
                    i != e; ++i)
                {
                    ComponentId componentId = i->second;
                    if (!Component::isMasterComponentId(componentId))
                        continue;
                    Component* component = gController->getComponentManager().getLocalComponent(componentId);
                    if (component != nullptr && !component->hasMultipleInstances())
                    {
                        char8_t nameBuf[128];
                        BLAZE_WARN_LOG(Log::CONTROLLER, "[RemoteServerInstance].initialize: "
                            "Unsharded remote master component: " << component->getName() 
                            << " is already hosted locally, rejecting remote instance: " 
                            << toString(nameBuf, sizeof(nameBuf)));
                        rc = CTRL_ERROR_SERVER_INSTANCE_ALREADY_HOSTS_COMPONENT;
                        break;
                    }
                }
                if (rc == ERR_OK)
                {
                    for (ServerInstanceInfo::ComponentMap::const_iterator 
                        i = resp.getInstanceInfo().getComponents().begin(),
                        e = resp.getInstanceInfo().getComponents().end(); 
                    i != e; ++i)
                    {
                        mComponentIds.insert(i->second);
                    }
                }
            }
            else
            {
                char8_t nameBuf[128];
                BLAZE_TRACE_LOG(Log::CONTROLLER, "[RemoteServerInstance].initialize: "
                    "Skip connecting to self: " << toString(nameBuf, sizeof(nameBuf)));
                rc = CTRL_ERROR_SERVER_INSTANCE_ALREADY_REGISTERED;
            }
        }
        else
        {
            char8_t addrBuf1[128];
            char8_t addrBuf2[128];
            BLAZE_ERR_LOG(Log::CONTROLLER, "[RemoteServerInstance].initialize: "
                "Internal endpoint information '" << remoteInternalAddr.toString(addrBuf1, sizeof(addrBuf1)) 
                << "' returned by remote server instance '" << resp.getInstanceInfo().getInstanceName() << ":"
                << resp.getInstanceInfo().getInstanceId() << "' differs from the endpoint information '"
                << mInternalAddress.toString(addrBuf2, sizeof(addrBuf2)) << "' used to establish "
                "outbound connection.");
            rc = ERR_SYSTEM;
        }
    }
    
    return rc;
}

/*! ***************************************************************************/
/*! \brief  activate
            
            Activates and registers all components belonging to this instance
            
    \return Blaze::BlazeRpcError 
*******************************************************************************/
BlazeRpcError RemoteServerInstance::activate()
{
    BlazeRpcError rc;

    Fiber::CreateParams params;
    params.blocking = true;
    params.blocksCaller = true;
    params.groupId = mFiberGroupId;
    params.namedContext = "RemoteServerInstance::activateInternal";
    params.stackSize = Fiber::STACK_LARGE;
    gFiberManager->scheduleCall(this, &RemoteServerInstance::activateInternal, &rc, params);

    return rc;
}

BlazeRpcError RemoteServerInstance::activateInternal()
{
    BlazeRpcError rc = ERR_SYSTEM;

    for (ComponentIdSet::const_iterator i = mComponentIds.begin(), e = mComponentIds.end(); i != e; ++i)
    {
        rc = gController->getComponentManager().addRemoteComponent(*i, mInstanceId);;
        if (rc != ERR_OK)
            break;
    }

    if (rc == ERR_OK)
    {
        // notify listener about activation
        if (mHandler != nullptr)
            mHandler->onRemoteInstanceStateChanged(*this);
        mIsActive = true;
    }

    return rc;
}

void RemoteServerInstance::setDraining(bool draining)
{
    if (draining != mIsDraining)
    {
        mIsDraining = draining;

        //loop over all components and tell the component manager to mark them draining/not draining
        for (ComponentIdSet::const_iterator itr = mComponentIds.begin(), end = mComponentIds.end(); itr != end; ++itr)
        {
            Component* comp = gController->getComponentManager().getComponent(*itr);
            if (comp != nullptr)
            {
                comp->setInstanceDraining(mInstanceId, mIsDraining);
            }
        }
    }
}

void RemoteServerInstance::setLoad(uint32_t totalLoad, uint32_t systemLoad)
{
    mTotalLoad = totalLoad < MAX_LOAD ? totalLoad : MAX_LOAD; // Always clamp load to max value
    mSystemLoad = systemLoad < mTotalLoad ? systemLoad : mTotalLoad; // Always clamp to total load
}

/*! ***************************************************************************/
/*! \brief  shutdown

            Performs a non-blocking disconnect followed by waiting for all
            ongoing operations to complete, then performing a blocking 
            remote replicated map cleanup.
            
    \return bool 
*******************************************************************************/
bool RemoteServerInstance::shutdown()
{
    if (mIsShutdown)
    {
        return false;
    }
    
    mIsShutdown = true;

    clearListener();
    disconnect();

    //Kill all our outstanding fibers for any other task
    Fiber::cancelGroup(mFiberGroupId, ERR_CANCELED);

    // deregister all components
    for (ComponentIdSet::const_iterator i = mComponentIds.begin(), e = mComponentIds.end(); i != e; ++i)
    {
        gController->getComponentManager().removeRemoteComponent(*i, mInstanceId);
    }

    if (!gReplicator->isShuttingDown())
    {
        // trigger replicated map cleanup on all components
        BlazeRpcError rc;
        for (ComponentIdSet::const_iterator i = mComponentIds.begin(), e = mComponentIds.end(); i != e; ++i)
        {
            // only try to clean up replicated maps if we actually activated the component
            Component* comp = gController->getComponentManager().getComponent(*i);
            if (comp != nullptr && comp->hasComponentReplication())
            {
                rc = gReplicator->cleanupRemoteCollection(*comp, mInstanceId);
                if (rc != ERR_OK)
                {
                    char8_t nameBuf[256];
                    BLAZE_WARN_LOG(Log::CONTROLLER, 
                        "[RemoteServerInstance].shutdown: "
                        "Error while cleaning remote collection "
                        << BlazeRpcComponentDb::getComponentNameById(*i) << " for remote instance "
                        << toString(nameBuf, sizeof(nameBuf)) 
                        << ", rc: " << ErrorHelp::getErrorName(rc));
                    continue;
                }
            }
        }
    }

    return true;
}

BlazeRpcError RemoteServerInstance::checkMutualConnectivity()
{
    BlazeRpcError rc;

    Fiber::CreateParams params;
    params.blocking = true;
    params.blocksCaller = true;
    params.groupId = mFiberGroupId;
    params.namedContext = "RemoteServerInstance::checkMutualConnectivityInternal";
    params.stackSize = Fiber::STACK_LARGE;
    gFiberManager->scheduleCall(this, &RemoteServerInstance::checkMutualConnectivityInternal, &rc, params);

    return rc;
}

BlazeRpcError RemoteServerInstance::checkMutualConnectivityInternal()
{
    BlazeRpcError rc = ERR_SYSTEM;

    if (hasComponent(Controller::COMPONENT_ID))
    {
        rc = gController->checkClusterMembershipOnRemoteInstance(mInstanceId); // Ask remote instance if it knows us and think we are ACTIVE.
        if (rc == ERR_OK)
        {
            mIsMutuallyConnected = true;
        }
    }
    else
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, 
            "[RemoteServerInstance].checkMutualConnectivity: missing remote controller proxy, id="
            << mInstanceId << ", name=" << mInstanceName);
    }
    
    return rc;
}

void RemoteServerInstance::onConnectionDisconnected(Connection& connection)
{
    if (&connection == mConnection)
    {
        disconnect();
    }
}


const char8_t* RemoteServerInstance::toString(char8_t* buf, uint32_t bufSize) const
{
    int32_t count;
    if (mInstanceId != INVALID_INSTANCE_ID)
        count = blaze_snzprintf(buf, bufSize, "%s:%" PRIu32 "@", mInstanceName, (uint32_t)mInstanceId);
    else
        count = blaze_snzprintf(buf, bufSize, "-:-@");
    mInternalAddress.toString(buf + count, bufSize - count);
    return buf;
}

void RemoteServerInstance::setInstanceId(InstanceId instanceId)
{
    mInstanceId = instanceId;
}

void RemoteServerInstance::setRegistrationId(uint64_t registrationId)
{
    mRegistrationId = registrationId;
}

void RemoteServerInstance::setInstanceName(const char8_t* name)
{
    blaze_strnzcpy(mInstanceName, name, sizeof(mInstanceName));
}

void RemoteServerInstance::setImmediatePartialLocalUserReplication(bool immediate)
{
    mImmediatePartialLocalUserReplication = immediate;
}

void RemoteServerInstance::setInstanceTypeName(const char8_t* typeName)
{
    blaze_strnzcpy(mInstanceTypeName, typeName, sizeof(mInstanceTypeName));
    const BootConfig::ServerConfigsMap& map = gProcessController->getBootConfigTdf().getServerConfigs();
    BootConfig::ServerConfigsMap::const_iterator it = map.find(mInstanceTypeName);
    if (it != map.end())
    {
        mInstanceType = it->second->getInstanceType();
    }
    else
    {
        mInstanceType = ServerConfig::UNKNOWN;
    }
}

void RemoteServerInstance::setServiceImpact(ServerConfig::ServiceImpact impact)
{
    mServiceImpact = impact;
}

void RemoteServerInstance::setInstanceTypeIndex(uint8_t typeIndex)
{
    mInstanceTypeIndex = typeIndex;
}

void RemoteServerInstance::setListener(RemoteServerInstanceHandler& listener)
{
    mHandler = &listener;
}

void RemoteServerInstance::clearListener()
{
    mHandler = nullptr;
}

bool RemoteServerInstance::getAddressForEndpointType(bool isInternalAddr, InetAddress& inetAddress, BindType bindType, const char8_t* channel, const char8_t* encoder, const char8_t* decoder) const
{
    for (ServerInstanceInfo::ServerEndpointInfoList::const_iterator it = mEndpointList.begin(), end = mEndpointList.end(); it != end; ++it)
    {
        const Redirector::ServerEndpointInfo* epi = *it;
        if ((epi->getBindType() == bindType) &&
            (blaze_strcmp(channel, epi->getChannel()) == 0) &&
            (blaze_strcmp(encoder, epi->getEncoder()) == 0) &&
            (blaze_strcmp(decoder, epi->getDecoder()) == 0))
        {
            for (Redirector::ServerEndpointInfo::ServerAddressInfoList::const_iterator addrIt = epi->getAddresses().begin(), addrEnd = epi->getAddresses().end(); addrIt != addrEnd; ++addrIt)
            {
                const Redirector::ServerAddressInfo* addrInfo = *addrIt;
                if (addrInfo->getAddress().getActiveMember() == Redirector::ServerAddress::MEMBER_IPADDRESS)
                {
                    bool isCurAddrInteral = (addrInfo->getType() == Redirector::INTERNAL_IPPORT);
                    if (isCurAddrInteral == isInternalAddr)
                    {
                        const Redirector::IpAddress& addr = *addrInfo->getAddress().getIpAddress();
                        inetAddress = InetAddress(addr.getIp(), addr.getPort(), InetAddress::HOST);
                        inetAddress.setHostname(addr.getHostname());
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

} // Blaze

