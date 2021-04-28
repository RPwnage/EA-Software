/*************************************************************************************************/
/*!
	\file   createInstance_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class CreateInstanceCommand

	Create Instance

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifagroups/rpc/fifagroupsslave/createinstance_stub.h"

// fifagroups includes
#include "fifagroups/tdf/fifagroupstypes.h"
#include "fifagroupsslaveimpl.h"
#include "fifagroupsconnection.h"

namespace Blaze
{
	namespace FifaGroups
	{
		class CreateInstanceCommand : public CreateInstanceCommandStub
		{
		public:
			CreateInstanceCommand(Message* message, CreateInstanceRequest* request, FifaGroupsSlaveImpl* componentImpl)
				: CreateInstanceCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~CreateInstanceCommand() { }

			/* Private methods *******************************************************************************/
		private:

			CreateInstanceCommandStub::Errors execute()
			{
				TRACE_LOG("[CreateInstanceCommand:" << this << "].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;
				
				const ConfigMap* config = mComponent->getComponentConfig();
				FifaGroupsConnection connection(config);
				error = CreateInstanceOperation(connection, mRequest, mResponse);

				if (mResponse.getHttpStatusCode() == HTTP_STATUS_OK)
				{
					mResponse.setSuccess(true);
				}
				else
				{
					mResponse.setSuccess(false);
				}
				return CreateInstanceCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			FifaGroupsSlaveImpl* mComponent;
		};

		// static factory method impl
		CreateInstanceCommandStub* CreateInstanceCommandStub::create(Message* message, CreateInstanceRequest* request, FifaGroupsSlave* componentImpl)
		{
			return BLAZE_NEW CreateInstanceCommand(message, request, static_cast<FifaGroupsSlaveImpl*>(componentImpl));
		}

	} // FifaGroups
} // Blaze
