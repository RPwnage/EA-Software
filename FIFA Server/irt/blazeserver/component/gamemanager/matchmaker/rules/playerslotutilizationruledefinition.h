/*! ************************************************************************************************/
/*!
    \file   playerslotutilizationruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_PLAYER_SLOT_UTILIZATION_RULE_DEFINITION
#define BLAZE_MATCHMAKING_PLAYER_SLOT_UTILIZATION_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"
#include "gamemanager/matchmaker/diagnostics/bucketpartitions.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class FitFormula;

    /*! ************************************************************************************************/
    /*!
        \brief The rule definition for a matchmaking session's desired percent full.

        The range of the matchable values, at different decay times, is controlled via the rule's 'rangeOffsetList'.
        The set of possible rangeOffsetLists is provided in the matchmaking rule's server configuration.

        The PlayerSlotUtilizationRule prioritizes matches via a fitPercent with a fit formula function that's clipped by min/max values
        (E.g. see http://en.wikipedia.org/wiki/Gaussian_function).  For Gaussian, instead of specifying the variance of the curve,
        we specify the 'fiftyPercentFitValueDifference'.  This is the percentFull  difference that produces a 50% fit,
        and like the variance, it controls the 'width' of the bell curve.
        
        See matchmaker rule configuration and the Dev Guide for Blaze Matchmaking for more information.
    ***************************************************************************************************/
    class PlayerSlotUtilizationRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(PlayerSlotUtilizationRuleDefinition);
        DEFINE_RULE_DEF_H(PlayerSlotUtilizationRuleDefinition, PlayerSlotUtilizationRule);
    public:
        PlayerSlotUtilizationRuleDefinition();
        ~PlayerSlotUtilizationRuleDefinition() override;

        const RangeList* getRangeOffsetList(const char8_t *listName) const;

        bool isDisabled() const override { return mRangeListContainer.empty(); }
        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        bool isReteCapable() const override { return false; } // post rete eval only

        //
        // From GameReteRuleDefinition
        //
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        // End from GameReteRuleDefintion

        float getFitPercent(uint16_t fullSlotRateA, uint16_t fullSlotRateB) const;

        uint16_t getGlobalMaxTotalPlayerSlotsInGame() const { return mGlobalMaxTotalPlayerSlotsInGame; }

        void updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const override;
        const BucketPartitions& getDiagnosticsBucketPartitions() const { return mDiagnosticsBucketPartitions; }

    private:
        PlayerSlotUtilizationRuleConfig mPlayerSlotUtilizationRuleConfigTdf;
        RangeListContainer mRangeListContainer;
        FitFormula* mFitFormula;
        uint16_t mGlobalMaxTotalPlayerSlotsInGame;

        BucketPartitions mDiagnosticsBucketPartitions;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_PLAYER_SLOT_UTILIZATION_RULE_DEFINITION
