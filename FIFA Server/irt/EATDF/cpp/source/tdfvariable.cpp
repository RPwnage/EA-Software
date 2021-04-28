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
#include <EATDF/tdfvariable.h>

namespace EA
{
namespace TDF
{

const TypeDescription& VariableTdfBase::getTypeDescription() const { return TypeDescriptionSelector<VariableTdfBase>::get(); }

bool VariableTdfBase::create(TdfId tdfid, const char8_t* allocName)
{
#if EA_TDF_INCLUDE_CHANGE_TRACKING
    markSet();
#endif

    clear();
   
    mTdf = TdfFactory::get().create(tdfid, mAllocator, allocName);

    return isValid();
}

bool VariableTdfBase::create(const char8_t* tdfClassName, const char8_t* allocName)
{
#if EA_TDF_INCLUDE_CHANGE_TRACKING
    markSet();
#endif

    clear();

    mTdf = TdfFactory::get().create(tdfClassName, mAllocator, allocName);
    
    return isValid();
}

void VariableTdfBase::set(const TdfPtr& tdf)
{
#if EA_TDF_INCLUDE_CHANGE_TRACKING
    markSet();
#endif
    mTdf = tdf;
}

void VariableTdfBase::clear()
{
    mTdf = nullptr;
#if EA_TDF_INCLUDE_CHANGE_TRACKING
    markSet();
#endif
}


void VariableTdfBase::copyInto(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, VariableTdfBase &copy) const
{
    copyInto(copy);

    if (mTdf != static_cast<const Tdf*>(nullptr))
    {
        //Now actually visit the TDF and copy from our own.
        visitor.visit(rootTdf, parentTdf, tag, *copy.get(), *mTdf);
    }
}


void VariableTdfBase::copyInto(VariableTdfBase &copy, const MemberVisitOptions& options) const
{
    if (this == &copy)
        return;

    copy.clear();

    if (mTdf != static_cast<const Tdf*>(nullptr))
    {
        // Don't use clone() - it ignores MemberVisitOptions
        Tdf* value = mTdf->getClassInfo().createInstance(mAllocator, "Tdf::Clone");
        mTdf->copyInto(*value, options);
        copy.set(*value);
    }
}

bool VariableTdfBase::equalsValue(const VariableTdfBase& rhs) const 
{
    if (isValid() != rhs.isValid())
        return false;

    if (!isValid())
        return true;

    return get()->equalsValue(*rhs.get());
}

#if EA_TDF_REGISTER_ALL
TypeRegistrationNode VariableTdfBase::REGISTRATION_NODE(TypeDescriptionSelector<VariableTdfBase>::get(), true);
#endif


} //namespace TDF

} //namespace EA

