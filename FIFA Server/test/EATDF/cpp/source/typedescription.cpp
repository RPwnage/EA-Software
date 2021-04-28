/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include <EATDF/typedescription.h>
#include <EATDF/tdfenuminfo.h>
#include <EATDF/tdfbase.h>

#include <EAAssert/eaassert.h>
#include <EAStdC/EAString.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
namespace TDF
{

const TypeDescription& TypeDescription::UNKNOWN_TYPE = TypeDescriptionSelector<void>::get();
const char8_t* TypeDescription::CHAR_FULLNAME = "char8_t";
#if EA_TDF_THREAD_SAFE_TYPEDESC_INIT_ENABLED
EA::Thread::Mutex TypeDescription::mTypeDescInitializationMutex;
#endif

TypeDescription::TypeDescription(TdfType _type, TdfId _id, const char8_t* _fullName) : type(_type), id(_id), fullName(_fullName), registrationNode(nullptr) 
{
    if (fullName != nullptr)
    {
        shortName = EA::StdC::Strrchr(fullName, ':');
        if (shortName != nullptr)
            shortName += 1;
        else
            shortName = fullName;
    }
    else
    {
        shortName = nullptr;
    }
}

const TypeDescriptionEnum* TypeDescription::asEnumMap() const { return type == TDF_ACTUAL_TYPE_ENUM ? static_cast<const TypeDescriptionEnum*>(this) : nullptr; }
const TypeDescriptionObject* TypeDescription::asObjectDescription() const { return isTdfObject() ? static_cast<const TypeDescriptionObject*>(this) : nullptr; }
const TypeDescriptionClass* TypeDescription::asClassInfo() const { return (type == TDF_ACTUAL_TYPE_TDF || type == TDF_ACTUAL_TYPE_UNION)? static_cast<const TypeDescriptionClass*>(this) : nullptr; }
const TypeDescriptionList* TypeDescription::asListDescription() const { return type == TDF_ACTUAL_TYPE_LIST ? static_cast<const TypeDescriptionList*>(this) : nullptr; }
const TypeDescriptionMap* TypeDescription::asMapDescription() const { return type == TDF_ACTUAL_TYPE_MAP ? static_cast<const TypeDescriptionMap*>(this) : nullptr; }
const TypeDescriptionBitfield* TypeDescription::asBitfieldDescription() const { return type == TDF_ACTUAL_TYPE_BITFIELD ? static_cast<const TypeDescriptionBitfield*>(this) : nullptr; }

bool TypeDescription::isIntegral() const
{
    switch (type)
    {
    case TDF_ACTUAL_TYPE_BOOL:
    case TDF_ACTUAL_TYPE_INT8:          
    case TDF_ACTUAL_TYPE_UINT8:         
    case TDF_ACTUAL_TYPE_INT16:         
    case TDF_ACTUAL_TYPE_UINT16:        
    case TDF_ACTUAL_TYPE_INT32:         
    case TDF_ACTUAL_TYPE_UINT32:        
    case TDF_ACTUAL_TYPE_INT64:         
    case TDF_ACTUAL_TYPE_UINT64:  
    case TDF_ACTUAL_TYPE_ENUM:
    case TDF_ACTUAL_TYPE_BITFIELD:
    case TDF_ACTUAL_TYPE_TIMEVALUE:
        return true;
    default:
        return false;
    }
}

bool TypeDescription::isIntegralChar() const
{
    return ((type == TDF_ACTUAL_TYPE_INT8) && (fullName != nullptr) && (EA::StdC::Strcmp(fullName, CHAR_FULLNAME) == 0));
}


bool TypeDescription::isTdfObject() const
{
    switch (type)
    {
    case TDF_ACTUAL_TYPE_TDF:
    case TDF_ACTUAL_TYPE_UNION:          
    case TDF_ACTUAL_TYPE_MAP:         
    case TDF_ACTUAL_TYPE_LIST:         
    case TDF_ACTUAL_TYPE_VARIABLE:        
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
    case TDF_ACTUAL_TYPE_BLOB:         
        return true;
    default:
        return false;
    }
}

bool TypeDescription::isBasicType() const
{
    return !isNamedType() && !isTemplatedType();
}

bool TypeDescription::isNamedType() const
{
    switch (type)
    {
    case TDF_ACTUAL_TYPE_TDF:
    case TDF_ACTUAL_TYPE_UNION:         
    case TDF_ACTUAL_TYPE_ENUM:          
    case TDF_ACTUAL_TYPE_BITFIELD:         
        return true;
    default:
        return false;
    }
}

bool TypeDescription::isTemplatedType() const
{
    switch (type)
    {
    case TDF_ACTUAL_TYPE_MAP:
    case TDF_ACTUAL_TYPE_LIST:          
        return true;
    default:
        return false;
    }
}


} //namespace TDF
} //namespace EA


