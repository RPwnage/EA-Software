/*! ************************************************************************************************/
/*!
\file singlegroupmatchruledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_SINGLEGROUPMATCHRULEDEFINITION_H
#define BLAZE_GAMEMANAGER_SINGLEGROUPMATCHRULEDEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/rete/wmemanager.h"

namespace Blaze
{

namespace Search
{
    class GameSessionSearchSlave;
}

namespace GameManager
{
namespace Matchmaker
{
    class SingleGroupMatchRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(SingleGroupMatchRuleDefinition);
        DEFINE_RULE_DEF_H(SingleGroupMatchRuleDefinition, SingleGroupMatchRule);
    public:

        static const char8_t* SGMRULE_SETTING_KEY;
        static const char8_t* SGMRULE_GROUP_ID_KEY;
        static const char8_t* SGMRULE_NUM_GROUPS_KEY;

        SingleGroupMatchRuleDefinition();
        ~SingleGroupMatchRuleDefinition() override;

        // RuleDefinition impls
        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        bool isReteCapable() const override { return true; }
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override {};
        //can't turn this one off
        bool isDisabled() const override { return false; }

        // GameReteRuleDefinition impls
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        Rete::WMEAttribute getWMEAttributeName(const char8_t* name, int64_t value) const;
        // needed because we're overloading a virtual from our parent above.
        Rete::WMEAttribute getWMEAttributeName(const char8_t* name) const override { return RuleDefinition::getWMEAttributeName(name); }


    private:
       
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

#endif
