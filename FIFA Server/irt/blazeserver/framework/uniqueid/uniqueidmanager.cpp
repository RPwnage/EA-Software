/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "blazerpcerrors.h"
#include "uniqueidmanager.h"

#include "framework/tdf/uniqueid_server.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/connection/selector.h"

namespace Blaze
{

UniqueIdSlave* UniqueIdSlave::createImpl()
{
    return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "Component") UniqueIdManager();
}

UniqueIdManager::UniqueIdManager()
{
    EA_ASSERT(gUniqueIdManager == nullptr);
    gUniqueIdManager = this;
}

UniqueIdManager::~UniqueIdManager()
{
    gUniqueIdManager = nullptr;    
}

bool UniqueIdManager::onValidateConfig(UniqueIdConfig& config, const UniqueIdConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    eastl::string msg;

    // check the default batch settings
    auto batchSize = config.getBatchSettings().getBatchSize();
    auto safeSize = config.getBatchSettings().getSafeSize();
    if (batchSize <= 0 || safeSize <= 0 || safeSize >= batchSize)
    {
        msg.sprintf("[UniqueIdManager].onValidateConfig: invalid default batch size(%d) and/or safe size(%d)", batchSize, safeSize);
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    for (auto itr : config.getBatchSettingsByComponent())
    {
        auto componentName = itr.first.c_str();
        auto componentId = BlazeRpcComponentDb::getComponentIdByName(componentName);
        if (componentId == Component::INVALID_COMPONENT_ID)
        {
            msg.sprintf("[UniqueIdManager].onValidateConfig: invalid component name(%s)", componentName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        batchSize = itr.second->getBatchSize();
        safeSize = itr.second->getSafeSize();
        if (batchSize <= 0 || safeSize <= 0 || safeSize >= batchSize)
        {
            msg.sprintf("[UniqueIdManager].onValidateConfig: invalid component(%s) batch size(%d) and/or safe size(%d)", componentName, batchSize, safeSize);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }

    return validationErrors.getErrorMessages().empty();
}

bool UniqueIdManager::onConfigure()
{
    return true;
}

BlazeRpcError UniqueIdManager::reserveIdType(uint16_t componentId, uint16_t idType)
{
    auto batchSize = 0, safeSize = 0;
    getBatchSettings(componentId, batchSize, safeSize);
    uint32_t dbId = getDbId();
    DbConnPtr dbConn = gDbScheduler->getConnPtr(dbId, LOGGING_CATEGORY);
    if (dbConn == nullptr)
    {
        // failed to obtain connection
        BLAZE_ERR_LOG(BlazeRpcLog::uniqueid, "[UniqueIdManager].reserveIdType: Failed to obtain connection");
        return ERR_SYSTEM;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
    if (query == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::uniqueid, "[UniqueIdManager].reserveIdType: Failed to create query");
        return ERR_SYSTEM;
    }
    // See https://dev.mysql.com/doc/refman/5.6/en/innodb-locking-reads.html, for explanation why we don't need transactions to obtain consistent incrementing counters
    query->append("INSERT INTO `unique_id` (`component_id`, `component`, `last_id`, `id_type`) VALUES ($u,'$s',$u, $u) " 
        "ON DUPLICATE KEY UPDATE `last_id` = LAST_INSERT_ID(`last_id` + $u)",
        componentId,
        BlazeRpcComponentDb::getComponentNameById(componentId),
        batchSize,
        idType,
        batchSize);

    DbResultPtr dbResult;
    BlazeRpcError dbrc = dbConn->executeQuery(query, dbResult); 
    if (dbrc != DB_ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::uniqueid, "[UniqueIdManager].reserveIdType: Query failed reason(" << getDbErrorString(dbrc) << ")");
        return ERR_SYSTEM;
    }

    uint64_t lastId = 0;
    const uint32_t affected = dbResult->getAffectedRowCount();
    if (affected == 1)
    {
        // NOTE: if affected == 1, ON DUPLICATE KEY UPDATE was never invoked (plain insert),
        // which leaves lastInsertId == 0, but we know that we inserted with last_id == batchSize, so set it here.
        lastId = batchSize;
    }
    else if (affected == 2)
    {
        // NOTE: MYSQL sets affected to 2 when ON DUPLICATE KEY UPDATE is invoked,
        // dbResult->getLastInsertId() will return the ID of the updated row.
        lastId = dbResult->getLastInsertId();
    }
    else
    {
        BLAZE_ERR_LOG(BlazeRpcLog::uniqueid, "[UniqueIdManager].reserveIdType: expected affected row count 1 or 2, actual(" << affected << ")");
        return ERR_SYSTEM;
    }

    BLAZE_TRACE_LOG(BlazeRpcLog::uniqueid, "[UniqueIdManager].reserveIdType: reserved new idRange(" << lastId - batchSize + 1 << ".." << lastId << ") for component(" << BlazeRpcComponentDb::getComponentNameById(componentId) << "), idType(" << idType << ")");

    mLastIdByComponentMap[componentId << 16 | idType] = ComponentIdInfo(componentId, idType, lastId, lastId - batchSize);

    return ERR_OK;
}

void UniqueIdManager::dbUpdateLastId(uint16_t componentId, uint16_t idType)
{        
    LastIdByComponentMap::iterator it=mLastIdByComponentMap.find(componentId << 16 | idType);
    if (it == mLastIdByComponentMap.end())
        return;

    auto batchSize = 0, safeSize = 0;
    getBatchSettings(componentId, batchSize, safeSize);

    ComponentIdInfo& info = it->second;

    uint32_t dbId = getDbId();
    DbConnPtr dbConn = gDbScheduler->getConnPtr(dbId);
    if (dbConn == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::uniqueid, "[UniqueIdManager].dbUpdateLastId: Unable to retrieve DbConnPtr for " << dbId << ".");    
        info.mUpdatingId = false;
        return;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
    if (query == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::uniqueid, "[UniqueIdManager].dbUpdateLastId: Failed to create query");
        info.mUpdatingId = false;
        return;
    }
    // See https://dev.mysql.com/doc/refman/5.6/en/innodb-locking-reads.html, for explanation why we don't need transactions to obtain consistent incrementing counters
    query->append("UPDATE `unique_id` SET `last_id` = LAST_INSERT_ID(`last_id` + $u) WHERE `component_id` = $u and `id_type` = $u;", batchSize, info.mComponentId, info.mIdType);
    DbResultPtr dbResult;
    BlazeRpcError dbrc = dbConn->executeQuery(query, dbResult);
    if (dbrc != DB_ERR_OK)
    {
        info.mUpdatingId = false;
        BLAZE_ERR_LOG(BlazeRpcLog::uniqueid, "[UniqueIdManager].dbUpdateLastId: Query failed reason(" << getDbErrorString(dbrc) << ")");
        return;
    }

    auto lastId = dbResult->getLastInsertId();

    BLAZE_TRACE_LOG(BlazeRpcLog::uniqueid, "[UniqueIdManager].dbUpdateLastId: reserved new idRange(" << lastId - batchSize + 1 << ".." << lastId << ") for component(" << BlazeRpcComponentDb::getComponentNameById(componentId) << "), idType(" << idType << "), prior idRange(" << info.mCurrentId + 1 << ".." << info.mLastId << ")");
    
    info.mLastId = lastId;
    info.mCurrentId = lastId - batchSize;
    info.mUpdatingId = false;
}

BlazeRpcError UniqueIdManager::getId(uint16_t componentId, uint16_t idType, uint64_t &id, uint64_t min)
{
    id = 0;

    LastIdByComponentMap::iterator it=mLastIdByComponentMap.find(componentId << 16 | idType);
    if (it == mLastIdByComponentMap.end())
    {        
        return UNIQUEID_ERR_NO_SUCH_COMPONENT_OR_ID_TYPE;
    }

    auto batchSize = 0, safeSize = 0;
    getBatchSettings(componentId, batchSize, safeSize);

    ComponentIdInfo& info=(it->second);
    if ( info.mCurrentId >= (info.mLastId - safeSize) && !info.mUpdatingId)
    {
        info.mUpdatingId = true;
        BLAZE_TRACE_LOG(BlazeRpcLog::uniqueid, "[UniqueIdManager].getId: component(" << info.mComponentId << ":" << componentId << 
            "), id type(" << idType << ") lastid(" << info.mLastId << ") nextid(" << (info.mCurrentId+1) << ")");

        // We cannot allow this fiber to block. Hopefully, the next batch of ids will be fetched in time
        Fiber::CreateParams params(Fiber::STACK_SMALL, getConfig().getFetchNextIdsTimeout());
        gSelector->scheduleFiberCall(this, &UniqueIdManager::dbUpdateLastId, componentId, idType, "UniqueIdManager::dbUpdateLastId", params);
    }     

    if (info.mCurrentId >= info.mLastId)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::uniqueid, "[UniqueIdManager].getId: component(" << info.mComponentId << ":" << componentId << 
            "), id type(" << idType << ") exhausted the generated ids.  Batch size(" << batchSize << "), safe size(" << safeSize << ")");
        return UNIQUEID_ERR_EXHAUST_GENERATE_IDS;
    }
    else
    {
        do {
            id = ++info.mCurrentId;     // NOTE:  This logic will always skip the first Id (0) and the first id in every batch (batchSize)
        } while (id < min);
    }
    return ERR_OK;
}

void UniqueIdManager::getBatchSettings(uint16_t componentId, int32_t& batchSize, int32_t& safeSize)
{
    batchSize = getConfig().getBatchSettings().getBatchSize();
    safeSize = getConfig().getBatchSettings().getSafeSize();

    auto componentName = BlazeRpcComponentDb::getComponentNameById(componentId);
    if (componentName == nullptr)
    {
        return;
    }

    auto itr = getConfig().getBatchSettingsByComponent().find(componentName);
    if (itr != getConfig().getBatchSettingsByComponent().end())
    {
        batchSize = itr->second->getBatchSize();
        safeSize = itr->second->getSafeSize();
    }
}

} // Blaze

