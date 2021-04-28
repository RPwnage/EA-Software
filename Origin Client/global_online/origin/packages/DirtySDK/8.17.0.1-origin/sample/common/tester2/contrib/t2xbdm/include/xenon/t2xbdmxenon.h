/*H********************************************************************************/
/*!
    \File t2xbdmxenon.h

    \Description
        Wrapper for the Microsoft (tm) debug manager interface.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 05/11/2005 (jfrank) First Version
*/
/********************************************************************************H*/

#ifndef _t2xbdmxenon_h
#define _t2xbdmxenon_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

// use this macro to declare the xbdm functions for inclusion in the imports library
#define T2XBDMXenonDllExport __declspec(dllexport) 

/*** Macros ***********************************************************************/

#define T2XBDMXENON_REBOOT_TITLE      (0)
#define T2XBDMXENON_REBOOT_WAIT       (1)
#define T2XBDMXENON_REBOOT_WARM       (2)
#define T2XBDMXENON_REBOOT_COLD       (4)
#define T2XBDMXENON_REBOOT_STOP       (8)

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

typedef void * T2XBDMXenonSessionT;
typedef void * T2XBDMXenonConnectionT;
typedef uint32_t(__stdcall *T2XBDMXenonExtNofityFunctionT)(const char *);
typedef uint32_t(__stdcall *T2XBDMXenonNotifyFunctionT)(uint32_t, uint32_t);

T2XBDMXenonDllExport int32_t T2XBDMXenonStartup(void);
T2XBDMXenonDllExport int32_t T2XBDMXenonCloseConnection(T2XBDMXenonConnectionT Connection);
T2XBDMXenonDllExport int32_t T2XBDMXenonCloseNotificationSession(T2XBDMXenonSessionT Session);
T2XBDMXenonDllExport int32_t T2XBDMXenonOpenConnection(T2XBDMXenonConnectionT *pConnection);
T2XBDMXenonDllExport int32_t T2XBDMXenonNotify(T2XBDMXenonSessionT Session, uint32_t uNotification, T2XBDMXenonNotifyFunctionT Handler);
T2XBDMXenonDllExport int32_t T2XBDMXenonOpenNotificationSession(uint32_t uFlags, T2XBDMXenonSessionT *pSession);
T2XBDMXenonDllExport int32_t T2XBDMXenonRegisterNotificationProcessor(T2XBDMXenonSessionT Session, char *pType, T2XBDMXenonExtNofityFunctionT Handler);
T2XBDMXenonDllExport int32_t T2XBDMXenonSendCommand(T2XBDMXenonConnectionT Connection, char *pCommand, char *pResponse, uint32_t *pResponseSize);
T2XBDMXenonDllExport int32_t T2XBDMXenonSetXboxNameNoRegister(char *pName);
T2XBDMXenonDllExport int32_t T2XBDMXenonTranslateError(int32_t hr, char *pBuffer, int32_t iBufferMax);
T2XBDMXenonDllExport int32_t T2XBDMXenonReboot(uint32_t uFlags);
T2XBDMXenonDllExport int32_t T2XBDMXenonShutdown(void);

#ifdef __cplusplus
};
#endif

#endif // _t2xbdmxenon_h

