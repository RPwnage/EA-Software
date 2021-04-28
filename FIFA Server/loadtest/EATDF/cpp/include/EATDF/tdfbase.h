/*! *********************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_BASE_H
#define EA_TDF_BASE_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/tdfobject.h>
#include <EATDF/tdfstring.h>
#include <EATDF/tdfmemberinfo.h>
#include <EATDF/tdfvisit.h>
#include <EATDF/tdfhashvisitor.h>
#include <EATDF/tdffactory.h>
#include <EASTL/bitset.h> // Needed for EA_TDF_INCLUDE_CHANGE_TRACKING

/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace EA
{

namespace TDF
{

const static size_t MAX_TDF_NAME_LEN = 64;
const static size_t MAX_SCOPED_TDF_NAME_LEN = 128;

class VariableTdfBase;
class TdfVectorBase;
class TdfMapBase;
class Tdf;
class TdfMemberInfoIterator;
struct TypeRegistrationNode;
struct TypeDescriptionClass;
class PrintHelper;

/*! ***********************************************************************/
/*! \class TdfClassInfo
    \brief Class that holds meta-data about a Tdf.
***************************************************************************/
struct EATDF_API TypeDescriptionClass : public TypeDescriptionObject
{
    TypeDescriptionClass(TdfType _type, TdfId _id, const char8_t* _name, const char8_t* _fullName, TdfCreator _creator, 
        const TdfMemberInfo* _memberInfo, const uint16_t _memberCount, const TypeRegistrationNode* _registrationNode) : 
        TypeDescriptionObject(_type, _id, _fullName, _creator), name(_name), memberInfo(_memberInfo), memberCount(_memberCount) {}
        
    const char8_t* name;
    const TdfMemberInfo* memberInfo;
    const uint16_t memberCount;

    Tdf* createInstance(EA::Allocator::ICoreAllocator& alloc, const char8_t* allocName, uint8_t* placementBuf = nullptr) const; 

    bool getMemberInfoByIndex(uint32_t index, const TdfMemberInfo*& mi) const;
    bool getMemberInfoByName(const char8_t* memberName, const TdfMemberInfo*& mi, uint32_t* memberIndex = nullptr) const;
    bool getMemberInfoByTag(uint32_t tag, const TdfMemberInfo*& mi, uint32_t* memberIndex = nullptr) const;

private:
    EA_TDF_NON_COPYABLE(TypeDescriptionClass);
};
typedef TypeDescriptionClass TdfClassInfo; //Deprecated, do not use.


/*! ***************************************************************/
/*! \class Tdf
    \brief Base class for all class type definitions.
*******************************************************************/
class EATDF_API Tdf : public TdfObject
{
    EA_TDF_NON_ASSIGNABLE(Tdf);
    friend class TdfMemberInfoIterator;
    friend class TdfMemberIterator;
    friend class TdfMemberIteratorConst;
public:

    typedef TypeDescriptionObject::CreateFunc CreateFunc;

    ~Tdf() override {}

    virtual void visit(TdfVisitor& visitor, Tdf &rootTdf, const Tdf &referenceTdf);
    virtual bool visit(TdfMemberVisitor& visitor, const MemberVisitOptions& options EA_TDF_EXT_ONLY_DEF_ARG(MemberVisitOptions()));
    virtual bool visit(TdfMemberVisitorConst& visitor, const MemberVisitOptions& options EA_TDF_EXT_ONLY_DEF_ARG(MemberVisitOptions())) const;

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    // See implementation in: TdfWithChangeTracking
    virtual void markMemberSet(uint32_t memberIndex, bool isSet = true) {}
    virtual bool isMemberSet(uint32_t memberIndex) const { return true; }
    
    virtual bool isSet() const { return true; }
    virtual void markSet() {}
    virtual void clearIsSet() {}
    bool isSetRecursive() const;
    void markSetRecursive();
    void clearIsSetRecursive();
#endif

    virtual Tdf* clone(EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* allocName = "Tdf::Clone") const
    {
        Tdf* newObj = getClassInfo().createInstance(allocator, allocName);
        copyInto(*newObj);
        return newObj;
    }

    /*! ***************************************************************/
    /*! \brief Prints the TDF's contents to a buffer using the default PrintEncoder

        \param buf The buffer to print to.
        \param bufsize Size of buf in bytes.
        \param indent Amount to indent each field in the printing process.
        \returns The char buffer representing the printed TDF.
    *******************************************************************/
    const char8_t* print(char8_t* buf, uint32_t bufsize, int32_t indent = 0, bool terse = false) const;
    void print(PrintHelper& printHelper, int32_t indent = 0, bool terse = false) const;

    //TDF Info functions - default to nullptr/empty
    const TypeDescriptionClass& getTypeDescription() const override { return getClassInfo(); }
    virtual const TypeDescriptionClass& getClassInfo() const = 0;
    const char8_t *getClassName() const { return getClassInfo().name; }
    const char8_t *getFullClassName() const { return getClassInfo().getFullName(); }

    bool getMemberIndexByInfo(const TdfMemberInfo* memberInfo, uint32_t& index) const;
    bool getMemberInfoByIndex(uint32_t index, const TdfMemberInfo*& memberInfo) const;
    bool getMemberInfoByName(const char8_t* memberName, const TdfMemberInfo*& memberInfo, uint32_t* memberIndex = nullptr) const;
    bool getMemberInfoByTag(uint32_t tag, const TdfMemberInfo*& memberInfo, uint32_t* memberIndex = nullptr) const;
    
    bool getMemberNameByIndex(uint32_t index, const char8_t*& memberFieldName) const;
    bool getMemberNameByTag(uint32_t tag, const char8_t*& memberFieldName) const;

    bool getTagByMemberName(const char8_t* memberFieldName, uint32_t& tag) const;


    /*! ***************************************************************/
    /*! \brief Returns the value of the member with the given tag.

     Note that for values of type VariableTDF, the "Class" member of value will be filled out, but
     it must be checked for nullptr as it may or may not exist.

    \param[in] tag  The tag of the value to look for.
    \param[out] value The value found.
    \return  True if the value is found, false otherwise.
    ********************************************************************/
    virtual bool getValueByTag(uint32_t tag, TdfGenericReference &value, const TdfMemberInfo** memberInfo = nullptr, bool* memberSet = nullptr);
    virtual bool getValueByTag(uint32_t tag, TdfGenericReferenceConst &value, const TdfMemberInfo** memberInfo = nullptr, bool* memberSet = nullptr) const;
    bool getValueByTags(const uint32_t tags[], size_t tagSize, TdfGenericReferenceConst &value, const TdfMemberInfo** memberInfo = nullptr, bool* memberSet = nullptr) const;
    bool getValueByName(const char8_t *memberName, TdfGenericReferenceConst &value) const;
    bool getValueByName(const char8_t* memberName, TdfGenericReference& value);

    bool setValueByTag(uint32_t tag, const TdfGenericConst &value);
    bool setValueByName(const char8_t *memberName, const TdfGenericConst &value);

    bool getIteratorByTag(uint32_t tag, TdfMemberIterator &value);
    bool getIteratorByName(const char8_t *memberName, TdfMemberIterator &value);

    //TDF Id and Registration
    TdfId getTdfId() const { return getClassInfo().getTdfId(); }
    bool isRegisteredTdf() const { return getClassInfo().isRegistered(); }

    template <class TdfType>
    static Tdf* createInstance(EA::Allocator::ICoreAllocator& allocator, const char8_t* debugMemName, uint8_t* placementBuf = nullptr) { return (Tdf*)TdfObject::createInstance<TdfType>(allocator, debugMemName, placementBuf); }

    void copyIntoObject(TdfObject& newObj, const MemberVisitOptions& options) const override;
    void copyInto(Tdf& newObj, const MemberVisitOptions& options = MemberVisitOptions()) const { copyIntoObject(newObj, options); }

    virtual bool equalsValue(const Tdf& rhs) const;

    bool computeHash(TdfHashValue& hash) const;

    bool visitMembers(EA::TDF::TdfMemberVisitor& visitor, const TdfVisitContext& callingContext);    
    bool visitMembers(EA::TDF::TdfMemberVisitorConst& visitor, const TdfVisitContextConst& callingContext) const;

protected:

    Tdf() : TdfObject() {}

    virtual bool setValueByIterator(const TdfMemberInfoIterator& itr, const TdfGenericConst& value);
    virtual bool fixupIterator(const TdfMemberInfoIterator& itr, TdfMemberIterator &value);
    virtual bool isValidIterator(const TdfMemberInfoIterator& itr) const { return true; }

    const TdfMemberInfo* getTdfMemberInfo() const { return getClassInfo().memberInfo; }

#if EA_TDF_INCLUDE_CHANGE_TRACKING    
    virtual bool isSetInternal() const { return false; }
    virtual void markSetInternal() {}
    virtual void clearIsSetInternal() {}
#endif

};

typedef tdf_ptr<Tdf> TdfPtr;

#if EA_TDF_INCLUDE_CHANGE_TRACKING
#define EA_TDF_TDF_MARK_SET(_id) markMemberSet(_id, true);
#else
#define EA_TDF_TDF_MARK_SET(_id)
#endif

//This needs to be after Tdf is defined, so put it down here.
inline Tdf* TypeDescriptionClass::createInstance(EA::Allocator::ICoreAllocator& alloc, const char8_t* allocName, uint8_t* placementBuf) const { return static_cast<Tdf*>((*createFunc)(alloc, allocName, placementBuf)); } 

} //namespace TDF

} //namespace EA

namespace EA
{
    namespace Allocator
    {
        namespace detail
        {
            template <>
            inline void DeleteObject(Allocator::ICoreAllocator*, ::EA::TDF::Tdf* object) { delete object; }
        }
    }
}


#endif

