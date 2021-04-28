/*H********************************************************************************/
/*!
    \File testermoduleshost.c

    \Description
        Xenon specific module startup.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/19/2005 (jfrank) First Version
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
    \Function TesterModulesRegisterPlatformCommands

    \Description
        Register all modules for the Xenon platform

    \Input *pState - module state
    
    \Output 0=success, error code otherwise

    \Version 04/19/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterModulesRegisterPlatformCommands(TesterModulesT *pState)
{
    TesterModulesRegister(pState, "help",       &CmdHelp);
    TesterModulesRegister(pState, "hlbuddy",    &CmdHLBuddy);
    TesterModulesRegister(pState, "lang",       &CmdLang);
    TesterModulesRegister(pState, "net",        &CmdNet);
    TesterModulesRegister(pState, "ping",       &CmdPing);
    TesterModulesRegister(pState, "registry",   &CmdRegistry);
    TesterModulesRegister(pState, "resource",   &CmdResource);
    //TesterModulesRegister(pState, "sleep",      &CmdSleep);

    return(TESTERMODULES_ERROR_NONE);
}

