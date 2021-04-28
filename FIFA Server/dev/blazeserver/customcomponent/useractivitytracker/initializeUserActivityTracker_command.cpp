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

#include "useractivitytracker/rpc/useractivitytrackerslave/initializeuseracivitytracker_stub.h"

// useractivitytracker includes
#include "useractivitytracker/tdf/useractivitytrackertypes.h"
#include "useractivitytracker/tdf/useractivitytrackertypes_server.h"
#include "useractivitytrackerslaveimpl.h"


namespace Blaze
{
	namespace UserActivityTracker
	{
		class InitializeUserActivtiyTrackerCommand : public InitializeUserAcivityTrackerCommandStub
		{
		public:
			InitializeUserActivtiyTrackerCommand(Message* message, InitializeUserActivityTrackerRequest* request, UserActivityTrackerSlaveImpl* componentImpl)
				: InitializeUserAcivityTrackerCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~InitializeUserActivtiyTrackerCommand() { }

			/* Private methods *******************************************************************************/
		private:

			InitializeUserActivtiyTrackerCommand::Errors execute()
			{
				TRACE_LOG("[InitializeUserActivtiyTrackerCommand:" << this << "].execute()");
				
				Blaze::BlazeRpcError error = mComponent->initializeUserActivity(mRequest, mResponse);

				mResponse.setSuccess(error == Blaze::ERR_OK);
				return InitializeUserActivtiyTrackerCommand::commandErrorFromBlazeError(0);
			}

		private:
			// Not owned memory.
			UserActivityTrackerSlaveImpl* mComponent;
		};

		// static factory method impl
		InitializeUserAcivityTrackerCommandStub* InitializeUserAcivityTrackerCommandStub::create(Message* message, InitializeUserActivityTrackerRequest* request, UserActivityTrackerSlave* componentImpl)
		{
			return BLAZE_NEW InitializeUserActivtiyTrackerCommand(message, request, static_cast<UserActivityTrackerSlaveImpl*>(componentImpl));
		}
	} // FifaGroups
} // Blaze
