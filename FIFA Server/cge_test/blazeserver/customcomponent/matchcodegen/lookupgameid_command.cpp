/*************************************************************************************************/
/*!
	\file   lookupgameid_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class LookupGameIdCommand

	

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "matchcodegen/rpc/matchcodegenslave/lookupgameid_stub.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// matchcodegen includes
#include "matchcodegenslaveimpl.h"
#include "matchcodegen/tdf/matchcodegentypes.h"

namespace Blaze
{
	namespace MatchCodeGen
	{
		class LookupGameIdCommand : public LookupGameIdCommandStub
		{
		public:
			LookupGameIdCommand(Message* message, LookupGameIdRequest* request, MatchCodeGenSlaveImpl* componentImpl)
				: LookupGameIdCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~LookupGameIdCommand() { }

			/* Private methods *******************************************************************************/
		private:

			LookupGameIdCommand::Errors execute()
			{
				TRACE_LOG("[LookupGameIdCommand].execute()");
				
				MatchCodeGenMaster* master = mComponent->getMaster();
				BlazeRpcError errorCode = master->lookupGameIdMaster(mRequest, mResponse);

				return commandErrorFromBlazeError(errorCode);
			}

		private:
			// Not owned memory.
			MatchCodeGenSlaveImpl* mComponent;
		};

		// static factory method impl
		LookupGameIdCommandStub* LookupGameIdCommandStub::create(Message* message, LookupGameIdRequest* request, MatchCodeGenSlave* componentImpl)
		{
			return BLAZE_NEW LookupGameIdCommand(message, request, static_cast<MatchCodeGenSlaveImpl*>(componentImpl));
		}

	} // MatchCodeGen
} // Blaze
