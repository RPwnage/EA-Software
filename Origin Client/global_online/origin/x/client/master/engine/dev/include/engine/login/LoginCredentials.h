//  LoginCredentials.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef LOGINCREDENTIALS_H
#define LOGINCREDENTIALS_H

#include <QtCore>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Engine
    {
        class ORIGIN_PLUGIN_API LoginCredentials
        {
        public:

            /// \brief The LoginCredentials constructor.
            /// \param username The username.
            /// \param password The password.
            LoginCredentials(QString const& username, QString const& password);

            /// \brief Returns the username used with a login session.
            /// \return The username.
            QString const& username() const;

            /// \brief Returns the password used with a login session.
            /// \return The password.
            QString const& password() const;
        };
    }
}

#endif // LOGINCREDENTIALS_H
