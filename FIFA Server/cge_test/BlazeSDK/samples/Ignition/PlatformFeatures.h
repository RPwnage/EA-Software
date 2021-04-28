
#ifndef PLATFORM_FEATURES_H
#define PLATFORM_FEATURES_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/usermanager/usermanager.h"

namespace Ignition
{

class PlatformFeatures :
    public LocalUserUiBuilder,
    protected Blaze::Idler
{
    public:
        

        PlatformFeatures(uint32_t userIndex);
        virtual ~PlatformFeatures();

        virtual void onAuthenticated();
        virtual void onDeAuthenticated();
        void idle(const uint32_t currentTime, const uint32_t elapsedTime);

    private:
        const char8_t* getInitialPlayerAttribute(const char8_t* strGameMode, Blaze::GameManager::GameType gameType);

#if defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
        PYRO_ACTION(PlatformFeatures, AcceptInvite);
        PYRO_ACTION(PlatformFeatures, ShowReceiveDialog);
        bool mFetchInvites;
        bool mFetchMetadata;
        bool mJoinGame;
#endif
};

}

#endif //PLATFORM_FEATURES_H
