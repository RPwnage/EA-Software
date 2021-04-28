/*! ************************************************************************************************/
/*!
\file productionlistener.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PRODUCTIONLISTENER_H
#define BLAZE_GAMEMANAGER_PRODUCTIONLISTENER_H

#include "gamemanager/rete/retetypes.h"
#include "gamemanager/rete/retenetwork.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    class ReteRule;
    class ProductionListener
    {
    public:
        ProductionListener();
        virtual ~ProductionListener();
        virtual ProductionId getProductionId() const = 0;
        virtual bool onTokenAdded(const ProductionToken& token, ProductionInfo& info) = 0;
        virtual void onTokenRemoved(const ProductionToken& token, ProductionInfo& info) = 0;
        virtual void onTokenUpdated(const ProductionToken& token, ProductionInfo& info) = 0;
        virtual void onProductionNodeHasTokens(ProductionNode& pNode, ProductionInfo& pInfo) {}
        virtual void onProductionNodeDepletedTokens(ProductionNode& pNode) {}
        virtual bool isLegacyProductionlistener() const { return false; }
        virtual void initialSearchComplete() {}

        virtual bool notifyOnTokenAdded(ProductionInfo& info) const { return true; }
        virtual bool notifyOnTokenRemoved(ProductionInfo& info) { return true; }

        void pushNode(const ConditionInfo& conditionInfo, ProductionInfo& productionInfo);
        void popNode(const ConditionInfo& conditionInfo, ProductionInfo& productionInfo) const;

        ProductionNodeList& getProductionNodeList() { return mProductionNodes; }


        ProductionBuildInfoList& getProductionBuildInfoList() { return mProductionBuildInfoList; }
        void addProductionBuildInfo(ProductionBuildInfo& info, uint64_t depth, int32_t loggingCategory);

        const ConditionInfo* getConditionInfo(const Condition& condition) const;

    protected:
        Condition mTopCondition;
        uint32_t mTopProductionBuildInfoSalience;

    private:
        typedef eastl::map<Condition, ConditionInfo> ConditionInfoMap;
        ConditionInfoMap mConditionInfoMap;
        ProductionBuildInfoList mProductionBuildInfoList;
        ProductionNodeList mProductionNodes;

    };

} // namespace Rete
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_PRODUCTIONLISTENER_H
