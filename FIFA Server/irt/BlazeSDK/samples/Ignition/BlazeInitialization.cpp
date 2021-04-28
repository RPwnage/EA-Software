
#include "Ignition/BlazeInitialization.h"

#include "Ignition/Connection.h"
#include "Ignition/Census.h"
#include "Ignition/Clubs.h"
#include "Ignition/GameManagement.h"
#include "Ignition/GamePlay.h"
#include "Ignition/Leaderboards.h"
#include "Ignition/LoginAndAccountCreation.h"
#include "Ignition/Messaging.h"
#include "Ignition/Stats.h"
#if !defined(EA_PLATFORM_NX)
#include "Ignition/voip/VoipConfig.h"
#endif
#include "Ignition/custom/dirtysockdev.h"
#include "Ignition/AssociationLists.h"

#include "BlazeSDK/version.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/component/framework/tdf/qosdatatypes.h"
#include "EATDF/time.h"

#if defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
#include <player_invitation_dialog.h>
#endif

namespace Ignition
{

void BlazeInitialization::BlazeSdkLogFunction(const char8_t *pText, void *data)
{
    BlazeInitialization *obj = (BlazeInitialization *)data;
    obj->getUiDriver()->addLog_va("[Blaze] ", Pyro::ErrorLevel::NONE, "%s", pText);
    #if defined(EA_PLATFORM_MICROSOFT)
        OutputDebugStringA(pText);
    #else
        printf("%s", pText);
    #endif
}

int32_t BlazeInitialization::DirtySockLogFunction(void *pParm, const char8_t *pText)
{
    BlazeInitialization *obj = (BlazeInitialization *)pParm;
    obj->getUiDriver()->addLog_va("[Dirty] ", Pyro::ErrorLevel::NONE, "%s", pText);
    #if defined(EA_PLATFORM_MICROSOFT)
        OutputDebugStringA(pText);
    #else
        printf("%s", pText);
    #endif
    return 0;
}

BlazeInitialization::BlazeInitialization()
    :  BlazeHubUiBuilder("Blaze Initialization")
{
    Blaze::Debug::setLogBufferingEnabled(true);

    getActions().add(&getActionStartupBlaze());
    getActions().add(&getActionShutdownBlaze());

    getActionShutdownBlaze().setIsVisible(false);
}

BlazeInitialization::~BlazeInitialization()
{
}

void BlazeInitialization::initActionStartupBlaze(Pyro::UiNodeAction &action)
{
    action.setText("Startup Blaze");
    action.setDescription("Initializes the BlzeHub (BlazeSDK).");

    Pyro::UiNodeParameterStruct &parameters = action.getParameters();

    const char8_t *category = "General Blaze Settings";
    parameters.addString("serviceName", "blazesample-latest-pc", "Service Name", nullptr, "The service name of the Blaze server to connect to", Pyro::ItemVisibility::SIMPLE, category);
    parameters.addEnum("environmentType", Blaze::ENVIRONMENT_STEST, "Environment Type", "The EnvironmentType where the Blaze server is running", Pyro::ItemVisibility::SIMPLE, category);
    parameters.addEnum("clientType", Blaze::CLIENT_TYPE_GAMEPLAY_USER, "Client Type", "The ClientType to emulate", Pyro::ItemVisibility::ADVANCED, category);
    parameters.addString("locale", Blaze::LOCALE_DEFAULT_STR, "Locale", nullptr, "The local to initialize the BlazeHub with (enUS, frCA, esMX, etc)", Pyro::ItemVisibility::ADVANCED, category);
    parameters.addBool("useSecureConnection", false, "Use Secure Connection", "Use an encrypted SSL connection when connecting to the Blaze server", Pyro::ItemVisibility::ADVANCED, category);
    parameters.addBool("usePlainTextTos", true, "Use Plain-Text TOS", "Retrieve the Terms of Service and Privacy Policy as plain text documents (otherwise, return as HTML)", Pyro::ItemVisibility::ADVANCED, category);
    parameters.addBool("useExternalLoginFlow", false, "Use External Login Flow", "Use the external (Nucleus portal) if the initial login attempt fails", Pyro::ItemVisibility::ADVANCED, category);
    parameters.addUInt32("userCount", 1, "Number of Users", "1,2,3,4", "Number of users on this device.", Pyro::ItemVisibility::ADVANCED, category);
    parameters.addBool("ignoreInactivityTimeout", false, "Ignore Inactivity Timeout", "When true, the Blaze server will not disconnect the client due to inactivity (e.g. caused by debugging).  The Blaze server endpoint must have enableInactivityTimeoutBypass set to true.", Pyro::ItemVisibility::ADVANCED, category);
    parameters.addInt32("maxNumTunnels", 32, "Max Tunnels", "8,16,24,32", "How many tunnels can be used, passed to ProtoTunnelCreate()", Pyro::ItemVisibility::ADVANCED, category);
#if !defined(EA_PLATFORM_NX)
    parameters.addInt32("maxNumVoipPeers", 8, "Max number of voip peers", "8,16,24,30", "Max number of voip peers passed to VoipStartup().", Pyro::ItemVisibility::ADVANCED, category);
#endif
    parameters.addBool("enableQoS", true, "Enable QoS", "Enable the QoS checks which also populate the external IP addresses for the client.", Pyro::ItemVisibility::ADVANCED, category);
    parameters.addBool("enableNetConnIdle", true, "Enable NetConnIdle", "Enable the call to NetConnIdle from the Blaze SDK ConnectionManager", Pyro::ItemVisibility::ADVANCED, category);
    Pyro::UiNodeParameter& param = parameters.addUInt16("gamePort", Blaze::BlazeHub::DEFAULT_GAME_PORT , "Game Port", nullptr, "The game port to use for this hub", Pyro::ItemVisibility::ADVANCED, category);
    param.setIncludePreviousValues(false);

    category = "DirtySDK Settings";
    parameters.addBool("enableDemangler", true, "Enable the Demangler", "Enable the DirtySDK demangler", Pyro::ItemVisibility::ADVANCED, category);

    parameters.addBool("multipleVirtualMachines", false, "Multiple Virtual Machines", "TRUE if multiple virtual machines are allowed. FALSE otherwise", Pyro::ItemVisibility::ADVANCED, category);

    category = "GameManager Settings";
    parameters.addString("gameProtocolVersion", "", "Game Protocol Version", nullptr, "All users in a game must have the same Game Protocol Version", Pyro::ItemVisibility::ADVANCED, category);
    parameters.addBool("preventMultipleGameMembership", false, "Prevent Multiple Game Membership", "If true, GameManager will automatically remove the local player from previous games when joining a new one.", Pyro::ItemVisibility::ADVANCED, category);

    category = "Redirector Override";
    parameters.addString("redirectorAddress", "", "Address", nullptr, "Specify a network address to use as the redirector.  Leave blank to use the default redirector.", Pyro::ItemVisibility::ADVANCED, category);
    parameters.addBool("redirectorSecure", true, "Secure", "When overriding the redirector address, this will be used to indicate if the connection should use SSL.", Pyro::ItemVisibility::ADVANCED, category);
    parameters.addUInt16("redirectorPort", 0, "Port", nullptr, "When overriding the redirector address, this will be used to indicate the port to connect to.  Set to 0 to the default port.", Pyro::ItemVisibility::ADVANCED, category);
}

void BlazeInitialization::actionStartupBlaze(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
#if DIRTYCODE_LOGGING
    NetPrintfHook(DirtySockLogFunction, this);
#endif

    VoipStartup(20, 0);

    Blaze::BlazeHub::InitParameters initParameters;
    strncpy(initParameters.ServiceName, parameters["serviceName"], Blaze::BlazeHub::SERVICE_NAME_MAX_LENGTH - 1);
    strncpy(initParameters.AdditionalNetConnParams, "-guest=true", Blaze::BlazeHub::CONNECT_PARAMS_MAX_LENGTH - 1);
    initParameters.Environment = parameters["environmentType"];
    initParameters.Client = parameters["clientType"];
    initParameters.Locale = static_cast<uint32_t>(LocaleTokenCreateFromString(parameters["locale"].getAsString()));
    initParameters.UserCount = parameters["userCount"];
    initParameters.Secure = parameters["useSecureConnection"];
    initParameters.UsePlainTextTOS = parameters["usePlainTextTos"];
    initParameters.IgnoreInactivityTimeout = parameters["ignoreInactivityTimeout"];
    initParameters.EnableQos = parameters["enableQoS"];
    initParameters.EnableNetConnIdle = parameters["enableNetConnIdle"];
    strncpy(initParameters.ClientName, "Blaze Ignition", Blaze::CLIENT_NAME_MAX_LENGTH - 1);
    strncpy(initParameters.ClientVersion, Blaze::getBlazeSdkVersionString(), Blaze::CLIENT_VERSION_MAX_LENGTH - 1);
    strncpy(initParameters.ClientSkuId, "BlazeSampleSku", Blaze::CLIENT_SKU_ID_MAX_LENGTH - 1);
    
    // if we're running a Dedicated Server, pick a random port to emulate
    // real-life situations, where each Ded.Serv. gets assigned a different port.
    if (initParameters.Client == Blaze::CLIENT_TYPE_DEDICATED_SERVER)
    {
        initParameters.GamePort = ((NetTick() % 2000) + 10000);
    }
    else
    {
        initParameters.GamePort = parameters["gamePort"];
    }

    strncpy(initParameters.Override.RedirectorAddress, parameters["redirectorAddress"], Blaze::BlazeHub::ADDRESS_MAX_LENGTH - 1);
    initParameters.Override.RedirectorSecure = parameters["redirectorSecure"];
    initParameters.Override.RedirectorPort = parameters["redirectorPort"];

    Blaze::BlazeError blazeError = rLOG(Blaze::BlazeHub::initialize(&gBlazeHub, initParameters, &gsAllocator, BlazeSdkLogFunction, this));

    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::BlazeNetworkAdapter::ConnApiAdapterConfig config;
#if defined(EA_PLATFORM_NX)
        config.mMaxNumVoipPeers = 0;
#else
        config.mMaxNumVoipPeers = parameters["maxNumVoipPeers"];
#endif
        config.mMaxNumTunnels = parameters["maxNumTunnels"];
        config.mPacketSize = 1200;
        config.mEnableDemangler = parameters["enableDemangler"];

#if !defined(EA_PLATFORM_NX)
        if(initParameters.Client == Blaze::CLIENT_TYPE_DEDICATED_SERVER)
#endif
            config.mEnableVoip = false;

        gConnApiAdapter = new Blaze::BlazeNetworkAdapter::ConnApiAdapter(config);
        gConnApiAdapter->addUserListener(this);

        gTransmission2.SetNetworkAdapter(gConnApiAdapter);

        Blaze::GameManager::GameManagerAPI::GameManagerApiParams gameManagerParams(gConnApiAdapter, 0, 0, 0, 0, parameters["gameProtocolVersion"], Blaze::MEM_GROUP_FRAMEWORK_TEMP, parameters["preventMultipleGameMembership"]);
        Blaze::Telemetry::TelemetryAPI::TelemetryApiParams telemetryParams(4, Blaze::Telemetry::TelemetryAPI::BUFFERTYPE_CIRCULAROVERWRITE, 1, 9000);

        Blaze::GameManager::GameManagerAPI::createAPI(*gBlazeHub, gameManagerParams);
        Blaze::Telemetry::TelemetryAPI::createAPI(*gBlazeHub, telemetryParams);
        Blaze::CensusData::CensusDataAPI::createAPI(*gBlazeHub);
        Blaze::Stats::LeaderboardAPI::createAPI(*gBlazeHub);
        Blaze::Messaging::MessagingAPI::createAPI(*gBlazeHub);
        Blaze::Stats::StatsAPI::createAPI(*gBlazeHub);

        Blaze::Association::AssociationListAPI::AssociationListApiParams associationListApiParams;
        Blaze::Association::AssociationListAPI::createAPI(*gBlazeHub, associationListApiParams);

#if !defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS5) && !defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBSX)
        for (uint32_t userIndex = 0; userIndex < gBlazeHub->getInitParams().UserCount; ++userIndex)
        {
            gBlazeHub->getLoginManager(userIndex)->setUseExternalLoginFlow(parameters["useExternalLoginFlow"]);
        }
#endif

        getUiDriverMaster()->onBlazeHubInitialized();

        getActionStartupBlaze().setIsVisible(false);
        getActionShutdownBlaze().setIsVisible(true);

        getUiDriver()->getMainValue("Service Name") = initParameters.ServiceName;
        getUiDriver()->getMainValue("Environment") = initParameters.Environment;
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void BlazeInitialization::initActionShutdownBlaze(Pyro::UiNodeAction &action)
{
    action.setText("Shutdown Blaze");
    action.setDescription("Shutdown the BlazeSDK.");
}

void BlazeInitialization::actionShutdownBlaze(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (gBlazeHub != nullptr)
    {
        getUiDriverMaster()->onBlazeHubFinalized();

        vLOG(Blaze::BlazeHub::shutdown(gBlazeHub));
        gBlazeHub = nullptr;

        delete gConnApiAdapter;

        getUiDriver()->getMainValue("Service Name") = "";
        getUiDriver()->getMainValue("Environment") = "";
    }

    getActionStartupBlaze().setIsVisible(true);
    getActionShutdownBlaze().setIsVisible(false);

    VoipShutdown(VoipGetRef(), 0);

#if DIRTYCODE_LOGGING
    NetPrintfHook(nullptr, nullptr);
#endif
}

void BlazeInitialization::onInitialized( const Blaze::Mesh *mesh, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error )
{
    getUiDriver()->addLog_va(
        "The Blaze Network adapter has become initialized on mesh %p with code %d\n", mesh, error);
}

void BlazeInitialization::onUninitialize( const Blaze::Mesh *mesh, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error )
{
    getUiDriver()->addLog_va(
        "The Blaze Network adapter has become un-initialized on mesh %p with code %d\n", mesh, error);

    #if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
    if ((mesh != nullptr)) 
    {
        //if the user has the invite dialog open close it (the session is over)
        if (DirtySessionManagerStatus(gConnApiAdapter->getDirtySessionManagerRefT(), 'gsdo', nullptr, 0) == TRUE)
        {
            DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'csid', 0, 0, nullptr);
        }

        //terminate the session invite dialog
        DirtySessionManagerControl(gConnApiAdapter->getDirtySessionManagerRefT(), 'sidt', 0, 0, nullptr);
    }
    #elif defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
    // if the dialog is running close it
    if (scePlayerInvitationDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING)
    {
        scePlayerInvitationDialogClose();
    }
    // terminate the dialog
    scePlayerInvitationDialogTerminate();
    #endif
}

}

