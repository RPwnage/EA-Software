/*************************************************************************************************/
/*!
	\file   incrementMatchPlayed_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class IncrementMatchPlayedCommand

	Submit User activity data for tracking

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "useractivitytracker/rpc/useractivitytrackerslave/incrementmatchplayed_stub.h"

// useractivitytracker includes
#include "useractivitytracker/tdf/useractivitytrackertypes.h"
#include "useractivitytracker/tdf/useractivitytrackertypes_server.h"
#include "useractivitytrackerslaveimpl.h"


namespace Blaze
{
	namespace UserActivityTracker
	{
		class IncrementMatchPlayedCommand : public IncrementMatchPlayedCommandStub
		{
		public:
			IncrementMatchPlayedCommand(Message* message, UserActivityTrackerSlaveImpl* componentImpl)
				: IncrementMatchPlayedCommandStub(message), mComponent(componentImpl)
			{ }

			virtual ~IncrementMatchPlayedCommand() { }

			/* Private methods *******************************************************************************/
		private:

			IncrementMatchPlayedCommand::Errors execute()
			{
				TRACE_LOG("[IncrementMatchPlayedCommand:" << this << "].execute()");
				
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				error = mComponent->incrementMatchPlayed(ACTIVITY_MODE_TYPE_ONLINE, gCurrentUserSession->getBlazeId());

				bool success = error == Blaze::ERR_OK;
				mResponse.setSuccess(success);

				if (!success)
				{
					mResponse.setErrorCode(static_cast<int64_t>(error));
					mResponse.setErrorMessage("unable to increment match played");
				}

				return IncrementMatchPlayedCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			UserActivityTrackerSlaveImpl* mComponent;
		};

		// static factory method impl
		IncrementMatchPlayedCommandStub* IncrementMatchPlayedCommandStub::create(Message* message, UserActivityTrackerSlave* componentImpl)
		{
			return BLAZE_NEW IncrementMatchPlayedCommand(message, static_cast<UserActivityTrackerSlaveImpl*>(componentImpl));
		}
	} // FifaGroups
} // Blaze
