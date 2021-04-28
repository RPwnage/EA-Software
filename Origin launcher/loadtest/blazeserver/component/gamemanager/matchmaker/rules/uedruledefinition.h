/*! ************************************************************************************************/
/*!
\file uedruledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_UEDRULEDEFINITION_H
#define BLAZE_GAMEMANAGER_UEDRULEDEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/groupuedexpressionlist.h"
#include "gamemanager/tdf/matchmaker_config_server.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "framework/userset/userset.h"
#include "gamemanager/matchmaker/diagnostics/bucketpartitions.h"
#include "gamemanager/matchmaker/calcuedutil.h"

namespace Blaze
{

namespace GameManager
{
namespace Matchmaker
{

    class FitFormula;

    class UEDRuleDefinition : public RuleDefinition, public CalcUEDUtil
    {
        NON_COPYABLE(UEDRuleDefinition);
        DEFINE_RULE_DEF_H(UEDRuleDefinition, UEDRule);
    public:
        UEDRuleDefinition();
        ~UEDRuleDefinition() override;

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        static void registerMultiInputValues(const char8_t* name);

        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        bool isReteCapable() const override { return true; }
        bool calculateFitScoreAfterRete() const override { return true; }

        //////////////////////////////////////////////////////////////////////////
        // GameReteRuleDefinition Functions
        //////////////////////////////////////////////////////////////////////////
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        void updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const override;

        const RangeList* getRangeOffsetList(const char8_t* thresholdName) const;

        float getFitPercent(UserExtendedDataValue myValue, UserExtendedDataValue otherValue) const;

        const char8_t* getUEDWMEAttributeNameStr() const { return mUEDRuleConfigTdf.getUserExtendedDataName(); }

        bool isDisabled() const override { return mRangeListContainer.empty(); }

        void updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const override;
        const BucketPartitions& getDiagnosticBucketPartitions() const { return mDiagnosticsBucketPartitions; }
        bool isDiagnosticsDisabled() const { return (mUEDRuleConfigTdf.getDiagnosticBucketPartitions() == 0); }

        bool isBidirectional() const { return mUEDRuleConfigTdf.getBidirectionalEvaluation(); }
    // Members
    protected:
        UEDRuleConfig mUEDRuleConfigTdf;
        RangeListContainer mRangeListContainer;

        FitFormula* mFitFormula;

        BucketPartitions mDiagnosticsBucketPartitions;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_UEDRULEDEFINITION_H
