
/*! *********************************************************************************************/
/*!
    \file   tdfmemberinfo.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_TDFMEMEBRINFO_H
#define EA_TDF_TDFMEMEBRINFO_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include <EATDF/internal/config.h>
#include <EATDF/typedescription.h>
#include <EATDF/time.h>


namespace EA
{
namespace TDF
{
 

//Definitions for Tdf member offsets used when initializing the TdfMemberInfo arrays
// returns the number of bytes into which a given member resides within an owning class
#define EA_TDF_OFFSETOF_MEMBER(OwningClassType, MemberName) (uint16_t)(size_t)EA_OFFSETOF(OwningClassType, MemberName)
// Does the same thing a OFFSETOF_MEMBER, but targets the Tdf portion of the member (handles the case when a member is a Tdf derived class, but has multiple inheritance)
#define EA_TDF_OFFSETOF_MEMBER_TDF(OwningClassType, MemberName, MemberType) (uint16_t)(size_t)(EA::TDF::Tdf*)((MemberType*)EA_OFFSETOF(OwningClassType, MemberName))
// Returns the number of bytes into a given class type where the actual base class begins (would usually be 0, but not in the case of multiple inheritance)
#define EA_TDF_OFFSETOF_BASE(ClassType, BaseClassType) (uint16_t)(size_t)((uint8_t*)((BaseClassType*)(ClassType*)0x01) - 0x01)
// Returns the number of bytes into a given class type where the actual Tdf begins (would usually be 0, but not in the case of multiple inheritance)
#define EA_TDF_OFFSETOF_TDF(ClassType) EA_TDF_OFFSETOF_BASE(ClassType, EA::TDF::Tdf)


#if EA_TDF_INCLUDE_RECONFIGURE_TAG_INFO
#define EA_TDF_DEFINE_RECONFIGURE_TAG_INFO(_reconfigure) , _reconfigure
#else
#define EA_TDF_DEFINE_RECONFIGURE_TAG_INFO(_reconfigure)
#endif

#if EA_TDF_INCLUDE_EXTENDED_TAG_INFO
#define EA_TDF_DEFINE_EXT_TAG_INFO( _printFormat, _desc, _default ) , _printFormat, _desc, _default
#else
#define EA_TDF_DEFINE_EXT_TAG_INFO(_printFormat, _desc, _default)
#endif

#if EA_TDF_INCLUDE_MEMBER_NAME_OVERRIDE
#define EA_TDF_DEFINE_MEMBER_NAME_OVERRIDE(_override) , _override
#else
#define EA_TDF_DEFINE_MEMBER_NAME_OVERRIDE(_override)
#endif

class TypeDescriptionEnum;

//Deprecated, use TYPE_ACTUAL_XXXX instead
typedef enum TdfMemberType {
    TDF_MEMBER_UNKNOWN = TDF_ACTUAL_TYPE_UNKNOWN,
    TDF_MEMBER_GENERIC_TYPE = TDF_ACTUAL_TYPE_GENERIC_TYPE,
    TDF_MEMBER_MAP = TDF_ACTUAL_TYPE_MAP,
    TDF_MEMBER_LIST = TDF_ACTUAL_TYPE_LIST,
    TDF_MEMBER_FLOAT = TDF_ACTUAL_TYPE_FLOAT,
    TDF_MEMBER_ENUM = TDF_ACTUAL_TYPE_ENUM,
    TDF_MEMBER_STRING = TDF_ACTUAL_TYPE_STRING,
    TDF_MEMBER_VARIABLE = TDF_ACTUAL_TYPE_VARIABLE,
    TDF_MEMBER_BITFIELD = TDF_ACTUAL_TYPE_BITFIELD,
    TDF_MEMBER_BLOB = TDF_ACTUAL_TYPE_BLOB,
    TDF_MEMBER_UNION = TDF_ACTUAL_TYPE_UNION,
    TDF_MEMBER_CLASS = TDF_ACTUAL_TYPE_TDF,
    TDF_MEMBER_OBJECT_TYPE = TDF_ACTUAL_TYPE_OBJECT_TYPE,
    TDF_MEMBER_OBJECT_ID = TDF_ACTUAL_TYPE_OBJECT_ID,
    TDF_MEMBER_TIMEVALUE = TDF_ACTUAL_TYPE_TIMEVALUE,
    TDF_MEMBER_BOOL = TDF_ACTUAL_TYPE_BOOL,
    TDF_MEMBER_INT8 = TDF_ACTUAL_TYPE_INT8,
    TDF_MEMBER_UINT8 = TDF_ACTUAL_TYPE_UINT8,
    TDF_MEMBER_INT16 = TDF_ACTUAL_TYPE_INT16,
    TDF_MEMBER_UINT16 = TDF_ACTUAL_TYPE_UINT16,
    TDF_MEMBER_INT32 = TDF_ACTUAL_TYPE_INT32,
    TDF_MEMBER_UINT32 = TDF_ACTUAL_TYPE_UINT32,
    TDF_MEMBER_INT64 = TDF_ACTUAL_TYPE_INT64,
    TDF_MEMBER_UINT64 = TDF_ACTUAL_TYPE_UINT64
} TdfMemberType;

const uint32_t TDF_MEMBER_INFO_FLAG_MASK = 0xFF;
const uint32_t TDF_MEMBER_INFO_FLAG_IS_POINTER = 0x01;

class TdfGenericValue;
class TdfGenericConst;

struct EATDF_API TdfMemberInfo
{
    //NOTE:  The below fields are optimally placed for packing 59 bytes into 64 bytes. Be wary of
    //changing anything below without investigating how this affects padding.

    enum ReconfigureSupport
    {
        RECONFIGURE_DEFAULT = 0,
        RECONFIGURE_YES = 1,
        RECONFIGURE_NO = 2
    };
    enum PrintFormat
    {
        NORMAL = 0,
        CENSOR = 1,
        HASH = 2,
        LOWER = 3
    };

// if EA_TDF_REGISTER_ALL=1, TypeDescription instances are initialized at static initialization 
// time, so store TypeDescription* for better performance.
// if EA_TDF_REGISTER_ALL=0, we drop use of dynamic initializers for TDF_MEMBER_INFO
// for smaller code size and store TypeDescriptionFunc to initialize and get the TypeDescription on-demand.
#if EA_TDF_REGISTER_ALL
    const TypeDescription* typeDesc;
#else
    TypeDescriptionFunc typeDescFunc;
#endif

    // Call getMemberName() instead of using this directly, unless you want to explicitly avoid using the override name.  (The Base name matches the TDF field name.)
    const char8_t* memberNameBase;
#if EA_TDF_INCLUDE_MEMBER_NAME_OVERRIDE
    const char8_t* memberNameOverride;
#endif

    // breaking up the uint64_t intDefault into 2 32-bit values as using uin64_t forces 8-byte alignment
    // the splitting reduces the structure's size down from 40 bytes to 32 bytes to reduce overall static fields size
    // by approx. 25KB
    union DefaultValue
    {        
        struct {
            uint32_t high;
            uint32_t low;
        } intDefault;
        float floatDefault;
        const char8_t* stringDefault;
    } memberDefault;
    static uint64_t getDefaultValueUInt64(const DefaultValue& defaultVal) { return static_cast<const uint64_t>(((uint64_t)defaultVal.intDefault.high << 32) | defaultVal.intDefault.low); }
    static DefaultValue getDefaultValue(float val) { DefaultValue r; r.floatDefault = val; return r; }
    static DefaultValue getDefaultValue(const char8_t* val) { DefaultValue r; r.stringDefault = val; return r; }
    static DefaultValue getDefaultValue(uint64_t val) { DefaultValue r; r.intDefault.high = val >> 32; r.intDefault.low = val & 0xffffffff; return r; }


    uint32_t tagAndFlags;
    
    uint32_t additionalValue;  //< Stores the string len for strings, and the offset to base for classes/unions.

    uint16_t offset;           //< Number of bytes into the class where the member is stored.

#if EA_TDF_INCLUDE_RECONFIGURE_TAG_INFO
    ReconfigureSupport reconfig : 4;
#endif

#if EA_TDF_INCLUDE_EXTENDED_TAG_INFO
    PrintFormat printFormat : 4;
    const char8_t* description;
    const char8_t* defaultString;    
#endif


    inline uint32_t getTag() const { return tagAndFlags & ~TDF_MEMBER_INFO_FLAG_MASK; }
    inline bool isPointer() const { return tagAndFlags & TDF_MEMBER_INFO_FLAG_IS_POINTER; }
#if EA_TDF_REGISTER_ALL
    inline const TypeDescription* getTypeDescription() const { return typeDesc; }
    inline TdfType getType() const { return typeDesc->getTdfType(); }
    inline TdfId getTdfId() const { return typeDesc->getTdfId(); }
#else
    inline const TypeDescription* getTypeDescription() const { return (typeDescFunc != nullptr) ? &(*typeDescFunc)() : nullptr; }
    inline TdfType getType() const { return (typeDescFunc != nullptr) ? (*typeDescFunc)().getTdfType() : TDF_ACTUAL_TYPE_UNKNOWN; }
    inline TdfId getTdfId() const { return (typeDescFunc != nullptr) ? (*typeDescFunc)().getTdfId() : INVALID_TDF_ID; }
#endif
    inline TdfId getTypeId() const { return getTdfId(); }  // DEPRECATED

    bool getDefaultValue(TdfGenericValue& value) const;
    bool equalsDefaultValue(const TdfGenericConst& value) const;
#if EA_TDF_INCLUDE_MEMBER_NAME_OVERRIDE
    inline const char8_t* getMemberName() const { return (memberNameOverride != nullptr)? memberNameOverride : memberNameBase; }
#else
    inline const char8_t* getMemberName() const { return memberNameBase; }
#endif
};


} //namespace TDF
} //namespace EA


#endif // EA_TDF_TDFMEMEBRINFO_H

