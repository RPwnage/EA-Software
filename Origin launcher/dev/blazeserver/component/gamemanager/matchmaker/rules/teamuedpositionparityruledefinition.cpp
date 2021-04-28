/*! ************************************************************************************************/
/*!
    \file teamuedpositionparityruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityruledefinition.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityrule.h"
#include "gamemanager/matchmaker/groupvalueformulas.h"
#include "gamemanager/matchmaker/fitformula.h"
#include "gamemanager/matchmaker/matchmakingutil.h" // for LOG_PREFIX
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/matchmaker/rangelist.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    DEFINE_RULE_DEF_CPP(TeamUEDPositionParityRuleDefinition, "teamUEDPositionParityRuleMap", RULE_DEFINITION_TYPE_MULTI);

    /*! ************************************************************************************************/
    /*! \brief Construct an uninitialized TeamUEDPositionParityRuleDefinition.  NOTE: do not use until initialized and
        at least 1 RangeList has been added
    *************************************************************************************************/
    TeamUEDPositionParityRuleDefinition::TeamUEDPositionParityRuleDefinition()
        : RuleDefinition(),
        mFitFormula(nullptr),
        mDataKey(INVALID_USER_EXTENDED_DATA_KEY)
    {
    }

    TeamUEDPositionParityRuleDefinition::~TeamUEDPositionParityRuleDefinition()
    {
        delete mFitFormula;
    }

    bool TeamUEDPositionParityRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        TeamUEDPositionParityRuleConfigMap::const_iterator iter = matchmakingServerConfig.getRules().getTeamUEDPositionParityRuleMap().find(name);
        if (iter == matchmakingServerConfig.getRules().getTeamUEDPositionParityRuleMap().end())
        {
            ERR_LOG("[TeamUEDPositionParityRuleDefinition].initialize failed because could not find generic rule by name " << name);
            return false;
        }

        const TeamUEDPositionParityRuleConfig& ruleConfig = *(iter->second);
        ruleConfig.copyInto(mRuleConfigTdf);

        if ((getBottomPlayerCount() == 0) && (getTopPlayerCount() == 0))
        {
            // we let this continue, as it's a viable way to 'disable' the rule for clients including it in requests
            WARN_LOG("[TeamUEDPositionParityRuleDefinition].initialize rule '" << name << "' was configured with 0 for top and bottom player count.");
        }

        if (mRuleConfigTdf.getUserExtendedDataName()[0] == '\0')
        {
            ERR_LOG("[TeamUEDPositionParityRuleDefinition].initialize ERROR userExtendedDataName empty for rule '" << name << "'.");
            return false;
        }

        RuleDefinition::initialize(name, salience, mRuleConfigTdf.getWeight());

        mFitFormula = FitFormula::createFitFormula(mRuleConfigTdf.getFitFormula().getName());
        if (mRuleConfigTdf.getFitFormula().getName() == FIT_FORMULA_LINEAR)
        {
            // We use the range configured in the range parameter of the rule because we calculate the fit score based on the absolute difference in Team UED's.
            // Side: LinearFitFormula ERR's on init if have negative minVal, but we're good here since using 0
            ((LinearFitFormula*)mFitFormula)->setDefaultMaxVal(getMaxRange() - getMinRange());
            ((LinearFitFormula*)mFitFormula)->setDefaultMinVal(0);
        }

        if (!mFitFormula->initialize(mRuleConfigTdf.getFitFormula(), &mRuleConfigTdf.getRangeOffsetLists()))
            return false;

        const GameManager::RangeOffsetLists& lists = mRuleConfigTdf.getRangeOffsetLists();
        if (!mRangeListContainer.initialize(getName(), lists))
            return false;

        return true; 
    }


    void TeamUEDPositionParityRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
    {
        if (matchmakingCache.isTeamInfoDirty() || matchmakingCache.isRosterDirty())
        {
            matchmakingCache.cacheTeamIndividualUEDValues(*this, gameSession);
            matchmakingCache.sortCachedTeamIndivdualUEDVectors(*this);
        }
    }

    float TeamUEDPositionParityRuleDefinition::calcFitPercent(const MatchmakingGameInfoCache::TeamIndividualUEDValues& teamUedValues, uint16_t teamCapacity, TeamIndex joiningTeamIndex, const TeamUEDVector& joiningTeamUeds, 
        UserExtendedDataValue topPlayersMaxDifference, UserExtendedDataValue bottomPlayersMaxDifference, ReadableRuleConditions& debugRuleConditions) const
    {
        
        float fitPercent = 0.0;
        // iterate over the teams (using the substitute list for the joining team
        // determine if the range at any considered position is out of spec
        // calc fit score by determining the fit percent of *each* position, and averaging the result

        // first check top N
        for (uint16_t i = 0; i < getTopPlayerCount(); ++i )
        {
            UserExtendedDataValue lowest = INVALID_USER_EXTENDED_DATA_VALUE;
            UserExtendedDataValue highest = INVALID_USER_EXTENDED_DATA_VALUE;
            // this method needs to take the rank we care about as a parameter
            if (!getHighestAndLowestUEDValueAtRank(i, teamUedValues, joiningTeamIndex, joiningTeamUeds, highest, lowest))
                return false; // we've logged this, something was wrong with the team vectors

            // skip the eval if no users at this position in the teams; note we do eval if at least two teams have a user in this position
            // see impl of getHighestAndLowestTopUEDValuedFinalizingTeams()
            // otherwise, this rule could become a de-facto team size balance rule.
            if ((highest != INVALID_USER_EXTENDED_DATA_VALUE) && (lowest != INVALID_USER_EXTENDED_DATA_VALUE))
            {
                UserExtendedDataValue largestTeamUedDiff = abs( highest - lowest );

                if (largestTeamUedDiff > topPlayersMaxDifference)
                {
                    debugRuleConditions.writeRuleCondition("Top %u th UED difference %" PRId64 " > %" PRId64 ". ", i, largestTeamUedDiff, topPlayersMaxDifference);
        
                    return FIT_PERCENT_NO_MATCH;
                }
                else
                {
                    debugRuleConditions.writeRuleCondition("Top %u th UED difference %" PRId64 " <= %" PRId64 ". ", i, largestTeamUedDiff, topPlayersMaxDifference);
                    fitPercent += getFitPercent(getMinRange(), largestTeamUedDiff);
                }
            }
            else
            {
                debugRuleConditions.writeRuleCondition("Top %u th position didn't have players to compare. ", i);
            }
        }

        // now check bottom N
        if (getBottomPlayerCount() != 0)
        {

            if (teamCapacity < getBottomPlayerCount())
            {
                // something is wrong, we are trying to eval more bottom positions than the game has capacity for
                // skip evaluating bottom players
                WARN_LOG("[TeamUEDPositionParityRuleDefinition].calcFitPercent: "
                    << " trying to evaluate (" << getBottomPlayerCount() << ") from bottom, with team capacity of (" << teamCapacity << ").");
                
            }
            else
            {
                for (uint16_t i = 0; i < getBottomPlayerCount(); ++i )
                {
                    // magic number to keep bottom index in bounds
                    uint16_t bottomIndex = teamCapacity - i - 1;
                    UserExtendedDataValue lowest = INVALID_USER_EXTENDED_DATA_VALUE;
                    UserExtendedDataValue highest = INVALID_USER_EXTENDED_DATA_VALUE;
                    // this method needs to take the rank we care about as a parameter
                    if (!getHighestAndLowestUEDValueAtRank(bottomIndex, teamUedValues, joiningTeamIndex, joiningTeamUeds, highest, lowest))
                        return false; // we've logged this, something was wrong with the team vectors

                    // skip the eval if no users at this position in the teams; note we do eval if at least two teams have a user in this position
                    // see impl of getHighestAndLowestTopUEDValuedFinalizingTeams()
                    // otherwise, this rule could become a de-facto team size balance rule.
                    if ((highest != INVALID_USER_EXTENDED_DATA_VALUE) && (lowest != INVALID_USER_EXTENDED_DATA_VALUE))
                    {
                        UserExtendedDataValue largestTeamUedDiff = abs( highest - lowest );

                        if (largestTeamUedDiff > topPlayersMaxDifference)
                        {
                            debugRuleConditions.writeRuleCondition("Bottom %u th UED difference %" PRId64 " > %" PRId64 ". ", i, largestTeamUedDiff, topPlayersMaxDifference);
                
                            return FIT_PERCENT_NO_MATCH;
                        }
                        else
                        {
                            debugRuleConditions.writeRuleCondition("Bottom %u th UED difference %" PRId64 " <= %" PRId64 ". ", i, largestTeamUedDiff, topPlayersMaxDifference);
                            fitPercent += getFitPercent(getMinRange(), largestTeamUedDiff);
                        }
                    }
                    else
                    {
                        debugRuleConditions.writeRuleCondition("Bottom %u th position didn't have players to compare. ", i);
                    }
                }
            }
            
        }


        // calc the fit score if we didn't DQ this game earlier

        uint16_t positionsToCompare = getTopPlayerCount() + getBottomPlayerCount();
        debugRuleConditions.writeRuleCondition("All positions checked were either empty or within acceptable range.");
        

        return (positionsToCompare == 0) ? (float)0.0 : (fitPercent / positionsToCompare);
    }

    bool TeamUEDPositionParityRuleDefinition::getHighestAndLowestUEDValueAtRank(uint16_t rankIndex, const MatchmakingGameInfoCache::TeamIndividualUEDValues& teamUedValues, TeamIndex joiningTeamIndex, const TeamUEDVector& joiningTeamUeds, 
        UserExtendedDataValue& highestUedValue, UserExtendedDataValue& lowestUedValue) const
    {
        highestUedValue = INVALID_USER_EXTENDED_DATA_VALUE;
        lowestUedValue = INVALID_USER_EXTENDED_DATA_VALUE;
        
        if (teamUedValues.empty())
        {
            ERR_LOG("[TeamUEDPositionParityRuleDefinition].getHighestAndLowestUEDValueAtRank: teamUedValues was empty.");
            return false;
        }
        
        size_t teamUedValuesCount = teamUedValues.size();
        for (size_t i = 0; i < teamUedValuesCount; ++i)
        {
            // sub in the temp vector if we're checking the joining team
            const TeamUEDVector& teamUedList = (i == joiningTeamIndex) ? joiningTeamUeds : teamUedValues[i];

            // if there's no player at the current rank, use INVALID_USER_EXTENDED_DATA_VALUE as a placeholder
            UserExtendedDataValue uedValue = rankIndex < teamUedList.size() ? teamUedList[rankIndex] : INVALID_USER_EXTENDED_DATA_VALUE;

            // we only do comparisons between teams that have a user in the slot
            // get the highest ued value
            if ((highestUedValue == INVALID_USER_EXTENDED_DATA_VALUE) || 
                ((uedValue != INVALID_USER_EXTENDED_DATA_VALUE) && (uedValue > highestUedValue)))
            {
                highestUedValue = uedValue;
            }
            // get the lowest ued value that exists
            if ((lowestUedValue == INVALID_USER_EXTENDED_DATA_VALUE) || 
                ((uedValue != INVALID_USER_EXTENDED_DATA_VALUE) && (uedValue < lowestUedValue)))
            {
                lowestUedValue = uedValue;
            }
        }

       
        SPAM_LOG("[TeamUEDPositionParityRuleDefinition].getHighestAndLowestUEDValueAtRank: at rank (" << rankIndex << ") from top, highest UED valued " << highestUedValue << ", lowest UED valued " << lowestUedValue);
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief side: desired value should be zero in most cases.
    *************************************************************************************************/
    float TeamUEDPositionParityRuleDefinition::getFitPercent(int64_t desiredValue, int64_t actualValue) const
    {
        if (mFitFormula != nullptr)
            return mFitFormula->getFitPercent(desiredValue, actualValue);
        else
        {
            EA_ASSERT(mFitFormula != nullptr);
            return 0;
        }
    }

    const RangeList* TeamUEDPositionParityRuleDefinition::getRangeOffsetList(const char8_t *listName) const
    {
        return mRangeListContainer.getRangeList(listName);
    }

    void TeamUEDPositionParityRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        mRangeListContainer.writeRangeListsToString(dest, prefix);
    }

    Blaze::UserExtendedDataKey TeamUEDPositionParityRuleDefinition::getUEDKey() const
    {
        if (mDataKey == INVALID_USER_EXTENDED_DATA_KEY)
        {
            // Lazy Init because user extended data isn't available on configuration.
            if (!gUserSessionManager->getUserExtendedDataKey(getUEDName(), mDataKey))
            {
                TRACE_LOG("[TeamUEDPositionParityRuleDefinition].getUEDKey() error: key not found for name '" << getUEDName() << "'");
            }
        }

        return mDataKey;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
