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
#include <companion_httpd/companion_httpd_types.h>

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef void (DirtyEventManagerUserServiceCallbackT)(void *pUserData, const SceUserServiceEvent *pEvent);
typedef void (DirtyEventManagerSystemServiceCallbackT)(void *pUserData, const SceSystemServiceEvent *pEvent);
typedef void (DirtyEventManagerCompanionHttpdCallbackT)(void *pUserData, const SceCompanionHttpdEvent *pEvent);
typedef void (DirtyEventManagerCompanionUtilCallbackT)(void *pUserData, const SceCompanionUtilEvent *pEvent);

/*** Functions *************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

// pDisptacher must be a EA::SEMD::SystemEventMessageDispatcher
// we cannot treat it as such here because this is a C interface
int32_t DirtyEventManagerInit(void);

int32_t DirtyEventManagerUpdate(void);

int32_t DirtyEventManagerShutdown(void);

int32_t DirtyEventManagerRegisterUserService(DirtyEventManagerUserServiceCallbackT *pCallback, void *pUserData);
int32_t DirtyEventManagerRegisterSystemService(DirtyEventManagerSystemServiceCallbackT *pCallback, void *pUserData);
int32_t DirtyEventManagerRegisterCompanionHttpd(DirtyEventManagerCompanionHttpdCallbackT *pCallback, void *pUserData);
int32_t DirtyEventManagerRegisterCompanionUtil(DirtyEventManagerCompanionUtilCallbackT *pCallback, void *pUserData);

int32_t DirtyEventManagerUnregisterUserService(DirtyEventManagerUserServiceCallbackT *pCallback);
int32_t DirtyEventManagerUnregisterSystemService(DirtyEventManagerSystemServiceCallbackT *pCallback);
int32_t DirtyEventManagerUnregisterCompanionHttpd(DirtyEventManagerCompanionHttpdCallbackT *pCallback);
int32_t DirtyEventManagerUnregisterCompanionUtil(DirtyEventManagerCompanionUtilCallbackT *pCallback);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtyeventmanagerps4_h

