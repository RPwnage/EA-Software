/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EA_TDF_TYPEDESCRIPTION_H
#define EA_TDF_TYPEDESCRIPTION_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include <EATDF/internal/config.h>
#include <EAStdC/EAString.h>//for StrCmp in TypeDescription::isIntegralChar
#include <eathread/eathread_mutex.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
namespace TDF
{

typedef uint32_t TdfId;

static const TdfId INVALID_TDF_ID = 0;

class TypeDescriptionEnum;
struct TypeDescriptionObject;
struct TypeDescriptionClass;
struct TypeDescriptionList;
struct TypeDescriptionMap;
struct TypeDescriptionBitfield;
class TdfBitfield;
class TdfObject;
class TdfString;
class TimeValue;
struct ObjectType;
struct ObjectId;
class GenericTdfType;
struct TypeRegistrationNode;


template <class T>
class tdf_ptr;

struct EATDF_API MemberVisitOptions
{
    MemberVisitOptions() : onlyIfSet(false), onlyIfNotDefault(false), subFieldTag(0){}
    bool onlyIfSet : 1; //only consider the value if it has changed
    bool onlyIfNotDefault : 1;  //only consider the value if it is not the default value.
    uint32_t subFieldTag : 32; // only consider the tag value if it is not 0
};

typedef enum  {
    TDF_ACTUAL_TYPE_UNKNOWN,
    TDF_ACTUAL_TYPE_GENERIC_TYPE,   // Generic is a wrapper type that can be used to represent any other TDF type. (Acts like a Union, just without member names)
    TDF_ACTUAL_TYPE_MAP,
    TDF_ACTUAL_TYPE_LIST,
    TDF_ACTUAL_TYPE_FLOAT,
    TDF_ACTUAL_TYPE_ENUM,
    TDF_ACTUAL_TYPE_STRING,
    TDF_ACTUAL_TYPE_VARIABLE,       // Variable types can represent any TdfObject that inherits from VariableTdfBase. (Sub-set of GenericType)
    TDF_ACTUAL_TYPE_BITFIELD,
    TDF_ACTUAL_TYPE_BLOB,
    TDF_ACTUAL_TYPE_UNION,
    TDF_ACTUAL_TYPE_TDF,
    TDF_ACTUAL_TYPE_OBJECT_TYPE,
    TDF_ACTUAL_TYPE_OBJECT_ID,
    TDF_ACTUAL_TYPE_TIMEVALUE,
    TDF_ACTUAL_TYPE_BOOL,
    TDF_ACTUAL_TYPE_INT8,
    TDF_ACTUAL_TYPE_UINT8,
    TDF_ACTUAL_TYPE_INT16,
    TDF_ACTUAL_TYPE_UINT16,
    TDF_ACTUAL_TYPE_INT32,
    TDF_ACTUAL_TYPE_UINT32,
    TDF_ACTUAL_TYPE_INT64,
    TDF_ACTUAL_TYPE_UINT64
} TdfType;

struct EATDF_API TypeDescription
{
    friend class TdfFactory;

    TypeDescription(TdfType _type, TdfId _id, const char8_t* _fullName);

    // Tdf Id - For basic types this will == TdfType. 
    //   For Tdf, Union, Variable, Bitfield, Enum, Map & List this will be FNV1_String8(fullName).
    TdfId getTdfId() const { return id; }
    TdfType getTdfType() const { return type; }
    const char8_t* getFullName() const { return fullName; }
    const char8_t* getShortName() const { return shortName; }
    const TypeRegistrationNode* getRegistrationNode() const { return registrationNode; }

    bool isRegistered() const { return registrationNode != nullptr; }

    bool isIntegral() const;
    bool isIntegralChar() const;
    bool isTdfObject() const;      // TdfObject base class types   - EA::TDF::ObjectType & ObjectId are *not* TdfObjects
    bool isEnum() const { return type == TDF_ACTUAL_TYPE_ENUM; }
    bool isString() const { return type == TDF_ACTUAL_TYPE_STRING; }
    bool isFloat() const { return type == TDF_ACTUAL_TYPE_FLOAT; }
    bool isBitfield() const { return type == TDF_ACTUAL_TYPE_BITFIELD; }

// Classification for Parsing: 
    bool isBasicType() const;       // Non-named, non-templated types (includes variable, generic, blob)
    bool isNamedType() const;       // Any type with a name (Classes, Unions, Enum, and Bitfields)
    bool isTemplatedType() const;   // Maps and Lists 

    const TypeDescriptionEnum* asEnumMap() const;
    const TypeDescriptionObject* asObjectDescription() const;
    const TypeDescriptionClass* asClassInfo() const;
    const TypeDescriptionList* asListDescription() const;
    const TypeDescriptionMap* asMapDescription() const;   
    const TypeDescriptionBitfield* asBitfieldDescription() const;  

// private: 
    TdfType type;
    TdfId id;
    const char8_t* fullName;
    const char8_t* shortName;  // Name without namespaces.
    const TypeRegistrationNode* registrationNode;       

    static const TypeDescription& UNKNOWN_TYPE;
    static const char8_t* CHAR_FULLNAME;
#if EA_TDF_THREAD_SAFE_TYPEDESC_INIT_ENABLED
    // if EA_TDF_THREAD_SAFE_TYPEDESC_INIT_ENABLED, we'll need to use this mutex to ensure that the initialization of the various TypeDescription objects are thread-safe
    static EA::Thread::Mutex mTypeDescInitializationMutex; 
#endif
};

typedef const TypeDescription& (*TypeDescriptionFunc)();

struct EATDF_API TypeDescriptionObject : public TypeDescription
{
    typedef TdfObject* (*CreateFunc)(EA::Allocator::ICoreAllocator&, const char8_t*, uint8_t* placementBuf);

    TypeDescriptionObject(TdfType _type, TdfId _id, const char8_t* _fullName, CreateFunc _createFunc) : 
        TypeDescription(_type, _id, _fullName), createFunc(_createFunc) {}
    CreateFunc createFunc;
};
typedef TypeDescriptionObject::CreateFunc TdfCreator;

struct EATDF_API TypeDescriptionList : public TypeDescriptionObject
{
    TypeDescriptionList(const TypeDescription& _valueType, CreateFunc _createFunc) : 
        TypeDescriptionObject(TDF_ACTUAL_TYPE_LIST, INVALID_TDF_ID, nullptr, _createFunc), valueType(_valueType) {}
    const TypeDescription& valueType;

    EA_TDF_NON_ASSIGNABLE(TypeDescriptionList);
};

struct EATDF_API TypeDescriptionMap : public TypeDescriptionObject
{

    TypeDescriptionMap(const TypeDescription& _keyType, const TypeDescription& _valueType, CreateFunc _createFunc) : 
        TypeDescriptionObject(TDF_ACTUAL_TYPE_MAP, INVALID_TDF_ID, nullptr, _createFunc), keyType(_keyType), valueType(_valueType) {}
    const TypeDescription& keyType;
    const TypeDescription& valueType;

    EA_TDF_NON_ASSIGNABLE(TypeDescriptionMap);
};



} //namespace TDF
} //namespace EA


#endif // EA_TDF_TYPEDESCRIPTION_H



