/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef EA_TDF_TDF_BLOB_HASH_H
#define EA_TDF_TDF_BLOB_HASH_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include <EATDF/internal/config.h>


namespace EA
{
namespace TDF
{
 
    /*! ***************************************************************/
    /*! \brief hash blobs as keys in eastl containers.
    *******************************************************************/
    template <typename T> struct tdfblobhash;
    template <> struct tdfblobhash<const TdfBlob*>
    {
        size_t operator()(const TdfBlob* b) const
        {
            if (b == nullptr)
                return 0;

            const uint32_t len = b->getCount();
            uint8_t* p = b->getData();
            size_t c, result = 2166136261U;  // FNV1 hash. Perhaps the best string hash. Based off eastl char* hash.
            for (uint32_t i = 0; i < len; ++i)
            {
                c = (uint8_t)*p;
                result = (result * 16777619) ^ c;
            }
            return (size_t)result;
        }
    };

    /*! ***************************************************************/
    /*! \brief Compares the blob data.
    *******************************************************************/
    struct tdfblob_equal_to : public eastl::binary_function<const TdfBlob*, const TdfBlob*, bool>
    {
        bool operator()(const TdfBlob* a, const TdfBlob* b) const
        {
            if ((a == nullptr) || (b == nullptr))
                return (a == b);
            if (a->getCount() != b->getCount())
                return false;

            const uint32_t count = a->getCount();
            const uint8_t* aData = a->getData();
            const uint8_t* bData = b->getData();
            for (uint32_t i = 0; i < count; ++i)
            {
                if (aData[i] != bData[i])
                    return false;
            }
            return true;
        }
    };

} //namespace TDF
} //namespace EA

#endif //EA_TDF_TDF_BLOB_HASH_H
