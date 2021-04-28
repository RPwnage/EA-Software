
#include "Ignition/Connection.h"
#include "Ignition/LoginAndAccountCreation.h"
#include "BlazeSDK/component/redirectorcomponent.h"

namespace Ignition
{

Connection::Connection()
    : BlazeHubUiBuilder("Connection"),
      mLoginParameters(nullptr),
      mIsExpressLogin(false)
{
    getActions().add(&getActionConnect());
    getActions().add(&getActionConnectAndStartLogin());
    getActions().add(&getActionConnectAndExpressLogin());
    getActions().add(&getActionDisconnect());
    getActions().add(&getActionDirtyDisconnect());
    getActions().add(&getActionSetNetworkQosDataDebug());
    getActions().add(&getActionSetQosPingSiteLatencyDebug());
    getActions().add(&getActionSetClientState());
    getActions().add(&getActionGetDrainStatus());
    getActions().add(&getActionSetNewServiceNameAndEnvironment());
    getActions().add(&getActionSetClientPlatformOverride());

    gBlazeHub->getConnectionManager()->addListener(this);
}

Connection::~Connection()
{
    gBlazeHub->getConnectionManager()->removeListener(this);
}

void Connection::onAuthenticatedPrimaryUser()
{
    getActionSetClientState().setIsVisible(true);
}

void Connection::onDeAuthenticatedPrimaryUser()
{
    getActionSetClientState().setIsVisible(false);
}

/*! ************************************************************************************************/
/*! \brief The BlazeSDK has made a connection to the blaze server.  This is user independent, as
    all local users share the same connection.
***************************************************************************************************/
void Connection::onConnected()
{
    REPORT_LISTENER_CALLBACK(nullptr);

    Pyro::UiNodeValueContainer &sessionValues = getUiDriver()->getMainValues();

    sessionValues["Blaze Server Version"] = gBlazeHub->getConnectionManager()->getServerVersion();
    sessionValues["Blaze Server Address"] = gBlazeHub->getConnectionManager()->getServerAddress();

    getUiDriver()->setCurrentStatus_va(
        "The BlazeHub is connected to %s in %s",
        gBlazeHub->getInitParams().ServiceName,
        Blaze::EnvironmentTypeToString(gBlazeHub->getInitParams().Environment));

    setIsVisibleForAll(false);
    getActionDisconnect().setIsVisible(true);
    getActionDirtyDisconnect().setIsVisible(true);
    getActionSetNetworkQosDataDebug().setIsVisible(true);
    getActionSetQosPingSiteLatencyDebug().setIsVisible(true);
    getActionGetDrainStatus().setIsVisible(true);
    getActionSetClientPlatformOverride().setIsVisible(false);

    if (mLoginParameters != nullptr)
    {
        LocalUserUiDriver *localUserUiDriver = (LocalUserUiDriver *)getUiDriverMaster()->getSlaves().findById_va("%" PRIu32, 0);
        if (localUserUiDriver != nullptr)
        {
            if (mIsExpressLogin)
                localUserUiDriver->getLoginAndAccountCreation().expressLoginUser(mLoginParameters);
            else
                localUserUiDriver->getLoginAndAccountCreation().loginUser(mLoginParameters);
        }
        mLoginParameters = nullptr;
    }
}

void Connection::onDisconnected()
{
    REPORT_LISTENER_CALLBACK(nullptr);

    Pyro::UiNodeValueContainer &sessionValues = getUiDriver()->getMainValues();

    sessionValues["Blaze Server Version"] = "";
    sessionValues["Blaze Server Address"] = "";

    getUiDriver()->setCurrentStatus("Not Connected");

    setIsVisibleForAll(false);
    getActionConnect().setIsVisible(true);
    getActionConnectAndStartLogin().setIsVisible(true);
    getActionConnectAndExpressLogin().setIsVisible(true);
    getActionSetNewServiceNameAndEnvironment().setIsVisible(true);
    getActionSetClientPlatformOverride().setIsVisible(true);

    mLoginParameters = nullptr;
}

void Connection::initActionConnect(Pyro::UiNodeAction &action)
{
    action.setText("Connect");
    action.setDescription("Connect to the Blaze server");
}

void Connection::actionConnect(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(gBlazeHub->getConnectionManager()->connect(
        Blaze::ConnectionManager::ConnectionManager::ConnectionMessagesCb(this, &Connection::connectionMessagesCb),
        false));
}

void Connection::initActionConnectAndStartLogin(Pyro::UiNodeAction &action)
{
    action.setText("Connect and Start Login");
    action.setDescription("Connect and Start Login");

    Ignition::LoginAndAccountCreation::setupCommonLoginParameters(action);

#if defined(BLAZE_USING_EADP_SDK)
    action.getParameters().addEnum("loginby", AUTH_CODE, "Login By", "Login authentication option", Pyro::ItemVisibility::ADVANCED);
#endif
}

void Connection::actionConnectAndStartLogin(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    mLoginParameters = parameters;
    mIsExpressLogin = false;
    actionConnect(action, parameters);
}

void Connection::initActionConnectAndExpressLogin(Pyro::UiNodeAction &action)
{
    action.setText("Connect and Express Login");
    action.setDescription("Connect and Express Login");

    Ignition::LoginAndAccountCreation::setupExpressLoginParameters(action);
    Ignition::LoginAndAccountCreation::setupCommonLoginParameters(action);
}

void Connection::actionConnectAndExpressLogin(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    mLoginParameters = parameters;
    mIsExpressLogin = true;
    actionConnect(action, parameters);
}

void Connection::connectionMessagesCb(Blaze::BlazeError blazeError, const Blaze::Redirector::DisplayMessageList *displayMessageList)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    for (Blaze::Redirector::DisplayMessageList::const_iterator i = displayMessageList->begin();
        i != displayMessageList->end();
        i++)
    {
        getUiDriver()->addLog_va("%s", (*i).c_str());

        //display the redirector message to the end user
        getUiDriver()->showMessage_va(
            Pyro::ErrorLevel::NONE,
            "Redirector Message",
            "%s\r\n",
            (*i).c_str());
    }
    
    const Blaze::ConnectionManager::ConnectionStatus& connStatus = gBlazeHub->getConnectionManager()->getConnectionStatus();
    uint32_t uConnStatus = (uint32_t)(connStatus.mNetConnStatus);

    getUiDriver()->addLog_va("ExtendedError=%s", gBlazeHub->getErrorName(connStatus.mError));
    getUiDriver()->addLog_va("NetConnStatus=%c%c%c%c", (uConnStatus>>24)&0xff, (uConnStatus>>16)&0xff, (uConnStatus>> 8)&0xff, (uConnStatus>> 0)&0xff);
    getUiDriver()->addLog_va("ProtoSSLError=%d",connStatus.mProtoSslError);
    getUiDriver()->addLog_va("SocketError=%d",connStatus.mSocketError);

    //For Ignition, we do not want to report further error when the server isn't found because the server is down during maintenance
    if (blazeError != Blaze::REDIRECTOR_SERVER_NOT_FOUND && blazeError != Blaze::REDIRECTOR_NO_MATCHING_INSTANCE)
    {
       REPORT_BLAZE_ERROR(blazeError);
    }
}

void Connection::onQosPingSiteLatencyRetrieved(bool success)
{
    getUiDriver()->addLog_va(success ? "QoS ping site latency retrieved" : "QoS ping site latency was not reteived");
}

void Connection::initActionDisconnect(Pyro::UiNodeAction &action)
{
    action.setText("Disconnect");
    action.setDescription("Disconnect from the Blaze server");
}

void Connection::actionDisconnect(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(gBlazeHub->getConnectionManager()->disconnect());
}

void Connection::initActionDirtyDisconnect(Pyro::UiNodeAction &action)
{
    action.setText("Dirty Disconnect");
    action.setDescription("Disconnect from the Blaze server");
}

void Connection::actionDirtyDisconnect(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(gBlazeHub->getConnectionManager()->dirtyDisconnect());
}

void Connection::initActionSetNetworkQosDataDebug(Pyro::UiNodeAction &action)
{
    action.setText("Override Network QoS Data");
    action.setDescription("Override the Network QoS data for debug purposes.");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);

    action.getParameters().addEnum("natType", Blaze::Util::NAT_TYPE_OPEN, "NAT Type", "The NAT type", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addUInt32("downstreamBps", 0, "down stream Bps", "", "", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addUInt32("upstreamBps", 0, "upstream stream Bps", "", "", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addUInt32("profileversion", 0, "the version of the qos profile served up by the coordinator", "", "", Pyro::ItemVisibility::ADVANCED);
}

void Connection::actionSetNetworkQosDataDebug(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Util::NetworkQosData networkQosData;
    Blaze::Util::NatType natType;

    Blaze::Util::ParseNatType(parameters["natType"], natType);

    networkQosData.setNatType(natType);
    networkQosData.setDownstreamBitsPerSecond(parameters["downstreamBps"]);
    networkQosData.setUpstreamBitsPerSecond(parameters["upstreamBps"]);
    networkQosData.setQosProfileVersion(parameters["profileversion"]);

    vLOG(gBlazeHub->getConnectionManager()->setNetworkQosDataDebug(networkQosData, true));
}

void Connection::initActionSetQosPingSiteLatencyDebug(Pyro::UiNodeAction &action)
{
    action.setText("Override QoS Ping Site Latency Data");
    action.setDescription("Override the QoS ping site latency data for debug purposes.");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);

    Pyro::UiNodeParameter* newParam = new Pyro::UiNodeParameter("map");
    newParam->setVisibility(Pyro::ItemVisibility::REQUIRED);
    newParam->setText("QoS ping site latency data override");

    newParam->setAsDataNodePtr(new Pyro::UiNodeParameterMap());
    Pyro::UiNodeParameterMapPtr newMap = newParam->getAsMap();

    // Create the map's key & value types:
    Pyro::UiNodeParameter* keyTypeParam = new Pyro::UiNodeParameter("mapkey");
    keyTypeParam->setVisibility(Pyro::ItemVisibility::REQUIRED);
    keyTypeParam->setText("Ping site");
    keyTypeParam->setAsString("");
    Pyro::UiNodeParameter* valueTypeParam = new Pyro::UiNodeParameter("mapvalue");
    valueTypeParam->setVisibility(Pyro::ItemVisibility::REQUIRED);
    valueTypeParam->setText("Latency");
    valueTypeParam->setAsString("");

    // Create the key value pair:
    Pyro::UiNodeParameterMapKeyValuePair* newKeyValueType = new Pyro::UiNodeParameterMapKeyValuePair();

    // Associate it with the underlying data type: 
    newKeyValueType->setMapKey(keyTypeParam);
    newKeyValueType->setMapValue(valueTypeParam);
    newMap->setKeyValueType(newKeyValueType);
    action.getParameters().add(newParam);
}

void Connection::actionSetQosPingSiteLatencyDebug(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::PingSiteLatencyByAliasMap pingSiteLatency;
    Pyro::UiNodeParameterMapPtr map = parameters["map"].getAsMap();
    for (int32_t i = 0; i < map->getCount(); ++i)
    {
        Pyro::UiNodeParameterMapKeyValuePair* uiMapKeyValuePair = map->at(i);
        const char8_t* key = uiMapKeyValuePair->getMapKey()->getAsString();
        int32_t value = uiMapKeyValuePair->getMapValue()->getAsInt32();

        pingSiteLatency[key] = value;
    }

    vLOG(gBlazeHub->getConnectionManager()->setQosPingSiteLatencyDebug(pingSiteLatency));
}

void Connection::initActionGetPermissions(Pyro::UiNodeAction &action)
{
    action.setText("Get Permissions");
}

void Connection::actionGetPermissions(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //gBlazeHub->getComponentManager()->getUserSessions()->getPermissions();
}

void Connection::initActionGetDrainStatus(Pyro::UiNodeAction &action)
{
    action.setText("Get Drain Status");
}

void Connection::actionGetDrainStatus(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //getUiDriver()->showMessage_va(
    //    Pyro::ErrorLevel::NONE,
    //    "Server Drain Status",
    //    "Server is %sdraining.\r\n",
    //    gBlazeHub->getConnectionManager()->isDraining() ? "" : "not ");
}

void Connection::initActionSetClientState(Pyro::UiNodeAction &action)
{
    action.setText("Set Client State");

    action.getParameters().addEnum("status", Blaze::ClientState::STATUS_NORMAL, "Status", "Client Status");
    action.getParameters().addEnum("mode", Blaze::ClientState::MODE_UNKNOWN, "Mode", "Client Mode");
}

void Connection::actionSetClientState(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::ClientState req;
    req.setStatus(parameters["status"]);
    req.setMode(parameters["mode"]);
    vLOG(gBlazeHub->getComponentManager()->getUtilComponent()->setClientState(req));
}

void Connection::initActionSetNewServiceNameAndEnvironment(Pyro::UiNodeAction &action)
{
    action.setText("Set New Service Name and Environment");
    action.setDescription("Change the service name and EnvironmentType of the Blaze server to connect to");

#ifndef BLAZESDK_CONSOLE_PLATFORM
    action.getParameters().addString("serviceName", "", "Service Name", nullptr, "The service name of the Blaze server to connect to");
    action.getParameters().addEnum("environmentType", Blaze::ENVIRONMENT_SDEV, "Environment Type", "The EnvironmentType where the Blaze server is running");
#endif
}

void Connection::actionSetNewServiceNameAndEnvironment(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(gBlazeHub->setServiceName(parameters["serviceName"]));
    vLOG(gBlazeHub->setEnvironment(parameters["environmentType"]));

    char8_t  serviceName[Blaze::BlazeHub::SERVICE_NAME_MAX_LENGTH];
    strncpy(serviceName, parameters["serviceName"], Blaze::BlazeHub::SERVICE_NAME_MAX_LENGTH - 1);

    getUiDriver()->getMainValue("Service Name") = serviceName;
    getUiDriver()->getMainValue("Environment") = Blaze::EnvironmentType(parameters["environmentType"]);

}

void Connection::initActionSetClientPlatformOverride(Pyro::UiNodeAction &action)
{
    action.setText("Set Client Platform");
    action.setDescription("Override the current client platform.");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);

    action.getParameters().addEnum("clientPlatform", Blaze::ClientPlatformType::NATIVE, "Platform", "Platform Override", Pyro::ItemVisibility::ADVANCED);
}

void Connection::actionSetClientPlatformOverride(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::ClientPlatformType clientPlatform;

    Blaze::ParseClientPlatformType(parameters["clientPlatform"], clientPlatform);
    vLOG(gBlazeHub->getConnectionManager()->setClientPlatformTypeOverride(clientPlatform));
}

}
