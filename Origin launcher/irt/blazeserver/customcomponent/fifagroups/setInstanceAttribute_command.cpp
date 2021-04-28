/*************************************************************************************************/
/*!
	\file   setInstanceAttribute_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class SetInstanceAttributeCommand

	Set Instance Attribute

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifagroups/rpc/fifagroupsslave/setinstanceattribute_stub.h"

// fifagroups includes
#include "fifagroups/tdf/fifagroupstypes.h"
#include "fifagroupsslaveimpl.h"
#include "fifagroupsconnection.h"

namespace Blaze
{
	namespace FifaGroups
	{
		class SetInstanceAttributeCommand : public SetInstanceAttributeCommandStub
		{
		public:
			SetInstanceAttributeCommand(Message* message, SetInstanceAttributeRequest* request, FifaGroupsSlaveImpl* componentImpl)
				: SetInstanceAttributeCommandStub(message, request), mComponent(componentImpl)
			{}

			virtual ~SetInstanceAttributeCommand() 
			{}

			/* Private methods *******************************************************************************/
		private:

			SetInstanceAttributeCommandStub::Errors execute()
			{
				TRACE_LOG("[SetInstanceAttributeCommand:" << this << "].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				const ConfigMap* config = mComponent->getComponentConfig();
				FifaGroupsConnection connection(config);
				error = SetInstanceAttributeOperation(connection, mRequest, mResponse);

				if (mResponse.getHttpStatusCode() == HTTP_STATUS_OK)
				{
					mResponse.setSuccess(true);
				}
				else
				{
					mResponse.setSuccess(false);
				}
				return SetInstanceAttributeCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			FifaGroupsSlaveImpl* mComponent;
		};

		// static factory method impl
		SetInstanceAttributeCommandStub* SetInstanceAttributeCommandStub::create(Message* message, SetInstanceAttributeRequest* request, FifaGroupsSlave* componentImpl)
		{
			return BLAZE_NEW SetInstanceAttributeCommand(message, request, static_cast<FifaGroupsSlaveImpl*>(componentImpl));
		}

	} // FifaGroups
} // Blaze
