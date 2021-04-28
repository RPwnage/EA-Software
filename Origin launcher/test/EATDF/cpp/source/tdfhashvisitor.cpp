/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/


#include <EATDF/tdfhashvisitor.h>
#include <EAAssert/eaassert.h>
#include <EATDF/tdfvector.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfblob.h>
#include <EATDF/tdfunion.h>
#include <EATDF/tdfbitfield.h>
#include <EATDF/tdfvariable.h>
#include <EAStdC/EAHashCRC.h>

namespace EA
{
namespace TDF
{
 

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
 
TdfHashVisitor::TdfHashVisitor()
    : mHash(EA::StdC::kCRC32InitialValue) 
{}

bool TdfHashVisitor::postVisitValue(const TdfVisitContextConst& context) 
{
    const TdfGenericReferenceConst& ref = context.getValue();   
    switch (ref.getType())            
    {
    case TDF_ACTUAL_TYPE_BITFIELD:
        {
            uint32_t bits = ref.asBitfield().getBits();
            mHash = EA::StdC::CRC32(&bits, sizeof(bits), mHash, false);
            break;
        }
    case TDF_ACTUAL_TYPE_BOOL:
        mHash = EA::StdC::CRC32(&ref.asBool(), sizeof(ref.asBool()), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_INT8:
        mHash = EA::StdC::CRC32(&ref.asInt8(), sizeof(ref.asInt8()), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_UINT8:
        mHash = EA::StdC::CRC32(&ref.asUInt8(), sizeof(ref.asUInt8()), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_INT16:
        mHash = EA::StdC::CRC32(&ref.asInt16(), sizeof(ref.asInt16()), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_UINT16:
        mHash = EA::StdC::CRC32(&ref.asUInt16(), sizeof(ref.asUInt16()), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_INT32:
        mHash = EA::StdC::CRC32(&ref.asInt32(), sizeof(ref.asInt32()), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_UINT32:
        mHash = EA::StdC::CRC32(&ref.asUInt32(), sizeof(ref.asUInt32()), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_INT64:
        mHash = EA::StdC::CRC32(&ref.asInt64(), sizeof(ref.asInt64()), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_UINT64:
        mHash = EA::StdC::CRC32(&ref.asUInt64(), sizeof(ref.asUInt64()), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_ENUM:
        mHash = EA::StdC::CRC32(&ref.asEnum(), sizeof(ref.asEnum()), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_STRING:
        mHash = EA::StdC::CRC32(ref.asString().c_str(), ref.asString().length(), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_BLOB:    
        mHash = EA::StdC::CRC32(ref.asBlob().getData(), ref.asBlob().getCount(), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_OBJECT_ID:
        mHash = EA::StdC::CRC32(&ref.asObjectId().id, sizeof(ref.asObjectId().id), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:
        {
            uint32_t objectType = ref.asObjectType().toInt();
            mHash = EA::StdC::CRC32(&objectType, sizeof(objectType), mHash, false);
            break;
        }
    case TDF_ACTUAL_TYPE_FLOAT:
        mHash = EA::StdC::CRC32(&ref.asFloat(), sizeof(ref.asFloat()), mHash, false);
        break;
    case TDF_ACTUAL_TYPE_TIMEVALUE:
        {
            int64_t timeValue = ref.asTimeValue().getMicroSeconds();
            mHash = EA::StdC::CRC32(&timeValue, sizeof(timeValue), mHash, false);
        }
        break;
    case TDF_ACTUAL_TYPE_UNKNOWN:
        EA_FAIL_MESSAGE("TdfHash::postVisitValue: Attempted hash unknown TDF type.");
        break;
    default:
        break;
    }

    if (!context.isRoot())
    {
        switch (context.getParentType()) 
        {
        case TDF_ACTUAL_TYPE_MAP:

            if (context.getKey().isTypeString())
            {
                mHash = EA::StdC::CRC32(context.getKey().asString().c_str(), context.getKey().asString().length(), mHash, false);
            }
            else if (context.getKey().isTypeInt())
            {
                if (context.getKey().getType() != EA::TDF::TDF_ACTUAL_TYPE_ENUM)
                {
                    mHash = EA::StdC::CRC32(&context.getKey().asInt64(), sizeof(context.getKey().asInt64()), mHash, false);
                }
                else
                {
                    mHash = EA::StdC::CRC32(&context.getKey().asEnum(), sizeof(context.getKey().asEnum()), mHash, false);
                }
            }
            else if (context.getKey().getType() == TDF_ACTUAL_TYPE_BLOB)
            {
                mHash = EA::StdC::CRC32(context.getKey().asBlob().getData(), context.getKey().asBlob().getCount(), mHash, false);
            }
            else
            {
                EA_FAIL_MESSAGE("Hash context tagged as map, but key value not integer or string or blob.");
            }
            break;
        default:
            break;
        }
    }

    if (context.getMemberInfo() != nullptr)
    {
        uint32_t tag = context.getMemberInfo()->getTag();
        mHash = EA::StdC::CRC32(&tag, sizeof(tag), mHash, false);
    }

    return true;
}

} //namespace TDF

} //namespace EA

