/*************************************************************************************************/
/*!
\file   perfmasterimpl.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PERF_MASTERIMPL_H
#define BLAZE_PERF_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "perf/rpc/perfmaster_stub.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
	namespace Perf
	{

		class PerfMasterImpl : public PerfMasterStub
		{
		public:
			PerfMasterImpl();
			~PerfMasterImpl() override;

		private:
			bool onConfigure() override;
		};

	} // Perf
} // Blaze

#endif  // BLAZE_PERF_MASTERIMPL_H