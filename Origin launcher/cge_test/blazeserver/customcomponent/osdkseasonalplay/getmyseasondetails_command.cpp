/*************************************************************************************************/
/*!
    \file   getmyseasondetails_command.cpp

    $Header: //gosdev/games/FIFA/2022/GenX/cge_test/blazeserver/customcomponent/osdkseasonalplay/getmyseasondetails_command.cpp#1 $
    $Change: 1653004 $
    $DateTime: 2021/03/02 12:48:45 $

    \attention
        (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetMySeasonDetailsCommand

    Retrieves the details of the specified season that are specific to the specified registered
    entity

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/getmyseasondetails_stub.h"

// global includes

// framework includes
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "osdkseasonalplay/osdkseasonalplaydb.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        class GetMySeasonDetailsCommand : public GetMySeasonDetailsCommandStub  
        {
        public:
            GetMySeasonDetailsCommand(Message* message, GetMySeasonDetailsRequest* request, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : GetMySeasonDetailsCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~GetMySeasonDetailsCommand() { }

            /* Private methods *******************************************************************************/
        private:

            GetMySeasonDetailsCommandStub::Errors execute()
            {
                TRACE_LOG("[GetMySeasonDetailsCommandStub:" << this << "].execute()");
                Blaze::BlazeRpcError error = Blaze::ERR_OK;
                SeasonId seasonId = 0xFFFFFFFF;

                // establish a database connection
                OSDKSeasonalPlayDb dbHelper(mComponent->getDbId());
                error = dbHelper.getBlazeRpcError();
                
                // fetch a season id
                if(Blaze::ERR_OK == error)
                {
                    error = dbHelper.fetchSeasonId(mRequest.getMemberId(), mRequest.getMemberType(), seasonId);
                }
                
                // verify if the entity is registered in a season
                if (Blaze::ERR_OK != error || seasonId != mRequest.getSeasonId())
                {
                    // entity is not registered in the season
                    error = Blaze::OSDKSEASONALPLAY_ERR_NOT_REGISTERED;
                }
                else
                {
                    mResponse.setSeasonId(seasonId);
                    
                    mResponse.setTournamentStatus(mComponent->getMemberTournamentStatus(dbHelper, mRequest.getMemberId(), mRequest.getMemberType(), error));
                    mResponse.setTournamentEligible(mComponent->getIsTournamentEligible(dbHelper, seasonId, mRequest.getMemberId(), mRequest.getMemberType(), error));

                    uint32_t memberDivisionRank = 0, memberOverallRank = 0, divisionStartingRank = 0;
                    uint8_t memberDivision = 0;

                    mComponent->calculateRankAndDivision(seasonId, mRequest.getMemberId(), memberDivisionRank, memberDivision, divisionStartingRank, memberOverallRank);

                    mResponse.setDivision(memberDivision);
                    mResponse.setDivisionRanking(memberDivisionRank);
                    mResponse.setDivisionStartingRank(divisionStartingRank);
                    mResponse.setOverallRank(memberOverallRank);
                }

                return GetMySeasonDetailsCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        GetMySeasonDetailsCommandStub* GetMySeasonDetailsCommandStub::create(Message* message, GetMySeasonDetailsRequest* request, OSDKSeasonalPlaySlave* componentImpl)
        {
            return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "GetMySeasonDetailsCommand") GetMySeasonDetailsCommand(message, request, static_cast<OSDKSeasonalPlaySlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
