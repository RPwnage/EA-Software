///////////////////////////////////////////////////////////////////////////////
// MOTDFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef MOTDFLOW_H
#define MOTDFLOW_H

#include "flows/AbstractFlow.h"
#include "widgets/motd/source/MOTDViewController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Client
	{
		class MOTDViewController;

        /// \brief Handles all high-level actions related to the message of the day.
		class ORIGIN_PLUGIN_API MOTDFlow : public AbstractFlow
		{
			Q_OBJECT

		public:
            /// \brief The MOTDFlow constructor.
			MOTDFlow();

            /// \brief Public interface for starting the MOTDflow.
			virtual void start();

            void raise();

		private:

			QScopedPointer<MOTDViewController>		mMOTDViewController; ///< Pointer to the MOTD view controller.
		};
	}
}

#endif // MOTDFLOW_H
