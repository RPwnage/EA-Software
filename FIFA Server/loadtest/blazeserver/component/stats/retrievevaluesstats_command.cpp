/*************************************************************************************************/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "statsslaveimpl.h"
#include "statsconfig.h"
#include "stats/rpc/statsslave/retrievevaluesstats_stub.h"
#include "updatestatshelper.h"

namespace Blaze
{
namespace Stats
{

class RetrieveValuesStatsCommand : public RetrieveValuesStatsCommandStub
{
public:
    RetrieveValuesStatsCommand(Message *message, RetrieveValuesStatsRequest *request, StatsSlaveImpl* componentImpl) 
    : RetrieveValuesStatsCommandStub(message, request),
      mComponent(componentImpl)
    {
    }

    RetrieveValuesStatsCommandStub::Errors execute() override
    {
        BlazeRpcError error = Blaze::ERR_OK;

        uint64_t transId = mRequest.getTransactionContextId();
        TransactionContextPtr transPtr = mComponent->getTransactionContext(transId);
        if (transPtr == nullptr)
        {
            WARN_LOG("[RetrieveValuesStatsCommand].execute: transaction Id: (" << transId << ") does not exist.");
            return ERR_SYSTEM;
        }

        StatsTransactionContext *transData = &static_cast<StatsTransactionContext&>(*transPtr);

        // 1. Get the stats:
        error = transData->fetchStats();
        if (error != ERR_OK)
        {
            return (commandErrorFromBlazeError(error));
        }

        // 2. Put the stats in the response:
        fillCacheRowList(transData, mResponse.getCacheRowList());

        // 3. Apply the initial stat updates, if they weren't before:
        if ((transData->getUpdateState() & APPLIED_UPDATES) == 0)
        {
            error = transData->calcStatsChange(true, false);
            if (error != ERR_OK)
            {
                return (commandErrorFromBlazeError(error));
            }

            // 4. Fill in the updated stats: 
            fillCacheRowList(transData, mResponse.getUpdatedCacheRowList());
        }

        return (commandErrorFromBlazeError(error));
    }

    void fillCacheRowList(StatsTransactionContext *transData, FetchedRowUpdateList& cacheList)
    {
        UpdateRowMap::const_iterator rowItr = transData->getUpdateHelper().getMap()->begin();
        UpdateRowMap::const_iterator rowEnd = transData->getUpdateHelper().getMap()->end();
        
        // loop for each row
        for (; rowItr != rowEnd; ++rowItr)
        {
            const UpdateRowKey* rowKey = &rowItr->first;
            const UpdateRow* dbRow = &rowItr->second;

            FetchedRowUpdate* cacheRowUpdate = cacheList.pull_back();
            cacheRowUpdate->setCategory(rowKey->category); 
            cacheRowUpdate->setEntityId(rowKey->entityId);
            cacheRowUpdate->setPeriodType(rowKey->period);

            // build the keyscope map
            ScopeNameValueMap::const_iterator ii = rowKey->scopeNameValueMap->begin(); 
            ScopeNameValueMap::const_iterator ee = rowKey->scopeNameValueMap->end();
            while (ii != ee)
            {
                cacheRowUpdate->getKeyScopeNameValueMap().insert(eastl::make_pair(ii->first, ii->second));
                ii++;
            }

            // fill the stats
            StatValMap::const_iterator catStatItr = dbRow->getNewStatValMap().begin();
            StatValMap::const_iterator catStatEnd = dbRow->getNewStatValMap().end();
            for (; catStatItr != catStatEnd; ++catStatItr)
            {
                FetchedStatNameValue* statNameValue = cacheRowUpdate->getStatUpdates().pull_back();
                statNameValue->setName(catStatItr->first);
                StatVal* stat = catStatItr->second;
                statNameValue->setType(stat->type);

                if (stat->type == STAT_TYPE_INT) //int
                {
                    statNameValue->setValueInt(stat->intVal);
                }
                else if (stat->type == STAT_TYPE_FLOAT)
                {
                    statNameValue->setValueFloat(stat->floatVal);
                }
                else if (stat->type == STAT_TYPE_STRING)
                {
                    statNameValue->setValueString(stat->stringVal);
                }
            }
        }
    }
private:
    StatsSlaveImpl* mComponent;
};

DEFINE_RETRIEVEVALUESSTATS_CREATE()

} // Stats
} // Blaze
