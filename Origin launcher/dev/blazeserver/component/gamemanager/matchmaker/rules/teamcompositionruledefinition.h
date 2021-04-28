/*! ************************************************************************************************/
/*!
    \file   teamcompositionruledefinition.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_COMPOSITION_RULE_DEFINITION
#define BLAZE_MATCHMAKING_TEAM_COMPOSITION_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/teamcompositionscommoninfo.h"
#include "gamemanager/matchmaker/fittable.h" // for TeamCompositionRuleDefinition::mFitTable

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class TeamCompositionRule;

    /*! ************************************************************************************************/
    /*! \brief TeamCompositionRule definition
    ***************************************************************************************************/
    class TeamCompositionRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(TeamCompositionRuleDefinition);
        DEFINE_RULE_DEF_H(TeamCompositionRuleDefinition, TeamCompositionRule);
    public:

        static const char8_t* TEAMCOMPOSITIONRULE_TEAM_ATTRIBUTE_SPACE;
        static const char8_t* TEAMCOMPOSITIONRULE_TEAM_ATTRIBUTE_GROUP_SIZE;

        typedef eastl::hash_set<uint16_t> GroupSizeSet;

        /*! ************************************************************************************************/
        /*! \brief to avoid overhead in determining matchable game team compositions, calculating fit percents
            and match times etc during matchmaking eval, finalization and FG rete, we pre-resolve and cache them
            off here for random access, by session group sizes and min fit thresholds for this rule.
        ***************************************************************************************************/
        struct GroupAndFitThresholdMatchInfoCache
        {
            GroupAndFitThresholdMatchInfoCache() : mMinFitThreshold(FIT_PERCENT_NO_MATCH), mMyGroupSize(0), mFitPercentMin(FIT_PERCENT_NO_MATCH), mAllExactMatches(false) {}

            GameTeamCompositionsInfoVector mAcceptableGameTeamCompositionsVector; //the matched game team compositions
            GroupSizeSet mAcceptableOtherSessionGroupSizesSet; //all sizes from the actual vector
            TeamCompositionRuleStatus mTeamCompositionRuleStatus; //cached async status
            float mMinFitThreshold; //min fit threshold this data pertains to
            uint16_t mMyGroupSize;  //MM session group size this data pertains to
            float mFitPercentMin;   //min of actual vector's fit percents
            bool mAllExactMatches;  //whether all the actual vector's items are all 'exact matches' (all teams identical compositions)

            bool isOtherGroupSizeAcceptable(uint16_t otherSessionGroupSize) const { return mAcceptableOtherSessionGroupSizesSet.find(otherSessionGroupSize) != mAcceptableOtherSessionGroupSizesSet.end(); }
            const char8_t* toLogStr() const;
        private:
            mutable eastl::string mLogStr;
        };
        /*! ************************************************************************************************/
        /*! \brief sorted map of fit threshold values, to GroupAndFitThresholdMatchInfoCache's.
        ***************************************************************************************************/
        typedef eastl::map<float, GroupAndFitThresholdMatchInfoCache, eastl::greater<float> > GroupAndFitThresholdMatchInfoCacheByMinFitMap;


    public:
        TeamCompositionRuleDefinition();
        ~TeamCompositionRuleDefinition() override;

        bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) override;

        bool isReteCapable() const override { return true; }

        /*! \brief Returns true to ensure post-RETE FindGame evaluation is done to precisely filter joinable games
            (RETE serves as a broad filter, but does not accurately account for the team you'll join nor cases of non full games) */
        bool callEvaluateForMMFindGameAfterRete() const override { return true; }

        // RETE impl
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        // end RETE

        float getFitPercent(const TeamCompositionVector& teamCompositionVectorForGame) const;
        const GroupAndFitThresholdMatchInfoCache* getMatchInfoCacheByGroupSizeAndMinFit(uint16_t groupSize, float fitThreshold) const;
        const GroupAndFitThresholdMatchInfoCacheByMinFitMap* getMatchInfoCacheByMinFitMap(uint16_t groupSize) const;
        
        typedef eastl::set<eastl::string> CompositionPossibleValueSet;
        const CompositionPossibleValueSet& getMathematicallyPossibleCompositionsForSlotcount(uint16_t slotCount) const;

        /*! \brief Returns the team capacity for the team compositions for this rule, based off this rule's configured possible values. */
        uint16_t getTeamCapacityForRule() const { return mTeamCapacityForRule; }
        uint16_t getGlobalMinPlayerCountInGame() const { return mGlobalMinPlayerCountInGame; }
        uint16_t getGlobalMaxTotalPlayerSlotsInGame() const { return mGlobalMaxTotalPlayerSlotsInGame; }
        uint16_t getMaxFinalizationGameTeamCompositionsAttempted() const { return ((mRuleConfigTdf.getMaxFinalizationGameTeamCompositionsAttempted() == 0)? UINT16_MAX : mRuleConfigTdf.getMaxFinalizationGameTeamCompositionsAttempted()); }
        uint16_t getTeamCount() const { return mRuleConfigTdf.getTeamCount(); }
        
    private:

        /*! ************************************************************************************************/
        /*! \brief sort by fit percent, team count, larger group counts, non-ascending
        ***************************************************************************************************/
        struct GameTeamCompositionsInfoComparitor
        {
            bool operator()(const GameTeamCompositionsInfo &a, const GameTeamCompositionsInfo &b) const 
            {
                if (a.mCachedFitPercent != b.mCachedFitPercent)
                    return (a.mCachedFitPercent > b.mCachedFitPercent);

                // tie break by preferring games with the larger number of teams (if title allows varied number)
                if (a.mTeamCompositionVector.size() != b.mTeamCompositionVector.size())
                    return (a.mTeamCompositionVector.size() > b.mTeamCompositionVector.size());

                // tie break by lower group count (fewer means generally larger groups, and larger groups harder to match so do first)
                if (a.getNumOfGroups() != b.getNumOfGroups())
                    return (a.getNumOfGroups() < b.getNumOfGroups());

                // tie break by higher top group size (again larger groups are harder to match)
                size_t aFirstGroupSizeSum = 0;
                for (size_t i = 0; i < a.mTeamCompositionVector.size(); ++i)
                {
                    if (!a.mTeamCompositionVector[i].empty())
                        aFirstGroupSizeSum += a.mTeamCompositionVector[i][0];
                }
                size_t bFirstGroupSizeSum = 0;
                for (size_t i = 0; i < b.mTeamCompositionVector.size(); ++i)
                {
                    if (!b.mTeamCompositionVector[i].empty())
                        bFirstGroupSizeSum += b.mTeamCompositionVector[i][0];
                }
                if (aFirstGroupSizeSum != bFirstGroupSizeSum)
                    return (aFirstGroupSizeSum > bFirstGroupSizeSum);

                // for debug-ability/logging, put team composition group counts together
                return (a.mTeamCompositionVector[0].size() < b.mTeamCompositionVector[0].size());
            }
        };

    private:
        /*! ************************************************************************************************/
        /*! \brief sort by group sizes non-ascending
        ***************************************************************************************************/
        void sortTeamComposition(TeamComposition& composition) const { eastl::sort(composition.begin(), composition.end(), eastl::greater<uint16_t>()); }

        //// initialization helpers
        bool validateFitTable(const TeamCompositionRuleConfig& ruleConfig) const;
        bool parsePossibleValuesAndInitAllTeamCompositions(const PossibleValuesList& possibleValuesStrings);
        bool possibleValueStringToTeamComposition(const ParamValue& possVal, TeamComposition& composition) const;
        const char8_t* teamCompositionToPossibleValueString(const TeamComposition& composition, eastl::string& possVal) const;

        void addPossibleTeamComposition(const TeamComposition& compositionValues);
        bool addPossibleGameTeamCompositionsInfos();
        bool addNewUnidentifiedGameTeamCompositionsInfosForPrefix(const TeamCompositionVector& tcVectorPrefix);
        bool addNewUnidentifiedGameTeamCompositionsInfo(const TeamCompositionVector& tcVector);
        const GroupSizeCountMapByTeamList* addNewGroupSizeCountMapByTeamList(const TeamCompositionVector& teamCompositionVector);

        bool cacheGroupAndFitThresholdMatchInfos();
        bool cacheGroupAndFitThresholdMatchInfos(uint16_t groupSize, float fitThreshold);
        void populateGroupAndFitThresholdMatchInfoCache(GroupAndFitThresholdMatchInfoCache& cache, const GameTeamCompositionsInfo& gtcInfo, uint16_t groupSize, float fitThreshold) const;
        void populateGroupSizeSetCache(GroupSizeSet& groupSizeSet, const GameTeamCompositionsInfo& gtcInfo, uint16_t ignoreFirstInstanceOfSize) const;
        void populateAsyncStatusCache(TeamCompositionRuleStatus& asyncStatus, const GameTeamCompositionsInfo& gtcInfo, uint16_t myGroupSize, CompositionPossibleValueSet& myTeamCompositionsAsyncStatusStrSet, CompositionPossibleValueSet& otherTeamCompositionsAsyncStatusStrSet) const;

        uint16_t calcTeamCapacityForRule(const TeamCompositionVector &possibleValues) const;
        uint16_t calcTeamCompositionSizesSum(const TeamComposition& composition) const;

        typedef eastl::hash_map<uint16_t, CompositionPossibleValueSet> CapacityToCompositionPossibleValueSetMap;
        bool cacheMathematicallyPossibleCompositionsForAllCapacitiesUpToMax(uint16_t maxTeamCapacity);
        void cacheMathematicallyPossibleCompositionsForCapacity(uint16_t slotCount, CompositionPossibleValueSet& compositionPossibleValueSet) const;
        bool checkMathematicallyPossibleCompositionsArePossibleValues() const;
        void populateMathematicallyPossibleCompositions(const TeamComposition& prefix, const TeamComposition& part1, const TeamComposition& part2, const TeamComposition& tail, CompositionPossibleValueSet& compositionPossibleValueSet) const;
        const char8_t* compositionPossibleValueSetToString(const CompositionPossibleValueSet& possCompnsSet, eastl::string& buf) const;
        //// End initialization helpers

        /*! \brief TeamComposition's indices to TeamCompositionRule fit tables for fit percent calc between 2 teams */
        typedef int TeamCompositionId;
        static const TeamCompositionId INVALID_TEAM_COMPOSITION_INDEX = -1;
        float getFitPercent(TeamCompositionId teamComposition1PossValueIndex, TeamCompositionId teamComposition2PossValueIndex) const;

        /*! ************************************************************************************************/
        /*! \brief return whether or not the specified sequence of group sizes represents a TeamComposition that's been initialized as a possible value for the rule.
        *************************************************************************************************/
        bool isPossibleCompositionForRule(const TeamComposition& composition) const { return (INVALID_TEAM_COMPOSITION_INDEX != findTeamCompositionId(composition, false)); }
        bool isPossibleGameTeamCompositionsForRule(const TeamCompositionVector& teamCompositionVectorForGame) const { return (INVALID_GAME_TEAM_COMPOSITONS_ID != findGameTeamCompositionId(teamCompositionVectorForGame, mPossibleGameTeamCompositionsVector, false)); }
        TeamCompositionId findTeamCompositionId(const TeamComposition& composition, bool failureAsError = true) const;
        GameTeamCompositionsId findGameTeamCompositionId(const TeamCompositionVector& gameTeamCompositionsVectorToFind, const GameTeamCompositionsInfoVector& gameTeamCompositionsToFindFrom, bool failureAsError /*= true*/) const;

        void insertOrUpsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave, bool doUpsert) const;
        void cleanupRemovedWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;
        const char8_t* matchInfoCacheByGroupSizeAndMinFitToLogStr(eastl::string& buf) const;
        static const char8_t* gameTeamCompositionsInfoVectorToLogStr(const GameTeamCompositionsInfoVector& infoVector, eastl::string& buf, bool newlinePerItem = false);

    private:
        typedef eastl::vector<GroupAndFitThresholdMatchInfoCacheByMinFitMap> GroupAndFitThresholdMatchInfoCacheByGroupSizeAndMinFitVector;

        TeamCompositionRuleConfig mRuleConfigTdf;
        FitTable mFitTable;

        uint16_t mTeamCapacityForRule;
        uint16_t mGlobalMinPlayerCountInGame;
        uint16_t mGlobalMaxTotalPlayerSlotsInGame;

        // the possible team compositions owned by this rule
        TeamCompositionVector mPossibleTeamCompositionVector;

        // the possible game team compositions owned by this rule
        GameTeamCompositionsInfoVector mPossibleGameTeamCompositionsVector;

        // cached info about the possible game team compositions (tdf's). Owned by this rule definition to ensure valid (see GameTeamCompositionsInfo).
        GroupSizeCountMapByTeamListList mPossibleGtcGroupSizeCountInfosVector;

        // cached info about MM sessions' acceptable game team compositions lists
        GroupAndFitThresholdMatchInfoCacheByGroupSizeAndMinFitVector mMatchInfoCacheByGroupSizeAndMinFit;

        mutable CapacityToCompositionPossibleValueSetMap mCapacityToMathematicallyPossCompositionsMap;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_COMPOSITION_RULE_DEFINITION
