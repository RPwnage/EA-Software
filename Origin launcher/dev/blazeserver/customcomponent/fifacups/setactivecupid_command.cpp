/*************************************************************************************************/
/*!
    \file   setactivecupid_command.cpp

 	\attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SetActiveCupIdCommand

    Set current active cup id

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifacups/rpc/fifacupsslave/setactivecupid_stub.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"
//#include "framework/system/identity.h"

// fifacups includes
#include "fifacupsslaveimpl.h"
#include "fifacups/tdf/fifacupstypes.h"
#include "fifacups/fifacupsdb.h"

namespace Blaze
{
    namespace FifaCups
    {

        class SetActiveCupIdCommand : public SetActiveCupIdCommandStub  
        {
        public:
            SetActiveCupIdCommand(Message* message, SetActiveCupIdRequest* request, FifaCupsSlaveImpl* componentImpl)
                : SetActiveCupIdCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~SetActiveCupIdCommand() { }

        /* Private methods *******************************************************************************/
        private:

            SetActiveCupIdCommand::Errors execute()
            {
				TRACE_LOG("[SetActiveCupIdCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				mComponent->setMemberActiveCupId(mRequest.getMemberId(), mRequest.getMemberType(), mRequest.getCupId());

				if (Blaze::ERR_OK != error)
				{
					error = Blaze::FIFACUPS_ERR_GENERAL;
				}
				
                return SetActiveCupIdCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            FifaCupsSlaveImpl* mComponent;
        };


        // static factory method impl
        SetActiveCupIdCommandStub* SetActiveCupIdCommandStub::create(Message* message, SetActiveCupIdRequest* request, FifaCupsSlave* componentImpl)
        {
            return BLAZE_NEW SetActiveCupIdCommand(message, request, static_cast<FifaCupsSlaveImpl*>(componentImpl));
        }

    } // FifaCups
} // Blaze
