#include "Ignition/custom/dirtysockdev.h"

#include "DirtySDK/platform.h"

#if !defined(EA_PLATFORM_NX)
#include "DirtySDK/voip/voipgroup.h"
#include "DirtySDK/voip/voipblocklist.h"
#endif
#include "DirtySDK/dirtysock/netconn.h"

namespace Ignition
{

DirtySockDev::DirtySockDev()
    : BlazeHubUiBuilder("DirtySockDev")
{
    setVisibility(Pyro::ItemVisibility::ADVANCED);
#if !defined(EA_PLATFORM_NX)
    getActions().add(&getActionRunFunc1());
#endif
    getActions().add(&getActionConnectToTicker());
#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX)
    getActions().add(&getActionSpeechToText());
#endif
}

DirtySockDev::~DirtySockDev()
{
}

void DirtySockDev::onAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(false);
    getActionRunFunc1().setIsVisible(true);
    getActionConnectToTicker().setIsVisible(true);
#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX)
    getActionSpeechToText().setIsVisible(true);
#endif
}

void DirtySockDev::onDeAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(false);
    getWindows().clear();
}

void DirtySockDev::initActionRunFunc1(Pyro::UiNodeAction &action)
{
    action.setText("Run DirtySDK Function 1");
    action.setDescription("Run test func");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("runFunc1Param", "Default", "Parameter Name", "", "Parameter Description", Pyro::ItemVisibility::ADVANCED);
}

void DirtySockDev::actionRunFunc1(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    //unit test block list

    /*
    Name        AccountID           PersonaID
    nexus001    0x00000002DD3C848B  0x000000E8DAFF1F2B
    nexus002    0x000000E8DEBF8811  0x0000000018D83C9C
    nexus003    0x00000002DD4BB877  0x000000E8D61EE597
    nexus004    0x0000000086627736  0x000000E8D60710F6
    nexus005    0x00000002DD774D0D  0x000000E8D4ACF6CD
    nexus006    0x00000002DC6250B1  0x000000E8D4AAA2F1
    nexus007    0x00000000866BE816  0x000000E8D608E0B6
    nexus008    0x00000000866C41B7  0x000000E8D4AD53D7
    nexus009    0x00000000866C41C3  0x000000E8D85F5F63
    */

#if !defined(EA_PLATFORM_NX)
    uint32_t uBlazeUserIndex = 0;
    uint32_t uDirytSDKUserIndex = gBlazeHub->getLoginManager(uBlazeUserIndex)->getDirtySockUserIndex();
    static bool bBlocked = false;

    if (!bBlocked)
    {
        bBlocked = TRUE;
        VoipBlockListAdd(VoipGetRef(), uDirytSDKUserIndex, 0x00000002DD3C848B);
        VoipBlockListAdd(VoipGetRef(), uDirytSDKUserIndex, 0x000000E8DEBF8811);
        VoipBlockListAdd(VoipGetRef(), uDirytSDKUserIndex, 0x00000002DD4BB877);
        VoipBlockListAdd(VoipGetRef(), uDirytSDKUserIndex, 0x0000000086627736);
        VoipBlockListAdd(VoipGetRef(), uDirytSDKUserIndex, 0x00000002DD774D0D);
        VoipBlockListAdd(VoipGetRef(), uDirytSDKUserIndex, 0x00000002DC6250B1);
        VoipBlockListAdd(VoipGetRef(), uDirytSDKUserIndex, 0x00000000866BE816);
        VoipBlockListAdd(VoipGetRef(), uDirytSDKUserIndex, 0x00000000866C41B7);
        VoipBlockListAdd(VoipGetRef(), uDirytSDKUserIndex, 0x00000000866C41C3);
    }
    else
    {
        bBlocked = FALSE;
        VoipBlockListRemove(VoipGetRef(), uDirytSDKUserIndex, 0x00000002DD3C848B);
        VoipBlockListRemove(VoipGetRef(), uDirytSDKUserIndex, 0x000000E8DEBF8811);
        VoipBlockListRemove(VoipGetRef(), uDirytSDKUserIndex, 0x00000002DD4BB877);
        VoipBlockListRemove(VoipGetRef(), uDirytSDKUserIndex, 0x0000000086627736);
        VoipBlockListRemove(VoipGetRef(), uDirytSDKUserIndex, 0x00000002DD774D0D);
        VoipBlockListRemove(VoipGetRef(), uDirytSDKUserIndex, 0x00000002DC6250B1);
        VoipBlockListRemove(VoipGetRef(), uDirytSDKUserIndex, 0x00000000866BE816);
        VoipBlockListRemove(VoipGetRef(), uDirytSDKUserIndex, 0x00000000866C41B7);
        VoipBlockListRemove(VoipGetRef(), uDirytSDKUserIndex, 0x00000000866C41C3);
    }
#endif
}

void DirtySockDev::initActionConnectToTicker(Pyro::UiNodeAction &action)
{
    action.setText("Connect to ticker");
    action.setDescription("Connect to ticker");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("TickerAddr", "", "Addr Override", "", "Ticker Server Addr Override", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("TickerPort", "", "Port Override", "", "Ticker Server Port Override", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("TickerName", "", "Name Override", "", "Ticker Server Name Override", Pyro::ItemVisibility::ADVANCED);
}

void DirtySockDev::actionConnectToTicker(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::LoginManager::LoginManager *pLoginManager = gBlazeHub->getLoginManager(0);
    const Blaze::Util::GetTickerServerResponse *pTickerResponse = pLoginManager->getTickerServer();

    if (pTickerResponse != nullptr)
    {
        NetPrintf(("DirtySockDev: got ticker response, implimentation todo.\n"));
    }
    else
    {
        NetPrintf(("DirtySockDev: could not get ticker response\n"));
    }
}

#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX)
void DirtySockDev::initActionSpeechToText(Pyro::UiNodeAction &action)
{
    action.setText("Speech-To-Text");
    action.setDescription("Enable/disable STT for specified local user");
    action.setVisibility(Pyro::ItemVisibility::ADVANCED);

#if defined(EA_PLATFORM_XBOXONE)
    action.getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15", "Xone User Index", Pyro::ItemVisibility::ADVANCED);
#else
    action.getParameters().addInt32("userIndex", 0, "User Index", "0,1,2,3", "User Index", Pyro::ItemVisibility::ADVANCED);
#endif
    action.getParameters().addBool("enabled", false, "On/off", "STT on/off switch for the specified local user", Pyro::ItemVisibility::ADVANCED);
}

void DirtySockDev::actionSpeechToText(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    if (VoipGetRef())
    {
        uint32_t bSTTEnabled = parameters["enabled"];
        int32_t iUserIndex = parameters["userIndex"];

        VoipControl(VoipGetRef(), 'stot', iUserIndex, &bSTTEnabled);
    }
}
#endif

} // namespace Ignition
