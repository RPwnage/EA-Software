/*************************************************************************************************/
/*!
    \file   registerentity_command..cpp

    \attention
    (c) Electronic Arts Inc. 2013
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RegisterEntityCommand

    Register the entiy into fifa cups

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifacups/rpc/fifacupsslave/registerentity_stub.h"
// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// osdkseasonalplay includes
#include "fifacupsslaveimpl.h"
#include "fifacups/tdf/fifacupstypes.h"
#include "fifacups/fifacupsdb.h"

namespace Blaze
{
    namespace FifaCups
    {

        class RegisterEntityCommand : public RegisterEntityCommandStub  
        {
        public:
            RegisterEntityCommand(Message* message, RegisterEntityRequest* request, FifaCupsSlaveImpl* componentImpl)
                : RegisterEntityCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~RegisterEntityCommand() { }

        /* Private methods *******************************************************************************/
        private:

            RegisterEntityCommandStub::Errors execute()
            {
                TRACE_LOG("[RegisterEntityCommandStub:" << this << "].execute()");
                Blaze::BlazeRpcError error = Blaze::ERR_OK;

				Blaze::FifaCups::MemberId fifaCupsMemberId = static_cast<Blaze::FifaCups::MemberId>(mRequest.getMemberId());
				Blaze::FifaCups::LeagueId leagueId = mRequest.getLeagueId();
				Blaze::FifaCups::MemberType memberType = mRequest.getMemberType();

				error = mComponent->registerEntity(fifaCupsMemberId, memberType, leagueId);
                
				if (error == Blaze::FIFACUPS_ERR_ALREADY_REGISTERED)
				{
					error = Blaze::ERR_OK;
				}

				return RegisterEntityCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            FifaCupsSlaveImpl* mComponent;
        };


        // static factory method impl
        RegisterEntityCommandStub* RegisterEntityCommandStub::create(Message* message, RegisterEntityRequest* request, FifaCupsSlave* componentImpl)
        {
            return BLAZE_NEW RegisterEntityCommand(message, request, static_cast<FifaCupsSlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
