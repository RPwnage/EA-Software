/*************************************************************************************************/
/*!
    \file   eaaccessmasterimpl.h

    $Header: //gosdev/games/FIFA/2015/Gen4/DEV/blazeserver/13.x/customcomponent/eaaccess/eaaccessmasterimpl.h#1 $

    \attention
        (c) Electronic Arts 2013. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EAACCESS_MASTERIMPL_H
#define BLAZE_EAACCESS_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "eaaccess/rpc/eaaccessmaster_stub.h"
#include "eaaccess/tdf/eaaccesstypes_server.h"
#include "eaaccess/mastermetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
	namespace EaAccess
	{

		class EaAccessMasterImpl : public EaAccessMasterStub
		{
			public:
				EaAccessMasterImpl();
				~EaAccessMasterImpl();

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

	} // EaAccess
} // Blaze

#endif  // BLAZE_EAACCESS_MASTERIMPL_H
