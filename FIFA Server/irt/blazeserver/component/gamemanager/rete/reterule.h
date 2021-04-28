/*! ************************************************************************************************/
/*!
\file reterule.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_RETERULE_H
#define BLAZE_GAMEMANAGER_RETERULE_H

#include "gamemanager/rete/retetypes.h"
#include "gamemanager/matchmaker/matchmakingutil.h"

namespace Blaze
{

namespace Search
{
    class GameSessionSearchSlave;
}

namespace GameManager
{
namespace Rete
{
    class ReteRule : public ConditionProvider
    {
    public:
        ~ReteRule() override {}
        /*! ************************************************************************************************/
        /*!
            \brief Returns the conditions block 
        
            \return The current search conditions for this rule.
        *************************************************************************************************/
        virtual bool addConditions(ConditionBlockList& conditionBlockList) const = 0;

        virtual FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, ConditionBlockId blockId, GameManager::Matchmaker::ReadableRuleConditions& debugRuleConditions) const = 0;

        virtual bool isMatchAny() const = 0;

        virtual bool isDedicatedServerEnabled() const { return false; }

        virtual FitScore getMaxFitScore() const = 0;

        virtual bool isDisabled() const { return false; }

        virtual const char8_t* getRuleName() const = 0;

        virtual bool calculateFitScoreAfterRete() const = 0;

        virtual bool isUsingUnset() const { return false; }

        // From Condition Provider
        const char8_t* getProviderName() const override { return getRuleName(); }
        const uint32_t getProviderSalience() const override = 0;
        const FitScore getProviderMaxFitScore() const override { return getMaxFitScore(); }
    };


    typedef eastl::vector<ReteRule*> ReteRuleContainer;

} // namespace Rete
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_RETERULE_H
