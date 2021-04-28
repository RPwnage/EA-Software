/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/storage/storagecommon.h"

namespace Blaze
{

const char8_t STORAGE_FIELD_PUBLIC_PREFIX[] = "pub";

const char8_t* addNonOverlappingStorageFieldPrefixToSet(StorageFieldPrefixSet& fieldPrefixSet, const char8_t* candidatePrefix)
{
    // construct a sorted set of existing, unique, non-overlapping field prefixes
    StorageFieldPrefixSet::insert_return_type ret = fieldPrefixSet.insert(candidatePrefix);
    if (ret.second)
    {
        // validate that this field does not overlap fields of previous or next prefix handlers
        if (fieldPrefixSet.begin() != ret.first)
        {
            // check that the inserted field does not overlap previous one
            const char8_t* prevFieldNamePrefix = *(ret.first-1);
            size_t prevFieldNamePrefixLen = strlen(prevFieldNamePrefix);
            if (blaze_strncmp(prevFieldNamePrefix, candidatePrefix, prevFieldNamePrefixLen) == 0)
            {
                fieldPrefixSet.erase(ret.first);
                return prevFieldNamePrefix;
            }
        }
        if (&(*fieldPrefixSet.rbegin()) != &(*ret.first))
        {
            // check that the inserted field does not overlap next one
            const char8_t* nextFieldNamePrefix = *(ret.first+1);
            size_t fieldNamePrefixLen = strlen(candidatePrefix);
            if (blaze_strncmp(nextFieldNamePrefix, candidatePrefix, fieldNamePrefixLen) == 0)
            {
                fieldPrefixSet.erase(ret.first);
                return nextFieldNamePrefix;
            }
        }
    }
    else
    {
        return *ret.first;
    }
    
    return nullptr;
}


} // namespace Blaze

