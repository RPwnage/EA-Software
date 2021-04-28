/*! ************************************************************************************************/
/*!
    \file   playerslotutilizationruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/random.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/matchmaker/fitformula.h"
#include "gamemanager/matchmaker/rules/playerslotutilizationruledefinition.h"
#include "gamemanager/matchmaker/rules/playerslotutilizationrule.h"
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)

#include "gamemanager/rete/wmemanager.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(PlayerSlotUtilizationRuleDefinition, "Predefined_PlayerSlotUtilizationRule", RULE_DEFINITION_TYPE_SINGLE);

    /*! ************************************************************************************************/
    /*!
        \brief Construct an uninitialized PlayerSlotUtilizationRuleDefinition.  NOTE: do not use until initialized.
    *************************************************************************************************/
    PlayerSlotUtilizationRuleDefinition::PlayerSlotUtilizationRuleDefinition()
        : mFitFormula(nullptr), mGlobalMaxTotalPlayerSlotsInGame(0)
    {
    }

    PlayerSlotUtilizationRuleDefinition::~PlayerSlotUtilizationRuleDefinition() 
    {
        delete mFitFormula;
    }

    bool PlayerSlotUtilizationRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        eastl::string err;

        const PlayerSlotUtilizationRuleConfig& ruleConfig =  matchmakingServerConfig.getRules().getPredefined_PlayerSlotUtilizationRule();
        if (!ruleConfig.isSet())
        {
            TRACE_LOG("[PlayerSlotUtilizationRuleDefinition] " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        // store off a copy of my configuration.
        ruleConfig.copyInto(mPlayerSlotUtilizationRuleConfigTdf);

        mGlobalMaxTotalPlayerSlotsInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMax();

        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, mPlayerSlotUtilizationRuleConfigTdf.getWeight());

        mFitFormula = FitFormula::createFitFormula(mPlayerSlotUtilizationRuleConfigTdf.getFitFormula().getName());
        if (mFitFormula == nullptr)
        {
            ERR_LOG("[PlayerSlotUtilizationRuleDefinition].create PlayerSlotUtilizationRuleDefinition mFitFormula failed. The fit formula name is " << mPlayerSlotUtilizationRuleConfigTdf.getFitFormula().getName());
            return false;
        }

        if (!mFitFormula->initialize(mPlayerSlotUtilizationRuleConfigTdf.getFitFormula(), &mPlayerSlotUtilizationRuleConfigTdf.getRangeOffsetLists()))
        {
            ERR_LOG("[PlayerSlotUtilizationRuleDefinition].initalize PlayerSlotUtilizationRuleDefinition mFitFormula failed.");
            return false;
        }

        if (!mDiagnosticsBucketPartitions.initialize(0, 100, mPlayerSlotUtilizationRuleConfigTdf.getDiagnosticBucketPartitions(), name))
        {
            return false;
        }

        return mRangeListContainer.initialize(getName(), mPlayerSlotUtilizationRuleConfigTdf.getRangeOffsetLists());
    }

    void PlayerSlotUtilizationRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        char8_t buf[512];
        mPlayerSlotUtilizationRuleConfigTdf.print(buf, sizeof(buf));
        dest.append_sprintf("%s %s", prefix, buf);
    }

    void PlayerSlotUtilizationRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // post rete eval only
        ERR_LOG("[PlayerSlotUtilizationRuleDefinition].insertWorkingMemoryElements NOT IMPLEMENTED.");
    }

    void PlayerSlotUtilizationRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // post rete eval only
        ERR_LOG("[PlayerSlotUtilizationRuleDefinition].upsertWorkingMemoryElements NOT IMPLEMENTED.");
    }

    float PlayerSlotUtilizationRuleDefinition::getFitPercent(uint16_t fullSlotRateA, uint16_t fullSlotRateB) const
    {
        if (mFitFormula != nullptr)
            return mFitFormula->getFitPercent(fullSlotRateA, fullSlotRateB);
        else
        {
            EA_ASSERT(mFitFormula != nullptr);
            return 0;
        }
    }

    const RangeList* PlayerSlotUtilizationRuleDefinition::getRangeOffsetList(const char8_t *listName) const
    {
        return mRangeListContainer.getRangeList(listName);
    }


    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void PlayerSlotUtilizationRuleDefinition::updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const
    {
        cache.updatePlayerSlotUtilizationGameCounts(gameSessionSlave, increment, *this);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
