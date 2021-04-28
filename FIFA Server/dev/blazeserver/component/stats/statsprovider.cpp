/*************************************************************************************************/
/*!
    \file   statsprovider.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateStatsCommand

    Update statistics.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "statsprovider.h"
#include "updatestatshelper.h"

namespace Blaze
{
namespace Stats
{

StatsProvider::~StatsProvider() 
{
    for (ResultSetVector::const_iterator it = mResultSetVector.begin();
        it != mResultSetVector.end();
        it ++)
    {
        delete *it;
    }
}

StatsProviderResultSet::StatsProviderResultSet(
    const char8_t* category,
    StatsProvider &statsProvider)
        : mStatsProvider(statsProvider) 
{
    blaze_strnzcpy(mCategory, category, sizeof(mCategory));
}

size_t StatsProvider::getResultsetCount() const
{
    return mResultSetVector.size();
}

bool StatsProvider::hasNextResultSet() const
{
    return (mResultSetIterator != mResultSetVector.end());
}

StatsProviderResultSet* StatsProvider::getNextResultSet()
{
    if (hasNextResultSet())
        return (*mResultSetIterator++);

    return nullptr;
}


}// Stats
} // Blaze
