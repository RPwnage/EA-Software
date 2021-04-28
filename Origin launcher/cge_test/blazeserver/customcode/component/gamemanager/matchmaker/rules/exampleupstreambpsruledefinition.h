/*! ************************************************************************************************/
/*!
\file   exampleupstreambpsrule.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_EXAMPLE_UPSTREAM_BPS_RULE_DEFINITION_H
#define BLAZE_MATCHMAKING_EXAMPLE_UPSTREAM_BPS_RULE_DEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rules/gaussianfunction.h" // for GaussianFunction class

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class RuleDefinitionCollection;

    /*! ************************************************************************************************/
    /*!
       \brief An example custom matchmaking rule.  Calculates the fit score based on the Upstream bits
       per second.  Defines the basic methods needed to create a MM Rule Definition.

       Also defines helper methods to calculate the fit percent & difference in BPS using
       a Gaussian function.  Using the Gaussian function is not required, but is one method
       in determining the fit score given a difference of 2 values.  See the fifty percent fit
       value difference to read up more about how to configure this rule and how the Gaussian function works.
    *************************************************************************************************/
    class ExampleUpstreamBPSRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(ExampleUpstreamBPSRuleDefinition);
        DEFINE_RULE_DEF_H(ExampleUpstreamBPSRuleDefinition, ExampleUpstreamBPSRule);
    public:

        ExampleUpstreamBPSRuleDefinition();
        ~ExampleUpstreamBPSRuleDefinition() override;

        static const uint32_t GAUSSIAN_PRECALC_CACHE_SIZE;

        /*! See exampleupstreambpsruledefinition.cpp for comments */
        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;

        /*! See exampleupstreambpsruledefinition.cpp for comments */
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        /*! ************************************************************************************************/
        /*! \brief Checks to see if this rule is capable of being evaluated in Rete.
            Rete is the backbone of our find game system that is a considerable improvement
            over basic rule evaluation.  It is used for Matchmaking FindGame evaluation, and 
            GameBrowser evaluations.

            Setting this value to false will make this rule run as a post evaluation step to other rules.
            Matchmaking find game and GameBrowser will evaluate all Rete capable rules to obtain a 
            list of possible matches.  That list of games will then be evaluated by all rules that
            have this flag set to false.
        
            \return false.  The example rule does not yet use Rete
        ***************************************************************************************************/
        bool isReteCapable() const override { return false;}

        //////////////////////////////////////////////////////////////////////////
        // GameReteRuleDefinition Functions
        // Rete functionality not implemented for this rule
        //////////////////////////////////////////////////////////////////////////

        /*! ************************************************************************************************/
        /*! \brief For Rete enabled rules, this will insert working memory elements for the game.
            Note that the exampleUpsstreamBPS rule is NOT Rete capable. (see isReteCapable)
        
            \param[in] wmeManager - the working memory manager to insert the elements into
            \param[in] gameSessionSlave - the game session to evaluate.
        ***************************************************************************************************/
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            ERR_LOG("[ExampleUpstreamBPSRuleDefinition].insertWorkingMemoryElements NOT IMPLEMENTED.");
        }

        /*! ************************************************************************************************/
        /*! \brief For Rete enabled rules, this will upsert working memory elements for the game.
            Note that the exampleUpsstreamBPS rule is NOT Rete capable. (see isReteCapable)
        
            \param[in] wmeManager - the working memory manager to insert the elements into
            \param[in] gameSessionSlave - the game session to evaluate.
        ***************************************************************************************************/
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override
        {
            ERR_LOG("[ExampleUpstreamBPSRuleDefinition].upsertWorkingMemoryElements NOT IMPLEMENTED.");
        }

        //////////////////////////////////////////////////////////////////////////
        // Example Rule Non Derived helper functions for Example Rules
        //////////////////////////////////////////////////////////////////////////

        /*! See exampleupstreambpsruledefinition.cpp for comments */
        float getFitPercent(uint32_t bpsA, uint32_t bpsB) const;

        /*! See exampleupstreambpsruledefinition.cpp for comments */
        uint32_t calcBpsDifference(float fitPercent) const;

    private:
        GaussianFunction mGaussianFunction;

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_EXAMPLE_UPSTREAM_BPS_RULE_DEFINITION_H
