/*************************************************************************************************/
/*!
    \file   getseasonsrankinginfo_command.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetSeasonsRankingInfoCommand

    Retrieves the current leaderboard ranking information for each season

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/getseasonsrankinginfo_stub.h"

// global includes

// framework includes
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        class GetSeasonsRankingInfoCommand : public GetSeasonsRankingInfoCommandStub  
        {
        public:
            GetSeasonsRankingInfoCommand(Message* message, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : GetSeasonsRankingInfoCommandStub(message), mComponent(componentImpl)
            { }

            virtual ~GetSeasonsRankingInfoCommand() { }

        /* Private methods *******************************************************************************/
        private:

            GetSeasonsRankingInfoCommandStub::Errors execute()
            {
                TRACE_LOG("[GetSeasonsRankingInfoCommand:" << this << "].execute()");

                Blaze::BlazeRpcError error = Blaze::ERR_OK;

                const OSDKSeasonalPlaySlaveImpl::SeasonConfigurationServerMap *pConfigMap = &mComponent->getSeasonConfigurationMap();
                
                if (pConfigMap->size() > 0)
                {
                    OSDKSeasonalPlaySlaveImpl::SeasonConfigurationServerMap::const_iterator itr = pConfigMap->begin();
                    OSDKSeasonalPlaySlaveImpl::SeasonConfigurationServerMap::const_iterator itr_end = pConfigMap->end();

                    for( ; itr != itr_end; ++itr)
                    {
                        const SeasonConfigurationServer *pSeason = itr->second;

                        SeasonRankingInfo *pRankingInfo = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "SeasonRankingInfo") SeasonRankingInfo();
                        pRankingInfo->setSeasonId(pSeason->getSeasonId());

                        uint32_t rankedMemberCount = mComponent->getRankedMemberCount(pSeason->getSeasonId());
                        pRankingInfo->setLeaderboardEntityCount(rankedMemberCount);
                        
                        if (0 < rankedMemberCount)
                        {
                            // get the division starting ranks
                            eastl::vector<uint32_t> divisionStartingRanks;
                            uint8_t divisionCount = 0;
                            mComponent->calculateDivisionStartingRanks(pSeason->getSeasonId(), rankedMemberCount, divisionStartingRanks, divisionCount);

                            for (uint32_t index = 0; index < divisionCount; ++index)
                            {
                                DivisionStartingRank *pStartingRank = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "DivisionStartingRank") DivisionStartingRank();
                                pStartingRank->setNumber(index+1);
                                pStartingRank->setStartingRank(divisionStartingRanks[index]);

                                pRankingInfo->getDivisionStartingRanks().push_back(pStartingRank);
                            }
                        }

                        mResponse.getSeasonRankingInfoList().push_back(pRankingInfo);
                    }
                }
                else
                {
                    error = Blaze::OSDKSEASONALPLAY_ERR_CONFIGURATION_ERROR;
                }

                return GetSeasonsRankingInfoCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        GetSeasonsRankingInfoCommandStub* GetSeasonsRankingInfoCommandStub::create(Message* message, OSDKSeasonalPlaySlave* componentImpl)
        {
            return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "GetSeasonsRankingInfoCommand") GetSeasonsRankingInfoCommand(message, static_cast<OSDKSeasonalPlaySlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
