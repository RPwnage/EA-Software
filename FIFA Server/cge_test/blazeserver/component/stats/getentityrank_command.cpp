/*************************************************************************************************/
/*!
\file   getentityrank_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
#include "framework/userset/userset.h"

#include "stats/statsconfig.h"
#include "stats/rpc/statsslave/getentityrank_stub.h"
#include "stats/statsslaveimpl.h"

namespace Blaze
{
    namespace Stats
    {

        class GetEntityRankCommand : public GetEntityRankCommandStub
        {
        public:
            GetEntityRankCommand(Message* message, FilteredLeaderboardStatsRequest* request, StatsSlaveImpl* componentImpl)
                : GetEntityRankCommandStub(message, request),
                mComponent(componentImpl)
            {
            }

            ~GetEntityRankCommand() override { }

        private:
            // Not owned memory
            StatsSlaveImpl *mComponent;

            GetEntityRankCommand::Errors execute() override
            {
                BlazeRpcError error = Blaze::ERR_OK;
                EA::TDF::ObjectId userSetId = mRequest.getUserSetId();

                if (userSetId != EA::TDF::OBJECT_ID_INVALID)
                {
                    BlazeIdList ids;
                    error = gUserSetManager->getUserBlazeIds(userSetId, ids);
                    if (error != Blaze::ERR_OK)
                    {   
                        WARN_LOG("[GetEntityRankCommand].execute: object ID is invalid <"
                            << userSetId.toString().c_str() << ">");
                        return STATS_ERR_INVALID_OBJECT_ID;
                    }

                    for (BlazeIdList::const_iterator blazeIdItr = ids.begin(); blazeIdItr != ids.end(); ++blazeIdItr)
                    {
                        BlazeId blazeId = *blazeIdItr;

                        /// @todo order N squared behavior here => optimize using a hash set
                        EntityIdList::const_iterator idItr = mRequest.getListOfIds().begin();
                        EntityIdList::const_iterator idEnd = mRequest.getListOfIds().end();
                        for (; idItr != idEnd; ++idItr)
                        {
                            if ((*idItr) == blazeId)
                            {
                                // already in entity id list
                                break;
                            }
                        }
                        if (idItr == idEnd)
                        {
                            mRequest.getListOfIds().push_back(blazeId);
                        }
                    }
                }

                if (mRequest.getListOfIds().size() == 0)
                {
                    TRACE_LOG("[GetEntityRankCommand].execute: leaderboard <"
                        << mRequest.getBoardName() << "> of ZERO size requested");
                    return ERR_OK;
                }


                int32_t periodId = 0;
                int32_t time = mRequest.getTime();
                EntityIdList keys;
                ScopeNameValueMap& scopeNameValueMap = mRequest.getKeyScopeNameValueMap();
                ScopeNameValueListMap combinedScopeNameValueListMap;
                int32_t periodIds[STAT_NUM_PERIODS];

                const StatLeaderboard *leaderboard = mComponent->getConfigData()->getStatLeaderboard(mRequest.getBoardId(), mRequest.getBoardName());

                if (leaderboard == nullptr)
                {
                    TRACE_LOG("[GetEntityRankCommand].execute() - invalid leaderboard ID: "
                        << mRequest.getBoardId() << "  or Name: " << mRequest.getBoardName() << ".");
                    return STATS_ERR_INVALID_LEADERBOARD_ID;
                }

                if (!leaderboard->isInMemory())
                {
                    return STATS_ERR_LEADERBOARD_NOT_IN_MEMORY;
                }

                StatPeriodType periodType = static_cast<StatPeriodType>(leaderboard->getPeriodType());
                mComponent->getPeriodIds(periodIds);
                keys.reserve(mRequest.getListOfIds().size());

                if (periodType != STAT_PERIOD_ALL_TIME)
                {
                    if (time == 0)
                    {
                        periodId = periodIds[periodType];

                        if (mRequest.getPeriodOffset()<0 || periodId<mRequest.getPeriodOffset())
                        {
                            ERR_LOG("[GetEntityRankCommand].execute(): bad period offset value");
                            return STATS_ERR_BAD_PERIOD_OFFSET;
                        }

                        periodId -= mRequest.getPeriodOffset();
                    }
                    else
                    {
                        periodId = mComponent->getPeriodIdForTime(time, periodType);
                    }
                }

                // Combine leaderboard-defined and user-specified keyscopes in order defined by lb config.
                // Note that we don't care if there are more user-specified keyscopes than needed.
                if (leaderboard->getHasScopes())
                {
                    const ScopeNameValueListMap* lbScopeNameValueListMap = leaderboard->getScopeNameValueListMap();
                    ScopeNameValueListMap::const_iterator lbScopeMapItr = lbScopeNameValueListMap->begin();
                    ScopeNameValueListMap::const_iterator lbScopeMapEnd = lbScopeNameValueListMap->end();
                    for (; lbScopeMapItr != lbScopeMapEnd; ++lbScopeMapItr)
                    {
                        // any aggregate key indicator has already been replaced with actual aggregate key value

                        if (mComponent->getConfigData()->getKeyScopeSingleValue(lbScopeMapItr->second->getKeyScopeValues()) != KEY_SCOPE_VALUE_USER)
                        {
                            combinedScopeNameValueListMap.insert(eastl::make_pair(lbScopeMapItr->first, lbScopeMapItr->second->clone()));
                            continue;
                        }

                        ScopeNameValueMap::const_iterator userScopeMapItr = scopeNameValueMap.find(lbScopeMapItr->first);
                        if (userScopeMapItr == scopeNameValueMap.end())
                        {
                            ERR_LOG("[GetEntityRankCommand].execute() - leaderboard: <" << leaderboard->getBoardName() 
                                << ">: scope value for <" << lbScopeMapItr->first.c_str() << "> is missing in request");
                            return STATS_ERR_BAD_SCOPE_INFO;
                        }

                        if (!mComponent->getConfigData()->isValidScopeValue(userScopeMapItr->first, userScopeMapItr->second))
                        {
                            ERR_LOG("[GetEntityRankCommand].execute() - leaderboard: <" << leaderboard->getBoardName() 
                                << ">: scope value: <" << userScopeMapItr->second << "> for scope name:<" << lbScopeMapItr->first.c_str() << "> is not defined");
                            return STATS_ERR_BAD_SCOPE_INFO;
                        }

                        ScopeValuesPtr scopeValues = BLAZE_NEW ScopeValues();
                        (scopeValues->getKeyScopeValues())[userScopeMapItr->second] = userScopeMapItr->second;
                        combinedScopeNameValueListMap.insert(eastl::make_pair(lbScopeMapItr->first, scopeValues));
                    }
                }

                // start building the TDF response with LeaderboardStatValuesRows for entity ids and ranks
                EntityRankMap& entityRankMap = mResponse.getEntityRankMap();
                LeaderboardStatValues values;
                bool sorted = false;
                error = mComponent->getFilteredLeaderboardEntries(*leaderboard, &combinedScopeNameValueListMap, periodId, 
                    mRequest.getListOfIds().asVector(), mRequest.getLimit(), keys, values, sorted);

                if (error != Blaze::ERR_OK)
                {
                    return commandErrorFromBlazeError(error);
                }

                LeaderboardStatValues::RowList& rowList = values.getRows();
                LeaderboardStatValues::RowList::iterator iter = rowList.begin();
                for (;iter != rowList.end(); ++iter)
                {
                    EntityId id = (*iter)->getUser().getBlazeId();
                    int32_t rank = (*iter)->getRank();
                    entityRankMap[id] = rank;
                }
                return ERR_OK;
            }
        };

        DEFINE_GETENTITYRANK_CREATE()
    } //Stats
} //Blaze
