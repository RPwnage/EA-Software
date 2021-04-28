/*! ************************************************************************************************/
/*!
\file gameattributeruledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMEATTRIBUTERULEDEFINITION_H
#define BLAZE_GAMEMANAGER_GAMEATTRIBUTERULEDEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rules/rule.h"

#include "gamemanager/matchmaker/voting.h"
#include "gamemanager/matchmaker/fittable.h"
#include "framework/util/random.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class GameAttributeRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(GameAttributeRuleDefinition);
        DEFINE_RULE_DEF_H(GameAttributeRuleDefinition, GameAttributeRule);
    public:
        static const int32_t INVALID_POSSIBLE_VALUE_INDEX;

        static const char8_t* ATTRIBUTE_RULE_DEFINITION_INVALID_LITERAL_VALUE; // internal sentinel to indicate an unmatchable value in RETE


        GameAttributeRuleDefinition();
        ~GameAttributeRuleDefinition() override;

        // Init Logic:
        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;
        virtual void initGenericRuleConfig(GenericRuleConfig &ruleConfig) const;
        bool initAttributeName(const char8_t* attribName);
        bool initDefaultAbstainValue(const Collections::AttributeValue& defaultAbstainValue);
        bool initPossibleValueList(const PossibleValueContainer& possibleValues);
        bool addPossibleValue(const Collections::AttributeValue& possibleValue);
        bool parsePossibleValues(const PossibleValuesList& possibleValuesList, PossibleValueContainer &possibleValues) const;

        static void registerMultiInputValues(const char8_t* name);

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;


        // WME Logic:
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const override;


        bool isReteCapable() const override { return true; }
        
        const char8_t* getAttributeName() const { return mAttributeName.c_str(); }
        AttributeRuleType getAttributeRuleType() const { return mAttributeRuleType; }

        const PossibleValueContainer& getPossibleActualValues() const { return mPossibleActualValues; }
        //MM_AUDIT: This looks unsafe.  Should have a check and ERR if possibleValueIndex > mPossibleValues.size();
        const Collections::AttributeValue& getPossibleValue(size_t possibleValueIndex) const { return mPossibleValues[possibleValueIndex]; }

        // Returns the value of this rule's attribute from the game session.
        // Returns nullptr if the attribute does not exist, or if multiple attributes would be required (player attributes).
        const char8_t* getGameAttributeValue(const GameSession& gameSession) const;
        static const char8_t* getGameAttributeRuleType(const char8_t* ruleName, const GameAttributeRuleServerConfig& genericRule);
     

        /*! ************************************************************************************************/
        /*!
            \brief return the integer index of a possibleValue string (or INVALID_POSSIBLE_VALUE_INDEX (-1) for error).

            \param[in]  value - the possibleValue string to get the index of.  (note: case-insensitive)
            \return The index, or INVALID_POSSIBLE_VALUE_INDEX (-1) if value isn't found in the possibleValue.
        *************************************************************************************************/
        int getPossibleValueIndex(const Collections::AttributeValue& value) const;
        int getPossibleActualValueIndex(const Collections::AttributeValue& value) const;

        size_t getPossibleValueCount() const { return mPossibleValues.size(); }

        bool isValueRandom(const char8_t* attribValue) const { return blaze_stricmp(attribValue, FitTable::RANDOM_LITERAL_CONFIG_VALUE) == 0; }
        bool isValueAbstain(const char8_t* attribValue) const { return blaze_stricmp(attribValue, FitTable::ABSTAIN_LITERAL_CONFIG_VALUE) == 0; }

        // return a random possible value, value will not include "random" or "abstain"
        const Collections::AttributeValue& getRandomPossibleValue() const;
        const Collections::AttributeValue& getDefaultAbstainValue() const;

        float getFitPercent(size_t myValueIndex, size_t otherEntityValueIndex) const;

        RuleVotingMethod getVotingMethod() const { return mVotingSystem.getVotingMethod(); }

        size_t vote(const Voting::VoteTally& voteTally, size_t totalVoteCount, RuleVotingMethod votingMethod) const
        { return mVotingSystem.vote(voteTally, totalVoteCount, votingMethod); }


    protected:
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;


    // Members
    private:
        Collections::AttributeName mAttributeName;
        AttributeRuleType mAttributeRuleType;

        Voting mVotingSystem;
        FitTable mFitTable;

        int32_t mDefaultAbstainValueIndex;
        PossibleValueContainer mPossibleValues;

        // copy of char* from mPossibleValues w/o ABSTAIN/RANDOM.
        // When choosing a 'random' possible value, we just index into this.
        // Note: value string mem still owned by mPossibleValues
        PossibleValueContainer mPossibleActualValues;

    public:
        float getMatchingFitPercent() const { return mMatchingFitPercent; }
        float getMismatchFitPercent() const { return mMismatchFitPercent; }

    // Members
    protected:
        float mMatchingFitPercent;
        float mMismatchFitPercent;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAMEATTRIBUTERULEDEFINITION_H
