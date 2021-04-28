/*! ************************************************************************************************/
/*!
\file reteruleprovider.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_RETERULEPROVIDER_H
#define BLAZE_GAMEMANAGER_RETERULEPROVIDER_H

#include "gamemanager/rete/retetypes.h"
#include "gamemanager/rete/retenetwork.h"
#include "gamemanager/rete/reterule.h"
#include "gamemanager/rete/productionlistener.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    class ReteRule;
 
    class ReteRuleProvider
    {
    public:
        ReteRuleProvider();
        virtual ~ReteRuleProvider();
        virtual ProductionId getProductionId() const = 0;
        virtual const ReteRuleContainer& getReteRules() const = 0; // returns the rules this provider built
        virtual void initialSearchComplete() {}

        const ConditionBlockList& getConditions() const { return mConditionBlockList; }
        ConditionBlockList& getConditions() { return mConditionBlockList; }
        MatchAnyRuleList& getMatchAnyRuleList() { return mMatchAnyRuleList; }

        void addUnevaluatedDedicatedServerFitScore(FitScore fitScore) { mUnevaluatedDedicatedServerFitScore += fitScore; }
        FitScore getUevaluatedDedicatedServerFitScore() const { return mUnevaluatedDedicatedServerFitScore; }

        void initializeReteProductions(Rete::ReteNetwork& reteNetwork, ProductionListener* pL);

    protected:
        void printConditions(Rete::ReteNetwork& reteNetwork);
        void printConditionBlock(Rete::ReteNetwork& reteNetwork, const char8_t* name, Rete::ConditionBlock& block) const;

    private:
        FitScore mUnevaluatedDedicatedServerFitScore;
        ConditionBlockList mConditionBlockList;
        MatchAnyRuleList mMatchAnyRuleList;
    };

} // namespace Rete
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_RETERULEPROVIDER_H
