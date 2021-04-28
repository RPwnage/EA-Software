/*! ************************************************************************************************/
/*!
\file legacyproductionlistener.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_LEGACYPRODUCTIONLISTENER_H
#define BLAZE_GAMEMANAGER_LEGACYPRODUCTIONLISTENER_H

#include "gamemanager/rete/productionlistener.h"
#include "gamemanager/rete/reteruleprovider.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    class LegacyProductionListener : public ProductionListener, public ReteRuleProvider
    {
    public:
        LegacyProductionListener() {};
        virtual ~LegacyProductionListener() {};
        virtual ProductionId getProductionId() const = 0;
        virtual bool onTokenAdded(const ProductionToken& token, ProductionInfo& info) = 0;
        virtual void onTokenRemoved(const ProductionToken& token, ProductionInfo& info) = 0;
        virtual void onTokenUpdated(const ProductionToken& token, ProductionInfo& info) = 0;

        virtual bool isLegacyProductionlistener() const { return true; }
        void initializeReteProductions(Rete::ReteNetwork& reteNetwork) { ReteRuleProvider::initializeReteProductions(reteNetwork, this); };

    };

} // namespace Rete
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_LEGACYPRODUCTIONLISTENER_H
