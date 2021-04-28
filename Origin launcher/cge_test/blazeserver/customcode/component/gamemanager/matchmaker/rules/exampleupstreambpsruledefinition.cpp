/*! ************************************************************************************************/
/*!
\file   exampleupstreambpsruledefinition.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "exampleupstreambpsrule.h"
#include "exampleupstreambpsruledefinition.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    // See ruledefinition.h for explanation of the RULE_DEFINE_TYPEs. Most custom rules will use RULE_DEFINITION_TYPE_SINGLE.
    DEFINE_RULE_DEF_CPP(ExampleUpstreamBPSRuleDefinition, "exampleUpstreamBPSRule", RULE_DEFINITION_TYPE_SINGLE);

    /*! The amount of values to pre-fill the Gaussian cache with to avoid run time calculations. */
    const uint32_t ExampleUpstreamBPSRuleDefinition::GAUSSIAN_PRECALC_CACHE_SIZE = 1000;

    ExampleUpstreamBPSRuleDefinition::ExampleUpstreamBPSRuleDefinition() { }
    ExampleUpstreamBPSRuleDefinition::~ExampleUpstreamBPSRuleDefinition() { }

    /*! ************************************************************************************************/
    /*! \brief Initializes the rule definition given the configuration map provided.  The map
            will be pointing to the config section that matches the rule name.

        \return success/failure
    *************************************************************************************************/
    bool ExampleUpstreamBPSRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        const ExampleUpstreamBPSRuleConfig& ruleConfig = matchmakingServerConfig.getCustomRules().getExampleUpstreamBPSRule();

        bool isCleanParse = RuleDefinition::initialize(name, salience, ruleConfig.getWeight(), ruleConfig.getMinFitThresholdLists());

        if (isCleanParse)
        {
            uint32_t bellCurveWidth = ruleConfig.getFiftyPercentFitValueDifference();

            // Initialize the Gaussian Function.  The Gaussian function is one type of curve used to
            // compare 2 values and generate a fit percent.  A linear function or other functions could be used.
            //
            // Mostly what is required here is the bell curve width (see Gaussian functions on the matchmaking dev guide)
            // and then the cache type for quick lookup and pre calculation.
            isCleanParse = mGaussianFunction.initialize(bellCurveWidth, GaussianFunction::CACHE_TYPE_ARRAY, GAUSSIAN_PRECALC_CACHE_SIZE);
        }

        return isCleanParse;
    }

    /*! ************************************************************************************************/
    /*! \brief Hook into toString, allows derived classes to write their custom rule defn info into
            an eastl string for logging.  NOTE: inefficient.

        The ruleDefn is written into the string using the config file's syntax.
        Helper subroutine to be provided by the specific implementation.

        \param[in/out]  str - the ruleDefn is written into this string, blowing away any previous value.
        \param[in]  prefix - optional whitespace string that's prepended to each line of the ruleDefn.
            Used to 'indent' a rule for more readable logs.
    *************************************************************************************************/
    void ExampleUpstreamBPSRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        dest.append_sprintf("%s  fiftyPercentFitValueDifference = %d\n", prefix, mGaussianFunction.getBellWidth());
    }

    //////////////////////////////////////////////////////////////////////////
    // Example Rule Non Derived helper functions for Example Rules
    //////////////////////////////////////////////////////////////////////////

    /*! ************************************************************************************************/
    /*! \brief Calculates the fit percent between two bits per second values on a Gaussian bell curve.

        \param[in] bpsA - The first bits per second
        \param[in] bpsB - The second bits per second
        \return the fit percent given by a Gaussian curve of the offset between the two values
    *************************************************************************************************/
    float ExampleUpstreamBPSRuleDefinition::getFitPercent(uint32_t bpsA, uint32_t bpsB) const
    {
        return mGaussianFunction.calculate(bpsA, bpsB);
    }

    /*! ************************************************************************************************/
    /*! \brief Calculates the difference in bps given the current min fit percent threshold.  This is used
            to determine the acceptable range of values given the current threshold desired +/- difference.

        \param[in] fitPercent - the fit percent at the current minfitthreshold
        \return the difference between the two values given the current fit percent
    *************************************************************************************************/
    uint32_t ExampleUpstreamBPSRuleDefinition::calcBpsDifference(float fitPercent) const
    {
        return mGaussianFunction.inverse(fitPercent);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
