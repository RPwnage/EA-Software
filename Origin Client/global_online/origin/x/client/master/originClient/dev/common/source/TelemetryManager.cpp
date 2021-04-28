///////////////////////////////////////////////////////////////////////////////|
//
// Copyright (C) 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////|

#include "TelemetryManager.h"
#include "TelemetryAPIDLL.h"

#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "engine/login/LoginController.h"



namespace Origin
{
    namespace Client
    {
        void TelemetryManager::setupWatchers()
        {
            ORIGIN_VERIFY_CONNECT(Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
                this, SLOT(settingChanged(const QString&, const Origin::Services::Variant&)));

            if (Services::SettingsManager::instance()->areServerUserSettingsLoaded())
            {
                // We already have our settings
                updateTelemetryOptOut();
            }

            ORIGIN_VERIFY_CONNECT(Origin::Engine::LoginController::instance(), SIGNAL(logoutComplete(Origin::Engine::LoginController::Error)), this, SLOT(onLogoutCompleted()));
        }


        void TelemetryManager::settingChanged(const QString& settingName, const Origin::Services::Variant& value)
        {
            if (settingName == Services::SETTING_ServerSideEnableTelemetry.name())
            {
                updateTelemetryOptOut();
            }
        }

        void TelemetryManager::updateTelemetryOptOut()
        {
            bool isSendNonCriticalTelemetry = Services::readSetting(Services::SETTING_ServerSideEnableTelemetry); 

            // If true, the telemetry API will filter out non-critical telemetry prior to sending.
            GetTelemetryInterface()->setUserSettingSendNonCriticalTelemetry(isSendNonCriticalTelemetry);
        }

        void TelemetryManager::onLogoutCompleted()
        {
            GetTelemetryInterface()->logout();
        }

    }
}
