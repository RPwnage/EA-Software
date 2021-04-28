
/*! *********************************************************************************************/
/*!
    \file   tdfmemberinfo.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_MEMBERITERATOR_H
#define EA_TDF_MEMBERITERATOR_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include <EATDF/internal/config.h>
#include <EATDF/tdfbasetypes.h>
#include <EATDF/tdfmemberinfo.h>
#include <EATDF/tdfbase.h>
#include <EATDF/tdfobjectid.h>

namespace EA
{

namespace TDF
{

struct TypeDescriptionClass;

class EATDF_API TdfMemberInfoIterator 
{
    EA_TDF_NON_ASSIGNABLE(TdfMemberInfoIterator);
public:
    static const int32_t INVALID_MEMBER_INFO_INDEX = -1;

    TdfMemberInfoIterator(const Tdf& tdf);
    TdfMemberInfoIterator(const TypeDescriptionClass& classInfo);

    bool atBegin() const { return mCurrentMemberInfo < begin(); }
    bool atEnd() const { return mCurrentMemberInfo >= end(); }
    bool isValid() const { return mCurrentMemberInfo >= begin() && mCurrentMemberInfo < end(); }

    bool assign(const TdfMemberInfoIterator& other);
    void reset();
    bool next();
    bool find(uint32_t tag);
    bool find(const char8_t* memberName);
    bool moveTo(uint32_t memberIndex);

    inline uint32_t getTag() const { return isValid() ? mCurrentMemberInfo->getTag() : 0; }
    inline int32_t getIndex() const { return static_cast<int32_t>(mCurrentMemberInfo - begin()); }
    inline TdfMemberInfo::DefaultValue getDefault() const { return isValid() ? mCurrentMemberInfo->memberDefault : TdfMemberInfo::DefaultValue(); }

    inline const TypeDescriptionClass& getClassInfo() const { return mClassInfo; }
    inline const TdfMemberInfo* getInfo() const { return isValid() ? mCurrentMemberInfo : nullptr;  }

protected:
    const TdfMemberInfo* begin() const { return mClassInfo.memberInfo; }
    const TdfMemberInfo* end() const { return mClassInfo.memberInfo + mClassInfo.memberCount; }

    const TypeDescriptionClass& mClassInfo;
private:
    const TdfMemberInfo* mCurrentMemberInfo;

};


/*! ***************************************************************/
/*! \class TdfMemberIterator
    \brief Used to iterate over the members of a Tdf.
*******************************************************************/
class EATDF_API TdfMemberIterator : public TdfGenericReference, public TdfMemberInfoIterator
{
    EA_TDF_NON_COPYABLE(TdfMemberIterator);
    TdfGenericReference& operator=(const TdfGenericReference& lhs) {EA_FAIL(); return *this; }

public:
    static const uint32_t INVALID_MEMBER_INFO_INDEX = UINT32_MAX;

    TdfMemberIterator(Tdf& tdf, bool markOnAccess = true);
    TdfMemberIterator(Tdf& tdf, const TdfMemberInfoIterator& itr, bool markOnAccess = true);
 
    void assign(const TdfMemberInfoIterator& itr);
    inline bool isMemberValid() const { return TdfMemberInfoIterator::isValid() && TdfGenericReference::isValid(); }

    void reset();
    bool next();
    bool find(uint32_t tag);
    bool find(const char8_t* memberName);
    bool moveTo(uint32_t memberIndex);

    inline Tdf& asClass() const                      { return asTdf(); } //DEPRECATED, use asTdf()
    

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    bool isSet() const;
    void markSet() const override;
    void clearIsSet() const;
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

    Tdf& getOwningTdf() const { return mTdf; }

    bool isDefaultValue() const { return isMemberValid() ? getInfo()->equalsDefaultValue(*this) : false; }
protected:
    void updateCurrentMember();
    Tdf& mTdf;

};


/*! ***************************************************************/
/*! \class TdfMemberIteratorConst
    \brief Used to iterate over the members of a Tdf.
*******************************************************************/
class EATDF_API TdfMemberIteratorConst :  public TdfGenericReferenceConst, public TdfMemberInfoIterator
{
    EA_TDF_NON_COPYABLE(TdfMemberIteratorConst);
    TdfGenericReferenceConst& operator=(const TdfGenericReferenceConst& lhs) {EA_FAIL(); return *this; }

public:
    static const uint32_t INVALID_MEMBER_INFO_INDEX = UINT32_MAX;

    TdfMemberIteratorConst(const Tdf &tdf);

    inline bool isMemberValid() const { return TdfMemberInfoIterator::isValid() && TdfGenericReferenceConst::isValid(); }

    void reset();
    bool next();
    bool find(uint32_t tag);
    bool find(const char8_t* memberName);
    bool moveTo(uint32_t memberIndex);

    inline const Tdf& asClass() const                      { return asTdf(); } //DEPRECATED, use asTdf()

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    bool isSet() const;
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

    bool isDefaultValue() const { return isMemberValid() ? getInfo()->equalsDefaultValue(*this) : false; }

protected:
    void updateCurrentMember();
    const Tdf& mTdf;
};

//Required here because our parent class has a templated constructor, and we want to ensure it does a copy 
template<>
inline TdfGenericReferenceConst::TdfGenericReferenceConst(const TdfMemberIteratorConst& other) : TdfGenericConst(other) {}

} // namespace TDF

} // namespace EA

#endif // EA_TDF_MEMBERITERATOR_H

