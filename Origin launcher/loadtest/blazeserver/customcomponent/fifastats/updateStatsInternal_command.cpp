/*************************************************************************************************/
/*!
	\file   updateStats_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class UpdateStatsCommand

	Update Stats

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifastats/rpc/fifastatsslave/updatestatsinternal_stub.h"

// fifastats includes
#include "fifastats/tdf/fifastatstypes.h"
#include "fifastatsslaveimpl.h"
#include "fifastatsconnection.h"
#include "fifastatsoperations.h"

namespace Blaze
{
	namespace FifaStats
	{
		class UpdateStatsInternalCommand : public UpdateStatsInternalCommandStub
		{
		public:
			UpdateStatsInternalCommand(Message* message, UpdateStatsRequest* request, FifaStatsSlaveImpl* componentImpl)
				: UpdateStatsInternalCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~UpdateStatsInternalCommand() { }

			/* Private methods *******************************************************************************/
		private:

			UpdateStatsInternalCommandStub::Errors execute()
			{
				TRACE_LOG("[UpdateStatsInternalCommand:" << this << "].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				const ConfigMap* config = mComponent->getComponentConfig();
				FifaStatsConnection connection(config);

				error = UpdateStatsOperation(connection, mRequest, mResponse);
							
				if (mResponse.getHttpStatusCode() == HTTP_STATUS_OK)
				{
					mResponse.setSuccess(true);
				}
				else				
				{					
					mResponse.setSuccess(false);
				}

				return UpdateStatsInternalCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			FifaStatsSlaveImpl* mComponent;
		};

		// static factory method impl
		UpdateStatsInternalCommandStub* UpdateStatsInternalCommandStub::create(Message* message, UpdateStatsRequest* request, FifaStatsSlave* componentImpl)
		{
			return BLAZE_NEW UpdateStatsInternalCommand(message, request, static_cast<FifaStatsSlaveImpl*>(componentImpl));
		}

	} // FifaStats
} // Blaze
