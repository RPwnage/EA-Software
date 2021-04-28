/*! ************************************************************************************************/
/*!
\file   commoninfo.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_GAMEMANAGER_COMMON_INFO_H
#define BLAZE_GAMEMANAGER_COMMON_INFO_H

#include "gamemanager/playerinfo.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "framework/usersessions/usersessionmanager.h" // used by BestNetworkHostComparitor
#include "framework/tdf/qosdatatypesnetwork.h" // for NetworkQosData
#include "gamemanager/rolescommoninfo.h" // for lookupPerPlayerJoinDataConst
#include "framework/util/networktopologycommoninfo.h"

#include "EASTL/vector.h"

namespace Blaze
{
namespace GameManager
{


    /*! ************************************************************************************************/
    /*! \brief list sort comparison structure. Returns true if n1 < n2 (by comparing the NAT of 2 players),
        if NAT type of 2 matchmaking sessions are same, return true if n1 > n2 (by comparing the upstream bandwidth of 2 sessions).

        Used when sorting the matchmaking session or /playerInfo to get an ordered container with the best host candidate at the top

        Note: we prefer to use a structure in list sort function, this way we can avoid using a loose function
    ***************************************************************************************************/
    struct BestNetworkHostComparitor
    {
        bool operator()(const PlayerInfo* p1, const PlayerInfo* p2) const
        {
            // Choose the match's stateful/non-guest users, as guests can't do many Game RPCs, and other stateless users are typically temporary
            if (UserSessionManager::isStatelessUser(p2 != nullptr && p2->getUserInfo().getUserInfo().getId()))
                return true;
            if (UserSessionManager::isStatelessUser(p1 != nullptr && p1->getUserInfo().getUserInfo().getId()))
                return false;

            const Util::NetworkQosData* n1 = (p1 != nullptr && p1->hasHostPermission()) ? &p1->getQosData() : nullptr;
            const Util::NetworkQosData* n2 = (p2 != nullptr && p2->hasHostPermission()) ? &p2->getQosData() : nullptr;
            // IMPORTANT: We *must* set n2IsHost = true when we don't have host info of n2 because otherwise 
            // if n1 and n2 have the same NAT type and upstream BPS, we get: 
            //      compareNetworkQosData(n1, n2, false) == compareNetworkQosData(n2, n1, false) == true,
            // which is obviously not allowed for a 'strictly less than' operator.
            // Caught by an assertion in eastl::sort() which validates comparators.
            return compareNetworkQosData(n1, n2, true);
        }

        bool operator()(const Matchmaker::MatchmakingSession::MemberInfoListNode& m1, 
            const Matchmaker::MatchmakingSession::MemberInfoListNode& m2) const
        {
            // Choose the match's stateful/non-guest users, as guests can't do many Game RPCs, and other stateless users are typically temporary
            const GameManager::UserSessionInfo &s1 = static_cast<const Matchmaker::MatchmakingSession::MemberInfo &>(m1).mUserSessionInfo;
            const GameManager::UserSessionInfo &s2 = static_cast<const Matchmaker::MatchmakingSession::MemberInfo &>(m2).mUserSessionInfo;
            if (UserSessionManager::isStatelessUser(s2.getUserInfo().getId()))
                return true;
            if (UserSessionManager::isStatelessUser(s1.getUserInfo().getId()))
                return false;

            // If either is optional, no need to compare network data below. Return true if 2nd is, false if just 1st is.
            if (static_cast<const Matchmaker::MatchmakingSession::MemberInfo &>(m2).mIsOptionalPlayer)
                return true;
            if (static_cast<const Matchmaker::MatchmakingSession::MemberInfo &>(m1).mIsOptionalPlayer)
                return false;

            const Util::NetworkQosData* n1 = s1.getHasHostPermission() ? &s1.getQosData() : nullptr;
            const Util::NetworkQosData* n2 = s2.getHasHostPermission() ? &s2.getQosData() : nullptr;
            return compareNetworkQosData(n1, n2, static_cast<const Matchmaker::MatchmakingSession::MemberInfo &>(m2).isMMSessionOwner());
        }
        
        /*! ************************************************************************************************/
        /*! \brief structure operator used by list sort function.

            In this logic we give preference to the NAT type.  The player with the best NAT type wins.
            If the NAT type is equal, we drop to deciding on upstream bandwidth.
            If bandwidth happens to be equal (most likely because of debug overrides), we give ties to the 
            first player UNLESS the second player happens to be the host, in which case they get preference.
            Giving host preference really doesn't make much of a difference in real world applications, but
            it allows us to have a predictable behavior for testing purposes.

            \param[in] n1 first matchmaking session to compare
            \param[in] n2 second matchmaking session to compare
            \returns true if n1 < n2 (by comparing the NAT of 2 matchmaking sessions), if NAT type of 2 sessions are same,
            return true if n1 > n2 (by comparing the upstream bandwidth of 2 sessions).
        ***************************************************************************************************/
        static bool compareNetworkQosData(const Util::NetworkQosData* n1,
            const Util::NetworkQosData* n2, bool n2IsHost) 
        {
            if (n1 == nullptr)
                return false;

            if (n2 == nullptr)
                return true;

            return (n1->getNatType() == n2->getNatType()) ? //Tied on NAT?
                   ((n1->getUpstreamBitsPerSecond() == n2->getUpstreamBitsPerSecond()) ?  //Tied on bandwidth
                    !n2IsHost : (n1->getUpstreamBitsPerSecond() > n2->getUpstreamBitsPerSecond())) : //N1 wins on a tie unless N2 is the host
                    (n1->getNatType() < n2->getNatType()); //If NAT not tied, decide on that
        }
    };

    inline uint16_t getTotalParticipantCapacity(const SlotCapacitiesVector& slotCapacities)
    {
        uint16_t capacity = 0;
        for (size_t slot = 0; slot < (uint16_t)MAX_PARTICIPANT_SLOT_TYPE; ++slot)
        {
            capacity += slotCapacities[slot];
        }
        return capacity;
    }

    inline uint16_t getTotalParticipantCapacity(const SlotCapacitiesMap& slotCapacities)
    {
        uint16_t capacity = 0;
        for (size_t slot = 0; slot < (uint16_t)MAX_PARTICIPANT_SLOT_TYPE; ++slot)
        {
            const auto& itr = slotCapacities.find((SlotType)slot);
            capacity += (itr != slotCapacities.end() ? itr->second : 0);
        }
        return capacity;
    }

    inline uint16_t getSlotCapacity(const SlotCapacitiesMap& slotCapacities, SlotType slotType = SLOT_PUBLIC_PARTICIPANT)
    {
        const auto& itr = slotCapacities.find(slotType);
        uint16_t capacity = (itr != slotCapacities.end() ? itr->second : 0);
        
        return capacity;
    }


    inline const char8_t* getEncryptedBlazeId(const PlayerJoinData& playerJoinData, const UserInfoData& userInfoData)
    {
        const PerPlayerJoinData* playerData = lookupPerPlayerJoinDataConst(playerJoinData, userInfoData);
        return (playerData != nullptr ? playerData->getEncryptedBlazeId() : "");
    }

    inline TeamIndex getTeamIndex(const PlayerJoinData& playerJoinData, TeamIndex memberTeamIndex)
    {
        return (memberTeamIndex == UNSPECIFIED_TEAM_INDEX ? playerJoinData.getDefaultTeamIndex() : memberTeamIndex);
    }

    inline TeamIndex getTeamIndex(const PlayerJoinData& playerJoinData, const UserInfoData& userInfoData)
    {
        const PerPlayerJoinData* playerData = lookupPerPlayerJoinDataConst(playerJoinData, userInfoData);
        return getTeamIndex(playerJoinData, playerData ? playerData->getTeamIndex() : UNSPECIFIED_TEAM_INDEX);
    }


    inline void setUnspecifiedTeamIndex(PlayerJoinData& playerJoinData, TeamIndex assignedTeamIndex)
    {
        if (assignedTeamIndex != UNSPECIFIED_TEAM_INDEX)
        {
            if (playerJoinData.getDefaultTeamIndex() == UNSPECIFIED_TEAM_INDEX)
            {
                playerJoinData.setDefaultTeamIndex(assignedTeamIndex);
            }
        
            PerPlayerJoinDataList::iterator playerIter = playerJoinData.getPlayerDataList().begin();
            PerPlayerJoinDataList::iterator playerEnd = playerJoinData.getPlayerDataList().end();
            for (; playerIter != playerEnd; ++playerIter)
            {
                if ((*playerIter)->getTeamIndex() == UNSPECIFIED_TEAM_INDEX)
                {
                    (*playerIter)->setTeamIndex(assignedTeamIndex);
                }
            }
        }
    }



    // returns true if rest of the connection group of a player will get kicked as a result of his removal reason
    inline bool removalKicksConngroup(PlayerRemovedReason playerRemovedReason)
    {
        // A list of error codes that will result in kicking out rest of the connection group for sure. Some error codes like BLAZESERVER_CONN_LOST have been omitted as they do not
        // ALWAYS mean that entire connection group should be kicked. For example, BLAZESERVER_CONN_LOST can happen as a result of the logout. 
        return ( playerRemovedReason == PLAYER_KICKED
            || playerRemovedReason == PLAYER_KICKED_WITH_BAN 
            || playerRemovedReason == PLAYER_KICKED_CONN_UNRESPONSIVE 
            || playerRemovedReason == PLAYER_KICKED_CONN_UNRESPONSIVE_WITH_BAN
            || playerRemovedReason == PLAYER_KICKED_POOR_CONNECTION 
            || playerRemovedReason == PLAYER_KICKED_POOR_CONNECTION_WITH_BAN
            );
    }

    inline bool isRemovalReasonABan(PlayerRemovedReason playerRemovedReason)
    {
        return ( playerRemovedReason == PLAYER_KICKED_WITH_BAN
            || playerRemovedReason == PLAYER_KICKED_CONN_UNRESPONSIVE_WITH_BAN
            || playerRemovedReason == PLAYER_KICKED_POOR_CONNECTION_WITH_BAN
            );
    }

} // namespace GameManager
} // namespace Blaze

#endif //BLAZE_GAMEMANAGER_COMMON_INFO_H
