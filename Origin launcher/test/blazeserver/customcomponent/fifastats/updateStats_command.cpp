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
#include "fifastats/rpc/fifastatsslave/updatestats_stub.h"

// fifastats includes
#include "fifastats/tdf/fifastatstypes.h"
#include "fifastatsslaveimpl.h"
#include "fifastatsconnection.h"

namespace Blaze
{
	namespace FifaStats
	{
		class UpdateStatsCommand : public UpdateStatsCommandStub
		{
		public:
			UpdateStatsCommand(Message* message, UpdateStatsRequest* request, FifaStatsSlaveImpl* componentImpl)
				: UpdateStatsCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~UpdateStatsCommand() { }

			/* Private methods *******************************************************************************/
		private:

			UpdateStatsCommandStub::Errors execute()
			{
				TRACE_LOG("[UpdateStatsCommand:" << this << "].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				const ConfigMap* config = mComponent->getComponentConfig();
				FifaStatsConnection connection(config);

				mRequest.setServiceName(gCurrentUserSession->getServiceName());
				error = UpdateStatsOperation(connection, mRequest, mResponse);
							
				if (mResponse.getHttpStatusCode() == HTTP_STATUS_OK)
				{
					mResponse.setSuccess(true);
				}
				else				
				{					
					mResponse.setSuccess(false);
				}

				return UpdateStatsCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			FifaStatsSlaveImpl* mComponent;
		};

		// static factory method impl
		UpdateStatsCommandStub* UpdateStatsCommandStub::create(Message* message, UpdateStatsRequest* request, FifaStatsSlave* componentImpl)
		{
			return BLAZE_NEW UpdateStatsCommand(message, request, static_cast<FifaStatsSlaveImpl*>(componentImpl));
		}

	} // FifaStats
} // Blaze
