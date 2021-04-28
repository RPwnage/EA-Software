/*! ************************************************************************************************/
/*!
    \file   gamenameruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/matchmaker/rules/gamenameruledefinition.h"
#include "gamemanager/matchmaker/rules/gamenamerule.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSearchSlave in insertWorkingMemoryElements

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    DEFINE_RULE_DEF_CPP(GameNameRuleDefinition, "Predefined_GameNameRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* GameNameRuleDefinition::GAMENAMERULE_RETE_KEY = "gameName";
    const char8_t* GameNameRuleDefinition::GAMENAME_SEARCHSTRING_MINLENGTH_KEY = "minLength";

    /*! ************************************************************************************************/
    /*! \brief error if try to configure rule min length to less than this value
    *************************************************************************************************/
    const uint16_t GameNameRuleDefinition::GAMENAME_SEARCHSTRING_MINIMUM_MINLENGTH = 3;
    
    GameNameRuleDefinition::GameNameRuleDefinition() :
        mSearchStringMinLength(0),
        mStringNormalizationTable(nullptr),
        mSearchStringTable(nullptr)
    {
    }
    GameNameRuleDefinition::~GameNameRuleDefinition()
    {
        if (mStringNormalizationTable != nullptr)
        {
            delete mStringNormalizationTable;
        }
    }    

    /*! ************************************************************************************************/
    /*! \brief initialize rete rule
    ***************************************************************************************************/
    bool GameNameRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        const GameNameRuleConfig& gameNameRuleConfig = matchmakingServerConfig.getRules().getPredefined_GameNameRule();

        if(!gameNameRuleConfig.isSet())
        {
            WARN_LOG("[GameNameRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, gameNameRuleConfig.getWeight());
        
        // init search string min length
        mSearchStringMinLength = (uint16_t)gameNameRuleConfig.getMinLength();
        if (mSearchStringMinLength == 0)
        {
            WARN_LOG("[GameNameRuleDefinition] Rule configured but disabled. matchmaker config search string min length value of 0 specified.");
            return false;
        }
        else if ((mSearchStringMinLength < GAMENAME_SEARCHSTRING_MINIMUM_MINLENGTH) && !matchmakingServerConfig.getAllowGameNameRuleMinLengthBelow3())
        {
            ERR_LOG("[GameNameRuleDefinition] matchmaker configured search string min length " << mSearchStringMinLength << " cannot be lower than the internal hard limit of " << GAMENAME_SEARCHSTRING_MINIMUM_MINLENGTH);
            return false;
        }
        else if (mSearchStringMinLength > Blaze::GameManager::MAX_GAMENAME_LEN)
        {
            ERR_LOG("[GameNameRuleDefinition] matchmaker configured search string min length " << mSearchStringMinLength << " > game name max length " << Blaze::GameManager::MAX_GAMENAME_LEN);
            return false;
        }

        // setup the normalization table used to init rete substring trie.
        mStringNormalizationTable = BLAZE_NEW Blaze::GameManager::Rete::ReteSubstringTrie::NormalizationTable(
            gameNameRuleConfig.getIgnoreCase(), gameNameRuleConfig.getRelevantChars());
        return true;
    }

    // MM_TODO: When the work to use the TDF parser for the MM Config is complete
    // use the resulting TDF instead of each rule doing their own parsing.
    /*! ************************************************************************************************/
    /*! \brief write the configuration information for this rule into a MatchmakingRuleInfo TDF object
        \param[in\out] ruleConfig - rule configuration object to pack with data for this rule
    ***************************************************************************************************/
    void GameNameRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();
        // override to only add these (rule does not have any min fit threshold list)
        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);
    }

    /*! ************************************************************************************************/
    /*! \brief rete insert the search-able substring facts for the new game
    ***************************************************************************************************/
    void GameNameRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager,
        const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        char8_t normalizedName[Blaze::GameManager::MAX_GAMENAME_LEN];
        if ((gameSessionSlave.getGameName()[0] == '\0') ||
            !normalizeStringAndValidateLength(gameSessionSlave.getGameName(), normalizedName))
        {
            TRACE_LOG("[GameNameRuleDefinition].insertWorkingMemoryElements: Not adding WME for game(" << gameSessionSlave.getGameId() << "), its name('" << gameSessionSlave.getGameName() << "',normalized:'" << ((gameSessionSlave.getGameName()[0] == '\0')? "": normalizedName) << "') did not pass rule length requirement checks. Required min length:" << getSearchStringMinLength());
            return;
        }

        TRACE_LOG("[GameNameRuleDefinition].insertWorkingMemoryElements: inserting wme for game(id=" << gameSessionSlave.getGameId() << ",name='" << gameSessionSlave.getGameName() << "',normalized:'" << normalizedName << "')");
        wmeManager.insertWorkingMemoryElementToSubstringTrie(gameSessionSlave.getGameId(), getWMEAttributeName(GAMENAMERULE_RETE_KEY),
            normalizedName);
    }

    /*! ************************************************************************************************/
    /*! \brief rete upsert game name facts, removing old name's facts as needed
        Pre: no need to remove wmes if reseting ds, as all old wmes already cleared by GM on-update handler
    ***************************************************************************************************/
    void GameNameRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager,
        const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        char8_t normalizedName[Blaze::GameManager::MAX_GAMENAME_LEN];

        normalizedName[0] = '\0';

        // first remove any substring facts for the old game name (if old normalized length met requirements)
        const GameManager::ReplicatedGameDataServer* prevGameSnapshot = gameSessionSlave.getPrevSnapshot();
        if (prevGameSnapshot != nullptr)
        {
            if (blaze_stricmp(prevGameSnapshot->getReplicatedGameData().getGameName(), gameSessionSlave.getGameName()) == 0)
                return;

            if ((prevGameSnapshot->getReplicatedGameData().getGameName()[0] != '\0') &&
                normalizeStringAndValidateLength(prevGameSnapshot->getReplicatedGameData().getGameName(), normalizedName))
            {
                TRACE_LOG("[GameNameRuleDefinition].upsertWorkingMemoryElements: removing old wme for game(id=" << gameSessionSlave.getGameId() << ",old name='" << prevGameSnapshot->getReplicatedGameData().getGameName() << "',normalized:'" << normalizedName << "') (after possible name change)");
                wmeManager.removeWorkingMemoryElementToSubstringTrie(gameSessionSlave.getGameId(), getWMEAttributeName(GAMENAMERULE_RETE_KEY), 
                    normalizedName);
            }
        }

        normalizedName[0] = '\0';

        if ((gameSessionSlave.getGameName()[0] == '\0') ||
            !normalizeStringAndValidateLength(gameSessionSlave.getGameName(), normalizedName))
        {
            TRACE_LOG("[GameNameRuleDefinition].upsertWorkingMemoryElements: Not adding WME for game(" << gameSessionSlave.getGameId() << "), its name('" << gameSessionSlave.getGameName() << "',normalized:'" << ((gameSessionSlave.getGameName()[0] == '\0')? "": normalizedName) << "') did not pass rule length requirement checks. Required min length:" << getSearchStringMinLength());
            return;
        }
        
        // add or update the new game name's substring facts, as needed
        TRACE_LOG("[GameNameRuleDefinition].upsertWorkingMemoryElements: updating wme for game(id=" << gameSessionSlave.getGameId() << ",name='" << gameSessionSlave.getGameName() << "',normalized:'" << normalizedName << "') (after possible name change)");
        wmeManager.insertWorkingMemoryElementToSubstringTrie(gameSessionSlave.getGameId(), getWMEAttributeName(GAMENAMERULE_RETE_KEY),
            normalizedName);
    }

    /*! ************************************************************************************************/
    /*! \brief normalizes string via the internal translation table and return whether meets length requirements.
        \param[out] normalized - buffer to store result (appends null terminator). Pre: sufficient size allocated.
    ***************************************************************************************************/
    bool GameNameRuleDefinition::normalizeStringAndValidateLength(const char8_t* string, char8_t* normalized) const
    {
        if ((string == nullptr) || (normalized == nullptr))
        {
            ERR_LOG("[GameNameRuleDefinition].normalizeStringAndValidateLength: internal error: invalid null input string argument");
            return false;
        }
        EA_ASSERT(mStringNormalizationTable != nullptr);

        const uint32_t normalizedLength = getNormalizationTable()->normalizeWord(string, normalized);
        normalized[normalizedLength] = '\0';

        if (normalizedLength < getSearchStringMinLength())
        {
            TRACE_LOG("[GameNameRuleDefinition].normalizeStringAndValidateLength(): string('" << string <<
                "',normalized:'" << normalized << "') has normalized search length '" << normalizedLength <<
                "' which is less than minimum configured allowed length of " << getSearchStringMinLength());
            return false;
        }
        return true;
    }

    void GameNameRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        dest.append_sprintf("%s  %s = %1.2f\n", prefix, GameNameRuleDefinition::GAMENAME_SEARCHSTRING_MINLENGTH_KEY, getSearchStringMinLength());
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
