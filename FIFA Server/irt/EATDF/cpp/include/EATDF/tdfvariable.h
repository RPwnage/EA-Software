/*! *********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_VARIABLE_H
#define EA_TDF_VARIABLE_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/tdfbase.h>


/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace EA
{

namespace TDF
{

class EATDF_API VariableTdfBase : public TdfObject
#if EA_TDF_INCLUDE_CHANGE_TRACKING
,   public TdfChangeTracker
#endif
{
public:
    VariableTdfBase(EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* debugMemName = EA_TDF_DEFAULT_ALLOC_NAME)
    :   TdfObject(),
        mTdf(nullptr),
        mAllocator(allocator)
    {}

    ~VariableTdfBase() override
    {
        clear();
    }

    const TypeDescription& getTypeDescription() const override;

    bool create(TdfId tdfid, const char8_t* allocName = EA_TDF_DEFAULT_ALLOC_NAME);
    bool create(const char8_t* tdfClassName, const char8_t* allocName = EA_TDF_DEFAULT_ALLOC_NAME);

    bool isValid() const { return mTdf != static_cast<const Tdf*>(nullptr); }

    Tdf *get() { return mTdf.get(); }
    const Tdf *get() const { return mTdf.get(); }

    void set(const TdfPtr& tdf);
    void set(Tdf &tdf) { set(TdfPtr(&tdf)); }
    void clear();

    void copyInto(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, VariableTdfBase &copy) const;

    void copyIntoObject(TdfObject &copy, const MemberVisitOptions& options = MemberVisitOptions()) const override { copyInto(static_cast<VariableTdfBase&>(copy), options); }
    void copyInto(VariableTdfBase &copy, const MemberVisitOptions& options = MemberVisitOptions()) const;

    bool equalsValue(const VariableTdfBase& rhs) const;

    //  returns the VariableTdf internal memory group
    virtual EA::Allocator::ICoreAllocator& getAllocator() const { return mAllocator; }

#if EA_TDF_REGISTER_ALL
    static TypeRegistrationNode REGISTRATION_NODE;
#endif

protected:
    TdfPtr mTdf;
    EA::Allocator::ICoreAllocator& mAllocator;      // mem group to be used when allocating memory internally

    EA_TDF_NON_COPYABLE(VariableTdfBase)
};

typedef EA::TDF::tdf_ptr<EA::TDF::VariableTdfBase> VariableTdfBasePtr;


template <> 
struct TypeDescriptionSelector<VariableTdfBase> 
{
    static const TypeDescriptionObject& get()
    { 
        static TypeDescriptionObject result(TDF_ACTUAL_TYPE_VARIABLE, TDF_ACTUAL_TYPE_VARIABLE, "variable", (EA::TDF::TypeDescriptionObject::CreateFunc) &TdfObject::createInstance<VariableTdfBase>); 
        return result; 
    }
};

} //namespace TDF

} //namespace EA


namespace EA
{
    namespace Allocator
    {
        namespace detail
        {
            template <>
            inline void DeleteObject(Allocator::ICoreAllocator*, ::EA::TDF::VariableTdfBase* object) { delete object; }
        }
    }
}

#endif // EA_TDF_H

