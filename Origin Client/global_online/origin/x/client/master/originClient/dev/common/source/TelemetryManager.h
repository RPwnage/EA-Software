#ifndef _TELEMETRYMANAGER_H
#define _TELEMETRYMANAGER_H

///////////////////////////////////////////////////////////////////////////////|
//
// Copyright (C) 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////|

#include <QObject> 
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    //Forward declare
    namespace Services
    {
        class Variant;
    }

    namespace Client
    {

        /// \brief Manages our telemetry API
        class ORIGIN_PLUGIN_API TelemetryManager : public QObject
        {
            Q_OBJECT
        public:
            TelemetryManager(){}
            ~TelemetryManager(){}

            // \brief  Start watching our settings for changes
            void setupWatchers();

        private:
            void updateTelemetryOptOut();

        private slots:
            /// \brief triggered when logout was been achieved.
            void onLogoutCompleted();

            /// Setting Changed handler.
            void settingChanged(const QString& settingName, const Origin::Services::Variant& value);

        };

    }
}

#endif
