/*************************************************************************************************/
/*!
\file   wipestats_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class WipeStatsCommand

Wipes Club¡¯s stats. GM privilege is required.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

// stats includes
#include "stats/statsconfig.h"
#include "stats/statsslaveimpl.h"

// clubs includes
#include "clubs/rpc/clubsslave/wipestats_stub.h"
#include "clubs/clubsslaveimpl.h"
#include "clubs/clubscommandbase.h"
#include "clubs/clubsdbconnector.h"

namespace Blaze
{
    namespace Clubs
    {
        class WipeStatsCommand : public WipeStatsCommandStub, private ClubsCommandBase
        {
        public:
            WipeStatsCommand(Message* message, WipeStatsRequest* request, ClubsSlaveImpl* componentImpl)
                : WipeStatsCommandStub(message, request),
                ClubsCommandBase(componentImpl)
            {
                isGetMemeberList = false;
            }

            ~WipeStatsCommand() override{}

        private:
            Clubs::BlazeIdList mMemberIdList;
            bool isGetMemeberList;

            WipeStatsCommandStub::Errors execute() override
            {
                TRACE_LOG("[WipeStatsCommand] start");

                const ClubId clubId = mRequest.getClubId();  

                ClubsDbConnector dbc(mComponent);
                if (!dbc.startTransaction())
                {
                    return ERR_SYSTEM;
                }

                // Lock this club to prevent racing condition
                uint64_t version = 0;
                BlazeRpcError error = dbc.getDb().lockClub(clubId, version);
                if (error != Blaze::ERR_OK)
                {
                    return commandErrorFromBlazeError(error);
                }

                const BlazeId blazeId = gCurrentUserSession->getBlazeId();
                //get the user's membership, check the member's right, only GM could wipe stats
                ClubMembership membership;
                error = dbc.getDb().getMembershipInClub(
                    clubId,
                    blazeId,
                    membership);

                // Figure out authorization level.
                bool rtorIsAdmin = false;
                bool rtorIsThisClubGM = false;
                MembershipStatus rtorMshipStatus = membership.getClubMember().getMembershipStatus();
                switch (error)
                {
                case Blaze::ERR_OK:
                    rtorIsThisClubGM = (rtorMshipStatus >= CLUBS_GM);
                    break;
                case Blaze::CLUBS_ERR_USER_NOT_MEMBER:
                    rtorIsThisClubGM = false;
                    break;
                default:
                    return commandErrorFromBlazeError(error);
                }

                if (!rtorIsThisClubGM)
                {
                    rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR);                    
                }
                if (!rtorIsAdmin && !rtorIsThisClubGM)
                {
                    if(!rtorIsAdmin)
                        WARN_LOG("[WipeStatsCommand].wipeStats: User [" << gCurrentUserSession->getBlazeId() << "] attempted to wipe stats, no permission!");
                    if (error == Blaze::ERR_OK)
                        return CLUBS_ERR_NO_PRIVILEGE;
                    else
                        return commandErrorFromBlazeError(error);
                }
                return wipeStats();
            }

            WipeStatsCommandStub::Errors wipeStats()
            {
                Stats::StatsSlaveImpl* stats = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlave::COMPONENT_ID));
                if (stats != nullptr)
                {
                    const char8_t* wipeSet = mRequest.getWipeSet();
                    //check whether existed in club config file
                    const ClubsConfig& clubsConfig = mComponent->getConfig();
                    Clubs::ClubStatsUpdates::const_iterator statsUpdateIter = clubsConfig.getClubStatsUpdates().begin();
                    bool wipeOpcodeExist = false;
                    BlazeRpcError error = Blaze::ERR_OK;
                    for (; statsUpdateIter != clubsConfig.getClubStatsUpdates().end(); ++statsUpdateIter)
                    {
                        if (blaze_stricmp(wipeSet, (*statsUpdateIter)->getWipeSet()))
                        {
                            continue;
                        }
                        wipeOpcodeExist = true;
                        //wipe stats
                        const Clubs::ClubStatsCategoryList& categories = (*statsUpdateIter)->getCategories();
                        if (categories.empty())
                        {
                            // Delete stats of the club and members
                            Stats::DeleteAllStatsByEntityRequest req;
                            req.setEntityType(Clubs::ENTITY_TYPE_CLUB);
                            req.setEntityId(mRequest.getClubId());
                            error = stats->deleteAllStatsByEntity(req);

                            if( mComponent->isUsingClubKeyscopedStats() )
                            {
                                Stats::ScopeName scopeName = "club";
                                Stats::ScopeValue scopeValue = mRequest.getClubId();
                                Stats::DeleteAllStatsByKeyScopeRequest delClubMemberStatsRequest;

                                delClubMemberStatsRequest.getKeyScopeNameValueMap().clear();
                                delClubMemberStatsRequest.getKeyScopeNameValueMap().insert(Stats::ScopeNameValueMap::value_type(scopeName, scopeValue));
                                error = stats->deleteAllStatsByKeyScope(delClubMemberStatsRequest);
                            }
                        }
                        else
                        {
                            Clubs::ClubStatsCategoryList::const_iterator categoryIter = categories.begin();
                            for (; categoryIter != categories.end(); ++categoryIter)
                            {
                                const char8_t* categoryName = (*categoryIter)->getName();
                                const char8_t* keyScopeName = (*categoryIter)->getClubKeyscopeName();

                                if (keyScopeName != nullptr && keyScopeName[0] != '\0')
                                {
                                    const Clubs::StatsList& statsList = (*categoryIter)->getStats();
                                    if (statsList.empty())
                                    {
                                        //wipe all members stats in this club by categoryName
                                        Stats::DeleteStatsByKeyScopeRequest statsReq;
                                        Stats::StatDeleteByKeyScope* statsDel = statsReq.getStatDeletes().pull_back();
                                        Stats::ScopeValue scopeValue = mRequest.getClubId();

                                        statsDel->setCategory(categoryName);
                                        statsDel->getKeyScopeNameValueMap().insert(Stats::ScopeNameValueMap::value_type(keyScopeName, scopeValue));

                                        error = stats->deleteStatsByKeyScope(statsReq);
                                    }
                                    else
                                    {
                                        //reset members stats by columns name

                                        if (!isGetMemeberList)
                                        {
                                            // Get the Club Member Blaze Id List from Club Component for the Stats request
                                            Clubs::GetMembersRequest getMembersReq;
                                            getMembersReq.setClubId(mRequest.getClubId());
                                            Clubs::GetMembersResponse getMembersRespond;
                                            BlazeRpcError err = mComponent->getMembers(getMembersReq, getMembersRespond);
                                            if(err != Blaze::ERR_OK)
                                            {
                                                ERR_LOG("[wipestats_command].wipeStatsbase() - getMembers Failed.");
                                                return commandErrorFromBlazeError(err);
                                            }

                                            Clubs::ClubMemberList& clubMemberList = getMembersRespond.getClubMemberList();
                                            Clubs::ClubMemberList::iterator it = clubMemberList.begin();
                                            Clubs::ClubMemberList::iterator itEnd = clubMemberList.end();
                                            // Extract the Blaze Id list of club member
                                            for(; it != itEnd; ++it)
                                            {
                                                mMemberIdList.push_back((*it)->getUser().getBlazeId());
                                            }
                                            isGetMemeberList = true;
                                        }

                                        //get the stats periodtype
                                        const Stats::CategoryMap* categoryMap = stats->getConfigData()->getCategoryMap();
                                        const Stats::StatCategory* cat = categoryMap->find(categoryName)->second;
                                        int32_t periodType = Stats::STAT_PERIOD_ALL_TIME;
                                        for (int32_t period = Stats::STAT_PERIOD_ALL_TIME; period < Stats::STAT_NUM_PERIODS; ++period)
                                        {
                                            if (cat->isValidPeriod(period))
                                            {
                                                periodType = period;
                                            }
                                        }
                                        error = resetClubMembersStats(stats, categoryName, periodType, statsList, mMemberIdList);
                                    }
                                }
                                else
                                {
                                    //wipe club stats by this categoryName.
                                    Stats::DeleteStatsRequest req;
                                    Stats::StatDelete* statDelete = req.getStatDeletes().pull_back();
                                    statDelete->setCategory(categoryName);
                                    statDelete->setEntityId(mRequest.getClubId());

                                    error = stats->deleteStats(req);
                                }

                                if (error != Blaze::ERR_OK)
                                {
                                    return commandErrorFromBlazeError(error);
                                }
                            }
                        }
                    }
                    if (!wipeOpcodeExist)
                    {
                        return CLUBS_ERR_WIPE_SET_UNKNOWN;
                    }
                }
                else
                {
                    return ERR_SYSTEM;
                }
                return commandErrorFromBlazeError(Blaze::ERR_OK);
            }

            BlazeRpcError resetClubMembersStats(Stats::StatsSlaveImpl* statsComponent, const char8_t* categoryName, int32_t periodType, const Clubs::StatsList& statsList, const Clubs::BlazeIdList& memberList)
            {
                if( !mComponent->isUsingClubKeyscopedStats() )
                {
                    TRACE_LOG( "Calling resetClubMembersStats when no 'club' keyscope exists. No stats changed." );
                    return Blaze::ERR_OK;
                }

                // Stats setting for update stats
                Stats::UpdateStatsRequest updateStatsRequest;
                Stats::ScopeName scopeName = "club";
                Stats::ScopeValue scopeValue = mRequest.getClubId();
                Stats::ScopeNameValueMap::value_type clubScopePair = Stats::ScopeNameValueMap::value_type(scopeName, scopeValue);

                // Set up Stats Request
                Stats::GetStatsRequest getStatsRequest;
                getStatsRequest.setCategory(categoryName);
                getStatsRequest.setPeriodType(periodType);
                getStatsRequest.getKeyScopeNameValueMap().insert(clubScopePair);

                //insert the blazeid of members
                for(BlazeIdList::const_iterator iter = memberList.begin(); iter != memberList.end(); ++iter)
                {
                    getStatsRequest.getEntityIds().push_back((*iter));
                }

                // Get Club Members' Stats for each category
                Stats::GetStatsResponse getStatsResponse;
                BlazeRpcError error = statsComponent->getStats(getStatsRequest, getStatsResponse);

                if (error != Blaze::ERR_OK)       
                {
                    ERR_LOG("[wipestats_command].resetClubMembersStats() - getStats Failed.");
                    return error;
                }

                // Read from stats respond, extract Club Member Blaze Id based on existing stat row
                Clubs::BlazeIdList updateBlazeIdList;
                const Stats::KeyScopeStatsValueMap &keyscopeStatsValueMap = getStatsResponse.getKeyScopeStatsValueMap();
                if(keyscopeStatsValueMap.size())
                {
                    const Stats::StatValues *statValues = keyscopeStatsValueMap.begin()->second;
                    const Stats::StatValues::EntityStatsList &entityStatsList = statValues->getEntityStatsList();
                    Stats::StatValues::EntityStatsList::const_iterator entityIter = entityStatsList.begin();
                    Stats::StatValues::EntityStatsList::const_iterator entityEnd = entityStatsList.end();
                    for(;entityIter != entityEnd; ++entityIter)
                    {
                        const Stats::EntityStats *entityStats = *entityIter;
                        updateBlazeIdList.push_back(entityStats->getEntityId());
                    }
                }

                // Prepare the update request list for this category for each club member who has existing stat row
                if (!updateBlazeIdList.empty())
                {
                    for (uint32_t updateIndex = 0; updateIndex < updateBlazeIdList.size(); ++updateIndex)
                    {
                        Stats::StatRowUpdate* updateRow = updateStatsRequest.getStatUpdates().pull_back();
                        updateRow->setCategory(categoryName);
                        updateRow->setEntityId(updateBlazeIdList[updateIndex]);
                        updateRow->getKeyScopeNameValueMap().insert(clubScopePair);

                        Clubs::StatsList::const_iterator start = statsList.begin();
                        Clubs::StatsList::const_iterator end = statsList.end();
                        for(; start != end; start++)
                        {
                            Stats::StatUpdate *statUpdate = updateRow->getUpdates().pull_back();    // create in-place
                            statUpdate->setName(start->c_str());
                            statUpdate->setUpdateType(Stats::STAT_UPDATE_TYPE_CLEAR);
                        }
                    }
                }                

                // Update the stats request to Stats Component
                error = statsComponent->updateStats(updateStatsRequest);
                if (error != Blaze::ERR_OK)
                {
                    ERR_LOG("[wipestats_command].resetClubMembersStats() - updateStats Failed.");
                    return error;
                }
                return Blaze::ERR_OK;
            }
        };
        // static factory method impl
        DEFINE_WIPESTATS_CREATE()
    }
}
