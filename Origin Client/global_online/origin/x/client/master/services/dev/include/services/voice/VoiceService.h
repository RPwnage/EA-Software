//  VoiceService.h
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#ifndef VOICESERVICE_H
#define VOICESERVICE_H

#include <QObject>
#include <QTcpSocket>

#include <services/config/OriginConfigService.h>
#include "services/session/SessionService.h"
#include <services/settings/SettingsManager.h>

namespace Origin
{
    namespace Services
    {
        class VoiceService : public QObject
        {
            Q_OBJECT

        public:

            /// \brief Initializes the voice service.
            static void init();

            /// \brief Releases the voice service.
            static void release();

			///
			/// \brief Returns true if voice chat is enabled (controlled through dynamic URL and eacore.ini).
			///
			static bool isVoiceEnabled();

        private:
            VoiceService();
        };
    }
}

#endif // VOICESERVICE_H