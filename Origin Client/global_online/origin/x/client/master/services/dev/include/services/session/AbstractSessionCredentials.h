//  SessionCredentials.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef SESSIONCREDENTIALS_H
#define SESSIONCREDENTIALS_H

#include "AbstractSession.h"
#include <QtCore>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        namespace Session
        {
            /// \brief Defines the interface for credentials. 
            class ORIGIN_PLUGIN_API AbstractSessionCredentials
            {
            public:

                virtual QString const& user() const = 0;
                virtual QString const& password() const = 0;

            protected:

                // we need to declare this so that we get a virtual destructor
                virtual ~AbstractSessionCredentials();
            };

            typedef QSharedPointer<AbstractSessionCredentials> SessionCredentialsRef;

            //////////////////////////////////////////////////////////////////////////
            //
            // inline implementations

            inline AbstractSessionCredentials::~AbstractSessionCredentials()
            {
            }

        } // namespace Session
    } // namespace Services
} // namespace Origin

#endif // SESSIONCREDENTIALS_H
