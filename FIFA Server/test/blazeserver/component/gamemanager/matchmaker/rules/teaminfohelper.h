/*! ************************************************************************************************/
/*!
    \file teaminfohelper.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_INFO_HELPER_H
#define BLAZE_MATCHMAKING_TEAM_INFO_HELPER_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/matchlist.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class MatchmakingCriteria;
    // Helper value that makes it more obvious how we're using the INVALID_TEAM_ID here to represent teams without a set id. 
    const uint16_t UNSET_TEAM_ID = INVALID_TEAM_ID;


    /*! ************************************************************************************************/
    /*! \brief Helper class to track which matched sessions want which teams, and if the players will fit.
    ***************************************************************************************************/
    class MatchingTeamInfoValidator
    {
         NON_COPYABLE(MatchingTeamInfoValidator)
    public:
        MatchingTeamInfoValidator(const MatchmakingSession& finalizingSession);

        void tallyTeamInfo(const MatchmakingSession& session);
        
        bool hasEnoughPlayers(const TeamId teamId, const uint16_t minPlayerCount);

    private: 

        typedef eastl::hash_map<TeamId, uint16_t> MatchingTeamInfoMap;
        MatchingTeamInfoMap mTeamInfoMap;

        const MatchmakingCriteria& mFinalizingCriteria;
    };



} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_INFO_HELPER_H
