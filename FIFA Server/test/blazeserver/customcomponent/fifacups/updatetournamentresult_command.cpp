/*************************************************************************************************/
/*!
    \file   updatetournamentresult_command.cpp

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
#include "fifacups/rpc/fifacupsslave/updatetournamentresult_stub.h"
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

        class UpdateTournamentResultCommand : public UpdateTournamentResultCommandStub  
        {
        public:
            UpdateTournamentResultCommand(Message* message, UpdateTournamentResultRequest* request, FifaCupsSlaveImpl* componentImpl)
                : UpdateTournamentResultCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~UpdateTournamentResultCommand() { }

        /* Private methods *******************************************************************************/
        private:

            UpdateTournamentResultCommandStub::Errors execute()
            {
                TRACE_LOG("[UpdateTournamentResultCommandStub:" << this << "].execute()");
                Blaze::BlazeRpcError error = Blaze::ERR_OK;

				Blaze::FifaCups::MemberId fifaCupsMemberId = static_cast<Blaze::FifaCups::MemberId>(mRequest.getMemberId());
				Blaze::FifaCups::MemberType memberType = mRequest.getMemberType();
				Blaze::FifaCups::SeasonId seasonId = mComponent->getSeasonId(fifaCupsMemberId, memberType, error);
				Blaze::FifaCups::TournamentResult result = mRequest.getTournamentResult();
				uint32_t tournamentLevel = mRequest.getTournamentLevel();
				if (Blaze::ERR_OK != error)
				{
					// member not registered in seasonal play, exit
					return UpdateTournamentResultCommand::commandErrorFromBlazeError(error);
				}

				Blaze::FifaCups::CupState cupState = mComponent->getCupState(seasonId);
	

				switch (result)
				{
				case Blaze::FifaCups::FIFACUPS_TOURNAMENTRESULT_WON:
					TRACE_LOG("[UpdateTournamentResultCommandStub:" << this << "].execute - FIFACUPS_TOURNAMENTRESULT_WON");

					if (Blaze::FifaCups::FIFACUPS_CUP_STATE_ACTIVE == cupState)
					{
						mComponent->setMemberTournamentStatus(fifaCupsMemberId, memberType, Blaze::FifaCups::FIFACUPS_TOURNAMENT_STATUS_WON);
						mComponent->updateMemberCupCount(fifaCupsMemberId, memberType, seasonId);
					}
					break;
				case Blaze::FifaCups::FIFACUPS_TOURNAMENTRESULT_ELIMINATED:
					TRACE_LOG("[UpdateTournamentResultCommandStub:" << this << "].execute - FIFACUPS_TOURNAMENTRESULT_ELIMINATED");

					if (Blaze::FifaCups::FIFACUPS_CUP_STATE_ACTIVE == cupState)
					{
						// update the member's seasonal play tournament status
						mComponent->setMemberTournamentStatus(fifaCupsMemberId, memberType, Blaze::FifaCups::FIFACUPS_TOURNAMENT_STATUS_ELIMINATED);
						mComponent->updateMemberElimCount(fifaCupsMemberId, memberType, seasonId);
						if (tournamentLevel==1)
						{
							mComponent->updateMemberCupsEntered(fifaCupsMemberId, memberType, seasonId);
						}
					}
					break;
				case Blaze::FifaCups::FIFACUPS_TOURNAMENTRESULT_ADVANCED:
					TRACE_LOG("[UpdateTournamentResultCommandStub:" << this << "].execute - FIFACUPS_TOURNAMENTRESULT_ADVANCED");

					if (Blaze::FifaCups::FIFACUPS_CUP_STATE_ACTIVE == cupState)
					{
						if (tournamentLevel==2)
						{
							mComponent->updateMemberCupsEntered(fifaCupsMemberId, memberType, seasonId);
						}
					}
					break;
				default:
					break;
				}
                return UpdateTournamentResultCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            FifaCupsSlaveImpl* mComponent;
        };


        // static factory method impl
        UpdateTournamentResultCommandStub* UpdateTournamentResultCommandStub::create(Message* message, UpdateTournamentResultRequest* request, FifaCupsSlave* componentImpl)
        {
            return BLAZE_NEW UpdateTournamentResultCommand(message, request, static_cast<FifaCupsSlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
