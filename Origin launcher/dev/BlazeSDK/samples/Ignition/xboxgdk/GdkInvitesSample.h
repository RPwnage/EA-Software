/*! ************************************************************************************************/
/*!
    \file GdkInvitesSample.h.h

    \brief Util for Xbox invites handling

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef GDK_INVITES_SAMPLE_H
#define GDK_INVITES_SAMPLE_H

#if defined(EA_PLATFORM_GDK)

#include "BlazeSDK/blazesdk.h"
#include <XTaskQueue.h>
#include <XGameInvite.h>
#include <xsapi-c/services_c.h> //for XblContextHandle in getXblContext etc
#include "BlazeSDK/component/framework/tdf/userdefines.h" //for ExternalXblAccountId in sendInvite

namespace Blaze { namespace GameManager { class Game; } }

namespace Ignition
{
    class GDKInvitesSample
    {
    public:
        static HRESULT init() { return gSampleInvites.initHandler(); }
        static void finalize();

    public:
        GDKInvitesSample();
        ~GDKInvitesSample();

        HRESULT initHandler();
        void cleanup();

    // helpers
        static void sendInvite(const Blaze::GameManager::Game& game, Blaze::ExternalXblAccountId toXuid);

    private:
        static XGameInviteEventCallback onShellUxEventCb;
        static void onShellUxEventMpsd(const eastl::string& eventUrlStr);
        static void sendInviteMpsd(const Blaze::GameManager::Game& game, Blaze::ExternalXblAccountId toXuid);
        static eastl::string& parseParamValueFromUrl(eastl::string& parsedValue, const eastl::string paramNameInUrl, const eastl::string& eventUrlStr, size_t valueMaxLen = eastl::string::npos);
        static bool getXblContext(XblContextHandle& xblContextHandle);
    private:
        static Ignition::GDKInvitesSample gSampleInvites;
        XTaskQueueHandle mTaskQueue;
        XTaskQueueRegistrationToken mGameInviteEventToken;
    };
   
} // ns Ignition

#endif 
#endif //GDK_INVITES_SAMPLE_H
