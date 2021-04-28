/*! *********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_UNION_H
#define EA_TDF_UNION_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/tdfbase.h>
#include <EATDF/tdfmemberiterator.h>


/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace EA
{

namespace TDF
{

#if EA_TDF_INCLUDE_CHANGE_TRACKING
#define EA_TDF_TRACK_CHANGES_BASE_CLASS_UNION() EA::TDF::TdfUnionWithChangeTracking
#else
#define EA_TDF_TRACK_CHANGES_BASE_CLASS_UNION() EA::TDF::TdfUnion
#endif


class EATDF_API TdfUnion : public Tdf
{
    EA_TDF_NON_ASSIGNABLE(TdfUnion);
    friend class TdfMemberIteratorConst;
    friend class TdfMemberIterator;
public:
    static const uint32_t RESERVED_VALUE_TAG = 0xda1b3500;  //"VALU", the magic tag for fetching the current value.
    static const uint32_t INVALID_MEMBER_INDEX = 127;

    /*! ***************************************************************************/
    /*!    \brief Switches the active member to a new member, in the process doing any
               deletions or additions of dynamic memory as needed

        \param member Member to switch to.
        \return bool Returns false if the member cannot be found, and INVALID_MEMBER_INDEX was not used
    *******************************************************************************/
    bool switchActiveMember(uint32_t member) 
    {
        const TdfMemberInfo* memberInfo = nullptr;
        if (member < getClassInfo().memberCount)
        {
            memberInfo = &getClassInfo().memberInfo[member];
            mActiveMemberIndex = member;
        }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
        if (memberInfo != nullptr)
            markSetInternal();
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

        mValue.clear();
        if (memberInfo != nullptr)
            mValue.setType(*memberInfo->getTypeDescription());
        else 
            mActiveMemberIndex = INVALID_MEMBER_INDEX;

        return (memberInfo != nullptr || member == INVALID_MEMBER_INDEX);  // Passing INVALID_MEMBER_INDEX is expected to clear the union. 
    }
   
    bool switchActiveMemberByName(const char8_t* memberName) 
    {
        const TdfMemberInfo* dummyInfo = nullptr; 
        uint32_t index = INVALID_MEMBER_INDEX;
        bool retValue = getMemberInfoByName(memberName, dummyInfo, &index);
        switchActiveMember(index);
        return retValue;
    }

    /*! ***************************************************************************/
    /*!    \brief Gets the id of the active member in the union as an integer.  Generally
               the typesafe "getActiveMember()" version should be used instead of this one.

        \return The id.
    *******************************************************************************/
    uint32_t getActiveMemberIndex() const { return mActiveMemberIndex; }

    /*! ***************************************************************************/
    /*!    \brief Specialized version of visit() that only visits the active member. 
                  Useful for output encoders like the printencoder.
    *******************************************************************************/
    void visitActiveMember(TdfVisitor& visitor, Tdf &rootTdf, const Tdf &referenceTdf);

    bool getActiveMemberValue(TdfGenericReferenceConst &value) const
    {
        if (getActiveMemberIndex() == INVALID_MEMBER_INDEX)
            return false;

        value.setRef(*(const TdfGenericValue*)&mValue);
        return true;
    }

    bool getActiveMemberValue(TdfGenericReference &value)
    {
        if (getActiveMemberIndex() == INVALID_MEMBER_INDEX)
            return false;

        value.setRef(*(TdfGenericValue*)&mValue);
        return true;
    }


#if EA_TDF_INCLUDE_CHANGE_TRACKING
    void clearIsSet() override {}
    virtual void markSet() override {}
    bool isSet() const override { return true; }
#endif

    bool getValueByTag(uint32_t tag, TdfGenericReference &value, const TdfMemberInfo** memberInfo = nullptr, bool* memberSet = nullptr) override;
    bool getValueByTag(uint32_t tag, TdfGenericReferenceConst &value, const TdfMemberInfo** memberInfo = nullptr, bool* memberSet = nullptr) const override;

    void copyIntoObject(TdfObject& newObj, const MemberVisitOptions& options) const override;
protected:
    TdfUnion(EA::Allocator::ICoreAllocator& alloc, uint8_t* placementBuf) : Tdf(), mActiveMemberIndex(INVALID_MEMBER_INDEX), mValue(alloc, placementBuf) {}
    EA::Allocator::ICoreAllocator& getAllocator() const { return mValue.getAllocator(); }

    bool isValidIterator(const TdfMemberInfoIterator& itr) const override { return (uint32_t)itr.getIndex() == mActiveMemberIndex; }
    bool setValueByIterator(const TdfMemberInfoIterator& itr, const TdfGenericConst& value) override;
    bool fixupIterator(const TdfMemberInfoIterator& itr, TdfMemberIterator &value) override;


protected:
    uint32_t mActiveMemberIndex;
    TdfGenericValuePlacement mValue;
};


#if EA_TDF_INCLUDE_CHANGE_TRACKING
class EATDF_API TdfUnionWithChangeTracking : public TdfUnion
{
    EA_TDF_NON_ASSIGNABLE(TdfUnionWithChangeTracking);

public:
    ~TdfUnionWithChangeTracking() override {}

    bool isSet() const override { return isSetRecursive(); }
    void clearIsSet() override { clearIsSetRecursive(); }

    void markMemberSet(uint32_t memberIndex, bool isSet) override { mIsMemberSet = isSet; }
    bool isMemberSet(uint32_t memberIndex) const override { return mIsMemberSet; }
protected:
    void clearIsSetInternal() override
    {
        mIsMemberSet = false;
    }

    void markSetInternal() override
    {
        mIsMemberSet = true;
    }

    bool isSetInternal() const override
    {
        return mIsMemberSet;
    }


    bool mIsMemberSet;

    TdfUnionWithChangeTracking(EA::Allocator::ICoreAllocator& allocator, uint8_t* placementBuf) : TdfUnion(allocator, placementBuf), mIsMemberSet(false) { }
};
#define EA_TDF_UNION_MARK_SET() markSet();
#else
#define EA_TDF_UNION_MARK_SET()
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

} //namespace TDF

} //namespace EA


#endif // EA_TDF_H

