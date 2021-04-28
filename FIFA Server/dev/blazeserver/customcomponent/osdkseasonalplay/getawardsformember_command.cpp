/*************************************************************************************************/
/*!
    \file   getawardsformember_command.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetAwardsForMemberCommand

    Retrieves the seasonal play awards for the specified member

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/getawardsformember_stub.h"

// framework includes
#include "framework/database/dbscheduler.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "osdkseasonalplay/osdkseasonalplaydb.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        class GetAwardsForMemberCommand : public GetAwardsForMemberCommandStub  
        {
        public:
            GetAwardsForMemberCommand(Message* message, GetAwardsForMemberRequest* request, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : GetAwardsForMemberCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~GetAwardsForMemberCommand() { }

        /* Private methods *******************************************************************************/
        private:

            GetAwardsForMemberCommandStub::Errors execute()
            {
                INFO_LOG("[GetAwardsForMemberCommand:" << this << "].execute()");

                Blaze::BlazeRpcError error = Blaze::ERR_OK;

                // TODO - break this mega method into smaller methods.
                               
                // establish a database connection
                OSDKSeasonalPlayDb dbHelper(mComponent->getDbId());
                error = dbHelper.getBlazeRpcError();

                if(Blaze::ERR_OK != error)
                {
                    return GetAwardsForMemberCommand::commandErrorFromBlazeError(error); // EARLY RETURN
                }

                // fetch all the awards for the member in that season regardless of season number as we need all
                // the awards to calculate the times won and last season won.
                // we'll filter on season number later if needed
                AwardList awardList;
                error = mComponent->getAwardsByMember(
                    dbHelper, mRequest.getMemberId(), mRequest.getMemberType(), mRequest.getSeasonId(), 1/*AWARD_ANY_SEASON_NUMBER*/,
                    awardList);

                if(Blaze::ERR_OK != error)
                {
                    return GetAwardsForMemberCommand::commandErrorFromBlazeError(error); // EARLY RETURN
                }

                dbHelper.setDbConnection(DbConnPtr()); // release the connection

                // construct our award summary map so we can update it
                eastl::map<AwardId, AwardSummary*>  awardSummaryMap;

                // populate the award summary map with all of the configured awards
                const OSDKSeasonalPlaySlaveImpl::AwardConfigurationMap *pConfigMap = &mComponent->getAwardConfigurationMap();
                if (pConfigMap->size() > 0)
                {
                    OSDKSeasonalPlaySlaveImpl::AwardConfigurationMap::const_iterator itr = pConfigMap->begin();
                    OSDKSeasonalPlaySlaveImpl::AwardConfigurationMap::const_iterator itr_end = pConfigMap->end();

                    for( ; itr != itr_end; ++itr)
                    {
                        const AwardConfiguration *pAward = itr->second;

                        AwardSummary *awardSummary = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "AwardSummary") AwardSummary();
                        awardSummary->setAwardId(pAward->getAwardId());
                        awardSummary->setTimesWon(0);
                        awardSummary->setSeasonNumberLastAwarded(0);

                        awardSummaryMap[awardSummary->getAwardId()] = awardSummary;
                    }
                }

                // iterate through all of the retrieved awards applying the season number if needed
                // and updating the award summary
                AwardList::const_iterator awardItr = awardList.begin();
                AwardList::const_iterator awardItr_end = awardList.end();
                for ( ; awardItr != awardItr_end; ++awardItr)
                {
                    const Award *award = *awardItr;

                    // check if we have to add it to the response's award list
                    if (static_cast<uint32_t>(AWARD_ANY_SEASON_NUMBER) == mRequest.getSeasonNumber() || 
                        award->getSeasonNumber() == mRequest.getSeasonNumber())
                    {
                        Award *awardToInsert = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "Award") Award();
                        award->copyInto(*awardToInsert);
                        mResponse.getAwardList().push_back(awardToInsert);
                    }

                    // update the award summary information
                    AwardSummary *awardSummary = awardSummaryMap[award->getAwardId()];
                    if (NULL != awardSummary)
                    {
                        awardSummary->setTimesWon(awardSummary->getTimesWon() + 1);

                        if (awardSummary->getSeasonNumberLastAwarded() < award->getSeasonNumber())
                        {
                            awardSummary->setSeasonNumberLastAwarded(award->getSeasonNumber());
                            awardSummary->setTimeStampLastAwarded(award->getTimeStamp());
                        }
                    }
                    else
                    {
                        TRACE_LOG("[GetAwardsForMemberCommand:" << this << "].execute(). Found awardid " << award->getAwardId() << " that doesn't exist in summary map");
                    }
                }

                // now copy the awardsummarymap into the response
                eastl::map<AwardId, AwardSummary*>::const_iterator  mapItr = awardSummaryMap.begin();
                eastl::map<AwardId, AwardSummary*>::const_iterator  mapItr_end = awardSummaryMap.end();

                for ( ; mapItr != mapItr_end; ++mapItr)
                {
                    AwardSummary *awardSummery = mapItr->second;

                    AwardSummary *awardSummaryToInsert = BLAZE_NEW AwardSummary();
                    awardSummery->copyInto(*awardSummaryToInsert);

                    mResponse.getAwardSummaryList().push_back(awardSummaryToInsert);

                    // need to delete the original award summary so that we don't
                    // leak memory in the awardSummaryMap
                    delete awardSummery;
                }

                return GetAwardsForMemberCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        GetAwardsForMemberCommandStub* GetAwardsForMemberCommandStub::create(Message* message, GetAwardsForMemberRequest* request, OSDKSeasonalPlaySlave* componentImpl)
        {
            return BLAZE_NEW GetAwardsForMemberCommand(message, request, static_cast<OSDKSeasonalPlaySlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
