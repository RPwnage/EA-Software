/*************************************************************************************************/
/*!
    \file   setqualifyingdiv_command.cpp

    $Header: //gosdev/games/FIFA/2013/Gen3/DEV/blazeserver/3.x/customcomponent/osdkseasonalplay/updatetournamentresult_command.cpp#3 $
    $Change: 367571 $
    $DateTime: 2012/03/15 15:45:43 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateTournamentresultCommand

    Update the tournament result of the given entity

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifacups/rpc/fifacupsslave/setqualifyingdiv_stub.h"
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

        class SetQualifyingDivCommand : public SetQualifyingDivCommandStub  
        {
        public:
            SetQualifyingDivCommand(Message* message, SetQualifyingDivRequest* request, FifaCupsSlaveImpl* componentImpl)
                : SetQualifyingDivCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~SetQualifyingDivCommand() { }

        /* Private methods *******************************************************************************/
        private:

            SetQualifyingDivCommandStub::Errors execute()
            {
                TRACE_LOG("[SetQualifyingDivCommandStub:" << this << "].execute()");
                Blaze::BlazeRpcError error = Blaze::ERR_OK;

				Blaze::FifaCups::MemberId fifaCupsMemberId = static_cast<Blaze::FifaCups::MemberId>(mRequest.getMemberId());
				Blaze::FifaCups::MemberType memberType = mRequest.getMemberType();
				uint32_t div = mRequest.getDiv();

				error = mComponent->setMemberQualifyingDivision(fifaCupsMemberId, memberType, div);
                
				return SetQualifyingDivCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            FifaCupsSlaveImpl* mComponent;
        };


        // static factory method impl
        SetQualifyingDivCommandStub* SetQualifyingDivCommandStub::create(Message* message, SetQualifyingDivRequest* request, FifaCupsSlave* componentImpl)
        {
            return BLAZE_NEW SetQualifyingDivCommand(message, request, static_cast<FifaCupsSlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
