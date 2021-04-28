/*! *********************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_VISIT_H
#define EA_TDF_VISIT_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/typedescription.h>
#include <EATDF/tdfmemberinfo.h>
#include <EATDF/tdfbasetypes.h>
#include <EATDF/tdfobjectid.h> 
#include <EATDF/time.h> 

/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace EA
{

namespace TDF
{

class Tdf;
class TdfBitfield;
class TdfString;
class TdfBlob;
class TdfUnion;
class VariableTdfBase;
class TdfVectorBase;
class TdfMapBase;
class TdfGenericReference;
class TdfGenericReferenceConst;
class TdfMemberIterator;
class TdfMemberIteratorConst;
struct TdfMemberInfo;
class TypeDescriptionEnum;


class EATDF_API TdfVisitContext 
{
    EA_TDF_NON_ASSIGNABLE(TdfVisitContext);
public:

    TdfVisitContext(const TdfVisitContext& context, const MemberVisitOptions& options) 
        : mParent(context.mParent), mMemberInfo(context.mMemberInfo), mKey(context.mKey), mValue(context.mValue), mFilterOptions(options) {}
    TdfVisitContext(Tdf& tdf, const MemberVisitOptions& options) 
        : mParent(nullptr), mMemberInfo(nullptr), mValue(tdf), mFilterOptions(options) {}
    TdfVisitContext(const TdfVisitContext& parent, const TdfGenericReference& value)
        : mParent(&parent), mMemberInfo(nullptr), mValue(value), mFilterOptions(parent.mFilterOptions) {}
    TdfVisitContext(const TdfVisitContext& parent, const TdfGenericConst& key, const TdfGenericReference& value) 
        : mParent(&parent), mMemberInfo(nullptr), mKey(key), mValue(value), mFilterOptions(parent.mFilterOptions) {}
    TdfVisitContext(const TdfVisitContext& parent,const TdfMemberInfo& memberInfo, const TdfGenericReference& key, const TdfGenericReference& value) 
        : mParent(&parent), mMemberInfo(&memberInfo), mKey(key), mValue(value), mFilterOptions(parent.mFilterOptions) {}
    
    const TdfGenericReference& getKey() const { return mKey; }
    const TdfGenericReference& getValue() const { return mValue; }

    TdfType getType() const { return mValue.getType(); }

    const TdfMemberInfo* getMemberInfo() const { return mMemberInfo; }

    const TdfVisitContext* getParent() const { return mParent; }
    TdfType getParentType() const { return mParent != nullptr ? mParent->getType() : TDF_ACTUAL_TYPE_UNKNOWN; }
    bool isRoot() const { return mParent == nullptr; }

    const MemberVisitOptions& getVisitOptions() const { return mFilterOptions; }

protected:
    const TdfVisitContext* mParent;
    const TdfMemberInfo* mMemberInfo;  //valid for non-array, non-map children
    TdfGenericReference mKey;  //value of the current "key".  Valid for children of maps and vectors.  Vectors have the array index as a key.
    TdfGenericReference mValue; //value of current item
    const MemberVisitOptions& mFilterOptions;
};

class EATDF_API TdfVisitContextConst 
{
    EA_TDF_NON_ASSIGNABLE(TdfVisitContextConst);
public:
    
    TdfVisitContextConst(const TdfVisitContextConst& context, const MemberVisitOptions& options) 
        : mParent(context.mParent), mMemberInfo(context.mMemberInfo), mKey(context.mKey), mValue(context.mValue), mFilterOptions(options) {}
    TdfVisitContextConst(const Tdf& tdf, const MemberVisitOptions& options) 
        : mParent(nullptr), mMemberInfo(nullptr), mValue(tdf), mFilterOptions(options) {}
    TdfVisitContextConst(const TdfVisitContextConst& parent, const TdfGenericReferenceConst& value)
        : mParent(&parent), mMemberInfo(nullptr), mValue(value), mFilterOptions(parent.mFilterOptions) {}
    TdfVisitContextConst(const TdfVisitContextConst& parent, const TdfGenericReferenceConst& key, const TdfGenericReferenceConst& value) 
        : mParent(&parent), mMemberInfo(nullptr), mKey(key), mValue(value), mFilterOptions(parent.mFilterOptions) {}
    TdfVisitContextConst(const TdfVisitContextConst& parent,const TdfMemberInfo& memberInfo, const TdfGenericReferenceConst& key, const TdfGenericReferenceConst& value) 
        : mParent(&parent), mMemberInfo(&memberInfo), mKey(key), mValue(value), mFilterOptions(parent.mFilterOptions) {}

    const TdfGenericReferenceConst& getKey() const { return mKey; }
    const TdfGenericReferenceConst& getValue() const { return mValue; }

    TdfType getType() const { return mValue.getType(); }

    const TdfMemberInfo* getMemberInfo() const { return mMemberInfo; }

    const TdfVisitContextConst* getParent() const { return mParent; }
    TdfType getParentType() const { return mParent != nullptr ? mParent->getType() : TDF_ACTUAL_TYPE_UNKNOWN; }
    bool isRoot() const { return mParent == nullptr; }

    const MemberVisitOptions& getVisitOptions() const { return mFilterOptions; }

protected:
    const TdfVisitContextConst* mParent;
    const TdfMemberInfo* mMemberInfo;  //valid for non-array, non-map children
    TdfGenericReferenceConst mKey;  //value of the current "key".  Valid for children of maps and vectors.  Vectors have the array index as a key.
    TdfGenericReferenceConst mValue; //value of current item
    const MemberVisitOptions& mFilterOptions;
};


class EATDF_API TdfMemberVisitor {
public:
    TdfMemberVisitor() : mStateDepth(0) {}
    virtual ~TdfMemberVisitor() {}

public:
    virtual bool visitContext(const TdfVisitContext& context);

private:
    // *deprecated functionality*, implement visitContext() directly
    virtual bool visitValue(const TdfVisitContext& context) { return false; }  //called for all value types.  For non-scalars this happens before visiting children.
    virtual bool postVisitValue(const TdfVisitContext& context) { return false; }  //called for all non-scalars after visiting children.

private:
    const static uint32_t MAX_STATE_DEPTH = 64;

    void pushStack();
    void popStack();

protected:
    struct State
    {
        State() : preBeginPreamblePosition(0), postBeginPreamblePosition(0) {}
        ptrdiff_t preBeginPreamblePosition;
        ptrdiff_t postBeginPreamblePosition;
    } mStateStack[MAX_STATE_DEPTH];
    uint32_t mStateDepth;
};

class EATDF_API TdfMemberVisitorConst {    
public:
    TdfMemberVisitorConst() : mStateDepth(0) {}
    virtual ~TdfMemberVisitorConst() {}

public:
    virtual bool visitContext(const TdfVisitContextConst& context);

private:
    // *deprecated functionality*, implement visitContext() directly
    virtual bool visitValue(const TdfVisitContextConst& context) { return false; } //called for all value types.  For non-scalars this happens before visiting children.
    virtual bool postVisitValue(const TdfVisitContextConst& context) { return false; } //called for all non-scalars after visiting children.

private:
    const static uint32_t MAX_STATE_DEPTH = 64;

    void pushStack();
    void popStack();

protected:
    struct State
    {
        State() : preBeginPreamblePosition(0), postBeginPreamblePosition(0) {}
        ptrdiff_t preBeginPreamblePosition;
        ptrdiff_t postBeginPreamblePosition;
    } mStateStack[MAX_STATE_DEPTH];
    uint32_t mStateDepth;
};


class EATDF_API TdfVisitor
{
public:
    virtual ~TdfVisitor() {}

    //Starts visiting a TDF
    virtual bool visit(Tdf &tdf, const Tdf &referenceValue) = 0;
    virtual bool visit(TdfUnion &tdf, const TdfUnion &referenceValue) = 0;

    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, TdfVectorBase &value, const TdfVectorBase &referenceValue) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, TdfMapBase &value, const TdfMapBase &referenceValue) = 0;

    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue = false) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue = '\0') = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue = 0) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue = 0) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue = 0) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue = 0) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue = 0) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue = 0) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue = 0) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue = 0) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue = 0.0f) = 0;

    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, TdfBitfield &value, const TdfBitfield &referenceValue) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, TdfString &value, const TdfString &referenceValue, const char8_t *defaultValue = "", const uint32_t maxLength = 0) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, TdfBlob &value, const TdfBlob &referenceValue) = 0;
    // enumeration visitor
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const TypeDescriptionEnum *enumMap, const int32_t defaultValue = 0 ) = 0;

    virtual bool visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, Tdf &value, const Tdf &referenceValue) = 0;
    virtual bool visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, TdfUnion &value, const TdfUnion &referenceValue) = 0;
    virtual bool visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, VariableTdfBase &value, const VariableTdfBase &referenceValue) = 0;
    virtual bool visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, GenericTdfType &value, const GenericTdfType &referenceValue) = 0;

    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, ObjectType &value, const ObjectType &referenceValue, const ObjectType defaultValue = OBJECT_TYPE_INVALID) = 0;
    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, ObjectId &value, const ObjectId &referenceValue, const ObjectId defaultValue = OBJECT_ID_INVALID) = 0;

    virtual void visit(Tdf &rootTdf, Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue &referenceValue, const TimeValue defaultValue = 0) = 0;

    virtual bool preVisitCheck(Tdf& rootTdf, TdfMemberIterator& memberIt, TdfMemberIteratorConst& referenceIt) { return true; }

public:
    void visitReference(Tdf& rootTdf, Tdf& parentTdf, uint32_t tag, const TdfGeneric& val, const TdfGenericConst* ref = nullptr, const TdfMemberInfo::DefaultValue& defaultValue = TdfMemberInfo::DefaultValue(), uint32_t additionalValue = 0);
  };

} //namespace TDF

} //namespace EA


#endif // EA_TDF_H


