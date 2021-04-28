/*************************************************************************************************/
/*!
	\file   setMultipleInstanceAttributes_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class SetMultipleInstanceAttributesCommand

	Set Multiple Instance Attributes

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifagroups/rpc/fifagroupsslave/setmultipleinstanceattributes_stub.h"

// fifagroups includes
#include "fifagroups/tdf/fifagroupstypes.h"
#include "fifagroupsslaveimpl.h"
#include "fifagroupsconnection.h"

namespace Blaze
{
	namespace FifaGroups
	{
		class SetMultipleInstanceAttributesCommand : public SetMultipleInstanceAttributesCommandStub
		{
		public:
			SetMultipleInstanceAttributesCommand(Message* message, SetMultipleInstanceAttributesRequest* request, FifaGroupsSlaveImpl* componentImpl)
				: SetMultipleInstanceAttributesCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~SetMultipleInstanceAttributesCommand() { }

			/* Private methods *******************************************************************************/
		private:

			SetMultipleInstanceAttributesCommandStub::Errors execute()
			{
				TRACE_LOG("[SetMultipleInstanceAttributesCommand:" << this << "].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				const ConfigMap* config = mComponent->getComponentConfig();
				FifaGroupsConnection connection(config);
				error = SetMultipleInstanceAttributesOperation(connection, mRequest, mResponse);

				if (mResponse.getHttpStatusCode() == HTTP_STATUS_OK)
				{
					mResponse.setSuccess(true);
				}
				else
				{
					mResponse.setSuccess(false);
				}
				return SetMultipleInstanceAttributesCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			FifaGroupsSlaveImpl* mComponent;
		};

		// static factory method impl
		SetMultipleInstanceAttributesCommandStub* SetMultipleInstanceAttributesCommandStub::create(Message* message, SetMultipleInstanceAttributesRequest* request, FifaGroupsSlave* componentImpl)
		{
			return BLAZE_NEW SetMultipleInstanceAttributesCommand(message, request, static_cast<FifaGroupsSlaveImpl*>(componentImpl));
		}

	} // FifaGroups
} // Blaze
