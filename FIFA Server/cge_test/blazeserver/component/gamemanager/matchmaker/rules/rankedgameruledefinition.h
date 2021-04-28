/*! ************************************************************************************************/
/*!
    \file   rankedgameruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_RANKED_GAME_RULE_DEFINITION
#define BLAZE_MATCHMAKING_RANKED_GAME_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"

#include "gamemanager/matchmaker/voting.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*!
        \brief The rankedGameRule definition contains the rule's votingMethod, weight, minFitThresholds, 
            and fitPercents to use for matches and mismatches.

        Rather than having a complete fitTable with a possibleValue list, we simply define two fitPercents:
          the mismatchFitPercent is used when the ranked values don't match (ranked vs unranked; unranked vs ranked)
          otherwise, we use the matchingFitPercent (ranked vs ranked; unranked vs unranked; abstain vs anything; random vs anything)

        The rule is configured in the server matchmaking config file (See RankedGameRuleDefinition):
          
          Predefined_RankedGameRule = {
              votingMethod = OWNER_WINS
              matchingFitPercent = 1.0
              mismatchFitPercent = .5
              weight = 0;
              minFitThresholdLists = {
                  requireExactMatch = [ 0:EXACT_MATCH_REQUIRED ]
                  testDecay = [ 0:1.0, 5:.5 ]
              }
          }
    ***************************************************************************************************/
    class RankedGameRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(RankedGameRuleDefinition);
        DEFINE_RULE_DEF_H(RankedGameRuleDefinition, RankedGameRule);
    public:

        static const char8_t* RANKEDRULE_RETE_KEY;

        /*! ************************************************************************************************/
        /*!
            \brief Construct an uninitialized RankedGameRuleDefinition.  NOTE: do not use until initialized and
            at least 1 minFitThresholdList has been added (see addMinFitThresholdList and initialize).
        *************************************************************************************************/
        RankedGameRuleDefinition();

        //! \brief destructor
        ~RankedGameRuleDefinition() override {}

        //////////////////////////////////////////////////////////////////////////
        // from GameReteRuleDefinition grandparent
        //////////////////////////////////////////////////////////////////////////
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        //////////////////////////////////////////////////////////////////////////
        // from RuleDefinition
        //////////////////////////////////////////////////////////////////////////
        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;
        bool isReteCapable() const override { return true; }

        //////////////////////////////////////////////////////////////////////////
        // rule specific
        //////////////////////////////////////////////////////////////////////////
        /*! ************************************************************************************************/
        /*!
            \brief return this rule's voting method
            \return the rule's voting method
        ***************************************************************************************************/
        RuleVotingMethod getVotingMethod() const { return mVotingSystem.getVotingMethod(); }

        /*! ************************************************************************************************/
        /*!
            \brief return the fitPercent to use when two rankedGame values match.
            \return return the fitPercent to use when two rankedGame values match
        ***************************************************************************************************/
        float getMatchingFitPercent() const { return mMatchingFitPercent; }

        /*! ************************************************************************************************/
        /*!
            \brief return the fitPercent to use when two rankedGame values don't match
            \return return the fitPercent to use when two rankedGame values don't match
        ***************************************************************************************************/
        float getMismatchFitPercent() const { return mMismatchFitPercent; }

        size_t vote(const Voting::VoteTally& voteTally, size_t totalVoteCount, RuleVotingMethod votingMethod) const
        {
            return mVotingSystem.vote(voteTally, totalVoteCount, votingMethod);
        }

    protected:

        //////////////////////////////////////////////////////////////////////////
        // from GameReteRuleDefinition grandparent
        //////////////////////////////////////////////////////////////////////////
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

    private:

        /*! ************************************************************************************************/
        /*!
            \brief return the supplied fitPercent value, clamped to the range (0..1).  If the value was clamped,
            'valueClamped' is set to true.
        
            \param[in] fitPercent an unclamped fitPercent value
            \param[out] valueClamped set to true if the fitPercent was clamped (otherwise not altered)
            \return the supplied fitPercent, clamped between 0 and 1.0.
        ***************************************************************************************************/
        static float clampFitPercent(float fitPercent, bool &valueClamped);

    private:
        Voting mVotingSystem;
        float mMatchingFitPercent;
        float mMismatchFitPercent;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_RANKED_GAME_RULE_DEFINITION
