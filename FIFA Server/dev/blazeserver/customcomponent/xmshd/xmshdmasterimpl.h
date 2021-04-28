/*************************************************************************************************/
/*!
    \file   xmshdmasterimpl.h

    $Header: //gosdev/games/FIFA/2014/Gen3/DEV/blazeserver/13.x/customcomponent/xmshd/xmshdmasterimpl.h#1 $

    \attention
        (c) Electronic Arts 2013. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_XMSHD_MASTERIMPL_H
#define BLAZE_XMSHD_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "xmshd/rpc/xmshdmaster_stub.h"
#include "xmshd/tdf/xmshdtypes_server.h"
#include "xmshd/mastermetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
	namespace XmsHd
	{

		class XmsHdMasterImpl : public XmsHdMasterStub
		{
			public:
				XmsHdMasterImpl();
				~XmsHdMasterImpl();

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

	} // XmsHd
} // Blaze

#endif  // BLAZE_XMSHD_MASTERIMPL_H
