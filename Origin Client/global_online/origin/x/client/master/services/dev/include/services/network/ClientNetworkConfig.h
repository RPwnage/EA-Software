///////////////////////////////////////////////////////////////////////////////
//  ClientNetworkConfig.h
//
//  Copyright (c) 2011, Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef CLIENTNETWORKCONFIG_H
#define CLIENTNETWORKCONFIG_H

#include <QMap>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API ClientNetworkConfig
        {
        public:
            ClientNetworkConfig()
            {

            }

        public:
            enum PropertyEnum
            {
                HeartbeatMessageServer,
                HeartbeatMessageInterval,
                ReleaseNotesURL
            };

            QMap<int, QString> mProperties;
        };
    }
}

#endif