/*************************************************************************************************/
/*!
    \file   getcupinfo_command.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/getcupinfo_command.cpp#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetCupInfoCommand

    Retrieves the cup info for the given entity

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifacups/rpc/fifacupsslave/getcupinfo_stub.h"

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

        class GetCupInfoCommand : public GetCupInfoCommandStub  
        {
        public:
            GetCupInfoCommand(Message* message, GetCupInfoRequest* request, FifaCupsSlaveImpl* componentImpl)
                : GetCupInfoCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~GetCupInfoCommand() { }

        /* Private methods *******************************************************************************/
        private:
            static const uint32_t INVALID_CUP_ID = 0;
            GetCupInfoCommand::Errors execute()
            {
				TRACE_LOG("[GetCupInfoCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				SeasonId seasonId = 0;
				uint32_t qualifyingDiv = 0;
				uint32_t pendingDiv = 0;
				uint32_t activeCupId = INVALID_CUP_ID;
				TournamentStatus tournamentStatus = FIFACUPS_TOURNAMENT_STATUS_NONE;
				mComponent->getRegistrationDetails(mRequest.getMemberId(), mRequest.getMemberType(), seasonId, qualifyingDiv, pendingDiv, activeCupId, tournamentStatus, error);

				if (Blaze::ERR_OK != error)
				{
					// if it's clubs then try registering
					if (mRequest.getMemberType() == FIFACUPS_MEMBERTYPE_CLUB)
					{
						mComponent->registerClub(mRequest.getMemberId(), 1);
						mComponent->getRegistrationDetails(mRequest.getMemberId(), mRequest.getMemberType(), seasonId, qualifyingDiv, pendingDiv, activeCupId, tournamentStatus, error);
					}
					else if (mRequest.getMemberType() == FIFACUPS_MEMBERTYPE_COOP)  
					{
						mComponent->getCupDetailsForMemberType(mRequest.getMemberType(), seasonId, qualifyingDiv, activeCupId, error);
					}
				}

				if (Blaze::ERR_OK != error)
				{
					// entity is not registered in the season
					error = Blaze::FIFACUPS_ERR_NOT_REGISTERED;
				}
				else
				{
					mResponse.setSeasonId(seasonId);
					mResponse.setQualifyingDivision(qualifyingDiv);					                  
					TRACE_LOG("[FifaCupsSlaveImpl].GetCupInfoCommand. qualifyingDiv = "<<qualifyingDiv<<" activeCupId = " <<activeCupId);
 
					// cup state
					mResponse.setCupState(mComponent->getCupState(seasonId));

					// cup start and end times
					mResponse.setCupStart(mComponent->getCupStartTime(seasonId));
					mResponse.setCupEnd(mComponent->getCupEndTime(seasonId));

					// next cup window start time
					mResponse.setNextCupStart(mComponent->getNextCupStartTime(seasonId));

					// tournament info
					uint32_t cupId = INVALID_CUP_ID;
					uint32_t tournamentId = 0;
					TournamentRule tournamentRule = FIFACUPS_TOURNAMENTRULE_ONE_ATTEMPT;

					mComponent->getTournamentDetails(seasonId, qualifyingDiv, tournamentId, cupId, tournamentRule, error);

					if (Blaze::ERR_OK == error)
					{
						CupInfo::CupList& cList = mResponse.getCups();
						mComponent->getCupList(seasonId, error, cList);
					}

					if (Blaze::ERR_OK != error)
					{
						// entity is not registered in the season
						error = Blaze::FIFACUPS_ERR_CONFIGURATION_ERROR;
					}
					else
					{
						mResponse.setTournamentId(tournamentId);
						mResponse.setTournamentStatus(tournamentStatus);
						mResponse.setTournamentEligible(mComponent->getIsTournamentEligible(seasonId, tournamentRule, tournamentStatus));

                        //verify if the cup player used to be engaged has expired or not
                        if (activeCupId != INVALID_CUP_ID)
                        {
                            for (const Blaze::FifaCups::Cup* cupInfo : mResponse.getCups())
                            {
                                if (cupInfo->getCupId() == activeCupId)
                                {
                                    if (cupInfo->getState() == FIFACUPS_CUP_STATE_INACTIVE)
                                    {
                                        //expired cup, let's reset active-cup-id to none
                                        activeCupId = INVALID_CUP_ID;
                                    }
                                    break;
                                }
                            }
                        }
                        mResponse.setCupId(activeCupId);
					}
				}

                return GetCupInfoCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            FifaCupsSlaveImpl* mComponent;
        };


        // static factory method impl
        GetCupInfoCommandStub* GetCupInfoCommandStub::create(Message* message, GetCupInfoRequest* request, FifaCupsSlave* componentImpl)
        {
            return BLAZE_NEW GetCupInfoCommand(message, request, static_cast<FifaCupsSlaveImpl*>(componentImpl));
        }

    } // FifaCups
} // Blaze
