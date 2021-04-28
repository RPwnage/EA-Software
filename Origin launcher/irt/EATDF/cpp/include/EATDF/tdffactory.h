/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EA_TDF_TDFFACTORY_H
#define EA_TDF_TDFFACTORY_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include <EATDF/typedescription.h>
#include <EATDF/tdfobjectid.h>
#include <EATDF/tdfbitfield.h>

#include <EASTL/fixed_substring.h>
#include <EASTL/vector.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/map.h>
#include <EASTL/intrusive_list.h>
#include <EASTL/intrusive_hash_map.h>
#include <EASTL/intrusive_hash_set.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAHashString.h>

#if EA_TDF_THREAD_SAFE_TDF_REGISTRATION_ENABLED
#include <eathread/eathread_mutex.h>
#endif


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
namespace TDF
{

/************************************************************************/
/*  How type registration works: 

  Basic types (int, float, blob, etc) - 
    TYPE_DESC_CONSTRUCT define the TypeDescriptionSelectors that holds the static TypeDescriptions.
    In the CPP file, we use TdfRegistrationHelpers to hold and register TypeRegistrationNode.
    The static TypeDescription held by TypeDescriptionSelector is not thread-safe but we work around this
    by using dynamic initialization of the TdfRegistrationHelpers to invoke TypeDescriptionSelector<T>::get()
    before main().

  Named Tdf Types (class, union, enum, bitfield) - 
    Tdf generation makes a TypeDescriptionSelector that holds the static TypeDescription.
    Tdf generation also makes a static TypeRegistrationNode member variable to register the class.
    If EA_TDF_REGISTER_ALL=1 then TypeDescriptionSelector<T>::get() is invoked via static initialization before main().
    If EA_TDF_REGISTER_ALL=0 then we're using EA::Thread::Mutex to ensure that there's no race condition when
    TypeDescription and TypeRegistrationNode static variables that are initialized upon first execution of TypeDescriptionSelector<T>::get()

  Templated Types (list, map) -
    tdfmap.h/tdfvector.h define the TypeDescriptionSelectors that holds the TypeDescription and TypeRegistrationNode.
    Whey TypeDescriptionSelector<T>::get() is invoked the TypeDescription is initialized and the class is registered.
    If EA_TDF_REGISTER_ALL=1 then TypeDescriptionSelector<T>::get() is invoked via static initialization before main().
    If EA_TDF_REGISTER_ALL=0 then we're using EA::Thread::Mutex to ensure that there's no race condition when
    TypeDescription and TypeRegistrationNode static variables that are initialized upon first execution of TypeDescriptionSelector<T>::get()

*************************************************************************/
#ifndef TDF_FACTORY_BUCKET_COUNT
#define TDF_FACTORY_BUCKET_COUNT 4999 // prime
#endif

struct EATDF_API TemplatedTypeFixupListNode : public eastl::intrusive_list_node {};
typedef eastl::intrusive_list<TemplatedTypeFixupListNode> TemplatedTypeFixupList;
struct EATDF_API ShortNameFixupListNode : public eastl::intrusive_list_node {};
typedef eastl::intrusive_list<ShortNameFixupListNode> ShortNameFixupList;

struct EATDF_API TdfIdToTypeRegistrationMapNode : public eastl::intrusive_hash_node {};
typedef eastl::intrusive_hash_map<TdfId, TdfIdToTypeRegistrationMapNode, TDF_FACTORY_BUCKET_COUNT> TypeRegistrationMap;

struct CaseInsensitiveStringEqualTo
{
    bool operator()(const char8_t* a, const char8_t* b) const { return (EA::StdC::Stricmp(a,b) == 0); }
};
struct CaseInsensitiveStringHash
{
    size_t operator()(const char8_t* p) const { return EA::StdC::FNV1_String8(p); }
};
struct EATDF_API FullTypeNameToTdfIdMapNode : public eastl::intrusive_hash_node {};
typedef eastl::intrusive_hash_map<const char8_t*, FullTypeNameToTdfIdMapNode, TDF_FACTORY_BUCKET_COUNT, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> FullTypeNameMap;
struct EATDF_API ShortTypeNameToTdfIdMapNode : public eastl::intrusive_hash_node {};
typedef eastl::intrusive_hash_map<const char8_t*, ShortTypeNameToTdfIdMapNode, TDF_FACTORY_BUCKET_COUNT, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> ShortTypeNameMap;

struct EATDF_API ListValueTdfIdToTypeRegistrationMapNode : public eastl::intrusive_hash_node {};
typedef eastl::intrusive_hash_map<TdfId, ListValueTdfIdToTypeRegistrationMapNode, TDF_FACTORY_BUCKET_COUNT> ListValueTdfIdMap;


class EATDF_API TdfClassValueMapHashNode : public eastl::intrusive_hash_node 
{
public:
    TdfClassValueMapHashNode(const EA_TDF_ALLOCATOR_TYPE& allocator) :
        mClass("", allocator),
        mValue("", allocator),
        mTdfValue("", allocator)
    {}
    
    EATDFEastlString mClass;
    EATDFEastlString mValue;
    EATDFEastlString mTdfValue;
};

struct TdfNameValueMapHashNodeEqualTo
{
    bool operator()(const TdfClassValueMapHashNode& a, const TdfClassValueMapHashNode& b) const 
    { 
        return (a.mClass.comparei(b.mClass) == 0) && (a.mValue.comparei(b.mValue) == 0); 
    }
};
struct TdfNameValueMapHashNodeHash
{
    size_t operator()(const TdfClassValueMapHashNode& p) const 
    { 
        size_t value = EA::StdC::FNV1_String8(p.mClass.c_str());
        return EA::StdC::FNV1_String8(p.mValue.c_str(), (uint32_t)value);
    }
};
typedef eastl::intrusive_hash_set<TdfClassValueMapHashNode, TDF_FACTORY_BUCKET_COUNT, TdfNameValueMapHashNodeHash, TdfNameValueMapHashNodeEqualTo> TdfClassValueMapHashMap;

class Tdf;
class TdfGenericValue;
class TdfGenericReference;
struct TypeDescriptionBitfieldMember;

/*! ***********************************************************************/
/*! \class TdfFactory
    \brief Factory that keeps registry of TdfId to TypeDescription mapping.
***************************************************************************/
class EATDF_API TdfFactory
{
public:
    TdfFactory();
    ~TdfFactory() {}

    // TDFs register with the TdfFactory by means of static initialization of the TypeRegistrationNode
    // member of each class that supports registration.  The TdfFactory instance is local static so that it gets initialized the
    // first time it gets used so as to avoid issues with static initialization order.  We do
    // not anticipate multi-threading issues here because static initialization will be run before
    // the main() and therefore before threads are created.
    static TdfFactory& get() { static TdfFactory instance; return instance; }
    
    bool registerType(TypeRegistrationNode& tdfRegistration);
    void deregisterType(TdfId tdfId);
    
    // Getting a Type description takes the qualified name.
    //   If the type name is unique across all TDF namespaces, then the namespace (Blaze::FooManager::) is not required. 
    // You can add members with a '.' separator, and can use [] to access string-mapped values.
    // Examples: 
    // "int32_t",  "float",  "TimeValue"                     - base types
    // "Blaze::GameManager::StartMatchmakingRequest"         - Tdf Type
    // "Blaze::AssociationList::UpdateListsRequest.mBlazeId" - Member Type (uint64_t)
    const TypeDescription* getTypeDesc(TdfId tdfId) const;
    const TypeDescription* getTypeDesc(const char8_t* qualifiedTypeNameWithMember, const TypeDescriptionBitfieldMember** outBfMember = nullptr, int32_t* outEnumValue = nullptr) const;
    const TypeDescription* getTypeDesc(const char8_t* qualifiedTypeName, const char8_t* memberName, const TypeDescriptionBitfieldMember** outBfMember = nullptr, int32_t* outEnumValue = nullptr) const;

    // List specific version of the TypeDesc lookup.  Makes it easier to construct TdfLists from Tdf types (still requires the list to be defined in a TDF file).  
    const TypeDescription* getListTypeDescFromValue(TdfId valueTdfId) const;

    // As with getTypeDesc, here you can provide a '.' separated string, and get a TdfGenericReference back.
    // String-mapped values are automatically added, if they did not already exist and addMissingEntry is set to true.
    bool getTdfMemberReference(const TdfGenericReference& baseRef, const char8_t* qualifiedTypeName, TdfGenericReference& outGenericRef, const TypeDescriptionBitfieldMember** outMember = nullptr, bool nameIncludesTdfName = true, bool addMissingEntry = true) const;


    // This helper allows you to map complex strings, to simpler versions:
    // If a '.' is used when looking up the class name, then it will lookup both the full name, and the name before the period.
    // Example: ("rankedGameRule", "desiredValue") -> ("Blaze::GameManager::MatchmakingCriteriaData.mRankedGameRulePrefs.mDesiredRankedGameValue")
    bool registerTypeName(const char8_t* className, const char8_t* valueName, const char8_t* mappedValue);
    bool lookupTypeName(const char8_t* className, const char8_t* valueName, const char8_t*& outMappedValue, bool findExactMatch = false) const;


    bool createGenericValue(TdfId tdfId, TdfGenericValue& outValue) const;
    bool createGenericValue(const char8_t* fullClassName, TdfGenericValue& outValue) const;

    // getTdfIdFromName looks up via shortName, then fullName:
    TdfId getTdfIdFromName(const char8_t* name) const;
    TdfId getTdfIdFromFullClassName(const char8_t* fullClassName) const;
    TdfId getTdfIdFromShortClassName(const char8_t* shortClassName) const;

// Tdf-class specific helper functions:
    TdfCreator getCreator(TdfId tdfId) const;

    Tdf* create(TdfId tdfId, EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* allocName = EA_TDF_DEFAULT_ALLOC_NAME) const
    {
        TdfCreator creator = getCreator(tdfId);
        return (creator != nullptr) ? (Tdf*) (*creator)(allocator, allocName, nullptr) : nullptr;
    }
    Tdf* create(const char8_t* name, EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* allocName = EA_TDF_DEFAULT_ALLOC_NAME) const
    {
        return create(getTdfIdFromName(name), allocator, allocName);
    }

    // Tdf-bitfield specific helper functions:
    BitfieldCreator getBitfieldCreator(TdfId tdfId) const;

    TdfBitfield* createBitField(TdfId tdfId, EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* allocName = EA_TDF_DEFAULT_ALLOC_NAME) const
    {
        BitfieldCreator creator = getBitfieldCreator(tdfId);
        return (creator != nullptr) ? (TdfBitfield*)(*creator)(allocator, allocName, nullptr) : nullptr;
    }
    TdfBitfield* createBitField(const char8_t* name, EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* allocName = EA_TDF_DEFAULT_ALLOC_NAME) const
    {
        return createBitField(getTdfIdFromName(name), allocator, allocName);
    }

    // Fix up step that occurs at start up (makes sure every list/map has a valid name & hashed id)
    // This also assigns the allocator used by the TDF factory (individual create() calls use the provided allocator)
    static void fixupTypes(EA::Allocator::ICoreAllocator& alloc EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* allocName = EA_TDF_DEFAULT_ALLOC_NAME);
    static void cleanupTypeAllocations();

    // Destructive string parsing helper:
    typedef eastl::fixed_substring<char8_t> EATDFEastlSubString;
    typedef eastl::fixed_vector<EATDFEastlSubString, 8, true, EA_TDF_ALLOCATOR_TYPE> SubStringList;
    bool parseString(EATDFEastlString& input, SubStringList& outList) const;

    // Direct Registry access is useful if your system need to register a handler for all types, or you just need to iterate over everything.
    // Do not use this if you just need to lookup a single type description (use the other helper functions).
    const TypeRegistrationMap& getRegistry() const {  return mRegistry;  }

private:
    void fixupAllTemplatedTypes();
    void fixupDuplicateShortNames();
    void cleanupTemplatedTypeAllocations();
    void fixupTemplatedType(const TypeDescription& curType);

    void setAllocator(EA::Allocator::ICoreAllocator& alloc, const char8_t* allocName);

    // getTypeDescInternal Attempts to lookup the name in the shortName map, then the fullName map. 
    const TypeDescription* getTypeDescInternal(const char8_t* name) const;
    const TypeDescription* getMemberTypeDescription(const TypeDescription* baseTypeDesc, SubStringList::const_iterator curMember, SubStringList::const_iterator endMember, const TypeDescriptionBitfieldMember** outBfMember, int32_t* outEnumValue = nullptr) const;
    bool getMemberReference(const TdfGenericReference& baseRef, SubStringList::const_iterator curMember, SubStringList::const_iterator endMember, TdfGenericReference& outGenericRef, const TypeDescriptionBitfieldMember** outBfMember, bool addMissingEntry = true) const;

    bool mFixupCompleted;
    TypeRegistrationMap mRegistry;
    ListValueTdfIdMap mListValueRegistry;// For list<VALUE>, maps the TdfId of VALUE to the TdfId of list<VALUE>. 
    FullTypeNameMap mFullClassNameToTdfIdMap;
    ShortTypeNameMap mShortClassNameToTdfIdMap;
    TemplatedTypeFixupList mTemplatedTypeFixupList;
    ShortNameFixupList mShortNameFixupList;

    // Allocators are cached when fixupTypes is called:
    const char8_t* mAllocationName;
    EA::Allocator::ICoreAllocator* mAllocator;

    TdfClassValueMapHashMap mClassValueMapping;

#if EA_TDF_THREAD_SAFE_TDF_REGISTRATION_ENABLED
    EA::Thread::Mutex mMutex;
#endif
};

/************************************************************************/
/*  Type Registration Classes:
************************************************************************/

struct EATDF_API TypeRegistrationNode :
    public TemplatedTypeFixupListNode,
    public ShortNameFixupListNode,
    public TdfIdToTypeRegistrationMapNode,
    public FullTypeNameToTdfIdMapNode,
    public ShortTypeNameToTdfIdMapNode,
    public ListValueTdfIdToTypeRegistrationMapNode
{
    TypeRegistrationNode(const TypeDescription& typeDesc, bool autoRegister = false);
    void registerType();

    const TypeDescription& info;

    EA_TDF_NON_COPYABLE(TypeRegistrationNode);
};

template <typename T>
struct TdfRegistrationHelper
{
    TdfRegistrationHelper(const TypeDescription& typeDesc)
    {
        if (!typeDesc.isRegistered())
        {
            // Create a registrationNode for the Type, and register it:
            static TypeRegistrationNode s_RegNode(typeDesc);
            s_RegNode.registerType();
        }
    }
};


/************************************************************************/
/*  Type Selection Classes:
************************************************************************/

template <class T>
struct TypeDescriptionSelector
{
    static const TypeDescription& get();
};

//We implement the void instance as a class because legacy create<void> methods expect a class type.
template <> 
struct TypeDescriptionSelector<void>
{
    static const TypeDescriptionClass& get(); 
};

#define TYPE_DESC_CONSTRUCT_NAME(type, tdfType, name) \
template <> \
struct TypeDescriptionSelector<type> { \
    static const TypeDescription& get() { \
        static TypeDescription result(tdfType, tdfType, name); \
        static TdfRegistrationHelper<type> registerHelper(result); \
        return result; \
    } \
};

#define TYPE_DESC_CONSTRUCT(type, tdfType) TYPE_DESC_CONSTRUCT_NAME(type, tdfType, #type)

TYPE_DESC_CONSTRUCT(bool, TDF_ACTUAL_TYPE_BOOL);
TYPE_DESC_CONSTRUCT_NAME(char, TDF_ACTUAL_TYPE_INT8, TypeDescription::CHAR_FULLNAME);
TYPE_DESC_CONSTRUCT(int8_t, TDF_ACTUAL_TYPE_INT8);
TYPE_DESC_CONSTRUCT(uint8_t, TDF_ACTUAL_TYPE_UINT8);
TYPE_DESC_CONSTRUCT(int16_t, TDF_ACTUAL_TYPE_INT16);
TYPE_DESC_CONSTRUCT(uint16_t, TDF_ACTUAL_TYPE_UINT16);
TYPE_DESC_CONSTRUCT(int32_t, TDF_ACTUAL_TYPE_INT32);
TYPE_DESC_CONSTRUCT(uint32_t, TDF_ACTUAL_TYPE_UINT32);
TYPE_DESC_CONSTRUCT(int64_t, TDF_ACTUAL_TYPE_INT64);
TYPE_DESC_CONSTRUCT(uint64_t, TDF_ACTUAL_TYPE_UINT64);
TYPE_DESC_CONSTRUCT_NAME(TdfString, TDF_ACTUAL_TYPE_STRING, "string");
TYPE_DESC_CONSTRUCT(float, TDF_ACTUAL_TYPE_FLOAT);
TYPE_DESC_CONSTRUCT(TimeValue, TDF_ACTUAL_TYPE_TIMEVALUE);
TYPE_DESC_CONSTRUCT(ObjectType, TDF_ACTUAL_TYPE_OBJECT_TYPE);
TYPE_DESC_CONSTRUCT(ObjectId, TDF_ACTUAL_TYPE_OBJECT_ID);

#undef TYPE_DESC_CONSTRUCT
#undef TYPE_DESC_CONSTRUCT_NAME


} //namespace TDF
} //namespace EA


namespace eastl
{
    template <>
    struct use_intrusive_key<EA::TDF::TdfIdToTypeRegistrationMapNode, EA::TDF::TdfId>
    {
        EA::TDF::TdfId operator()(const EA::TDF::TdfIdToTypeRegistrationMapNode& x) const
        {
            return static_cast<const EA::TDF::TypeRegistrationNode &>(x).info.getTdfId();
        }
    };

    template <>
    struct use_intrusive_key<EA::TDF::ListValueTdfIdToTypeRegistrationMapNode, EA::TDF::TdfId>
    {
        EA::TDF::TdfId operator()(const EA::TDF::ListValueTdfIdToTypeRegistrationMapNode& x) const
        {
            return static_cast<const EA::TDF::TypeRegistrationNode &>(x).info.asListDescription()->valueType.getTdfId();
        }
    };

    template <>
    struct use_intrusive_key<EA::TDF::FullTypeNameToTdfIdMapNode, const char8_t*>
    {
        const char8_t* operator()(const EA::TDF::FullTypeNameToTdfIdMapNode& x) const
        {
            return static_cast<const EA::TDF::TypeRegistrationNode &>(x).info.getFullName();
        }
    };

    template <>
    struct use_intrusive_key<EA::TDF::ShortTypeNameToTdfIdMapNode, const char8_t*>
    {
        const char8_t* operator()(const EA::TDF::ShortTypeNameToTdfIdMapNode& x) const
        {
            return static_cast<const EA::TDF::TypeRegistrationNode &>(x).info.getShortName();
        }
    };
}

#endif // EA_TDF_TDFFACTORY_H



