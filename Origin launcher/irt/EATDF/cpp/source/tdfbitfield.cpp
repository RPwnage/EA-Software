/*! *********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/


/************* Include files ***************************************************************************/
#include <EATDF/tdfbitfield.h>
#include <EATDF/tdfbasetypes.h>

/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace EA
{

namespace TDF
{

TypeDescriptionBitfield::TypeDescriptionBitfield(TypeDescriptionBitfield::CreateFunc _createFunc, TypeDescriptionBitfieldMember* entries, uint32_t count, TdfId _id, const char* _fullname) 
    : TypeDescription(TDF_ACTUAL_TYPE_BITFIELD, _id, _fullname ), createFunc(_createFunc) 
{
    for(uint32_t idx = 0; idx < count; ++idx)
    {
        mBitfieldMembersMap.insert(entries[idx]); // entries[idx].mName, 
    }    
}

bool TypeDescriptionBitfield::findByName(const char8_t* name, const TypeDescriptionBitfieldMember*& value) const
{
    TdfBitfieldMembersMap::const_iterator gameSettingDesc = mBitfieldMembersMap.find(name);
    if (gameSettingDesc != mBitfieldMembersMap.end())
    {
        value = &static_cast<const EA::TDF::TypeDescriptionBitfieldMember&>(*gameSettingDesc);
        return true;
    }
    return false;
}

bool TypeDescriptionBitfield::isBitfieldMemberDesc(const TypeDescriptionBitfieldMember* value) const
{
    TdfBitfieldMembersMap::const_iterator curIter = mBitfieldMembersMap.begin();
    TdfBitfieldMembersMap::const_iterator endIter = mBitfieldMembersMap.end();

    for (; curIter != endIter; ++curIter)
    {
        if (&static_cast<const EA::TDF::TypeDescriptionBitfieldMember&>(*curIter) == value)
            return true;
    }
    return false;
}

/*! ***************************************************************/
/*! \brief Sets the value of the field. (The value is masked to the size of the field.) 
*******************************************************************/
bool TdfBitfield::setValueByName(const char8_t *name, const TdfGenericConst &value)
{
    uint32_t tempValue;
    TdfGenericReference tempRef(tempValue);
    if (value.convertToIntegral(tempRef))
    {
        return setValueByName(name, tempValue);
    }
    return false;
}
bool TdfBitfield::setValueByName(const char8_t *name, uint32_t value)
{
    const TypeDescriptionBitfieldMember* desc = nullptr;
    if (getTypeDescription().findByName(name, desc))
    {
        if (desc->mIsBool)
        {
            if (value)
                mBits |= desc->mBitMask;
            else
                mBits &= ~(desc->mBitMask);
        }
        else
        {
            mBits |= ((value << desc->mBitStart) & desc->mBitMask);
        }
        return true;
    }
    return false;
}

bool TdfBitfield::setValueByDesc(const TypeDescriptionBitfieldMember* desc, const TdfGenericConst &value)
{
    uint32_t tempValue;
    TdfGenericReference tempRef(tempValue);
    if (value.convertToIntegral(tempRef))
    {
        return setValueByDesc(desc, tempValue);
    }
    return false;
}

bool TdfBitfield::setValueByDesc(const TypeDescriptionBitfieldMember* desc, uint32_t value)
{
    if (getTypeDescription().isBitfieldMemberDesc(desc))
    {
        if (desc->mIsBool)
        {
            if (value)
                mBits |= desc->mBitMask;
            else
                mBits &= ~(desc->mBitMask);
        }
        else
        {
            mBits |= ((value << desc->mBitStart) & desc->mBitMask);
        }
        return true;
    }
    return false;
}

/*! ***************************************************************/
/*! \brief Gets the value associated with the field name, puts it in outValue.
*******************************************************************/
bool TdfBitfield::getValueByName(const char8_t *name, uint32_t& outValue) const
{
    const TypeDescriptionBitfieldMember* desc = nullptr;
    if (getTypeDescription().findByName(name, desc))
    {
        outValue = ((mBits & desc->mBitMask) >> desc->mBitStart); 
        return true;
    }
    return false;
}

bool TdfBitfield::getValueByDesc(const TypeDescriptionBitfieldMember* desc, uint32_t& outValue) const
{
    if (getTypeDescription().isBitfieldMemberDesc(desc))
    {
        outValue = ((mBits & desc->mBitMask) >> desc->mBitStart); 
        return true;
    }
    return false;
}


} //namespace TDF

} //namespace EA
