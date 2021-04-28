/*! ************************************************************************************************/
/*!
\file groupvalueformulas.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "groupvalueformulas.h"

#include "gamemanager/matchmaker/matchmakingutil.h"

#include "EASTL/algorithm.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    bool GroupValueFormulas::calcGroupValueAverage(UserExtendedDataKey uedKey, const GroupUedExpressionList& groupExpressionList, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersUserInfo, UserExtendedDataValue& uedValue)
{
        UserExtendedDataValue totalPlayersUEDValue = 0;
        uint32_t numPlayers = 0;

        for (GameManager::UserSessionInfoList::const_iterator itr = membersUserInfo.begin(), end = membersUserInfo.end(); itr != end; ++itr)
        {
            UserExtendedDataValue dataValue = 0;
            if (MatchmakingUtil::getLocalUserData(**itr, uedKey, dataValue))
            {
                
                // look up group size
                uint16_t groupSize = (*itr)->getGroupSize();
                // default to ownerGroupSize if unset, if it is unset, we're calling on behalf of the group owner.
                if (groupSize == 0)
                {
                    groupSize = ownerGroupSize;
                }

                dataValue = groupExpressionList.calculateAdjustedUedValue(dataValue, groupSize);
                totalPlayersUEDValue += dataValue;
                numPlayers++;
            }
        }

        if (numPlayers > 0)
        {
            uedValue = static_cast<UserExtendedDataValue>(totalPlayersUEDValue / numPlayers);
            return true;
        }

        return false;
    }

    bool GroupValueFormulas::calcGroupValueMax(UserExtendedDataKey uedKey, const GroupUedExpressionList& groupExpressionList, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersUserInfo, UserExtendedDataValue& uedValue)
{
        bool success = false;
        UserExtendedDataValue maxValue = INT64_MIN;

        for (GameManager::UserSessionInfoList::const_iterator itr = membersUserInfo.begin(), end = membersUserInfo.end(); itr != end; ++itr)
        {
            UserExtendedDataValue dataValue = INT64_MIN;
            if (MatchmakingUtil::getLocalUserData(**itr, uedKey, dataValue))
            {
                // look up group size
                uint16_t groupSize = (*itr)->getGroupSize();
                // default to ownerGroupSize if unset, if it is unset, we're calling on behalf of the group owner.
                if (groupSize == 0)
                {
                    groupSize = ownerGroupSize;
                }

                dataValue = groupExpressionList.calculateAdjustedUedValue(dataValue, groupSize);
                maxValue = eastl::max(maxValue, dataValue);
                success = true;
            }           
        }

        if (success)
        {
            uedValue = maxValue;
        }

        return success;
    }

    bool GroupValueFormulas::calcGroupValueMin(UserExtendedDataKey uedKey, const GroupUedExpressionList& groupExpressionList, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersUserInfo, UserExtendedDataValue& uedValue)
{
        bool success = false;
        UserExtendedDataValue minValue = INT64_MAX;

        for (GameManager::UserSessionInfoList::const_iterator itr = membersUserInfo.begin(), end = membersUserInfo.end(); itr != end; ++itr)
        {
            UserExtendedDataValue dataValue = INT64_MAX;
            if (MatchmakingUtil::getLocalUserData(**itr, uedKey, dataValue))
            {
                // look up group size
                uint16_t groupSize = (*itr)->getGroupSize();
                // default to ownerGroupSize if unset, if it is unset, we're calling on behalf of the group owner.
                if (groupSize == 0)
                {
                    groupSize = ownerGroupSize;
                }
                
                dataValue = groupExpressionList.calculateAdjustedUedValue(dataValue, groupSize);
                minValue = eastl::min(minValue, dataValue);
                success = true;
            }            
        }

        if (success)
        {
            uedValue = minValue;
        }

        return success;
    }

    bool GroupValueFormulas::calcGroupValueLeader(UserExtendedDataKey uedKey, const GroupUedExpressionList& groupExpressionList, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersUserInfo, UserExtendedDataValue& uedValue)
{
        bool success = false;
        for (GameManager::UserSessionInfoList::const_iterator itr = membersUserInfo.begin(), end = membersUserInfo.end(); itr != end; ++itr)
        {
            if ((*itr)->getUserInfo().getId() == ownerBlazeId)
            {
                if (MatchmakingUtil::getLocalUserData(**itr, uedKey, uedValue))
                {
                    uedValue = groupExpressionList.calculateAdjustedUedValue(uedValue, ownerGroupSize);
                    success = true;
                }

                break;
            }            
        }

        if (!success)
        {
            TRACE_LOG("[GroupValueFormulas].calcGroupValueLeader WARN UED Value for owner session not found, skipping player.");
        }

        return success;
    }

    bool GroupValueFormulas::calcGroupValueSum(UserExtendedDataKey uedKey, const GroupUedExpressionList& groupExpressionList, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersUserInfo, UserExtendedDataValue& uedValue)
{
        UserExtendedDataValue totalPlayersUEDValue = 0;
        bool success = false;

        for (GameManager::UserSessionInfoList::const_iterator itr = membersUserInfo.begin(), end = membersUserInfo.end(); itr != end; ++itr)
        {
            UserExtendedDataValue dataValue = 0;
            if (MatchmakingUtil::getLocalUserData(**itr, uedKey, dataValue))
            {
                // look up group size
                uint16_t groupSize = (*itr)->getGroupSize();
                // default to ownerGroupSize if unset, if it is unset, we're calling on behalf of the group owner.
                if (groupSize == 0)
                {
                    groupSize = ownerGroupSize;
                }

                dataValue = groupExpressionList.calculateAdjustedUedValue(dataValue, groupSize);
                totalPlayersUEDValue += dataValue;
                success = true;
            }
        }

        if (success)
        {
            uedValue = totalPlayersUEDValue;
        }

        return success;
    }

    /*! ************************************************************************************************/
    /*! \brief calculate/update a group's UED value, based off the existing UED value and updating member value.
    *************************************************************************************************/
    bool GroupValueFormulas::updateGroupValueAverage(uint16_t preExistingGroupSize, UserExtendedDataValue joinOrLeavingMemberUedValue, uint16_t joinOrLeavingMemberSize, bool isLeavingSession, UserExtendedDataValue& uedValue, TeamUEDVector& memberUEDValues)
    {
        // no op in case we're called on a joining session of size 0
        if (joinOrLeavingMemberSize == 0)
            return true;

        if (!updateMemberUEDValuesList(memberUEDValues, joinOrLeavingMemberUedValue, isLeavingSession))
            return false;

        int64_t totalSize = getNewGroupSize(preExistingGroupSize, joinOrLeavingMemberSize, isLeavingSession);
        if (totalSize < 0)
            return false;//logged

        // get the sum of all members
        UserExtendedDataValue sumAddingUEDValues = ((isLeavingSession? -1:1) * (joinOrLeavingMemberUedValue * joinOrLeavingMemberSize));
        UserExtendedDataValue sumPreExistingUEDValues = ((preExistingGroupSize == 0)? 0 : (uedValue * preExistingGroupSize));

        UserExtendedDataValue newSumUEDValues = sumAddingUEDValues + sumPreExistingUEDValues;
        uedValue = ((totalSize == 0)? 0 : (newSumUEDValues / totalSize));
        return true;
    }

    bool GroupValueFormulas::updateGroupValueMax(uint16_t preExistingGroupSize, UserExtendedDataValue joinOrLeavingMemberUedValue, uint16_t joinOrLeavingMemberSize, bool isLeavingSession, UserExtendedDataValue& uedValue, TeamUEDVector& memberUEDValues)
    {
        if (joinOrLeavingMemberSize == 0)
            return true;

        // update member UED list and resort
        if (!updateMemberUEDValuesList(memberUEDValues, joinOrLeavingMemberUedValue, isLeavingSession))
        {
            return false;
        }
        uedValue = (memberUEDValues.empty()? 0 : memberUEDValues.back());
        return true;
    }

    bool GroupValueFormulas::updateGroupValueMin(uint16_t preExistingGroupSize, UserExtendedDataValue joinOrLeavingMemberUedValue, uint16_t joinOrLeavingMemberSize, bool isLeavingSession, UserExtendedDataValue& uedValue, TeamUEDVector& memberUEDValues)
    {
        if (joinOrLeavingMemberSize == 0)
            return true;

        // update member UED list and resort
        if (!updateMemberUEDValuesList(memberUEDValues, joinOrLeavingMemberUedValue, isLeavingSession))
        {
            return false;
        }
        uedValue = (memberUEDValues.empty()? 0 : *memberUEDValues.begin());
        return true;
    }
    
    bool GroupValueFormulas::updateGroupValueSum(uint16_t preExistingGroupSize, UserExtendedDataValue joinOrLeavingMemberUedValue, uint16_t joinOrLeavingMemberSize, bool isLeavingSession, UserExtendedDataValue& uedValue, TeamUEDVector& memberUEDValues)
    {
        if (joinOrLeavingMemberSize == 0)
            return true;

        if (!updateMemberUEDValuesList(memberUEDValues, joinOrLeavingMemberUedValue, isLeavingSession))
            return false;

        // get the sum of all members
        UserExtendedDataValue sumAddingUEDValues = ((isLeavingSession? -1:1) * joinOrLeavingMemberUedValue);
        UserExtendedDataValue sumPreExistingUEDValues = ((preExistingGroupSize == 0)? 0 : uedValue);

        UserExtendedDataValue newSumUEDValues = sumAddingUEDValues + sumPreExistingUEDValues;
        uedValue = newSumUEDValues;
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief calculate the desired UED value of the 'would-be' joining group or additional member,
        in order to attain a specified new value, based pre-existing group size, and pre-existing group UED.
        Note: the additional member may be one or more players in a MM session, or game.
    *************************************************************************************************/
    bool GroupValueFormulas::calcDesiredJoiningMemberValueAverage(uint16_t preExistingGroupSize, UserExtendedDataValue preExistingUedValue, UserExtendedDataValue& otherMemberUedValue, uint16_t otherMemberSize, UserExtendedDataValue targetUedValue)
    {
        int64_t totalSize = getNewGroupSize(preExistingGroupSize, otherMemberSize, false);
        if (totalSize < 0)
            return false;//logged

        UserExtendedDataValue sumPreExistingUEDValues = ((preExistingGroupSize == 0)? 0 : (preExistingUedValue * preExistingGroupSize));

        otherMemberUedValue = (((targetUedValue * totalSize) - sumPreExistingUEDValues) / otherMemberSize);
        return true;
    }

    bool GroupValueFormulas::calcDesiredJoiningMemberValueMax(uint16_t preExistingGroupSize, UserExtendedDataValue preExistingUedValue, UserExtendedDataValue& otherMemberUedValue, uint16_t otherMemberSize, UserExtendedDataValue targetUedValue)
    {
        preExistingUedValue = ((preExistingGroupSize == 0)? INT64_MIN : preExistingUedValue);
        otherMemberUedValue = (targetUedValue > preExistingUedValue? targetUedValue : preExistingUedValue);
        return true;
    }

    bool GroupValueFormulas::calcDesiredJoiningMemberValueMin(uint16_t preExistingGroupSize, UserExtendedDataValue preExistingUedValue, UserExtendedDataValue& otherMemberUedValue, uint16_t otherMemberSize, UserExtendedDataValue targetUedValue)
    {
        preExistingUedValue = ((preExistingGroupSize == 0)? INT64_MAX : preExistingUedValue);
        otherMemberUedValue = (targetUedValue < preExistingUedValue? targetUedValue : preExistingUedValue);
        return true;
    }

    bool GroupValueFormulas::calcDesiredJoiningMemberValueSum(uint16_t preExistingGroupSize, UserExtendedDataValue preExistingUedValue, UserExtendedDataValue& otherMemberUedValue, uint16_t otherMemberSize, UserExtendedDataValue targetUedValue)
    {
        UserExtendedDataValue sumPreExistingUEDValues = ((preExistingGroupSize == 0)? 0 : preExistingUedValue);

        otherMemberUedValue = (targetUedValue - sumPreExistingUEDValues);
        return true;
    }



    /*! \brief helper, calculate the new group size */
    int64_t GroupValueFormulas::getNewGroupSize(uint16_t preExistingGroupSize, uint16_t joinOrLeavingMemberSize, bool isLeavingSession)
    {
        int64_t totalSize = (int64_t)preExistingGroupSize + ((isLeavingSession? -1:1) * joinOrLeavingMemberSize);
        if (totalSize < 0)
        {
            ERR_LOG("[GroupValueFormulas].getNewGroupSize UED Value team size inputs invalid, attempted to work on a team that would be of negative size '" << totalSize << "', original size '"<< preExistingGroupSize << "'");
        }
        return totalSize;
    }

    /*! \brief helper, adds/removes from the list and sorts by UED value. Pre: teams are typically small, sorting their UED values is low cost. */
    bool GroupValueFormulas::updateMemberUEDValuesList(TeamUEDVector& memberUEDValues, UserExtendedDataValue joinOrLeavingMemberUedValue, bool isLeavingSession)
    {
        if (isLeavingSession)
        {
            TeamUEDVector::iterator foundItr =  eastl::find(memberUEDValues.begin(), memberUEDValues.end(), joinOrLeavingMemberUedValue);
            if (foundItr == memberUEDValues.end())
            {
                ERR_LOG("[GroupValueFormulas].updateMemberUEDValuesList failed to find expected UED value '" << joinOrLeavingMemberUedValue << "' to remove from the member UED's list, on removal of member session from the team/group. The cached off list of UED values appear to be out of sync with actual members in the team/group.");
                return false;
            }
            memberUEDValues.erase(foundItr);
        }
        else
        {
            memberUEDValues.push_back(joinOrLeavingMemberUedValue);
            eastl::sort(memberUEDValues.begin(), memberUEDValues.end());
        }
        return true;
    }

    bool GroupValueFormulas::getLocalUserData(const GameManager::UserSessionInfo& sessionInfo, UserExtendedDataKey dataKey, UserExtendedDataValue& dataValue)
    {
        if (!UserSession::getDataValue(sessionInfo.getDataMap(), dataKey, dataValue))
        {
            TRACE_LOG("[GroupValueFormulas].getLocalUserData() - Cannot get value for userSessionId '" << sessionInfo.getSessionId() << "' and dataKey '" << SbFormats::HexLower(dataKey) << "'.");
            return false;
        }

        return true;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
