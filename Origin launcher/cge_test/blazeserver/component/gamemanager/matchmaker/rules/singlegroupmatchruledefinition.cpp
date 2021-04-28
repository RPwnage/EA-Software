/*! ************************************************************************************************/
/*!
\file singlegroupmatchruledefinition.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/matchmaker/rules/singlegroupmatchruledefinition.h"
#include "gamemanager/matchmaker/rules/singlegroupmatchrule.h"
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(SingleGroupMatchRuleDefinition, "SingleGroupMatchRule", RULE_DEFINITION_TYPE_PRIORITY);
    const char8_t* SingleGroupMatchRuleDefinition::SGMRULE_GROUP_ID_KEY = "GroupId";
    const char8_t* SingleGroupMatchRuleDefinition::SGMRULE_NUM_GROUPS_KEY = "NumGroups";
    const char8_t* SingleGroupMatchRuleDefinition::SGMRULE_SETTING_KEY = "Setting";


    SingleGroupMatchRuleDefinition::SingleGroupMatchRuleDefinition()
    {
    }

    SingleGroupMatchRuleDefinition::~SingleGroupMatchRuleDefinition() {}

    // GameReteRuleDefinition impls
    void SingleGroupMatchRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        if (gameSessionSlave.isResetable())
        {
            return;
        }

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
            SGMRULE_SETTING_KEY, getWMEBooleanAttributeValue(gameSessionSlave.getGameSettings().getEnforceSingleGroupJoin()), *this);

        GameManager::SingleGroupMatchIdList::const_iterator iter = gameSessionSlave.getSingleGroupMatchIds().begin();
        GameManager::SingleGroupMatchIdList::const_iterator end = gameSessionSlave.getSingleGroupMatchIds().end();

        int64_t numGroups = 0;
        // add in all group ids in the single group match set on the game
        for (; iter !=end; ++iter)
        {
            UserGroupId groupId = *iter;
            eastl::string buf;
            buf.append_sprintf("%s_%" PRId64, SingleGroupMatchRuleDefinition::SGMRULE_GROUP_ID_KEY, groupId.id);
            wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), buf.c_str(), getWMEBooleanAttributeValue(true), *this, true);

            numGroups++;
        }

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
            SGMRULE_NUM_GROUPS_KEY, getWMEInt64AttributeValue(numGroups), *this);
    }

    void SingleGroupMatchRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        if (gameSessionSlave.isResetable())
        {
            return;
        }

        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            SGMRULE_SETTING_KEY, getWMEBooleanAttributeValue(gameSessionSlave.getGameSettings().getEnforceSingleGroupJoin()), *this);

        GameManager::SingleGroupMatchIdList::const_iterator iter = gameSessionSlave.getSingleGroupMatchIds().begin();
        GameManager::SingleGroupMatchIdList::const_iterator end = gameSessionSlave.getSingleGroupMatchIds().end();

        int64_t numGroups = 0;
        // add in all group ids in the single group match set on the game
        for (; iter !=end; ++iter)
        {
            UserGroupId groupId = *iter;
            eastl::string buf;
            buf.append_sprintf("%s_%" PRId64, SingleGroupMatchRuleDefinition::SGMRULE_GROUP_ID_KEY, groupId.id);
            wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), buf.c_str(), getWMEBooleanAttributeValue(true), *this, true);

            numGroups++;
        }

        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            SGMRULE_NUM_GROUPS_KEY, getWMEInt64AttributeValue(numGroups), *this);
    }


    Rete::WMEAttribute SingleGroupMatchRuleDefinition::getWMEAttributeName(const char8_t* attributeName, int64_t value) const
    {
        char8_t buf[512];
        blaze_snzprintf(buf, sizeof(buf), "%s_%s_%" PRId64, getName(), attributeName, value);

        return GameReteRuleDefinition::getStringTable().reteHash(buf);
    }

    bool SingleGroupMatchRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // Rule has no server configuration, so weight is 0
        uint32_t weight = 0;
        RuleDefinition::initialize(name, salience, weight);
        return true;
    }
}
}
}
