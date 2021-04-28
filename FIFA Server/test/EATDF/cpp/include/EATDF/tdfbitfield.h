/*! *********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_BITFIELD_H
#define EA_TDF_BITFIELD_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/typedescription.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAHashString.h>
#include <EASTL/intrusive_hash_map.h>
#include <EATDF/tdfobject.h>

/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace EA
{

namespace TDF
{

class TdfGenericConst;

struct TypeDescriptionBitfieldMemberNode { mutable const TypeDescriptionBitfieldMemberNode* mpNext; };
struct EATDF_API TypeDescriptionBitfieldMember : public TypeDescriptionBitfieldMemberNode
{
    TypeDescriptionBitfieldMember(const char8_t* name, uint8_t bitSize, bool isBool, uint8_t bitStart, uint32_t bitMask) : 
        mName(name), mBitSize(bitSize), mIsBool(isBool), mBitStart(bitStart), mBitMask(bitMask) {}
    const char8_t* mName;
    uint8_t mBitSize;
    bool mIsBool;
    uint8_t mBitStart;
    uint32_t mBitMask;
};

struct EATDF_API TypeDescriptionBitfield : public TypeDescription
{
    typedef TdfBitfield* (*CreateFunc)(EA::Allocator::ICoreAllocator&, const char8_t*, uint8_t* placementBuf);
    struct CaseInsensitiveStringEqualTo
    {
        bool operator()(const char8_t* a, const char8_t* b) const { return (EA::StdC::Stricmp(a,b) == 0); }
    };

    struct CaseInsensitiveStringHash
    {
        size_t operator()(const char8_t* p) const { return EA::StdC::FNV1_String8(p); }
    };

    typedef eastl::intrusive_hash_map<const char8_t*, const TypeDescriptionBitfieldMemberNode, 8, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> TdfBitfieldMembersMap;

    TypeDescriptionBitfield(CreateFunc _createFunc, TypeDescriptionBitfieldMember* entries, uint32_t count, TdfId _id, const char* _fullname);

    bool findByName(const char8_t* name, const TypeDescriptionBitfieldMember*& value) const;
    bool isBitfieldMemberDesc(const TypeDescriptionBitfieldMember* value) const;

    CreateFunc createFunc;
    TdfBitfieldMembersMap mBitfieldMembersMap;
};

typedef TypeDescriptionBitfield::CreateFunc BitfieldCreator;

/*! ***************************************************************/
/*!    \class TdfBitfield
    \brief The base bitfield class used by TDFs.
*******************************************************************/
class EATDF_API TdfBitfield
{
public:
    TdfBitfield() : mBits(0) {}
    virtual ~TdfBitfield() {}

    /*! ***************************************************************/
    /*! \brief Sets all the bits of the field.
    *******************************************************************/
    void setBits(uint32_t bits) { mBits = bits; }

    /*! ***************************************************************/
    /*! \brief Gets all the bits of the field.
    *******************************************************************/
    uint32_t getBits() const { return mBits; }
    uint32_t &getBits() { return mBits; }

    void operator=(uint32_t bits) { mBits = bits; }
    bool operator==(const TdfBitfield& lhs) const { return mBits == lhs.mBits; }

    /*! ***************************************************************/
    /*! \brief Sets the value of the field. (The value is masked to the size of the field.) 
    *******************************************************************/
    bool setValueByName(const char8_t *name, const TdfGenericConst &value);
    bool setValueByName(const char8_t *name, uint32_t value);
    bool setValueByDesc(const TypeDescriptionBitfieldMember* desc, const TdfGenericConst &value);
    bool setValueByDesc(const TypeDescriptionBitfieldMember* desc, uint32_t value);

    /*! ***************************************************************/
    /*! \brief Gets the value associated with the field name, puts it in outValue.
    *******************************************************************/
    bool getValueByName(const char8_t *name, uint32_t& outValue) const;
    bool getValueByDesc(const TypeDescriptionBitfieldMember* desc, uint32_t& outValue) const;

    /*! ***************************************************************/
    /*! \brief Get the description, for member information
    *******************************************************************/
    virtual const TypeDescriptionBitfield& getTypeDescription() const = 0;

    template <class BitfieldType>
    static TdfBitfield* createBitfield(EA::Allocator::ICoreAllocator& allocator, const char8_t* memName, uint8_t* placementBuf = nullptr) 
    { 
        return placementBuf == nullptr ? EA::TDF::TdfObjectAllocHelper() << CORE_NEW(&allocator, memName, 0) BitfieldType() : new (placementBuf) BitfieldType(); 
    }

    /* \cond INTERNAL_DOCS */
protected:
    uint32_t mBits;
    /* \endcond INTERNAL_DOCS */
};

} //namespace TDF

} //namespace EA


// Template specialization so that the lookup of the key goes to mName, rather than the default mKey. 
namespace eastl
{
    template <>
    struct use_intrusive_key<const EA::TDF::TypeDescriptionBitfieldMemberNode, const char8_t*>
    {
        const char8_t* operator()(const EA::TDF::TypeDescriptionBitfieldMemberNode& x) const
        {
            return static_cast<const EA::TDF::TypeDescriptionBitfieldMember &>(x).mName;
        }
    };
}


#endif // EA_TDF_BITFIELD_H

