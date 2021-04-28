/*H********************************************************************************/
/*!
    \File t2xbdmxenon.c

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
#include <xbdm.h>
#include "DirtySDK/platform.h"
#include "t2xbdmxenon.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

uint32_t uXBDMXenonInitialized;

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonMain

    \Description
        Entry point for this DLL.

    \Input *pModule   - unused
    \Input  uReason   - unused
    \Input *pReserved - unused
    
    \Output int32_t       - always returns 1 (success)

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t APIENTRY T2XBDMXenonMain( void *pModule, uint32_t uReason, void *pReserved)
{
    switch(uReason)
    {
        case DLL_PROCESS_ATTACH:    printf("xbdm: DLL_PROCESS_ATTACH - uInit(%u)\n", uXBDMXenonInitialized);   break;
        case DLL_THREAD_ATTACH:     printf("xbdm: DLL_THREAD_ATTACH  - uInit(%u)\n", uXBDMXenonInitialized);   break;
        case DLL_THREAD_DETACH:     printf("xbdm: DLL_THREAD_DETACH  - uInit(%u)\n", uXBDMXenonInitialized);   break;
        case DLL_PROCESS_DETACH:    printf("xbdm: DLL_PROCESS_DETACH - uInit(%u)\n", uXBDMXenonInitialized);   break;
        default:  printf("xbdm: Unknown reason for call [%d = 0x%x]  - uInit(%u)\n", uReason, uReason, uXBDMXenonInitialized);
    }
    return(1);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonStartup

    \Description
        Initialization code for the Xenon XBDM.

    \Input None
    
    \Output int32_t - 0 for success, <0 indicates an error occurred

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonStartup(void)
{
    const char *strPathXEDK = getenv("xedk");
    const char *strPath     = getenv("path");
    char *pPathXBDM;
    uint32_t uPathSize = 0;
    HMODULE hXBDM;

    printf("xbdmxenon: Startup\n");

    // load the xenon debug manager
    if (!strPathXEDK)
    {
        strPathXEDK = "";
    }

    // Build up a new path using std::string to handle memory management.
    uPathSize += (uint32_t)strlen(strPath);
    uPathSize += (uint32_t)strlen(strPathXEDK);
    uPathSize += 100;       // fudge factor for extras we will add on
    pPathXBDM = (char *)malloc(uPathSize);
    memset(pPathXBDM, 0, uPathSize);
    strcat(pPathXBDM, "path=");
    strcat(pPathXBDM, strPath);
    strcat(pPathXBDM, ";");
    strcat(pPathXBDM, strPathXEDK);
    strcat(pPathXBDM, "\\bin\\win32");

    // Set the path to the update version and free memory
    _putenv( pPathXBDM );
    free(pPathXBDM);

    // load the library
    hXBDM = LoadLibrary("xbdm.dll");
    if (!hXBDM)
    {
        if ( strPathXEDK[0] )
        {
            printf("xbdmxenon: Couldn't load xbdm.dll");
            return(-1);
        }
        else
        {
            printf("xbdmxenon: Couldn't load xbdm.dll\nXEDK environment variable not set.");
            return(-1);
        }
    }
    printf("xbdmxenon: loaded [xbdm.dll]\n");
    uXBDMXenonInitialized = 1;
    return(0);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonCloseConnection

    \Description
        See Microsoft Xenon documentation for DmCloseConnection

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonCloseConnection(T2XBDMXenonConnectionT Connection)
{
    printf("xbdm: CloseConnection\n");
    return(uXBDMXenonInitialized ? DmCloseConnection((PDM_CONNECTION)Connection) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonCloseNotificationSession

    \Description
        See Microsoft Xenon documentation for DmCloseNotificationSession

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonCloseNotificationSession(T2XBDMXenonSessionT Session)
{
    printf("xbdm: CloseNotificationSession\n");
    return(uXBDMXenonInitialized ? DmCloseNotificationSession((PDMN_SESSION)Session) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonOpenConnection

    \Description
        See Microsoft Xenon documentation for DmOpenConnection

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonOpenConnection(T2XBDMXenonConnectionT *pConnection)
{
    printf("xbdm: OpenConnection\n");
    return(uXBDMXenonInitialized ? DmOpenConnection((PDM_CONNECTION *)pConnection) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonNotify

    \Description
        See Microsoft Xenon documentation for DmNotify

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonNotify(T2XBDMXenonSessionT Session, uint32_t uNotification, T2XBDMXenonNotifyFunctionT pHandler)
{
    printf("xbdm: Notify\n");
    return(uXBDMXenonInitialized ? DmNotify((PDMN_SESSION)Session, uNotification, (PDM_NOTIFY_FUNCTION)pHandler) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonOpenNotificationSession

    \Description
        See Microsoft Xenon documentation for DmOpenNotificationSession

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonOpenNotificationSession(uint32_t uFlags, T2XBDMXenonSessionT *pSession)
{
    printf("xbdm: OpenNotificationSession\n");
    return(uXBDMXenonInitialized ? DmOpenNotificationSession(uFlags, (PDMN_SESSION *)pSession) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonRegisterNotificationProcessor

    \Description
        See Microsoft Xenon documentation for DmRegisterNotificationProcessor

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonRegisterNotificationProcessor(T2XBDMXenonSessionT Session, char *pType, T2XBDMXenonExtNofityFunctionT pHandler)
{
    printf("xbdm: RegisterNotificationProcessor\n");
    return(uXBDMXenonInitialized ? DmRegisterNotificationProcessor((PDMN_SESSION)Session, pType, pHandler) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonSendCommand

    \Description
        See Microsoft Xenon documentation for DmSendCommand

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonSendCommand(T2XBDMXenonConnectionT Connection, char *pCommand, char *pResponse, uint32_t *pResponseSize)
{
    printf("xbdm: SendCommand\n");
    return(uXBDMXenonInitialized ? DmSendCommand((PDM_CONNECTION)Connection, pCommand, pResponse, pResponseSize) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonSetXboxNameNoRegister

    \Description
        See Microsoft Xenon documentation for DmSetXboxNameNoRegister

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonSetXboxNameNoRegister(char *pName)
{
    printf("xbdm: SetXenonNameNoRegister\n");
    return(uXBDMXenonInitialized ? DmSetXboxNameNoRegister(pName) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonTranslateError

    \Description
        See Microsoft Xenon documentation for DmTranslateError

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonTranslateError(int32_t hr, char *pBuffer, int32_t iBufferMax)
{
    printf("xbdm: TranslateError\n");
    return(uXBDMXenonInitialized ? DmTranslateError(hr, pBuffer, iBufferMax) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonReboot

    \Description
        See Microsoft Xenon documentation for DmReboot

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonReboot(uint32_t uFlags)
{
    printf("xbdm: Reboot\n");
    return(uXBDMXenonInitialized ? DmReboot(uFlags) : -1);
}


/*F********************************************************************************/
/*!
    \Function T2XBDMXenonShutdown

    \Description
        Shutdown code for the Xenon XBDM.

    \Input None
    
    \Output int32_t - 0 for success

    \Version 05/18/2005 (jfrank)
*/
/********************************************************************************F*/
T2XBDMXenonDllExport int32_t T2XBDMXenonShutdown(void)
{
    printf("xbdmxenon: Shutdown\n");
    uXBDMXenonInitialized = 0;
    return(0);
}
