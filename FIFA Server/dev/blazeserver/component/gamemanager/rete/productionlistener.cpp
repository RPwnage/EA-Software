/*! ************************************************************************************************/
/*!
\file productionlistener.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rete/productionlistener.h"
#include "gamemanager/rete/reterule.h"
#include "gamemanager/rete/productionmanager.h"
#include "gamemanager/rpc/searchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    ProductionListener::ProductionListener()
        : mTopProductionBuildInfoSalience(0),
        mConditionInfoMap(BlazeStlAllocator("mConditionInfoMap", Search::SearchSlave::COMPONENT_MEMORY_GROUP))
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    ProductionListener::~ProductionListener()
    {
        EA_ASSERT(getProductionBuildInfoList().empty());
    }

    void ProductionListener::pushNode(const ConditionInfo& conditionInfo, ProductionInfo& productionInfo)
    {
        productionInfo.fitScore += conditionInfo.fitScore;

        ConditionInfoMap::iterator iter = mConditionInfoMap.find(conditionInfo.condition);
        if (iter != mConditionInfoMap.end())
        {
            iter->second = conditionInfo;
        }
        else
        {
            mConditionInfoMap[conditionInfo.condition] = conditionInfo;
        }
    }

    void ProductionListener::popNode(const ConditionInfo& conditionInfo, ProductionInfo& productionInfo) const
    {
        productionInfo.fitScore -= conditionInfo.fitScore;
    }

    void ProductionListener::addProductionBuildInfo(ProductionBuildInfo& info, uint64_t depth, int32_t loggingCategory)
    {
        getProductionBuildInfoList().push_back(info);

        if (depth > mTopProductionBuildInfoSalience)
        {
            BLAZE_TRACE_LOG(loggingCategory, "[ProductionListener].addProductionBuildInfo: updating production build info depth to " << depth)
                mTopProductionBuildInfoSalience = (uint32_t)depth;
            mTopCondition = info.getParent().getProductionTest();
        }
    }

    const ConditionInfo* ProductionListener::getConditionInfo(const Condition& condition) const
    {
        ConditionInfoMap::const_iterator iter = mConditionInfoMap.find(condition);
        if (iter != mConditionInfoMap.end())
        {
            return &(iter->second);
        }
        return nullptr;
    }
} // namespace Rete
} // namespace GameManager
} // namespace Blaze
