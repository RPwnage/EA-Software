/*************************************************************************************************/
/*!
	\file   deleteInstance_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class DeleteInstanceCommand

	Delete Instance

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifagroups/rpc/fifagroupsslave/deleteinstance_stub.h"

// fifagroups includes
#include "fifagroups/tdf/fifagroupstypes.h"
#include "fifagroupsslaveimpl.h"
#include "fifagroupsconnection.h"

namespace Blaze
{
	namespace FifaGroups
	{
		class DeleteInstanceCommand : public DeleteInstanceCommandStub
		{
		public:
			DeleteInstanceCommand(Message* message, DeleteInstanceRequest* request, FifaGroupsSlaveImpl* componentImpl)
				: DeleteInstanceCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~DeleteInstanceCommand() { }

			/* Private methods *******************************************************************************/
		private:

			DeleteInstanceCommandStub::Errors execute()
			{
				TRACE_LOG("[DeleteInstanceCommand:" << this << "].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;
				
				const ConfigMap* config = mComponent->getComponentConfig();
				FifaGroupsConnection connection(config);
				error = DeleteInstanceOperation(connection, mRequest, mResponse);

				if (mResponse.getHttpStatusCode() == HTTP_STATUS_OK)
				{
					mResponse.setSuccess(true);
				}
				else
				{
					mResponse.setSuccess(false);
				}
				return DeleteInstanceCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			FifaGroupsSlaveImpl* mComponent;
		};

		// static factory method impl
		DeleteInstanceCommandStub* DeleteInstanceCommandStub::create(Message* message, DeleteInstanceRequest* request, FifaGroupsSlave* componentImpl)
		{
			return BLAZE_NEW DeleteInstanceCommand(message, request, static_cast<FifaGroupsSlaveImpl*>(componentImpl));
		}

	} // FifaGroups
} // Blaze
