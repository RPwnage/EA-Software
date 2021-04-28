/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#include <EATDF/printencoder.h>

#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>
#include <EAAssert/eaassert.h>
#include <EATDF/tdfenuminfo.h>

namespace EA
{
namespace TDF
{

TypeDescriptionEnum::TypeDescriptionEnum(const TdfEnumInfo* entries, uint32_t count, TdfId _id, const char* _fullname)
    : TypeDescription(TDF_ACTUAL_TYPE_ENUM, _id, _fullname),
      mEntries(entries),
      mCount(count)
{
    for(uint32_t idx = 0; idx < count; ++idx)
    {
        mValuesByName.insert(entries[idx]);
        mNamesByValue.insert(entries[idx]);
    }
}

bool TypeDescriptionEnum::findByName(const char8_t* name, int32_t& value) const
{
    if (name == nullptr)
        return false;
    ValuesByName::const_iterator find = mValuesByName.find(name);
    if (find != mValuesByName.end())
    {
        value = ((const TdfEnumInfo&)*find).mValue;
        return true;
    }

    return false;
}

bool TypeDescriptionEnum::findByValue(int32_t value, const char8_t** name) const
{
    NamesByValue::const_iterator find = mNamesByValue.find(value);
    if (find != mNamesByValue.end())
    {
        if (name != nullptr)
            *name = ((const TdfEnumInfo&)*find).mName;
        return true;
    }

    if (name != nullptr)
        *name = "UNKNOWN";
    return  false;
}

bool TypeDescriptionEnum::exists(int32_t value) const
{
    NamesByValue::const_iterator find = mNamesByValue.find(value);
    if (find != mNamesByValue.end())
    {
        return true;
    }

    return  false;
}


} //namespace TDF

} //namespace EA

