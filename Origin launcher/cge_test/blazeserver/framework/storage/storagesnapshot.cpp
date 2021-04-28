/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/redis/redisresponse.h"
#include "framework/storage/storagesnapshot.h"

namespace Blaze
{

const char8_t StorageRecordSnapshot::PUB_VERSION_FIELD[] = "pub.version";

StorageRecordSnapshot::StorageRecordSnapshot(Blaze::Allocator& _allocator, int32_t _logCategory)
    : allocator(_allocator), logCategory(_logCategory), recordId(0), recordVersion(0), fieldMap(BlazeStlAllocator("StorageRecordSnapshot::FieldEntryMap", &_allocator))
{
}

bool StorageRecordSnapshot::populateFromStorage(const RedisResponse& resp)
{
    EA_ASSERT_MSG(resp.isArray(), "Response must be a redis array!");

    const uint32_t arraySize = resp.getArraySize();
    fieldMap.reserve(arraySize/2);

    EA::TDF::TdfFactory& factory = EA::TDF::TdfFactory::get();
    Heat2Decoder decoder;

    // HGETALL returns an array of field/value pairs
    for (uint32_t i = 1; i < arraySize; i += 2)
    {
        const RedisResponse& fieldValueRsp = resp.getArrayElement(i);
        EA_ASSERT_MSG(fieldValueRsp.isString(), "Field value must be a redis string!");
        const size_t fieldValueBytes = fieldValueRsp.getStringLen();
        if (fieldValueBytes > 0)
        {
            // field has been upserted
            RawBuffer fieldValueBuf((uint8_t*)fieldValueRsp.getString(), fieldValueBytes, false);
            fieldValueBuf.put(fieldValueBytes);
            EA::TDF::TdfId tdfId = 0;
            if (fieldValueBuf.datasize() < sizeof(tdfId))
            {
                BLAZE_ASSERT_LOG(logCategory, "[StorageRecordSnapshot].populateFromStorage: Malformed field contents at index:" << i);
                return false;
            }

            memcpy(&tdfId, fieldValueBuf.data(), sizeof(tdfId));
            EA::TDF::TdfPtr tdf = factory.create(tdfId, allocator, "StorageRecordSnapshot_redis");
            if (tdf != nullptr)
            {
                fieldValueBuf.pull(sizeof(tdfId));
                BlazeRpcError rc = decoder.decode(fieldValueBuf, *tdf, false);
                if (rc == ERR_OK)
                {
                    const RedisResponse& fieldNameRsp = resp.getArrayElement(i-1);
                    EA_ASSERT_MSG(fieldNameRsp.isString(), "Field name must be a redis string!");
                    const char8_t* fieldName = fieldNameRsp.getString();
                    if (strcmp(fieldName, PUB_VERSION_FIELD) == 0)
                    {
                        StorageRecordVersionTdf& recordVersionTdf = static_cast<StorageRecordVersionTdf&>(*tdf);
                        recordVersion = recordVersionTdf.getVersion();
                    }
                    else
                    {
                        fieldMap[fieldName] = tdf;
                    }
                }
                else
                {
                    BLAZE_ASSERT_LOG(logCategory, "[StorageRecordSnapshot].populateFromStorage: Failed to decode field(tdfId):" << tdfId);
                    return false;
                }
            }
            else
            {
                BLAZE_ASSERT_LOG(logCategory, "[StorageRecordSnapshot].populateFromStorage: Failed to construct field(tdfId):" << tdfId);
                return false;
            }
        }
        else
        {
            BLAZE_ASSERT_LOG(logCategory, "[StorageRecordSnapshot].populateFromStorage: Storage should not contain empty fields.");
            return false;
        }
    }

    if (recordVersion == 0)
    {
        BLAZE_ASSERT_LOG(logCategory, "[StorageRecordSnapshot].populateFromStorage: Storage record version cannot be zero.");
        return false;
    }

    return true;
}

bool StorageRecordSnapshot::populateFromReplication(const StorageRecordUpdate& update)
{
    recordId = update.getRecordId();
    recordVersion = update.getRecordVersion();

    EA::TDF::TdfFactory& factory = EA::TDF::TdfFactory::get();
    fieldMap.reserve(update.getFields().size());
    Heat2Decoder decoder;
    for (StorageRecordUpdate::FieldEntryList::const_iterator i = update.getFields().begin(), e = update.getFields().end(); i != e; ++i)
    {
        const char8_t* fieldName = (*i)->getFieldName();
        const EA::TDF::TdfBlob& fieldBlob = (*i)->getFieldValue();
        size_t size = fieldBlob.getCount();
        if (size > 0)
        {
            // field has been upserted
            RawBuffer fieldValueBuf((uint8_t*)fieldBlob.getData(), size, false);
            fieldValueBuf.put(size);
            EA::TDF::TdfId tdfId = 0;

            if (fieldValueBuf.datasize() < sizeof(tdfId))
            {
                BLAZE_ASSERT_LOG(logCategory, "[StorageRecordSnapshot].populateFromReplication: Malformed field contents: " << fieldName);
                return false;
            }

            memcpy(&tdfId, fieldValueBuf.data(), sizeof(tdfId));
            EA::TDF::TdfPtr tdf = factory.create(tdfId, allocator, "StorageRecordSnapshot_repl");
            if (tdf != nullptr)
            {
                fieldValueBuf.pull(sizeof(tdfId));
                BlazeRpcError rc = decoder.decode(fieldValueBuf, *tdf, false);
                if (rc == ERR_OK)
                {
                    fieldMap[fieldName] = tdf;
                }
                else
                {
                    BLAZE_ASSERT_LOG(logCategory, "[StorageRecordSnapshot].populateFromReplication: Failed to decode field:" << fieldName);
                    return false;
                }
            }
            else
            {
                BLAZE_ASSERT_LOG(logCategory, "[StorageRecordSnapshot].populateFromReplication: Failed to construct field:" << fieldName);
                return false;
            }
        }
        else
        {
            // field has been erased
            fieldMap[fieldName] = nullptr;
        }
    }

    return true;
}

} // namespace Blaze

