/*H*************************************************************************************/
/*!
    \File dirtyeventmanagerps4.cpp

    \Description
        DirtyEventManagerps4 wraps the EASystemEventMessageDispatcher functionality
        for delegating callbacks from Sony's UserService

    \Copyright
        Copyright (c) / Electronic Arts 2013.  ALL RIGHTS RESERVED.

    \Version 1.0 07/11/2013 (amakoukji)  First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/
#include "EASystemEventMessageDispatcher/SystemEventMessageDispatcher.h"
#include "DirtySDK/dirtysock/ps4/dirtyeventmanagerps4.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "coreallocator/icoreallocator_interface.h"
#include <new>

/*** Variables 1 ***********************************************************************/

#define DIRTYEVENTMANAGER_MAX_DELEGATES 100

static DirtyEventManagerUserServiceCallbackT    *pgHandlerUserService[DIRTYEVENTMANAGER_MAX_DELEGATES];    //<! list of callbacks
static DirtyEventManagerSystemServiceCallbackT  *pgHandlerSystemService[DIRTYEVENTMANAGER_MAX_DELEGATES];  //<! list of callbacks
static DirtyEventManagerCompanionHttpdCallbackT *pgHandlerCompanionHttpd[DIRTYEVENTMANAGER_MAX_DELEGATES]; //<! list of callbacks
static DirtyEventManagerCompanionUtilCallbackT  *pgHandlerCompanionUtil[DIRTYEVENTMANAGER_MAX_DELEGATES];  //<! list of callbacks

static void *pgUserDataUserService[DIRTYEVENTMANAGER_MAX_DELEGATES];      //<! list of user objects
static void *pgUserDataSystemService[DIRTYEVENTMANAGER_MAX_DELEGATES];    //<! list of user objects
static void *pgUserDataCompanionHttpd[DIRTYEVENTMANAGER_MAX_DELEGATES];   //<! list of user objects
static void *pgUserDataCompanionUtil[DIRTYEVENTMANAGER_MAX_DELEGATES];    //<! list of user objects

/*** Defines ***************************************************************************/

// DirtyEventHandler defines how to deal with the callback from EATech's SystemEventMessageDispatcher
// in our case we will simply be pushing the event forward to our own handler
struct DirtyEventHandler : EA::Messaging::IHandler
{
    virtual bool HandleMessage(EA::Messaging::MessageId messageId, void* pMessage)
    {
        const SceUserServiceEvent *pUserServiceEvent = NULL;
        const SceSystemServiceEvent *pSystemServiceEvent = NULL;
        const SceCompanionUtilEvent *pCompanionUtilEvent = NULL;
        const SceCompanionHttpdEvent *pCompanionHttpdEvent = NULL;
        EA::SEMD::Message *pSEMDMessage = reinterpret_cast<EA::SEMD::Message*>(pMessage);

        if (messageId == EA::SEMD::kGroupUserService)
        {
            pUserServiceEvent = pSEMDMessage->GetData<const SceUserServiceEvent*>();

            // forward to our own handler
            for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
            {
                if (pgHandlerUserService[i])
                {
                    pgHandlerUserService[i](pgUserDataUserService[i], pUserServiceEvent);
                }
            }
            
            return(true);
        }
        else if (messageId == EA::SEMD::kGroupSystemService)
        {
            pSystemServiceEvent = pSEMDMessage->GetData<const SceSystemServiceEvent*>();

            // forward to our own handler
            for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
            {
                if (pgHandlerSystemService[i])
                {
                    pgHandlerSystemService[i](pgUserDataSystemService[i], pSystemServiceEvent);
                }
            }
            
            return(true);
        }
        else if (messageId == EA::SEMD::kGroupCompanionUtil)
        {
            pCompanionUtilEvent = pSEMDMessage->GetData<const SceCompanionUtilEvent*>();

            // forward to our own handler
            for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
            {
                if (pgHandlerCompanionUtil[i])
                {
                    pgHandlerCompanionUtil[i](pgUserDataCompanionUtil[i], pCompanionUtilEvent);
                }
            }
            
            return(true);
        }
        else if (messageId == EA::SEMD::kGroupCompanionHttpd)
        {
            pCompanionHttpdEvent = pSEMDMessage->GetData<const SceCompanionHttpdEvent*>();

            // forward to our own handler
            for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
            {
                if (pgHandlerCompanionHttpd[i])
                {
                    pgHandlerCompanionHttpd[i](pgUserDataCompanionHttpd[i], pCompanionHttpdEvent);
                }
            }
            
            return(true);
        }

        return(false);
    }
};

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Variables 2 ********************************************************************/

static EA::SEMD::SystemEventMessageDispatcher  *_pgSystemEventMessageDispatcher = NULL;          //!< module state pointer
static DirtyEventHandler _gHandler;

/*** Private Functions *****************************************************************/

int32_t _DirtyEventManagerInit(void);
int32_t _DirtyEventManagerShutdown(void);
int32_t _DirtyEventManagerRegisterUserService(DirtyEventManagerUserServiceCallbackT *pCallback, void *pUserData);
int32_t _DirtyEventManagerRegisterSystemService(DirtyEventManagerSystemServiceCallbackT *pCallback, void *pUserData);
int32_t _DirtyEventManagerRegisterCompanionHttpd(DirtyEventManagerCompanionHttpdCallbackT *pCallback, void *pUserData);
int32_t _DirtyEventManagerRegisterCompanionUtil(DirtyEventManagerCompanionUtilCallbackT *pCallback, void *pUserData);
int32_t _DirtyEventManagerUnregisterUserService(DirtyEventManagerUserServiceCallbackT *pCallback);
int32_t _DirtyEventManagerUnregisterSystemService(DirtyEventManagerSystemServiceCallbackT *pCallback);
int32_t _DirtyEventManagerUnregisterCompanionHttpd(DirtyEventManagerCompanionHttpdCallbackT *pCallback);
int32_t _DirtyEventManagerUnregisterCompanionUtil(DirtyEventManagerCompanionUtilCallbackT *pCallback);

int32_t _DirtyEventManagerInit()
{
    bool bAlreadyInitialized = (_pgSystemEventMessageDispatcher != NULL);

    _pgSystemEventMessageDispatcher = EA::SEMD::SystemEventMessageDispatcher::GetInstance();

    if (_pgSystemEventMessageDispatcher == NULL)
    {
        NetPrintf(("dirtyeventmanagerps4: error, SystemEventMessageDispatcher has not been initialized.\n"));
        return(-1);
    }

    if (_pgSystemEventMessageDispatcher->HasTicked())
    {
        NetPrintf(("dirtyeventmanagerps4: ************************************************************************************\n"));
        NetPrintf(("dirtyeventmanagerps4: error, EASEMD has ticked before DirtyEventManager was initialized. Events may have been lost.\n"));
        NetPrintf(("dirtyeventmanagerps4: *** PLEASE INITIALIZE THE DIRTYEVENTMANAGER EARLIER ***\n"));
        return(-1);
    }

    // if handlers are not already registered, register them
    if (!bAlreadyInitialized)
    {
        _pgSystemEventMessageDispatcher->AddMessageHandler(&_gHandler, EA::SEMD::kGroupUserService, false, EA::Messaging::kPriorityHigh);
        _pgSystemEventMessageDispatcher->AddMessageHandler(&_gHandler, EA::SEMD::kGroupSystemService, false, EA::Messaging::kPriorityHigh);
        _pgSystemEventMessageDispatcher->AddMessageHandler(&_gHandler, EA::SEMD::kGroupCompanionUtil, false, EA::Messaging::kPriorityHigh);
        _pgSystemEventMessageDispatcher->AddMessageHandler(&_gHandler, EA::SEMD::kGroupCompanionHttpd, false, EA::Messaging::kPriorityHigh);

        for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
        {
            pgHandlerUserService[i]  = NULL;
            pgHandlerSystemService[i]  = NULL;
            pgHandlerCompanionHttpd[i]  = NULL;
            pgHandlerCompanionUtil[i]  = NULL;

            pgUserDataUserService[i]  = NULL;
            pgUserDataSystemService[i]  = NULL;
            pgUserDataCompanionHttpd[i]  = NULL;
            pgUserDataCompanionUtil[i]  = NULL;
        }
    }

    return(0);
}

int32_t _DirtyEventManagerShutdown(void)
{
    if (_pgSystemEventMessageDispatcher == NULL)
    {
        NetPrintf(("dirtyeventmanagerps4: DirtyEventManagerShutdown() called while module is inactive\n"));
        return(0);
    }

    // clean up the dispatcher
    _pgSystemEventMessageDispatcher->RemoveMessageHandler(&_gHandler, EA::SEMD::kGroupUserService);
    _pgSystemEventMessageDispatcher->RemoveMessageHandler(&_gHandler, EA::SEMD::kGroupSystemService);
    _pgSystemEventMessageDispatcher->RemoveMessageHandler(&_gHandler, EA::SEMD::kGroupCompanionUtil);
    _pgSystemEventMessageDispatcher->RemoveMessageHandler(&_gHandler, EA::SEMD::kGroupCompanionHttpd);
    
    // reset state
    _pgSystemEventMessageDispatcher = NULL;
    for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
    {
        pgHandlerUserService[i]  = NULL;
        pgHandlerSystemService[i]  = NULL;
        pgHandlerCompanionHttpd[i]  = NULL;
        pgHandlerCompanionUtil[i]  = NULL;

        pgUserDataUserService[i]  = NULL;
        pgUserDataSystemService[i]  = NULL;
        pgUserDataCompanionHttpd[i]  = NULL;
        pgUserDataCompanionUtil[i]  = NULL;
    }

    return(0);
}

int32_t _DirtyEventManagerRegisterUserService(DirtyEventManagerUserServiceCallbackT *pCallback, void *pUserData)
{
    int32_t iFirstAvailable = -1;

    // loop through all the available callback to find the first empty slot and check to see if the callback is already registered.
    for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
    {
        if (iFirstAvailable < 0 && pgHandlerUserService[i]  == NULL)
        {
            iFirstAvailable = i;
        }

        if (pgHandlerUserService[i]  == pCallback)
        {
            NetPrintf(("dirtyeventmanagerps4: attempt to register duplicate handler [%p].\n", pCallback));
            return(-1);
        }
    }

    if (iFirstAvailable >= 0 && iFirstAvailable < DIRTYEVENTMANAGER_MAX_DELEGATES)
    {
        pgHandlerUserService[iFirstAvailable] = pCallback;
        pgUserDataUserService[iFirstAvailable] = pUserData;
        return(0);
    }
    else
    {
        NetPrintf(("dirtyeventmanagerps4: unable to register handler [%p], queue is full.\n", pCallback));
        return(-1);
    }
}

int32_t _DirtyEventManagerRegisterSystemService(DirtyEventManagerSystemServiceCallbackT *pCallback, void *pUserData)
{
    int32_t iFirstAvailable = -1;

    // loop through all the available callback to find the first empty slot and check to see if the callback is already registered.
    for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
    {
        if (iFirstAvailable < 0 && pgHandlerSystemService[i]  == NULL)
        {
            iFirstAvailable = i;
        }

        if (pgHandlerSystemService[i]  == pCallback)
        {
            NetPrintf(("dirtyeventmanagerps4: attempt to register duplicate handler [%p].\n", pCallback));
            return(-1);
        }
    }

    if (iFirstAvailable >= 0 && iFirstAvailable < DIRTYEVENTMANAGER_MAX_DELEGATES)
    {
        pgHandlerSystemService[iFirstAvailable] = pCallback;
        pgUserDataSystemService[iFirstAvailable] = pUserData;
        return(0);
    }
    else
    {
        NetPrintf(("dirtyeventmanagerps4: unable to register handler [%p], queue is full.\n", pCallback));
        return(-1);
    }
}

int32_t _DirtyEventManagerRegisterCompanionHttpd(DirtyEventManagerCompanionHttpdCallbackT *pCallback, void *pUserData)
{
    int32_t iFirstAvailable = -1;

    // loop through all the available callback to find the first empty slot and check to see if the callback is already registered.
    for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
    {
        if (iFirstAvailable < 0 && pgHandlerCompanionHttpd[i]  == NULL)
        {
            iFirstAvailable = i;
        }

        if (pgHandlerCompanionHttpd[i]  == pCallback)
        {
            NetPrintf(("dirtyeventmanagerps4: attempt to register duplicate handler [%p].\n", pCallback));
            return(-1);
        }
    }

    if (iFirstAvailable >= 0 && iFirstAvailable < DIRTYEVENTMANAGER_MAX_DELEGATES)
    {
        pgHandlerCompanionHttpd[iFirstAvailable] = pCallback;
        pgUserDataCompanionHttpd[iFirstAvailable] = pUserData;
        return(0);
    }
    else
    {
        NetPrintf(("dirtyeventmanagerps4: unable to register handler [%p], queue is full.\n", pCallback));
        return(-1);
    }
}

int32_t _DirtyEventManagerRegisterCompanionUtil(DirtyEventManagerCompanionUtilCallbackT *pCallback, void *pUserData)
{
    int32_t iFirstAvailable = -1;

    // loop through all the available callback to find the first empty slot and check to see if the callback is already registered.
    for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
    {
        if (iFirstAvailable < 0 && pgHandlerCompanionUtil[i]  == NULL)
        {
            iFirstAvailable = i;
        }

        if (pgHandlerCompanionUtil[i]  == pCallback)
        {
            NetPrintf(("dirtyeventmanagerps4: attempt to register duplicate handler [%p].\n", pCallback));
            return(-1);
        }
    }

    if (iFirstAvailable >= 0 && iFirstAvailable < DIRTYEVENTMANAGER_MAX_DELEGATES)
    {
        pgHandlerCompanionUtil[iFirstAvailable] = pCallback;
        pgUserDataCompanionUtil[iFirstAvailable] = pUserData;
        return(0);
    }
    else
    {
        NetPrintf(("dirtyeventmanagerps4: unable to register handler [%p], queue is full.\n", pCallback));
        return(-1);
    }
}

int32_t _DirtyEventManagerUnregisterUserService(DirtyEventManagerUserServiceCallbackT *pCallback)
{
    for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
    {
        if (pCallback == pgHandlerUserService[i])
        {
            pgHandlerUserService[i] = NULL;
            pgUserDataUserService[i] = NULL;
            return(0);
        }
    }

    NetPrintf(("dirtyeventmanagerps4: attempt to unregister unknown handler [%p]\n", pCallback));
    return(-1);
}

int32_t _DirtyEventManagerUnregisterSystemService(DirtyEventManagerSystemServiceCallbackT *pCallback)
{
    for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
    {
        if (pCallback == pgHandlerSystemService[i])
        {
            pgHandlerSystemService[i] = NULL;
            pgUserDataSystemService[i] = NULL;
            return(0);
        }
    }

    NetPrintf(("dirtyeventmanagerps4: attempt to unregister unknown handler [%p]\n", pCallback));
    return(-1);
}

int32_t _DirtyEventManagerUnregisterCompanionHttpd(DirtyEventManagerCompanionHttpdCallbackT *pCallback)
{
    for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
    {
        if (pCallback == pgHandlerCompanionHttpd[i])
        {
            pgHandlerCompanionHttpd[i] = NULL;
            pgUserDataCompanionHttpd[i] = NULL;
            return(0);
        }
    }

    NetPrintf(("dirtyeventmanagerps4: attempt to unregister unknown handler [%p]\n", pCallback));
    return(-1);
}

int32_t _DirtyEventManagerUnregisterCompanionUtil(DirtyEventManagerCompanionUtilCallbackT *pCallback)
{
    for (uint32_t i = 0; i < DIRTYEVENTMANAGER_MAX_DELEGATES; ++i)
    {
        if (pCallback == pgHandlerCompanionUtil[i])
        {
            pgHandlerCompanionUtil[i] = NULL;
            pgUserDataCompanionUtil[i] = NULL;
            return(0);
        }
    }

    NetPrintf(("dirtyeventmanagerps4: attempt to unregister unknown handler [%p]\n", pCallback));
    return(-1);
}

/*** Public Functions *****************************************************************/

// C style wrappers that call C++ functionality

extern "C" int32_t DirtyEventManagerInit(void)
{
    return(_DirtyEventManagerInit());
}

extern "C" int32_t DirtyEventManagerShutdown(void)
{
    return(_DirtyEventManagerShutdown());
}

extern "C" int32_t DirtyEventManagerRegisterUserService(DirtyEventManagerUserServiceCallbackT *pCallback, void *pUserData)
{
    return(_DirtyEventManagerRegisterUserService(pCallback, pUserData));
}

extern "C" int32_t DirtyEventManagerRegisterSystemService(DirtyEventManagerSystemServiceCallbackT *pCallback, void *pUserData)
{
    return(_DirtyEventManagerRegisterSystemService(pCallback, pUserData));
}

extern "C" int32_t DirtyEventManagerRegisterCompanionHttpd(DirtyEventManagerCompanionHttpdCallbackT *pCallback, void *pUserData)
{
    return(_DirtyEventManagerRegisterCompanionHttpd(pCallback, pUserData));
}

extern "C" int32_t DirtyEventManagerRegisterCompanionUtil(DirtyEventManagerCompanionUtilCallbackT *pCallback, void *pUserData)
{
    return(_DirtyEventManagerRegisterCompanionUtil(pCallback, pUserData));
}

extern "C" int32_t DirtyEventManagerUnregisterUserService(DirtyEventManagerUserServiceCallbackT *pCallback)
{
    return(_DirtyEventManagerUnregisterUserService(pCallback));
}

extern "C" int32_t DirtyEventManagerUnregisterSystemService(DirtyEventManagerSystemServiceCallbackT *pCallback)
{
    return(_DirtyEventManagerUnregisterSystemService(pCallback));
}

extern "C" int32_t DirtyEventManagerUnregisterCompanionHttpd(DirtyEventManagerCompanionHttpdCallbackT *pCallback)
{
    return(_DirtyEventManagerUnregisterCompanionHttpd(pCallback));
}

extern "C" int32_t DirtyEventManagerUnregisterCompanionUtil(DirtyEventManagerCompanionUtilCallbackT *pCallback)
{
    return(_DirtyEventManagerUnregisterCompanionUtil(pCallback));
}








