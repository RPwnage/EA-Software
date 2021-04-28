/*************************************************************************************************/
/*!
	\file   joinGroup_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class JoinGroupCommand

	Join Group

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifagroups/rpc/fifagroupsslave/joingroup_stub.h"

// fifagroups includes
#include "fifagroups/tdf/fifagroupstypes.h"
#include "fifagroupsslaveimpl.h"
#include "fifagroupsconnection.h"

namespace Blaze
{
	namespace FifaGroups
	{
		class JoinGroupCommand : public JoinGroupCommandStub
		{
		public:
			JoinGroupCommand(Message* message, JoinGroupRequest* request, FifaGroupsSlaveImpl* componentImpl)
				: JoinGroupCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~JoinGroupCommand() { }

			/* Private methods *******************************************************************************/
		private:

			JoinGroupCommandStub::Errors execute()
			{
				TRACE_LOG("[JoinGroupCommand:" << this << "].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				const ConfigMap* config = mComponent->getComponentConfig();
				FifaGroupsConnection connection(config);
				error = JoinGroupOperation(connection, mRequest, mResponse);

				if (mResponse.getHttpStatusCode() == HTTP_STATUS_OK)
				{
					mResponse.setSuccess(true);
				}
				else
				{
					mResponse.setSuccess(false);
				}
				return JoinGroupCommand::commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			FifaGroupsSlaveImpl* mComponent;
		};

		// static factory method impl
		JoinGroupCommandStub* JoinGroupCommandStub::create(Message* message, JoinGroupRequest* request, FifaGroupsSlave* componentImpl)
		{
			return BLAZE_NEW JoinGroupCommand(message, request, static_cast<FifaGroupsSlaveImpl*>(componentImpl));
		}

	} // FifaGroups
} // Blaze
