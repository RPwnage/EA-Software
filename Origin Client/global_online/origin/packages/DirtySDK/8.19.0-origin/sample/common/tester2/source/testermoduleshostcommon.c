/*H********************************************************************************/
/*!
    \File testermoduleshost.c

    \Description
        PC specific module startup.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/11/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "platform.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function TesterModulesRegisterHostCommands

    \Description
        Register all PC-specific modules

    \Input *pState - module state
    
    \Output 0=success, error code otherwise

    \Version 04/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterModulesRegisterHostCommands(TesterModulesT *pState)
{
    // tester2 built-ins
    TesterModulesRegister(pState, "help",       &CmdHelp);
    TesterModulesRegister(pState, "history",    &CmdHistory);
    TesterModulesRegister(pState, "!",          &CmdHistory);
    TesterModulesRegister(pState, "memdebug",   &CmdMemDebug);
    TesterModulesRegister(pState, "registry",   &CmdRegistry);

    // common tester2 modules
    TesterModulesRegister(pState, "aries",      &CmdAries);
    TesterModulesRegister(pState, "driver",     &CmdDriver);
#if (defined(DIRTYCODE_PS3)) || (defined(DIRTYCODE_XENON))
    TesterModulesRegister(pState, "friend",     &CmdFriendApi);
#endif
    TesterModulesRegister(pState, "http",       &CmdHttp);
    TesterModulesRegister(pState, "httpmgr",    &CmdHttpMgr);
#if (defined(DIRTYCODE_PC))
    TesterModulesRegister(pState, "ic",         &CmdImgConv);
#endif
    TesterModulesRegister(pState, "lang",       &CmdLang);
    TesterModulesRegister(pState, "lanplay",    &CmdLanPlay);
#if (defined(DIRTYCODE_PC)) || (defined(DIRTYCODE_XENON))
    TesterModulesRegister(pState, "locker",     &CmdLocker);
#endif
#if (defined(DIRTYCODE_PC))
    TesterModulesRegister(pState, "digobj",     &CmdDigObj);
#endif
    TesterModulesRegister(pState, "net",        &CmdNet);
    TesterModulesRegister(pState, "netprint",   &CmdNetPrint);
    TesterModulesRegister(pState, "ping",       &CmdPing);
    TesterModulesRegister(pState, "resource",   &CmdResource);
#if !defined(DIRTYCODE_LINUX) && !defined(DIRTYCODE_APPLEIOS) && !defined(DIRTYCODE_APPLEOSX)
    TesterModulesRegister(pState, "stream",     &CmdStream);
#endif
    TesterModulesRegister(pState, "telemetry",  &CmdTelemetry);
    TesterModulesRegister(pState, "ticker",     &CmdTicker);
    TesterModulesRegister(pState, "time",       &CmdTime);
    TesterModulesRegister(pState, "buddytickerglue",     &CmdBuddyTickerGlue);
    TesterModulesRegister(pState, "tunnel",     &CmdTunnel);
    TesterModulesRegister(pState, "utf8",       &CmdUtf8);
    TesterModulesRegister(pState, "xml",        &CmdXml);
    TesterModulesRegister(pState, "xmllist",    &CmdXmlList);
    TesterModulesRegister(pState, "qos",        &CmdQos);

    // tester2 modules for non-Xbox platforms
#if !defined(DIRTYCODE_XENON)
    TesterModulesRegister(pState, "demangler",  &CmdDemangler);
    TesterModulesRegister(pState, "tracert",    &CmdTracert);
    TesterModulesRegister(pState, "upnp",  &CmdUpnp);
#endif
    
    return(TESTERMODULES_ERROR_NONE);
}

