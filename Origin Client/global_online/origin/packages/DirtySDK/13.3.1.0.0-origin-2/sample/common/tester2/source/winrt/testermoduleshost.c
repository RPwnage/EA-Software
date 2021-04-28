/*H********************************************************************************/
/*!
    \File testermodulespchost.c

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
    \Function TesterModulesRegisterPlatformCommands

    \Description
        Register all PC-specific modules

    \Input *pState - module state
    
    \Output 0=success, error code otherwise

    \Version 04/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterModulesRegisterPlatformCommands(TesterModulesT *pState)
{
    // tester2 winrt-specific modules
    TesterModulesRegister(pState, "secure",&CmdSecure);
    return(TESTERMODULES_ERROR_NONE);
}

