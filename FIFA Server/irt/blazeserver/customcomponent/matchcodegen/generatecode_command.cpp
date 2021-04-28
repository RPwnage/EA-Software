/*************************************************************************************************/
/*!
	\file   generatecode_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
	\class GenerateCodeCommand

	Stores a GameId and generates a code for future lookup

	\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "matchcodegen/rpc/matchcodegenslave/generatecode_stub.h"

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
		class GenerateCodeCommand : public GenerateCodeCommandStub
		{
		public:
			GenerateCodeCommand(Message* message, GenerateCodeRequest* request, MatchCodeGenSlaveImpl* componentImpl)
				: GenerateCodeCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~GenerateCodeCommand() { }

			/* Private methods *******************************************************************************/
		private:

			GenerateCodeCommand::Errors execute()
			{
				TRACE_LOG("[GenerateCodeCommand].execute()");

				MatchCodeGenMaster* master = mComponent->getMaster();
				BlazeRpcError errorCode = master->generateCodeMaster(mRequest, mResponse);

				return commandErrorFromBlazeError(errorCode);
			}

		private:
			// Not owned memory.
			MatchCodeGenSlaveImpl* mComponent;
		};

		// static factory method impl
		GenerateCodeCommandStub* GenerateCodeCommandStub::create(Message* message, GenerateCodeRequest* request, MatchCodeGenSlave* componentImpl)
		{
			return BLAZE_NEW GenerateCodeCommand(message, request, static_cast<MatchCodeGenSlaveImpl*>(componentImpl));
		}

	} // MatchCodeGen
} // Blaze
