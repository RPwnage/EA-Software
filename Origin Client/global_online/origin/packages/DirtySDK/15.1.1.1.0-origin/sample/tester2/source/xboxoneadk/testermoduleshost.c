/*H********************************************************************************/
/*!
    \File testermoduleshost.c

    \Description
        XboxOne ADK specific module startup.

    \Copyright
        Copyright (c) 2013 Electronic Arts Inc.

    \Version 05/15/2013 (mclouatre) First Version
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

    \Version 05/15/2013 (mclouatre)
*/
/********************************************************************************F*/
int32_t TesterModulesRegisterPlatformCommands(TesterModulesT *pState)
{
    return(TESTERMODULES_ERROR_NONE);
}
