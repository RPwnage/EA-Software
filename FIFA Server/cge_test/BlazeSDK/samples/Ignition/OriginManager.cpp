#include "OriginManager.h"

namespace Ignition
{

OriginManager::OriginManager(OriginListener& owner) 
    : mOwner(owner)
{
    gBlazeHub->addIdler(this, "OriginManager");
}

OriginManager::~OriginManager()
{
    gBlazeHub->getScheduler()->removeByAssociatedObject(this);
    gBlazeHub->removeIdler(this);
}

void OriginManager::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
#if defined(EA_PLATFORM_WINDOWS)
    OriginUpdate();
#endif
}

#if defined(EA_PLATFORM_WINDOWS)
OriginErrorT OriginManager::OnAuthCodeReceived(void *pContext, const char **authcode, OriginSizeT length, OriginErrorT error) 
{
    OriginManager *pOriginManager = (OriginManager *)pContext;
    if (authcode == NULL) 
    {
        pOriginManager->mOwner.onOriginAuthCode(NULL);
    }
    else 
    {
        pOriginManager->mOwner.onOriginAuthCode(*authcode);
        
    }
    return error;
}

OriginErrorT OriginManager::OriginResourceCallback(void * pContext, void * resource, OriginSizeT length, OriginErrorT error)
{
    return error;
}

OriginErrorT OriginManager::RegisterCallbacks(void * pContext)
{
    // Register for SDK Events
    OriginRegisterEventCallback(~0x0, EventCallback, pContext);
    OriginRegisterEventCallback(ORIGIN_EVENT_LOGIN, LoginEventCallback, pContext);
    return ORIGIN_SUCCESS;
}

OriginErrorT OriginManager::UnRegisterCallbacks(void * pContext)
{
    // UnRegister for SDK Events
    OriginUnregisterEventCallback(~0x0, EventCallback, pContext);
    OriginUnregisterEventCallback(ORIGIN_EVENT_LOGIN, LoginEventCallback, pContext);
    return ORIGIN_SUCCESS;
}

// Callback for All SDK Event handling
OriginErrorT OriginManager::EventCallback(int32_t eventId, void* pContext, void* eventData, OriginSizeT count)
{
    return ORIGIN_SUCCESS;
}

// Callback for SDK Login Event handling
OriginErrorT OriginManager::LoginEventCallback(int32_t eventId, void* pContext, void* eventData, OriginSizeT count)
{
    OriginLoginT* pLoginEvent = (OriginLoginT *)eventData;
    OriginManager *pOriginManager = (OriginManager *)pContext;

    // OriginGetDefaultUser() value gets updated after a call to OriginGetProfile()
    OriginGetProfile(pLoginEvent->UserIndex, 3000, OriginResourceCallback, pOriginManager);

    if (pLoginEvent->IsLoggedIn)
    {
        pOriginManager->mOwner.onOriginClientLogin();
    }
    else
    {
        pOriginManager->mOwner.onOriginClientLogout();
    }

    return ORIGIN_SUCCESS;
}
#endif

void OriginManager::startOrigin(const char* contentId, const char* title)
{
#if defined(EA_PLATFORM_WINDOWS)
    OriginStartupOutputT out;
    OriginStartupInputT in;

    in.ContentId = contentId; //  "71314";
    in.MultiplayerId = "";
    in.Title = title;         //  "SDK Tool";
    in.Language = "en_US";
    
    OriginErrorT err = OriginStartup(0, 0, &in, &out);
    if (err == ORIGIN_SUCCESS || err == ORIGIN_WARNING_SDK_ALREADY_INITIALIZED)
    {
        if (err == ORIGIN_WARNING_SDK_ALREADY_INITIALIZED)
            OriginShutdown();
        else 
            RegisterCallbacks(this);

        if (OriginGetDefaultUser() != 0)
        {
            mOwner.onOriginClientLogin();
        }
    }
    else
    {
        OriginShutdown();
    }
#else
    //no op on console platform, used to remove unused mOwner field warning
    mOwner.onOriginClientLogin();
#endif
}

void OriginManager::shutdownOrigin()
{
#if defined(EA_PLATFORM_WINDOWS)
    UnRegisterCallbacks(this);
    OriginShutdown();
#endif
}

void OriginManager::requestAuthCode(const char8_t* clientId)
{
#if defined(EA_PLATFORM_WINDOWS)
    OriginRequestAuthCode(OriginGetDefaultUser(), clientId, OnAuthCodeReceived, this, 300000);
#endif
}

} // Ignition