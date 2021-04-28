/*! ************************************************************************************************/
/*!
\file uedruledefinition.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/uedruledefinition.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/rules/uedrule.h"
#include "gamemanager/matchmaker/groupvalueformulas.h"
#include "gamemanager/matchmaker/fitformula.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(UEDRuleDefinition, "userExtendedDataRuleMap", RULE_DEFINITION_TYPE_MULTI);

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    UEDRuleDefinition::UEDRuleDefinition()
        : mFitFormula(nullptr)
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    UEDRuleDefinition::~UEDRuleDefinition()
    {
        delete mFitFormula;
    }

    bool UEDRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        UEDRuleConfigMap::const_iterator iter = matchmakingServerConfig.getRules().getUserExtendedDataRuleMap().find(name);
        if (iter == matchmakingServerConfig.getRules().getUserExtendedDataRuleMap().end())
        {
            ERR_LOG("[UEDRuleDefinition].initialize failed because could not find generic rule by name " << name);
            return false;
        }
        const UEDRuleConfig& ruleConfig = *(iter->second);
        ruleConfig.copyInto(mUEDRuleConfigTdf);

        if (!CalcUEDUtil::initialize(mUEDRuleConfigTdf.getUserExtendedDataName(), mUEDRuleConfigTdf.getGroupValueFormula(), mUEDRuleConfigTdf.getRange().getMin(), mUEDRuleConfigTdf.getRange().getMax(), &mUEDRuleConfigTdf.getGroupAdjustmentFormula()))
        {
            return false;
        }

        RuleDefinition::initialize(name, salience, mUEDRuleConfigTdf.getWeight());

        mFitFormula = FitFormula::createFitFormula(mUEDRuleConfigTdf.getFitFormula().getName());
        mFitFormula->initialize(mUEDRuleConfigTdf.getFitFormula(), &mUEDRuleConfigTdf.getRangeOffsetLists());

        cacheWMEAttributeName(getUEDWMEAttributeNameStr());

        if (!mDiagnosticsBucketPartitions.initialize(getMinRange(), getMaxRange(), mUEDRuleConfigTdf.getDiagnosticBucketPartitions(), name))
        {
            return false;
        }

        const GameManager::RangeOffsetLists& lists = mUEDRuleConfigTdf.getRangeOffsetLists();
        return mRangeListContainer.initialize(getName(), lists);
    }

    void UEDRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        dest.append_sprintf("%s  %s = %s\n", prefix, "ruleDefinitionType", getType());
        MatchmakingUtil::writeUEDNameToString(dest, prefix, "UEDKey", mUEDRuleConfigTdf.getUserExtendedDataName());
        mRangeListContainer.writeRangeListsToString(dest, prefix);
    }

    const RangeList* UEDRuleDefinition::getRangeOffsetList(const char8_t *listName) const
    {
        return mRangeListContainer.getRangeList(listName);
    }

    void UEDRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        UserExtendedDataValue uedValue = gameSessionSlave.getMatchmakingGameInfoCache()->getCachedGameUEDValue(*this);

        if (uedValue == INVALID_USER_EXTENDED_DATA_VALUE)
        {
            wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), getUEDWMEAttributeNameStr(), getWMEInt64AttributeValue(uedValue), *this);
        }
        else
        {
            wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), getUEDWMEAttributeNameStr(), uedValue, *this);
        }

    }

    void UEDRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        UserExtendedDataValue uedValue = gameSessionSlave.getMatchmakingGameInfoCache()->getCachedGameUEDValue(*this);

        if (uedValue == INVALID_USER_EXTENDED_DATA_VALUE)
        {
            wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), getUEDWMEAttributeNameStr(), getWMEInt64AttributeValue(uedValue), *this);
        }
        else
        {
            wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), getUEDWMEAttributeNameStr(), uedValue, *this);
        }
    }

    void UEDRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
    {
        if (matchmakingCache.isRosterDirty())
        {
            matchmakingCache.updateGameUEDValue(*this, gameSession);
        }
    }

    float UEDRuleDefinition::getFitPercent(UserExtendedDataValue myValue, UserExtendedDataValue otherValue) const
    {
        if (mFitFormula != nullptr)
            return mFitFormula->getFitPercent(myValue, otherValue);

        return 0.0f;
    }

    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void UEDRuleDefinition::updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const
    {
        if (isDiagnosticsDisabled())
        {
            return;
        }
        cache.updateUedGameCounts(gameSessionSlave, increment, *this);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
