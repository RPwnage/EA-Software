
#ifndef VOIP_CONFIG_H
#define VOIP_CONFIG_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"
#include "DirtySDK/voip/voipdef.h"
#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipgroup.h"

namespace Ignition
{

class VoipConfig
    : public BlazeHubUiBuilder,
      public BlazeHubUiDriver::PrimaryLocalUserAuthenticationListener
{
    public:
        VoipConfig();
        virtual ~VoipConfig();

        virtual void onAuthenticatedPrimaryUser();
        virtual void onDeAuthenticatedPrimaryUser();

    private:
        static void onIdle(void *pData, uint32_t uTick);

        static const int VOIPCONFIG_MAX_SLOT = 4;

        uint8_t mShowVoipConfig;
        char mChannelSlotString[32];
        char mChannelCfgString[32];
        char mChannelIdString[8];
        VoipChanModeE mChannelMode[VOIP_MAXLOCALUSERS_EXTENDED][VOIPCONFIG_MAX_SLOT];
        int32_t mChannelId[VOIP_MAXLOCALUSERS_EXTENDED][VOIPCONFIG_MAX_SLOT];
        bool mLoopback[VOIP_MAXLOCALUSERS];
        bool mSpeechToTextConversion[VOIP_MAXLOCALUSERS];
        VoipGroupRefT *mVoipGroup;

        PYRO_ACTION(VoipConfig, SelectChannel);
        PYRO_ACTION(VoipConfig, ResetChannels);

#if defined(EA_PLATFORM_PS4)
        PYRO_ACTION(VoipConfig, AddMostRestrictiveVoipContributor);
        PYRO_ACTION(VoipConfig, DelMostRestrictiveVoipContributor);
#endif 

        //party chat management for xbone
        PYRO_ACTION(VoipConfig, SuspendPartyChat)
        PYRO_ACTION(VoipConfig, ResumePartyChat)
        PYRO_ACTION(VoipConfig, TextChatNarrate)
        PYRO_ACTION(VoipConfig, ToggleLoopback)
        PYRO_ACTION(VoipConfig, ConvertTextToSpeech)
        PYRO_ACTION(VoipConfig, ConvertSpeechToText)

#if defined(EA_PLATFORM_WINDOWS)
        PYRO_ACTION(VoipConfig, SelectCodec)
#endif

};

}

#endif
