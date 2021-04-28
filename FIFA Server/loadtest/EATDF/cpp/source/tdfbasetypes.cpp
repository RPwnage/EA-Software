/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include <EATDF/tdfbasetypes.h>
#include <EATDF/tdfbitfield.h>
#include <EATDF/tdfstring.h>
#include <EATDF/tdfvariable.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfvector.h>
#include <EATDF/tdfbase.h>
#include <EATDF/tdfblob.h>
#include <EATDF/tdfunion.h>

#include <EAAssert/eaassert.h>
#include <EAStdC/EASprintf.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
namespace TDF
{

bool TdfGenericConst::equalsValue(const TdfGenericConst& other) const
{
    if (other.getTdfId() != getTdfId())
    {
        return false;
    }

    switch (getType())
    {
    case TDF_ACTUAL_TYPE_MAP:           return asMap().equalsValue(other.asMap());
    case TDF_ACTUAL_TYPE_LIST:          return asList().equalsValue(other.asList());
    case TDF_ACTUAL_TYPE_FLOAT:         return (asFloat() == other.asFloat());
    case TDF_ACTUAL_TYPE_ENUM:          return (asEnum() == other.asEnum()); 
    case TDF_ACTUAL_TYPE_STRING:        return (asString() == other.asString()); 
    case TDF_ACTUAL_TYPE_VARIABLE:      return asVariable().equalsValue(other.asVariable());
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:  return asGenericType().equalsValue(other.asGenericType());
    case TDF_ACTUAL_TYPE_BITFIELD:      return (asBitfield() == other.asBitfield());
    case TDF_ACTUAL_TYPE_BLOB:          return (asBlob() == other.asBlob());
    case TDF_ACTUAL_TYPE_UNION:         return asUnion().equalsValue(other.asUnion());
    case TDF_ACTUAL_TYPE_TDF:           return asTdf().equalsValue(other.asTdf());
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:   return (asObjectType() == other.asObjectType()); 
    case TDF_ACTUAL_TYPE_OBJECT_ID:     return (asObjectId() == other.asObjectId());
    case TDF_ACTUAL_TYPE_TIMEVALUE:     return (asTimeValue() == other.asTimeValue());
    case TDF_ACTUAL_TYPE_BOOL:          return (asBool() == other.asBool());
    case TDF_ACTUAL_TYPE_INT8:          return (asInt8() == other.asInt8());
    case TDF_ACTUAL_TYPE_UINT8:         return (asUInt8() == other.asUInt8());
    case TDF_ACTUAL_TYPE_INT16:         return (asInt16() == other.asInt16());
    case TDF_ACTUAL_TYPE_UINT16:        return (asUInt16() == other.asUInt16());
    case TDF_ACTUAL_TYPE_INT32:         return (asInt32() == other.asInt32());
    case TDF_ACTUAL_TYPE_UINT32:        return (asUInt32() == other.asUInt32());
    case TDF_ACTUAL_TYPE_INT64:         return (asInt64() == other.asInt64());
    case TDF_ACTUAL_TYPE_UINT64:        return (asUInt64() == other.asUInt64());
    case TDF_ACTUAL_TYPE_UNKNOWN:       return true;        
    default:
        EA_FAIL_MSG("Invalid info type in tdf member info struct");
        return false; //bad news
    }
}

#if EA_TDF_INCLUDE_CHANGE_TRACKING
bool TdfGenericConst::isTdfObjectSet() const
{
    if (!isValid() || !mTypeDesc->isTdfObject())
        return false;

    switch (getType())
    {
    case TDF_ACTUAL_TYPE_MAP:           return asMap().isSet();
    case TDF_ACTUAL_TYPE_LIST:          return asList().isSet();
    case TDF_ACTUAL_TYPE_VARIABLE:      return asVariable().isSet();
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:  return asGenericType().isSet();
    case TDF_ACTUAL_TYPE_BLOB:          return asBlob().isSet();
    case TDF_ACTUAL_TYPE_UNION:         return asUnion().isSet();
    case TDF_ACTUAL_TYPE_TDF:           return asTdf().isSet(); 
    default:
        EA_FAIL_MSG("Invalid info type in tdf member info struct");
        return false; //bad news
    }
}
#endif


bool TdfGenericConst::createIntegralKey(TdfGenericReference& result) const
{
    return convertToIntegral(result);
}

// convertToIntegral Also converts to floats:
bool TdfGenericConst::convertToIntegral(TdfGenericReference& result) const
{
#define SWITCH_CASE_HELPER(resultTdfType, resultType, asType)   \
    case resultTdfType: \
    switch(getType()) \
    { \
    case TDF_ACTUAL_TYPE_BOOL:          result.asType() = (resultType)asBool();     return true; \
    case TDF_ACTUAL_TYPE_INT8:          result.asType() = (resultType)asInt8();     return true; \
    case TDF_ACTUAL_TYPE_UINT8:         result.asType() = (resultType)asUInt8();    return true; \
    case TDF_ACTUAL_TYPE_INT16:         result.asType() = (resultType)asInt16();    return true; \
    case TDF_ACTUAL_TYPE_UINT16:        result.asType() = (resultType)asUInt16();   return true; \
    case TDF_ACTUAL_TYPE_INT32:         result.asType() = (resultType)asInt32();    return true; \
    case TDF_ACTUAL_TYPE_UINT32:        result.asType() = (resultType)asUInt32();   return true; \
    case TDF_ACTUAL_TYPE_INT64:         result.asType() = (resultType)asInt64();    return true; \
    case TDF_ACTUAL_TYPE_UINT64:        result.asType() = (resultType)asUInt64();   return true; \
    case TDF_ACTUAL_TYPE_ENUM:          result.asType() = (resultType)asEnum();     return true; \
    case TDF_ACTUAL_TYPE_FLOAT:         result.asType() = (resultType)asFloat();    return true; \
    case TDF_ACTUAL_TYPE_TIMEVALUE:     result.asType() = (resultType)asTimeValue().getMicroSeconds();    return true; \
    case TDF_ACTUAL_TYPE_BITFIELD:      result.asType() = (resultType)asBitfield().getBits();    return true; \
    default:                                                                        return false;\
    }

    switch (result.getType())
    {
    case TDF_ACTUAL_TYPE_BOOL: 
        switch(getType()) 
        { 
        case TDF_ACTUAL_TYPE_BOOL:          result.asBool() = asBool();          return true; 
        case TDF_ACTUAL_TYPE_INT8:          result.asBool() = asInt8() != 0;     return true; 
        case TDF_ACTUAL_TYPE_UINT8:         result.asBool() = asUInt8() != 0;    return true; 
        case TDF_ACTUAL_TYPE_INT16:         result.asBool() = asInt16() != 0;    return true; 
        case TDF_ACTUAL_TYPE_UINT16:        result.asBool() = asUInt16() != 0;   return true; 
        case TDF_ACTUAL_TYPE_INT32:         result.asBool() = asInt32() != 0;    return true; 
        case TDF_ACTUAL_TYPE_UINT32:        result.asBool() = asUInt32() != 0;   return true; 
        case TDF_ACTUAL_TYPE_INT64:         result.asBool() = asInt64() != 0;    return true; 
        case TDF_ACTUAL_TYPE_UINT64:        result.asBool() = asUInt64() != 0;   return true; 
        case TDF_ACTUAL_TYPE_FLOAT:         result.asBool() = asFloat() != 0;    return true; 
        case TDF_ACTUAL_TYPE_TIMEVALUE:     result.asBool() = asTimeValue().getMicroSeconds() != 0;    return true;
        case TDF_ACTUAL_TYPE_BITFIELD:      result.asBool() = asBitfield().getBits() != 0;    return true;
        default:                                                                 return false;
        }
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_INT8,     int8_t,     asInt8);
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_UINT8,    uint8_t,    asUInt8);
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_INT16,    int16_t,    asInt16);
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_UINT16,   uint16_t,   asUInt16);
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_INT32,    int32_t,    asInt32);
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_UINT32,   uint32_t,   asUInt32);
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_INT64,    int64_t,    asInt64);
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_UINT64,   uint64_t,   asUInt64);
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_ENUM,     int32_t,    asEnum);
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_FLOAT,    float,      asFloat);
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_BITFIELD, uint32_t,   asBitfield);
    SWITCH_CASE_HELPER(TDF_ACTUAL_TYPE_TIMEVALUE, int64_t,   asTimeValue);

    default: return false;
    }

#undef SWITCH_CASE_HELPER
}

bool TdfGenericConst::convertToString(TdfGenericReference& result) const
{
    if (result.getType() != TDF_ACTUAL_TYPE_STRING)
        return false;

    if (getType() == TDF_ACTUAL_TYPE_STRING)
    {
        result.asString().set(asString().c_str());                
        return true;
    }

    char buffer[64];
    switch (getType())
    {
        case TDF_ACTUAL_TYPE_BOOL:         EA::StdC::Sprintf(buffer, "%" PRIu64, asBool() ? 1 : 0);  break;
        case TDF_ACTUAL_TYPE_UINT8:        EA::StdC::Sprintf(buffer, "%" PRIu8, asUInt8());          break;
        case TDF_ACTUAL_TYPE_UINT16:       EA::StdC::Sprintf(buffer, "%" PRIu16, asUInt16());        break;
        case TDF_ACTUAL_TYPE_UINT32:       EA::StdC::Sprintf(buffer, "%" PRIu32, asUInt32());        break;
        case TDF_ACTUAL_TYPE_UINT64:       EA::StdC::Sprintf(buffer, "%" PRIu64, asUInt64());        break;
        case TDF_ACTUAL_TYPE_INT8:         EA::StdC::Sprintf(buffer, "%" PRIi8, asInt8());           break;
        case TDF_ACTUAL_TYPE_INT16:        EA::StdC::Sprintf(buffer, "%" PRIi16, asInt16());         break;
        case TDF_ACTUAL_TYPE_INT32:        EA::StdC::Sprintf(buffer, "%" PRIi32, asInt32());         break;
        case TDF_ACTUAL_TYPE_INT64:        EA::StdC::Sprintf(buffer, "%" PRIi64, asInt64());         break;
        case TDF_ACTUAL_TYPE_ENUM:         
        {
            // Attempt to maintain enum as a string for enum mapping:
            // This results in a loss of precision if multiple enums names map to the same value.  
            const char8_t* enumString = nullptr;
            if (getTypeDescription().asEnumMap()->findByValue(asEnum(), &enumString))
                EA::StdC::Sprintf(buffer, "%s", enumString);
            else
                EA::StdC::Sprintf(buffer, "%" PRIi32, asEnum());
            break;
        }
        case TDF_ACTUAL_TYPE_FLOAT:        EA::StdC::Sprintf(buffer, "%f", asFloat());               break;
        case TDF_ACTUAL_TYPE_BITFIELD:     EA::StdC::Sprintf(buffer, "%" PRIu32, asBitfield().getBits()); break;
        case TDF_ACTUAL_TYPE_TIMEVALUE:    asTimeValue().toStringBasedOnValue(buffer, sizeof(buffer)); break;
        default:                return false;
    }

    result.asString().set(buffer);    
    return true;
}

bool TdfGenericConst::convertFromString(TdfGenericReference& result) const
{
    if (getType() != TDF_ACTUAL_TYPE_STRING)
        return false;

    if (result.getType() == TDF_ACTUAL_TYPE_STRING)
    {
        result.asString().set(asString().c_str());                
        return true;
    }

    switch (result.getType())
    {
        case TDF_ACTUAL_TYPE_BOOL:         result.asBool()     = (EA::StdC::AtoI32(asString().c_str()) == 1 || EA::StdC::Stricmp(asString().c_str(), "true") == 0);  break;
        case TDF_ACTUAL_TYPE_UINT8:        result.asUInt8()    = (uint8_t) EA::StdC::AtoU32(asString().c_str());  break;
        case TDF_ACTUAL_TYPE_UINT16:       result.asUInt16()   = (uint16_t)EA::StdC::AtoU32(asString().c_str());  break;
        case TDF_ACTUAL_TYPE_UINT32:       result.asUInt32()   = EA::StdC::AtoU32(asString().c_str());  break;
        case TDF_ACTUAL_TYPE_UINT64:       result.asUInt64()   = EA::StdC::AtoU64(asString().c_str());  break;
        case TDF_ACTUAL_TYPE_INT8:         result.asInt8()     = (int8_t) EA::StdC::AtoI32(asString().c_str());  break;
        case TDF_ACTUAL_TYPE_INT16:        result.asInt16()    = (int16_t)EA::StdC::AtoI32(asString().c_str());  break;
        case TDF_ACTUAL_TYPE_INT32:        result.asInt32()    = EA::StdC::AtoI32(asString().c_str());  break;
        case TDF_ACTUAL_TYPE_INT64:        result.asInt64()    = EA::StdC::AtoI64(asString().c_str());  break;
        case TDF_ACTUAL_TYPE_ENUM:         
        {
            // Attempt to convert string using strings in enum mapping:
            if (!result.getTypeDescription().asEnumMap()->findByName(asString().c_str(), result.asEnum()))
            {
                // Fall back on integer parsing if that fails:
                result.asEnum()     = EA::StdC::AtoI32(asString().c_str());  
            }
            break;
        }
        case TDF_ACTUAL_TYPE_FLOAT:        result.asFloat()    = (float) EA::StdC::Atof(asString().c_str());    break;
        case TDF_ACTUAL_TYPE_TIMEVALUE:    return result.asTimeValue().parseTimeAllFormats(asString().c_str());         // Note: This can fail.
        case TDF_ACTUAL_TYPE_BITFIELD:     result.asBitfield() = EA::StdC::AtoU32(asString().c_str());  break;          // No complex string parsing for Bitfields. (Currently)
        default:                return false;
    }
    return true;
}

bool TdfGenericConst::convertToList(TdfGenericReference& result) const
{               
    if (result.getType() != TDF_ACTUAL_TYPE_LIST ||
       !canTypesConvert(getTypeDescription(), result.getTypeDescription()))
    {
        return false;
    }

    TdfGenericReference resultListValueRef;
    result.asList().pullBackRef(resultListValueRef);
    if (convertToResult(resultListValueRef))
    {
        return true;
    }

    result.asList().popBackRef();
    return false;
}

bool TdfGenericConst::convertFromList(TdfGenericReference& result) const
{               
    if (getType() != TDF_ACTUAL_TYPE_LIST ||
       !canTypesConvert(getTypeDescription(), result.getTypeDescription()) ||
        asList().vectorSize() != 1)
    {
        return false;
    }

    // Special case: List with single element sent from client ->  Single element on server
    TdfGenericReferenceConst listValueRef;
    asList().getFrontValue(listValueRef);
    if (listValueRef.convertToResult(result))
    {
        // Value sent from client was assigned correctly (as a single element)
        return true;
    }

    return false;
}

bool TdfGenericConst::convertBetweenLists(TdfGenericReference& result) const
{
    if (getType() != TDF_ACTUAL_TYPE_LIST || result.getType() != TDF_ACTUAL_TYPE_LIST)
    {
        return false;
    }

    // Convert all the values one at a time: 
    for (size_t index = 0; index < asList().vectorSize(); ++index)
    {
        TdfGenericReferenceConst listValueRef;
        asList().getValueByIndex(index, listValueRef);

        TdfGenericReference resultListValueRef;
        result.asList().pullBackRef(resultListValueRef);
        if (!listValueRef.convertToResult(resultListValueRef))
        {
            return false;
        }
    }

    return true;
}

bool TdfGenericConst::convertBetweenMaps(TdfGenericReference& result) const
{
    if (getType() != TDF_ACTUAL_TYPE_MAP || result.getType() != TDF_ACTUAL_TYPE_MAP)
    {
        return false;
    }

    // Convert all keys and values: 
    for (size_t index = 0; index < asMap().mapSize(); ++index)
    {
        TdfGenericReferenceConst mapKeyRef, mapValueRef;
        asMap().getValueByIndex(index, mapKeyRef, mapValueRef);

        EA::TDF::TdfGenericValue resultMapKey(result.asMap().getAllocator());
        resultMapKey.setType(result.asMap().getKeyTypeDesc());
        EA::TDF::TdfGenericReference resultMapKeyRef(resultMapKey);
        if (!mapKeyRef.convertToResult(resultMapKeyRef))
        {
            return false;
        }

        TdfGenericReference resultMapValueRef;
        result.asMap().insertKeyGetValue(resultMapKeyRef, resultMapValueRef);
        if (!mapValueRef.convertToResult(resultMapValueRef))
        {
            return false;
        }
    }

    return true;
}

bool TdfGenericConst::convertToResult(TdfGenericReference& result, const TypeDescriptionBitfieldMember* bfMember) const
{
    // Bitfield Member output:
    if (bfMember != nullptr)
    {
        if (result.getType() != TDF_ACTUAL_TYPE_BITFIELD)
            return false;

        // Special Case for Bitfield Members: 
        uint32_t bits;
        TdfGenericReference bitsRef(bits);
        if (convertToResult(bitsRef))
        {
            return result.asBitfield().setValueByDesc(bfMember, bits);
        }
        return false;
    }

// Convert based on the types:

    // If types match, just set the values directly: 
    if (getTdfId() == result.getTdfId())
        return result.setValue(*this);

    // Attempt to convert to/from basic convertible types:
    if (isSimpleConvertibleType(getTypeDescription()) && isSimpleConvertibleType(result.getTypeDescription()))
    {
        if (result.getTypeDescription().isString())
            return convertToString(result);
        else if (getTypeDescription().isString())
            return convertFromString(result);
        else
            return convertToIntegral(result);
    }

    // Convert list/maps with convertable types:
    if (getType() == TDF_ACTUAL_TYPE_LIST && result.getType() == TDF_ACTUAL_TYPE_LIST)
        return convertBetweenLists(result);
    if (getType() == TDF_ACTUAL_TYPE_MAP && result.getType() == TDF_ACTUAL_TYPE_MAP)
        return convertBetweenMaps(result);

    // Attempt to do the list conversion:  (No Merge Op)
    // Single element list conversion:
    if (getType() == TDF_ACTUAL_TYPE_LIST || result.getType() == TDF_ACTUAL_TYPE_LIST)
    {
        if (convertToList(result))    //  V ->  list<V>[0]
            return true;

        if (convertFromList(result))  // list<V>[0] -> V
            return true;
    }
    return false;
}

bool TdfGenericConst::isSimpleConvertibleType(const TypeDescription& type1)
{
    return type1.isIntegral() || type1.isFloat() || type1.isString();
}

bool TdfGenericConst::canTypesConvert(const TypeDescription& type1, const TypeDescription& type2)
{
    // Matching types (no conversion needed):
    if (type1.getTdfId() == type2.getTdfId())
        return true;

    // Basic Conversion supported (int, float, string):
    if ((isSimpleConvertibleType(type1)) && (isSimpleConvertibleType(type2)) )
        return true;

    // List element conversion:  (May fail if >1 list element is used)
    if ((type1.getTdfType() == TDF_ACTUAL_TYPE_LIST && canTypesConvert(type1.asListDescription()->valueType, type2)) ||
        (type2.getTdfType() == TDF_ACTUAL_TYPE_LIST && canTypesConvert(type2.asListDescription()->valueType, type1)) )
        return true;
    
    // List to List conversion:  (Only Basic convertible type support)
    if (type1.getTdfType() == TDF_ACTUAL_TYPE_LIST && type2.getTdfType() == TDF_ACTUAL_TYPE_LIST &&
        isSimpleConvertibleType(type1.asListDescription()->valueType) && isSimpleConvertibleType(type2.asListDescription()->valueType))
        return true;

    // Map to Map conversion:  (Only Basic convertible type support)
    if (type1.getTdfType() == TDF_ACTUAL_TYPE_MAP && type2.getTdfType() == TDF_ACTUAL_TYPE_MAP &&
        isSimpleConvertibleType(type1.asMapDescription()->keyType) && isSimpleConvertibleType(type2.asMapDescription()->keyType) &&
        isSimpleConvertibleType(type1.asMapDescription()->valueType) && isSimpleConvertibleType(type2.asMapDescription()->valueType))
        return true;

    return false;
}


void TdfGenericConst::setRef(const Tdf& val)                    { mTypeDesc = &val.getTypeDescription();  mRefPtr = (uint8_t*) &val; }    
void TdfGenericConst::setRef(const TdfUnion& val)               { mTypeDesc = &val.getTypeDescription();  mRefPtr = (uint8_t*) &val; }    
void TdfGenericConst::setRef(const TdfVectorBase& val)          { mTypeDesc = &val.getTypeDescription();  mRefPtr = (uint8_t*) &val; }    
void TdfGenericConst::setRef(const TdfMapBase& val)             { mTypeDesc = &val.getTypeDescription();  mRefPtr = (uint8_t*) &val; }   
void TdfGenericConst::setRef(const TdfObject& val)              { mTypeDesc = &val.getTypeDescription();  mRefPtr = (uint8_t*) &val; }

void TdfGenericConst::setRef(const TdfGenericConst& val)        { mTypeDesc = val.mTypeDesc;              mRefPtr = val.mRefPtr;     }
void TdfGenericConst::setRef(const TdfGeneric& val)             { mTypeDesc = val.mTypeDesc;              mRefPtr = val.mRefPtr;     }
void TdfGenericConst::setRef(const TdfGenericValue& val)        { mTypeDesc = val.mTypeDesc;              mRefPtr = val.mRefPtr;     }
void TdfGenericConst::setRef(const TdfGenericReference& val)    { mTypeDesc = val.mTypeDesc;              mRefPtr = val.mRefPtr;     }
void TdfGenericConst::setRef(const TdfGenericReferenceConst& val){mTypeDesc = val.mTypeDesc;              mRefPtr = val.mRefPtr;     }
void TdfGenericConst::setRef(TdfGenericConst& val)              { mTypeDesc = val.mTypeDesc;              mRefPtr = val.mRefPtr;     }
void TdfGenericConst::setRef(TdfGeneric& val)                   { mTypeDesc = val.mTypeDesc;              mRefPtr = val.mRefPtr;     }
void TdfGenericConst::setRef(TdfGenericValue& val)              { mTypeDesc = val.mTypeDesc;              mRefPtr = val.mRefPtr;     }
void TdfGenericConst::setRef(TdfGenericReference& val)          { mTypeDesc = val.mTypeDesc;              mRefPtr = val.mRefPtr;     }
void TdfGenericConst::setRef(TdfGenericReferenceConst& val)     { mTypeDesc = val.mTypeDesc;              mRefPtr = val.mRefPtr;     }


TdfGenericReferenceConst::TdfGenericReferenceConst(const Tdf& val) : TdfGenericConst(val.getTypeDescription(), (uint8_t*) &val) {}
TdfGenericReferenceConst::TdfGenericReferenceConst(const TdfUnion& val) : TdfGenericConst(val.getTypeDescription(), (uint8_t*) &val) {}      
TdfGenericReferenceConst::TdfGenericReferenceConst(const TdfVectorBase& val) : TdfGenericConst(val.getTypeDescription(), (uint8_t*) &val) {} 
TdfGenericReferenceConst::TdfGenericReferenceConst(const TdfMapBase& val) : TdfGenericConst(val.getTypeDescription(), (uint8_t*) &val) {} 
TdfGenericReferenceConst::TdfGenericReferenceConst(const TdfObject& val) : TdfGenericConst(val.getTypeDescription(), (uint8_t*) &val) {}

bool TdfGenericReference::setValue(const TdfGenericConst& value, const MemberVisitOptions& options)
{
    if (value.getTdfId() != getTdfId()) 
    {
        return false;
    }

    switch (getType())
    {
    case TDF_ACTUAL_TYPE_MAP:           value.asMap().copyIntoObject(asMap(), options); break;
    case TDF_ACTUAL_TYPE_LIST:          value.asList().copyIntoObject(asList(), options); break;
    case TDF_ACTUAL_TYPE_FLOAT:         asFloat() = value.asFloat(); break;
    case TDF_ACTUAL_TYPE_ENUM:          asEnum() = value.asInt32(); break;
    case TDF_ACTUAL_TYPE_STRING:        asString().set(value.asString()); break;
    case TDF_ACTUAL_TYPE_VARIABLE:      value.asVariable().copyInto(asVariable(), options); break;
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:  value.asGenericType().copyIntoObject(asGenericType(), options); break;
    case TDF_ACTUAL_TYPE_BITFIELD:      asBitfield().setBits(value.asBitfield().getBits()); break;
    case TDF_ACTUAL_TYPE_BLOB:          value.asBlob().copyInto(asBlob(), options); break;
    case TDF_ACTUAL_TYPE_UNION:         value.asUnion().copyInto(asUnion(), options); break;
    case TDF_ACTUAL_TYPE_TDF:           value.asTdf().copyInto(asTdf(), options); break;
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:   asObjectType() = value.asObjectType(); break;
    case TDF_ACTUAL_TYPE_OBJECT_ID:     asObjectId() = value.asObjectId();  break;
    case TDF_ACTUAL_TYPE_TIMEVALUE:     asTimeValue() = value.asTimeValue();  break;
    case TDF_ACTUAL_TYPE_BOOL:          asBool() = value.asBool(); break;
    case TDF_ACTUAL_TYPE_INT8:          asInt8() = value.asInt8(); break;
    case TDF_ACTUAL_TYPE_UINT8:         asUInt8() = value.asUInt8(); break;
    case TDF_ACTUAL_TYPE_INT16:         asInt16() = value.asInt16(); break;
    case TDF_ACTUAL_TYPE_UINT16:        asUInt16() = value.asUInt16(); break;
    case TDF_ACTUAL_TYPE_INT32:         asInt32() = value.asInt32(); break;
    case TDF_ACTUAL_TYPE_UINT32:        asUInt32() = value.asUInt32(); break;
    case TDF_ACTUAL_TYPE_INT64:         asInt64() = value.asInt64(); break;
    case TDF_ACTUAL_TYPE_UINT64:        asUInt64() = value.asUInt64(); break;
    default:
        EA_FAIL_MSG("Invalid info type in tdf member info struct");
        return false; //bad news
    }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    markSet();
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

    return true;
}



bool TdfGenericValue::set(const TdfGenericConst& ref)
{
    switch (ref.getType())
    {
    case TDF_ACTUAL_TYPE_MAP:                set(ref.asMap()); break;
    case TDF_ACTUAL_TYPE_LIST:               set(ref.asList()); break;
    case TDF_ACTUAL_TYPE_FLOAT:              set(ref.asFloat()); break;
    case TDF_ACTUAL_TYPE_ENUM:               set(ref.asEnum(), static_cast<const TypeDescriptionEnum&>(ref.getTypeDescription())); break;
    case TDF_ACTUAL_TYPE_STRING:             set(ref.asString()); break;
    case TDF_ACTUAL_TYPE_VARIABLE:           set(ref.asVariable()); break;
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:       set(ref.asGenericType()); break;
    case TDF_ACTUAL_TYPE_BITFIELD:           set(ref.asBitfield()); break;
    case TDF_ACTUAL_TYPE_BLOB:               set(ref.asBlob()); break;
    case TDF_ACTUAL_TYPE_UNION:              set(ref.asUnion()); break;
    case TDF_ACTUAL_TYPE_TDF:                set(ref.asTdf()); break;
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:        set(ref.asObjectType()); break;
    case TDF_ACTUAL_TYPE_OBJECT_ID:          set(ref.asObjectId()); break;
    case TDF_ACTUAL_TYPE_TIMEVALUE:          set(ref.asTimeValue()); break;
    case TDF_ACTUAL_TYPE_BOOL:               set(ref.asBool()); break;
    case TDF_ACTUAL_TYPE_INT8:               set(ref.asInt8()); break;
    case TDF_ACTUAL_TYPE_UINT8:              set(ref.asUInt8()); break;
    case TDF_ACTUAL_TYPE_INT16:              set(ref.asInt16()); break;
    case TDF_ACTUAL_TYPE_UINT16:             set(ref.asUInt16()); break;
    case TDF_ACTUAL_TYPE_INT32:              set(ref.asInt32()); break;
    case TDF_ACTUAL_TYPE_UINT32:             set(ref.asUInt32()); break;
    case TDF_ACTUAL_TYPE_INT64:              set(ref.asInt64()); break;
    case TDF_ACTUAL_TYPE_UINT64:             set(ref.asUInt64()); break;
    case TDF_ACTUAL_TYPE_UNKNOWN:            clear(); break;
    default:
        EA_FAIL_MSG("Invalid info type in tdf member info struct");
        return false; //bad news
    }

    return true;
}

void TdfGenericValue::set(int32_t val, const TypeDescriptionEnum& enumMap) { setType(enumMap); Value.mInt32 = static_cast<int32_t>(val); }

void TdfGenericValue::set(const TdfString& val) { setType(TypeDescriptionSelector<TdfString>::get()); *((TdfString*) Value.mString) = val; mRefPtr = (uint8_t*) (TdfString*) Value.mString; }
void TdfGenericValue::set(const char8_t* val) { setType(TypeDescriptionSelector<TdfString>::get()); *((TdfString*) Value.mString) = val; mRefPtr = (uint8_t*) (TdfString*) Value.mString;}
void TdfGenericValue::set(const ObjectType& val) { setType(TypeDescriptionSelector<ObjectType>::get()); *((ObjectType*) Value.mObjectType) = val; mRefPtr = (uint8_t*) ((ObjectType*) Value.mObjectType); }
void TdfGenericValue::set(const ObjectId& val) { setType(TypeDescriptionSelector<ObjectId>::get()); *((ObjectId*) Value.mObjectType) = val; mRefPtr = (uint8_t*) ((ObjectId*) Value.mObjectType);  }
void TdfGenericValue::set(const TimeValue& val) { setType(TypeDescriptionSelector<TimeValue>::get()); *((TimeValue*) Value.mObjectType) = val; mRefPtr = (uint8_t*) ((TimeValue*) Value.mObjectType);  }


void TdfGenericValue::set(const TdfBitfield& val)
{
   setType(val.getTypeDescription());
   *(Value.mBitfield) = val;
   mRefPtr = (uint8_t*) Value.mBitfield;
}

void TdfGenericValue::set(const Tdf& val) { set(static_cast<TdfObject&>(const_cast<Tdf&>(val))); }
void TdfGenericValue::set(const TdfUnion& val) { set(static_cast<TdfObject&>(const_cast<TdfUnion&>(val))); }
void TdfGenericValue::set(const TdfBlob& val) { set(static_cast<TdfObject&>(const_cast<TdfBlob&>(val))); }
void TdfGenericValue::set(const VariableTdfBase& val) { set(static_cast<TdfObject&>(const_cast<VariableTdfBase&>(val))); }
void TdfGenericValue::set(const GenericTdfType& val) { set(static_cast<TdfObject&>(const_cast<GenericTdfType&>(val))); }
void TdfGenericValue::set(const TdfVectorBase& val) { set(static_cast<TdfObject&>(const_cast<TdfVectorBase&>(val))); }
void TdfGenericValue::set(const TdfMapBase& val) { set(static_cast<TdfObject&>(const_cast<TdfMapBase&>(val))); }
void TdfGenericValue::set(TdfObject& val)     
{
    setType(val.getTypeDescription());
    val.copyIntoObject(asTdfObject());
}

void TdfGenericValue::setTypeInternal(const TypeDescription& newType, uint8_t* placementBuf)
{
    //If types are identical, its a no-op
    if (getTdfId() == newType.getTdfId())
        return;

    //First cleanup the old one
    switch (getType())
    {
        case TDF_ACTUAL_TYPE_TDF:                
        case TDF_ACTUAL_TYPE_UNION:              
        case TDF_ACTUAL_TYPE_MAP:                
        case TDF_ACTUAL_TYPE_LIST:               
        case TDF_ACTUAL_TYPE_BLOB:
        case TDF_ACTUAL_TYPE_VARIABLE:
        case TDF_ACTUAL_TYPE_GENERIC_TYPE:
            if (placementBuf == nullptr)               
                ((TdfObjectPtr*) Value.mObjPtr)->~TdfObjectPtr(); //place delete the object ptr
            else
                Value.mTdfObj->~TdfObject(); //otherwise we placement new'd this - destruct it
            break;
        case TDF_ACTUAL_TYPE_BITFIELD:
            if (placementBuf == nullptr)
                CORE_DELETE(&mAllocator, Value.mBitfield);
            else
                Value.mBitfield->~TdfBitfield();
            break;
        case TDF_ACTUAL_TYPE_STRING:             
            ((TdfString*) Value.mString)->~TdfString();
            break;
        case TDF_ACTUAL_TYPE_OBJECT_TYPE:      
            ((ObjectType*) Value.mObjectType)->~ObjectType();
            break;
        case TDF_ACTUAL_TYPE_OBJECT_ID: 
            ((ObjectId*) Value.mObjectId)->~ObjectId();
            break;
        case TDF_ACTUAL_TYPE_TIMEVALUE:   
            ((TimeValue*) Value.mTimeValue)->~TimeValue();
            break;
        
        default:
            //Primitive or unknown, nothing to clean.
            break;
    }

    mTypeDesc = &newType;
    Value.mUInt64 = 0;

    switch (getType())
    {
    case TDF_ACTUAL_TYPE_TDF:                
    case TDF_ACTUAL_TYPE_UNION:              
    case TDF_ACTUAL_TYPE_MAP:                
    case TDF_ACTUAL_TYPE_LIST:               
    case TDF_ACTUAL_TYPE_BLOB:
    case TDF_ACTUAL_TYPE_VARIABLE:
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
        {
            //place new the object ptr
            TdfObject* newObj = (*newType.asObjectDescription()->createFunc)(mAllocator, "TdfGenericValue Object", placementBuf);
            mRefPtr =  (uint8_t*) newObj;
            if (placementBuf == nullptr)
                new (Value.mObjPtr) TdfObjectPtr(newObj);
            else
                Value.mTdfObj = newObj;
        }
        break;
    case TDF_ACTUAL_TYPE_BITFIELD:
        Value.mBitfield = (*newType.asBitfieldDescription()->createFunc)(mAllocator, "TdfGenericValue Bitfield", placementBuf);
        mRefPtr = (uint8_t*) Value.mBitfield;
        break;
    case TDF_ACTUAL_TYPE_STRING:             
        new (Value.mString) TdfString(mAllocator);
        mRefPtr = (uint8_t*) Value.mString;
        break;
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:      
        new (Value.mObjectType) ObjectType();
        mRefPtr = (uint8_t*) Value.mObjectType;
        break;
    case TDF_ACTUAL_TYPE_OBJECT_ID: 
        new (Value.mObjectId) ObjectId();
        mRefPtr = (uint8_t*) Value.mObjectId;
        break;
    case TDF_ACTUAL_TYPE_TIMEVALUE:   
        new (Value.mTimeValue) TimeValue();
        mRefPtr = (uint8_t*) Value.mTimeValue;
        break;
    case TDF_ACTUAL_TYPE_FLOAT: mRefPtr = (uint8_t*) &Value.mFloat; break;
    case TDF_ACTUAL_TYPE_BOOL: mRefPtr = (uint8_t*) &Value.mBool; break;
    case TDF_ACTUAL_TYPE_INT8: mRefPtr = (uint8_t*) &Value.mInt8; break;
    case TDF_ACTUAL_TYPE_UINT8: mRefPtr = (uint8_t*) &Value.mUInt8; break;
    case TDF_ACTUAL_TYPE_INT16: mRefPtr = (uint8_t*) &Value.mInt16; break;
    case TDF_ACTUAL_TYPE_UINT16: mRefPtr = (uint8_t*) &Value.mUInt16; break;
    case TDF_ACTUAL_TYPE_INT32: mRefPtr = (uint8_t*) &Value.mInt32; break;
    case TDF_ACTUAL_TYPE_ENUM: mRefPtr = (uint8_t*) &Value.mInt32; break;
    case TDF_ACTUAL_TYPE_UINT32: mRefPtr = (uint8_t*) &Value.mUInt32; break;
    case TDF_ACTUAL_TYPE_INT64: mRefPtr = (uint8_t*) &Value.mInt64; break;
    case TDF_ACTUAL_TYPE_UINT64: mRefPtr = (uint8_t*) &Value.mUInt64; break;

    default:
        //Primitive or unknown, nothing to construct.
        mRefPtr = nullptr;
        break;
    }    
}

const TypeDescription& GenericTdfType::getTypeDescription() const 
{ 
    return TypeDescriptionSelector<GenericTdfType>::get(); 
}

#if EA_TDF_REGISTER_ALL
TypeRegistrationNode GenericTdfType::REGISTRATION_NODE(TypeDescriptionSelector<GenericTdfType >::get(), true);
#endif

} //namespace TDF
} //namespace EA


