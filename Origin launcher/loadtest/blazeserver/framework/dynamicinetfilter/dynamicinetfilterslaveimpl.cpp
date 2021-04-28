/*************************************************************************************************/
/*!
    \file   dynamicinetfilterslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DynamicInetFilterSlaveImpl

    Implements the slave portion of the DynamicInetFilter component.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"

#include "framework/dynamicinetfilter/tdf/dynamicinetfilter.h"
#include "framework/dynamicinetfilter/rpc/dynamicinetfilterslave.h"

#include "framework/dynamicinetfilter/dynamicinetfilterslaveimpl.h"

namespace Blaze
{
namespace DynamicInetFilter
{
/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief DynamicInetFilterSlaveImpl

    Constructor for the DynamicInetFilterSlaveImpl object that owns the DynamicInetFilter database.
*/
/*************************************************************************************************/
DynamicInetFilterSlaveImpl::DynamicInetFilterSlaveImpl() 
{
    BLAZE_TRACE_LOG(Log::DYNAMIC_INET, "[DynamicInetFilterSlaveImpl].DynamicInetFilterSlaveImpl(): Constructing the DynamicInetFilter slave component");
    gDynamicInetFilter = this;
}

/*************************************************************************************************/
/*!
    \brief ~DynamicInetFilterSlaveImpl

    Destroys the DynamicInetFilter object.
*/
/*************************************************************************************************/
DynamicInetFilterSlaveImpl::~DynamicInetFilterSlaveImpl() 
{
    BLAZE_TRACE_LOG(Log::DYNAMIC_INET, "[DynamicInetFilterSlaveImpl].~DynamicInetFilterSlaveImpl(): Destroying the DynamicInetFilter slave component");
    gDynamicInetFilter = nullptr;
}

/*************************************************************************************************/
/*!
    \brief onConfigure

    Fetches all DynamicInetFilters from the DB, and populates the in-memory representation with
     its contents

    \return - true if successful, false otherwise
*/
/*************************************************************************************************/
bool DynamicInetFilterSlaveImpl::onConfigure()
{
    BLAZE_TRACE_LOG(Log::DYNAMIC_INET, "[DynamicInetFilterSlaveImpl].configure: configuring component.");

    clearFilterGroups();

    BLAZE_INFO_LOG(Log::DYNAMIC_INET, "[DynamicInetFilterSlaveImpl].configure: Loading IP whitelist from DB");

    if (!loadFiltersFromDb())
    {
        BLAZE_ERR_LOG(Log::DYNAMIC_INET, "[DynamicInetFilterSlaveImpl].configure: loading IP whitelist failed");
        return false;
    }
    insertFilters(mEntryList);
    mEntryList.clear();
    return true;
}

bool DynamicInetFilterSlaveImpl::onPrepareForReconfigure(const DynamicinetfilterConfig& config)
{
    BLAZE_TRACE_LOG(Log::DYNAMIC_INET, "[DynamicInetFilterSlaveImpl].reconfigure: reconfiguring component.");

    BLAZE_INFO_LOG(Log::DYNAMIC_INET, "[DynamicInetFilterSlaveImpl].reconfigure: Loading IP whitelist from DB");
    if (!loadFiltersFromDb())
    {
        BLAZE_ERR_LOG(Log::DYNAMIC_INET, "[DynamicInetFilterSlaveImpl].reconfigure: IP whitelist reload failed");
        return false;
    }
    return true;
}

bool DynamicInetFilterSlaveImpl::onValidateConfig(DynamicinetfilterConfig& config, const DynamicinetfilterConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    return true;
}

/*************************************************************************************************/
/*!
    \brief onReconfigure

    Reloads DynamicInetFilters from DB, and populates the in-memory representation with its contents

    \return - always true
*/
/*************************************************************************************************/
bool DynamicInetFilterSlaveImpl::onReconfigure()
{
    insertFilters(mEntryList);
    mEntryList.clear();
    //onReconfigure should not return false, but it should keep the state as is if the reconfigure fails.
    return true;
}

/*************************************************************************************************/
/*!
    \brief onShutdown

    Shutdown slave

*/
/*************************************************************************************************/
void DynamicInetFilterSlaveImpl::onShutdown()
{

}

/*************************************************************************************************/
/*!
    \brief onAddOrUpdateNotification

    Received when entry has been added or updated

*/
/*************************************************************************************************/
void DynamicInetFilterSlaveImpl::onAddOrUpdateNotification(const Entry &request, Blaze::UserSession *)
{
    EntryPtr entry = BLAZE_NEW Entry;
    RowId rowId = request.getRowId();

    // If the entry exists in the in-memory representation, remove it from filtergroups.
    if(getFilterGroupEntry(rowId) != nullptr)
        eraseFilterGroupEntry(rowId);

    request.copyInto(*entry);
    insertFilterGroupEntry(entry);
}

/*************************************************************************************************/
/*!
    \brief onRemoveNotification

    Received when entry has been removed

*/
/*************************************************************************************************/
void DynamicInetFilterSlaveImpl::onRemoveNotification(const Blaze::DynamicInetFilter::RemoveRequest &request, Blaze::UserSession *)
{
    if(getFilterGroupEntry(request.getRowId()) != nullptr)
        eraseFilterGroupEntry(request.getRowId());
}

/*************************************************************************************************/
/*!
    \brief loadFiltersFromDb

    Reloads DynamicInetFilters from DB.
    If there are any problems retrieving the DB contents, the in-memory representation remains unchanged

    \return true is successful
*/
/*************************************************************************************************/
bool DynamicInetFilterSlaveImpl::loadFiltersFromDb()
{
    bool success = true;

    // Fetch all filters from DB
    DbConnPtr dbConnPtr = gDbScheduler->getReadConnPtr(getDbId());
    if ( dbConnPtr != nullptr )
    {
        QueryPtr query = DB_NEW_QUERY_PTR(dbConnPtr);
        if (query != nullptr)
        {
            query->append("SELECT `row_id`, `group`, `owner`, `ip`, `prefix_length`, `comment` FROM dynamicinetfilter;");
            DbResultPtr dbResult;
            BlazeRpcError dbError = dbConnPtr->executeQuery(query, dbResult);
            if(dbError != DB_ERR_OK)
            {
                success = false;
                BLAZE_ERR_LOG(Log::DYNAMIC_INET, "[DynamicInetFilterSlave].loadFiltersFromDb: Failed retrieving IP whitelist from DB, cannot continue");
            }
            else
            {
                for (DbResult::const_iterator dbEntry = dbResult->begin(), dbEnd = dbResult->end(); dbEntry != dbEnd; ++dbEntry)
                {
                    const DbRow* dbRow = *dbEntry;

                    EntryPtr entry = BLAZE_NEW Entry;
                    entry->setRowId(dbRow->getUInt("row_id"));
                    entry->setGroup(dbRow->getString("group"));
                    entry->setOwner(dbRow->getString("owner"));
                    entry->getSubNet().setIp(dbRow->getString("ip"));
                    entry->getSubNet().setPrefixLength(dbRow->getUInt("prefix_length"));
                    entry->setComment(dbRow->getString("comment"));
                    mEntryList.push_back(entry);
                }
            }
        }
        else
            success = false;
    }
    else
        success = false;

    return success;
}

/*************************************************************************************************/
/*!
    \brief insertFilters

    Insert entries to DynamicInetFilter.
*/
/*************************************************************************************************/
void DynamicInetFilterSlaveImpl::insertFilters(EntryList& entryList)
{
    // Clear in-memory representation
    clearFilterGroups();
    
    EntryList::iterator iter = entryList.begin();
    EntryList::iterator end = entryList.end();
    for (; iter != end; ++iter)
    {
        EntryPtr entry = (*iter);
        // If the entry doesn't exist in the in-memory representation, stuff new entry into filtergroups.
        if(getFilterGroupEntry(entry->getRowId()) == nullptr)
            insertFilterGroupEntry(entry);
    }
}

// static
DynamicInetFilterSlave* DynamicInetFilterSlave::createImpl()
{
    return BLAZE_NEW_NAMED("DynamicInetFilterSlaveImpl") DynamicInetFilterSlaveImpl();
    //return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "Component") DynamicInetFilterSlaveImpl();
}

}
} // Blaze

