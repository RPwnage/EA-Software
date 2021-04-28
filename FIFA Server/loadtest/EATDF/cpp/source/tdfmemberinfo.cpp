
/*! *********************************************************************************************/
/*!
    \file   tdfmemberinfo.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include <EATDF/tdfmemberinfo.h>
#include <EATDF/tdfbasetypes.h>
#include <EATDF/tdfobjectid.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfvector.h>
#include <EATDF/tdfvariable.h>
#include <EATDF/tdfblob.h>
#include <EATDF/tdfunion.h>

#include <EAStdC/EAString.h>

namespace EA
{
namespace TDF
{
 

bool TdfMemberInfo::getDefaultValue(TdfGenericValue& value) const
{
    const TypeDescription* typeDescTemp = getTypeDescription();
    switch (typeDescTemp->getTdfType())
    {
    case TDF_ACTUAL_TYPE_FLOAT:
        value.set((float) memberDefault.floatDefault);
        break;
    case TDF_ACTUAL_TYPE_ENUM:
        value.set(static_cast<int32_t>(memberDefault.intDefault.low), *typeDescTemp->asEnumMap());
        break;
    case TDF_ACTUAL_TYPE_STRING:
        value.set((const char8_t*) memberDefault.stringDefault);
        break;
    case TDF_ACTUAL_TYPE_TIMEVALUE:
        value.set(TimeValue(static_cast<int64_t>(((uint64_t)memberDefault.intDefault.high << 32) | memberDefault.intDefault.low)));
        break;
    case TDF_ACTUAL_TYPE_BOOL:
        value.set(memberDefault.intDefault.low != 0);
        break;
    case TDF_ACTUAL_TYPE_INT8:
        value.set(static_cast<const int8_t>(memberDefault.intDefault.low));
        break;
    case TDF_ACTUAL_TYPE_UINT8:
        value.set(static_cast<const uint8_t>(memberDefault.intDefault.low));
        break;
    case TDF_ACTUAL_TYPE_INT16:
        value.set(static_cast<const int16_t>(memberDefault.intDefault.low));
        break;
    case TDF_ACTUAL_TYPE_UINT16:
        value.set(static_cast<const uint16_t>(memberDefault.intDefault.low));
        break;
    case TDF_ACTUAL_TYPE_INT32:
        value.set(static_cast<const int32_t>(memberDefault.intDefault.low));
        break;
    case TDF_ACTUAL_TYPE_UINT32:
        value.set(static_cast<const uint32_t>(memberDefault.intDefault.low));
        break;
    case TDF_ACTUAL_TYPE_INT64:
        value.set(static_cast<const int64_t>(((uint64_t)memberDefault.intDefault.high << 32) | memberDefault.intDefault.low));
        break;
    case TDF_ACTUAL_TYPE_UINT64:
        value.set(static_cast<const uint64_t>(((uint64_t)memberDefault.intDefault.high << 32) | memberDefault.intDefault.low));
        break;
    case TDF_ACTUAL_TYPE_MAP:
    case TDF_ACTUAL_TYPE_LIST:
    case TDF_ACTUAL_TYPE_VARIABLE:
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
    case TDF_ACTUAL_TYPE_BITFIELD:
    case TDF_ACTUAL_TYPE_BLOB:
    case TDF_ACTUAL_TYPE_UNION:
    case TDF_ACTUAL_TYPE_TDF:
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:
    case TDF_ACTUAL_TYPE_OBJECT_ID:

    default:
        return false;
    }

    return true;
}


bool TdfMemberInfo::equalsDefaultValue(const TdfGenericConst& value) const
{
    if (value.getTdfId() != getTdfId())
        return false; 

    switch (getType())
    {
    case TDF_ACTUAL_TYPE_FLOAT:
        return value.asFloat() == (float) memberDefault.floatDefault;
    case TDF_ACTUAL_TYPE_ENUM:
        return value.asEnum() == static_cast<int32_t>(memberDefault.intDefault.low);
    case TDF_ACTUAL_TYPE_STRING:
        return EA::StdC::Strcmp(value.asString().c_str(), (const char8_t*) (memberDefault.stringDefault != nullptr) ? memberDefault.stringDefault : "") == 0;
    case TDF_ACTUAL_TYPE_TIMEVALUE:
        return value.asTimeValue().getMicroSeconds() == static_cast<int64_t>(((uint64_t)memberDefault.intDefault.high << 32) | memberDefault.intDefault.low);
    case TDF_ACTUAL_TYPE_BOOL:
        return value.asBool() == (memberDefault.intDefault.low != 0);
    case TDF_ACTUAL_TYPE_INT8:
        return value.asInt8() == static_cast<const int8_t>(memberDefault.intDefault.low);
    case TDF_ACTUAL_TYPE_UINT8:
        return value.asUInt8() == static_cast<const uint8_t>(memberDefault.intDefault.low);
    case TDF_ACTUAL_TYPE_INT16:
        return value.asInt16() == static_cast<const int16_t>(memberDefault.intDefault.low);
    case TDF_ACTUAL_TYPE_UINT16:
        return value.asUInt16() == static_cast<const uint16_t>(memberDefault.intDefault.low);
    case TDF_ACTUAL_TYPE_INT32:
        return value.asInt32() == static_cast<const int32_t>(memberDefault.intDefault.low);
    case TDF_ACTUAL_TYPE_UINT32:
        return value.asUInt32() == static_cast<const uint32_t>(memberDefault.intDefault.low);
    case TDF_ACTUAL_TYPE_INT64:
        return value.asInt64() == static_cast<const int64_t>(((uint64_t)memberDefault.intDefault.high << 32) | memberDefault.intDefault.low);
    case TDF_ACTUAL_TYPE_UINT64:
        return value.asUInt64() == static_cast<const uint64_t>(((uint64_t)memberDefault.intDefault.high << 32) | memberDefault.intDefault.low);
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
        return value.asGenericType().get().getType() == EA::TDF::TDF_ACTUAL_TYPE_UNKNOWN;
    case TDF_ACTUAL_TYPE_MAP:
        return value.asMap().mapSize() == 0;
    case TDF_ACTUAL_TYPE_LIST:
        return value.asList().vectorSize() == 0;
    case TDF_ACTUAL_TYPE_VARIABLE:
        return !(value.asVariable().isValid());
    case TDF_ACTUAL_TYPE_BITFIELD:
        return value.asBitfield().getBits() == 0;
    case TDF_ACTUAL_TYPE_BLOB:
        return value.asBlob().getCount() == 0;
    case TDF_ACTUAL_TYPE_UNION:
        return value.asUnion().getActiveMemberIndex() == TdfUnion::INVALID_MEMBER_INDEX;
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:
        return value.asObjectType().toInt() == 0;
    case TDF_ACTUAL_TYPE_OBJECT_ID:
        return value.asObjectId() == OBJECT_ID_INVALID;
    case TDF_ACTUAL_TYPE_TDF:
    default:
        return false;
    }
}

} //namespace TDF
} //namespace EA

