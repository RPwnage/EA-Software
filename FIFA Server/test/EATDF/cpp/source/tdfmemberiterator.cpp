/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include <EATDF/tdfmemberiterator.h>
#include <EATDF/tdfbase.h>
#include <EATDF/tdfunion.h>
#include <EATDF/tdfblob.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfvector.h>
#include <EATDF/tdfvariable.h>
#include <EATDF/tdfbitfield.h>

#include <EAAssert/eaassert.h>

namespace EA
{

namespace TDF
{

TdfMemberInfoIterator::TdfMemberInfoIterator(const Tdf &tdf) :
    mClassInfo(tdf.getClassInfo())
{
    reset();
}

TdfMemberInfoIterator::TdfMemberInfoIterator(const TypeDescriptionClass &classInfo) :
    mClassInfo(classInfo)
{
    reset();
}

void TdfMemberInfoIterator::reset()
{ 
    mCurrentMemberInfo = begin() - 1;
}

bool TdfMemberInfoIterator::assign(const TdfMemberInfoIterator& other)
{
    if (other.getClassInfo().getTdfId() != getClassInfo().getTdfId())
        return false;

    mCurrentMemberInfo = other.mCurrentMemberInfo;

    return true;
}


bool TdfMemberInfoIterator::next()
{
    ++mCurrentMemberInfo;

    return !atEnd();
}

bool TdfMemberInfoIterator::find(uint32_t tag)
{
    reset();
    while (next())
    {
        if (getTag() == tag)
            return true;
    }
    return false;
}

bool TdfMemberInfoIterator::find(const char8_t* memberName)
{
    reset();
    while (next())
    {
        const TdfMemberInfo* info = getInfo();
        const char8_t* tmpMemberName = info->getMemberName();
        if (EA::StdC::Stricmp(tmpMemberName, memberName) == 0)
            return true;
    }
    return false;
}


bool TdfMemberInfoIterator::moveTo(uint32_t memberIndex)
{
    mCurrentMemberInfo = begin() + memberIndex;
    return !atEnd();
}

// we intentionally call default contructor of TdfGenericReference()
// as TdfMemberIterator directly modifies the TdfGenericReference::mTypeDesc
// and TdfGenericReference::mRefPtr in updateCurrentMember() instead of
// setting these via TdfGenericReference::setRef() which is called
// from TdfGenericReference(Tdf& tdf) constructor
TdfMemberIterator::TdfMemberIterator(Tdf &tdf, bool markOnAccess) :  
    TdfGenericReference(), // intentionally calling default constructor
    TdfMemberInfoIterator(tdf), 
    mTdf(tdf)    
{
#if EA_TDF_INCLUDE_CHANGE_TRACKING
    setMarkOnAccess(markOnAccess);
#endif
    reset();
}

TdfMemberIterator::TdfMemberIterator(Tdf& tdf, const TdfMemberInfoIterator& itr, bool markOnAccess) : 
    TdfGenericReference(), // intentionally calling default constructor
    TdfMemberInfoIterator(itr),
    mTdf(tdf)
{
#if EA_TDF_INCLUDE_CHANGE_TRACKING
    setMarkOnAccess(markOnAccess);
#endif
    updateCurrentMember();
}

void TdfMemberIterator::reset()
{
    clear();
    TdfMemberInfoIterator::reset();
}

bool TdfMemberIterator::next()
{
    while (TdfMemberInfoIterator::next() && !mTdf.isValidIterator(*this));
    updateCurrentMember();
    return !atEnd();
}

bool TdfMemberIterator::find(uint32_t tag)
{
    TdfMemberInfoIterator::find(tag);

    updateCurrentMember();
    return !atEnd();
}

bool TdfMemberIterator::find(const char8_t* memberName)
{
    TdfMemberInfoIterator::find(memberName);

    updateCurrentMember();
    return !atEnd();
}

bool TdfMemberIterator::moveTo(uint32_t memberIndex)
{
    TdfMemberInfoIterator::moveTo(memberIndex);
    updateCurrentMember();

    return !atEnd();
}

void TdfMemberIterator::assign(const TdfMemberInfoIterator& itr)
{
    if (!TdfMemberInfoIterator::assign(itr))
        return;

    updateCurrentMember();
}

#if EA_TDF_INCLUDE_CHANGE_TRACKING
bool TdfMemberIterator::isSet() const
{
    switch (getType())
    {
    case TDF_ACTUAL_TYPE_UNION:      
    case TDF_ACTUAL_TYPE_TDF:           return TdfGenericConst::asTdf().isSet();
    case TDF_ACTUAL_TYPE_VARIABLE:      return TdfGenericConst::asVariable().isSet();
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:  return TdfGenericConst::asGenericType().isSet();
    case TDF_ACTUAL_TYPE_BLOB:          return TdfGenericConst::asBlob().isSet();
    case TDF_ACTUAL_TYPE_MAP:           return TdfGenericConst::asMap().isSet();
    case TDF_ACTUAL_TYPE_LIST:          return TdfGenericConst::asList().isSet();
    default:                            return mTdf.isMemberSet(getIndex());
    }
}

void TdfMemberIterator::markSet() const
{
    switch (getType())
    {
    case TDF_ACTUAL_TYPE_UNION:  
    case TDF_ACTUAL_TYPE_TDF:           (*reinterpret_cast<Tdf*>(mRefPtr)).markSet(); break;
    case TDF_ACTUAL_TYPE_VARIABLE:      (*reinterpret_cast<VariableTdfBase*>(mRefPtr)).markSet(); break;
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:  (*reinterpret_cast<GenericTdfType*>(mRefPtr)).markSet(); break;
    case TDF_ACTUAL_TYPE_BLOB:          (*reinterpret_cast<TdfBlob*>(mRefPtr)).markSet(); break;
    case TDF_ACTUAL_TYPE_MAP:           (*reinterpret_cast<TdfMapBase*>(mRefPtr)).markSet(); break;
    case TDF_ACTUAL_TYPE_LIST:          (*reinterpret_cast<TdfVectorBase*>(mRefPtr)).markSet(); break;
    default:                            mTdf.markMemberSet(getIndex(), true); break;
    }
}

void TdfMemberIterator::clearIsSet() const
{
    switch (getType())
    {
    case TDF_ACTUAL_TYPE_UNION:
    case TDF_ACTUAL_TYPE_TDF:           (*reinterpret_cast<Tdf*>(mRefPtr)).clearIsSet(); break;
    case TDF_ACTUAL_TYPE_VARIABLE:      (*reinterpret_cast<VariableTdfBase*>(mRefPtr)).clearIsSet(); break;
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:  (*reinterpret_cast<GenericTdfType*>(mRefPtr)).clearIsSet(); break;
    case TDF_ACTUAL_TYPE_BLOB:          (*reinterpret_cast<TdfBlob*>(mRefPtr)).clearIsSet(); break;
    case TDF_ACTUAL_TYPE_MAP:           (*reinterpret_cast<TdfMapBase*>(mRefPtr)).clearIsSet(); break;
    case TDF_ACTUAL_TYPE_LIST:          (*reinterpret_cast<TdfVectorBase*>(mRefPtr)).clearIsSet(); break;
    default:                            mTdf.markMemberSet(getIndex(), false); break;
    }
}
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING


void TdfMemberIterator::updateCurrentMember()
{
    if (!TdfMemberInfoIterator::isValid() || !mTdf.isValidIterator(*this))
    {
        //we're no longer valid
        clear();
        return;
    }

    const TdfMemberInfo* currentInfo = getInfo();
    mTypeDesc = currentInfo->getTypeDescription();

    if (mTdf.getTdfType() == TDF_ACTUAL_TYPE_UNION)
    {
        mRefPtr = static_cast<TdfUnion&>(mTdf).mValue.asAny();
    }
    else
    {
        if (currentInfo->isPointer())
        {
            mRefPtr = *(uint8_t**)(reinterpret_cast<uint8_t*>(&mTdf) + currentInfo->offset);
        }
        else
        {
            mRefPtr = reinterpret_cast<uint8_t*>(&mTdf) + currentInfo->offset;
        }

        switch (mTypeDesc->getTdfType())
        {
        case TDF_ACTUAL_TYPE_LIST: 
        case TDF_ACTUAL_TYPE_MAP: 
        case TDF_ACTUAL_TYPE_TDF: 
            mRefPtr += currentInfo->additionalValue;
            break;
        default: //nothing extra to set
            break;                
        }
    }    
}


TdfMemberIteratorConst::TdfMemberIteratorConst(const Tdf &tdf) : 
    TdfGenericReferenceConst(), // intentionally calling default constructor
    TdfMemberInfoIterator(tdf), 
    mTdf(tdf)
{
    reset();
}

void TdfMemberIteratorConst::reset()
{
    clear();
    TdfMemberInfoIterator::reset();
}

bool TdfMemberIteratorConst::next()
{
    while (TdfMemberInfoIterator::next() && !mTdf.isValidIterator(*this));
    updateCurrentMember();
    return !atEnd();
}

bool TdfMemberIteratorConst::find(uint32_t tag)
{
    TdfMemberInfoIterator::find(tag);
    updateCurrentMember();
    return !atEnd();
}

bool TdfMemberIteratorConst::find(const char8_t* memberName)
{
    TdfMemberInfoIterator::find(memberName);
    updateCurrentMember();
    return !atEnd();
}

bool TdfMemberIteratorConst::moveTo(uint32_t memberIndex)
{
    TdfMemberInfoIterator::moveTo(memberIndex);
    updateCurrentMember();
    return !atEnd();
}



#if EA_TDF_INCLUDE_CHANGE_TRACKING
bool TdfMemberIteratorConst::isSet() const
{
    switch (getType())
    {
    case TDF_ACTUAL_TYPE_UNION:         return TdfGenericReferenceConst::asUnion().isSet();
    case TDF_ACTUAL_TYPE_TDF:           return TdfGenericReferenceConst::asTdf().isSet();
    case TDF_ACTUAL_TYPE_VARIABLE:      return TdfGenericReferenceConst::asVariable().isSet();
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:  return TdfGenericReferenceConst::asGenericType().isSet();
    case TDF_ACTUAL_TYPE_BLOB:          return TdfGenericReferenceConst::asBlob().isSet();
    case TDF_ACTUAL_TYPE_MAP:           return TdfGenericReferenceConst::asMap().isSet();
    case TDF_ACTUAL_TYPE_LIST:          return TdfGenericReferenceConst::asList().isSet();
    default:                            return mTdf.isMemberSet(getIndex());
    }
}
#endif


void TdfMemberIteratorConst::updateCurrentMember()
{
    if (!TdfMemberInfoIterator::isValid() || !mTdf.isValidIterator(*this))
    {           
        //Iterator and current union member don't match - just bail!
        clear();
        return;        
    }

    const TdfMemberInfo* currentInfo = getInfo();
    mTypeDesc = currentInfo->getTypeDescription();

    if (mTdf.getTdfType() == TDF_ACTUAL_TYPE_UNION)
    {
        mRefPtr = static_cast<const TdfUnion&>(mTdf).mValue.asAny();
    }
    else
    {
        if (currentInfo->isPointer())
        {
            mRefPtr = *(uint8_t**)(reinterpret_cast<const uint8_t*>(&mTdf) + currentInfo->offset);
        }
        else
        {
            mRefPtr = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&mTdf)) + currentInfo->offset;
        }

        switch (mTypeDesc->getTdfType())
        {
        case TDF_ACTUAL_TYPE_LIST: 
        case TDF_ACTUAL_TYPE_MAP: 
        case TDF_ACTUAL_TYPE_TDF: 
            mRefPtr += currentInfo->additionalValue;
            break;
        default: //nothing extra to set
            break;                
        }
    }     
}



} // namespace TDF

} // namespace EA


