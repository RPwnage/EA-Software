/*! ************************************************************************************************/
/*!
    \file   totalplayerslotsruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rangelist.h"
#include "gamemanager/matchmaker/fitformula.h"
#include "gamemanager/matchmaker/rules/totalplayerslotsruledefinition.h"
#include "gamemanager/matchmaker/rules/totalplayerslotsrule.h"
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/rete/wmemanager.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(TotalPlayerSlotsRuleDefinition, "Predefined_TotalPlayerSlotsRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* TotalPlayerSlotsRuleDefinition::TOTALPLAYERSLOTSRULE_CAPACITY_RETE_KEY = "totalParticipantSlots";

    /*! ************************************************************************************************/
    /*!
        \brief Construct an uninitialized TotalPlayerSlotsRuleDefinition.  NOTE: do not use until initialized.
    *************************************************************************************************/
    TotalPlayerSlotsRuleDefinition::TotalPlayerSlotsRuleDefinition()
        : mFitFormula(nullptr), mGlobalMinPlayerCountInGame(0), mGlobalMaxTotalPlayerSlotsInGame(0)
    {
    }

    TotalPlayerSlotsRuleDefinition::~TotalPlayerSlotsRuleDefinition() 
    {
        delete mFitFormula;
    }

    bool TotalPlayerSlotsRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        eastl::string buf;

        const TotalPlayerSlotsRuleConfig& totSlotsRuleConfig = matchmakingServerConfig.getRules().getPredefined_TotalPlayerSlotsRule();

        if(!totSlotsRuleConfig.isSet())
        {
            // we allow this to be optional, given another game size rule is used
            TRACE_LOG("[TotalPlayerSlotsRuleDefinition] " << getConfigKey() << " disabled, not present in configuration.");
        }
        else
        {
            // store off a copy of my configuration.
            totSlotsRuleConfig.copyInto(mTotalPlayerSlotsRuleConfigTdf);
        }

        mGlobalMinPlayerCountInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMin();
        mGlobalMaxTotalPlayerSlotsInGame = (uint16_t)matchmakingServerConfig.getGameSizeRange().getMax();

        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, mTotalPlayerSlotsRuleConfigTdf.getWeight());

        mFitFormula = FitFormula::createFitFormula(mTotalPlayerSlotsRuleConfigTdf.getFitFormula().getName());
        if (mFitFormula == nullptr)
        {
            ERR_LOG("[TotalPlayerSlotsRuleDefinition].initialize TotalPlayerSlotsRuleDefinition mFitFormula failed. The fit formula name is " << mTotalPlayerSlotsRuleConfigTdf.getFitFormula().getName());
            return false;
        }

        if(!mFitFormula->initialize(mTotalPlayerSlotsRuleConfigTdf.getFitFormula(), &mTotalPlayerSlotsRuleConfigTdf.getRangeOffsetLists()))
        {
            ERR_LOG("[TotalPlayerSlotsRuleDefinition].initalize TotalPlayerSlotsRuleDefinition mFitFormula failed.");
            return false;
        }

        cacheWMEAttributeName(TOTALPLAYERSLOTSRULE_CAPACITY_RETE_KEY);

        return mRangeListContainer.initialize(getName(), mTotalPlayerSlotsRuleConfigTdf.getRangeOffsetLists());
    }

    void TotalPlayerSlotsRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        char8_t buf[512];
        mTotalPlayerSlotsRuleConfigTdf.print(buf, sizeof(buf));
        dest.append_sprintf("%s %s", prefix, buf);
    }

    void TotalPlayerSlotsRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        uint16_t totalParticipantCapcity = gameSessionSlave.getTotalParticipantCapacity();

        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
            TOTALPLAYERSLOTSRULE_CAPACITY_RETE_KEY, totalParticipantCapcity, *this);
    }

    void TotalPlayerSlotsRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        uint16_t totalParticipantCapcity = gameSessionSlave.getTotalParticipantCapacity();

        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            TOTALPLAYERSLOTSRULE_CAPACITY_RETE_KEY, totalParticipantCapcity, *this);
    }


    float TotalPlayerSlotsRuleDefinition::getFitPercent(uint16_t desiredSlotCountA, uint16_t desiredSlotCountB) const
    {
        if (mFitFormula != nullptr)
            return mFitFormula->getFitPercent(desiredSlotCountA, desiredSlotCountB);
        else
        {
            EA_ASSERT(mFitFormula != nullptr);
            return 0;
        }
    }

    const RangeList* TotalPlayerSlotsRuleDefinition::getRangeOffsetList(const char8_t *listName) const
    {
        return mRangeListContainer.getRangeList(listName);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
