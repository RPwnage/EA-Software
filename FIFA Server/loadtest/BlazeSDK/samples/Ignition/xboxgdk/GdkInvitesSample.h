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

namespace Ignition
{
    class GDKInvitesSample
    {
    public:
        static HRESULT init() { return gSampleInvites.initHandler(); }
        static void finalize();

    public:
        GDKInvitesSample();
        ~GDKInvitesSample() { cleanup(); }

        HRESULT initHandler();
        void cleanup();

    // helpers
        static bool getXblContext(XblContextHandle& xblContextHandle);

    private:
        static XGameInviteEventCallback onInviteCb;
    private:
        static Ignition::GDKInvitesSample gSampleInvites;
        XTaskQueueHandle mTaskQueue;
        XTaskQueueRegistrationToken mGameInviteEventToken;
    };
   
} // ns Ignition

#endif 
#endif //GDK_INVITES_SAMPLE_H
