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

#include <EATDF/tdfbase.h>
#include <EATDF/tdfvisit.h>
#include <EATDF/tdfunion.h>
#include <EATDF/tdfblob.h>
#include <EATDF/tdfbitfield.h>
#include <EATDF/tdfvariable.h>
#include <EATDF/tdfmemberiterator.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfvector.h>

namespace EA
{
namespace TDF
{

#if EA_TDF_INCLUDE_CHANGE_TRACKING


void Tdf::markSetRecursive()
{
    markSetInternal();
    TdfMemberIterator memberIt(*this);
    while (memberIt.next())
    {        
        memberIt.markSet();
    }
}

void Tdf::clearIsSetRecursive()
{
    clearIsSetInternal();
    TdfMemberIterator memberIt(*this);
    while (memberIt.next())
    {        
        memberIt.clearIsSet();
    }
}

bool Tdf::isSetRecursive() const
{
    if (isSetInternal())
        return true;

    TdfMemberIteratorConst memberIt(*this);
    while (memberIt.next())
    {        
        if (memberIt.isSet())
            return true;
    }
    return false;
}

#endif


void Tdf::visit(TdfVisitor& visitor, Tdf &rootTdf, const Tdf &referenceTdf)
{
    TdfMemberIterator memberIt(*this, false);
    TdfMemberIteratorConst referenceIt(referenceTdf);
    while (memberIt.next() && referenceIt.next())
    {
        if (visitor.preVisitCheck(rootTdf, memberIt, referenceIt))
        {
            visitor.visitReference(rootTdf, *this, memberIt.getTag(), memberIt, &referenceIt, referenceIt.getDefault(), referenceIt.getInfo()->additionalValue);
        }
    }
}



bool Tdf::visitMembers(TdfMemberVisitor& visitor, const TdfVisitContext& callingContext)
{
    const MemberVisitOptions& options = callingContext.getVisitOptions();
    TdfMemberIterator memberIt(*this);
    while (memberIt.next())
    {
        // if referenceIt is set, we do the copy below. Otherwise, we continue
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        if (options.onlyIfSet && !memberIt.isSet())
            continue;
#endif

        if (options.onlyIfNotDefault && memberIt.isDefaultValue())
            continue;

        // check if subfield is disable or match subfield tag
        int32_t memberIndex = memberIt.getIndex();
        TdfGenericReference memberIndexKey(memberIndex);
        TdfVisitContext c(callingContext, *memberIt.getInfo(), memberIndexKey, memberIt);
        if (options.subFieldTag == 0)
        {
            if (!visitor.visitContext(c))
                return false;
        }
        else 
        {
            const TdfVisitContext* parent = c.getParent();
            if (parent != nullptr && parent->isRoot()) 
            {
                // if we find the subfield, then visit its children. Otherwise, continue
                if (options.subFieldTag == memberIt.getTag()) 
                {
                    if (!visitor.visitContext(c))
                        return false;
                }
            }
            else
            {
                // Visiting the child of the subfield tdf
                if (!visitor.visitContext(c))
                    return false;
            }
        }
    }
    return true;
}

bool Tdf::visit(TdfMemberVisitor &visitor, const MemberVisitOptions& options)
{
    TdfVisitContext c(*this, options);
    return visitor.visitContext(c);
}

bool Tdf::visitMembers(TdfMemberVisitorConst& visitor, const TdfVisitContextConst& callingContext) const
{
    const MemberVisitOptions& options = callingContext.getVisitOptions();
    TdfMemberIteratorConst memberIt(*this);
    while (memberIt.next())
    {
        // if referenceIt is set, we do the copy below. Otherwise, we continue
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        if (options.onlyIfSet && !memberIt.isSet())
            continue;
#endif

        if (options.onlyIfNotDefault && memberIt.isDefaultValue())
            continue;

        int32_t memberIndex = memberIt.getIndex();
        TdfGenericReferenceConst memberIndexKey(memberIndex);
        // check if subfield is disable or match subfield tag
        TdfVisitContextConst c(callingContext, *memberIt.getInfo(), memberIndexKey, memberIt);
        if (options.subFieldTag == 0)
        {
            if (!visitor.visitContext(c))
                return false;
        }
        else 
        {
            // The subfield member has to be one of the members of the root tdf
            const TdfVisitContextConst* parent = c.getParent();
            if (parent != nullptr && parent->isRoot()) 
            {
                // if we find the subfield, then visit its children. Otherwise, skip
                if (options.subFieldTag == memberIt.getTag()) 
                {
                    if (!visitor.visitContext(c))
                        return false;
                }
            }
            else
            {
                // Visiting the child of the subfield tdf
                if (!visitor.visitContext(c))
                    return false;
            }
        }
    }
    return true;
}

bool Tdf::visit(TdfMemberVisitorConst &visitor, const MemberVisitOptions& options) const
{
    TdfVisitContextConst c(*this, options);
    return visitor.visitContext(c);   
}



void Tdf::copyIntoObject(TdfObject& _newObj, const MemberVisitOptions& options) const
{
    if (_newObj.getTdfId() != getTdfId())
    {
        EA_FAIL_MESSAGE("Attempt to copy TDF into a TDF of a different class");
        return;
    }

    Tdf& newObj = static_cast<Tdf&>(_newObj);

    TdfMemberIterator memberIt(newObj);
    TdfMemberIteratorConst referenceIt(*this);
    while (referenceIt.next())
    {
        memberIt.next();

#if EA_TDF_INCLUDE_CHANGE_TRACKING
        if (!options.onlyIfSet || referenceIt.isSet())
#endif
        {
            if (!options.onlyIfNotDefault || !referenceIt.isDefaultValue())
            {
                memberIt.setValue(referenceIt, options);
            }
        }
    }     
}

bool Tdf::equalsValue(const Tdf& rhs) const
{
    if (getTdfId() != rhs.getTdfId())
        return false;

    TdfMemberIteratorConst it(*this);
    TdfMemberIteratorConst rit(rhs);
    while (it.next())
    {
        //If for some reason rit can't move on, we are unequal.
        if (!rit.next())
            return false; 

        if (!it.equalsValue(rit))
            return false;
    }

    if (rit.next())
        return false;  //if rit can move on but it can't, we aren't equal

    return true;
}

const char8_t* Tdf::print(char8_t* buf, uint32_t bufsize, int32_t indent, bool terse) const
{
    TdfPrintHelperCharBuf helper(buf, bufsize);
    PrintEncoder printer;
    printer.print(helper, *this, indent, terse);
    return buf;
}

void Tdf::print(PrintHelper& printHelper, int32_t indent, bool terse) const
{
    PrintEncoder printer;
    printer.print(printHelper, *this, indent, terse);
}


bool TypeDescriptionClass::getMemberInfoByIndex(uint32_t index, const TdfMemberInfo*& mi) const
{
    TdfMemberInfoIterator itr(*this);
    if (itr.moveTo(index))
    {
        mi = itr.getInfo();
        return true;
    }

    return false;
}

bool TypeDescriptionClass::getMemberInfoByName(const char8_t* memberName, const TdfMemberInfo*& mi, uint32_t* memberIndex) const
{
    TdfMemberInfoIterator itr(*this);
    if (itr.find(memberName))
    {
        mi = itr.getInfo();
        if (memberIndex != nullptr)
            *memberIndex = itr.getIndex();

        return true;
    }

    return false;
}

bool TypeDescriptionClass::getMemberInfoByTag(uint32_t tag, const TdfMemberInfo*& mi, uint32_t* memberIndex) const
{
    TdfMemberInfoIterator itr(*this);    
    if (itr.find(tag))
    {
        mi = itr.getInfo();
        if (memberIndex != nullptr)
            *memberIndex = itr.getIndex();

        return true;
    }

    return false;
}

bool Tdf::getMemberInfoByIndex(uint32_t index, const TdfMemberInfo*& memberInfo) const
{
    return getClassInfo().getMemberInfoByIndex(index, memberInfo);
}

bool Tdf::getMemberInfoByName(const char8_t* memberName, const TdfMemberInfo*& memberInfo, uint32_t* memberIndex) const
{
    return getClassInfo().getMemberInfoByName(memberName, memberInfo, memberIndex);
}

bool Tdf::getMemberInfoByTag(uint32_t tag, const TdfMemberInfo*& memberInfo, uint32_t* memberIndex) const
{
    return getClassInfo().getMemberInfoByTag(tag, memberInfo, memberIndex);
}

bool Tdf::getMemberIndexByInfo(const TdfMemberInfo* memberInfo, uint32_t& index) const
{
    index = 0;
    const TdfMemberInfo* tdfMemberInfo = nullptr;
    while (getMemberInfoByIndex(index, tdfMemberInfo))
    {
        if (tdfMemberInfo == memberInfo)
            return true;

        ++index;
    }

    return false;
}

bool Tdf::getMemberNameByIndex(uint32_t index, const char8_t*& memberFieldName) const
{
    const TdfMemberInfo* tdfMemberInfo = nullptr;
    if (getMemberInfoByIndex(index, tdfMemberInfo))
    {
        memberFieldName = tdfMemberInfo->getMemberName();
        return true;
    }

    return false;
}

bool Tdf::getMemberNameByTag(uint32_t tag, const char8_t*& memberFieldName) const
{
    const TdfMemberInfo* tdfMemberInfo = nullptr;
    if (getMemberInfoByTag(tag, tdfMemberInfo))
    {
        memberFieldName = tdfMemberInfo->getMemberName();
        return true;
    }

    return false;
}

bool Tdf::getTagByMemberName(const char8_t* memberFieldName, uint32_t& tag) const
{
    const TdfMemberInfo* tdfMemberInfo = nullptr;
    if (getMemberInfoByName(memberFieldName, tdfMemberInfo))
    {
        tag = tdfMemberInfo->getTag();
        return true;
    }

    return false;
}


/*! ***************************************************************/
/*! \brief Returns the value of the member with the given name

    \param[in] memberName  The name of the member to look for.
    \param[out] value The value found.
    \return  True if the value is found, false otherwise.
********************************************************************/
bool Tdf::getValueByName(const char8_t* memberName, TdfGenericReferenceConst& value) const
{
    TdfMemberIteratorConst memberIt(*this);
    if (!memberIt.find(memberName))
        return false;

    value = memberIt;
    return true;
}

bool Tdf::getValueByName(const char8_t* memberName, TdfGenericReference& value)
{
    TdfMemberIterator memberIt(*this);
    if (!memberIt.find(memberName))
        return false;

    value = memberIt;
    return true;
}

bool Tdf::getValueByTag(uint32_t tag, TdfGenericReferenceConst &value, const TdfMemberInfo** outMemberInfo, bool* memberSet) const
{
    TdfGenericReference outVal;
    bool ret = const_cast<Tdf*>(this)->getValueByTag(tag, outVal, outMemberInfo, memberSet);
    value = outVal;
    return ret;
}

bool Tdf::getValueByTag(uint32_t tag, TdfGenericReference &value, const TdfMemberInfo** outMemberInfo, bool* memberSet) 
{
    TdfMemberIterator memberIt(*this);
    if (!memberIt.find(tag))
        return false;

    value = memberIt;

    //if caller wants the member info, give it to them.
    if (outMemberInfo != nullptr)
    {
        *outMemberInfo = memberIt.getInfo();
    }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    if (memberSet != nullptr)
    {
        *memberSet = memberIt.isSet();
    }
#endif

    return true;  //we found it, now get out of here
}

bool Tdf::getValueByTags(const uint32_t tags[], size_t tagSize, TdfGenericReferenceConst &value, const TdfMemberInfo** memberInfo, bool* memberSet) const
{
    if (tags == nullptr || tagSize == 0)
        return false;

    //Go down into the substructures
    const Tdf* currentTdf = this;
    for (size_t counter = 0; counter < tagSize - 1; ++counter)
    {
        TdfGenericReferenceConst tdfVal;
        if (currentTdf->getValueByTag(tags[counter], tdfVal))
        {
            if (tdfVal.getType() == TDF_ACTUAL_TYPE_GENERIC_TYPE)
            {
                // Get the value of the GenericTdfType. Note that this value will never
                // have type TDF_ACTUAL_TYPE_GENERIC_TYPE.
                if (tdfVal.asGenericType().isValid())
                    tdfVal = tdfVal.asGenericType().getValue();
                else
                    return false;
            }

            //advance
            switch (tdfVal.getType())
            {
            case TDF_ACTUAL_TYPE_TDF:
                currentTdf = &tdfVal.asTdf();
                break;

            case TDF_ACTUAL_TYPE_UNION:
                currentTdf = &tdfVal.asUnion();
                break;

            case TDF_ACTUAL_TYPE_VARIABLE:
                if (tdfVal.asVariable().isValid())
                    currentTdf = tdfVal.asVariable().get();
                else
                    return false;
                break;

            default:
                //no other type can have subtags, so this is a malformed request.  Bail.
                return false;
            }
        }
        else
        {
            //one of our tags not found, bail
            return false;
        }
    }

    //finally we just return the last tag out of the last TDF
    return currentTdf->getValueByTag(tags[tagSize - 1], value, memberInfo, memberSet);
}

bool Tdf::setValueByName(const char8_t* memberName, const TdfGenericConst& value)
{
    TdfMemberInfoIterator memberIt(*this);
    if (!memberIt.find(memberName))
        return false;

    return setValueByIterator(memberIt, value);
}

bool Tdf::setValueByTag(uint32_t tag, const TdfGenericConst &value)
{
    TdfMemberInfoIterator memberIt(*this);
    if (!memberIt.find(tag))
        return false;

    return setValueByIterator(memberIt, value);
}

bool Tdf::setValueByIterator(const TdfMemberInfoIterator& itr, const TdfGenericConst &value)
{
    TdfMemberIterator i(*this, itr);
    i.setValue(value);

    return true;
}

bool Tdf::fixupIterator(const TdfMemberInfoIterator& itr, TdfMemberIterator &value)
{ 
    value.assign(itr); 
    return true; 
}

bool Tdf::getIteratorByName(const char8_t* memberName, TdfMemberIterator& value)
{
    if (&value.getOwningTdf() != this)
        return false;

    TdfMemberInfoIterator memberIt(*this);
    if (!memberIt.find(memberName))
        return false;

    return fixupIterator(memberIt, value);
}

bool Tdf::getIteratorByTag(uint32_t tag, TdfMemberIterator &value)
{
    if (&value.getOwningTdf() != this)
        return false;

    TdfMemberInfoIterator memberIt(*this);
    if (!memberIt.find(tag))
        return false;

    return fixupIterator(memberIt, value);
}

bool Tdf::computeHash(TdfHashValue& hash) const
{
    TdfHashVisitor tdfHashVisitor;
    if (!visit(tdfHashVisitor, MemberVisitOptions()))
    {
        hash = 0;
        return false;
    }
    hash = tdfHashVisitor.getHash();
    return true;
}


} //namespace TDF

} //namespace EA

