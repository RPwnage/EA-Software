/*! *********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

/************* Include files ***************************************************************************/
#include <EATDF/tdfunion.h>

/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace EA
{

namespace TDF
{

void TdfUnion::visitActiveMember(TdfVisitor& visitor, Tdf &rootTdf, const Tdf &referenceTdf)
{
    uint32_t activeIndex = getActiveMemberIndex();
    if (activeIndex == INVALID_MEMBER_INDEX)
        return;

    TdfMemberIterator memberIt(*this, false);
    TdfMemberIteratorConst referenceIt(referenceTdf);
    if (memberIt.next() && referenceIt.next())
    {
        if (visitor.preVisitCheck(rootTdf, memberIt, referenceIt))
        {
            visitor.visitReference(rootTdf, *this, memberIt.getTag(), memberIt, &referenceIt, referenceIt.getDefault(), referenceIt.getInfo()->additionalValue);
        }
    }
}

bool TdfUnion::getValueByTag(uint32_t tag, TdfGenericReference &value, const TdfMemberInfo** memberInfo, bool* memberSet)
{
    if (tag != RESERVED_VALUE_TAG)
        return Tdf::getValueByTag(tag, value, memberInfo, memberSet); 

    //special handling for legacy behavior for unions that don't have unique tag names per item


    TdfMemberIterator memberIt(*this);
    //first attempt to just get the active item
    if (!memberIt.next() || memberIt.getTag() != tag)
    {
        //if not, attempt to find it
        if (!memberIt.find(tag))
            return false;
    }

    value = memberIt;

    //if caller wants the member info, give it to them.
    if (memberInfo != nullptr)
    {
        *memberInfo = memberIt.getInfo();
    }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    if (memberSet != nullptr)
    {
        *memberSet = memberIt.isSet();
    }
#endif

    return true;  //we found it, now get out of here
}

bool TdfUnion::getValueByTag(uint32_t tag, TdfGenericReferenceConst &value, const TdfMemberInfo** memberInfo, bool* memberSet) const
{
    TdfMemberIteratorConst memberIt(*this);
    //first attempt to just get the active item
    if (!memberIt.next() || memberIt.getTag() != tag)
    {
        //if not, attempt to find it
        if (!memberIt.find(tag))
            return false;
    }

    // Don't change the active member for const Unions:  ()
    if (mActiveMemberIndex != INVALID_MEMBER_INDEX && (mActiveMemberIndex == (uint32_t)memberIt.getIndex() || tag == RESERVED_VALUE_TAG))
    {
        value = memberIt;
    }

    //if caller wants the member info, give it to them.
    if (memberInfo != nullptr)
    {
        *memberInfo = memberIt.getInfo();
    }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    if (memberSet != nullptr)
    {
        *memberSet = memberIt.isSet();
    }
#endif
    return true;
};

bool TdfUnion::setValueByIterator(const TdfMemberInfoIterator& itr, const TdfGenericConst &value)
{
    if (itr.isValid())
    {
        if ((uint32_t)itr.getIndex() != mActiveMemberIndex)
            switchActiveMember(itr.getIndex());

        return Tdf::setValueByIterator(itr, value);
    }

    return false;
}

bool TdfUnion::fixupIterator(const TdfMemberInfoIterator& itr, TdfMemberIterator &value)
{
    if (itr.isValid())
    {
        if ((uint32_t)itr.getIndex() != mActiveMemberIndex)
            switchActiveMember(itr.getIndex());

        value.assign(itr);
        return true;
    }

    return false;
}

void TdfUnion::copyIntoObject(TdfObject& newObj, const MemberVisitOptions& options) const
{
    // Verfy that it's the same union type
    if (getTdfId() != newObj.getTdfId())
    {
        EA_FAIL_MESSAGE("Attempt to copy TdfUnion into different (possibly non-TdfUnion) object.");
        return;
    }

    TdfUnion& newUnion = static_cast<TdfUnion&>(newObj);

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    if (!options.onlyIfSet || isSet())  //don't touch anything if we aren't set
#endif
    {
        if (!options.onlyIfNotDefault || mValue.isValid()) //a union's default is "unset", so not set == not default
        {
            if (newUnion.getActiveMemberIndex() != getActiveMemberIndex())
                newUnion.switchActiveMember(getActiveMemberIndex());
            Tdf::copyIntoObject(newObj, options);
        }
    }
}

} //namespace TDF

} //namespace EA


