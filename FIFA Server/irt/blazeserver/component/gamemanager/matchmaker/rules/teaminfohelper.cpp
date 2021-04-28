/*! ************************************************************************************************/
/*!
    \file teaminfohelper.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/teaminfohelper.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerroster.h"
#include "EASTL/sort.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief Constructor - starts by tracking team info for the finalizing session.  Other matched
        sessions will need to call tallyTeamInfo at a later time.
    ***************************************************************************************************/
    MatchingTeamInfoValidator::MatchingTeamInfoValidator(const MatchmakingSession& finalizingSession)
        : mTeamInfoMap(BlazeStlAllocator("mTeamInfoMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mFinalizingCriteria(finalizingSession.getCriteria())
    {
        tallyTeamInfo(finalizingSession);
    }

    void MatchingTeamInfoValidator::tallyTeamInfo(const MatchmakingSession& session)
    {
        // Overall container to track total number of players requesting each team.
        const TeamIdSizeList* myTeamIds = session.getCriteria().getTeamIdSizeListFromRule();
        if (myTeamIds == nullptr)
            return;

        TeamIdSizeList::const_iterator teamIdSize = myTeamIds->begin();
        TeamIdSizeList::const_iterator teamIdSizeEnd = myTeamIds->end();
        for (; teamIdSize != teamIdSizeEnd; ++teamIdSize)
        {
            TeamId myTeamId = teamIdSize->first;
            uint16_t myTeamJoinCount = teamIdSize->second;
            mTeamInfoMap[myTeamId] += myTeamJoinCount;
        }
    }


    bool MatchingTeamInfoValidator::hasEnoughPlayers(const TeamId teamId, const uint16_t minPlayerCount)
    {
        if (minPlayerCount == 0)
        {
            return true;
        }

        // Find the number of any players
        uint16_t numAnyPlayers = 0;
        MatchingTeamInfoMap::iterator anyIter = mTeamInfoMap.find(ANY_TEAM_ID);
        if (anyIter != mTeamInfoMap.end())
        {
            numAnyPlayers = anyIter->second;
        }

        MatchingTeamInfoMap::iterator matchIter = mTeamInfoMap.find(teamId);
        // We have people finalizing who want this specific team id.
        if (matchIter != mTeamInfoMap.end())
        {
            TRACE_LOG("[MatchingTeamInfoValidator] Found team " << teamId << " with " << matchIter->second << " players available.");
            if (minPlayerCount <= matchIter->second)
            {
                // we have enough players that directly want this team.
                return true;
            }
            else
            {
                // otherwise see if we have some any's that could fill this team.
                if (minPlayerCount <= ( matchIter->second + numAnyPlayers))
                {
                    // update the number of any players left
                    uint16_t numAnyPlayersUsed = minPlayerCount - matchIter->second;
                    if (anyIter != mTeamInfoMap.end())
                    {
                        if (anyIter->second > numAnyPlayersUsed)
                        {
                            anyIter->second -= numAnyPlayersUsed;
                        }
                        else
                        {
                            anyIter->second = 0;
                        }
                    }
                    return true;
                }
                else if (teamId == ANY_TEAM_ID)
                {
                    // Assertion: the logical complexity of trying to decide who goes where is not worth
                    // it here.  We can just go ahead and try to finalize.
                    return true;
                }
            }
        }
        else
        {
            TRACE_LOG("[MatchingTeamInfoValidator] Did not find team " << teamId << " with " << numAnyPlayers << " ANY players available.");
            // my specific team was not found, if I can land on any all good.
            // Assertion: the logical complexity of trying to decide who goes where is not worth
            // it here.  We can just go ahead and try to finalize.
            if ((minPlayerCount <= numAnyPlayers) || (teamId == ANY_TEAM_ID))
            {
                return true;
            }
        }

        // We were unable to quickly tell that we'd have enough players for a team.
        TRACE_LOG("[MatchingTeamInfoValidator] Unable to find " << minPlayerCount << " players for team (" << teamId << "). Num any players (" << numAnyPlayers << ")");
        return false;
    }



} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
