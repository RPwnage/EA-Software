/*! ************************************************************************************************/
/*!
    \file   PseudoGameRuleDefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/pseudogameruledefinition.h"
#include "gamemanager/matchmaker/rules/pseudogamerule.h"
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/rete/wmemanager.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(PseudoGameRuleDefinition, "Predefined_PseudoGameRule", RULE_DEFINITION_TYPE_PRIORITY);

    const char8_t* PseudoGameRuleDefinition::PSEUDO_GAME_RETE_KEY = "PseudoGame";

    /*! ************************************************************************************************/
    /*!
        \brief Construct an uninitialized PseudoGameRuleDefinition.  NOTE: do not use until initialized.
    *************************************************************************************************/
    PseudoGameRuleDefinition::PseudoGameRuleDefinition()
    {
    }

    PseudoGameRuleDefinition::~PseudoGameRuleDefinition() 
    {
    }

    bool PseudoGameRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, 0);
        return  true;
    }

    void PseudoGameRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

    void PseudoGameRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // Pseudo Game check:
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            PSEUDO_GAME_RETE_KEY, getWMEBooleanAttributeValue(gameSessionSlave.isPseudoGame()), *this);
    }

    void PseudoGameRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        insertWorkingMemoryElements(wmeManager, gameSessionSlave);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
