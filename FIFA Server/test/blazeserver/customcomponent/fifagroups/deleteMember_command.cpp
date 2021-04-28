/*************************************************************************************************/
/*!
	\file   deleteMember_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class DeleteMemberCommand

	Delete Member

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifagroups/rpc/fifagroupsslave/deletemember_stub.h"

// fifagroups includes
#include "fifagroups/tdf/fifagroupstypes.h"
#include "fifagroupsslaveimpl.h"
#include "fifagroupsconnection.h"

namespace Blaze
{
	namespace FifaGroups
	{
		class DeleteMemberCommand : public DeleteMemberCommandStub
		{
		public:
			DeleteMemberCommand(Message* message, DeleteMemberRequest* request, FifaGroupsSlaveImpl* componentImpl)
				: DeleteMemberCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~DeleteMemberCommand() { }

			/* Private methods *******************************************************************************/
		private:

			DeleteMemberCommandStub::Errors execute()
			{
				TRACE_LOG("[DeleteMemberCommand:" << this << "].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				const ConfigMap* config = mComponent->getComponentConfig();
				FifaGroupsConnection connection(config);
				error = DeleteMemberOperation(connection, mRequest, mResponse);

				if (mResponse.getHttpStatusCode() == HTTP_STATUS_OK)
				{
					mResponse.setSuccess(true);
				}
				else
				{
					mResponse.setSuccess(false);
				}
				return DeleteMemberCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			FifaGroupsSlaveImpl* mComponent;
		};

		// static factory method impl
		DeleteMemberCommandStub* DeleteMemberCommandStub::create(Message* message, DeleteMemberRequest* request, FifaGroupsSlave* componentImpl)
		{
			return BLAZE_NEW DeleteMemberCommand(message, request, static_cast<FifaGroupsSlaveImpl*>(componentImpl));
		}

	} // FifaGroups
} // Blaze
