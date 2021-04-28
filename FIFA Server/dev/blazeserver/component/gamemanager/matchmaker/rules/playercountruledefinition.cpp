/*! ************************************************************************************************/
/*!
    \file   playercountruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/matchmaker/fitformula.h"
#include "gamemanager/matchmaker/rules/playercountruledefinition.h"
#include "gamemanager/matchmaker/rules/playercountrule.h"
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/tdf/matchmaker_server.h" // for FitFormulaNameToString in addGameSizeRuleConfigSettings
#include "gamemanager/rete/wmemanager.h"
#include "gamemanager/matchmaker/rules/rostersizeruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    DEFINE_RULE_DEF_CPP(PlayerCountRuleDefinition, "Predefined_PlayerCountRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* PlayerCountRuleDefinition::PLAYERCOUNTRULE_PARTICIPANTCOUNT_RETE_KEY = "participantCount";

    /*! ************************************************************************************************/
    /*!
        \brief Construct an uninitialized PlayerCountRuleDefinition.  NOTE: do not use until initialized.
    *************************************************************************************************/
    PlayerCountRuleDefinition::PlayerCountRuleDefinition()
        : mFitFormula(nullptr), mGlobalMinPlayerCountInGame(0), mGlobalMaxTotalPlayerSlotsInGame(0)
    {
    }

    PlayerCountRuleDefinition::~PlayerCountRuleDefinition() 
    {
        delete mFitFormula;
    }

    bool PlayerCountRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        eastl::string buf;

        const PlayerCountRuleConfig& playerCountConfig =  matchmakingServerConfig.getRules().getPredefined_PlayerCountRule();

        if(!playerCountConfig.isSet())
        {
            // we allow this to be optional, given another game size rule is used
            TRACE_LOG("[PlayerCountRuleDefinition] " << getConfigKey() << " disabled, not present in configuration.");
         
        }
        else
        {
            // store off a copy of my configuration.
            playerCountConfig.copyInto(mPlayerCountRuleConfigTdf);
        }

        mGlobalMinPlayerCountInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMin();
        mGlobalMaxTotalPlayerSlotsInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMax();

        // append a sentinel values for RosterSizeRule to mPlayerCountRuleConfigTdf
        if (!addRosterSizeRuleConfigSettings())
        {
            return false;//err logged
        }

        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, mPlayerCountRuleConfigTdf.getWeight());

        mFitFormula = FitFormula::createFitFormula(mPlayerCountRuleConfigTdf.getFitFormula().getName());
        if (mFitFormula == nullptr)
        {
            ERR_LOG("[PlayerCountRuleDefinition].create PlayerCountRuleDefinition mFitFormula failed. The fit formula name is " << mPlayerCountRuleConfigTdf.getFitFormula().getName());
            return false;
        }

        if(!mFitFormula->initialize(mPlayerCountRuleConfigTdf.getFitFormula(), &mPlayerCountRuleConfigTdf.getRangeOffsetLists()))
        {
            ERR_LOG("[PlayerCountRuleDefinition].initalize PlayerCountRuleDefinition mFitFormula failed.");
            return false;
        }

        cacheWMEAttributeName(PLAYERCOUNTRULE_PARTICIPANTCOUNT_RETE_KEY);

        return mRangeListContainer.initialize(getName(), mPlayerCountRuleConfigTdf.getRangeOffsetLists());
    }

    /*! ************************************************************************************************/
    /*!
        \brief Roster Size Rule Pref's are implemented via PlayerCountRule internally. Roster Size Rule
             doesn't have range offset lists specifiable so jsut add a sentinel match any one for PlayerCountRule.
    *************************************************************************************************/
    bool PlayerCountRuleDefinition::addRosterSizeRuleConfigSettings()
    {
        // validate no PlayerCountRule config uses the reserved RosterSizeRule range offset list name
        for (RangeOffsetLists::const_iterator itr = mPlayerCountRuleConfigTdf.getRangeOffsetLists().begin(),
            end = mPlayerCountRuleConfigTdf.getRangeOffsetLists().end(); itr != end; ++itr)
        {
            if (blaze_stricmp((*itr)->getName(), RosterSizeRuleDefinition::ROSTERSIZERULE_MATCHANY_RANGEOFFSETLIST_NAME) == 0)
            {
                ERR_LOG("[PlayerCountRuleDefinition].addRosterSizeRuleConfigSettings PlayerCountRule or GameSizeRule configuration uses reserved range list name " << RosterSizeRuleDefinition::ROSTERSIZERULE_MATCHANY_RANGEOFFSETLIST_NAME << ".");
                return false;
            }
        }

        // add a match any range offset list to use
        RangeOffsetList* newList = mPlayerCountRuleConfigTdf.getRangeOffsetLists().pull_back();
        newList->setName(RosterSizeRuleDefinition::ROSTERSIZERULE_MATCHANY_RANGEOFFSETLIST_NAME);
        RangeOffset* offset = newList->getRangeOffsets().pull_back();
        offset->setT(0);
        offset->getOffset().push_back(RangeListContainer::INFINITY_RANGE_TOKEN);
        return true;
    }


    void PlayerCountRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        char8_t buf[512];
        mPlayerCountRuleConfigTdf.print(buf, sizeof(buf));
        dest.append_sprintf("%s %s", prefix, buf);
    }

    void PlayerCountRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const GameManager::PlayerRoster& gameRoster = *(gameSessionSlave.getPlayerRoster());
        uint64_t totalPlayers = (uint64_t)gameRoster.getParticipantCount();

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), 
            PLAYERCOUNTRULE_PARTICIPANTCOUNT_RETE_KEY, totalPlayers, *this);
    }

    void PlayerCountRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {

        const GameManager::PlayerRoster& gameRoster = *(gameSessionSlave.getPlayerRoster());

        uint64_t totalParticipants = (uint64_t)gameRoster.getParticipantCount();

        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), 
            PLAYERCOUNTRULE_PARTICIPANTCOUNT_RETE_KEY, totalParticipants, *this);
    }


    /*! ************************************************************************************************/
    /*!
        \brief given two player counts, determine the difference, and plug that value into the fit formula function,
            returning a fitPercent between 0 and 1.

        \param[in] playerCountA one of the two playerCounts to plug into the equation
        \param[in] playerCountB one of the two playerCounts to plug into the equation
        \return the fitPercent between A & B (note: A,B is the same as B,A due to symmetry)
    ***************************************************************************************************/
    float PlayerCountRuleDefinition::getFitPercent(uint16_t playerCountA, uint16_t playerCountB) const
    {
        if (mFitFormula != nullptr)
            return mFitFormula->getFitPercent(playerCountA, playerCountB);
        else
        {
            EA_ASSERT(mFitFormula != nullptr);
            return 0;
        }
    }


    const RangeList* PlayerCountRuleDefinition::getRangeOffsetList(const char8_t *listName) const
    {
        return mRangeListContainer.getRangeList(listName);
    }
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
