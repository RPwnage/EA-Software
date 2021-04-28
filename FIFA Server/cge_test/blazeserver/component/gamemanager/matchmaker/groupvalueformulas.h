/*! ************************************************************************************************/
/*!
\file groupvalueformulas.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GROUPVALUEFORMULAS_H
#define BLAZE_GAMEMANAGER_GROUPVALUEFORMULAS_H

#include "gamemanager/tdf/matchmaker_config_server.h"

#include "framework/tdf/userextendeddatatypes.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/groupuedexpressionlist.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class GroupValueFormulas
    {
    public:
        typedef bool (*CalcGroupFunction)(UserExtendedDataKey, const GroupUedExpressionList&, BlazeId, uint16_t, const GameManager::UserSessionInfoList&, UserExtendedDataValue&);

        static CalcGroupFunction getCalcGroupFunction(GroupValueFormula groupValueFormula)
        {
            switch (groupValueFormula)
            {
            case GROUP_VALUE_FORMULA_AVERAGE:
                return &calcGroupValueAverage;
            case GROUP_VALUE_FORMULA_MIN:
                return &calcGroupValueMin;
            case GROUP_VALUE_FORMULA_MAX:
                return &calcGroupValueMax;
            case GROUP_VALUE_FORMULA_LEADER:
                return &calcGroupValueLeader;
            case GROUP_VALUE_FORMULA_SUM:
                return &calcGroupValueSum;
            default: // GROUP_VALUE_FORMULA_LEADER
                return &calcGroupValueLeader;
            }
        }

        static bool calcGroupValueAverage(UserExtendedDataKey uedKey, const GroupUedExpressionList& groupExpressionList, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersUserInfo, UserExtendedDataValue& uedValue);
        static bool calcGroupValueMax(UserExtendedDataKey uedKey, const GroupUedExpressionList& groupExpressionList, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersUserInfo, UserExtendedDataValue& uedValue);
        static bool calcGroupValueMin(UserExtendedDataKey uedKey, const GroupUedExpressionList& groupExpressionList, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersUserInfo, UserExtendedDataValue& uedValue);
        static bool calcGroupValueLeader(UserExtendedDataKey uedKey, const GroupUedExpressionList& groupExpressionList, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersUserInfo, UserExtendedDataValue& uedValue);
        static bool calcGroupValueSum(UserExtendedDataKey uedKey, const GroupUedExpressionList& groupExpressionList, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersUserInfo, UserExtendedDataValue& uedValue);

        /*! ************************************************************************************************/
        /*! \brief calculate/update a group's UED value, based off the existing UED value and updating member value. Side:
            these are more efficient than CalcGroupFunction methods, avoiding the excess user session infos UED look ups.
            These methods don't use the groupExpressionLists since all of the pre-cached values have already accounted for the boost provided by the group value formulas.
        *************************************************************************************************/
        typedef bool (*UpdateGroupFunction)(uint16_t preExistingGroupSize, UserExtendedDataValue joinOrLeavingMemberUedValue, uint16_t joinOrLeavingMemberSize, bool isLeavingSession, UserExtendedDataValue& uedValue, TeamUEDVector& memberUEDValues);

        static UpdateGroupFunction getUpdateGroupFunction(GroupValueFormula groupValueFormula)
        {
            switch (groupValueFormula)
            {
            case GROUP_VALUE_FORMULA_AVERAGE:
                return &updateGroupValueAverage;
            case GROUP_VALUE_FORMULA_MAX:
                return &updateGroupValueMax;
            case GROUP_VALUE_FORMULA_MIN:
                return &updateGroupValueMin;
            case GROUP_VALUE_FORMULA_SUM:
                return &updateGroupValueSum;
            case GROUP_VALUE_FORMULA_LEADER://Note: deprecated. Not used (in create game finalization)
            default:
                return &updateGroupValueSum;
            }
        }
        static bool updateGroupValueAverage(uint16_t preExistingGroupSize, UserExtendedDataValue joinOrLeavingMemberUedValue, uint16_t joinOrLeavingMemberSize, bool isLeavingSession, UserExtendedDataValue& uedValue, TeamUEDVector& memberUEDValues);
        static bool updateGroupValueMax(uint16_t preExistingGroupSize, UserExtendedDataValue joinOrLeavingMemberUedValue, uint16_t joinOrLeavingMemberSize, bool isLeavingSession, UserExtendedDataValue& uedValue, TeamUEDVector& memberUEDValues);
        static bool updateGroupValueMin(uint16_t preExistingGroupSize, UserExtendedDataValue joinOrLeavingMemberUedValue, uint16_t joinOrLeavingMemberSize, bool isLeavingSession, UserExtendedDataValue& uedValue, TeamUEDVector& memberUEDValues);
        static bool updateGroupValueSum(uint16_t preExistingGroupSize, UserExtendedDataValue joinOrLeavingMemberUedValue, uint16_t joinOrLeavingMemberSize, bool isLeavingSession, UserExtendedDataValue& uedValue, TeamUEDVector& memberUEDValues);

        /*! ************************************************************************************************/
        /*! \brief calculate the desired UED value of a member that would be added, in order to attain a specified
            target value for the group, based on rule's group value formula, and, the new and pre-existing group
            size, and pre-existing group UED value.

            For group formula MAX and MIN, if target would be the new max/min, then the desired joining member's
            value will be equal to target. Otherwise the returned desired value will be the pre-existing max/min.
        *************************************************************************************************/
        typedef bool (*CalcDesiredJoiningMemberFunction)(uint16_t preExistingGroupSize, UserExtendedDataValue preExistingUedValue, UserExtendedDataValue& otherMemberUedValue, uint16_t otherMemberSize, UserExtendedDataValue targetUedValue);

        static CalcDesiredJoiningMemberFunction getCalcDesiredJoiningMemberFunction(GroupValueFormula groupValueFormula)
        {
            switch (groupValueFormula)
            {
            case GROUP_VALUE_FORMULA_AVERAGE:
                return &calcDesiredJoiningMemberValueAverage;
            case GROUP_VALUE_FORMULA_MAX:
                return &calcDesiredJoiningMemberValueMax;
            case GROUP_VALUE_FORMULA_MIN:
                return &calcDesiredJoiningMemberValueMin;
            case GROUP_VALUE_FORMULA_SUM:
                return &calcDesiredJoiningMemberValueSum;
            case GROUP_VALUE_FORMULA_LEADER://Note: deprecated. Not used (in create game finalization)
            default:
                return &calcDesiredJoiningMemberValueSum;
            }
        }
        static bool calcDesiredJoiningMemberValueAverage(uint16_t preExistingGroupSize, UserExtendedDataValue preExistingUedValue, UserExtendedDataValue& otherMemberUedValue, uint16_t otherMemberSize, UserExtendedDataValue targetUedValue);
        static bool calcDesiredJoiningMemberValueMax(uint16_t preExistingGroupSize, UserExtendedDataValue preExistingUedValue, UserExtendedDataValue& otherMemberUedValue, uint16_t otherMemberSize, UserExtendedDataValue targetUedValue);
        static bool calcDesiredJoiningMemberValueMin(uint16_t preExistingGroupSize, UserExtendedDataValue preExistingUedValue, UserExtendedDataValue& otherMemberUedValue, uint16_t otherMemberSize, UserExtendedDataValue targetUedValue);
        static bool calcDesiredJoiningMemberValueSum(uint16_t preExistingGroupSize, UserExtendedDataValue preExistingUedValue, UserExtendedDataValue& otherMemberUedValue, uint16_t otherMemberSize, UserExtendedDataValue targetUedValue);
    private:
        static int64_t getNewGroupSize(uint16_t preExistingGroupSize, uint16_t updatingMemberSize, bool isLeavingSession);
        static bool updateMemberUEDValuesList(TeamUEDVector& memberUEDValues, UserExtendedDataValue joinOrLeavingMemberUedValue, bool isLeavingSession);

        /*! ************************************************************************************************/
        /*!
            \brief getLocalUserData
                Gets the local user data for a given dataKey for the user associated with the
                session in the user extended data.
    
            \param[in] session The user session of the user.
            \param[in] dataKey The key associated with the data in the user extended data.
            \param[in/out] dataValue The Value of that key for this sessions user.
            \return true on successfully fetching the user data.
        *************************************************************************************************/
        static bool getLocalUserData(const GameManager::UserSessionInfo& sessionInfo, UserExtendedDataKey dataKey, UserExtendedDataValue& dataValue);
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GROUPVALUEFORMULAS_H
