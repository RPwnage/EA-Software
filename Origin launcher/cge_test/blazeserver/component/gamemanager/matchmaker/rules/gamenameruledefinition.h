/*! ************************************************************************************************/
/*!
    \file   gamenameruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_GAME_NAME_RULE_DEFINITION
#define BLAZE_MATCHMAKING_GAME_NAME_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/rete/retesubstringtrie.h" // for mStringNormalizationTable

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief The GameNameRule definition
    ***************************************************************************************************/
    class GameNameRuleDefinition : public GameManager::Matchmaker::RuleDefinition
    {
        NON_COPYABLE(GameNameRuleDefinition);
        DEFINE_RULE_DEF_H(GameNameRuleDefinition, GameNameRule);
    public:

        static const char8_t* GAMENAMERULE_RETE_KEY;
        static const char8_t* GAMENAME_SEARCHSTRING_MINLENGTH_KEY;

        static const uint16_t GAMENAME_SEARCHSTRING_MINIMUM_MINLENGTH;
                
        GameNameRuleDefinition();
        ~GameNameRuleDefinition() override;

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;

        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        bool isReteCapable() const override { return true; }
        // Rule disabled globally if configured server wide min length is 0.
        bool isDisabled() const override { return mSearchStringMinLength == 0; }

        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        bool normalizeStringAndValidateLength(const char8_t* string, char8_t* normalized) const;
        uint16_t getSearchStringMaxLength() const { return Blaze::GameManager::MAX_GAMENAME_LEN; }
        uint16_t getSearchStringMinLength() const { return mSearchStringMinLength; }
        const Blaze::GameManager::Rete::ReteSubstringTrie::NormalizationTable* getNormalizationTable() const { return mStringNormalizationTable; }


        /*! ************************************************************************************************/
        /*! \brief returns numeric hash value for a search string.
            Pre: internal string table here is the same for retrieving the string from hash, in rete network.
            Intentionally not passing through the garbageCollectable flag- streantable has it's own cleanup code 
            for the string table entries, and has its own string table. This is intended to avoid knock-ons in GameNameRule

        *************************************************************************************************/
        Blaze::GameManager::Rete::WMEAttribute reteHash(const char8_t* unhashedString, bool garbageCollectable = false) const override { EA_ASSERT(mSearchStringTable); return mSearchStringTable->reteHash(unhashedString); }

        /*! ************************************************************************************************/
        /*! \brief sets the internal searches string table
        *************************************************************************************************/
        void setSearchStringTable(Blaze::GameManager::Rete::StringTable& stringTable) { mSearchStringTable = &stringTable; }

    private:

        uint16_t mSearchStringMinLength;
        const Blaze::GameManager::Rete::ReteSubstringTrie::NormalizationTable* mStringNormalizationTable;
        Blaze::GameManager::Rete::StringTable* mSearchStringTable;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_GAME_NAME_RULE_DEFINITION
#endif
