/*************************************************************************************************/
/*!
	\file   submitUserActivity_command.cpp.cpp


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

#include "EATDF/time.h"

#include "framework/blaze.h"
#include "useractivitytracker/rpc/useractivitytrackerslave/submituseractivity_stub.h"

// useractivitytracker includes
#include "useractivitytracker/tdf/useractivitytrackertypes.h"
#include "useractivitytracker/tdf/useractivitytrackertypes_server.h"
#include "useractivitytrackerslaveimpl.h"


namespace Blaze
{
	namespace UserActivityTracker
	{
		class SubmitUserActivityCommand : public SubmitUserActivityCommandStub
		{
		public:
			SubmitUserActivityCommand(Message* message, SubmitUserActivityRequest* request, UserActivityTrackerSlaveImpl* componentImpl)
				: SubmitUserActivityCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~SubmitUserActivityCommand() { }

			/* Private methods *******************************************************************************/
		private:

			SubmitUserActivityCommand::Errors execute()
			{
				TRACE_LOG("[SubmitUserActivityCommand:" << this << "].execute()");
				
				Blaze::BlazeRpcError error = Blaze::ERR_OK;
				error = mComponent->processUserActivity(ACTIVITY_MODE_TYPE_OFFLINE, gCurrentUserSession->getBlazeId(), mRequest);

				mResponse.setSuccess(error == Blaze::ERR_OK);
				return SubmitUserActivityCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			UserActivityTrackerSlaveImpl* mComponent;
		};

		// static factory method impl
		SubmitUserActivityCommandStub* SubmitUserActivityCommandStub::create(Message* message, SubmitUserActivityRequest* request, UserActivityTrackerSlave* componentImpl)
		{
			return BLAZE_NEW SubmitUserActivityCommand(message, request, static_cast<UserActivityTrackerSlaveImpl*>(componentImpl));
		}
	} // FifaGroups
} // Blaze
