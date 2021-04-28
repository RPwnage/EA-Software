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

#include "DirtySDK/platform.h"
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
    TesterModulesRegister(pState, "source",     &CmdSource);

    // common tester2 modules
    TesterModulesRegister(pState, "aries",      &CmdAries);
    TesterModulesRegister(pState, "crypt",      &CmdCrypt);
    TesterModulesRegister(pState, "driver",     &CmdDriver);
#if (defined(DIRTYCODE_PS3)) || (defined(DIRTYCODE_XENON))
    TesterModulesRegister(pState, "friend",     &CmdFriendApi);
#endif
    TesterModulesRegister(pState, "http",       &CmdHttp);
    TesterModulesRegister(pState, "httpmgr",    &CmdHttpMgr);
    TesterModulesRegister(pState, "httpserv",   &CmdHttpServ);
#if (defined(DIRTYCODE_PC))
    TesterModulesRegister(pState, "ic",         &CmdImgConv);
#endif
    TesterModulesRegister(pState, "json",       &CmdJson);
    TesterModulesRegister(pState, "lang",       &CmdLang);
    TesterModulesRegister(pState, "lanplay",    &CmdLanPlay);
    TesterModulesRegister(pState, "net",        &CmdNet);
    TesterModulesRegister(pState, "netprint",   &CmdNetPrint);
    TesterModulesRegister(pState, "ping",       &CmdPing);
    TesterModulesRegister(pState, "resource",   &CmdResource);
    TesterModulesRegister(pState, "socket",     &CmdSocket);
#if defined(DIRTYCODE_PC) || defined(DIRTYCODE_XENON) || defined(DIRTYCODE_PS3)
    TesterModulesRegister(pState, "stream",     &CmdStream);
#endif
    TesterModulesRegister(pState, "string",     &CmdString);
    TesterModulesRegister(pState, "time",       &CmdTime);
    TesterModulesRegister(pState, "tunnel",     &CmdTunnel);
    TesterModulesRegister(pState, "udp",        &CmdUdp);
    TesterModulesRegister(pState, "utf8",       &CmdUtf8);
    TesterModulesRegister(pState, "ws",         &CmdWS);
    TesterModulesRegister(pState, "xml",        &CmdXml);
    TesterModulesRegister(pState, "qos",        &CmdQos);

    // tester2 modules for non-Xbox platforms
#if !defined(DIRTYCODE_XENON)
    TesterModulesRegister(pState, "demangler",  &CmdDemangler);
    TesterModulesRegister(pState, "tracert",    &CmdTracert);
    TesterModulesRegister(pState, "upnp",       &CmdUpnp);
#endif

    return(TESTERMODULES_ERROR_NONE);
}
