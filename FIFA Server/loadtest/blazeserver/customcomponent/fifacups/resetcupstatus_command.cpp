/*************************************************************************************************/
/*!
    \file   resetcupstatus_command.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/resetcupstatus_command.cpp#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ResetCupStatusCommand

    Reset the cup tournament tree and tournament status for the given entity

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifacups/rpc/fifacupsslave/resetcupstatus_stub.h"

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

#include "osdktournaments/rpc/osdktournamentsslave.h"
#include "osdktournaments/tdf/osdktournamentstypes_custom.h"
#include "osdktournaments/tdf/osdktournamentstypes.h"

namespace Blaze
{
    namespace FifaCups
    {

        class ResetCupStatusCommand : public ResetCupStatusCommandStub  
        {
        public:
            ResetCupStatusCommand(Message* message, ResetCupStatusRequest* request, FifaCupsSlaveImpl* componentImpl)
                : ResetCupStatusCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~ResetCupStatusCommand() { }

        /* Private methods *******************************************************************************/
        private:

            ResetCupStatusCommand::Errors execute()
            {
				TRACE_LOG("[ResetCupStatusCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				SeasonId seasonId = 0;
				uint32_t qualifyingDiv = 0;
				uint32_t pendingDiv = 0;
				uint32_t activeCupId = 0;
				TournamentStatus tournamentStatus = FIFACUPS_TOURNAMENT_STATUS_NONE;
				mComponent->getRegistrationDetails(mRequest.getMemberId(), mRequest.getMemberType(), seasonId, qualifyingDiv, pendingDiv, activeCupId, tournamentStatus, error);

				if (Blaze::ERR_OK != error)
				{
					// entity is not registered in the season
					error = Blaze::FIFACUPS_ERR_NOT_REGISTERED;
				}
				else
				{
					// tournament info
					uint32_t cupId = 0;
					uint32_t tournamentId = 0;
					TournamentRule tournamentRule = FIFACUPS_TOURNAMENTRULE_ONE_ATTEMPT;

					mComponent->getTournamentDetails(seasonId, qualifyingDiv, tournamentId, cupId, tournamentRule, error);
					if (Blaze::ERR_OK != error)
					{
						// entity is not registered in the season
						error = Blaze::FIFACUPS_ERR_CONFIGURATION_ERROR;
					}
					else
					{
						DbConnPtr conn = gDbScheduler->getConnPtr(mComponent->getDbId());
						if (conn == NULL)
						{
							// early return here when the scheduler is unable to find a connection
							ERR_LOG("[ResetMyTournamentCommand].execute: unable to find a database connection");
							return ResetCupStatusCommand::commandErrorFromBlazeError(Blaze::FIFACUPS_ERR_DB);
						}

						OSDKTournaments::OSDKTournamentsSlave *osdkTournamentsComponent =
							static_cast<OSDKTournaments::OSDKTournamentsSlave*>(gController->getComponent(OSDKTournaments::OSDKTournamentsSlave::COMPONENT_ID,false));
						if (osdkTournamentsComponent != NULL)
						{
							OSDKTournaments::TournamentMode mode = OSDKTournaments::MODE_FIFA_CUPS_USER;
							switch (mRequest.getMemberType())
							{
							case FIFACUPS_MEMBERTYPE_USER:	mode = OSDKTournaments::MODE_FIFA_CUPS_USER; break;
							case FIFACUPS_MEMBERTYPE_CLUB:	mode = OSDKTournaments::MODE_FIFA_CUPS_CLUB; break;
							case FIFACUPS_MEMBERTYPE_COOP:	mode = OSDKTournaments::MODE_FIFA_CUPS_COOP; break;
							default:					mode = OSDKTournaments::MODE_FIFA_CUPS_USER; break;
							}

							OSDKTournaments::ResetTournamentRequest resetRequest;
							resetRequest.setMode(mode);
							resetRequest.setTournamentId(tournamentId);
							resetRequest.setSetActive(true);
							resetRequest.setMemberIdOverride(mRequest.getMemberId());

							error = osdkTournamentsComponent->resetTournament(resetRequest);

							if (error != DB_ERR_OK)
							{
								return ResetCupStatusCommand::commandErrorFromBlazeError(Blaze::FIFACUPS_ERR_DB);
							}
							else
							{
								//reset tournament status
								error = mComponent->setMemberTournamentStatus(mRequest.getMemberId(), mRequest.getMemberType(), FIFACUPS_TOURNAMENT_STATUS_NONE, conn);
							}

							if (error != Blaze::ERR_OK)
							{
								return ResetCupStatusCommand::commandErrorFromBlazeError(Blaze::FIFACUPS_ERR_GENERAL);
							}
							else
							{
								error = mComponent->setMemberActiveCupId(mRequest.getMemberId(), mRequest.getMemberType(), 0);
							}
						}
					}
				}

                return ResetCupStatusCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            FifaCupsSlaveImpl* mComponent;
        };


        // static factory method impl
        ResetCupStatusCommandStub* ResetCupStatusCommandStub::create(Message* message, ResetCupStatusRequest* request, FifaCupsSlave* componentImpl)
        {
            return BLAZE_NEW ResetCupStatusCommand(message, request, static_cast<FifaCupsSlaveImpl*>(componentImpl));
        }

    } // FifaCups
} // Blaze
