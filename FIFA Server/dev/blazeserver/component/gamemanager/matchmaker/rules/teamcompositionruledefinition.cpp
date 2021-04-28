/*! ************************************************************************************************/
/*!
    \file teamcompositionruledefinition.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/teamcompositionruledefinition.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/teamcompositionrule.h"
#include "framework/util/shared/blazestring.h" // for blaze_str2int in possibleValueStringToTeamComposition()

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    DEFINE_RULE_DEF_CPP(TeamCompositionRuleDefinition, "teamCompositionRuleMap", RULE_DEFINITION_TYPE_MULTI);

    const char8_t* TeamCompositionRuleDefinition::TEAMCOMPOSITIONRULE_TEAM_ATTRIBUTE_SPACE = "Space";
    const char8_t* TeamCompositionRuleDefinition::TEAMCOMPOSITIONRULE_TEAM_ATTRIBUTE_GROUP_SIZE = "GroupSize";

    /*! ************************************************************************************************/
    /*! \brief Construct an uninitialized TeamCompositionRuleDefinition.  NOTE: do not use until initialized and
        at least 1 MinFitThresholdList has been added
    *************************************************************************************************/
    TeamCompositionRuleDefinition::TeamCompositionRuleDefinition()
        : RuleDefinition(),
        mTeamCapacityForRule(0),
        mGlobalMinPlayerCountInGame(0),
        mGlobalMaxTotalPlayerSlotsInGame(0),
        mCapacityToMathematicallyPossCompositionsMap(BlazeStlAllocator("mCapacityToMathematicallyPossCompositionsMap", GameManagerSlave::COMPONENT_MEMORY_GROUP))
    {
    }

    TeamCompositionRuleDefinition::~TeamCompositionRuleDefinition()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief initialize rule
    *************************************************************************************************/
    bool TeamCompositionRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        TeamCompositionRuleConfigMap::const_iterator iter = matchmakingServerConfig.getRules().getTeamCompositionRuleMap().find(name);
        if (iter == matchmakingServerConfig.getRules().getTeamCompositionRuleMap().end())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].initialize failed because could not find the rule by name " << name);
            return false;
        }

        const TeamCompositionRuleConfig& ruleConfig = *(iter->second);
        ruleConfig.copyInto(mRuleConfigTdf);

        if (!mFitTable.initialize(ruleConfig.getFitTable(), ruleConfig.getSparseFitTable(), ruleConfig.getPossibleValues()))
        {
            ERR_LOG("[TeamCompositionRuleDefinition].initialize failed to initialize fit table for rule '" << name << "'");
            return false;
        }

        if (!RuleDefinition::initialize(name, salience, ruleConfig.getWeight(), ruleConfig.getMinFitThresholdLists()))
             return false;

        // size related rules require non-0 max
        mGlobalMinPlayerCountInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMin();
        mGlobalMaxTotalPlayerSlotsInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMax();
        if (getGlobalMaxTotalPlayerSlotsInGame() <= 0)
        {
            ERR_LOG("[TeamCompositionRuleDefinition] configured max total player slots of games '" << getGlobalMaxTotalPlayerSlotsInGame() << "' must be greater than 0.");
            return false;
        }

        // validate the fit table is mirrored for all possible values
        if (!validateFitTable(ruleConfig))
            return false;

        // parse and validate possible values from cfg. Add/cache all team composition infos.
        if (!parsePossibleValuesAndInitAllTeamCompositions(ruleConfig.getPossibleValues()))
            return false;

        // validate and cache team compositions' team capacity (after cached the possible team composition infos above)
        mTeamCapacityForRule = calcTeamCapacityForRule(mPossibleTeamCompositionVector);
        if (getTeamCapacityForRule() == 0)
            return false;

        // note: In order to optimize finalization and reduce overhead, MM caches off the permutations and data related to
        // possible game team compositions below. To do this we also need to know how many teams there are. Validate this.
        if ((ruleConfig.getTeamCount() == 0) || ((ruleConfig.getTeamCount() * getTeamCapacityForRule()) > getGlobalMaxTotalPlayerSlotsInGame()))
        {
            ERR_LOG("[TeamCompositionRuleDefinition] configured team count '" << ruleConfig.getTeamCount() << "' for rule '" << name << "', must be greater than zero, and its product with the rules teamCapacity '" << getTeamCapacityForRule() << "', must be <= matchmaker's configured max total player slots of games '" << getGlobalMaxTotalPlayerSlotsInGame() << "'.");
            return false;
        }

        // Add/cache all our game team compositions
        if (!addPossibleGameTeamCompositionsInfos())
            return false;

        // for MM evaluations to quickly access game team compositions by group sizes and fit thresholds, cache to a map
        if (!cacheGroupAndFitThresholdMatchInfos())
            return false;

        // for FG calc's (and reused by below config checks), cache the mathematically possible compositions, formed by future joiners to teams of a certain number of vacant/joinable slots
        if (!cacheMathematicallyPossibleCompositionsForAllCapacitiesUpToMax(getTeamCapacityForRule()))
            return false;

        // check possible values list contains all the possible compositions that add up to max team capacity.
        if (!checkMathematicallyPossibleCompositionsArePossibleValues() && ruleConfig.getRequireMathematicallyCompletePossibleValues())
            return false;

        return true;
    }

    bool TeamCompositionRuleDefinition::validateFitTable(const TeamCompositionRuleConfig& ruleConfig) const
    {
        for (size_t i = 0; i < ruleConfig.getPossibleValues().size(); ++i)
        {
            for (size_t j = (i+1); j < ruleConfig.getPossibleValues().size(); ++j)
            {
                if (mFitTable.getFitPercent(i, j) != mFitTable.getFitPercent(j, i))
                {
                    ERR_LOG("[TeamCompositionRuleDefinition].validateFitTable fit percent for row " << i << ", col " << j << " '" << mFitTable.getFitPercent(i, j) << "', does not equal its mirroring fit percent i.e. for row " << j << ", col " << i << " '" << mFitTable.getFitPercent(j, i) << "'.");
                    return false;
                }        
            }    
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief parse config's possible values, and store to my list of possible team compositions.
    *************************************************************************************************/
    bool TeamCompositionRuleDefinition::parsePossibleValuesAndInitAllTeamCompositions(const PossibleValuesList& possibleValuesStrings)
    {
        EA_ASSERT(mPossibleTeamCompositionVector.empty());
        mPossibleTeamCompositionVector.reserve(possibleValuesStrings.size());

        int index = 0;
        for (PossibleValuesList::const_iterator possValItr = possibleValuesStrings.begin(); possValItr != possibleValuesStrings.end(); ++possValItr)
        {
            const ParamValue& possVal = *possValItr;
            TeamComposition potentialComposition;

            // parse to composition, sizes in composition are in non-ascending sorted order
            if (!possibleValueStringToTeamComposition(possVal, potentialComposition))
                return false;//logged

            // verify its not a duplicate we've added before
            if (isPossibleCompositionForRule(potentialComposition))
            {
                eastl::string buf;
                ERR_LOG("[TeamCompositionRuleDefinition].parsePossibleValuesAndInitAllTeamCompositions configured value '" << possVal.c_str() << "' at index " << index << " in the possible values list, resolved to composition '" << teamCompositionToLogStr(potentialComposition, buf) << "', duplicates already-specified composition in the list at index " << findTeamCompositionId(potentialComposition) << ".");
                return false;   
            }
            // add possible value, assign its id (based on the composition values)
            addPossibleTeamComposition(potentialComposition);
        }
        eastl::string buf;
        TRACE_LOG("[TeamCompositionRuleDefinition].parsePossibleValuesAndInitAllTeamCompositions initialized all possible TeamCompositions configured for rule " << getName() << " : " << "\n\t" << teamCompositionVectorToLogStr(mPossibleTeamCompositionVector, buf, false));
        return true;
    }

    /*! ************************************************************************************************/
    /*! \param[in] possVal String form representing the team composition. pre: format is e.g. "3,2,1"
    *************************************************************************************************/
    bool TeamCompositionRuleDefinition::possibleValueStringToTeamComposition(const ParamValue& possVal, TeamComposition& composition) const
    {
        if (possVal.empty())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].possibleValueStringToTeamComposition Configuration could not parse possible team composition value '" << possVal.c_str() << "' due to invalid format. Format must be numbers separated in between by commas.");
            return false;
        }

        // parse the number strings out
        eastl::vector<eastl::string> numberStrings;
        eastl::string numberStringBuf;
        bool gotFirstRelevantChar = false;
        for (EA::TDF::TdfStringLength i = 0; i < possVal.length(); ++i)
        {
            // validate format must be numbers separated by commas (with optional whitespace)
            const char8_t c = possVal[i];
            if ((c == ',') && !gotFirstRelevantChar)// first character can't be ','
            {
                ERR_LOG("[TeamCompositionRuleDefinition].possibleValueStringToTeamComposition Configuration could not parse possible team composition value '" << possVal.c_str() << "' due to first relevant character being invalid value ('" << c << "'). Format must be numbers separated in between by commas.");
                return false;
            }    
            if ((c == ' ') || (c == '\t') || (c == ','))
            {
                continue;
            }
            if ((c < '0') || (c > '9'))
            {
                ERR_LOG("[TeamCompositionRuleDefinition].possibleValueStringToTeamComposition Configuration could not parse possible team composition value '" << possVal.c_str() << "' due to invalid format. Format must be numbers separated in between by commas.");
                return false;
            }
            gotFirstRelevantChar = true;

            numberStringBuf.append_sprintf("%c", c);

            // if reached end of a number string, store to list, and reset buffer for next one
            if ((i+1 == possVal.length()) || (possVal[i+1] == ','))
            {
                numberStrings.push_back(numberStringBuf);
                numberStringBuf.clear();
            }
        }

        // convert number strings to integers and store them to the group-size sorted composition
        const size_t numberStringsCount = numberStrings.size();
        for (size_t i = 0; i < numberStringsCount; ++i)
        {
            uint16_t groupSize;
            if (blaze_str2int(numberStrings[i].c_str(), &groupSize) == numberStrings[i].c_str())
            {
                ERR_LOG("[TeamCompositionRuleDefinition].possibleValueStringToTeamComposition Configuration could not parse possible team composition value '" << possVal.c_str() << "' due to invalid format of its 'number string' '" << numberStrings[i].c_str() << "'. Format must be numbers separated in between by commas.");
                return false;
            }
            composition.push_back(groupSize);
        }
        sortTeamComposition(composition);
        return true;
    }

    /*! \brief Convert a TeamComposition back to its configuration possible value string */
    const char8_t* TeamCompositionRuleDefinition::teamCompositionToPossibleValueString(const TeamComposition& composition, eastl::string& possVal) const
    {
        const size_t numGroupsInComposition = composition.size();
        for (size_t j = 0; j < numGroupsInComposition; ++j)
            possVal.append_sprintf("%u%s", composition[j], ((j+1 < numGroupsInComposition)? "," : ""));
        return possVal.c_str();
    }

    
    /*! ************************************************************************************************/
    /*! \brief Add the team composition as a possible value for this rule
    ***************************************************************************************************/
    void TeamCompositionRuleDefinition::addPossibleTeamComposition(const TeamComposition& compositionValues)
    {
        mPossibleTeamCompositionVector.resize(mPossibleTeamCompositionVector.size()+1);//note: nodes non-POD, avoid push_back_uninitialized
        TeamComposition& newComposition = mPossibleTeamCompositionVector.back();
        newComposition = compositionValues;
    }
    
    /*! ************************************************************************************************/
    /*! \brief Generate GameTeamCompositionsId's for every permutation of TeamCompositions possible in games, from config.
        To minimize overhead during actual matchmaking, we cache that id plus relevant data for these permutations
        into 'GameTeamCompositionsInfo's owned by this rule definition, at startup.

        Pre: the team count, team capacity and possible TeamComposition's for this rule have been parsed.

        NOTE: to simplify CG/FG algorithms (and since fit tables can have different fit percents in different directions),
        we do NOT treat as dupes mirroring's like 'a vs b' and 'b vs a', (if a != b). We add both as *separate* GameTeamCompositionsInfo's.

    ***************************************************************************************************/
    bool TeamCompositionRuleDefinition::addPossibleGameTeamCompositionsInfos()
    {
        // pre: the possible team compositions already populated
        if (mPossibleTeamCompositionVector.empty())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].addPossibleGameTeamCompositionsInfos internal error: internal team compositions possible values(count=" << mPossibleTeamCompositionVector.size() << ") not yet initialized or empty. Unable to process team compositions.");
            return false;
        }

        // recursively add all game team compositions infos, to possible gtc infos vector
        TeamCompositionVector tcVectorPrefix;
        if (!addNewUnidentifiedGameTeamCompositionsInfosForPrefix(tcVectorPrefix))
            return false;

        // we've gotten all GameTeamCompositions populated, but without their ids. Sort the owning vector, to ensure we always
        // access them in the preferred order, and for convenience set their ids as their indices in the vector, below.
        eastl::sort(mPossibleGameTeamCompositionsVector.begin(), mPossibleGameTeamCompositionsVector.end(), GameTeamCompositionsInfoComparitor());
        
        // assign new GameTeamCompositionsId's
        for (size_t i = 0; i < mPossibleGameTeamCompositionsVector.size(); ++i)
        {
            GameTeamCompositionsInfo& gtcInfo = mPossibleGameTeamCompositionsVector[i];
            gtcInfo.mGameTeamCompositionsId = i;
        }

        eastl::string buf;
        TRACE_LOG("[TeamCompositionRuleDefinition].addPossibleGameTeamCompositionsInfos initialized all possible GameTeamCompositions configured for rule " << getName() << " (group sizes 1-" << getTeamCapacityForRule() << ") and sorted them by general match preference: " << gameTeamCompositionsInfoVectorToLogStr(mPossibleGameTeamCompositionsVector, buf, true));
        return true;
    }

    /*! \brief Helper to recursively build up the team compositions vector, and at 'team count' length add the game team compositions info.*/
    bool TeamCompositionRuleDefinition::addNewUnidentifiedGameTeamCompositionsInfosForPrefix(const TeamCompositionVector& tcVectorPrefix)
    {
        if (tcVectorPrefix.size() == getTeamCount())
        {
            //base
            return addNewUnidentifiedGameTeamCompositionsInfo(tcVectorPrefix);
        }
        for (size_t i = 0; i < mPossibleTeamCompositionVector.size(); ++i)
        {
            //side: for simplicity just copy. performance ignored as this is only called at server startup.
            TeamCompositionVector nextTcVectorPrefix = tcVectorPrefix;
            nextTcVectorPrefix.push_back(mPossibleTeamCompositionVector[i]);
            if (!addNewUnidentifiedGameTeamCompositionsInfosForPrefix(nextTcVectorPrefix))
                return false;
        }
        return true;
    }
    
    /*! ************************************************************************************************/
    /*! \brief add a new 'GameTeamCompositionsInfo' to the possible GameTeamCompositionsInfoVector, filling in all its
        data except GameTeamCompositionsId (caller expected to fill that in later).
        Caches off group sizes info cache for the set of TeamCompositions, as well as its fit percent etc.
    ***************************************************************************************************/
    bool TeamCompositionRuleDefinition::addNewUnidentifiedGameTeamCompositionsInfo(const TeamCompositionVector& tcVector)
    {
        // validate its ok before adding
        if ((tcVector.size() != getTeamCount()) || tcVector.empty())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].addNewUnidentifiedGameTeamCompositionsInfo internal error, team composition specified with team count " << tcVector.size() << " does not match rules valid team count " << getTeamCount() << ", for rule " << getName() << ".");
            return false;
        }
        if (isPossibleGameTeamCompositionsForRule(tcVector))
        {
            eastl::string buf;
            ERR_LOG("[TeamCompositionRuleDefinition].addNewUnidentifiedGameTeamCompositionsInfo possible internal error or configuration error, invalid duplicates detected: attempted to initialize a second instance of already-initialized GameTeamCompositions for rule " << getName() << ": " << teamCompositionVectorToLogStr(tcVector, buf));
            return false;
        }

        // add the cached info. Do NOT set the id's yet, we will do that after all added and we've sorted the vector.
        mPossibleGameTeamCompositionsVector.resize(mPossibleGameTeamCompositionsVector.size() + 1);//note: nodes non-POD, avoid push_back_uninitialized
        GameTeamCompositionsInfo& newCachedInfo = mPossibleGameTeamCompositionsVector.back();
        newCachedInfo.mTeamCompositionVector = tcVector;

        newCachedInfo.mCachedFitPercent = getFitPercent(newCachedInfo.mTeamCompositionVector);
        newCachedInfo.mIsExactMatch = areAllTeamCompositionsEqual(newCachedInfo);

        // Cache off quick-access hash map of group sizes for the new game team compositions info.
        // Note: this tdf-based info is owned outside the game team compositions info class (which just refs it),
        // as that class needs to be sortable in vectors (and thus all-members-copyable), and tdf's cannot be copied (gets compile error).
        newCachedInfo.mCachedGroupSizeCountsByTeam = addNewGroupSizeCountMapByTeamList(newCachedInfo.mTeamCompositionVector);
        return ((newCachedInfo.getCachedGroupSizeCountsByTeam() != nullptr) && !newCachedInfo.getCachedGroupSizeCountsByTeam()->empty());
    }

    /*! ************************************************************************************************/
    /*! \brief Create tdf's holding group sizes related cached info for the game team compositions' vector of team compositions.
        Owned by this rule definition to ensure they remain valid when referenced by pointers in GameTeamCompositionInfo's.
    ***************************************************************************************************/
    const GroupSizeCountMapByTeamList*
        TeamCompositionRuleDefinition::addNewGroupSizeCountMapByTeamList(const TeamCompositionVector& teamCompositionVector)
    {
        if (teamCompositionVector.empty())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].addNewGroupSizeCountMapByTeamList internal error, internal team compositions possible values not yet initialized or empty. Unable to process team compositions.");
            return nullptr;
        }
        eastl::string buf, buf2;

        GroupSizeCountMapByTeamList& groupSizeCountsByTeamList = *mPossibleGtcGroupSizeCountInfosVector.pull_back();
        groupSizeCountsByTeamList.resize(teamCompositionVector.size());
        for (size_t teamIndex = 0; teamIndex < teamCompositionVector.size(); ++teamIndex)
        {
            const TeamComposition& tc = teamCompositionVector[teamIndex];
            if (tc.empty())
            {
                ERR_LOG("[TeamCompositionRuleDefinition].addNewGroupSizeCountMapByTeamList internal error, internal team compositions possible values not yet initialized or empty for composition '" << teamCompositionToLogStr(tc, buf) << "'. Unable to process team compositions.");
                GroupSizeCountMapByTeamListList::iterator itr = mPossibleGtcGroupSizeCountInfosVector.end();
                mPossibleGtcGroupSizeCountInfosVector.erase(--itr);//cleanup
                return nullptr;
            }
            // cache group size counts
            for (size_t groupIndex = 0; groupIndex < tc.size(); ++groupIndex)
            {
                const uint16_t groupSize = tc[groupIndex];
                incrementGroupSizeCountInMap(groupSize, *groupSizeCountsByTeamList[teamIndex], 1);
            }
        }

        SPAM_LOG("[TeamCompositionRuleDefinition].addNewGroupSizeCountMapByTeamList " << teamCompositionVectorToLogStr(teamCompositionVector, buf) << ", got cached group size datas: " << groupSizeCountMapByTeamListToLogStr(groupSizeCountsByTeamList, buf2) << ".");
        return &groupSizeCountsByTeamList;
    }

    /*! ************************************************************************************************/
    /*! \brief for quick access during MM rule evaluations, cache off info about possible acceptable
        game team compositions for the rule for the different possible MM sessions group sizes and fit thresholds.
    ***************************************************************************************************/
    bool TeamCompositionRuleDefinition::cacheGroupAndFitThresholdMatchInfos()
    {
        // pre: the possible game team compositions already populated
        if (mPossibleGameTeamCompositionsVector.empty())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].cacheGroupAndFitThresholdMatchInfos internal error: internal game team compositions possible values(count=" << mPossibleGameTeamCompositionsVector.size() << ") not yet initialized or empty. Unable to process team compositions.");
            return false;
        }
        if (mMinFitThresholdListContainer.empty() || mRuleConfigTdf.getMinFitThresholdLists().empty())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].cacheGroupAndFitThresholdMatchInfos internal error: internal mMinFitThresholdListContainer(count=" << mPossibleGameTeamCompositionsVector.size() << ") or mRuleConfigTdf.getMinFitThresholdLists()(count=" << mRuleConfigTdf.getMinFitThresholdLists().size() << " not yet initialized or empty. Unable to process team compositions.");
            return false;
        }
        if (getTeamCapacityForRule() == 0)
        {
            ERR_LOG("[TeamCompositionRuleDefinition].cacheGroupAndFitThresholdMatchInfos internal error: maxTeamCapacity not yet initialized or zero. Unable to process team compositions.");
            return false;
        }

        // for all configured min fit threshold lists
        for (FitThresholdsMap::const_iterator itr = mRuleConfigTdf.getMinFitThresholdLists().begin(); itr != mRuleConfigTdf.getMinFitThresholdLists().end(); ++itr)
        {
            const MinFitThresholdList* minFitThreshList = mMinFitThresholdListContainer.getMinFitThresholdList(itr->first);
            if ((minFitThreshList == nullptr) || (minFitThreshList->getSize() == 0))
            {
                ERR_LOG("[TeamCompositionRuleDefinition].cacheGroupAndFitThresholdMatchInfos internal error: internal minFitThreshList(name=" << itr->first << ") not yet initialized or empty. Unable to process team compositions.");
                return false;
            }
            // for every fit threshold in the list
            for (uint32_t i = 0; i < minFitThreshList->getSize(); ++i)
            {
                // for every possible group size
                float fitThresh = minFitThreshList->getMinFitThresholdByIndex(i);
                for (uint16_t groupSize = 1; groupSize <= getTeamCapacityForRule(); ++groupSize)
                {
                    // init the respective cache
                    if (!cacheGroupAndFitThresholdMatchInfos(groupSize, fitThresh))
                        return false;
                }
            }
        }
        eastl::string buf;
        TRACE_LOG("[TeamCompositionRuleDefinition].cacheGroupAndFitThresholdMatchInfos initialized acceptable game team compositions info caches for rule " << getName() << ":" << matchInfoCacheByGroupSizeAndMinFitToLogStr(buf) << ".");
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief helper adds the new cache item as needed (see caller)
    *************************************************************************************************/
    bool TeamCompositionRuleDefinition::cacheGroupAndFitThresholdMatchInfos(uint16_t groupSize, float fitThreshold)
    {
        // to avoid dupes, we'll use set of strings for convenience.
        CompositionPossibleValueSet myTeamCompositionsAsyncStatusStrSet;
        CompositionPossibleValueSet otherTeamCompositionsAsyncStatusStrSet;

        // scan every possible game team compositions, and add to the cache item's acceptable list as appropriate
        const bool isExactMatchRequired = (fitThreshold == MinFitThresholdList::EXACT_MATCH_REQUIRED);
        for (size_t i = 0; i < mPossibleGameTeamCompositionsVector.size(); ++i)
        {
            const GameTeamCompositionsInfo& gtcInfo = mPossibleGameTeamCompositionsVector[i];
            if ((gtcInfo.getCachedGroupSizeCountsByTeam() == nullptr) || gtcInfo.getCachedGroupSizeCountsByTeam()->empty() || (gtcInfo.mCachedFitPercent == FIT_PERCENT_NO_MATCH))
            {
                ERR_LOG("[TeamCompositionRuleDefinition].cacheGroupAndFitThresholdMatchInfos internal error: internal associated cached " << ((gtcInfo.mCachedFitPercent == FIT_PERCENT_NO_MATCH)? "fit percent" : "group size data") << " not yet initialized or empty, for " << gtcInfo.toFullLogStr() << ". Unable to process team compositions.");
                return false;
            }
            // exact match requires all game's team compositions identical
            if (isExactMatchRequired && !gtcInfo.mIsExactMatch)
            {
                continue;
            }
            // if group size is in the game team compositions, it should be in the acceptable list for this cache item.
            const bool isGroupSizePresent = hasGroupSizeInGroupSizeCountMaps(groupSize, *gtcInfo.getCachedGroupSizeCountsByTeam());
            if (isGroupSizePresent && (gtcInfo.mCachedFitPercent >= fitThreshold))
            {
                if (mMatchInfoCacheByGroupSizeAndMinFit.size() < (size_t)(groupSize + 1))
                {
                    // add new item for the group size. Note: index 0 kept for groupSize 0 just so things line up (readability and so its like a map)
                    mMatchInfoCacheByGroupSizeAndMinFit.resize(groupSize + 1);
                }

                // get the cache item to add the gtc to
                GroupAndFitThresholdMatchInfoCache& cacheItem = mMatchInfoCacheByGroupSizeAndMinFit[groupSize][fitThreshold];

                if (INVALID_GAME_TEAM_COMPOSITONS_ID != findGameTeamCompositionId(gtcInfo.mTeamCompositionVector,
                    cacheItem.mAcceptableGameTeamCompositionsVector, false))
                {
                    // we've already added this item, on a different min fit threshold list (with common fit thresholds). Skip dupe.
                    continue;
                }
                // add its basic data
                populateGroupAndFitThresholdMatchInfoCache(cacheItem, gtcInfo, groupSize, fitThreshold);
                // cache its group sizes. ensure only includes my groupSize here if a game's compositions included more than one instance of my size.
                populateGroupSizeSetCache(cacheItem.mAcceptableOtherSessionGroupSizesSet, gtcInfo, groupSize);
                // cache its async statuses data. Note already-added team compositions (dupes) are omitted
                populateAsyncStatusCache(cacheItem.mTeamCompositionRuleStatus, gtcInfo, groupSize, myTeamCompositionsAsyncStatusStrSet, otherTeamCompositionsAsyncStatusStrSet);
            }
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \param[in,out] cache Add data for this cache item, from the given game team compositions info.
    *************************************************************************************************/
    void TeamCompositionRuleDefinition::populateGroupAndFitThresholdMatchInfoCache(
        GroupAndFitThresholdMatchInfoCache& cache, const GameTeamCompositionsInfo& gtcInfo,
        uint16_t groupSize, float fitThreshold) const
    {
        //ensure cached its fit threshold and groupSize (if not already)
        cache.mMinFitThreshold = fitThreshold;
        cache.mMyGroupSize = groupSize;

        //cache the compositions to list. ensure list always sorted. note: ok to re-sort every iteration, this fn is only called at server startup, performance doesn't need consideration here
        cache.mAcceptableGameTeamCompositionsVector.push_back(gtcInfo);
        eastl::sort(cache.mAcceptableGameTeamCompositionsVector.begin(), cache.mAcceptableGameTeamCompositionsVector.end(), GameTeamCompositionsInfoComparitor());

        //cache min fit percent
        if ((cache.mFitPercentMin == FIT_PERCENT_NO_MATCH) || (gtcInfo.mCachedFitPercent < cache.mFitPercentMin))
        {
            cache.mFitPercentMin = gtcInfo.mCachedFitPercent;
        }
        cache.mAllExactMatches = areAllTeamCompositionsEqual(gtcInfo);
    }

    /*! ************************************************************************************************/
    /*! \brief retrieve set of all the group sizes present in the GameTeamCompositionsInfo.
        \param[in] ignoreFirstInstanceOfSize will only add this group size to the groupSizeSet,
            if the GameTeamCompositions includes more than once instance of this value.
    *************************************************************************************************/
    void TeamCompositionRuleDefinition::populateGroupSizeSetCache(GroupSizeSet& groupSizeSet, const GameTeamCompositionsInfo& gtcInfo,
        uint16_t ignoreFirstInstanceOfSize) const
    {
        if (gtcInfo.mTeamCompositionVector.empty())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].populateGroupSizeSetCache internal error, internal team compositions possible values not yet initialized or empty, for GameTeamCompositionsInfo(" << gtcInfo.toFullLogStr() << "). Unable to process team compositions.");
            return;
        }
        bool foundSizeToIgnore = false;
        for (size_t i = 0; i < gtcInfo.mTeamCompositionVector.size(); ++i)
        {
            const TeamComposition& tc = gtcInfo.mTeamCompositionVector[i];
            for (size_t j = 0; j < tc.size(); ++j)
            {
                if ((tc[j] == ignoreFirstInstanceOfSize) && !foundSizeToIgnore)
                {
                    foundSizeToIgnore = true;
                    continue;
                }
                groupSizeSet.insert(tc[j]);
            }
        }
    }
    /*! ************************************************************************************************/
    /*! \brief retrieve async status notification info for the GameTeamCompositionsInfo
        \param[in,out] asyncStatus adds results into this
    *************************************************************************************************/
    void TeamCompositionRuleDefinition::populateAsyncStatusCache(TeamCompositionRuleStatus& asyncStatus,
        const GameTeamCompositionsInfo& gtcInfo, uint16_t myGroupSize,
        CompositionPossibleValueSet& myTeamCompositionsAsyncStatusStrSet,
        CompositionPossibleValueSet& otherTeamCompositionsAsyncStatusStrSet) const
    {
        eastl::string teamCompBuf;
        asyncStatus.setRuleName(getName());

        for (size_t teamIndex = 0; teamIndex < gtcInfo.mTeamCompositionVector.size(); ++teamIndex)
        {
            if ((gtcInfo.getCachedGroupSizeCountsByTeam() == nullptr) || (gtcInfo.getCachedGroupSizeCountsByTeam()->size() <= teamIndex))
            {
                ERR_LOG("[TeamCompositionRuleDefinition].populateAsyncStatusCache internal error, internal team compositions possible values not yet initialized or empty, for GameTeamCompositionsInfo(" << gtcInfo.toFullLogStr() << "). Unable to process team compositions.");
                return;
            }

            const TeamComposition& tc = gtcInfo.mTeamCompositionVector[teamIndex];

            // find the team(s) I'm on. Each time, add my team for my list, and all other teams to the others list
            if ((findGroupSizeCountInMap(myGroupSize, *(*gtcInfo.getCachedGroupSizeCountsByTeam())[teamIndex]) > 0))
            {
                // add 'my' team's compositions
                teamCompBuf.clear();
                if ((teamCompositionToPossibleValueString(tc, teamCompBuf) != nullptr) &&
                    (myTeamCompositionsAsyncStatusStrSet.find(teamCompBuf) == myTeamCompositionsAsyncStatusStrSet.end()))
                {
                    myTeamCompositionsAsyncStatusStrSet.insert(teamCompBuf);
                    TeamCompositionStatus* tcStatus = asyncStatus.getAcceptableCompositionsForMyTeam().pull_back();
                    tcStatus->reserve(tc.size());
                    for (size_t groupIndex = 0; groupIndex < tc.size(); ++groupIndex)
                        tcStatus->push_back(tc[groupIndex]);
                }

                // add 'other' team's compositions
                for (size_t teamIndex2 = 0; teamIndex2 < gtcInfo.mTeamCompositionVector.size(); ++teamIndex2)
                {
                    if (teamIndex2 == teamIndex)
                        continue;

                    const TeamComposition& tc2 = gtcInfo.mTeamCompositionVector[teamIndex2];
                    teamCompBuf.clear();
                    if ((teamCompositionToPossibleValueString(tc2, teamCompBuf) != nullptr) &&
                        (otherTeamCompositionsAsyncStatusStrSet.find(teamCompBuf) == otherTeamCompositionsAsyncStatusStrSet.end()))
                    {
                        otherTeamCompositionsAsyncStatusStrSet.insert(teamCompBuf);
                        TeamCompositionStatus* tcStatus = asyncStatus.getAcceptableCompositionsForOtherTeams().pull_back();
                        tcStatus->reserve(tc2.size());
                        for (size_t groupIndex = 0; groupIndex < tc2.size(); ++groupIndex)
                            tcStatus->push_back(tc2[groupIndex]);
                    }
                }
            }
        }
        
    }

    /*! ************************************************************************************************/
    /*! \brief get the cached acceptable game team compositions list and its related info, or nullptr on failure.
        \param[in] groupSize the calling MM session's group size
        \param[in] fitThreshold the calling MM session's current min fit threshold
    ***************************************************************************************************/
    const TeamCompositionRuleDefinition::GroupAndFitThresholdMatchInfoCache*
        TeamCompositionRuleDefinition::getMatchInfoCacheByGroupSizeAndMinFit(uint16_t groupSize, float fitThreshold) const
    {
        const GroupAndFitThresholdMatchInfoCacheByMinFitMap* matchInfoCacheByFitMap = getMatchInfoCacheByMinFitMap(groupSize);
        if (matchInfoCacheByFitMap == nullptr)
            return nullptr;//logged
    
        GroupAndFitThresholdMatchInfoCacheByMinFitMap::const_iterator itr = matchInfoCacheByFitMap->find(fitThreshold);
        if (itr == matchInfoCacheByFitMap->end())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].getMatchInfoCacheByGroupSizeAndMinFit possible internal error: fitThreshold '" << fitThreshold << "' was not found in cached fitThreshold for group size " << groupSize << ". Cannot retrieve acceptable game team compositions infos.");
            return nullptr;
        }
        return &itr->second;
    }
    /*! ************************************************************************************************/
    /*! \brief get the cached acceptable game team compositions lists, sorted by fit thresholds, or nullptr on failure.
        \param[in] groupSize the calling MM session's group size
    ***************************************************************************************************/
    const TeamCompositionRuleDefinition::GroupAndFitThresholdMatchInfoCacheByMinFitMap*
        TeamCompositionRuleDefinition::getMatchInfoCacheByMinFitMap(uint16_t groupSize) const
    {
        if (mMatchInfoCacheByGroupSizeAndMinFit.size() < (size_t)(groupSize + 1))
        {
            ERR_LOG("[TeamCompositionRuleDefinition].getMatchInfoCacheByMinFitMap possible internal error: group size '" << groupSize << "' is beyond the max possible cached group size " << (mMatchInfoCacheByGroupSizeAndMinFit.empty()? 0 : (mMatchInfoCacheByGroupSizeAndMinFit.size() -1)) << ". . Cannot retrieve acceptable game team compositions infos.");
            return nullptr;
        }
        return &mMatchInfoCacheByGroupSizeAndMinFit[groupSize];
    }

    /*! ************************************************************************************************/
    /*! \brief get max team capacity based on the parsed compositions. Validates whether all possible compositions
        for rule add up to a common team capacity, and that the team capacity is greater than 1.
        \return the max team capacity value, or 0 if aforementioned validation failed or there was an error.
    *************************************************************************************************/
    uint16_t TeamCompositionRuleDefinition::calcTeamCapacityForRule(const TeamCompositionVector &possibleValues) const
    {
        if (possibleValues.empty())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].calcTeamCapacityForRule internal error: internal team compositions possible values not yet initialized or empty. Unable to process team compositions.");
            return 0;
        }
        eastl::string buf;

        bool areAllCompositionsSameCapacity = true;
        uint16_t maxTeamCapacity = 0;
        for (size_t i = 0; i < possibleValues.size(); ++i)
        {
            // check whether all possible values add up to a common maximum team capacity. Warn if possible misconfiguration.
            if (possibleValues[i].empty())
            {
                ERR_LOG("[TeamCompositionRuleDefinition].calcTeamCapacityForRule internal error: internal team compositions possible values not yet initialized or empty for composition(" << teamCompositionToLogStr(possibleValues[i], buf) << "). Unable to process team compositions.");
                return 0;
            }
            const uint16_t sum = calcTeamCompositionSizesSum(possibleValues[i]);
            if ((maxTeamCapacity != 0) && (maxTeamCapacity != sum))
            {
                ERR_LOG("[TeamCompositionRuleDefinition].calcTeamCapacityForRule Configuration has inconsistenly sized teams detected, in the possible values section. This can lead to unbalanced teams. The possible value composition [" << teamCompositionToLogStr(possibleValues[i], buf) << "] has a sum of sizes " << sum << " which does not equal a prior sum of another compositions sizes " << maxTeamCapacity << ".");
                areAllCompositionsSameCapacity = false;
            }
            if (maxTeamCapacity < sum)
                maxTeamCapacity = sum;
        }
        if (maxTeamCapacity <= 1)
        {
            ERR_LOG("[TeamCompositionRuleDefinition].calcTeamCapacityForRule Configuration of possibleValues specifies a teamCapacity " << maxTeamCapacity << ".which is less than the minimum required of 2 for TeamCompositionRule.");
            return 0;
        }
        return (areAllCompositionsSameCapacity? maxTeamCapacity : 0);
    }

    /*! \brief get the sum of composition's sizes */
    uint16_t TeamCompositionRuleDefinition::calcTeamCompositionSizesSum(const TeamComposition& composition) const
    {
        uint16_t sum = 0;
        for (size_t i = 0; i < composition.size(); ++i)
        {
            sum += composition[i];
        }
        return sum;
    }
    
    /*! ************************************************************************************************/
    /*! \brief cache the mathematically possible compositions given the slot capacity (see called helper),
            for all capacities up to the specified max. Used by startup's config validation, and, by
            FG eval's of games that wouldn't be full after a session joins (to check poss outcomes on the vacant space,see FG).
        \param[in] maxTeamCapacity Do for each possible capacity up to this (each poss cap gets its own cache).
    *************************************************************************************************/
    bool TeamCompositionRuleDefinition::cacheMathematicallyPossibleCompositionsForAllCapacitiesUpToMax(uint16_t maxTeamCapacity)
    {
        if (maxTeamCapacity == 0)//pre: team capacity for rule was initialized
        {
            ERR_LOG("[TeamCompositionRuleDefinition].cacheMathematicallyPossibleCompositionsForAllCapacitiesUpToMax internal error: maxTeamCapacity not yet initialized or zero. Unable to process team compositions.");
            return false;
        }
        for (uint16_t capacity = 1; capacity <= maxTeamCapacity; ++capacity)
        {
            cacheMathematicallyPossibleCompositionsForCapacity(capacity, mCapacityToMathematicallyPossCompositionsMap[capacity]);
        }
        return true;
    }
    /*! ************************************************************************************************/
    /*! \brief cache the mathematically possible compositions given the slot capacity,
            i.e. all possible sequences of positive integers (group sizes) adding up to the capacity.
        \param[out] compositionPossibleValueSet The result set of team compositions, as possible value strings
            (for log readability). Note: using a set to ensure no dupes.
    *************************************************************************************************/
    void TeamCompositionRuleDefinition::cacheMathematicallyPossibleCompositionsForCapacity(uint16_t capacity,
        CompositionPossibleValueSet& compositionPossibleValueSet) const
    {
        if (capacity == 0)
        {
            return;
        }
        TeamComposition c0;
        TeamComposition c1;
        TeamComposition c2;
        TeamComposition c3;
        c1.push_back(capacity);
        populateMathematicallyPossibleCompositions(c0, c1, c2, c3, compositionPossibleValueSet);
        
        //log
        eastl::string bufPoss;
        TRACE_LOG("[TeamCompositionRuleDefinition].cacheMathematicallyPossibleCompositionsForCapacity for capacity " << capacity << " (unsorted) " << compositionPossibleValueSetToString(compositionPossibleValueSet, bufPoss) << ".");
    }

    /*! ************************************************************************************************/
    /*! \brief helper to retrieve the possible team composition permutations based on specified subsequences of the group sizes.

        \param[in] prefix group sizes to tack onto the front of the team compositions we'll retrieve (see body).
        \param[in] part1 sub-part of the next team composition to get, and then recursively break up into further
            sub-parts to add subsequent possible compositions for (see body).
    *************************************************************************************************/
    void TeamCompositionRuleDefinition::populateMathematicallyPossibleCompositions(const TeamComposition& prefix,
        const TeamComposition& part1, const TeamComposition& part2, const TeamComposition& suffix,
        CompositionPossibleValueSet& compositionPossibleValueSet) const
    {
        if (part1.empty() && part2.empty() && prefix.empty() && suffix.empty())
            return;

        // 1. Base: this frame adds the composition which is the concat of (prefix + part1 + part2 + suffix).
        TeamComposition base;
        base.reserve(prefix.size() + part1.size() + part2.size() + suffix.size());
        base.insert(base.end(), prefix.begin(), prefix.end());
        base.insert(base.end(), part1.begin(), part1.end());
        base.insert(base.end(), part2.begin(), part2.end());
        base.insert(base.end(), suffix.begin(), suffix.end());
        sortTeamComposition(base);
        eastl::string buf;
        teamCompositionToPossibleValueString(base, buf);
        compositionPossibleValueSet.insert(buf);

        // Recursively break up part1 and part2 into sub-parts and add the appropriate possible compositions for those.

        // 2a. recursively break up part1 into subsizes that add up to it.
        TeamComposition newPrefix;
        TeamComposition newSuffix;

        // setup the common pre-fix and suffix parts
        newPrefix.insert(newPrefix.end(), prefix.begin(), prefix.end());
        newSuffix.insert(newSuffix.end(), part2.begin(), part2.end());// (don't forget to pre-pend the old part 2 to the suffix)
        newSuffix.insert(newSuffix.end(), suffix.begin(), suffix.end());
        sortTeamComposition(newPrefix);
        sortTeamComposition(newSuffix);

        for (size_t i = 0; i < part1.size(); ++i)
        {
            uint16_t part1Size = part1[i];
            for (uint16_t j = 1; j < part1Size; ++j)
            {
                // recursively break up part1 into 2 subsizes that add up to it
                uint16_t nextpart1Size = (part1Size - j);
                uint16_t nextpart2Size = (part1Size - nextpart1Size);

                TeamComposition c1;
                c1.push_back(nextpart1Size);
                TeamComposition c2;
                c2.push_back(nextpart2Size);

                populateMathematicallyPossibleCompositions(newPrefix, c1, c2, newSuffix, compositionPossibleValueSet);
            }
        }

        // 2b. recursively break up part2 into subsizes that add up to it
        newPrefix.clear();
        newSuffix.clear();

        // setup the common pre-fix and suffix parts
        newPrefix.insert(newPrefix.end(), prefix.begin(), prefix.end());
        newPrefix.insert(newPrefix.end(), part1.begin(), part1.end());// (don't forget to append the old part 1 to prefix)
        newSuffix.insert(newSuffix.end(), suffix.begin(), suffix.end());
        sortTeamComposition(newPrefix);
        sortTeamComposition(newSuffix);

        for (size_t i = 0; i < part2.size(); ++i)
        {
            uint16_t part2Size = part2[i];
            for (uint16_t j = 1; j < part2Size; ++j)
            {
                // recursively break up part2 into 2 subsizes that add up to it
                uint16_t nextpart1Size = (part2Size - j);
                uint16_t nextpart2Size = (part2Size - nextpart1Size);

                TeamComposition c1;
                c1.push_back(nextpart1Size);
                TeamComposition c2;
                c2.push_back(nextpart2Size);

                populateMathematicallyPossibleCompositions(newPrefix, c1, c2, newSuffix, compositionPossibleValueSet);
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief validates that all mathematically possible team compositions for a known max team capacity
        were specified as possible values in the matchmaker configuration for the rule.
    *************************************************************************************************/
    bool TeamCompositionRuleDefinition::checkMathematicallyPossibleCompositionsArePossibleValues() const
    {
        if ((getTeamCapacityForRule() == 0) || mPossibleTeamCompositionVector.empty())//pre: team capacity for rule was initialized
        {
            ERR_LOG("[TeamCompositionRuleDefinition].checkMathematicallyPossibleCompositionsArePossibleValues internal error: team compositions max team capacity or rule possible values not yet initialized. Unable to process team compositions.");
            return false;
        }

        if (mCapacityToMathematicallyPossCompositionsMap.size() < getTeamCapacityForRule())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].checkMathematicallyPossibleCompositionsArePossibleValues internal error: the rules teamCapacity " << getTeamCapacityForRule() << " is less than the max possible teamCapacity " << mCapacityToMathematicallyPossCompositionsMap.size() << "for which we've cached mathematically possible compositions for. Unable to process team compositions.");
            return false;
        }
        const CompositionPossibleValueSet& possMathematicalSet = mCapacityToMathematicallyPossCompositionsMap[getTeamCapacityForRule()];

        // validate they are all possible values
        bool isPass = true;
        for (CompositionPossibleValueSet::const_iterator itr = possMathematicalSet.begin(); itr != possMathematicalSet.end(); ++itr)
        {
            TeamComposition c;
            if (!possibleValueStringToTeamComposition(itr->c_str(), c))
                return false;//logged

            if (!isPossibleCompositionForRule(c))
            {
                eastl::string buf;
                WARN_LOG("[TeamCompositionRuleDefinition].checkMathematicallyPossibleCompositionsArePossibleValues The possible value composition [" << teamCompositionToLogStr(c, buf) << "] was NOT a possible value specified by rule " << getName() << ". Configuration may have inadvertently missed including this composition as a possible value. If this composition can actually occur, it will not be possible to be matched.");
                isPass = false;
            }
        }
        return isPass;
    }

    /*! ************************************************************************************************/
    /*! \brief retrieve mathematically possible compositions (i.e. ordered group size sequences) given the team-capacity/slotCount.
    *************************************************************************************************/
    const TeamCompositionRuleDefinition::CompositionPossibleValueSet&
        TeamCompositionRuleDefinition::getMathematicallyPossibleCompositionsForSlotcount(uint16_t slotCount) const
    {
        CapacityToCompositionPossibleValueSetMap::iterator itr = mCapacityToMathematicallyPossCompositionsMap.find(slotCount);
        if ((itr == mCapacityToMathematicallyPossCompositionsMap.end())
            || ((slotCount != 0) && mCapacityToMathematicallyPossCompositionsMap[slotCount].empty()))
        {
            // Just in case this fn somehow called on a capacity not cached at startup (use case changes etc), recover by caching now.
            WARN_LOG("[TeamCompositionRuleDefinition].getMathematicallyPossibleCompositionsForSlotcount internal error: the composition capacity specified " << slotCount << " must be no less than the max possible teamCapacity " << mCapacityToMathematicallyPossCompositionsMap.size() << " for which we've cached mathematically possible compositions for, and its cache must be initialized (currently false). Attempting to recover by rebuilding cache (Note: if this error log appears often, performance will likely be negatively affected).");
            cacheMathematicallyPossibleCompositionsForCapacity(slotCount, mCapacityToMathematicallyPossCompositionsMap[slotCount]);
        }
        if (IS_LOGGING_ENABLED(Logging::TRACE))
        {
            eastl::string buf;
            TRACE_LOG("[TeamCompositionRuleDefinition].getMathematicallyPossibleCompositionsForSlotcount mathematically possible for slot count of " << slotCount << ": " << compositionPossibleValueSetToString(mCapacityToMathematicallyPossCompositionsMap[slotCount], buf) << ".");
        }
        return mCapacityToMathematicallyPossCompositionsMap[slotCount];
    }

    /*! ************************************************************************************************/
    /*! \brief insert RETE wmes. Note: actual full filtering is completed post-RETE (see).
    *************************************************************************************************/
    void TeamCompositionRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        if ((gameSessionSlave.getTeamCount() < 2) || (gameSessionSlave.getTeamCapacity() != getTeamCapacityForRule()))
        {
            TRACE_LOG("[TeamCompositionRuleDefinition].insertWorkingMemoryElement not adding WME for game(" << gameSessionSlave.getGameId() << "), must contain 2 or more teams (actual count " << gameSessionSlave.getTeamCount() << ") and its team capacity '" << gameSessionSlave.getTeamCapacity() << "' must equal this rules required team capacity '" << getTeamCapacityForRule() << "', for rule " << getName() << ".");
            return;
        }

        RoleSizeMap& cachedWmeNames = 
            const_cast<Search::GameSessionSearchSlave&>(gameSessionSlave).getMatchmakingGameInfoCache()->getTeamCompositionRuleWMENameList(*this);

        // 1. add wme for the most vacant space on a given team in the game, searchers look for anything from joiningCount to team capacity
        uint16_t teamCount = gameSessionSlave.getTeamCount();
        uint16_t mostSpace = 0;

        for (TeamIndex teamIndex = 0; teamIndex < teamCount; ++teamIndex)
        {
            uint16_t vacantSpace = (gameSessionSlave.getTeamCapacity() - gameSessionSlave.getPlayerRoster()->getTeamSize(teamIndex));

            if (mostSpace < vacantSpace)
            {
                mostSpace = vacantSpace;
            }
        }

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), TeamCompositionRuleDefinition::TEAMCOMPOSITIONRULE_TEAM_ATTRIBUTE_SPACE, mostSpace, *this);

        GroupSizeSet presentGroupSizeSet;
        for (TeamIndex i = 0; i < gameSessionSlave.getTeamCount(); ++i)
        {
            // 2. Add wmes for the present group sizes (so searcher can filter out games that violate its compositions).
            GroupSizeCountMap groupSizesOnTeam;
            gameSessionSlave.getPlayerRoster()->getTeamGroupSizeCountMap(groupSizesOnTeam, i);
            for (GroupSizeCountMap::const_iterator itr = groupSizesOnTeam.begin(); itr != groupSizesOnTeam.end(); ++itr)
            {
                if (presentGroupSizeSet.insert(itr->first).second)//add once
                {
                    eastl::string buf;
                    buf.append_sprintf("%s_%u", TeamCompositionRuleDefinition::TEAMCOMPOSITIONRULE_TEAM_ATTRIBUTE_GROUP_SIZE, itr->first);
                    cachedWmeNames[buf.c_str()] = itr->first;

                    wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), buf.c_str(), getWMEBooleanAttributeValue(true), *this);
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief upsert RETE wmes. Note: actual full filtering is completed post-RETE (see).
    *************************************************************************************************/
    void TeamCompositionRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        uint16_t teamCount = gameSessionSlave.getTeamCount();
        uint16_t mostSpace = 0;

        for (TeamIndex teamIndex = 0; teamIndex < teamCount; ++teamIndex)
        {
            uint16_t vacantSpace = (gameSessionSlave.getTeamCapacity() - gameSessionSlave.getPlayerRoster()->getTeamSize(teamIndex));

            if (mostSpace < vacantSpace)
            {
                mostSpace = vacantSpace;
            }
        }

        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), TeamCompositionRuleDefinition::TEAMCOMPOSITIONRULE_TEAM_ATTRIBUTE_SPACE, mostSpace, *this);
        

        cleanupRemovedWorkingMemoryElements(wmeManager, gameSessionSlave);
        
        RoleSizeMap& cachedWmeNames = 
            const_cast<Search::GameSessionSearchSlave&>(gameSessionSlave).getMatchmakingGameInfoCache()->getTeamCompositionRuleWMENameList(*this);

        GroupSizeSet presentGroupSizeSet;
        for (TeamIndex i = 0; i < gameSessionSlave.getTeamCount(); ++i)
        {
            // 2. Add wmes for the present group sizes (so searcher can filter out games that violate its compositions).
            GroupSizeCountMap groupSizesOnTeam;
            gameSessionSlave.getPlayerRoster()->getTeamGroupSizeCountMap(groupSizesOnTeam, i);
            for (GroupSizeCountMap::const_iterator itr = groupSizesOnTeam.begin(); itr != groupSizesOnTeam.end(); ++itr)
            {
                if (presentGroupSizeSet.insert(itr->first).second)//add once
                {
                    eastl::string buf;
                    buf.append_sprintf("%s_%u", TeamCompositionRuleDefinition::TEAMCOMPOSITIONRULE_TEAM_ATTRIBUTE_GROUP_SIZE, itr->first);

                    cachedWmeNames[buf.c_str()] = itr->first;

                    wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), buf.c_str(), getWMEBooleanAttributeValue(true), *this);
                }
            }
        }
    }

    void TeamCompositionRuleDefinition::cleanupRemovedWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        RoleSizeMap& cachedNameList = const_cast<Search::GameSessionSearchSlave&>(gameSessionSlave).getMatchmakingGameInfoCache()->getTeamCompositionRuleWMENameList(*this);
       
        // build set of existing group sizes
        GroupSizeSet presentGroupSizeSet;
        for (TeamIndex i = 0; i < gameSessionSlave.getTeamCount(); ++i)
        {
            GroupSizeCountMap groupSizesOnTeam;
            gameSessionSlave.getPlayerRoster()->getTeamGroupSizeCountMap(groupSizesOnTeam, i);
            for (GroupSizeCountMap::const_iterator itr = groupSizesOnTeam.begin(); itr != groupSizesOnTeam.end(); ++itr)
            {
                presentGroupSizeSet.insert(itr->first);
            }
        }

        // We need to remove any group sizes which no longer exist
        RoleSizeMap::const_iterator iter = cachedNameList.begin();
        while (iter != cachedNameList.end())
        {
            GroupSizeSet::const_iterator groupIter = presentGroupSizeSet.find(iter->second);
            if (groupIter == presentGroupSizeSet.end())
            {
                wmeManager.removeWorkingMemoryElement(gameSessionSlave.getGameId(), iter->first.c_str(), getWMEBooleanAttributeValue(true), *this);
                iter = cachedNameList.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief retrieve the fit percent for the game's team compositions. For this rule, the fit percent
        is calculated not by comparing vs other session/game's value directly, but by checking two
        possible team's compositions between each other in the possible game's team compositions for which
        both your MM session and the other session/game would be compatible with.

        Note: if there are more than two teams, returns the lowest fit percent between any two adjacent teams.
    *************************************************************************************************/
    float TeamCompositionRuleDefinition::getFitPercent(const TeamCompositionVector& teamCompositionVectorForGame) const
    {
        float lowestFitPercent = mFitTable.getMaxFitPercent();
        for (size_t i = 0; i < teamCompositionVectorForGame.size(); ++i)
        {
            // compare vs other teams
            for (size_t j = (i + 1); j < teamCompositionVectorForGame.size(); ++j)
            {
                //call below
                const TeamComposition& tci = teamCompositionVectorForGame[i];
                const TeamComposition& tcj = teamCompositionVectorForGame[j];
                float nextFitPercent = getFitPercent(findTeamCompositionId(tci), findTeamCompositionId(tcj));
                if (nextFitPercent == FIT_PERCENT_NO_MATCH)
                {
                    return FIT_PERCENT_NO_MATCH;
                }
                if (nextFitPercent < lowestFitPercent)
                {
                    lowestFitPercent = nextFitPercent;
                }
            }
        }
        return lowestFitPercent;
    }

    /*! ************************************************************************************************/
    /*! \brief retrieve the fit percent between the two team compositions, by their cached fit table indices
    *************************************************************************************************/
    float TeamCompositionRuleDefinition::getFitPercent(TeamCompositionId teamCompositionId1, TeamCompositionId teamCompositionId2) const
    {
        if (mPossibleTeamCompositionVector.empty())
        {
            ERR_LOG("[TeamCompositionRuleDefinition].getFitPercent internal error, internal team compositions possible values not yet initialized or empty. Unable to process team compositions.");
            return FIT_PERCENT_NO_MATCH;
        }

        const size_t numPossibleValues = mPossibleTeamCompositionVector.size();
        if ((teamCompositionId1 < 0) || (teamCompositionId2 < 0) ||
            ((size_t)teamCompositionId1 >= numPossibleValues) || ((size_t)teamCompositionId2 >= numPossibleValues))
        {
            ERR_LOG("[TeamCompositionRuleDefinition].getFitPercent internal error: team composition possible value indices must be valid and less than the possible team compositions vector size " << numPossibleValues << ". The indices specified were " << teamCompositionId1 << ", " << teamCompositionId2 << ".");
            return FIT_PERCENT_NO_MATCH;
        }
        return mFitTable.getFitPercent((size_t)teamCompositionId1, (size_t)teamCompositionId2);
    }

    void TeamCompositionRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        char8_t buf[4096];
        mRuleConfigTdf.print(buf, sizeof(buf));
        dest.append_sprintf("%s %s", prefix, buf);
    }

    /*! ************************************************************************************************/
    /*! \brief return the TeamComposition's id (unique per rule), or INVALID_TEAM_COMPOSITION_INDEX if its not a possible
        TeamComposition for the rule. The id is its index in the cached possible TeamComposition infos vector,
        as well its index for getting fit percent in the rule's fit table.
        \param[in] failureAsError if true and the composition isn't found in the possible values, logs error.
    *************************************************************************************************/
    TeamCompositionRuleDefinition::TeamCompositionId TeamCompositionRuleDefinition::findTeamCompositionId(const TeamComposition& composition, bool failureAsError /*= true*/) const
    {
        if (mPossibleTeamCompositionVector.empty() || composition.empty())
        {
            if (failureAsError)
            {
                ERR_LOG("[TeamCompositionRuleDefinition].findTeamCompositionId internal error or configuration error, " << ((composition.empty()? "team composition specified" : "team compositions possible values")) << " not yet initialized or empty. Unable to process team compositions.");
            }
            return INVALID_TEAM_COMPOSITION_INDEX;
        }

        for(size_t tcVectorIndex = 0; tcVectorIndex < mPossibleTeamCompositionVector.size(); ++tcVectorIndex)
        {
            const TeamComposition& tc = mPossibleTeamCompositionVector[tcVectorIndex];
            if (areTeamCompositionsEqual(tc, composition))
            {
                return (TeamCompositionId)tcVectorIndex;
            }
        }
        return INVALID_TEAM_COMPOSITION_INDEX;
    }

    /*! ************************************************************************************************/
    /*! \brief return the GameTeamCompositionsId by its input team compositions or INVALID_GAME_TEAM_COMPOSITION_ID if n/a.
    *************************************************************************************************/
    GameTeamCompositionsId TeamCompositionRuleDefinition::findGameTeamCompositionId(
        const TeamCompositionVector& gameTeamCompositionsVectorToFind,
        const GameTeamCompositionsInfoVector& gameTeamCompositionsToFindFrom, bool failureAsError /*= true*/) const
    {
        if (gameTeamCompositionsToFindFrom.empty() || gameTeamCompositionsVectorToFind.empty())
        {
            if (failureAsError)
            {
                ERR_LOG("[TeamCompositionRuleDefinition].findGameTeamCompositionId internal error or configuration error, " << ((gameTeamCompositionsVectorToFind.empty()? "game team composition specified" : "game team compositions possible values")) << " not yet initialized or empty. Unable to process game team compositions.");
            }
            return INVALID_GAME_TEAM_COMPOSITONS_ID;
        }

        for(size_t gtcVectorIndex = 0; gtcVectorIndex < gameTeamCompositionsToFindFrom.size(); ++gtcVectorIndex)
        {
            const GameTeamCompositionsInfo& gtcInfo = gameTeamCompositionsToFindFrom[gtcVectorIndex];
            if (areGameTeamCompositionsEqual(gtcInfo.mTeamCompositionVector, gameTeamCompositionsVectorToFind))
            {
                // extra validation to check the returned id is the index in the possible values vector
                if (mPossibleGameTeamCompositionsVector.size() <= (size_t)gtcInfo.mGameTeamCompositionsId)
                {
                    ERR_LOG("[TeamCompositionRuleDefinition].findGameTeamCompositionId internal error team composition was found in possible values list with a cached GameTeamCompositionsId(" << gtcInfo.mGameTeamCompositionsId << ") which greater than the current max possible values index(" << mPossibleGameTeamCompositionsVector.size() << ").");
                }
                if (mPossibleGameTeamCompositionsVector[gtcInfo.mGameTeamCompositionsId].mGameTeamCompositionsId != gtcInfo.mGameTeamCompositionsId)
                {
                    eastl::string buf;
                    ERR_LOG("[TeamCompositionRuleDefinition].findGameTeamCompositionId internal error team composition was found in possible values list with a cached GameTeamCompositionsId(" << gtcInfo.mGameTeamCompositionsId << ") which is not the at its possible values index(" << gtcInfo.mGameTeamCompositionsId << "). The actual cached game team compositions info at that index was instead: " << mPossibleGameTeamCompositionsVector[gtcInfo.mGameTeamCompositionsId].toFullLogStr() << ". The expected game team compositions there was: " << teamCompositionVectorToLogStr(gameTeamCompositionsVectorToFind, buf) << ".");
                }
                return gtcInfo.mGameTeamCompositionsId;
            }
        }
        return INVALID_GAME_TEAM_COMPOSITONS_ID;
    }

    ////////// Logging Helpers

    /*! ************************************************************************************************/
    /*! \brief logging helper, appends to buffer
    *************************************************************************************************/
    const char8_t* TeamCompositionRuleDefinition::matchInfoCacheByGroupSizeAndMinFitToLogStr(eastl::string& buf) const
    {
        for (size_t i = 1; i < mMatchInfoCacheByGroupSizeAndMinFit.size(); ++i)
        {
            const GroupAndFitThresholdMatchInfoCacheByMinFitMap& byFitMap = mMatchInfoCacheByGroupSizeAndMinFit[i];
            for (GroupAndFitThresholdMatchInfoCacheByMinFitMap::const_iterator itr = byFitMap.begin(); itr != byFitMap.end(); ++itr)
                buf.append_sprintf("\n%s", itr->second.toLogStr());
        }
        return buf.c_str();
    }

    /*! \brief logging helper */
    const char8_t* TeamCompositionRuleDefinition::GroupAndFitThresholdMatchInfoCache::toLogStr() const
    {
        if (mLogStr.empty())
        {
            eastl::string gtcBuf;
            mLogStr.append_sprintf("for myGroupSize=%u,at minFitThresh=%1.2f: (fitPctMin=%1.2f,allExactMatches=%s,acceptableGroupSizes='", mMyGroupSize, mMinFitThreshold, mFitPercentMin, (mAllExactMatches? "true":"false"));
            for (GroupSizeSet::const_iterator itr = mAcceptableOtherSessionGroupSizesSet.begin(); itr != mAcceptableOtherSessionGroupSizesSet.end(); ++itr)
            {
                mLogStr.append_sprintf("%s%u", ((itr != mAcceptableOtherSessionGroupSizesSet.begin())? ",":""),*itr);
            }
            mLogStr.append_sprintf("'), the gameteamcomps='%s'", gameTeamCompositionsInfoVectorToLogStr(mAcceptableGameTeamCompositionsVector, gtcBuf));
        }
        return mLogStr.c_str();
    }

    /*! \brief logging helper, appends to buffer */
    const char8_t* TeamCompositionRuleDefinition::gameTeamCompositionsInfoVectorToLogStr(const GameTeamCompositionsInfoVector& infoVector, eastl::string& buf, bool newlinePerItem /*= false*/)
    {
        buf.append_sprintf("(total=%" PRIsize ") ", infoVector.size());
        for (size_t i = 0; i < infoVector.size(); ++i)
        {
            buf.append_sprintf("%s[%s]%s", (newlinePerItem? "\n\t":""), infoVector[i].toFullLogStr(), ((i+1 < infoVector.size())? ",":""));
        }
        return buf.c_str();
    }

    /*! \brief logging helper, appends to buffer */
    const char8_t* TeamCompositionRuleDefinition::compositionPossibleValueSetToString(
        const CompositionPossibleValueSet& possCompnsSet, eastl::string& buf) const
    {
        buf.append_sprintf("(total=%" PRIsize "):'", possCompnsSet.size());
        for (CompositionPossibleValueSet::const_iterator itr = possCompnsSet.begin(); itr != possCompnsSet.end(); ++itr)
        {
            buf.append_sprintf("%s[%s]", ((itr != possCompnsSet.begin())? ",":""), itr->c_str());
        }
        return buf.append("'").c_str();
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
