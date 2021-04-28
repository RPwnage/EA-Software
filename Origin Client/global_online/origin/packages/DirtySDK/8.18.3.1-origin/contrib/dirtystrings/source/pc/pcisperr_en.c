/*H*************************************************************************************************/
/*!

    \File    pcisperr_en.c

    \Description
        Error alert table for PCIsp in English

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2005.  ALL RIGHTS RESERVED.

    \Version 1.0 09/15/2005 (dclark) First Version
*/
/*************************************************************************************************H*/

/*** Include files *********************************************************************/

#include "dirtysock.h"
#include "dirtystrings.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables

const PCIspAlertT    _PCIsp_AlertTable_En[PCISP_NUMALERTS] =
{
    /* PCISP_ALERT_MISSING  */  { PCISP_ALERTFLAG_OK, "Account Info Missing",    "No account information was found on this machine.",                       "Please run EA Connect to create a new account, or use an existing one." },
    /* PCISP_ALERT_ACCESS   */  { PCISP_ALERTFLAG_OK, "Account Access Denied",   "Access Denied error when trying to read registry entry.",                 "" },    
    /* PCISP_ALERT_REGERROR */  { PCISP_ALERTFLAG_OK, "Account Error",           "An error occured when reading account information from the registry.",    "" },
};

/*** Private Functions *****************************************************************/

/*** Public Functions ******************************************************************/
