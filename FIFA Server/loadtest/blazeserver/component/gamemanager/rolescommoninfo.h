/*! ************************************************************************************************/
/*!
\file   rolescommoninfo.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_GAMEMANAGER_ROLES_COMMON_INFO_H
#define BLAZE_GAMEMANAGER_ROLES_COMMON_INFO_H

#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/gamemanager_server.h"

#include "EASTL/vector_map.h"
#include "EASTL/hash_map.h"
#include "EASTL/map.h"

namespace Blaze
{
namespace GameManager
{
    typedef eastl::vector<RoleName> PlayerRoleList; // Even though we had a TDF vector, RoleNameList, we use this because
                                                    // otherwise we'd need to allocate the list on the heap as TdfPrimitiveVector's assignment operator is private
    typedef eastl::vector_map<RoleName, uint16_t, CaseInsensitiveStringLessThan> RoleSizeMap;
    typedef eastl::hash_map<PlayerRoleList, uint32_t> RolesSizeMap;
    typedef eastl::vector_map<PlayerId, PlayerRoleList> PlayerToRoleNameMap;
    typedef eastl::map<TeamId, PlayerToRoleNameMap> TeamIdPlayerRoleNameMap;
    typedef eastl::map<TeamIndex, RoleSizeMap> TeamIndexRoleSizeMap;
    typedef eastl::map<TeamId, RoleSizeMap> TeamIdRoleSizeMap;

    typedef eastl::pair<TeamId, uint16_t> TeamIdSize;
    struct TeamIdSizeSortComparitor
    {
        bool operator()(const TeamIdSize &a, const TeamIdSize &b) const
        {
            return (a.second > b.second);
        }
    };
    typedef eastl::vector<TeamIdSize> TeamIdSizeList;

    template <typename T>
    using UserRoleCount = typename eastl::pair<T, size_t>;
    template <typename T>
    using UserRoleCounts = typename eastl::vector<UserRoleCount<T> >;
    template <typename T>
    struct UserRoleCountComparator
    {
        bool operator()(const UserRoleCount<T>& a, const UserRoleCount<T>& b)
        {
            return (a.second < b.second);
        }
    };

    uint16_t lookupRoleCount(const RoleNameToPlayerMap& roleRosters, const RoleName& name, uint16_t numJoiningPlayers);

    const RoleName* lookupPlayerRoleName(const RoleNameToPlayerMap& roleRosters, PlayerId playerId);

    const char8_t* lookupPlayerRoleName(const PlayerJoinData& playerJoinData, PlayerId playerId);

    const PerPlayerJoinData* lookupPerPlayerJoinDataConst(const PlayerJoinData& playerJoinData, const UserInfoData& userInfoData);

    PerPlayerJoinData* lookupPerPlayerJoinData(PlayerJoinData& playerJoinData, const UserInfoData& userInfoData);

    const char8_t* lookupExternalPlayerRoleName(const PlayerJoinData& playerJoinData,
        const UserInfoData& playerUserInfo);

    /*! \brief Retrieve user's role from the request's external or non-external players role roster depending on whether its an external user */
    const char8_t* lookupPlayerRoleName(const PlayerJoinData &playerJoinData, const UserSessionInfo &joiningUserInfo);

    void buildSortedTeamIdSizeList(const TeamIdRoleSizeMap &teamRoleSizes, TeamIdSizeList& sortedTeamList);

    SlotType getSlotType(const PlayerJoinData& playerJoinData, SlotType memberSlotType);
    SlotType getSlotType(const PlayerJoinData& playerJoinData, const UserInfoData& userInfoData);


    // note: for non-MM joins optional players are never required, so includeOptionalPlayers should be set to false.
    // For MM joins, their roles are required, so includeOptionalPlayers should be used. 
    void buildTeamIdRoleSizeMap(const PlayerJoinData &playerJoinData, TeamIdRoleSizeMap &teamRoleRequirements, uint16_t joiningPlayerCount, bool includeOptionalPlayers);

    // Build a map tracking which roles each player requires. Used by the roles rule to determine our RETE conditions
    void buildTeamIdPlayerRoleMap(const PlayerJoinData &playerJoinData, TeamIdPlayerRoleNameMap &teamRoleRequirements, uint16_t joiningPlayerCount, bool includeOptionalPlayers);

    void buildTeamIndexRoleSizeMap(const PlayerJoinData &playerJoinData, TeamIndexRoleSizeMap &teamRoleRequirements, uint16_t joiningPlayerCount, bool includeOptionalPlayers);

    /*! ************************************************************************************************/
    /*! \brief if a role join roster is empty, this will fill it out with the default role setup.
          Single RoleName, "" and one entry for BlazeId, INVALID_BLAZE_ID

        \param[in\out] roleRosters - the joining role roster to set up
    ***************************************************************************************************/
    void setUpDefaultJoiningRoles(RoleNameToPlayerMap &roleRosters);

    /*! ************************************************************************************************/
    /*! \brief if a role join roster is empty, this will fill it out with the default role setup.
          Single RoleName, "" with capacity equaling the provided teamCapacity

        \param[in\out] roleInformation - the RoleInformation to set up if uninitalized
        \param[in] teamCapacity - the size of teams in the game session
    ***************************************************************************************************/
    void setUpDefaultRoleInformation(RoleInformation &roleInformation, uint16_t teamCapacity);

    /*! ************************************************************************************************/
    /*! \brief if the user with the specified id has its role specified in the input role join roster,
            copy/add its entry to the other roster.
    ***************************************************************************************************/
    void copyPlayerRoleRosterEntry(BlazeId playerId, const RoleNameToPlayerMap& orig, PlayerJoinData& playerJoinData);

    void copyPlayerJoinDataEntry(const UserInfoData& userInfo, const PlayerJoinData& origData, PlayerJoinData& newJoinData);



    /*! ************************************************************************************************/
    /*! \brief if the user with the specified id has its role specified in the external join roster,
            copy/add its entry to the non-external role roster. Used to track a user originally specified
            in an external players list, in the same maps as 'regular player' for all role roster cap/criteria checks.
        \return pointer to the join roster player's entry got added to. nullptr, if it had no role found in original roster.
    ***************************************************************************************************/
    PerPlayerJoinData* externalRoleRosterEntryToNonExternalRoster(const UserInfoData& userInfo, PlayerJoinData& playerJoinData);

    /*! ************************************************************************************************/
    /*! \brief Ensures that the role information present in the join request is valid and
            updates the role information for use internally
        \return error indicating if validation was successful
    ***************************************************************************************************/
    BlazeRpcError fixupPlayerJoinDataRoles(PlayerJoinData& playerJoinData, bool singleRoleRequired, int32_t logCategory, EntryCriteriaError* errorMsg = nullptr);

} // namespace GameManager
} // namespace Blaze

#endif //BLAZE_GAMEMANAGER_ROLES_COMMON_INFO_H
