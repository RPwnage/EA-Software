#ifndef ORIGIN_MANAGER_H
#define ORIGIN_MANAGER_H

#if defined(EA_PLATFORM_WINDOWS)
#include <OriginSDK/OriginSDK.h>
#include <OriginSDK/OriginError.h>
#include <OriginSDK/OriginTypes.h>
#endif
#include "BlazeSDK/blazehub.h"
#include "Ignition/Ignition.h"

namespace Ignition
{
    class OriginListener
    {
    public:
        virtual void onOriginClientLogin() = 0;
        virtual void onOriginClientLogout() = 0;
        virtual void onOriginAuthCode(const char8_t* code) = 0;
    };

    class OriginManager : public Blaze::Idler {
        OriginListener& mOwner;
    public:
        OriginManager(OriginListener& owner);
        ~OriginManager();
#ifdef EA_PLATFORM_WINDOWS
        OriginErrorT RegisterCallbacks(void* pContext);
        OriginErrorT UnRegisterCallbacks(void* pContext);
#endif
        virtual void idle(const uint32_t, const uint32_t);
#if defined(EA_PLATFORM_WINDOWS)

        static OriginErrorT EventCallback(int32_t eventId, void* context, void* eventData, OriginSizeT count);
        static OriginErrorT LoginEventCallback(int32_t eventId, void* contexst, void* eventData, OriginSizeT count);
        static OriginErrorT OnAuthCodeReceived(void* context, const char **authCode, OriginSizeT length, OriginErrorT error);
        static OriginErrorT OriginResourceCallback (void * pContext, void * resource, OriginSizeT length, OriginErrorT error);
#endif
        void startOrigin(const char* contentId, const char* title);
        void shutdownOrigin();
        void requestAuthCode(const char8_t* clientId);
    };
}
#endif