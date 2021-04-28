/*! ************************************************************************************************/
/*!
    \file   reputationruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_REPUTATION_RULE_DEFINITION
#define BLAZE_MATCHMAKING_REPUTATION_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"


namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class ReputationRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(ReputationRuleDefinition);
        DEFINE_RULE_DEF_H(ReputationRuleDefinition, ReputationRule);
    public:

        static const char8_t* REPUTATIONRULE_ALLOW_ANY_REPUTATION_RETE_KEY;
        static const char8_t* REPUTATIONRULE_REQUIRE_GOOD_REPUTATION_RETE_KEY;
        static const char8_t* REPUTATIONRULE_DEFINITION_ATTRIBUTE_KEY;
        ReputationRuleDefinition();
        ~ReputationRuleDefinition() override;

        //////////////////////////////////////////////////////////////////////////
        // from GameReteRuleDefinition grandparent
        //////////////////////////////////////////////////////////////////////////
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        //////////////////////////////////////////////////////////////////////////
        // from RuleDefinition
        //////////////////////////////////////////////////////////////////////////
        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;
        bool isReteCapable() const override { return true; }
        bool isDisabled() const override { return mDisabled; }

    protected:

        //////////////////////////////////////////////////////////////////////////
        // from GameReteRuleDefinition grandparent
        //////////////////////////////////////////////////////////////////////////
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

    // Members
    private:
        bool mDisabled;
    };
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_REPUTATION_RULE_DEFINITION
