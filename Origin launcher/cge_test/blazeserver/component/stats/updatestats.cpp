/*************************************************************************************************/
/*!
    \file   updatestats.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateStats

    Update statistics.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/util/expression.h"

#include "statsconfig.h"
#include "statsslaveimpl.h"
#include "updatestatshelper.h"

namespace Blaze
{
namespace Stats
{

/*************************************************************************************************/
/*!
    \brief resolveStatValue

    Implements the ResolveVariableCb needed by the Blaze expression framework to look up the values
    of named variables.  This method is used for all rows except aggregate stat rows.  A future
    cleanup project should allow this to be consolidated with resolveAggregateStatsValue.

    \param[in]  nameSpace - the name of the category to lookup
    \param[in]  name - the name of the stat to lookup
    \param[in]  type - the data type the variable is expected to have
    \param[in]  context - opaque reference to what we know is a map of stat names to values
    \param[out] val - value to be filled in by this method
*/
/*************************************************************************************************/
void resolveStatValue(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type,
    const void* context, Blaze::Expression::ExpressionVariableVal& val)
{
    const ResolveContainer* container = static_cast<const ResolveContainer*>(context);

    UpdateRowKey key(nameSpace == nullptr ? container->categoryName : nameSpace,
        container->catRowKey->entityId, container->catRowKey->period, container->catRowKey->scopeNameValueMap);
    const StatValMap& statMap = container->updateRowMap->find(key)->second.getNewStatValMap();
    StatVal* statVal = statMap.find(name)->second;

    switch (type)
    {
        case Blaze::EXPRESSION_TYPE_INT:
            val.intVal = statVal->intVal;
            break;
        case Blaze::EXPRESSION_TYPE_FLOAT:
            val.floatVal = statVal->floatVal;
            break;
        case Blaze::EXPRESSION_TYPE_STRING:
            val.stringVal = statVal->stringVal;
            break;
        default:
            break;
    }
}

/*************************************************************************************************/
/*!
    \brief resolveAggregateStatValue

    Implements the ResolveVariableCb needed by the Blaze expression framework to look up the values
    of named variables.  This method is used specifically when updating derived stats for aggregate
    keyscope rows, as the data structures that are used to resolve stat values are not quite
    identical to the non-aggregate case.  This is a good candidate for revisting and consolidating
    in the future.

    \param[in]  nameSpace - the name of the category to lookup
    \param[in]  name - the name of the stat to lookup
    \param[in]  type - the data type the variable is expected to have
    \param[in]  context - opaque reference to what we know is a map of stat names to values
    \param[out] val - value to be filled in by this method
*/
/*************************************************************************************************/
void resolveAggregateStatValue(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type,
    const void* context, Blaze::Expression::ExpressionVariableVal& val)
{
    const ResolveAggregateContainer* container = static_cast<const ResolveAggregateContainer*>(context);

    UpdateAggregateRowKey key(nameSpace == nullptr ? container->categoryName : nameSpace,
        container->catRowKey->mEntityId, container->catRowKey->mPeriod, container->catRowKey->mRowScopesVector);
    const StatValMap& statMap = container->updateRowMap->find(key)->second;
    StatVal* statVal = statMap.find(name)->second;

    switch (type)
    {
        case Blaze::EXPRESSION_TYPE_INT:
            val.intVal = statVal->intVal;
            break;
        case Blaze::EXPRESSION_TYPE_FLOAT:
            val.floatVal = statVal->floatVal;
            break;
        case Blaze::EXPRESSION_TYPE_STRING:
            val.stringVal = statVal->stringVal;
            break;
        default:
            break;
    }
}

} // Stats
} // Blaze
