//  Services.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef SERVICES_H
#define SERVICES_H

#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        /// \fn init
        /// \brief Prepares the Services layer for use.
        bool init(int argc, char* argv[]);

        /// \fn release
        /// \brief Frees the Services.
        void release();

        /// \fn environment
        /// \brief Temporary API to return the current environment.
        ORIGIN_PLUGIN_API QString const& environment();
    }
}

#endif // SERVICES_H
