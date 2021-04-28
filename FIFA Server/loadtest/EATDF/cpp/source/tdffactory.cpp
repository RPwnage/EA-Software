/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include <EATDF/tdffactory.h>
#include <EATDF/typedescription.h>
#include <EATDF/tdfenuminfo.h>
#include <EATDF/tdfbase.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfvector.h>
#include <EATDF/tdfvariable.h>
#include <EATDF/tdfbasetypes.h>


#include <EAAssert/eaassert.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
namespace TDF
{

TdfFactory::TdfFactory()
    : mFixupCompleted(false),
      mAllocationName(EA_TDF_DEFAULT_ALLOC_NAME),
      mAllocator(nullptr)
{
}

void TdfFactory::cleanupTypeAllocations()
{
    TdfFactory::get().cleanupTemplatedTypeAllocations();
}
void TdfFactory::cleanupTemplatedTypeAllocations()
{
#if EA_TDF_THREAD_SAFE_TDF_REGISTRATION_ENABLED
    // the server could be running with multiple slave threads per process
    // therefore, we need to employ mutex in order to avoid race condition
    EA::Thread::AutoMutex mutex(mMutex);
#endif

    // If no allocator exists, just assume no allocations took place
    if (mAllocator == nullptr)
    {
        return;
    }

    // Go through every type, if it's a list or map, fix it up:
    TemplatedTypeFixupList::iterator itr = mTemplatedTypeFixupList.begin();
    TemplatedTypeFixupList::iterator end = mTemplatedTypeFixupList.end();

    while(itr != end)
    {
        TypeDescription& curType = const_cast<TypeDescription&>(((TypeRegistrationNode&)(*itr)).info);

        if (curType.getFullName() != nullptr || curType.getShortName() != nullptr) 
        {
            // We remove the types from the list first, since otherwise they'll mess up the hash lookup:
            deregisterType(curType.getTdfId());

            mAllocator->Free((void*)curType.getFullName());
            mAllocator->Free((void*)curType.getShortName());
            curType.fullName = nullptr;
            curType.shortName = nullptr;
            curType.id = INVALID_TDF_ID;
        }

        ++itr;
    }

    mFixupCompleted = false;

    // Clear all map allocations:
    TdfClassValueMapHashMap::iterator curIter = mClassValueMapping.begin();
    while (curIter != mClassValueMapping.end())
    {
        TdfClassValueMapHashNode* prevItem = const_cast<TdfClassValueMapHashNode*>(&(*curIter));
        ++curIter;
        prevItem->~TdfClassValueMapHashNode();
        mAllocator->Free((void*)prevItem);
    }
    mClassValueMapping.clear();
}

void TdfFactory::fixupTemplatedType(const TypeDescription& curType)
{
    if (curType.getFullName() != nullptr || mAllocator == nullptr) 
    {
        return;
    }

    if (curType.getTdfType() == TDF_ACTUAL_TYPE_LIST)
    {
        TypeDescriptionList* curList = const_cast<TypeDescriptionList*>(curType.asListDescription());
        if (curList->valueType.getFullName() == nullptr)
        {
            fixupTemplatedType(curList->valueType);
        }

        static size_t baseSize = sizeof("list<>") + 1;
        size_t fullSize = baseSize + strlen(curList->valueType.getFullName());
        char* fullName = (char*)mAllocator->Alloc(fullSize, mAllocationName, 0);
        strcpy(fullName, "list<");
        strcat(fullName, curList->valueType.getFullName());
        strcat(fullName, ">");

        size_t shortSize = baseSize + strlen(curList->valueType.getShortName());
        char* shortName = (char*)mAllocator->Alloc(shortSize, mAllocationName, 0);
        strcpy(shortName, "list<");
        strcat(shortName, curList->valueType.getShortName());
        strcat(shortName, ">");

        curList->fullName = fullName;
        curList->id = EA::StdC::FNV1_String8(curList->getFullName());
        curList->shortName = shortName;
    }
    else if (curType.getTdfType() == TDF_ACTUAL_TYPE_MAP)
    {
        TypeDescriptionMap* curMap = const_cast<TypeDescriptionMap*>(curType.asMapDescription());
        if (curMap->valueType.getFullName() == nullptr)
        {
            fixupTemplatedType(curMap->valueType);
        }
        if (curMap->keyType.getFullName() == nullptr)
        {
            fixupTemplatedType(curMap->keyType);
        }

        static size_t baseSize = sizeof("map<,>") + 1;
        size_t fullSize = baseSize + strlen(curMap->valueType.getFullName()) + strlen(curMap->keyType.getFullName());
        char* fullName = (char*)mAllocator->Alloc(fullSize, mAllocationName, 0);
        strcpy(fullName, "map<");
        strcat(fullName, curMap->keyType.getFullName());
        strcat(fullName, ",");
        strcat(fullName, curMap->valueType.getFullName());
        strcat(fullName, ">");

        size_t shortSize = baseSize + strlen(curMap->valueType.getShortName()) + strlen(curMap->keyType.getShortName());
        char* shortName = (char*)mAllocator->Alloc(shortSize, mAllocationName, 0);
        strcpy(shortName, "map<");
        strcat(shortName, curMap->keyType.getShortName());
        strcat(shortName, ",");
        strcat(shortName, curMap->valueType.getShortName());
        strcat(shortName, ">");

        curMap->fullName = fullName;
        curMap->id = EA::StdC::FNV1_String8(curMap->getFullName());
        curMap->shortName = shortName;
    }
    else
    {
        // This is unexpected. ERROR.
        EA_FAIL_FORMATTED(("[TDF] Error: Missing a name for type %u. This is unexpected.", curType.type));
        return;
    }

    TypeRegistrationNode* registrationNode = const_cast<TypeRegistrationNode*>(curType.registrationNode);
    registerType(*registrationNode);
}

void TdfFactory::fixupTypes(EA::Allocator::ICoreAllocator& alloc, const char8_t* allocName)
{
    TdfFactory::get().setAllocator(alloc, allocName);
    TdfFactory::get().fixupAllTemplatedTypes();
    TdfFactory::get().fixupDuplicateShortNames();
}
void TdfFactory::setAllocator(EA::Allocator::ICoreAllocator& alloc, const char8_t* allocName)
{
    mAllocationName = allocName;
    mAllocator = &alloc;
}

void TdfFactory::fixupAllTemplatedTypes()
{
    // Go through every type, if it's a list or map, fix it up:
    TemplatedTypeFixupList::iterator itr = mTemplatedTypeFixupList.begin();
    TemplatedTypeFixupList::iterator end = mTemplatedTypeFixupList.end();

    while (itr != end)
    {
        const TypeDescription& curType = ((TypeRegistrationNode&)(*itr)).info;

        if (curType.getFullName() == nullptr) 
        {
            // Fix up the type:
            fixupTemplatedType(curType);
        }

        ++itr;
    }

    mFixupCompleted = true;
}

void TdfFactory::fixupDuplicateShortNames()
{
    // Go through all elements: Remove the original
    ShortNameFixupList::iterator itr = mShortNameFixupList.begin();
    ShortNameFixupList::iterator end = mShortNameFixupList.end();

    while (itr != end)
    {
        mShortClassNameToTdfIdMap.remove((TypeRegistrationNode&)(*itr));
        ++itr;
    }

    mShortNameFixupList.clear();
}


bool TdfFactory::registerType(TypeRegistrationNode& registrationNode)
{    
#if EA_TDF_THREAD_SAFE_TDF_REGISTRATION_ENABLED
    // the server could be running with multiple slave threads per process
    // therefore, we need to employ mutex in order to avoid race condition
    // for explicit TDF de-registration on the server
    EA::Thread::AutoMutex mutex(mMutex);
#endif

    if (registrationNode.info.getFullName() == nullptr)
    {
        const_cast<TypeDescription&>(registrationNode.info).registrationNode = &registrationNode;

        // If the node is missing it's name, add it to the list for later fixup:
        mTemplatedTypeFixupList.push_back(const_cast<TypeRegistrationNode&>(registrationNode));

        if (mFixupCompleted && mAllocator != nullptr)
        {
            // Fix up the type:
            const TypeDescription& curType = registrationNode.info;
            fixupTemplatedType(curType);
        }

        return true;
    }


    TypeRegistrationMap::const_iterator itr = mRegistry.find(registrationNode.info.getTdfId());
    if (itr != mRegistry.end())
    {
        if (EA::StdC::Strcmp(registrationNode.info.getFullName(), ((TypeRegistrationNode&)(*itr)).info.getFullName()) != 0)
        {
            // Special Exception: char == char8_t == int8_t - Still register the node's name (Technically this means that the TypeDescs != each other)
            if (registrationNode.info.getTdfId() == TDF_ACTUAL_TYPE_INT8)
            {
                mFullClassNameToTdfIdMap.insert(const_cast<TypeRegistrationNode&>(registrationNode));
                mShortClassNameToTdfIdMap.insert(const_cast<TypeRegistrationNode&>(registrationNode));
                return true;
            }

            EA_FAIL_FORMATTED(("[TDF] Error: TdfId(%u) is already registered for %s so it cannot be used again for %s",
                               registrationNode.info.getTdfId(), ((TypeRegistrationNode&)(*itr)).info.getFullName(), registrationNode.info.getFullName()));
            return false;
        }
        else
        {
            //already registered so make sure to register the node
            const_cast<TypeDescription&>(registrationNode.info).registrationNode = &registrationNode;
            return true;
        }
    }

    // Make sure the name isn't taken somehow either
    TdfId id = getTdfIdFromFullClassName(registrationNode.info.getFullName());
    if (id != INVALID_TDF_ID)
    {
        EA_FAIL_FORMATTED(("[TDF] Error: The name %s is already in use, so it cannot be used again for TdfId %u", registrationNode.info.getFullName(), registrationNode.info.getTdfId()));
        return false;
    }

    //explicit const cast here to avoid needing to cast to come into this function with static const instances of this class.
    //the map nodes are a private contract between the class info and the registrar, so as far as the public contract goes we're still
    //honoring the constness of the class.
    mRegistry.insert(const_cast<TypeRegistrationNode&>(registrationNode));
    mFullClassNameToTdfIdMap.insert(const_cast<TypeRegistrationNode&>(registrationNode));

    
    ShortTypeNameMap::iterator cit = mShortClassNameToTdfIdMap.find(registrationNode.info.getShortName());
    if (cit == mShortClassNameToTdfIdMap.end())
    {
        mShortClassNameToTdfIdMap.insert(const_cast<TypeRegistrationNode&>(registrationNode));
    }
    else
    {
        // This indicates that the same name is used for multiple classes/types in different namespaces.
        // This is not a fatal error, it just means we need to change/remove the original registrationNode. 
        if (mFixupCompleted == false)
        {
            // Add it to a fixup list and remove the nodes later
            TypeRegistrationNode& node = const_cast<TypeRegistrationNode&>(static_cast<const TypeRegistrationNode&>(*cit));
            if (mShortNameFixupList.contains(node) == false)
            {
                mShortNameFixupList.push_back(node);
            }
        }
        else
        {
            // If we've already passed the fixup stage, we just remove the duplicate mapping:
            mShortClassNameToTdfIdMap.erase(cit);
        }
    }
    
    // List value registration:
    auto listDesc = registrationNode.info.asListDescription();
    if (listDesc != nullptr)
    {
        auto temp = mListValueRegistry.find(listDesc->valueType.getTdfId());
        if (temp != mListValueRegistry.end())
        {
            return false;
        }
        mListValueRegistry.insert(const_cast<TypeRegistrationNode&>(registrationNode));
    }
        

    // Add the register the node, once registration is complete:
    const_cast<TypeDescription&>(registrationNode.info).registrationNode = &registrationNode;
    return true;
}

void TdfFactory::deregisterType(TdfId tdfId)
{
#if EA_TDF_THREAD_SAFE_TDF_REGISTRATION_ENABLED
    // the server could be running with multiple slave threads per process
    // therefore, we need to employ mutex in order to avoid race condition
    // for explicit TDF deregistration on the server
    EA::Thread::AutoMutex mutex(mMutex);
#endif

    TypeRegistrationMap::iterator itr = mRegistry.find(tdfId);
    if (itr != mRegistry.end())
    {
        FullTypeNameMap::iterator cit = mFullClassNameToTdfIdMap.find(((TypeRegistrationNode&)(*itr)).info.getFullName());
        if (cit != mFullClassNameToTdfIdMap.end())
        {
            mFullClassNameToTdfIdMap.erase(cit);
        }

        ShortTypeNameMap::iterator cit1 = mShortClassNameToTdfIdMap.find(((TypeRegistrationNode&)(*itr)).info.getShortName());
        if (cit1 != mShortClassNameToTdfIdMap.end())
        {
            mShortClassNameToTdfIdMap.erase(cit1);
        }

        mRegistry.erase(itr);
    }
}


const TypeDescription* TdfFactory::getTypeDesc(TdfId tdfId) const
{
    TypeRegistrationMap::const_iterator regItr = mRegistry.find(tdfId);
    if (regItr != mRegistry.end())
    {
        return &(static_cast<const TypeRegistrationNode&>(*regItr)).info;
    }
    return nullptr;
}

const TypeDescription* TdfFactory::getListTypeDescFromValue(TdfId valueTdfId) const
{
    auto regItr = mListValueRegistry.find(valueTdfId);
    if (regItr != mListValueRegistry.end())
    {
        return &(static_cast<const TypeRegistrationNode&>(*regItr)).info;
    }
    return nullptr;
}


const TypeDescription* TdfFactory::getTypeDescInternal(const char8_t* name) const
{
    ShortTypeNameMap::const_iterator cit1 = mShortClassNameToTdfIdMap.find(name);
    if (cit1 != mShortClassNameToTdfIdMap.end())
    {
        return &(static_cast<const TypeRegistrationNode&>(*cit1)).info;
    }

    FullTypeNameMap::const_iterator cit = mFullClassNameToTdfIdMap.find(name);
    if (cit != mFullClassNameToTdfIdMap.end())
    {
        return &(static_cast<const TypeRegistrationNode&>(*cit)).info;
    }
    return nullptr;
}


bool TdfFactory::createGenericValue(TdfId tdfId, TdfGenericValue& outValue) const
{
    const TypeDescription* typeDesc = getTypeDesc(tdfId);
    if (typeDesc != nullptr) 
    {
        outValue.setType(*typeDesc);
        return true;
    }

    return false;
}
bool TdfFactory::createGenericValue(const char8_t* fullClassName, TdfGenericValue& outValue) const
{
    const TypeDescription* typeDesc = getTypeDesc(fullClassName);
    if (typeDesc != nullptr) 
    {
        outValue.setType(*typeDesc);
        return true;
    }

    return false;
}

TdfCreator TdfFactory::getCreator(TdfId tdfId) const
{
    TypeRegistrationMap::const_iterator regItr = mRegistry.find(tdfId);
    if (regItr != mRegistry.end())
    {
        const TypeDescriptionObject* obj = (static_cast<const TypeRegistrationNode&>(*regItr)).info.asObjectDescription();
        return obj ? obj->createFunc : nullptr;
    }
    return nullptr;
}


BitfieldCreator TdfFactory::getBitfieldCreator(TdfId tdfId) const
{
    TypeRegistrationMap::const_iterator regItr = mRegistry.find(tdfId);
    if (regItr != mRegistry.end())
    {
        const TypeDescriptionBitfield* bitfield = (static_cast<const TypeRegistrationNode&>(*regItr)).info.asBitfieldDescription();
        return bitfield ? bitfield->createFunc : nullptr;
    }
    return nullptr;
}


TdfId TdfFactory::getTdfIdFromName(const char8_t* name) const
{
    TdfId id = getTdfIdFromShortClassName(name);
    if (id == INVALID_TDF_ID)
        id = getTdfIdFromFullClassName(name);

    return id;
}

TdfId TdfFactory::getTdfIdFromFullClassName(const char8_t* fullClassName) const
{
    FullTypeNameMap::const_iterator cit = mFullClassNameToTdfIdMap.find(fullClassName);
    if (cit == mFullClassNameToTdfIdMap.end())
        return INVALID_TDF_ID;

    return (static_cast<const TypeRegistrationNode&>(*cit)).info.getTdfId();
}

TdfId TdfFactory::getTdfIdFromShortClassName(const char8_t* shortClassName) const
{
    ShortTypeNameMap::const_iterator cit = mShortClassNameToTdfIdMap.find(shortClassName);
    if (cit == mShortClassNameToTdfIdMap.end())
        return INVALID_TDF_ID;

    return (static_cast<const TypeRegistrationNode&>(*cit)).info.getTdfId();
}

TypeRegistrationNode::TypeRegistrationNode(const TypeDescription& typeDesc, bool autoRegister)
    : info(typeDesc) 
{
    if (autoRegister) 
    {
        registerType();
    }
};

void TypeRegistrationNode::registerType()
{
#ifdef EA_ASSERT_ENABLED
    bool success =  
#endif
        TdfFactory::get().registerType(*this);

    EA_ASSERT(success);
}






static TdfObject* sGetNullTdfObject(EA::Allocator::ICoreAllocator&, const char8_t*, uint8_t*) { return nullptr; }
const TypeDescriptionClass& TypeDescriptionSelector<void>::get() 
{   
    static const EA::TDF::TypeDescriptionClass result(EA::TDF::TDF_ACTUAL_TYPE_UNKNOWN, 0, "", "", &sGetNullTdfObject, nullptr, 0, nullptr);
    return result; 
}








/** 
 *  Destructively tokenize the input string using the: '[' , ']', '.' 
 *  delimiters and append the resulting substings into an output list.
 * 
 * NOTE:
 *  a. The tokenization inserts '\0' elements into the input string.
 *  b. The lifetime of the outList is bound to that of the input string 
 *     because elements in the outList point to data in the input string.
 */
bool TdfFactory::parseString(EATDFEastlString& input, SubStringList& outList) const
{
    outList.clear();

    bool justParsedArray = false;
    EATDFEastlString::size_type tokenStart = 0;
    EATDFEastlString::size_type tokenEnd = 0;
    while ((tokenEnd = input.find_first_of(".[", tokenStart)) != EATDFEastlString::npos)
    {
        if (tokenEnd == tokenStart)
        {
            if (!justParsedArray)
                return false;
        }
        else
        {
            outList.push_back().assign(input.begin() + tokenStart, tokenEnd - tokenStart);
        }

        if (input[tokenEnd] == '[')
        {
             // Remove the rest:
            tokenStart = tokenEnd + 1;

            if ((tokenEnd = input.find_first_of(']', tokenStart)) == EATDFEastlString::npos)
            {
                // Missing end bracket
                return false;
            }
            input[tokenStart-1] = '\0';

            outList.push_back().assign(input.begin() + tokenStart, tokenEnd - tokenStart);

             // Remove the rest:
            tokenStart = tokenEnd + 1;

            if (input.size() > tokenStart && input.find_first_of(".[", tokenStart) == EATDFEastlString::npos)
            {
                // Extra stuff on end.
                return false;
            }
            input[tokenStart-1] = '\0';
            justParsedArray = true;
        }
        else
        {
             // Remove the rest:
            tokenStart = tokenEnd + 1;
            input[tokenStart-1] = '\0';
            justParsedArray = false;
        }
    }

    EATDFEastlString::size_type inputLength = input.size();
    if (tokenStart == inputLength)
    {
        if (!justParsedArray)
        {
            // Ended on delimiter
            return false;
        }
    }
    else
    {
        outList.push_back().assign(input.begin() + tokenStart, inputLength - tokenStart);
    }

    return true;
}

const TypeDescription* TdfFactory::getTypeDesc(const char8_t* qualifiedTypeName, const char8_t* memberName, const TypeDescriptionBitfieldMember** outBfMember, int32_t* outEnumValue) const
{
    if (!memberName)
        return nullptr;

    SubStringList memberLocation(EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
    EATDFEastlString name(memberName, EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
    if (!parseString(name, memberLocation))
        return nullptr;

    SubStringList::const_iterator curMember = memberLocation.begin();
    SubStringList::const_iterator endMember = memberLocation.end();

    if (curMember == endMember)
        return nullptr;

    // First member is the base type description:
    const TypeDescription* baseTypeDesc = getTypeDescInternal(qualifiedTypeName);
    if (baseTypeDesc == nullptr)
        return baseTypeDesc;

    return getMemberTypeDescription(baseTypeDesc, curMember, endMember, outBfMember, outEnumValue);
}
const TypeDescription* TdfFactory::getTypeDesc(const char8_t* qualifiedTypeName, const TypeDescriptionBitfieldMember** outBfMember, int32_t* outEnumValue) const
{
    if (!qualifiedTypeName)
        return nullptr;

    SubStringList memberLocation(EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
    EATDFEastlString name(qualifiedTypeName, EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
    if (!parseString(name, memberLocation))
        return nullptr;

    SubStringList::const_iterator curMember = memberLocation.begin();
    SubStringList::const_iterator endMember = memberLocation.end();

    if (curMember == endMember)
        return nullptr;

    // First member is the base type description:
    const TypeDescription* baseTypeDesc = getTypeDescInternal(curMember->c_str());
    if (baseTypeDesc == nullptr)
        return baseTypeDesc;

    ++curMember;
    return getMemberTypeDescription(baseTypeDesc, curMember, endMember, outBfMember, outEnumValue);
}
const TypeDescription* TdfFactory::getMemberTypeDescription(const TypeDescription* baseTypeDesc, SubStringList::const_iterator curMember, SubStringList::const_iterator endMember, const TypeDescriptionBitfieldMember** outBfMember, int32_t* outEnumValue) const
{
    if (curMember == endMember)
        return baseTypeDesc;

    switch (baseTypeDesc->type)
    {
    // Simple types:
    case TDF_ACTUAL_TYPE_FLOAT:
    case TDF_ACTUAL_TYPE_STRING:
    case TDF_ACTUAL_TYPE_TIMEVALUE:
    case TDF_ACTUAL_TYPE_BOOL:
    case TDF_ACTUAL_TYPE_INT8:
    case TDF_ACTUAL_TYPE_UINT8:
    case TDF_ACTUAL_TYPE_INT16:
    case TDF_ACTUAL_TYPE_UINT16:
    case TDF_ACTUAL_TYPE_INT32:
    case TDF_ACTUAL_TYPE_UINT32:
    case TDF_ACTUAL_TYPE_INT64:
    case TDF_ACTUAL_TYPE_UINT64:
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:   // These three have no member to access
    case TDF_ACTUAL_TYPE_OBJECT_ID:
    case TDF_ACTUAL_TYPE_BLOB:
        return nullptr;

    case TDF_ACTUAL_TYPE_ENUM:
    {
        // Enum special case:  Now you can get the enum member's value, similar to how you can get a bitfield member's 'type'.
        if ((curMember + 1) != endMember)
            return nullptr;

        int32_t tempInt = 0;
        int32_t* tempIntPtr = outEnumValue ? outEnumValue : &tempInt;
        if (!baseTypeDesc->asEnumMap()->findByName(curMember->c_str(), *tempIntPtr))
            return nullptr;

        return baseTypeDesc;
    }

    case TDF_ACTUAL_TYPE_VARIABLE:
    {
        // Syntax: Blaze::VariableMap[][Blaze::FooTdf].foobar
        const TypeDescription* typeDesc = getTypeDescInternal(curMember->c_str()); 
        if (typeDesc == nullptr || typeDesc->type != TDF_ACTUAL_TYPE_TDF)
        {
            return nullptr;
        }
        ++curMember;
        return getMemberTypeDescription(typeDesc, curMember, endMember, outBfMember, outEnumValue);
    }

    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
    {
        // Syntax: Blaze::GenericMap[][Blaze::FooTdf].foobar
        const TypeDescription* typeDesc = getTypeDescInternal(curMember->c_str()); 
        if (typeDesc == nullptr)
        {
            return nullptr;
        }
        ++curMember;
        return getMemberTypeDescription(typeDesc, curMember, endMember, outBfMember, outEnumValue);
    }

    case TDF_ACTUAL_TYPE_LIST:
    {
        ++curMember;
        baseTypeDesc = &baseTypeDesc->asListDescription()->valueType;
        return getMemberTypeDescription(baseTypeDesc, curMember, endMember, outBfMember, outEnumValue);
    }

    case TDF_ACTUAL_TYPE_MAP:
    {
        if (!baseTypeDesc->asMapDescription()->keyType.isString() && !baseTypeDesc->asMapDescription()->keyType.isIntegral())
        {
            // Unless we can parse the value as a string, this won't work.
            return nullptr;
        }
        ++curMember;
        baseTypeDesc = &baseTypeDesc->asMapDescription()->valueType;
        return getMemberTypeDescription(baseTypeDesc, curMember, endMember, outBfMember, outEnumValue);
    }
    case TDF_ACTUAL_TYPE_BITFIELD:
    {
        // We only support bitfields as the last member (early outed above), or as the 2nd to last member, with the last member being the bitfield member name:
        if ((curMember + 1) != endMember)
            return nullptr;

        const TypeDescriptionBitfieldMember* tempBfMember;
        if (outBfMember == nullptr)
            outBfMember = &tempBfMember;

        // Find the element by name, if it doesn't exist then we fail here:
        if (!baseTypeDesc->asBitfieldDescription()->findByName(curMember->c_str(), *outBfMember))
            return nullptr;

        return baseTypeDesc;
    }            
    case TDF_ACTUAL_TYPE_UNION:
    case TDF_ACTUAL_TYPE_TDF:
    {
        const TdfMemberInfo* memInfo = nullptr;
        if (!baseTypeDesc->asClassInfo()->getMemberInfoByName(curMember->c_str(), memInfo))
            return nullptr;

        ++curMember;
        baseTypeDesc = memInfo->getTypeDescription();
        if (curMember == endMember)
            return baseTypeDesc;

        return getMemberTypeDescription(baseTypeDesc, curMember, endMember, outBfMember, outEnumValue);
    }

    case TDF_ACTUAL_TYPE_UNKNOWN:
        return nullptr;
    }

    return nullptr;
}

bool TdfFactory::getTdfMemberReference(const TdfGenericReference& baseRef, const char8_t* qualifiedTypeName, TdfGenericReference& outGenericRef, const TypeDescriptionBitfieldMember** outBfMember, bool nameIncludesTdfName, bool addMissingEntry) const
{
    outGenericRef.clear();
    SubStringList memberLocation(EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
    EATDFEastlString name(qualifiedTypeName, EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
    if (!parseString(name, memberLocation))
        return false;

    SubStringList::const_iterator curMember = memberLocation.begin();
    SubStringList::const_iterator endMember = memberLocation.end();

    if (curMember == endMember)
        return false;

    if (nameIncludesTdfName)
    {
        // First member is the base type description:
        const TypeDescription* baseTypeDesc = getTypeDescInternal(curMember->c_str());
        if (baseTypeDesc == nullptr)
            return false;
        if (baseTypeDesc->getTdfId() != baseRef.getTdfId())
            return false;

        ++curMember;
    }
    return getMemberReference(baseRef, curMember, endMember, outGenericRef, outBfMember, addMissingEntry);
}
bool TdfFactory::getMemberReference(const TdfGenericReference& baseRef, SubStringList::const_iterator curMember, SubStringList::const_iterator endMember, TdfGenericReference& outGenericRef, const TypeDescriptionBitfieldMember** outBfMember, bool addMissingEntry) const
{
    outGenericRef = baseRef;
    if (curMember == endMember)
        return true;

    switch (baseRef.getType())
    {
    // Simple types:
    case TDF_ACTUAL_TYPE_FLOAT:
    case TDF_ACTUAL_TYPE_ENUM:
    case TDF_ACTUAL_TYPE_STRING:
    case TDF_ACTUAL_TYPE_TIMEVALUE:
    case TDF_ACTUAL_TYPE_BOOL:
    case TDF_ACTUAL_TYPE_INT8:
    case TDF_ACTUAL_TYPE_UINT8:
    case TDF_ACTUAL_TYPE_INT16:
    case TDF_ACTUAL_TYPE_UINT16:
    case TDF_ACTUAL_TYPE_INT32:
    case TDF_ACTUAL_TYPE_UINT32:
    case TDF_ACTUAL_TYPE_INT64:
    case TDF_ACTUAL_TYPE_UINT64:
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:   // These three have no member to access
    case TDF_ACTUAL_TYPE_OBJECT_ID:
    case TDF_ACTUAL_TYPE_BLOB:
        return false;
        
    case TDF_ACTUAL_TYPE_VARIABLE:
    {
        // Syntax:
        // Blaze::VariableMap[][Blaze::FooTdf].foobar
        const TypeDescription* typeDesc = getTypeDescInternal(curMember->c_str()); 
        if (typeDesc == nullptr || typeDesc->type != TDF_ACTUAL_TYPE_TDF)
        {
            return false;
        }
        
        // Set the type (if it's not already set and matching):
        bool wasTypeSet = false;
        VariableTdfBase& variableType = baseRef.asVariable();
        if (!variableType.isValid())
        {
            if (!addMissingEntry)
                return false;

            wasTypeSet = true;
            variableType.create(typeDesc->getTdfId());
        }
        else if (variableType.get()->getTdfId() != typeDesc->getTdfId())
        {
            // This means the types were different for some reason. 
            return false;
        }
        outGenericRef = TdfGenericReference(*variableType.get());

        ++curMember;
        if (curMember == endMember)
            return true;

        // Lookup member of value:
        if (getMemberReference(outGenericRef, curMember, endMember, outGenericRef, outBfMember, addMissingEntry))
        {
            return true;
        }

        // If we fail member lookup further down, we will need to reset the generic value:
        if (wasTypeSet)
        {
            variableType.clear();
        }
        return false;
    }

    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
    {
        // Syntax:
        // Blaze::GenericMap[][Blaze::FooTdf].foobar
        // or (if the value is already set)
        // Blaze::GenericMap[].foobar

        
        // Set the type (if it's not already set and matching):
        bool wasTypeSet = false;
        GenericTdfType& genericType = baseRef.asGenericType();
        if (!genericType.isValid())
        {
            if (!addMissingEntry)
                return false;

            const TypeDescription* typeDesc = getTypeDescInternal(curMember->c_str()); 
            if (typeDesc == nullptr)
            {
                return false;
            }

            wasTypeSet = true;
            genericType.setType(*typeDesc);
        }
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        else if (genericType.isSet())
#else
        else
#endif   
        {
            // valid TDF, process it!
            outGenericRef = TdfGenericReference(genericType.getValue());
            // don't advance the iterator, because we want to reprocess this one
            if (getMemberReference(outGenericRef, curMember, endMember, outGenericRef, outBfMember, addMissingEntry))
            {
                return true;
            }
        }
     
        outGenericRef = TdfGenericReference(genericType.getValue());

        ++curMember;
        if (curMember == endMember)
            return true;

        // Lookup member of value:
        if (getMemberReference(outGenericRef, curMember, endMember, outGenericRef, outBfMember, addMissingEntry))
        {
            return true;
        }

        // If we fail member lookup further down, we will need to reset the generic value:
        if (wasTypeSet)
        {
            genericType.clear();
        }
        return false;
    }

    case TDF_ACTUAL_TYPE_LIST:
    {
        // Add the member to the map:
        EA::TDF::TdfVectorBase& curVec = baseRef.asList();
        size_t addedMemberNum = 0;

        char8_t* pEnd = nullptr;
        size_t val = (size_t)EA::StdC::StrtoU64(curMember->c_str(), &pEnd, 10);
        if (curMember->c_str() == pEnd)
        {
            return false;
        }

        size_t index = curVec.vectorSize();
        while (index <= val)
        {
            if (!addMissingEntry)
                return false;

            // Add dummy elements such that we end up at the correct index
            EA::TDF::TdfGenericReference dummyValue;
            curVec.pullBackRef(dummyValue);
            ++index;
            ++addedMemberNum;
        }

        if (curVec.getReferenceByIndex(val, outGenericRef))
        {
            ++curMember;
            if (curMember == endMember)
                return true;

            // Lookup member of value:
            if (getMemberReference(outGenericRef, curMember, endMember, outGenericRef, outBfMember, addMissingEntry))
            {
                return true;
            }
        }

        // If we fail member lookup further down, we will need to remove the dummy elements and actual element
        while (addedMemberNum > 0)
        {
            curVec.popBackRef();
            addedMemberNum--;
        }

        return false;
    }

    case TDF_ACTUAL_TYPE_MAP:
    {
        if (!baseRef.getTypeDescription().asMapDescription()->keyType.isString() && !baseRef.getTypeDescription().asMapDescription()->keyType.isIntegral())
        {
            // Unless we can parse the value as a string, this won't work.
            return false;
        }

        // Add the member to the map:
        EA::TDF::TdfMapBase& curMap = baseRef.asMap();
        bool addedMember = false;

        // Convert the string input into whatever type it is (if possible) - Need to make this much less code. 
        TdfString mapKeyString(curMember->c_str(), *mAllocator);
        TdfGenericValue mapKey(*mAllocator);
        mapKey.setType(baseRef.getTypeDescription().asMapDescription()->keyType);
        TdfGenericReference mapKeyRef(mapKey);
        TdfGenericReference mapKeyStringRef(mapKeyString);
        if (!mapKeyStringRef.convertToResult(mapKeyRef))
        {
            // Unable to parse the map key type:
            return false;
        }

        if (!curMap.getValueByKey(mapKey, outGenericRef))
        {
            if (!addMissingEntry)
                return false;

            addedMember = true;
            curMap.insertKeyGetValue(mapKey, outGenericRef);
        }

        ++curMember;
        if (curMember == endMember)
        {
            // member was in the map
            if (!addedMember && outGenericRef.isValid() && outGenericRef.getType() == TDF_ACTUAL_TYPE_GENERIC_TYPE)
            {
                if (getMemberReference(outGenericRef, --curMember, endMember, outGenericRef, outBfMember, addMissingEntry))
                {
                    return true;
                }
            }
            return true;
        }
        // Lookup member of value:
        if (getMemberReference(outGenericRef, curMember, endMember, outGenericRef, outBfMember, addMissingEntry))
        {
            return true;
        }

        // If we fail member lookup further down, we will need to remove it
        if (addedMember)
        {
            curMap.eraseValueByKey(mapKey);
        }
        return false;
    }
    case TDF_ACTUAL_TYPE_BITFIELD:
    {
        // We only support bitfields as the last member (early outed above), or as the 2nd to last member, with the last member being the bitfield member name:
        if ((curMember + 1) != endMember)
            return false;

        const TypeDescriptionBitfieldMember* tempBfMember;
        if (outBfMember == nullptr)
            outBfMember = &tempBfMember;

        // Find the element by name, if it doesn't exist then we fail here:
        if (!baseRef.asBitfield().getTypeDescription().findByName(curMember->c_str(), *outBfMember))
            return false;

        return true;
    }    
    case TDF_ACTUAL_TYPE_UNION:
    case TDF_ACTUAL_TYPE_TDF:
    {
        if (!baseRef.asTdf().getValueByName(curMember->c_str(), outGenericRef))
            return false;

        ++curMember;
        if (curMember == endMember)
            return true;

        return getMemberReference(outGenericRef, curMember, endMember, outGenericRef, outBfMember, addMissingEntry);
    }

    case TDF_ACTUAL_TYPE_UNKNOWN:
        return false;
    }

    return false;
}



bool TdfFactory::registerTypeName(const char8_t* className, const char8_t* valueName, const char8_t* mappedValue)
{
#if EA_TDF_THREAD_SAFE_TDF_REGISTRATION_ENABLED
    // the server could be running with multiple slave threads per process
    // therefore, we need to employ mutex in order to avoid race condition
    EA::Thread::AutoMutex mutex(mMutex);
#endif

    if (className == nullptr || valueName == nullptr || mappedValue == nullptr)
    {
        return false;
    }

    TdfClassValueMapHashNode* tempNode = new(mAllocator->Alloc(sizeof(TdfClassValueMapHashNode), mAllocationName, 0)) TdfClassValueMapHashNode(EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
    tempNode->mClass = className;
    tempNode->mValue = valueName;
    tempNode->mTdfValue = mappedValue;

    TdfClassValueMapHashMap::iterator iter = mClassValueMapping.find(*tempNode);
    if (iter == mClassValueMapping.end())
    {
        mClassValueMapping.insert(*tempNode);
    }
    else
    {
        tempNode->~TdfClassValueMapHashNode();
        mAllocator->Free(tempNode);
        *const_cast<EATDFEastlString*>(&iter->mTdfValue) = mappedValue;  
    }

    EATDFEastlString tempName(className, EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));

    // Try again with just the first substring: 
    SubStringList outList(EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
    if (!parseString(tempName, outList))
    {
        return false;
    }

    EATDFEastlString tempName2(className, EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
    if (tempName2.comparei(outList[0].c_str()) != 0)
    {
        TdfClassValueMapHashNode* tempNode2 = new(mAllocator->Alloc(sizeof(TdfClassValueMapHashNode), mAllocationName, 0)) TdfClassValueMapHashNode(EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
        tempNode2->mClass = outList[0];
        tempNode2->mValue = valueName;
        tempNode2->mTdfValue = mappedValue;

        TdfClassValueMapHashMap::iterator iterTemp = mClassValueMapping.find(*tempNode2);
        if (iterTemp == mClassValueMapping.end())
        {
            mClassValueMapping.insert(*tempNode2);
        }
        else
        {
            tempNode2->~TdfClassValueMapHashNode();
            mAllocator->Free(tempNode2);
            *const_cast<EATDFEastlString*>(&iterTemp->mTdfValue) = mappedValue;  
        }
    }
    return true;
}

bool TdfFactory::lookupTypeName(const char8_t* className, const char8_t* valueName, const char8_t*& outMappedValue, bool findExactMatch) const
{
    if (className == nullptr || valueName == nullptr)
        return false;

    TdfClassValueMapHashNode tempNode(EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
    tempNode.mClass = className;
    tempNode.mValue = valueName;
    TdfClassValueMapHashMap::const_iterator iter = mClassValueMapping.find(tempNode);
    if (iter == mClassValueMapping.end() && !findExactMatch)
    {
        // Try again with just the first substring: 
        EATDFEastlString tempName(className, EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
        SubStringList outList(EA_TDF_ALLOCATOR_TYPE(mAllocationName, mAllocator));
        if (!parseString(tempName, outList))
        {
            return false;
        }
        tempNode.mClass = outList[0];
        iter = mClassValueMapping.find(tempNode);
    }

    if (iter != mClassValueMapping.end())
    {
        outMappedValue = iter->mTdfValue.c_str();
        return true;
    }
    return false;
}

TdfRegistrationHelper<bool> registrationHelper_bool(TypeDescriptionSelector<bool>::get());
TdfRegistrationHelper<char> registrationHelper_char(TypeDescriptionSelector<char>::get());
TdfRegistrationHelper<int8_t> registrationHelper_int8_t(TypeDescriptionSelector<int8_t>::get());
TdfRegistrationHelper<uint8_t> registrationHelper_uint8_t(TypeDescriptionSelector<uint8_t>::get());
TdfRegistrationHelper<int16_t> registrationHelper_int16_t(TypeDescriptionSelector<int16_t>::get());
TdfRegistrationHelper<uint16_t> registrationHelper_uint16_t(TypeDescriptionSelector<uint16_t>::get());
TdfRegistrationHelper<int32_t> registrationHelper_int32_t(TypeDescriptionSelector<int32_t>::get());
TdfRegistrationHelper<uint32_t> registrationHelper_uint32_t(TypeDescriptionSelector<uint32_t>::get());
TdfRegistrationHelper<int64_t> registrationHelper_int64_t(TypeDescriptionSelector<int64_t>::get());
TdfRegistrationHelper<uint64_t> registrationHelper_uint64_t(TypeDescriptionSelector<uint64_t>::get());
TdfRegistrationHelper<TdfString> registrationHelper_TdfString(TypeDescriptionSelector<TdfString>::get());
TdfRegistrationHelper<float> registrationHelper_float(TypeDescriptionSelector<float>::get());
TdfRegistrationHelper<TimeValue> registrationHelper_TimeValue(TypeDescriptionSelector<TimeValue>::get());
TdfRegistrationHelper<ObjectType> registrationHelper_ObjectType(TypeDescriptionSelector<ObjectType>::get());
TdfRegistrationHelper<ObjectId> registrationHelper_ObjectId(TypeDescriptionSelector<ObjectId>::get());

} //namespace TDF
} //namespace EA


