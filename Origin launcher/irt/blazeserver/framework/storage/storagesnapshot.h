/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STORAGE_SNAPSHOT_H
#define BLAZE_STORAGE_SNAPSHOT_H

/*** Include files *******************************************************************************/
#include "framework/storage/storagecommon.h"
#include "framework/tdf/storage_server.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class RedisResponse;

class StorageRecordSnapshot
{
    NON_COPYABLE(StorageRecordSnapshot);
public:
    typedef eastl::vector_map<StorageFieldName, EA::TDF::TdfPtr> FieldEntryMap;
    static const char8_t PUB_VERSION_FIELD[];

    StorageRecordSnapshot(Blaze::Allocator& _allocator, int32_t _logCategory);
    bool populateFromStorage(const RedisResponse& resp);
    bool populateFromReplication(const StorageRecordUpdate& update);

    Blaze::Allocator& allocator;
    int32_t logCategory;
    StorageRecordId recordId;
    StorageRecordVersion recordVersion;
    FieldEntryMap fieldMap;
};

} // Blaze


#endif // BLAZE_STORAGE_SNAPSHOT_H
