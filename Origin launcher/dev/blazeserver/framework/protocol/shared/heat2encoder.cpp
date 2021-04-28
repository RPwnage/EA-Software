/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Heat2Encoder

    This class provides a mechanism to encode data using the Heat encoding format as defined at
    Blaze documentation on Confluence.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "EATDF/tdfbasetypes.h"
#include "framework/protocol/shared/heat2encoder.h"
#include "framework/protocol/shared/heat2util.h"
#include "framework/util/shared/rawbuffer.h"

#ifndef BLAZE_CLIENT_SDK
#include "framework/controller/controller.h"
#endif

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#define TDF_ENCODE_INTEGER(value) \
    if (!mBuffer->acquire(headerSpace + Heat2Util::VARSIZE_MAX_ENCODE_SIZE)) \
        return false; \
    if (encodeHeader) \
        putHeader(tag, Heat2Util::HEAT_TYPE_INTEGER); \
    encodeVarsizeInteger((int64_t)value);

#define TDF_PUT_HEADER(tag,type) \
    if (encodeHeader) \
        putHeader(tag, type);

/*** Public Methods ******************************************************************************/

bool Heat2Encoder::encode(RawBuffer& buffer, const EA::TDF::Tdf& tdf
#ifndef BLAZE_CLIENT_SDK
    , const RpcProtocol::Frame* frame
#endif
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    , bool onlyEncodeChanged
#endif
)
{
#ifndef BLAZE_CLIENT_SDK
    // On the server, we use a config setting to enable this change.
    // We only use this value to turn back compat on, not off.  Once on, it stays on.
    if (gController != nullptr && mHeat1BackCompat == false)
        mHeat1BackCompat = gController->isHeat1BackCompatEnabled();
#endif

    mBuffer = &buffer;

    EA::TDF::MemberVisitOptions options;
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    options.onlyIfSet = onlyEncodeChanged; // Should this be the default?
#endif
    options.onlyIfNotDefault = mDefaultDiff;
    bool result = tdf.visit(*this, options);

    mBuffer = nullptr;

    return result;
}


bool Heat2Encoder::visitContext(const EA::TDF::TdfVisitContextConst& context)
{
    if (mBuffer == nullptr)
        return false;

    const EA::TDF::TdfGenericReferenceConst* refs[2];
    const EA::TDF::TdfType parentType = context.getParentType();

    uint32_t refCount = 0;
    if (parentType == EA::TDF::TDF_ACTUAL_TYPE_MAP)
        refs[refCount++] = &context.getKey(); // For maps, key/value pair is encoded in a single visitContext()
    refs[refCount++] = &context.getValue();

    // NOTE: encodeHeader is a stack var, since it only affects entities immeditely owned by the collection, not their children
    bool encodeHeader = false;
    size_t headerSpace = 0;
    uint32_t tag = Heat2Util::TAG_UNSPECIFIED;
    const EA::TDF::TdfMemberInfo* memberInfo = Heat2Util::getContextSpecificMemberInfo(context);
    if (memberInfo != nullptr)
    {
        encodeHeader = true;
        headerSpace = Heat2Util::HEADER_SIZE;
        tag = memberInfo->getTag();
    }

    for (uint32_t i = 0; i < refCount; ++i)
    {
        const EA::TDF::TdfGenericReferenceConst& ref = *refs[i];
        const EA::TDF::TdfType type = ref.getType();
        switch (type)
        {
        case EA::TDF::TDF_ACTUAL_TYPE_UNKNOWN:
            return false; // Nothing to do
        case EA::TDF::TDF_ACTUAL_TYPE_MAP:
            {
                const EA::TDF::TdfMapBase& value = ref.asMap();

                size_t length = value.mapSize();
                if (length == 0 && encodeHeader
    #ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    && !context.getVisitOptions().onlyIfSet
    #endif
                    )
                    return true;

                // Reserve 1 byte for the key type and 1 for the value type
                if (!mBuffer->acquire(headerSpace + 2 + Heat2Util::VARSIZE_MAX_ENCODE_SIZE))
                    return false;

                TDF_PUT_HEADER(tag, Heat2Util::HEAT_TYPE_MAP);
                uint8_t* buffer = mBuffer->tail();

                buffer[0] = (uint8_t) Heat2Util::getCollectionHeatTypeFromTdfType(value.getKeyType(), mHeat1BackCompat);
                buffer[1] = (uint8_t) Heat2Util::getCollectionHeatTypeFromTdfType(value.getValueType(), mHeat1BackCompat);
                mBuffer->put(2);

                encodeVarsizeInteger(length);

                EA::TDF::MemberVisitOptions opt(context.getVisitOptions());
    #ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                // Since changed collections are currently always serialized wholesale, 
                // we need to disable 'change encode' mode for all member encodes.
                opt.onlyIfSet = false;
    #endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
                EA::TDF::TdfVisitContextConst c(context, opt);
                if (!value.visitMembers(*this, c))
                    return false;
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_LIST:
            {
                const EA::TDF::TdfVectorBase& value = ref.asList();

                size_t length = value.vectorSize();
                if (length == 0 && encodeHeader
    #ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    && !context.getVisitOptions().onlyIfSet
    #endif
                    )
                    return true;

                // Reserve one byte of the vector type
                if (!mBuffer->acquire(headerSpace + 1 + Heat2Util::VARSIZE_MAX_ENCODE_SIZE))
                    return false;

                TDF_PUT_HEADER(tag, Heat2Util::HEAT_TYPE_LIST);
                uint8_t* buffer = mBuffer->tail();

                buffer[0] = (uint8_t) Heat2Util::getCollectionHeatTypeFromTdfType(value.getValueType(), mHeat1BackCompat);
                mBuffer->put(1);

                encodeVarsizeInteger(length);

                EA::TDF::MemberVisitOptions opt(context.getVisitOptions());
    #ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                // Since changed collections are currently always serialized wholesale, 
                // we need to disable 'change encode' mode for all member encodes.
                opt.onlyIfSet = false;
    #endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
                EA::TDF::TdfVisitContextConst c(context, opt);
                if (!value.visitMembers(*this, c))
                    return false;
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:
            {
                const float value = ref.asFloat();
                uint32_t tmpValue;
                memcpy(&tmpValue, &value, sizeof(float));

                if (!mBuffer->acquire(headerSpace + Heat2Util::FLOAT_SIZE)) 
                    return false; 

                TDF_PUT_HEADER(tag, Heat2Util::HEAT_TYPE_FLOAT);

                //Dump the 4 bytes of the float into the buffer
                uint8_t* buffer = mBuffer->tail();
                buffer[0] = (uint8_t)( tmpValue >> 24);
                buffer[1] = (uint8_t)( tmpValue >> 16);
                buffer[2] = (uint8_t)( tmpValue >> 8);
                buffer[3] = (uint8_t)tmpValue;
                mBuffer->put(Heat2Util::FLOAT_SIZE);
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_ENUM:
            {
                int32_t value = ref.asInt32();
                TDF_ENCODE_INTEGER(value); // use standard int32_t encode
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_STRING:
            {
                const EA::TDF::TdfString& value = ref.asString();
    #ifdef BLAZE_CLIENT_SDK
                // On client, don't send non-utf8 strings around.  
                if (!value.isValidUtf8())
                    return false;
    #endif

                uint32_t length = (uint32_t)value.length() + 1;
                if (!mBuffer->acquire(headerSpace + Heat2Util::VARSIZE_MAX_ENCODE_SIZE + length))
                    return false;

                TDF_PUT_HEADER(tag, Heat2Util::HEAT_TYPE_STRING);
                encodeVarsizeInteger(length);
                uint8_t* buffer = mBuffer->tail();
                memcpy(buffer, value.c_str(), length);
                mBuffer->put(length);
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_VARIABLE:
            {
                if (!mBuffer->acquire(headerSpace + 1 + Heat2Util::VARSIZE_MAX_ENCODE_SIZE))
                    return false;
                const EA::TDF::VariableTdfBase& value = ref.asVariable();
                bool valid = value.isValid();
                TDF_PUT_HEADER(tag, Heat2Util::HEAT_TYPE_VARIABLE);
                mBuffer->tail()[0] = (uint8_t)(valid ? 1 : 0);
                mBuffer->put(1);

                if (valid)
                {
    #ifndef BLAZE_CLIENT_SDK
                    // On the BlazeSDK we don't register types with the "explicit" tdfRegistration setting. 
                    if (!(value.get()->isRegisteredTdf())) // cannot encode an unregistered TDF as a variable TDF
                        return false;
    #endif
                    encodeVarsizeInteger((int64_t)value.get()->getTdfId());

                    EA::TDF::TdfGenericReferenceConst tdfRef(*value.get());
                    EA::TDF::TdfVisitContextConst c(context, tdfRef);
                    if (!visitContext(c))
                        return false;

                    // Place a struct terminator at the end of the encoding to allow for easy skipping on
                    // the decode side.
                    if (!mBuffer->acquire(1))
                        return false;
                    mBuffer->tail()[0] = Heat2Util::ID_TERM;
                    mBuffer->put(1);
                }
            }
        
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE:
            {
                if (!mBuffer->acquire(headerSpace + 1 + Heat2Util::VARSIZE_MAX_ENCODE_SIZE))
                    return false;
                const EA::TDF::GenericTdfType& value = ref.asGenericType();
                bool valid = value.isValid();
                TDF_PUT_HEADER(tag, Heat2Util::HEAT_TYPE_GENERIC_TYPE);
                mBuffer->tail()[0] = (uint8_t)(valid ? 1 : 0);
                mBuffer->put(1);

                if (valid)
                {
                    if (value.get().getTdfId() == EA::TDF::INVALID_TDF_ID) // This means that EA::TDF::TdfFactory::fixupTypes() was not called, and you're encoding a list or map. 
                        return false;

                    encodeVarsizeInteger((int64_t)value.get().getTdfId());

                    EA::TDF::TdfGenericReferenceConst tdfRef(value.get());
                    EA::TDF::TdfVisitContextConst c(context, tdfRef);
                    if (!visitContext(c))
                        return false;

                    // Place a struct terminator at the end of the encoding to allow for easy skipping on
                    // the decode side.
                    if (!mBuffer->acquire(1))
                        return false;
                    mBuffer->tail()[0] = Heat2Util::ID_TERM;
                    mBuffer->put(1);
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_BITFIELD:
            TDF_ENCODE_INTEGER(ref.asBitfield().getBits());
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_BLOB:
            {
                const EA::TDF::TdfBlob& value = ref.asBlob();
                uint32_t length = value.getCount();
                if (!mBuffer->acquire(headerSpace + Heat2Util::VARSIZE_MAX_ENCODE_SIZE + length))
                    return false;

                TDF_PUT_HEADER(tag, Heat2Util::HEAT_TYPE_BLOB);
                encodeVarsizeInteger(length);
                uint8_t* buffer = mBuffer->tail();
                memcpy(buffer, value.getData(), length);
                mBuffer->put(length);
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_UNION:
            {
                // Reserve 1 byte for the union active member index
                if (!mBuffer->acquire(headerSpace + 1))
                    return false;

                TDF_PUT_HEADER(tag, Heat2Util::HEAT_TYPE_UNION);
                uint8_t* buffer = mBuffer->tail();

                const EA::TDF::TdfUnion& value = ref.asUnion();
                buffer[0] = (uint8_t)value.getActiveMemberIndex();
                mBuffer->put(1);

                if (!value.visitMembers(*this, context))
                    return false;
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_TDF:
            {
                if (!mBuffer->acquire(headerSpace + 1))
                    return false;

                TDF_PUT_HEADER(tag, Heat2Util::HEAT_TYPE_TDF);

                const EA::TDF::Tdf& value = ref.asTdf();

                if (!value.visitMembers(*this, context))
                    return false;

                // NOTE: Only write terminator if not encoding a root tdf
                const bool encodeTerminator = (parentType != EA::TDF::TDF_ACTUAL_TYPE_UNKNOWN);
                if (encodeTerminator)
                {
                    // Place a struct terminator at the end of the encoding to allow for easy skipping on
                    // the decode side.
                    if (!mBuffer->acquire(1))
                        return false;
                    mBuffer->tail()[0] = Heat2Util::ID_TERM;
                    mBuffer->put(1);
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_TYPE:
            {
                if (!mBuffer->acquire(headerSpace + Heat2Util::VARSIZE_MAX_ENCODE_SIZE*2))
                    return false;
                TDF_PUT_HEADER(tag, Heat2Util::HEAT_TYPE_OBJECT_TYPE);
                const EA::TDF::ObjectType& value = ref.asObjectType();
                encodeVarsizeInteger((int64_t)value.component);
                encodeVarsizeInteger((int64_t)value.type);
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_ID:
            {
                if (!mBuffer->acquire(headerSpace + Heat2Util::VARSIZE_MAX_ENCODE_SIZE*3))
                    return false;
                TDF_PUT_HEADER(tag, Heat2Util::HEAT_TYPE_OBJECT_ID);
                const EA::TDF::ObjectId& value = ref.asObjectId();
                encodeVarsizeInteger((int64_t)value.type.component);
                encodeVarsizeInteger((int64_t)value.type.type);
                encodeVarsizeInteger((int64_t)value.id);
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE:
            {
                const EA::TDF::TimeValue& value = ref.asTimeValue();
                TDF_ENCODE_INTEGER(value.getMicroSeconds());
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_BOOL:
            TDF_ENCODE_INTEGER(ref.asBool() ? 1 : 0);
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT8:
            TDF_ENCODE_INTEGER(ref.asInt8());
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT8:
            TDF_ENCODE_INTEGER(ref.asUInt8());
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT16:
            TDF_ENCODE_INTEGER(ref.asInt16());
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT16:
            TDF_ENCODE_INTEGER(ref.asUInt16());
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT32:
            TDF_ENCODE_INTEGER(ref.asInt32());
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT32:
            TDF_ENCODE_INTEGER(ref.asUInt32());
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT64:
            TDF_ENCODE_INTEGER(ref.asInt64());
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT64:
            TDF_ENCODE_INTEGER(ref.asUInt64());
            break;
        }
    }

    return true;
}

bool Heat2Encoder::putHeader(uint32_t tag, Heat2Util::HeatType type)
{
    uint8_t* buf = mBuffer->acquire(Heat2Util::HEADER_SIZE);
    if (buf == nullptr)
        return false;

    buf[0] = (uint8_t)(tag >> 24);
    buf[1] = (uint8_t)(tag >> 16);
    buf[2] = (uint8_t)(tag >> 8);
    buf[Heat2Util::HEADER_TYPE_OFFSET] = (uint8_t)(type & Heat2Util::ELEMENT_TYPE_MASK);

    buf = mBuffer->put(Heat2Util::HEADER_SIZE);

    return true;
}

void Heat2Encoder::encodeVarsizeInteger(int64_t value)
{
    uint8_t* buffer = mBuffer->tail();

    if (value == 0)
    {
        buffer[0] = 0;
        mBuffer->put(1);
        return;
    }

    uint32_t oidx = 0;
    if (value < 0)
    {
        value = -value;

        // encode as a negative value
        buffer[oidx++] = (uint8_t)(value | (Heat2Util::VARSIZE_MORE | Heat2Util::VARSIZE_NEGATIVE));
    }
    else
    {
        buffer[oidx++] = (uint8_t)((value & (Heat2Util::VARSIZE_NEGATIVE - 1)) | Heat2Util::VARSIZE_MORE);
    }
    value >>= 6;

    while (value > 0)
    {
        buffer[oidx++] = (uint8_t)(value | Heat2Util::VARSIZE_MORE);
        value >>= 7;
    }
    buffer[oidx-1] &= ~Heat2Util::VARSIZE_MORE;

    mBuffer->put(oidx);
}

} // Blaze

