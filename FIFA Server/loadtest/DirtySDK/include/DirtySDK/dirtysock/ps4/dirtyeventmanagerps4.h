/*H*************************************************************************************/
/*!
    \File dirtyeventmanagerps4.h

    \Description
        DirtyEventManagerps4 wraps the EASystemEventMessageDispatcher functionality
        for delegating callbacks from Sony's UserService

    \Copyright
        Copyright (c) Electronic Arts 2013

    \Version 1.0 07/11/2013 (amakoukji)  First Version
*/
/*************************************************************************************H*/

#ifndef _dirtyeventmanagerps4_h
#define _dirtyeventmanagerps4_h

/*!
\Moduledef DirtyEventManagerPS4 DirtyEventManagerPS4
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include <user_service.h>
#include <system_service.h>
#include <sdk_version.h>

#include "DirtySDK/platform.h"

#if !defined(DIRTYCODE_PS5) && SCE_ORBIS_SDK_VERSION < 0x08008011u
#include <companion_httpd/companion_httpd_types.h>
#endif

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef void (DirtyEventManagerUserServiceCallbackT)(void *pUserData, const SceUserServiceEvent *pEvent);
typedef void (DirtyEventManagerSystemServiceCallbackT)(void *pUserData, const SceSystemServiceEvent *pEvent);
#if !defined(DIRTYCODE_PS5) && SCE_ORBIS_SDK_VERSION < 0x08008011u
typedef void (DirtyEventManagerCompanionHttpdCallbackT)(void *pUserData, const SceCompanionHttpdEvent *pEvent);
typedef void (DirtyEventManagerCompanionUtilCallbackT)(void *pUserData, const SceCompanionUtilEvent *pEvent);
#else
typedef void (DirtyEventManagerCompanionHttpdCallbackT)(void *pUserData, const void *pEvent);
typedef void (DirtyEventManagerCompanionUtilCallbackT)(void *pUserData, const void *pEvent);
#endif

/*** Functions *************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

// initialize the module
int32_t DirtyEventManagerInit(uint8_t bCheckTick);

// shutdown the module
int32_t DirtyEventManagerShutdown(void);

// register user service callback
int32_t DirtyEventManagerRegisterUserService(DirtyEventManagerUserServiceCallbackT *pCallback, void *pUserData);

// register system service callback
int32_t DirtyEventManagerRegisterSystemService(DirtyEventManagerSystemServiceCallbackT *pCallback, void *pUserData);

#if SCE_ORBIS_SDK_VERSION < 0x08008011u
// register companion httpd callback
int32_t DirtyEventManagerRegisterCompanionHttpd(DirtyEventManagerCompanionHttpdCallbackT *pCallback, void *pUserData);

// register companion util callback
int32_t DirtyEventManagerRegisterCompanionUtil(DirtyEventManagerCompanionUtilCallbackT *pCallback, void *pUserData);
#endif

// unregister user service callback
int32_t DirtyEventManagerUnregisterUserService(DirtyEventManagerUserServiceCallbackT *pCallback);

// unregister system service callback
int32_t DirtyEventManagerUnregisterSystemService(DirtyEventManagerSystemServiceCallbackT *pCallback);

#if SCE_ORBIS_SDK_VERSION < 0x08008011u
// unregister companion httpd callback
int32_t DirtyEventManagerUnregisterCompanionHttpd(DirtyEventManagerCompanionHttpdCallbackT *pCallback);

// unregister companion util callback
int32_t DirtyEventManagerUnregisterCompanionUtil(DirtyEventManagerCompanionUtilCallbackT *pCallback);
#endif

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtyeventmanagerps4_h

