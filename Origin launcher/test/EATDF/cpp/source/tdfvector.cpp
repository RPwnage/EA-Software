/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#include <EATDF/tdfvector.h>

#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>
#include <EAAssert/eaassert.h>


namespace EA
{
namespace TDF
{

bool TdfVectorBase::equalsValue(const TdfVectorBase &rhs) const 
{
    if (getTdfId() != rhs.getTdfId() || vectorSize() != rhs.vectorSize())
        return false;
    
    size_t size = vectorSize();
    for (size_t counter = 0; counter < size; ++counter)
    {
        TdfGenericReferenceConst lh, rh;
        if (!getValueByIndex(counter, lh) || !rhs.getValueByIndex(counter, rh) || !lh.equalsValue(rh))
            return false;
    }

    return true;
}

void TdfObjectVector::visitMembers(TdfVisitor &visitor, Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, const TdfVectorBase &refVector)
{
    const TdfObjectVector &referenceVectorTyped = (const TdfObjectVector &) refVector;
    const_iterator ri = referenceVectorTyped.mTdfVector.begin();
    for(iterator i = mTdfVector.begin(), e = mTdfVector.end(); i != e; ++i, ++ri)
    {
        TdfGenericReferenceConst ref((TdfObject&)(**ri));
        visitor.visitReference(rootTdf, parentTdf, tag, TdfGenericReference((TdfObject&)(**i)), &ref);
    }
}

bool TdfObjectVector::visitMembers(TdfMemberVisitor &visitor, const TdfVisitContext& callingContext)
{        
    for (iterator i = mTdfVector.begin(), e = mTdfVector.end(); i != e; ++i)
    {
        uint64_t index = static_cast<uint64_t>((i - mTdfVector.begin()));            
        TdfGenericReferenceConst key(index);
        TdfGenericReference value((TdfObject&)(**i));
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        if (callingContext.getVisitOptions().onlyIfSet && !value.isTdfObjectSet())
            continue;
#endif

        TdfVisitContext c(callingContext, key, value);
        if (!visitor.visitContext(c))
            return false;
    }
    return true;
}

bool TdfObjectVector::visitMembers(TdfMemberVisitorConst &visitor, const TdfVisitContextConst& callingContext) const
{        
    for (const_iterator i = mTdfVector.begin(), e = mTdfVector.end(); i != e; ++i)
    {
        uint64_t index = static_cast<uint64_t>((i - mTdfVector.begin()));
        TdfGenericReferenceConst key(index);
        TdfGenericReferenceConst value((TdfObject&)(**i));
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        if (callingContext.getVisitOptions().onlyIfSet && !value.isTdfObjectSet())
            continue;
#endif

        TdfVisitContextConst c(callingContext, key, value);
        if (!visitor.visitContext(c))
            return false;
    }
    return true;
}

bool TdfObjectVector::getValueByIndex(size_t index, TdfGenericReferenceConst &value) const
{
    if (index < mTdfVector.size())
    {
        value.setRef((TdfObject&)*(mTdfVector.at(static_cast<size_type>(index))));
        return true;
    }

    return false;
}

bool TdfObjectVector::getReferenceByIndex(size_t index, TdfGenericReference &value)
{
    if (index < mTdfVector.size())
    {
        value.setRef((TdfObject&)*(mTdfVector.at(static_cast<size_type>(index))));
        return true;
    }

    return false;
}

void TdfObjectVector::copyInto(TdfObjectVector& lhs, const MemberVisitOptions& options) const
{
    if (this == &lhs)
    {
        return;
    }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    lhs.markSet();
#endif

    lhs.clearVector();
    for (const_iterator itr = mTdfVector.begin(), eitr =  mTdfVector.end(); itr != eitr; ++itr)
    {            
        (*itr)->copyIntoObject((TdfObject&)*(lhs.pull_back()), options);
    }
}

void TdfObjectVector::resize(size_type count, bool bClearVector /* = true */)
{
    if (bClearVector)
        clearVector();

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    markSet();
#endif

    size_type curSize = mTdfVector.size();
    if (count < curSize)
    {
        mTdfVector.resize(count);
    }
    else if (count > curSize)
    {
        size_type toAdd = count - curSize;
        while (toAdd--)
        {
            pull_back();
        }
    }
}

} //namespace TDF

} //namespace EA

