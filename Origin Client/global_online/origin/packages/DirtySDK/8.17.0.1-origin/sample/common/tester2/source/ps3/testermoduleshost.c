/*H********************************************************************************/
/*!
    \File testermoduleshost.c

    \Description
        PS3 specific module startup.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 04/14/2006 (tdills) First Version
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
    \Function TesterModulesRegisterAllCommands

    \Description
        Register all PS3-specific modules

    \Input *pState - module state
    
    \Output 0=success, error code otherwise

    \Version 04/14/2006 (tdills)
*/
/********************************************************************************F*/
int32_t TesterModulesRegisterPlatformCommands(TesterModulesT *pState)
{
    // tester2 ps2-specific modules
//    TesterModulesRegister(pState, "mc",    &CmdMemoryCard);
    TesterModulesRegister(pState, "pt",    &CmdPortTester);
    TesterModulesRegister(pState, "mbox",  &CmdMbTest);
    TesterModulesRegister(pState, "hlbuddy", &CmdHLBuddy);
    TesterModulesRegister(pState, "ps3sys", &CmdPS3SysUtils);
    TesterModulesRegister(pState, "voice", &CmdVoice);
    TesterModulesRegister(pState, "nplookup", &CmdNpLookup);
    return(TESTERMODULES_ERROR_NONE);
}

