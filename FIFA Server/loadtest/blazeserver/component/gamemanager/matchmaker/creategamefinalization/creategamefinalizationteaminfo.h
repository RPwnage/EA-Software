/*! ************************************************************************************************/
/*!
    \file   creategamefinalizationteaminfo.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_CREATE_GAME_FINALIZATION_TEAM_INFO
#define BLAZE_CREATE_GAME_FINALIZATION_TEAM_INFO

#include "gamemanager/matchmaker/creategamefinalization/matchedsessionsbucket.h"
#include "gamemanager/rolescommoninfo.h"
#include "gamemanager/matchmaker/teamcompositionscommoninfo.h"

namespace Blaze
{

namespace GameManager
{

namespace Matchmaker
{
    class MatchmakingSession;
    class CreateGameFinalizationSettings;
    class TeamCompositionFilledInfo;

    // Team size rule's needs to be optimized such that the team is built
    // as we find sessions.  When we get to finalization we should already
    // have picked our team.
    class CreateGameFinalizationTeamInfo
    {
    public:
        CreateGameFinalizationTeamInfo(CreateGameFinalizationSettings* p, TeamId teamId, TeamIndex teamIndex, uint16_t teamCapacity, UserExtendedDataValue initialUedValue);

        bool canPotentiallyJoinTeam(const MatchmakingSession& session, bool failureAsError, TeamId joiningTeamId) const;
        bool joinTeam(const MatchmakingSession& session, TeamId joiningTeamId);
        // validates that a joining MM session won't take too much space with their role requirements
        // returns true if the join won't violate the provided roleInformation's limits
        bool checkJoiningRoles(const RoleSizeMap& joiningRoleSizeMap) const;
        bool checkJoiningTeamCompositions(const GameTeamCompositionsInfoVector* otherAcceptableGtcInfoVector, uint16_t sessionSize, const MatchmakingSession& session) const;


        // increments the roleSizeMap based on the joiningRoleSizeMap provided
        void addSessionToTeam(const MatchmakingSession& addedSession, TeamId addedTeamId);
        // decrements the roleSizeMap based on the removingRoleSizeMap provided
        void removeSessionFromTeam(const MatchmakingSession& removedSession, TeamId removedTeamId);

        // checks if session/bucket already 'tried' for the current pick of the finalization. Tracked per team.
        bool wasSessionTriedForCurrentPick(MatchmakingSessionId mmSessionId, uint16_t pickNumber) const;
        bool wasBucketTriedForCurrentPick(MatchedSessionsBucketingKey bucketKey, uint16_t pickNumber) const;
        void markSessionTriedForCurrentPick(MatchmakingSessionId mmSessionId, uint16_t pickNumber);
        void markBucketTriedForCurrentPick(MatchedSessionsBucketingKey bucketKey, uint16_t pickNumber);
        void clearSessionsTried(uint16_t abovePickNumber);
        void clearBucketsTried(uint16_t abovePickNumber);

        void setTeamCompositionToFill(TeamCompositionFilledInfo* teamCompositionToFill);
        TeamCompositionFilledInfo* getTeamCompositionToFill() const { return mTeamCompositionToFill; }
        bool markTeamCompositionToFillGroupSizeFilled(uint16_t groupSize);
        void clearTeamCompositionToFillGroupSizeFilled(uint16_t groupSize);

        bool isFull() const { return (mTeamSize >= mTeamCapacity); }

        UserExtendedDataValue getRankedUedValue(uint16_t rank) const;

        const char8_t* toLogStr() const;

        CreateGameFinalizationSettings* mParent;
        TeamId mTeamId;
        TeamIndex mTeamIndex;
        uint16_t mTeamCapacity;
        uint16_t mTeamSize;

        UserExtendedDataValue mUedValue;
        //for max, min group formulas, these are the ued values of each MM session joining this team
        TeamUEDVector mMatchmakingSessionUedValues;
        //individual UED values for all team members, used by TeamUEDPositionParity verification
        TeamUEDVector mMemberUedValues;

    private:

        // increments the roleSizeMap based on the joiningRoleSizeMap provided
        void addSessionRolesToTeam(const RoleSizeMap& joiningRoleSizeMap);
        // decrements the roleSizeMap based on the removingRoleSizeMap provided
        void removeSessionRolesFromTeam(const RoleSizeMap& removingRoleSizeMap);

        RoleSizeMap mRoleSizeMap;
        bool mTeamHasMultiRoleMembers;

        typedef eastl::vector<MatchmakingSessionId> MatchmakingSessionIdList;
        typedef eastl::hash_map<uint16_t, MatchmakingSessionIdList> PickNumberToMmSessionIdsMap;
        PickNumberToMmSessionIdsMap mPickNumberToMmSessionIdsTriedMap;

        typedef eastl::vector<MatchedSessionsBucketingKey> BucketKeyList;
        typedef eastl::hash_map<uint16_t, BucketKeyList> PickNumberToBucketKeyListMap;
        PickNumberToBucketKeyListMap mPickNumberToBucketsTriedMap;
        mutable eastl::string mLogBuf;

        TeamCompositionFilledInfo* mTeamCompositionToFill;//nullptr if team compositions disabled
    };

    typedef eastl::vector<CreateGameFinalizationTeamInfo> CreateGameFinalizationTeamInfoVector;

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_CREATE_GAME_FINALIZATION_TEAM_INFO
