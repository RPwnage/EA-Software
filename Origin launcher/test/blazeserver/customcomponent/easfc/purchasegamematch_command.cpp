/*************************************************************************************************/
/*!
    \file   purchasegamematch_command.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/easfc/purchasegamematch_command.cpp#1 $

    \attention
    (c) Electronic Arts Inc. 2012
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class PurchaseGameMatchCommand

    Grant an extra match in the current H2H seasons.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"
#include "framework/controller/controller.h"

#include "component/stats/updatestatshelper.h"
#include "component/stats/updatestatsrequestbuilder.h"

#include "customcode/component/gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplay.h"

// easfc includes
#include "easfcslaveimpl.h"
#include "easfc/tdf/easfctypes.h"
#include "easfc/rpc/easfcslave/purchasegamematch_stub.h"

namespace Blaze
{
    namespace EASFC
    {

        class PurchaseGameMatchCommand : public PurchaseGameMatchCommandStub  
        {
        public:
            PurchaseGameMatchCommand(Message* message, PurchaseGameRequest* request, EasfcSlaveImpl* componentImpl)
                : PurchaseGameMatchCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~PurchaseGameMatchCommand() { }

        /* Private methods *******************************************************************************/
        private:

            PurchaseGameMatchCommand::Errors execute()
            {
				TRACE_LOG("[PurchaseGameMatchCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				int64_t entityId = mRequest.getMemberId();

				eastl::string divisionalStatsCategory = "";
				switch(mRequest.getMemberType())
				{
				case EASFC_MEMBERTYPE_USER:
					divisionalStatsCategory = "SPDivisionalPlayerStats";
					break;
				case EASFC_MEMBERTYPE_COOP_PAIR:
					divisionalStatsCategory = "SPDivisionalCoopStats";
					break;
				case EASFC_MEMBERTYPE_CLUB: //lint -fallthrough
				default:
					error = Blaze::EASFC_ERR_INVALID_PARAMS;
					TRACE_LOG("[PurchaseGameMatchCommand].execute(): ERROR! invalid MemberType");
					break;
				}

				Blaze::Stats::UpdateStatsRequestBuilder builder;
				Blaze::Stats::UpdateStatsHelper updateStatsHelper;

				//----------------------------------------------------------------------------------
				// Set up the initial selects for the stats we want to update
				//----------------------------------------------------------------------------------	
				if (error == Blaze::ERR_OK)
				{
					builder.startStatRow(divisionalStatsCategory.c_str(), entityId, NULL);
					builder.selectStat(STATNAME_PROJPTS);
					builder.selectStat(STATNAME_PREVPROJPTS);
					builder.selectStat(STATNAME_GAMES_PLAYED);
					builder.selectStat(STATNAME_EXTRAMATCHES);
					builder.selectStat(STATNAME_SPPOINTS);
					builder.completeStatRow();

					bool strict = true; //mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();		
					error = updateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)builder, strict);
				}
				//----------------------------------------------------------------------------------
				// Perform the fetching of the select statements
				//----------------------------------------------------------------------------------
				if (error == Blaze::ERR_OK)
				{
					error = updateStatsHelper.fetchStats();
				}

				//----------------------------------------------------------------------------------
				// Execute the logic and perform the update of the incrementStat statements
				//----------------------------------------------------------------------------------
				if (error == Blaze::ERR_OK)
				{
					Stats::UpdateRowKey* key = builder.getUpdateRowKey(divisionalStatsCategory.c_str(), entityId);
					Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key->period);
					GameReporting::DefinesHelper lDefinesHelper;
					int64_t projectedPoints = updateStatsHelper.getValueInt(key, STATNAME_PROJPTS, periodType, true);
					int64_t prevProjectedPoints = updateStatsHelper.getValueInt(key, STATNAME_PREVPROJPTS, periodType, true);
					int64_t gamesPlayed = updateStatsHelper.getValueInt(key, STATNAME_GAMES_PLAYED, periodType, true);
					int64_t extraMatches = updateStatsHelper.getValueInt(key, STATNAME_EXTRAMATCHES, periodType, true);
					int64_t seasonCurrentPoints = updateStatsHelper.getValueInt(key, STATNAME_SPPOINTS, periodType, true);

					extraMatches++;
					GameReporting::FifaSeasonalPlay::calculateProjectedPoints(projectedPoints,prevProjectedPoints,gamesPlayed,extraMatches,seasonCurrentPoints,&lDefinesHelper);
					updateStatsHelper.setValueInt(key, STATNAME_PROJPTS, periodType, projectedPoints);
					updateStatsHelper.setValueInt(key, STATNAME_PREVPROJPTS, periodType, prevProjectedPoints);
					updateStatsHelper.setValueInt(key, STATNAME_GAMES_PLAYED, periodType, gamesPlayed);
					updateStatsHelper.setValueInt(key, STATNAME_EXTRAMATCHES, periodType, extraMatches);
					updateStatsHelper.setValueInt(key, STATNAME_MATCHREDEEMED, periodType, EasfcSlaveImpl::SeasonsItemState::ITEM_REDEEMED);
					updateStatsHelper.setValueInt(key, STATNAME_SPPOINTS, periodType, seasonCurrentPoints);

					error = updateStatsHelper.commitStats();
				}

                return PurchaseGameMatchCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            EasfcSlaveImpl* mComponent;
        };


        // static factory method impl
        PurchaseGameMatchCommandStub* PurchaseGameMatchCommandStub::create(Message* message, PurchaseGameRequest* request, EasfcSlave* componentImpl)
        {
            return BLAZE_NEW PurchaseGameMatchCommand(message, request, static_cast<EasfcSlaveImpl*>(componentImpl));
        }

    } // EASFC
} // Blaze
