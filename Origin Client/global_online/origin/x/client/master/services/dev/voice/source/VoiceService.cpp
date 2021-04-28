// VoiceService.cpp
// Copyright (C) 2014 Electronic Arts. All rights reserved.

#include "services/voice/VoiceService.h"
#include "services/platform/PlatformService.h"
#include "services/debug/DebugService.h"

namespace Origin
{
    namespace Services
    {
        void VoiceService::init()
        {
        }

        void VoiceService::release()
        {
        }

		bool VoiceService::isVoiceEnabled() {

#if ENABLE_VOICE_CHAT

			static QString voiceEnabled;

			// use cached value, if set
			if (voiceEnabled.isEmpty()) {
				// eacore.ini VoiceEnable takes precedence over server dynamic url
				voiceEnabled = Origin::Services::readSetting(Origin::Services::SETTING_VoiceEnable, Origin::Services::Session::SessionService::currentSession());
				if (voiceEnabled.isEmpty()) {
					voiceEnabled = OriginConfigService::instance()->miscConfig().voipEnable;
				}
				// fall back to "beta" if we don't have dynamic url setting
				if (voiceEnabled.isEmpty()) {
					voiceEnabled = "beta";
				}
			}

			// "all" => always enabled
			if (voiceEnabled.compare("all", Qt::CaseInsensitive) == 0)
				return true;

			// "beta" => only enabled if user is beta opt-inned
			if (voiceEnabled.compare("beta", Qt::CaseInsensitive) == 0) {
				return Origin::Services::readSetting(Origin::Services::SETTING_BETAOPTIN, Origin::Services::Session::SessionService::currentSession());
			}
#endif // ENABLE_VOICE_CHAT

			// always disabled
			return false;
		}
	}
}