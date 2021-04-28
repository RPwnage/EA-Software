/*! ************************************************************************************************/
/*!
    \file   playercountruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_PLAYER_COUNT_RULE_DEFINITION
#define BLAZE_MATCHMAKING_PLAYER_COUNT_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class FitFormula;

    /*! ************************************************************************************************/
    /*!
        \brief The rule definition for a matchmaking session's desired player count.

        The range of the matchable values, at different decay times, is controlled via the rule's 'rangeOffsetList'.
        The set of possible rangeOffsetLists is provided in the matchmaking rule's server configuration.

        The PlayerCountRule prioritizes matches via a fitPercent with a fit formula function that's clipped by min/max values
        (E.g. see http://en.wikipedia.org/wiki/Gaussian_function).  For Gaussian, instead of specifying the variance of the curve,
        we specify the 'fiftyPercentFitValueDifference'.  This is the playerCount difference that produces a 50% fit,
        and like the variance, it controls the 'width' of the bell curve.
        
        See matchmaker rule configuration and the Dev Guide for Blaze Matchmaking for more information.
    ***************************************************************************************************/
    class PlayerCountRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(PlayerCountRuleDefinition);
        DEFINE_RULE_DEF_H(PlayerCountRuleDefinition, PlayerCountRule);
    public:
        static const char8_t* PLAYERCOUNTRULE_PARTICIPANTCOUNT_RETE_KEY;

        PlayerCountRuleDefinition();

        //! \brief destructor
        ~PlayerCountRuleDefinition() override;

        const RangeList* getRangeOffsetList(const char8_t *listName) const;

        bool isDisabled() const override { return mRangeListContainer.empty(); }

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        //
        // From GameReteRuleDefinition
        //

        bool isReteCapable() const override { return true; }
        bool calculateFitScoreAfterRete() const override { return true; }

        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        // End from GameReteRuleDefintion


        /*! ************************************************************************************************/
        /*!
            \brief given two player counts, determine the difference, and plug that value into the fit formula function,
                returning a fitPercent between 0 and 1.

            \param[in] playerCountA one of the two playerCounts to plug into the equation
            \param[in] playerCountB one of the two playerCounts to plug into the equation
            \return the fitPercent between A & B (note: A,B is the same as B,A due to symmetry)
        ***************************************************************************************************/
        float getFitPercent(uint16_t playerCountA, uint16_t playerCountB) const;

        uint16_t getGlobalMinPlayerCountInGame() const { return mGlobalMinPlayerCountInGame; }
        uint16_t getGlobalMaxTotalPlayerSlotsInGame() const { return mGlobalMaxTotalPlayerSlotsInGame; }

    private:
        bool addRosterSizeRuleConfigSettings();
    private:
        PlayerCountRuleConfig mPlayerCountRuleConfigTdf;
        RangeListContainer mRangeListContainer;
        FitFormula* mFitFormula;
        uint16_t mGlobalMinPlayerCountInGame;
        uint16_t mGlobalMaxTotalPlayerSlotsInGame;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_PLAYER_COUNT_RULE_DEFINITION
