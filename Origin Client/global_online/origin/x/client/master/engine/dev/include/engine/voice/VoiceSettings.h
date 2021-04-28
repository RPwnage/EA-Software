// Copyright (C) 2014 Electronic Arts. All rights reserved.

#ifndef VOICE_SETTINGS_H
#define VOICE_SETTINGS_H

#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Engine
    {
        namespace Voice
        {
            struct VoiceSettings
            {
#if ENABLE_VOICE_CHAT
                VoiceSettings()
                {
                    // Grab some initial setting values
                    pushToTalk = Services::readSetting(Services::SETTING_EnablePushToTalk).toQVariant().toBool();
                    autoGain = Services::readSetting(Services::SETTING_AutoGain).toQVariant().toBool();
                    pushToTalkString = Services::readSetting(Services::SETTING_PushToTalkKeyString);
                    speakerGain = Services::readSetting(Services::SETTING_SpeakerGain);
                    microphoneGain = Services::readSetting(Services::SETTING_MicrophoneGain);
                    voiceActivationThreshold = Services::readSetting(Services::SETTING_VoiceActivationThreshold);
                }

                bool pushToTalk;
                bool autoGain;
                QString pushToTalkString;
                int microphoneGain;
                int speakerGain;
                int voiceActivationThreshold;
#endif // ENABLE_VOICE_CHAT
            };
        }
    }
}

#endif //VOICE_SETTINGS_H