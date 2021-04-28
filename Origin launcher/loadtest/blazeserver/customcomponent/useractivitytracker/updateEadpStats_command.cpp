/*************************************************************************************************/
/*!
	\file   updateEadpStats_command.cpp.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class SubmitUserActivityCommand

	Submit User activity data for tracking

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "useractivitytracker/rpc/useractivitytrackerslave/updateeadpstats_stub.h"

// useractivitytracker includes
#include "useractivitytracker/tdf/useractivitytrackertypes.h"
#include "useractivitytracker/tdf/useractivitytrackertypes_server.h"
#include "useractivitytrackerslaveimpl.h"

// fifastats includes
#include "fifastats/tdf/fifastatstypes.h"


namespace Blaze
{
	namespace UserActivityTracker
	{
		class UpdateEadpStatsCommand : public UpdateEadpStatsCommandStub
		{
		public:
			UpdateEadpStatsCommand(Message* message, FifaStats::UpdateStatsRequest* request, UserActivityTrackerSlaveImpl* componentImpl)
				: UpdateEadpStatsCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~UpdateEadpStatsCommand() { }

			/* Private methods *******************************************************************************/
		private:

			UpdateEadpStatsCommand::Errors execute()
			{
				TRACE_LOG("[UpdateEadpStatsCommand:" << this << "].execute()");
				
				Blaze::BlazeRpcError error = Blaze::ERR_OK;
				error = mComponent->updateEadpStats(mRequest, mResponse);

				mResponse.setSuccess(mResponse.getHttpStatusCode() == HTTP_STATUS_OK);
				return UpdateEadpStatsCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			UserActivityTrackerSlaveImpl* mComponent;
		};

		// static factory method impl
		UpdateEadpStatsCommandStub* UpdateEadpStatsCommandStub::create(Message* message, FifaStats::UpdateStatsRequest* request, UserActivityTrackerSlave* componentImpl)
		{
			return BLAZE_NEW UpdateEadpStatsCommand(message, request, static_cast<UserActivityTrackerSlaveImpl*>(componentImpl));
		}
	} // UserActivityTracker
} // Blaze
