///////////////////////////////////////////////////////////////////////////////
// AbstractFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ABSTRACTFLOW_H
#define ABSTRACTFLOW_H

#include <qobject.h>
#include "FlowResult.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        /// \brief Provides the visual presentation to the user, and creates the
        /// various user interaction flows.
        class ORIGIN_PLUGIN_API AbstractFlow : public QObject
        {
            Q_OBJECT

        public:

            /// \brief Initiates the flow execution.
            virtual void start() = 0;

			/// \brief Base classes must have their destructors as virtual.
			virtual ~AbstractFlow() {}
        };
    }
}

#endif // ABSTRACTFLOW_H
