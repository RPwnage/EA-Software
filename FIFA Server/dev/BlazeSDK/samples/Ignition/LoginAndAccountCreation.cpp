#include "Ignition/LoginAndAccountCreation.h"

#if !defined(EA_PLATFORM_NX)
#include "DirtySDK/voip/voip.h"
#endif

namespace Ignition
{
LoginAndAccountCreation::LoginAndAccountCreation(uint32_t userIndex)
    : LocalUserUiBuilder("Login And Account Creation", userIndex),
      mNucleusLoginHelper(*this),
      mEadpLoginHelper(*this)
{
    getActions().add(&getActionCancelLogin());
    getActions().add(&getActionStartLogin());
    getActions().add(&getActionAuthCodeLogin());
    getActions().add(&getActionAccessTokenLogin());
    getActions().add(&getActionStartGuestLogin());

    getActions().add(&getActionExpressLogin());
    getActions().add(&getActionExpressLoginNexus());

    getActions().add(&getActionLogout());
    getActions().add(&getActionForceOwnSessionLogout());
    getActions().add(&getActionCreateWalUserSession());

    getActions().add(&getActionUpdateAccessToken());

#if !defined(EA_PLATFORM_NX)
    getActions().add(&getActionSelectAudioDevices());
#endif

#if defined(BLAZESDK_CONSOLE_PLATFORM)
    getActions().add(&getActionSetConsoleUserIndex());
#endif

    gBlazeHub->getLoginManager(getUserIndex())->addListener(this);

    ::srand((unsigned int)::time(nullptr));
}

LoginAndAccountCreation::~LoginAndAccountCreation()
{
    gBlazeHub->getLoginManager(getUserIndex())->removeListener(this);
}

void LoginAndAccountCreation::loginUserByAuthCode(const char8_t* code, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt)
{
    gBlazeHub->getLoginManager(getUserIndex())->addListener(this);
    gBlazeHub->getLoginManager(getUserIndex())->startAuthCodeLoginProcess(code, productName, crossPlatformOpt);
}

void LoginAndAccountCreation::loginUserByAccessToken(const char8_t* accessToken, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt)
{
    gBlazeHub->getLoginManager(getUserIndex())->addListener(this);
    gBlazeHub->getLoginManager(getUserIndex())->startAccessTokenLoginProcess(accessToken, productName, crossPlatformOpt);
}

void LoginAndAccountCreation::updateAccessToken(const char8_t* accessToken)
{
    Pyro::UiNodeWindow &window = getWindow("NewAccessToken");
    window.setDockingState(Pyro::DockingState::DOCK_TOP);

    Pyro::UiNodeTable &table = window.getTable("New Access Token");
    table.getRows().clear();
    table.getColumn("accesstoken").setText("New Access Token (May not display full string if token is too long like JWT)");

    Pyro::UiNodeTableRow& row = table.getRows().getById_va("%d", 0);
    row.getField("accesstoken") = accessToken;

    gBlazeHub->getLoginManager(getUserIndex())->addListener(this);
    gBlazeHub->getLoginManager(getUserIndex())->updateAccessToken(accessToken, Blaze::MakeFunctor(this, &LoginAndAccountCreation::UpdateAccessTokenCb));
}

void LoginAndAccountCreation::onEadpLoginByAuthCode(const char8_t* authCode, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt)
{
    vLOG(loginUserByAuthCode(authCode, productName, crossPlatformOpt));
}

void LoginAndAccountCreation::onEadpLoginByAccessToken(const char8_t* accessToken, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt)
{
    vLOG(loginUserByAccessToken(accessToken, productName, crossPlatformOpt));
}

void LoginAndAccountCreation::onEadpUpdateAccessToken(const char8_t* accessToken)
{
    vLOG(updateAccessToken(accessToken));
}

void LoginAndAccountCreation::onNucleusLogin(const char8_t* code, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt)
{
    vLOG(loginUserByAuthCode(code, productName, crossPlatformOpt));
}

void LoginAndAccountCreation::onAuthenticatedPrimaryUser()
{
}

void LoginAndAccountCreation::onDeAuthenticatedPrimaryUser()
{
    getWindows().clear();
}

void LoginAndAccountCreation::onPrimaryLocalUserChanged(uint32_t userIndex)
{
    Blaze::UserManager::LocalUser *user = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
    if (user)
    {
        getUiDriver()->setText_va("%s%s%s", user->getName(), ((user->getUserSessionType() == Blaze::USER_SESSION_GUEST) ? " (Guest)" : ""),
            ((userIndex == getUserIndex()) ? " (PrimaryLocalUser)" : ""));
    }
}

void LoginAndAccountCreation::onAuthenticated()
{
    Blaze::UserManager::LocalUser *user = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());

    getUiDriver()->setText_va("%s%s%s", user->getName(), ((user->getUserSessionType() == Blaze::USER_SESSION_GUEST) ? " (Guest)" : ""),
        ((gBlazeHub->getUserManager()->getPrimaryLocalUserIndex() == getUserIndex()) ? " (PrimaryLocalUser)" : ""));
    getUiDriver()->setCurrentStatus_va("Logged in as '%s'%s", user->getName(), (user->getUserSessionType() == Blaze::USER_SESSION_GUEST ? " (Guest)" : ""));

    Blaze::TimeValue lastLogin(gBlazeHub->getLoginManager(getUserIndex())->getTitleLastLoginTime());
    char8_t lastLoginStr[100];
    getUiDriver()->getMainValue("Last Login") = lastLogin.toString(lastLoginStr, sizeof(lastLoginStr));

    setIsVisibleForAll(false);
    getActionLogout().setIsVisible(true);

#ifdef EA_PLATFORM_WINDOWS
    VoipRefT *voipRefT = VoipGetRef();
    getActionSelectAudioDevices().setIsVisible(voipRefT != nullptr);
#endif

    getActionForceOwnSessionLogout().setIsVisible(true);
    getActionCreateWalUserSession().setIsVisible(true);
    getActionUpdateAccessToken().setIsVisible(true);

    SubmitEvent();
}

void LoginAndAccountCreation::onDeAuthenticated()
{
    // This is the Ignition side piece that resets gui.
    getUiDriver()->setText("Not Logged In");
    getUiDriver()->setCurrentStatus_va("Local user index [%d]", getUserIndex());

    setIsVisibleForAll(false);
    getActionAuthCodeLogin().setIsVisible(true);
    getActionAccessTokenLogin().setIsVisible(true);
    getActionStartLogin().setIsVisible(true);
    getActionStartGuestLogin().setIsVisible(true);
    getActionExpressLogin().setIsVisible(true);
    getActionExpressLoginNexus().setIsVisible(true);
    getActionSetConsoleUserIndex().setIsVisible(true);

    getWindows().clear();

    mEadpLoginHelper.onLogout();
}

void LoginAndAccountCreation::onForcedDeAuthenticated(Blaze::UserSessionForcedLogoutType forcedLogoutType)
{
    getUiDriver()->showMessage_va(
        Pyro::ErrorLevel::WARNING,
        "Forced Logout",
        "User has been forcefully logged out due to: %s", Blaze::UserSessionForcedLogoutTypeToString(forcedLogoutType));
}

void LoginAndAccountCreation::onLoginFailure(Blaze::BlazeError blazeError, const char8_t* portalUrl)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    if (blazeError == Blaze::AUTH_ERR_PERSONA_NOT_FOUND)
    {
        getUiDriver()->showMessage_va(
            Pyro::ErrorLevel::NONE,
            "Nucleus Account Creation Portal",
            "Go to Nucleus portal for account creation: %s\r\n"
            "\r\n"
            "After successfully creating the new account, return here a click Start Login Process again to login.",
            (portalUrl != nullptr)?portalUrl:"N/A");
    }
    else
    {
        onDeAuthenticated();
        REPORT_BLAZE_ERROR(blazeError);
    }
}

void LoginAndAccountCreation::onNucleusLoginFailure(Blaze::BlazeError blazeError)
{
    REPORT_ERROR(blazeError, "Ignition Error");
}

void LoginAndAccountCreation::onGetAccountInfoError(Blaze::BlazeError blazeError)
{
    REPORT_LISTENER_CALLBACK(nullptr);

    REPORT_BLAZE_ERROR(blazeError);
}

void LoginAndAccountCreation::onSdkError(Blaze::BlazeError blazeError)
{
    REPORT_LISTENER_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

void LoginAndAccountCreation::setupCommonLoginParameters(Pyro::UiNodeAction &action)
{
    action.getParameters().addString("product", "", "Product Name", "", "Product name to authenticate against.");
    action.getParameters().addEnum("crossplatformopt", Blaze::Authentication::CrossPlatformOptSetting::DEFAULT, "Cross Platform Opt", "Sets the user's cross platform option.", Pyro::ItemVisibility::ADVANCED);
}

void LoginAndAccountCreation::setupExpressLoginParameters(Pyro::UiNodeAction &action)
{
    action.getParameters().addString("email", "", "Email", "", "The email address of the account to login");
    action.getParameters().addString("password", "", "Password", "", "The password of the account to login");
    action.getParameters().addString("personaName", "", "Persona Name", "", "The persona name to login");

    action.getParameters().addEnum("clienttype", gBlazeHub->getInitParams().Client, "Client Type", "Overrides the default client type.", Pyro::ItemVisibility::ADVANCED);
}

void LoginAndAccountCreation::initActionStartLogin(Pyro::UiNodeAction &action)
{
    action.setText("Start Login");
    action.setDescription("Start the login process");

    setupCommonLoginParameters(action);

#if defined(BLAZE_USING_EADP_SDK)
    action.getParameters().addEnum("loginby", AUTH_CODE, "Login By", "Login authentication option", Pyro::ItemVisibility::ADVANCED);
#endif
}

void LoginAndAccountCreation::actionStartLogin(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    loginUser(parameters);
}

void LoginAndAccountCreation::initActionAuthCodeLogin(Pyro::UiNodeAction &action)
{
    action.setText("Start Auth Code Login");
    action.setDescription("Start the login process with Auth Code");

    action.getParameters().addString("authcode", "", "Auth Code", "", "Nucleus 2.0 Auth Code");
    setupCommonLoginParameters(action);
}

void LoginAndAccountCreation::actionAuthCodeLogin(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(loginUserByAuthCode(parameters["authcode"], parameters["product"], parameters["crossplatformopt"]));
}

void LoginAndAccountCreation::initActionAccessTokenLogin(Pyro::UiNodeAction &action)
{
    action.setText("Start Access Token Login");
    action.setDescription("Start the login process with Access Token");

    action.getParameters().addString("accessToken", "", "Access Token", "", "Nucleus 2.0 Access Token");
    setupCommonLoginParameters(action);
}

void LoginAndAccountCreation::actionAccessTokenLogin(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(loginUserByAccessToken(parameters["accessToken"], parameters["product"], parameters["crossplatformopt"]));
}

void LoginAndAccountCreation::loginUser(Pyro::UiNodeParameterStructPtr loginParameters)
{
    getActionCancelLogin().setIsVisible(true);

#if defined(BLAZE_USING_EADP_SDK)
    vLOG(mEadpLoginHelper.onStartLoginProcess(loginParameters["product"], loginParameters["crossplatformopt"], loginParameters["loginby"]));
#elif defined(BLAZE_USING_NUCLEUS_SDK)
    vLOG(mNucleusLoginHelper.onStartLoginProcess(gBlazeHub->getLoginManager(getUserIndex())->getDirtySockUserIndex(), loginParameters["product"], loginParameters["crossplatformopt"]));
#else
    vLOG(gBlazeHub->getLoginManager(getUserIndex())->startLoginProcess(loginParameters["product"], loginParameters["crossplatformopt"]));
#endif
}

void LoginAndAccountCreation::initActionStartGuestLogin(Pyro::UiNodeAction &action)
{
    action.setText("Start Guest Login");
    action.setDescription("Start the Guest login process");
}

void LoginAndAccountCreation::actionStartGuestLogin(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(gBlazeHub->getLoginManager(getUserIndex())->addListener(this));
    vLOG(gBlazeHub->getLoginManager(getUserIndex())->startGuestLoginProcess((Blaze::MakeFunctor(this, &LoginAndAccountCreation::onGuestLogin))));
}

void LoginAndAccountCreation::onGuestLogin(Blaze::BlazeError blazeError, const Blaze::SessionInfo* info)
{
    REPORT_BLAZE_ERROR(blazeError);
}

void LoginAndAccountCreation::expressLoginUser(Pyro::UiNodeParameterStructPtr loginParameters)
{
    Blaze::Authentication::ExpressLoginRequest request;
    request.setEmail(loginParameters["email"]);
    request.setPassword(loginParameters["password"]);
    request.setPersonaName((gBlazeHub->getClientPlatformType() == Blaze::ClientPlatformType::steam) ? SteamHelper::getPersonaFromNexusPersona(loginParameters["personaName"].getAsString()) : loginParameters["personaName"].getAsString());
    request.setProductName(loginParameters["product"]);
    request.setClientType(loginParameters["clienttype"]);
    request.setCrossPlatformOpt(loginParameters["crossplatformopt"]);

    Blaze::Authentication::AuthenticationComponent *auth = gBlazeHub->getComponentManager(getUserIndex())->getAuthenticationComponent();
    auth->expressLogin(request, Blaze::Authentication::AuthenticationComponent::ExpressLoginCb(this, &LoginAndAccountCreation::ExpressLoginCb));
}

void LoginAndAccountCreation::initActionExpressLogin(Pyro::UiNodeAction &action)
{
    action.setText("Express Login");
    action.setDescription("Use the Authentication::expressLogin() RPC to login");

    setupExpressLoginParameters(action);
    setupCommonLoginParameters(action);
}

void LoginAndAccountCreation::actionExpressLogin(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    expressLoginUser(parameters);
}

void LoginAndAccountCreation::initActionExpressLoginNexus(Pyro::UiNodeAction &action)
{
    action.setText("Express Login (Debug Personas)");
    action.setDescription("Use the Authentication::expressLogin() RPC to login with random Nexus account information");
}

void LoginAndAccountCreation::actionExpressLoginNexus(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Authentication::ExpressLoginRequest request;
    // randomly pick a nexus test account from nexus001 to nexus014
    eastl::string personaName, emailAddress;
    int randNum = (rand() % 14) + 1;
    personaName.sprintf( "nexus%03d", randNum);
    emailAddress.sprintf( "%s@ea.com", personaName.c_str());
    request.setEmail(emailAddress.c_str());
    request.setPassword("t3st3r");
    request.setPersonaName(gBlazeHub->getClientPlatformType() == Blaze::ClientPlatformType::steam ? SteamHelper::getPersonaFromNexusPersona(personaName.c_str()) : personaName.c_str());
    request.setClientType(gBlazeHub->getInitParams().Client);

    Blaze::Authentication::AuthenticationComponent *auth = gBlazeHub->getComponentManager(getUserIndex())->getAuthenticationComponent();
    auth->expressLogin(request, Blaze::Authentication::AuthenticationComponent::ExpressLoginCb(this, &LoginAndAccountCreation::ExpressLoginCb));
}

void LoginAndAccountCreation::ExpressLoginCb(const Blaze::Authentication::LoginResponse *response, Blaze::BlazeError blazeError, Blaze::JobId JobId)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

void LoginAndAccountCreation::initActionLogout(Pyro::UiNodeAction &action)
{
    action.setText("Logout");
    action.setDescription("Log out of the blaze server");
}

void LoginAndAccountCreation::actionLogout(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(gBlazeHub->getLoginManager(getUserIndex())->logout());
}

void LoginAndAccountCreation::initActionForceOwnSessionLogout(Pyro::UiNodeAction &action)
{
    action.setText("Force User Session Logout");
    action.setDescription("Force log out another of user's unresponsive sessions");
    action.getParameters().addInt64("blazeId", 0, "BlazeId", "", "The blaze id of the user to log out");
    action.getParameters().addString("sessionKey", "", "SessionKey", "", "The session key of the user to log out");
}

void LoginAndAccountCreation::actionForceOwnSessionLogout(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::ForceOwnSessionLogoutRequest request;
    request.setBlazeId(parameters["blazeId"]);
    request.setSessionKey(parameters["sessionKey"]);

    Blaze::UserSessionsComponent *userSessionsComp = gBlazeHub->getComponentManager(getUserIndex())->getUserSessionsComponent();
    userSessionsComp->forceOwnSessionLogout(request, Blaze::UserSessionsComponent::ForceOwnSessionLogoutCb(this, &LoginAndAccountCreation::ForceOwnSessionLogoutCb));
}

void LoginAndAccountCreation::ForceOwnSessionLogoutCb(Blaze::BlazeError blazeError, Blaze::JobId JobId)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

void LoginAndAccountCreation::initActionCreateWalUserSession(Pyro::UiNodeAction &action)
{
    action.setText("Create WAL UserSession");
    action.setDescription("Creates a UserSession on the server that is owned by this user, and can be accessed via an HTTP (WAL) connection.");
}

void LoginAndAccountCreation::actionCreateWalUserSession(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Authentication::AuthenticationComponent *authenticationComp = gBlazeHub->getComponentManager(getUserIndex())->getAuthenticationComponent();
    authenticationComp->createWalUserSession(Blaze::Authentication::AuthenticationComponent::CreateWalUserSessionCb(this, &LoginAndAccountCreation::CreateWalUserSessionCb));
}

void LoginAndAccountCreation::CreateWalUserSessionCb(const Blaze::Authentication::UserLoginInfo* userLoginInfo, Blaze::BlazeError blazeError, Blaze::JobId JobId)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);
    REPORT_BLAZE_ERROR(blazeError);
}

void LoginAndAccountCreation::initActionCancelLogin(Pyro::UiNodeAction &action)
{
    action.setText("Cancel Login");
    action.setDescription("Cancel the login process");
}

void LoginAndAccountCreation::actionCancelLogin(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (mNucleusLoginHelper.isLoginInProgress())
    {
        vLOGIgnition(mNucleusLoginHelper.cancelLogin());
        getActionCancelLogin().setIsVisible(false);
    }
    else
    {
        vLOG(gBlazeHub->getLoginManager(getUserIndex())->cancelLogin(Blaze::MakeFunctor(this, &LoginAndAccountCreation::CancelLoginCb)));
    }
}

void LoginAndAccountCreation::CancelLoginCb(Blaze::BlazeError error)
{
    onDeAuthenticated();
}

#if !defined(EA_PLATFORM_NX)
void LoginAndAccountCreation::initActionSelectAudioDevices(Pyro::UiNodeAction &action)
{
    action.setText("Select Audio Devices");
    action.setDescription("Select the desired audio devices to be used (PC only)");
    //action.Execute += Pyro::UiNodeAction::ExecuteEventHandler(this, &LoginAndAccountCreation::execActionSelectAudioDevices);

    VoipRefT *voipRefT = VoipGetRef();
    if( voipRefT != nullptr)
    {
        char deviceName[256];
        eastl::string availableDevices;
        int32_t i, numDevices;

        availableDevices = "";
        numDevices = VoipStatus(voipRefT, 'idev', -1, nullptr, 0);
        for (i = 0; i < numDevices; i++)
        {
            // Query name of input device
            VoipStatus(voipRefT, 'idev', i, deviceName, sizeof(deviceName));

            availableDevices += deviceName;
            availableDevices += ",";
        }

        action.getParameters().addString("inputAudioDevice", "", "Input Audio Device", availableDevices.c_str());

        availableDevices = "";
        numDevices = VoipStatus(voipRefT, 'odev', -1, nullptr, 0);
        for (i = 0; i < numDevices; i++)
        {
            // Query name of output device
            VoipStatus(voipRefT, 'odev', i, deviceName, sizeof(deviceName));

            availableDevices += deviceName;
            availableDevices += ",";
        }

        action.getParameters().addString("outputAudioDevice", "", "Output Audio Device", availableDevices.c_str());

        VoipControl(voipRefT, 'idev', 0, nullptr);
        VoipControl(voipRefT, 'odev', 0, nullptr);
    }
}

void LoginAndAccountCreation::actionSelectAudioDevices(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
#ifdef EA_PLATFORM_WINDOWS
    VoipRefT *voipRefT = VoipGetRef();

    if(voipRefT != nullptr)
    {
        char deviceName[256];
        int32_t i, numDevices;
        int32_t selectedInput = 0;
        int32_t selectedOutput = 0;

        numDevices = VoipStatus(voipRefT, 'idev', -1, nullptr, 0);
        for (i = 0; i < numDevices; i++)
        {
            // Query name of input device
            VoipStatus(voipRefT, 'idev', i, deviceName, sizeof(deviceName));

            if (strcmp(deviceName, parameters["inputAudioDevice"]) == 0)
            {
                selectedInput = i;
                break;
            }
        }

        numDevices = VoipStatus(voipRefT, 'odev', -1, nullptr, 0);
        for (i = 0; i < numDevices; i++)
        {
            // Query name of output device
            VoipStatus(voipRefT, 'odev', i, deviceName, sizeof(deviceName));

            if (strcmp(deviceName, parameters["outputAudioDevice"]) == 0)
            {
                selectedOutput = i;
                break;
            }
        }

        VoipControl(voipRefT, 'idev', selectedInput, nullptr);
        VoipControl(voipRefT, 'odev', selectedOutput, nullptr);
    }
#endif
}
#endif

void LoginAndAccountCreation::initActionSetConsoleUserIndex(Pyro::UiNodeAction &action)
{
    action.setText("Set Controller Index");
    action.setDescription("Set the 1st part user index for this user");

#if defined(BLAZESDK_CONSOLE_PLATFORM)
    action.getParameters().addUInt32("consoleUserIndex", getUserIndex(), "Controller Index", "0,1,2,3");
#endif
}

void LoginAndAccountCreation::actionSetConsoleUserIndex(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    gBlazeHub->getLoginManager(getUserIndex())->setConsoleUserIndex(parameters["consoleUserIndex"]);
}

void LoginAndAccountCreation::initActionUpdateAccessToken(Pyro::UiNodeAction &action)
{
    action.setText("Update Access Token");
    action.setDescription("Update user access token used by Blaze server");

    action.getParameters().addBool("autonewtoken", true, "Auto New Token", "Automatically retrieve a new access token and use it instead of the entered access token.");
    action.getParameters().addString("accesstoken", "", "Access Token", "New user access token for Blaze server to use. Does not change user access token cached in Ignition.");
}

void LoginAndAccountCreation::actionUpdateAccessToken(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (parameters["autonewtoken"])
    {
        mEadpLoginHelper.updateAccessToken();
    }
    else
    {
        updateAccessToken(parameters["accesstoken"]);
    }
}

void LoginAndAccountCreation::UpdateAccessTokenCb(Blaze::BlazeError error)
{
    if (error != Blaze::ERR_OK)
    {
        REPORT_BLAZE_ERROR(error);
    }
}

int32_t LoginAndAccountCreation::SubmitEvent()
{
    TelemetryApiEvent3T event3;
    int32_t result;

    uint32_t test3 = 8234;

    char custumStr[5] = {0};
    sprintf(custumStr, "%04X", test3); // 202A

    uint32_t final = ((uint32_t)custumStr[0] << 24) + 
        ((uint32_t)custumStr[1] << 16) + 
        ((uint32_t)custumStr[2] << 8) + 
        ((uint32_t)custumStr[3]);

    result = TelemetryApiInitEvent3(&event3,'TEST','TLM3',final);
    if(result != TC_OKAY) {
        getUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "ERROR: TeslemetryApiInitEvent3 (userIndex: %d) returned %d", getUserIndex(), result);
    }
    result = TelemetryApiEncAttributeInt(&event3, 'data', 0);
    if(result != TC_OKAY) {
        getUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "ERROR: TelemetryApiEncAttributeString (userIndex: %d) returned %d \n", getUserIndex(), result);
    }

 /*   result = TelemetryApiSubmitEvent3Ex(pTelemetryRef,&event3, 
        2,  
        'lstr', TELEMETRY3_ATTRIBUTE_TYPE_STRING, "This is a very long string. Longer than 256 characters in fact. Therefore is cannot be used in a TelemetryApiEncAttributeString() call. It can however, be added to an event like this. So if you need to add a long string to an event, you can use TelemetryApiSubmitEvent3Ex() or TelemetryApiSubmitEvent3Long().",  
        'flt1', TELEMETRY3_ATTRIBUTE_TYPE_FLOAT, 1.2, 
        'flt2', TELEMETRY3_ATTRIBUTE_TYPE_CHAR, 'c', 
        'flt3', TELEMETRY3_ATTRIBUTE_TYPE_INT, 123);
        */

    result = TelemetryApiSubmitEvent3(gBlazeHub->getTelemetryAPI(getUserIndex())->getTelemetryApiRefT(), &event3);

    if(result < 0) 
    {
        getUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "ERROR: TelemetryApiSubmitEvent3 (userIndex: %d) returned %d", getUserIndex(), result);
    }
    else
    {
        getUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "SUCCESS: TelemetryApiSubmitEvent3 (userIndex: %d) bytes written: %d", getUserIndex(), result);
        return 1;
    }

    return 0;
}


}
