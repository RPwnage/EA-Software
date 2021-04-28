/*H********************************************************************************/
/*!
    \File t2xbdmxbox.h

    \Description
        Wrapper for the Microsoft (tm) debug manager interface.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 05/11/2005 (jfrank) First Version
*/
/********************************************************************************H*/

#ifndef _t2xbdmxbox_h
#define _t2xbdmxbox_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

// use this macro to declare the xbdm functions for inclusion in the imports library
#define T2XBDMXboxDllExport __declspec(dllexport) 

/*** Macros ***********************************************************************/

#define T2XBDMXBOX_REBOOT_WAIT       (1)
#define T2XBDMXBOX_REBOOT_WARM       (2)
#define T2XBDMXBOX_REBOOT_NODEBUG    (4)
#define T2XBDMXBOX_REBOOT_STOP       (8)

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// The following functions are all direct wrappers for the xbdm debug library
// for documentation on the calls, please see either the xbox or xenon
// debug monitor documentation.

//HRESULT DmCloseConnection(PDM_CONNECTION pConnection);
//HRESULT DmCloseNotificationSession(PDMN_SESSION Session);
//HRESULT DmOpenConnection(PDM_CONNECTION *ppConnection);
//HRESULT DmNotify(PDMN_SESSION Session, DWORD dwNotification, PDM_NOTIFY_FUNCTION pfnHandler);
//HRESULT DmOpenNotificationSession(DWORD dwFlags, PDMN_SESSION *pSession);
//HRESULT DmRegisterNotificationProcessor(PDMN_SESSION Session, LPCSTR szType, PDM_EXT_NOTIFY_FUNCTION pfn);
//HRESULT DmSendCommand(PDM_CONNECTION pConnection, LPCSTR szCommand, LPSTR szResponse, LPDWORD lpdwResponseSize);
//HRESULT DmSetXboxNameNoRegister(LPCSTR name);
//HRESULT DmTranslateError(HRESULT hr, LPTSTR lpBuffer, int32_t nBufferMax);

typedef void * T2XBDMXboxSessionT;
typedef void * T2XBDMXboxConnectionT;
typedef uint32_t(__stdcall *T2XBDMXboxExtNofityFunctionT)(const char *);
typedef uint32_t(__stdcall *T2XBDMXboxNotifyFunctionT)(uint32_t, uint32_t);

T2XBDMXboxDllExport int32_t T2XBDMXboxStartup(void);
T2XBDMXboxDllExport int32_t T2XBDMXboxCloseConnection(T2XBDMXboxConnectionT Connection);
T2XBDMXboxDllExport int32_t T2XBDMXboxCloseNotificationSession(T2XBDMXboxSessionT Session);
T2XBDMXboxDllExport int32_t T2XBDMXboxOpenConnection(T2XBDMXboxConnectionT *pConnection);
T2XBDMXboxDllExport int32_t T2XBDMXboxNotify(T2XBDMXboxSessionT Session, uint32_t uNotification, T2XBDMXboxNotifyFunctionT Handler);
T2XBDMXboxDllExport int32_t T2XBDMXboxOpenNotificationSession(uint32_t uFlags, T2XBDMXboxSessionT *pSession);
T2XBDMXboxDllExport int32_t T2XBDMXboxRegisterNotificationProcessor(T2XBDMXboxSessionT Session, char *pType, T2XBDMXboxExtNofityFunctionT Handler);
T2XBDMXboxDllExport int32_t T2XBDMXboxSendCommand(T2XBDMXboxConnectionT Connection, char *pCommand, char *pResponse, uint32_t *pResponseSize);
T2XBDMXboxDllExport int32_t T2XBDMXboxSetXboxNameNoRegister(char *pName);
T2XBDMXboxDllExport int32_t T2XBDMXboxTranslateError(int32_t hr, char *pBuffer, int32_t iBufferMax);
T2XBDMXboxDllExport int32_t T2XBDMXboxReboot(uint32_t uFlags);
T2XBDMXboxDllExport int32_t T2XBDMXboxShutdown(void);

#ifdef __cplusplus
};
#endif

#endif // _t2xbdmxbox_h

