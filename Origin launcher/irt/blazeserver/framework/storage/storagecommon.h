/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STORAGE_COMMON_H
#define BLAZE_STORAGE_COMMON_H

/*** Include files *******************************************************************************/
#include "EASTL/fixed_string.h"
#include "framework/callback.h"
#include "framework/redis/redisid.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
    class OwnedSliver;
    class StorageRecordSnapshot;
    typedef uint64_t StorageRecordId;
    typedef uint64_t StorageRecordVersion;
    typedef eastl::fixed_string<char8_t, 32> StorageFieldName;

    typedef Functor3<StorageRecordId, const char8_t*, EA::TDF::Tdf&> UpsertStorageFieldCb;
    typedef Functor3<StorageRecordId, const char8_t*, EA::TDF::Tdf&> EraseStorageFieldCb;

    typedef Functor2<OwnedSliver&, const StorageRecordSnapshot&> UpsertStorageRecordCb;
    typedef Functor1<StorageRecordId> EraseStorageRecordCb;

    typedef Functor2<OwnedSliver&, const StorageRecordSnapshot&> ImportStorageRecordCb;
    typedef Functor1<StorageRecordId> ExportStorageRecordCb;

    typedef eastl::vector_set<const char8_t*, CaseSensitiveStringLessThan> StorageFieldPrefixSet;

    extern const char8_t STORAGE_FIELD_PUBLIC_PREFIX[];

    /*! \brief returns candidatePrefix if successful, overlapping prefix if not. */
    const char8_t* addNonOverlappingStorageFieldPrefixToSet(StorageFieldPrefixSet& fieldPrefixSet, const char8_t* candidatePrefix);

    template <typename V>
    typename eastl::vector_map<StorageFieldName, V>::const_iterator getStorageFieldByPrefix(const eastl::vector_map<StorageFieldName, V>& fieldMap, const char8_t* fieldNamePrefix)
    {
        typename eastl::vector_map<StorageFieldName, V>::const_iterator fieldItr = fieldMap.lower_bound(fieldNamePrefix);
        typename eastl::vector_map<StorageFieldName, V>::const_iterator fieldEnd = fieldMap.end();
        if (fieldItr != fieldEnd)
        {
            size_t fieldNamePrefixLen = strlen(fieldNamePrefix);
            if (blaze_strncmp(fieldNamePrefix, fieldItr->first.c_str(), fieldNamePrefixLen) == 0)
            {
                return fieldItr;
            }
        }
        return fieldEnd;
    }

} // Blaze

namespace eastl
{
    template <>
    struct hash<Blaze::StorageFieldName>
    {
        size_t operator()(const Blaze::StorageFieldName& p) const 
        {
            return eastl::hash<const char8_t*>()(p.c_str());
        }
    };
}

#endif // BLAZE_STORAGE_LISTENER_H
