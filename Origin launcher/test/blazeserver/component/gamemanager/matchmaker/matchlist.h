/*! ************************************************************************************************/
/*!
    \file   matchlist.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_MATCH_LIST_H
#define BLAZE_MATCHMAKING_MATCH_LIST_H

#include "EASTL/intrusive_list.h"
#include "gamemanager/tdf/matchmaker.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class MatchmakingSession;

    // For CreateGame sessions, we need to track a list of our matched sessions (sorted by totalFitScore)
    // "this matched to other"
    struct MatchedToListNode : public eastl::intrusive_list_node
    {
        GameManager::FitScore sFitScore;
        uint32_t sMatchTimeSeconds;
        uint16_t sOtherSessionSize;
        EA::TDF::tdf_ptr<GameManager::DebugRuleResultMap> sRuleResultMap;
        bool sOtherHasMultiRoleMembers;
    };

    typedef eastl::intrusive_list<MatchedToListNode> MatchedToList;

    // sort by descending fit score
    struct MatchedToListCompare
    {
        bool operator()(MatchedToListNode &a, MatchedToListNode &b) const
        {
            return (a.sFitScore > b.sFitScore);
        }
    };

    // sort by descending session size, then by fit score.
    struct MatchedToListTeamsCompare
    {
        bool operator()(MatchedToListNode &a, MatchedToListNode &b) const
        {
            if (a.sOtherSessionSize > b.sOtherSessionSize)
                return true;
            else if (a.sOtherSessionSize == b.sOtherSessionSize)
                return (a.sFitScore > b.sFitScore);
            else
                return false;
        }
    };

    // sort by those accepting fewer roles, then descending session size, then by fit score.
    struct MatchedToListTeamsMultiRolesCompare
    {
        bool operator()(MatchedToListNode &a, MatchedToListNode &b) const
        {
            if (!a.sOtherHasMultiRoleMembers && b.sOtherHasMultiRoleMembers)
                return true;
            else if (a.sOtherHasMultiRoleMembers && !b.sOtherHasMultiRoleMembers)
                return false;
            else
            {
                if (a.sOtherSessionSize > b.sOtherSessionSize)
                    return true;
                else if (a.sOtherSessionSize < b.sOtherSessionSize)
                    return false;
                else
                    return (a.sFitScore > b.sFitScore);
            }
        }
    };


    struct MatchedFromListNode 
        : public eastl::intrusive_list_node
    {
    };

    // Each Session owns it's To Info and has a reference to the matched sessions from info.
    struct MatchmakingSessionMatchNode  : public MatchedToListNode, public MatchedFromListNode
    {
        const MatchmakingSession *sMatchedToSession;
        MatchmakingSession *sMatchedFromSession;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_MATCH_LIST_H
