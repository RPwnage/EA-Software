/*! ************************************************************************************************/
/*!
    \file   virtualgameruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_VIRTUAL_GAME_RULE_DEFINITION
#define BLAZE_MATCHMAKING_VIRTUAL_GAME_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"


namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class VirtualGameRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(VirtualGameRuleDefinition);
        DEFINE_RULE_DEF_H(VirtualGameRuleDefinition, VirtualGameRule);
    public:

        static const char8_t* VIRTUALRULE_RETE_KEY;
        VirtualGameRuleDefinition();
        ~VirtualGameRuleDefinition() override;

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

        float getMatchingFitPercent() const { return mMatchingFitPercent; }
        float getMismatchFitPercent() const { return mMismatchFitPercent; }

    protected:

        //////////////////////////////////////////////////////////////////////////
        // from GameReteRuleDefinition grandparent
        //////////////////////////////////////////////////////////////////////////
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

    // Members
    private:
        float mMatchingFitPercent;
        float mMismatchFitPercent;
    };
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_VIRTUAL_GAME_RULE_DEFINITION
