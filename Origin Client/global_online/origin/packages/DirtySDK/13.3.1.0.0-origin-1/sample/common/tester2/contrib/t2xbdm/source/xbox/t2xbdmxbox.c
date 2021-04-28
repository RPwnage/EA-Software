/*H********************************************************************************/
/*!
    \File t2xbdmxbox.c

    \Description
        Wrapper for the Microsoft (tm) debug manager interface.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 05/11/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <windows.h>
#include <stdio.h>
#include <XboxDbg.h>
#include "DirtySDK/platform.h"
#include "t2xbdmxbox.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

uint32_t uXBDMXboxInitialized = 0;

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxMain

    \Description
        Entry point for this DLL.

    \Input *pModule   - unused
    \Input  uReason   - unused
    \Input *pReserved - unused
    
    \Output int32_t       - always returns 1 (success)

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t APIENTRY T2XBDMXboxMain( void *pModule, uint32_t uReason, void *pReserved)
{
    switch(uReason)
    {
        case DLL_PROCESS_ATTACH:    printf("xbdm: DLL_PROCESS_ATTACH - uInit(%u)\n", uXBDMXboxInitialized);   break;
        case DLL_THREAD_ATTACH:     printf("xbdm: DLL_THREAD_ATTACH  - uInit(%u)\n", uXBDMXboxInitialized);   break;
        case DLL_THREAD_DETACH:     printf("xbdm: DLL_THREAD_DETACH  - uInit(%u)\n", uXBDMXboxInitialized);   break;
        case DLL_PROCESS_DETACH:    printf("xbdm: DLL_PROCESS_DETACH - uInit(%u)\n", uXBDMXboxInitialized);   break;
        default:  printf("xbdm: Unknown reason for call [%d = 0x%x]  - uInit(%u)\n", uReason, uReason, uXBDMXboxInitialized);
    }
    return(TRUE);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxStartup

    \Description
        Initialization code for the Xbox XBDM.

    \Input None
    
    \Output int32_t - 0 for success, <0 indicates an error occurred

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxStartup(void)
{
    printf("xbdmxbox: Startup\n");
    uXBDMXboxInitialized = 1;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function T2XBDMXboxCloseConnection

    \Description
        See Microsoft Xbox documentation for DmCloseConnection

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxCloseConnection(T2XBDMXboxConnectionT Connection)
{
    printf("xbdm: CloseConnection\n");
    return(uXBDMXboxInitialized ? DmCloseConnection((PDM_CONNECTION)Connection) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxCloseNotificationSession

    \Description
        See Microsoft Xbox documentation for DmCloseNotificationSession

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxCloseNotificationSession(T2XBDMXboxSessionT Session)
{
    printf("xbdm: CloseNotificationSession\n");
    return(uXBDMXboxInitialized ? DmCloseNotificationSession((PDMN_SESSION)Session) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxOpenConnection

    \Description
        See Microsoft Xbox documentation for DmOpenConnection

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxOpenConnection(T2XBDMXboxConnectionT *pConnection)
{
    printf("xbdm: OpenConnection\n");
    return(uXBDMXboxInitialized ? DmOpenConnection((PDM_CONNECTION *)pConnection) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxNotify

    \Description
        See Microsoft Xbox documentation for DmNotify

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxNotify(T2XBDMXboxSessionT Session, uint32_t uNotification, T2XBDMXboxNotifyFunctionT pHandler)
{
    printf("xbdm: Notify\n");
    return(uXBDMXboxInitialized ? DmNotify((PDMN_SESSION)Session, uNotification, (PDM_NOTIFY_FUNCTION)pHandler) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxOpenNotificationSession

    \Description
        See Microsoft Xbox documentation for DmOpenNotificationSession

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxOpenNotificationSession(uint32_t uFlags, T2XBDMXboxSessionT *pSession)
{
    printf("xbdm: OpenNotificationSession\n");
    return(uXBDMXboxInitialized ? DmOpenNotificationSession(uFlags, (PDMN_SESSION *)pSession) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxRegisterNotificationProcessor

    \Description
        See Microsoft Xbox documentation for DmRegisterNotificationProcessor

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxRegisterNotificationProcessor(T2XBDMXboxSessionT Session, char *pType, T2XBDMXboxExtNofityFunctionT pHandler)
{
    printf("xbdm: RegisterNotificationProcessor\n");
    return(uXBDMXboxInitialized ? DmRegisterNotificationProcessor((PDMN_SESSION)Session, pType, pHandler) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxSendCommand

    \Description
        See Microsoft Xbox documentation for DmSendCommand

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxSendCommand(T2XBDMXboxConnectionT Connection, char *pCommand, char *pResponse, uint32_t *pResponseSize)
{
    printf("xbdm: SendCommand\n");
    return(uXBDMXboxInitialized ? DmSendCommand((PDM_CONNECTION)Connection, pCommand, pResponse, pResponseSize) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxSetXboxNameNoRegister

    \Description
        See Microsoft Xbox documentation for DmSetXboxNameNoRegister

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxSetXboxNameNoRegister(char *pName)
{
    printf("xbdm: SetXboxNameNoRegister\n");
    return(uXBDMXboxInitialized ? DmSetXboxNameNoRegister(pName) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxTranslateError

    \Description
        See Microsoft Xbox documentation for DmTranslateError

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxTranslateError(int32_t hr, char *pBuffer, int32_t iBufferMax)
{
    printf("xbdm: TranslateError\n");
    return(uXBDMXboxInitialized ? DmTranslateError(hr, pBuffer, iBufferMax) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxReboot

    \Description
        See Microsoft Xbox documentation for DmReboot

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxReboot(uint32_t uFlags)
{
    printf("xbdm: Reboot\n");
    return(uXBDMXboxInitialized ? DmReboot(uFlags) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXboxShutdown

    \Description
        Shutdown code for the Xbox XBDM.

    \Input None
    
    \Output int32_t - 0 for success

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXboxDllExport int32_t T2XBDMXboxShutdown(void)
{
    printf("xbdmxbox: Shutdown\n");
    uXBDMXboxInitialized = 0;
    return(0);
}
