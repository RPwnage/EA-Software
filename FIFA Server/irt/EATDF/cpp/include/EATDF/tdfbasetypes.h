/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EA_TDF_TDFBASETYPES_H
#define EA_TDF_TDFBASETYPES_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/tdfobject.h>
#include <EATDF/typedescription.h>
#include <EATDF/tdfstring.h>
#include <EATDF/tdfobjectid.h>
#include <EATDF/time.h>
#include <EATDF/tdfbitfield.h>
#include <EAAssert/eaassert.h>
#include <EATDF/tdffactory.h>

#include <EASTL/type_traits.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
namespace TDF
{
    

class TdfObject;
class Tdf;
class TdfUnion;
class TdfMapBase;
class TdfVectorBase;
class TdfBitfield;
class TdfBlob;
struct ObjectType;
struct ObjectId;
class TdfString;
class VariableTdfBase;
class TimeValue;
class GenericTdfType;
class TdfGeneric;
class TdfGenericValue;
class TdfGenericReference;
class TdfGenericReferenceConst;

class EATDF_API TdfGenericConst
{
public:
    TdfType getType() const { return mTypeDesc->getTdfType(); }
    const TypeDescription& getTypeDescription() const { return *mTypeDesc; }
    TdfId getTdfId() const { return mTypeDesc->getTdfId(); }
    const char8_t* getFullName() const { return mTypeDesc->getFullName(); }
    bool isTypeInt() const { return mTypeDesc->isIntegral(); }
    bool isTypeString() const { return mTypeDesc->isString(); }
    bool isValid() const { return mRefPtr != nullptr; }

    bool equalsValue(const TdfGenericConst& other) const;
#if EA_TDF_INCLUDE_CHANGE_TRACKING
    // Check if the type is a TdfObject, and then calls isSet() on that object type.  (Not valid for primitive types)
    bool isTdfObjectSet() const;
#endif

    const TdfMapBase& asMap() const                 { return *reinterpret_cast<TdfMapBase*>(mRefPtr); }
    const TdfVectorBase& asList() const             { return *reinterpret_cast<TdfVectorBase*>(mRefPtr); }
    const float& asFloat() const                    { return *reinterpret_cast<float*>(mRefPtr); }
    const int32_t& asEnum() const                   { return *reinterpret_cast<int32_t*>(mRefPtr); }
    const TdfString& asString() const               { return *reinterpret_cast<TdfString*>(mRefPtr); }
    const VariableTdfBase& asVariable() const       { return *reinterpret_cast<VariableTdfBase*>(mRefPtr); }
    const GenericTdfType& asGenericType() const     { return *reinterpret_cast<GenericTdfType*>(mRefPtr); }
    const TdfBitfield& asBitfield() const           { return *reinterpret_cast<TdfBitfield*>(mRefPtr); }
    const TdfBlob& asBlob() const                   { return *reinterpret_cast<TdfBlob*>(mRefPtr); }
    const TdfUnion& asUnion() const                 { return *reinterpret_cast<TdfUnion*>(mRefPtr); }
    const Tdf& asTdf() const                        { return *reinterpret_cast<Tdf*>(mRefPtr); }
    const TdfObject& asTdfObject() const            { return *reinterpret_cast<TdfObject*>(mRefPtr); }
    const ObjectType& asObjectType() const          { return *reinterpret_cast<ObjectType*>(mRefPtr); }
    const ObjectId& asObjectId() const              { return *reinterpret_cast<ObjectId*>(mRefPtr); }
    const TimeValue& asTimeValue() const            { return *reinterpret_cast<TimeValue*>(mRefPtr); }
    const bool& asBool() const                      { return *reinterpret_cast<bool*>(mRefPtr); }
    const int8_t& asInt8() const                    { return *reinterpret_cast<int8_t*>(mRefPtr); }
    const uint8_t& asUInt8() const                  { return *reinterpret_cast<uint8_t*>(mRefPtr); }
    const int16_t& asInt16() const                  { return *reinterpret_cast<int16_t*>(mRefPtr); }
    const uint16_t& asUInt16() const                { return *reinterpret_cast<uint16_t*>(mRefPtr); }
    const int32_t& asInt32() const                  { return *reinterpret_cast<int32_t*>(mRefPtr); }
    const uint32_t& asUInt32() const                { return *reinterpret_cast<uint32_t*>(mRefPtr); }
    const int64_t& asInt64() const                  { return *reinterpret_cast<int64_t*>(mRefPtr); }
    const uint64_t& asUInt64() const                { return *reinterpret_cast<uint64_t*>(mRefPtr); }
    const uint8_t* asAny() const                    { return mRefPtr; }

    template<typename T>
    const T& asType() const                         { return *reinterpret_cast<T*>(mRefPtr); }

    // Conversion functions:

    // Converts a given integral generic, into another integral type:
    bool convertToIntegral(TdfGenericReference& result) const;
    bool convertToString(TdfGenericReference& result) const;        // String conversion
    bool convertFromString(TdfGenericReference& result) const;
    bool convertToList(TdfGenericReference& result) const;          // Single element list conversion
    bool convertFromList(TdfGenericReference& result) const;

    bool convertBetweenLists(TdfGenericReference& result) const;    // Converts list to list (no merge ops)
    bool convertBetweenMaps(TdfGenericReference& result) const;     // Converts map to map (no merge ops)

    // General conversion function. Calls the above functions.  (Special cased for Bitfield Members)
    bool convertToResult(TdfGenericReference& result, const TypeDescriptionBitfieldMember* bfMember = nullptr) const;
    static bool isSimpleConvertibleType(const EA::TDF::TypeDescription& type1);
    static bool canTypesConvert(const EA::TDF::TypeDescription& type1, const EA::TDF::TypeDescription& type2);

    bool createIntegralKey(TdfGenericReference& result) const;  // DEPRECATED

protected:   
    TdfGenericConst(const TypeDescription& typeDesc = TypeDescription::UNKNOWN_TYPE, uint8_t* refPtr = nullptr) : mTypeDesc(&typeDesc), mRefPtr(refPtr) { }

    void clear() { mTypeDesc = &TypeDescription::UNKNOWN_TYPE; mRefPtr = nullptr; }

    template <typename T>
    void setRef(const T& val)                      { mTypeDesc = &TypeDescriptionSelector<T>::get(); mRefPtr = (uint8_t*) &val; }    

    template <typename T>
    void setRef(const tdf_ptr<T>& val)             { if (val != nullptr) setRef(*val.get()); }    

    void setRef(const Tdf& val);          
    void setRef(const TdfUnion& val);     
    void setRef(const TdfVectorBase& val);
    void setRef(const TdfMapBase& val);
    void setRef(const TdfObject& val);

    void setRef(const TdfGenericConst& val);
    void setRef(const TdfGeneric& val);
    void setRef(const TdfGenericValue& val);
    void setRef(const TdfGenericReference& val);
    void setRef(const TdfGenericReferenceConst& val);
    void setRef(TdfGenericConst& val);
    void setRef(TdfGeneric& val);
    void setRef(TdfGenericValue& val);
    void setRef(TdfGenericReference& val);
    void setRef(TdfGenericReferenceConst& val);

protected:
    const TypeDescription* mTypeDesc;
    uint8_t* mRefPtr;
};

class EATDF_API TdfGenericReferenceConst : public TdfGenericConst
{
public: 
    //Public setter functions that correctly set the value and type.
    TdfGenericReferenceConst() { clear(); }
    TdfGenericReferenceConst(const TdfGenericConst& val)         { setRef(val); }
    TdfGenericReferenceConst(const TdfGeneric& val)              { setRef(val); }
    TdfGenericReferenceConst(const TdfGenericValue& val)         { setRef(val); }
    TdfGenericReferenceConst(const TdfGenericReference& val)     { setRef(val); }
    TdfGenericReferenceConst(const TdfGenericReferenceConst& val):TdfGenericConst(val) { setRef(val); }
    TdfGenericReferenceConst(TdfGenericConst& val)               { setRef(val); }
    TdfGenericReferenceConst(TdfGeneric& val)                    { setRef(val); }
    TdfGenericReferenceConst(TdfGenericValue& val)               { setRef(val); }
    TdfGenericReferenceConst(TdfGenericReference& val)           { setRef(val); }

    TdfGenericReferenceConst(const Tdf& val);
    TdfGenericReferenceConst(const TdfUnion& val);
    TdfGenericReferenceConst(const TdfVectorBase& val);
    TdfGenericReferenceConst(const TdfMapBase& val);
    TdfGenericReferenceConst(const TdfObject& val);

    template <typename T>
    explicit TdfGenericReferenceConst(const T& val) : TdfGenericConst(TypeDescriptionSelector<T>::get(), const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&val))) {}

    template <typename T>
    explicit TdfGenericReferenceConst(const tdf_ptr<T>& val) { setRef(val); }


    virtual ~TdfGenericReferenceConst() {}
 
    TdfGenericReferenceConst& operator=(const TdfGenericReferenceConst& lhs)
    {
        mTypeDesc = lhs.mTypeDesc;
        mRefPtr = lhs.mRefPtr;
        return *this;
    }

    //make clear & setref public
    using TdfGenericConst::clear;
    using TdfGenericConst::setRef;

};

#if EA_TDF_INCLUDE_CHANGE_TRACKING
#define CALL_MARK_SET() if (mMarkOnAccess) { markSet(); }
#else
#define CALL_MARK_SET()
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING


class EATDF_API TdfGeneric : public TdfGenericConst
{
public:
    TdfMapBase& asMap() const                { return *reinterpret_cast<TdfMapBase*>(mRefPtr); }
    TdfVectorBase& asList() const            { return *reinterpret_cast<TdfVectorBase*>(mRefPtr); }
    float& asFloat() const                   { CALL_MARK_SET(); return *reinterpret_cast<float*>(mRefPtr); }
    int32_t& asEnum() const                  { CALL_MARK_SET(); return *reinterpret_cast<int32_t*>(mRefPtr); }
    TdfString& asString() const              { CALL_MARK_SET(); return *reinterpret_cast<TdfString*>(mRefPtr); }
    VariableTdfBase& asVariable() const      { return *reinterpret_cast<VariableTdfBase*>(mRefPtr); }
    GenericTdfType& asGenericType() const    { return *reinterpret_cast<GenericTdfType*>(mRefPtr); }
    TdfBitfield& asBitfield() const          { CALL_MARK_SET(); return *reinterpret_cast<TdfBitfield*>(mRefPtr); }
    TdfBlob& asBlob() const                  { return *reinterpret_cast<TdfBlob*>(mRefPtr); }
    TdfUnion& asUnion() const                { return *reinterpret_cast<TdfUnion*>(mRefPtr); }
    Tdf& asTdf() const                       { return *reinterpret_cast<Tdf*>(mRefPtr); }
    TdfObject& asTdfObject() const           { return *reinterpret_cast<TdfObject*>(mRefPtr); }
    ObjectType& asObjectType() const         { CALL_MARK_SET(); return *reinterpret_cast<ObjectType*>(mRefPtr); }
    ObjectId& asObjectId() const             { CALL_MARK_SET(); return *reinterpret_cast<ObjectId*>(mRefPtr); }
    TimeValue& asTimeValue() const           { CALL_MARK_SET(); return *reinterpret_cast<TimeValue*>(mRefPtr); }
    bool& asBool() const                     { CALL_MARK_SET(); return *reinterpret_cast<bool*>(mRefPtr); }
    int8_t& asInt8() const                   { CALL_MARK_SET(); return *reinterpret_cast<int8_t*>(mRefPtr); }
    uint8_t& asUInt8() const                 { CALL_MARK_SET(); return *reinterpret_cast<uint8_t*>(mRefPtr); }
    int16_t& asInt16() const                 { CALL_MARK_SET(); return *reinterpret_cast<int16_t*>(mRefPtr); }
    uint16_t& asUInt16() const               { CALL_MARK_SET(); return *reinterpret_cast<uint16_t*>(mRefPtr); }
    int32_t& asInt32() const                 { CALL_MARK_SET(); return *reinterpret_cast<int32_t*>(mRefPtr); }
    uint32_t& asUInt32() const               { CALL_MARK_SET(); return *reinterpret_cast<uint32_t*>(mRefPtr); }
    int64_t& asInt64() const                 { CALL_MARK_SET(); return *reinterpret_cast<int64_t*>(mRefPtr); }
    uint64_t& asUInt64() const               { CALL_MARK_SET(); return *reinterpret_cast<uint64_t*>(mRefPtr); }
    uint8_t* asAny() const                   { return mRefPtr; }

    template<typename T>
    T& asType() const                  { if (!getTypeDescription().isTdfObject()) { CALL_MARK_SET(); } return *reinterpret_cast<T*>(mRefPtr); }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    void setMarkOnAccess(bool val) { mMarkOnAccess = val; }
    bool getMarkOnAccess() const { return mMarkOnAccess; }
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

protected:
    TdfGeneric() 
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        : mMarkOnAccess(true) 
#endif
    {}

    virtual ~TdfGeneric() {}

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    virtual void markSet() const {}
    bool mMarkOnAccess;
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

};

#undef CALL_MARK_SET


class EATDF_API TdfGenericReference : public TdfGeneric
{
public:
    TdfGenericReference() {}

    template <typename T>
    explicit TdfGenericReference(T& val) 
    { 
        setRef(val); 
    }

    bool setValue(const TdfGenericConst& value, const MemberVisitOptions& options = MemberVisitOptions());

    //make clear & setref public
    using TdfGenericConst::clear;
    using TdfGenericConst::setRef;
};


class EATDF_API TdfGenericValue : public TdfGeneric
{
public:
    //Public setter functions that correctly set the value and type.
    TdfGenericValue(EA::Allocator::ICoreAllocator& alloc EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, uint8_t* placementBuf = nullptr) : mAllocator(alloc) { Value.mUInt64 = 0; }
    TdfGenericValue(const TdfGenericValue& other) : mAllocator(other.mAllocator) { Value.mUInt64 = 0; set(other); }

    template <typename T>
    explicit TdfGenericValue(const T& ref, EA::Allocator::ICoreAllocator& alloc EA_TDF_DEFAULT_ALLOCATOR_ASSIGN) : mAllocator(alloc) { Value.mUInt64 = 0; set(ref); }

    ~TdfGenericValue() override { clear(); }

    virtual void setType(const TypeDescription& newType) { setTypeInternal(newType, nullptr); }

    void clear() { setType(TypeDescription::UNKNOWN_TYPE); }

    bool set(const TdfGenericConst& ref);
    void set(bool val) { setType(TypeDescriptionSelector<bool>::get()); Value.mBool = val; } 
    void set(char8_t val) { setType(TypeDescriptionSelector<uint8_t>::get()); Value.mUInt8 = (uint8_t)val; }
    void set(uint8_t val) { setType(TypeDescriptionSelector<uint8_t>::get()); Value.mUInt8 = val; }
    void set(uint16_t val) { setType(TypeDescriptionSelector<uint16_t>::get()); Value.mUInt16 = val; }
    void set(uint32_t val) { setType(TypeDescriptionSelector<uint32_t>::get()); Value.mUInt32 = val; }
    void set(uint64_t val, const TypeDescription& typeDesc = TypeDescriptionSelector<uint64_t>::get()) { setType(typeDesc); Value.mUInt64 = val; }
    void set(int8_t val) { setType(TypeDescriptionSelector<int8_t>::get()); Value.mInt8 = val; }
    void set(int16_t val) { setType(TypeDescriptionSelector<int16_t>::get()); Value.mInt16 = val; }
    void set(int32_t val) { setType(TypeDescriptionSelector<int32_t>::get()); Value.mInt32 = val; }
    void set(int64_t val) { setType(TypeDescriptionSelector<int64_t>::get()); Value.mInt64 = val; }
    void set(float val) { setType(TypeDescriptionSelector<float>::get()); Value.mFloat = val; }

    template <typename T>
    void setEnum(T enumVal) { setType(TypeDescriptionSelector<T>::get()); Value.mInt32 = *(int32_t*)(void*)(&enumVal); }    // Void* used to avoid casting complaints
    void set(int32_t val, const TypeDescriptionEnum& enumMap);



    void set(const TdfString& val);
    void set(const char8_t* val);
    void set(const ObjectType& val);
    void set(const ObjectId& val);
    void set(const TimeValue& val);
    
    void set(const TdfBitfield& val);
    void set(const Tdf& val);
    void set(const TdfUnion& val);
    void set(const TdfBlob& val);
    void set(const VariableTdfBase& val);
    void set(const GenericTdfType& val);
    void set(const TdfVectorBase& val);
    void set(const TdfMapBase& val);
    void set(TdfObject& val);       

    bool operator==(const TdfGenericValue& other) const { return equalsValue(other); }
    TdfGenericValue& operator=(const TdfGenericValue& other) { set(other); return *this; }
    TdfGenericValue& operator=(const TdfGenericConst& other) { set(other); return *this; }

    EA::Allocator::ICoreAllocator& getAllocator() const { return mAllocator; }
protected:
    void setTypeInternal(const TypeDescription& newType, uint8_t* placementBuf);

    EA::Allocator::ICoreAllocator& mAllocator;
    union 
    {
        bool mBool;
        uint8_t mUInt8;
        int8_t mInt8;
        uint16_t mUInt16;
        int16_t mInt16;
        uint32_t mUInt32;
        int32_t mInt32;
        uint64_t mUInt64;
        int64_t mInt64;
        float mFloat;        
        uint32_t mObjectId[(sizeof(ObjectId) / sizeof(uint32_t))];         // mObjectId already contains 4 bytes of padding. (uint64,uint16,uint16,padding)
        uint32_t mObjectType[(sizeof(ObjectType) / sizeof(uint32_t)) + 1];
        uint32_t mTimeValue[(sizeof(TimeValue) / sizeof(uint32_t)) + 1];        
        uint32_t mString[(sizeof(TdfString) / sizeof(uint32_t))];        
        uint32_t mObjPtr[(sizeof(TdfObjectPtr) / sizeof(uint32_t)) + 1];
        TdfBitfield* mBitfield;
        GenericTdfType* mGenericType;
        TdfObject* mTdfObj;
    } Value;
};

// Specialized version of the TdfGenericValue that adds a placement buffer for the TdfUnion type.
// (Removing this pointer saves 8-16 bytes from the TdfGenericValue)
class EATDF_API TdfGenericValuePlacement : public TdfGenericValue
{
public:
    //Public setter functions that correctly set the value and type.
    TdfGenericValuePlacement(EA::Allocator::ICoreAllocator& alloc EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, uint8_t* placementBuf = nullptr) : TdfGenericValue(alloc), mPlacementBuf(placementBuf) { Value.mUInt64 = 0; }
    ~TdfGenericValuePlacement() override { clear(); }

    void setType(const TypeDescription& newType) override { setTypeInternal(newType, mPlacementBuf); }

protected:
    uint8_t* mPlacementBuf;
};

typedef TdfGenericValue TdfBaseTypeValue; //Deprecated, do not use.


// This is a TDF type that can represent any other TDF type:
class EATDF_API GenericTdfType : public TdfObject
#if EA_TDF_INCLUDE_CHANGE_TRACKING
    ,public TdfChangeTracker
#endif
{
public:
    GenericTdfType(EA::Allocator::ICoreAllocator& alloc EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* debugMemName = EA_TDF_DEFAULT_ALLOC_NAME)
        :   TdfObject(),
            mValue(alloc)
    { 
        mValue.clear();
    }
    ~GenericTdfType() override 
    {
        clear();
    }

    const TypeDescription& getTypeDescription() const override;

    void copyInto(GenericTdfType &copy, const MemberVisitOptions& options = MemberVisitOptions()) const
    {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        if (isSet() || !options.onlyIfSet)
#endif
            copy.setValue(mValue);
    }
    void copyIntoObject(TdfObject &copy, const MemberVisitOptions& options = MemberVisitOptions()) const override 
    { 
        if (copy.getTdfType() != TDF_ACTUAL_TYPE_GENERIC_TYPE)
        {
            EA_FAIL_MSG("Invalid type used in GenericTdfType::copyIntoObject. Only GenericTdfType is supported, use get/set() otherwise.");
        }

        copyInto(static_cast<GenericTdfType&>(copy), options);
    }


    // Getting a reference to the value as a non-const marks the value:
    TdfGenericValue& get() { return getValue(); }
    TdfGenericValue& getValue() 
    {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
#endif
        return mValue; 
    }

    const TdfGenericValue& get() const { return getValue(); }
    const TdfGenericValue& getValue() const { return mValue; }


private:
    struct HelperSetEnum
    {
        template <typename T>
        static void set(TdfGenericValue& mValue, const T& val) 
        { 
            // static_assert(eastl::is_enum<T>::value, "This function should only be compiled for enum values.");
            mValue.setEnum(val); 
        }
    };
    struct HelperSetNonEnum
    {
        template <typename T>
        static void set(TdfGenericValue& mValue, const T& val) { mValue.set(val); }
    };
public:

    // Templated type to handle all the basic assignments cases. 
    template <typename T>
    void set(const T& val) 
    { 
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
#endif
        // Here we conditionally use one of two helper classes (they have to be classes because conditional doesn't work with functions),
        // and then call its static set() function to either set by enum or by type.
        typedef typename eastl::conditional<eastl::is_enum<T>::value, 
                                            HelperSetEnum, 
                                            HelperSetNonEnum >::type setHelper;
        setHelper::set(mValue, val);
    }
    void set(const TdfGenericValue& val) { setValue(val); }
    void set(const TdfGenericConst& val) { setValue(val); }
    void setValue(const TdfGenericConst& val)
    { 
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
#endif
        mValue = val; 
    }
    void setValue(const TdfGenericValue& val) 
    { 
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
#endif
        mValue = val; 
    }


    void clear() { clearType(); }
    void clearType() { setType(TypeDescription::UNKNOWN_TYPE); }

    // Sets the type without setting any values:
    bool create(TdfId tdfId) 
    { 
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
#endif
        return EA::TDF::TdfFactory::get().createGenericValue(tdfId, mValue); 
    }
    bool create(const char8_t* fullClassName) 
    { 
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
#endif
        return EA::TDF::TdfFactory::get().createGenericValue(fullClassName, mValue); 
    }
    
    template <typename T>
    T& setType() 
    { 
        setType(TypeDescriptionSelector<T>::get());
        return get().asType<T>();
    }
    void setType(const TypeDescription& newType) 
    { 
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
#endif
        mValue.setType(newType); 
    }


    // Only valid with other types:
    bool isValid() const { return (mValue.getType() != TDF_ACTUAL_TYPE_UNKNOWN); }
    bool equalsValue(const GenericTdfType& genericType) const { return equalsValue(genericType.mValue); }
    bool equalsValue(const TdfGenericValue& genericValue) const { return mValue == genericValue; }

    // Outputs the internal value into the result.  Applies conversion to convert to the result's type. 
    template <typename T>
    bool convertToResult(T& result, const TypeDescriptionBitfieldMember* bfMember = nullptr) const
    {
        TdfGenericReference ref(result);
        return getValue().convertToResult(ref, bfMember);
    }


#if EA_TDF_REGISTER_ALL
    static TypeRegistrationNode REGISTRATION_NODE;
#endif

protected:
    TdfGenericValue mValue;
};


template <>
struct TypeDescriptionSelector<GenericTdfType >
{
    static const TypeDescriptionObject& get() { 
        static TypeDescriptionObject result(TDF_ACTUAL_TYPE_GENERIC_TYPE, TDF_ACTUAL_TYPE_GENERIC_TYPE, "generic", (EA::TDF::TypeDescriptionObject::CreateFunc)  &TdfObject::createInstance<GenericTdfType>); 
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
            inline void DeleteObject(Allocator::ICoreAllocator*, ::EA::TDF::GenericTdfType* object) { delete object; }
        }
    }
}

#endif // EA_TDF_TDFBASETYPES_H



