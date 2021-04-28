/*! ************************************************************************************************/
/*!
    \file teamuedbalanceruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/teamuedbalanceruledefinition.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/matchmaker/rules/teamuedbalancerule.h"
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

    DEFINE_RULE_DEF_CPP(TeamUEDBalanceRuleDefinition, "teamUEDBalanceRuleMap", RULE_DEFINITION_TYPE_MULTI);

    const char8_t* TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_IMBALANCE = "Imbalance";
    const char8_t* TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_TOP_TEAM = "Top";
    const char8_t* TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_2ND_TOP_TEAM = "Top2";

    /*! ************************************************************************************************/
    /*! \brief Construct an uninitialized TeamUEDBalanceRuleDefinition.  NOTE: do not use until initialized and
        at least 1 RangeList has been added
    *************************************************************************************************/
    TeamUEDBalanceRuleDefinition::TeamUEDBalanceRuleDefinition()
        : RuleDefinition(),
        mFitFormula(nullptr),
        mDataKey(INVALID_USER_EXTENDED_DATA_KEY),
        mGlobalMaxTeamUEDImbalance(0)
    {
    }

    TeamUEDBalanceRuleDefinition::~TeamUEDBalanceRuleDefinition()
    {
        delete mFitFormula;
    }

    bool TeamUEDBalanceRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        TeamUEDBalanceRuleConfigMap::const_iterator iter = matchmakingServerConfig.getRules().getTeamUEDBalanceRuleMap().find(name);
        if (iter == matchmakingServerConfig.getRules().getTeamUEDBalanceRuleMap().end())
        {
            ERR_LOG("[TeamUEDBalanceRuleDefinition].initialize failed because could not find generic rule by name " << name);
            return false;
        }

        const TeamUEDBalanceRuleConfig& ruleConfig = *(iter->second);
        ruleConfig.copyInto(mRuleConfigTdf);

        if (mRuleConfigTdf.getUserExtendedDataName()[0] == '\0')
        {
            ERR_LOG("[TeamUEDBalanceRuleDefinition].initialize ERROR userExtendedDataName empty for rule '" << name << "'.");
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
        if (!mRangeListContainer.initialize(getName(), lists, false))//false to check cfg's 'offsets' use single value form only
            return false;

        if (getTeamValueFormula() == GROUP_VALUE_FORMULA_LEADER)
        {
            ERR_LOG("[TeamUEDBalanceRuleDefinition].initialize configured group value formula GROUP_VALUE_FORMULA_LEADER is not supported for TeamUEDBalanceRule's.");
            return false;
        }

        mGlobalMaxTeamUEDImbalance = (ruleConfig.getRange().getMax() - ruleConfig.getRange().getMin());
        if (mGlobalMaxTeamUEDImbalance <= 0)
        {
            ERR_LOG("[TeamUEDBalanceRuleDefinition].initialize configured team UED imbalance range magnitude (Team UED max - min) cannot be <= 0. The currently configured max is '" << ruleConfig.getRange().getMax() << "', min is '" << ruleConfig.getRange().getMin() << "'");
            return false;
        }

        bool expressionInitResult = mGroupUedExpressionList.initialize(mRuleConfigTdf.getGroupAdjustmentFormula());
        if (!expressionInitResult)
        {
            ERR_LOG("[TeamUEDBalanceRuleDefinition].initialize ERROR failed to initialize group UED expressions for rule '" << name << "'.");
            return false;
        }

        return true;
    }

    void TeamUEDBalanceRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, 
        const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        if (!isReteCapable())
        {
            // post rete eval only
            return;
        }
        const uint16_t teamCount = gameSessionSlave.getTeamCount();
        if (teamCount < 2)
        {
            TRACE_LOG("[TeamUEDBalanceRuleDefinition].insertWorkingMemoryElements Not adding WME for game(" << gameSessionSlave.getGameId() << "), does not contain 2 or more teams.");
            return;
        }

        // We find the two teams with the largest and smallest team UED values. Note: RETE will be a broad bucketed filter
        // (also unlike actual post-rete we assume players might join either side for now without accounting for other criteria)
        UserExtendedDataValue lowestTeamUED = INVALID_USER_EXTENDED_DATA_VALUE;
        UserExtendedDataValue highestTeamUED = INVALID_USER_EXTENDED_DATA_VALUE;
        UserExtendedDataValue actualUEDImbal = calcGameImbalance(gameSessionSlave, lowestTeamUED, highestTeamUED);
        if ((actualUEDImbal == INVALID_USER_EXTENDED_DATA_VALUE) || (lowestTeamUED == INVALID_USER_EXTENDED_DATA_VALUE) || (highestTeamUED == INVALID_USER_EXTENDED_DATA_VALUE))
        {
            ERR_LOG("[TeamUEDBalanceRuleDefinition].insertWorkingMemoryElements: failed to calculate Team UED imbalance for teams in game " << gameSessionSlave.getGameId() << " to join, for rule definition " << getName() << ".");
            return;
        }

        switch (getTeamValueFormula())
        {
            case GROUP_VALUE_FORMULA_MAX:
            case GROUP_VALUE_FORMULA_MIN:
                buildWmesMaxOrMin(wmeManager, gameSessionSlave, lowestTeamUED, highestTeamUED, actualUEDImbal, false);
                break;
            case GROUP_VALUE_FORMULA_AVERAGE:
            case GROUP_VALUE_FORMULA_SUM:
                buildWmesSum(wmeManager, gameSessionSlave, lowestTeamUED, highestTeamUED, actualUEDImbal, false);
                break;
            case GROUP_VALUE_FORMULA_LEADER:
            default:
                ERR_LOG("[TeamUEDBalanceRuleDefinition].insertWorkingMemoryElements unhandled/invalid group value formula " << GameManager::GroupValueFormulaToString(getTeamValueFormula()));
                break;
        };
    }

    void TeamUEDBalanceRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        if (!isReteCapable())
            return;

        const uint16_t teamCount = gameSessionSlave.getTeamCount();
        if (teamCount < 2)
        {
            TRACE_LOG("[TeamUEDBalanceRuleDefinition].upsertWorkingMemoryElements Not adding WME for game(" << gameSessionSlave.getGameId() << "), does not contain 2 or more teams.");
            return;
        }

        UserExtendedDataValue lowestTeamUED = INVALID_USER_EXTENDED_DATA_VALUE;
        UserExtendedDataValue highestTeamUED = INVALID_USER_EXTENDED_DATA_VALUE;
        UserExtendedDataValue actualUEDImbal = calcGameImbalance(gameSessionSlave, lowestTeamUED, highestTeamUED);
        if ((actualUEDImbal == INVALID_USER_EXTENDED_DATA_VALUE) || (lowestTeamUED == INVALID_USER_EXTENDED_DATA_VALUE) || (highestTeamUED == INVALID_USER_EXTENDED_DATA_VALUE))
        {
            ERR_LOG("[TeamUEDBalanceRuleDefinition].upsertWorkingMemoryElements: failed to calculate Team UED imbalance for teams in game " << gameSessionSlave.getGameId() << " to join, for rule definition " << getName() << ".");
            return;
        }

        switch (getTeamValueFormula())
        {
        case GROUP_VALUE_FORMULA_MAX:
        case GROUP_VALUE_FORMULA_MIN:
            buildWmesMaxOrMin(wmeManager, gameSessionSlave, lowestTeamUED, highestTeamUED, actualUEDImbal, true);
            break;
        case GROUP_VALUE_FORMULA_AVERAGE:
        case GROUP_VALUE_FORMULA_SUM:
            buildWmesSum(wmeManager, gameSessionSlave, lowestTeamUED, highestTeamUED, actualUEDImbal, true);
            break;
        case GROUP_VALUE_FORMULA_LEADER:
        default:
            ERR_LOG("[TeamUEDBalanceRuleDefinition].upsertWorkingMemoryElements unhandled/invalid group value formula " << GameManager::GroupValueFormulaToString(getTeamValueFormula()));
            break;
        };
    }

    /*! ************************************************************************************************/
    /*! \brief calculate the current team ued imbalance between largest-valued and smallest-valued teams in game.
        \param[in] joiningSessionRule if non nullptr, account for this potential joiner as if it were in the game.
        \param[in] joiningTeamIndex if non UNSPECIFIED_TEAM_INDEX, account for the potential joiner as if it were in this team
    *************************************************************************************************/
    UserExtendedDataValue TeamUEDBalanceRuleDefinition::calcGameImbalance(const Search::GameSessionSearchSlave& gameSession,
        UserExtendedDataValue& lowestTeamUED, UserExtendedDataValue& highestTeamUED,
        const TeamUEDBalanceRule* joiningSessionRule /*= nullptr*/, TeamIndex joiningTeamIndex /*= UNSPECIFIED_TEAM_INDEX*/, bool ignoreEmptyTeams /*= false*/) const
    {
        const uint16_t teamCount = gameSession.getTeamCount();
        if (teamCount < 2)
        {
            return 0;
        }
        
        // note: for GB or WME updating, we do not account for any potential joiner's UED
        const bool calcWithPotentialJoiner = ((joiningSessionRule != nullptr) && (joiningSessionRule->getPlayerCount() > 0));
        if (calcWithPotentialJoiner && (UNSPECIFIED_TEAM_INDEX == joiningTeamIndex))
        {
            ERR_LOG("[TeamUEDBalanceRuleDefinition].calcGameImbalance: invalid team index or joining session size specified for joining session, for calculation of game " << gameSession.getGameId() << " Team UED.");
            return INVALID_USER_EXTENDED_DATA_VALUE;
        }

        // pre: Matchmaking game info cache is up to date here.
        const TeamUEDVector* cachedTeamUEDs = gameSession.getMatchmakingGameInfoCache()->getCachedTeamUEDVector(*this);
        if ((cachedTeamUEDs == nullptr) || (cachedTeamUEDs->size() != teamCount))
        {
            ERR_LOG("[TeamUEDBalanceRuleDefinition].calcGameImbalance: expected '" << teamCount << "' number of cached team UED values for game " << gameSession.getGameId() << ", for rule definition " << getName() << ". Actual number found '" << ((cachedTeamUEDs != nullptr)? cachedTeamUEDs->size() : 0) << "'.");
            return INVALID_USER_EXTENDED_DATA_VALUE;
        }

        // find the largest and smallest team UED values
        lowestTeamUED = INT64_MAX;
        highestTeamUED = INT64_MIN;
        for (TeamIndex teamIndex = 0; teamIndex < teamCount; ++teamIndex)
        {
            UserExtendedDataValue curTeamUED = (*cachedTeamUEDs)[teamIndex];
            uint16_t preExistingTeamSize = gameSession.getPlayerRoster()->getTeamSize(teamIndex);

            if (calcWithPotentialJoiner && (teamIndex == joiningTeamIndex))
            {
                // account for the team player will join store recalc'd value to curTeamUED
                TeamUEDVector preExistingUedsList;
                if (preExistingTeamSize != 0)
                {
                    // (side: here, to be able to reuse the recalc helper below, for any group formulas (avg/sum/min/max) simply treat the existing entire pre-existing team as a single group, push onto this list arg to helper below)
                    preExistingUedsList.push_back(curTeamUED);
                }

                if (!recalcTeamUEDValue(curTeamUED, preExistingTeamSize,
                    joiningSessionRule->getUEDValue(), joiningSessionRule->getPlayerCount(), false, preExistingUedsList, joiningTeamIndex))
                {
                    ERR_LOG("[TeamUEDBalanceRuleDefinition].calcGameImbalance: failed to recalculate potential new team UED value for game " << gameSession.getGameId() << ", for rule " << getName());
                }
            }

            // if this team is either already non-empty OR this is the team the user will be joining, update the highest/lowest UED
            // we skip teams that will remain empty for evaluation purposes, as team size and player count rules are in charge of determining if an empty game is considered a match
            if ((calcWithPotentialJoiner && (teamIndex == joiningTeamIndex)) || (preExistingTeamSize != 0) || !ignoreEmptyTeams)
            {
                if (curTeamUED > highestTeamUED)
                {
                    highestTeamUED = curTeamUED;
                }
                if (curTeamUED < lowestTeamUED)
                {
                    lowestTeamUED = curTeamUED;
                }
            }
        }
        
        if ((lowestTeamUED != INT64_MAX) && (highestTeamUED != INT64_MIN)) 
        {
            // we set highest/lowest values, so calc the diff
            UserExtendedDataValue gameBalance = (highestTeamUED - lowestTeamUED);
            return abs(gameBalance);
        }

        lowestTeamUED = highestTeamUED = getMinRange();
        return 0;
    }

    /*! ************************************************************************************************/
    /*! \brief Add WME's for max/min group formulas.
    *************************************************************************************************/
    void TeamUEDBalanceRuleDefinition::buildWmesMaxOrMin(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSession,
        UserExtendedDataValue lowestTeamUED, UserExtendedDataValue highestTeamUED, UserExtendedDataValue actualUEDImbal, bool doUpsert) const
    {
        const UserExtendedDataValue valueTop = ((getTeamValueFormula() == GROUP_VALUE_FORMULA_MAX)? highestTeamUED : lowestTeamUED);
        // get 2nd from top team
        UserExtendedDataValue value2ndToTop = get2ndFromTopUedValuedTeam(gameSession, lowestTeamUED, highestTeamUED);

        TRACE_LOG("[TeamUEDBalanceRuleDefinition].buildWmesMaxOrMin doing " << (doUpsert? "upsert":"insert") << " of WME for rule(" << getName() 
            << ", uedName=" << getUEDName() << ") for game(" << gameSession.getGameId() 
            << ") with an imbalance of " << actualUEDImbal << ", highest team UED " << highestTeamUED << ", lowest team UED " << lowestTeamUED 
            << "), 'top' team UED (post-bucketization=" << valueTop << "), '2nd to top' team UED (post-bucketization=" << value2ndToTop << ").");
        if (!doUpsert)
        {
            // NOTE: this rule's RETE is a broad filter, we rely on post-RETE to precisely filter

            // I might not affect any team's max/min. If game's current imbalance is in our range, count as possible RETE match.
            wmeManager.insertWorkingMemoryElement(gameSession.getGameId(), TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_IMBALANCE, actualUEDImbal, *this);

            // I might join and need to compare with the top or else 2nd-to-top max/min-valued team. If that team's and my UED is in range, count as possible RETE match.
            wmeManager.insertWorkingMemoryElement(gameSession.getGameId(), TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_TOP_TEAM, valueTop, *this);
            wmeManager.insertWorkingMemoryElement(gameSession.getGameId(), TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_2ND_TOP_TEAM, value2ndToTop, *this);
        }
        else
        {
            wmeManager.upsertWorkingMemoryElement(gameSession.getGameId(), TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_IMBALANCE, actualUEDImbal, *this);
            wmeManager.upsertWorkingMemoryElement(gameSession.getGameId(), TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_TOP_TEAM, valueTop, *this);
            wmeManager.upsertWorkingMemoryElement(gameSession.getGameId(), TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_2ND_TOP_TEAM, value2ndToTop, *this);
        }
    }


    /*! ************************************************************************************************/
    /*! \brief Add WME's for sum group formula
        Note: for simplicity (and efficiency), group formula average will also re-use this for RETE part (rely on post-RETE to precisely filter).
    *************************************************************************************************/
    void TeamUEDBalanceRuleDefinition::buildWmesSum(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSession,
        UserExtendedDataValue lowestTeamUED, UserExtendedDataValue highestTeamUED, UserExtendedDataValue actualUEDImbal, bool doUpsert) const
    {
        const UserExtendedDataValue valueImbal = (highestTeamUED - lowestTeamUED);
        TRACE_LOG("[TeamUEDBalanceRuleDefinition].buildWmesSum doing " << (doUpsert? "upsert":"insert") << " of WME for rule(" << getName() 
            << ", uedName=" << getUEDName() << ") for game(" << gameSession.getGameId() << ") with an imbalance of " << actualUEDImbal << "(post-bucketization=" << valueImbal 
            << "), highest team UED " << highestTeamUED << ", lowest team UED " << lowestTeamUED << ".");
        if (!doUpsert)
        {
            wmeManager.insertWorkingMemoryElement(gameSession.getGameId(), TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_IMBALANCE, valueImbal, *this);
        }
        else
        {
            wmeManager.upsertWorkingMemoryElement(gameSession.getGameId(), TeamUEDBalanceRuleDefinition::TEAMUEDBALANCE_ATTRIBUTE_IMBALANCE, valueImbal, *this);
        }
    }



    void TeamUEDBalanceRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
    {
        if (matchmakingCache.isTeamInfoDirty() || matchmakingCache.isRosterDirty())
        {
            matchmakingCache.cacheTeamUEDValues(*this, gameSession);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief side: desired value should be zero in most cases.
    *************************************************************************************************/
    float TeamUEDBalanceRuleDefinition::getFitPercent(int64_t desiredValue, int64_t actualValue) const
    {
        if (mFitFormula != nullptr)
            return mFitFormula->getFitPercent(desiredValue, actualValue);
        else
        {
            EA_ASSERT(mFitFormula != nullptr);
            return 0;
        }
    }

    const RangeList* TeamUEDBalanceRuleDefinition::getRangeOffsetList(const char8_t *listName) const
    {
        return mRangeListContainer.getRangeList(listName);
    }

    void TeamUEDBalanceRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        MatchmakingUtil::writeUEDNameToString(dest, prefix, "TeamUEDKey", mRuleConfigTdf.getUserExtendedDataName());
        mRangeListContainer.writeRangeListsToString(dest, prefix);
    }

    /*! \brief metrics helper */
    bool TeamUEDBalanceRuleDefinition::getSessionMaxOrMinUEDValue(bool getMax, UserExtendedDataValue& uedValue, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersSessionInfo) const
    {
        GroupValueFormula formula = (getMax? GROUP_VALUE_FORMULA_MAX : GROUP_VALUE_FORMULA_MIN);
        UserExtendedDataKey uedKey = getUEDKey();

        GroupUedExpressionList dummyGroupExpressionList;
        if (!(*GroupValueFormulas::getCalcGroupFunction(formula))(uedKey, dummyGroupExpressionList, ownerBlazeId, ownerGroupSize, membersSessionInfo, uedValue)) /*lint !e613 */
        {
            TRACE_LOG("[TeamUEDBalanceRuleDefinition].getSessionMaxOrMinUEDValue WARN UED Value for group session not found or failed to calculate, for rule '" << ((getName() != nullptr)? getName() : "") << "', UED '" << mRuleConfigTdf.getUserExtendedDataName() << "'");
            return false;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief calculates the UED value for the user or group depending on which is valid.
        NOTE: this method does lookups of the actual user session's extended data's in order to update the overall calculated value.
        \return false if the value fails to be calculated (indicates a system error)
    *************************************************************************************************/
    bool TeamUEDBalanceRuleDefinition::calcUEDValue(UserExtendedDataValue& uedValue, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersSessionInfo) const
    {   //TODO: rem dependencies on rules for Team UEDs also, refactor to reuse CalcUEDUtil
        UserExtendedDataKey uedKey = getUEDKey();
        
        if (!(*GroupValueFormulas::getCalcGroupFunction(getTeamValueFormula()))(uedKey, mGroupUedExpressionList, ownerBlazeId, ownerGroupSize, membersSessionInfo, uedValue)) /*lint !e613 */
        {
            TRACE_LOG("[TeamUEDBalanceRuleDefinition].calcUEDValue WARN UED Value for group session not found or failed to calculate, for rule '" << ((getName() != nullptr)? getName() : "") << "', UED '" << mRuleConfigTdf.getUserExtendedDataName() << "'");
            return false;
        }


        // If we successfully calculate the value, clamp at the min/max values.
        if (normalizeUEDValue(uedValue))
        {
            TRACE_LOG("[TeamUEDBalanceRuleDefinition].calcUEDValue calculated uedValue '" << uedValue << "' was clamped to keep it in range, for rule '" << ((getName() != nullptr)? getName() : "") << "', UED '" << mRuleConfigTdf.getUserExtendedDataName() << "'");
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief re-calculates the UED value for the team when member joins or leaves
        \param[in,out] uedValue value to update.
        \param[in,out] memberUEDValues list of team's member session's UED values (for group formulas max/min).
        \return the calculated UED value, or INVALID_USER_EXTENDED_DATA_VALUE if no UED value calculated.
    *************************************************************************************************/
    bool TeamUEDBalanceRuleDefinition::recalcTeamUEDValue(UserExtendedDataValue& uedValue, uint16_t preExistingTeamSize,
        UserExtendedDataValue joinOrLeavingUedValue, uint16_t joinOrLeavingSessionSize, bool isLeavingSession,
        TeamUEDVector& memberUEDValues, TeamIndex teamIndex) const
    {
        // no op in case we're called on a joining session of size 0
        if (joinOrLeavingSessionSize == 0)
            return true;

        UserExtendedDataValue origUedValue = uedValue;

        // For correct calc, if team's uedValue was previously clamped at max or min
        // refresh team's uedValue without normalization to start (we'll re-normalize below).
        UserExtendedDataValue unNormalizedUedValue = uedValue;
        if ((uedValue >= getMaxRange()) || (uedValue <= getMinRange()))
        {
            if (!calcUnNormalizedTeamUEDValue(unNormalizedUedValue, memberUEDValues))
                return false;
        }

        // If we successfully calculate the value, clamp at the min/max values.
        if ((*GroupValueFormulas::getUpdateGroupFunction(getTeamValueFormula()))(preExistingTeamSize, joinOrLeavingUedValue, joinOrLeavingSessionSize,
            isLeavingSession, unNormalizedUedValue, memberUEDValues)) /*lint !e613 */
        {
            // side: for consistency w/UEDRule's calc for empty games, empty teams get UED value get clamped to min
            uedValue = unNormalizedUedValue;
            if (normalizeUEDValue(uedValue))
            {
                TRACE_LOG("[TeamUEDBalanceRuleDefinition].recalcTeamUEDValue calculated ued value was out of range, clamped to '" << uedValue << "'");
            }
            TRACE_LOG("[TeamUEDBalanceRuleDefinition].recalcTeamUEDValue " << "rule '" << getName() << "' " << GameManager::GroupValueFormulaToString(getTeamValueFormula())
                << ", updating team(index'" << teamIndex << "') UED value to '" << uedValue << "', after " << (isLeavingSession? "leaving" : "joining") << " MM session(with uedValue=" << joinOrLeavingUedValue << ",size=" << joinOrLeavingSessionSize << ")"
                << ". Pre-existing team size: '" << preExistingTeamSize << "', pre-existing team UED value: '" << origUedValue << "'.");
            return true;
        }
        return false;
    }
    //TODO: rem dependencies on rules for Team UEDs also, refactor to reuse CalcUedUtil
    bool TeamUEDBalanceRuleDefinition::calcUnNormalizedTeamUEDValue(UserExtendedDataValue& unNormalizedUedValue, const TeamUEDVector& memberUEDValues) const
    {
        GroupValueFormulas::UpdateGroupFunction groupFn = GroupValueFormulas::getUpdateGroupFunction(getTeamValueFormula());
        TeamUEDVector dummyMemberUEDValues;
        uint16_t dummyTeamSize = 0;
        for (TeamUEDVector::const_iterator itr = memberUEDValues.begin(); itr != memberUEDValues.end(); ++itr)
        {
            if (!(*groupFn)(dummyTeamSize++, *itr, 1, false, unNormalizedUedValue, dummyMemberUEDValues)) /*lint !e613 */
                return false;//logged
        }
        return true;
    }
    //TODO: rem dependencies on rules for Team UEDs also, refactor to reuse CalcUedUtil
    /*! ************************************************************************************************/
    /*! \brief calculate the desired UED value of a member that would be added, in order to attain a specified
        target value for the group, based on rule's group value formula, and, the new and pre-existing group
        size, and pre-existing group UED value.

        For group formula MAX and MIN, if target would be the new max/min, then the desired joining member's
        value will be equal to target. Otherwise the returned desired value will be the pre-existing max/min.
        \param[out] otherMemberUedValue stores the result.
    *************************************************************************************************/
    bool TeamUEDBalanceRuleDefinition::calcDesiredJoiningMemberUEDValue(UserExtendedDataValue& otherMemberUedValue, uint16_t otherMemberSize, uint16_t preExistingTeamSize, UserExtendedDataValue preExistingUedValue, UserExtendedDataValue targetUedValue) const
    {
        if ((*GroupValueFormulas::getCalcDesiredJoiningMemberFunction(getTeamValueFormula()))(preExistingTeamSize, preExistingUedValue, otherMemberUedValue, otherMemberSize, targetUedValue)) /*lint !e613 */
        {
            if (normalizeUEDValue(otherMemberUedValue))
            {
                SPAM_LOG("[TeamUEDBalanceRuleDefinition].calcDesiredJoiningMemberUEDValue calculated ued value was out of range, clamped to '" << otherMemberUedValue << "'");
            }
            SPAM_LOG("[TeamUEDBalanceRuleDefinition].calcDesiredJoiningMemberUEDValue " << GameManager::GroupValueFormulaToString(getTeamValueFormula()) <<
                ", calculated desired UED value '" << otherMemberUedValue << "' for a joining MM session of size '" << otherMemberSize << "' (in order to reach target team UED value of '" << targetUedValue << "), given pre-existing team size '" << preExistingTeamSize << "', pre-existing team UED value '" << preExistingUedValue << "'.");
            return true;
        }
        return false;//logged
    }

    Blaze::UserExtendedDataKey TeamUEDBalanceRuleDefinition::getUEDKey() const
    {
        if (mDataKey == INVALID_USER_EXTENDED_DATA_KEY)
        {
            // Lazy Init because user extended data isn't available on configuration.
            if (!gUserSessionManager->getUserExtendedDataKey(getUEDName(), mDataKey))
            {
                TRACE_LOG("[TeamUEDBalanceRuleDefinition].getUEDKey() error: key not found for name '" << getUEDName() << "'");
            }
        }

        return mDataKey;
    }

    bool TeamUEDBalanceRuleDefinition::normalizeUEDValue(UserExtendedDataValue& value, bool warnOnClamp/*= false*/) const
    {
        UserExtendedDataValue minRange = getMinRange();
        UserExtendedDataValue maxRange = getMaxRange();

        if (value < minRange)
        {
            if (warnOnClamp)
            {
                WARN_LOG("[TeamUEDBalanceRuleDefinition].normalizeUEDValue RuleName:" << getName() << ", UedName:" << mRuleConfigTdf.getUserExtendedDataName() 
                    <<  " value '" << value << "' outside of min range, clamping to min range '" << minRange << "'");
            }
            value = minRange;
            return true;
        }

        if (value > maxRange)
        {
            if (warnOnClamp)
            {
                WARN_LOG("[TeamUEDBalanceRuleDefinition].normalizeUEDValue RuleName:" << getName() << ", UedName:" << mRuleConfigTdf.getUserExtendedDataName() 
                    <<  " value '" << value << "' outside of max range, clamping to max range '" << maxRange << "'");
            }
            value = maxRange;
            return true;
        }

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief get 2nd from 'top' ued valued team
    *************************************************************************************************/
    UserExtendedDataValue TeamUEDBalanceRuleDefinition::get2ndFromTopUedValuedTeam(const Search::GameSessionSearchSlave& gameSession,
        UserExtendedDataValue lowestTeamUED, UserExtendedDataValue highestTeamUED) const
    {
        const UserExtendedDataValue topValue = ((getTeamValueFormula() == GROUP_VALUE_FORMULA_MAX)? highestTeamUED : lowestTeamUED);
        UserExtendedDataValue secondToTop = ((topValue == highestTeamUED)? lowestTeamUED : highestTeamUED);

        // start with it as the bottom valued, then check if there's another that's better valued below
        if (gameSession.getTeamCount() > 2)
        {
            // pre: Matchmaking game info cache is up to date here.
            const TeamUEDVector* cachedTeamUEDs = gameSession.getMatchmakingGameInfoCache()->getCachedTeamUEDVector(*this);
            if ((cachedTeamUEDs == nullptr) || (cachedTeamUEDs->size() != gameSession.getTeamCount()))
            {
                ERR_LOG("[TeamUEDBalanceRuleDefinition].get2ndFromTopUedValuedTeam: expected '" << gameSession.getTeamCount() << "' number of cached team UED values for game " << gameSession.getGameId() << ", for rule definition " << getName() << ". Actual number found '" << ((cachedTeamUEDs != nullptr)? cachedTeamUEDs->size() : 0) << "'. Assuming bottom ued-valued team is the second-to-top instead.");
                return secondToTop;
            }

            bool visitedTop = false;
            for (TeamIndex teamIndex = 0; teamIndex < gameSession.getTeamCount(); ++teamIndex)
            {
                UserExtendedDataValue curTeamUED = (*cachedTeamUEDs)[teamIndex];
                if (curTeamUED == topValue)
                {
                    if (visitedTop)//skip first instance
                    {
                        secondToTop = curTeamUED;
                        break;
                    }
                    visitedTop = true;
                }

                if ((getTeamValueFormula() == GROUP_VALUE_FORMULA_MAX) && (curTeamUED > secondToTop))
                    secondToTop = curTeamUED;
                else if ((getTeamValueFormula() == GROUP_VALUE_FORMULA_MIN) && (curTeamUED < secondToTop))
                    secondToTop = curTeamUED;
            }
        }
        return secondToTop;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
