/*! ************************************************************************************************/   
/*!
    \file   PseudoGameruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_PSEUDO_GAME_RULE_DEFINITION
#define BLAZE_MATCHMAKING_PSEUDO_GAME_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*!
        \brief The rule definition for finding a game that matches the desired pseudo game setting.
        
    ***************************************************************************************************/
    class PseudoGameRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(PseudoGameRuleDefinition);
        DEFINE_RULE_DEF_H(PseudoGameRuleDefinition, PseudoGameRule);
    public:
        static const char8_t* PSEUDO_GAME_RETE_KEY;

        PseudoGameRuleDefinition();

        //! \brief destructor
        ~PseudoGameRuleDefinition() override;

        bool isDisabled() const override { return false; }

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        //
        // From GameReteRuleDefinition
        //

        bool isReteCapable() const override { return true; }
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        // End from GameReteRuleDefintion

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_PSEUDO_GAME_RULE_DEFINITION
