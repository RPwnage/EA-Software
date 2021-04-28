/*! *********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_ENUMINFO_H
#define EA_TDF_ENUMINFO_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/typedescription.h>
#include <EASTL/intrusive_hash_map.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAHashString.h>

/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace EA
{

namespace TDF
{

struct TdfEnumInfoNameValueNode { mutable const TdfEnumInfoNameValueNode* mpNext; };  //mpNext is marked mutable as these objects should remain const after init, but 
struct TdfEnumInfoValueNameNode { mutable const TdfEnumInfoValueNameNode* mpNext; };  //are inserted into the map after creation. 
struct EATDF_API TdfEnumInfo : public TdfEnumInfoNameValueNode, public TdfEnumInfoValueNameNode
{
    TdfEnumInfo(const char8_t* name, int32_t value) : mName(name), mValue(value) {}
    const char8_t* mName;
    int32_t mValue;
};

class EATDF_API TypeDescriptionEnum : public TypeDescription
{
public:
    TypeDescriptionEnum(const TdfEnumInfo* entries, uint32_t count, TdfId _id, const char* _fullname);

    bool findByName(const char8_t* name, int32_t& value) const;
    bool findByValue(int32_t value, const char8_t** name) const;
    bool exists(int32_t value) const;

    typedef eastl::intrusive_hash_map<int32_t, const TdfEnumInfoValueNameNode, 8, eastl::hash<int32_t>, eastl::equal_to<int32_t> > NamesByValue;
    const NamesByValue& getNamesByValue() const { return mNamesByValue; }

private:
    const TdfEnumInfo* mEntries;
    uint32_t mCount;

    struct CaseInsensitiveStringEqualTo
    {
        bool operator()(const char8_t* a, const char8_t* b) const { return (EA::StdC::Stricmp(a,b) == 0); }
    };

    struct CaseInsensitiveStringHash
    {
        size_t operator()(const char8_t* p) const { return EA::StdC::FNV1_String8(p); }
    };

    typedef eastl::intrusive_hash_map<const char8_t*, const TdfEnumInfoNameValueNode, 8, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> ValuesByName;
    ValuesByName mValuesByName;
    NamesByValue mNamesByValue;
};

typedef TypeDescriptionEnum TdfEnumMap; //Deprecated, do not use.

} //namespace TDF

} //namespace EA


namespace eastl
{
    template <>
    struct use_intrusive_key<const EA::TDF::TdfEnumInfoNameValueNode, const char8_t*>
    {
        const char8_t* operator()(const EA::TDF::TdfEnumInfoNameValueNode& x) const
        {
            return static_cast<const EA::TDF::TdfEnumInfo &>(x).mName;
        }
    };

    template <>
    struct use_intrusive_key<const EA::TDF::TdfEnumInfoValueNameNode, int32_t>
    {
        int32_t operator()(const EA::TDF::TdfEnumInfoValueNameNode& x) const
        {
            return static_cast<const EA::TDF::TdfEnumInfo &>(x).mValue;
        }
    };

}


#endif // EA_TDF_H

