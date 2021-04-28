/*************************************************************************************************/
/*!
	\file   mergeGroupInstances_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class MergeGroupInstancesCommand

	Merge Group Instances

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifagroups/rpc/fifagroupsslave/mergegroupinstances_stub.h"

// fifagroups includes
#include "fifagroups/tdf/fifagroupstypes.h"
#include "fifagroupsslaveimpl.h"
#include "fifagroupsconnection.h"

namespace Blaze
{
	namespace FifaGroups
	{
		class MergeGroupInstancesCommand : public MergeGroupInstancesCommandStub
		{
		public:
			MergeGroupInstancesCommand(Message* message, MergeGroupInstancesRequest* request, FifaGroupsSlaveImpl* componentImpl)
				: MergeGroupInstancesCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~MergeGroupInstancesCommand() { }

			/* Private methods *******************************************************************************/
		private:

			MergeGroupInstancesCommand::Errors execute()
			{
				TRACE_LOG("[MergeGroupInstancesCommand:" << this << "].execute()");
				
				Blaze::BlazeRpcError error = Blaze::ERR_OK;
				error = mComponent->processMergeGroupInstances(mRequest);

				mResponse.setSuccess(error == Blaze::ERR_OK);
				mResponse.setHttpStatusCode(HTTP_STATUS_OK);
				return MergeGroupInstancesCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			FifaGroupsSlaveImpl* mComponent;
		};

		// static factory method impl
		MergeGroupInstancesCommandStub* MergeGroupInstancesCommandStub::create(Message* message, MergeGroupInstancesRequest* request, FifaGroupsSlave* componentImpl)
		{
			return BLAZE_NEW MergeGroupInstancesCommand(message, request, static_cast<FifaGroupsSlaveImpl*>(componentImpl));
		}
	} // FifaGroups
} // Blaze
