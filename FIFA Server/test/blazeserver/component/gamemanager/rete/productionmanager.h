/*! ************************************************************************************************/
/*!
\file productionmanager.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PRODUCTIONMANAGER_H
#define BLAZE_GAMEMANAGER_PRODUCTIONMANAGER_H

#include "EATDF/time.h"
#include "gamemanager/rete/legacyproductionlistener.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    class ReteNetwork;
    class Production;

    using EA::TDF::TimeValue;

    class ProductionManager
    {
        NON_COPYABLE(ProductionManager);
    public:
        ProductionManager(ReteNetwork* reteNetwork);
        virtual ~ProductionManager();

        void addProduction(LegacyProductionListener& pl);
        void addProduction(ProductionListener& pl, ReteRuleProvider& rl);

        void expandProduction(ProductionListener& pl) const;
        void removeProduction(ProductionListener& pl) const;

        void buildReteTree(JoinNode& currentJoinNode, const ConditionBlock& conditionBlock,
            ConditionBlock::const_iterator itr, ConditionBlock::const_iterator end, ProductionInfo& productionInfo);
    // Members
    private:
        ReteNetwork* mReteNetwork;

        uint32_t mPbiCount;
        TimeValue mPbiTime;

        uint32_t mApCount;
        TimeValue mApTime;

        uint32_t mJnWalked;

    public:
        uint32_t mJnBeta;
        uint32_t mJnAlpha;
        uint32_t mJnCreated;
        TimeValue mJnCreatedTime;
    };

} // Rete
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_PRODUCTIONMANAGER_H
