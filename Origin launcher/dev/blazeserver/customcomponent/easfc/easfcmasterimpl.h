/*************************************************************************************************/
/*!
    \file   easfcmasterimpl.h

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/easfc/easfcmasterimpl.h#1 $

    \attention
        (c) Electronic Arts 2012. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EASFC_MASTERIMPL_H
#define BLAZE_EASFC_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "easfc/rpc/easfcmaster_stub.h"
#include "easfc/tdf/easfctypes_server.h"
#include "easfc/mastermetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
	namespace EASFC
	{

		class EasfcMasterImpl : public EasfcMasterStub
		{
			public:
				EasfcMasterImpl();
				~EasfcMasterImpl();

			private:
				//! Called periodically to collect status and statistics about this component that can be used to determine what future improvements should be made.
				virtual void getStatusInfo(ComponentStatus& status) const;

				//! Called when the component should grab configuration for the very first time, during startup.
				virtual bool onConfigure();

				//! Called when the component should update to the latest configuration, for runtime patching.
				virtual bool onReconfigure();

				//! Health monitoring statistics.
				MasterMetrics       mMetrics;
		};

	} // EASFC
} // Blaze

#endif  // BLAZE_EASFC_MASTERIMPL_H
