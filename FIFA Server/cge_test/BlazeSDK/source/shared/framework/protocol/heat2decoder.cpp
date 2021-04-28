/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Heat2Decoder

    This class provides a mechanism to decode data using the Heat encoding format as defined at
    Blaze documentation on Confluence.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "EATDF/tdfbasetypes.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/protocol/shared/heat2util.h"
#include "framework/util/shared/rawbuffer.h"


namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define GET_OPTIONAL_TDF_HEADER(tag,type) (!decodeHeader || getHeader(tag, type))
#define IS_MISSING_TDF_HEADER(tag,type) (decodeHeader && !getHeader(tag, type))
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
// IMPORTANT: Unfortunately, despite all the macroified virtual function calls to markSet() in the TdfGeneric::as*() members, 
// it only handles setting the tracking bits for *complex* members (blob/list/map/variable/generic),
// leaving all the primitive types (uint/float/timevalue/objecttype/objectid/bitset/string) to fend for themselves. Ugh!
// NOTE: Tdf::visitMembers() always explicitly sets the context 'key' to the member's index in parent TDF.
#define MARK_TDF_PRIMITIVE_MEMBER_SET() if (parentType == EA::TDF::TDF_ACTUAL_TYPE_TDF) { context.getParent()->getValue().asTdf().markMemberSet(context.getKey().asInt32(), true); }
#else
#define MARK_TDF_PRIMITIVE_MEMBER_SET()
#endif

const uint32_t Heat2Decoder::MAX_SKIP_ELEMENT_DEPTH = 50;

/*** Public Methods ******************************************************************************/
/*************************************************************************************************/
/*!
    \brief Heat2Decoder

    Construct a new heat decoder that decodes from the given buffer

    \param[in]  buffer        - the buffer to decode from
    \param[in]  decodeInPlace - flag whether items are decoded in place (when possible) pointing to the input buffer rather than copied out.
*/
/*************************************************************************************************/
Heat2Decoder::Heat2Decoder()
    : mBuffer(nullptr)
#ifndef BLAZE_CLIENT_SDK
    , mValidationResult(Blaze::ERR_OK)
#endif
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    , mOnlyDecodeChanged(false)
#endif
    , mCurrentSkipDepth(0)
    , mFatalError(false)
    , mHeat1BackCompat(false)
{
}

/*************************************************************************************************/
/*!
    \brief ~Heat2Decoder

    Destructor for Heat2Decoder class.
*/
/*************************************************************************************************/
Heat2Decoder::~Heat2Decoder()
{
}

#ifndef BLAZE_CLIENT_SDK
Blaze::BlazeRpcError
#else
Blaze::BlazeError
#endif
Heat2Decoder::decode(RawBuffer& buffer, EA::TDF::Tdf& tdf
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    , bool onlyDecodeChanged
#endif
)
{
#ifndef BLAZE_CLIENT_SDK
    // On the server, we use a config setting to enable this change.
    // We only use this value to turn back compat on, not off.  Once on, it stays on.
    if (gController != nullptr && mHeat1BackCompat == false)
        mHeat1BackCompat = gController->isHeat1BackCompatEnabled();
#endif

    setBuffer(&buffer);
    mFatalError = false;

#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    mOnlyDecodeChanged = onlyDecodeChanged;
#endif
    bool result = tdf.visit(*this);
    mBuffer = nullptr;

#ifndef BLAZE_CLIENT_SDK
    return result ? ERR_OK : mValidationResult;
#else
    return result ? ERR_OK : ERR_SYSTEM;
#endif
}

void Heat2Decoder::setBuffer(RawBuffer* buffer)
{
    mBuffer = buffer;
#ifndef BLAZE_CLIENT_SDK
    mValidationResult = Blaze::ERR_OK;
#endif
    mCurrentSkipDepth = 0;
}

bool Heat2Decoder::visitContext(const EA::TDF::TdfVisitContext& context)
{
    if (mBuffer == nullptr)
        return false;

    const EA::TDF::TdfGenericReference* refs[2];
    const EA::TDF::TdfType parentType = context.getParentType();

    uint32_t refCount = 0;
    if (parentType == EA::TDF::TDF_ACTUAL_TYPE_MAP)
        refs[refCount++] = &context.getKey(); // For maps, key/value pair is decoded in a single visitContext()
    refs[refCount++] = &context.getValue();

    // NOTE: decodeHeader is a stack var, since it only affects entities immediately owned by the collection, not their children
    bool decodeHeader = false;
    uint32_t tag = Heat2Util::TAG_UNSPECIFIED;
    const EA::TDF::TdfMemberInfo* memberInfo = Heat2Util::getContextSpecificMemberInfo(context);
    if (memberInfo != nullptr)
    {
        decodeHeader = true;
        tag = memberInfo->getTag();
    }

    for (uint32_t i = 0; i < refCount; ++i)
    {
        const EA::TDF::TdfGenericReference& ref = *refs[i];
        const EA::TDF::TdfType type = ref.getType();
        switch (type)
        {
        case EA::TDF::TDF_ACTUAL_TYPE_UNKNOWN:
            return false; // Nothing to do
        case EA::TDF::TDF_ACTUAL_TYPE_MAP:
            {
                EA::TDF::TdfMapBase& value = ref.asMap();
                if (GET_OPTIONAL_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_MAP))
                {
                    if (mBuffer->datasize() < 2)
                    {
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Map (" << tag << ") missing header.");
#endif
                        return false;
                    }

                    uint8_t* data = mBuffer->data();
                    Heat2Util::HeatType keyType = (Heat2Util::HeatType)data[0];
                    Heat2Util::HeatType valueType = (Heat2Util::HeatType)data[1];

                    // For now, we accept either the expected heat type, or the old/invalid heat type 
                    if ((keyType == Heat2Util::getCollectionHeatTypeFromTdfType(value.getKeyType(), false) || keyType == Heat2Util::getCollectionHeatTypeFromTdfType(value.getKeyType(), true)) &&
                        (valueType == Heat2Util::getCollectionHeatTypeFromTdfType(value.getValueType(), false) || valueType == Heat2Util::getCollectionHeatTypeFromTdfType(value.getValueType(), true)))
                    {
                        mBuffer->pull(2);

                        int64_t signedSize = 0;
                        if (!decodeVarsizeInteger(signedSize))
                        {
                            // Couldn't decode a valid int
                            return false;
                        }
                        size_t size = (size_t) signedSize;

                        // The minimum size of an element in map is one byte.
                        // We will not allow frame to specify more collection elements  
                        // than remaining bytes in buffer. This will
                        // prevent crashing on initializing map size when corrupted 
                        // frame is specifying large map size.
                        if (size > mBuffer->datasize())
                        {
#ifndef BLAZE_CLIENT_SDK
                            BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Map (" << tag << ") size requested exceeds buffer.");
#endif
                            return false;
                        }

                        if (
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                            !mOnlyDecodeChanged || 
#endif
                            value.mapSize() != size)
                        {
                            value.initMap(size);
                        }

                        if (!value.visitMembers(*this, context))
                            return false;

                        break; // Done
                    }

                    // Map key or value type doesn't match so skip over the entire map
                    if (!skipElement(Heat2Util::HEAT_TYPE_MAP))
                        return false;
                }
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                if (!mOnlyDecodeChanged)
#endif
                {
                    // Init the map to empty
                    value.initMap(0);
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_LIST:
            {
                EA::TDF::TdfVectorBase& value = ref.asList();
                if (GET_OPTIONAL_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_LIST))
                {
                    if (mBuffer->datasize() < 1)
                    {
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: List (" << tag << ") missing header.");
#endif
                        return false;
                    }

                    uint8_t* data = mBuffer->data();
                    Heat2Util::HeatType valueType = (Heat2Util::HeatType)data[0];

                    // For now, we accept either the expected heat type, or the old/invalid heat type 
                    if (valueType == Heat2Util::getCollectionHeatTypeFromTdfType(value.getValueType(), false) || valueType == Heat2Util::getCollectionHeatTypeFromTdfType(value.getValueType(), true))
                    {
                        mBuffer->pull(1);

                        int64_t signedSize = 0;
                        if (!decodeVarsizeInteger(signedSize))
                        {
                            // Couldn't decode a valid int
                            return false;
                        }
                        size_t size = (size_t) signedSize;

                        // The minimum size of an element in map is one byte.
                        // We will not allow frame to specify more collection elements  
                        // than remaining bytes in buffer. This will
                        // prevent crashing on initializing map size when corrupted 
                        // frame is specifying large map size.
                        if (size > mBuffer->datasize())
                        {
#ifndef BLAZE_CLIENT_SDK
                            BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: List (" << tag << ") size requested exceeds buffer.");
#endif
                            return false;
                        }

                        if (
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                            !mOnlyDecodeChanged || 
#endif
                            value.vectorSize() != size)
                        {
                            value.initVector(size);
                        }

                        if (!value.visitMembers(*this, context))
                            return false;

                        break; // Done
                    }

                    // value type doesn't match so skip over the entire list
                    if (!skipElement(Heat2Util::HEAT_TYPE_LIST))
                        return false;
                }

#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                if (!mOnlyDecodeChanged)
#endif
                {
                    // Init the vector to empty
                    value.initVector(0);
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:
            {
                float& value = ref.asFloat();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_FLOAT))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = memberInfo->memberDefault.floatDefault; // nullptr check on memberInfo is handled by IS_MISSING_TDF_HEADER 
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                //Value is encoded as a 32-bit unsigned int, which handles all our bits in correct endian-ness
                uint8_t* buffer = mBuffer->data();
                uint32_t len = (uint32_t)mBuffer->datasize();

                if (len < Heat2Util::FLOAT_SIZE)
                {
#ifndef BLAZE_CLIENT_SDK
                    BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Float (" << tag << ") missing data for value.");
#endif

#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = (memberInfo != nullptr) ? memberInfo->memberDefault.floatDefault : 0.0f;
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    
                    // Since we always encode 4 bytes for a float, if we get here it indicates that the data was corrupted. 
                    return false;
                }

                //We read this into a uint32_t first to make sure the endian-ness is correct - otherwise could just
                //read it straight into binary
                uint32_t tmpValue32 = (uint32_t) (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3];
                memcpy(&value, &tmpValue32, sizeof(uint32_t));
                MARK_TDF_PRIMITIVE_MEMBER_SET();

                mBuffer->pull(Heat2Util::FLOAT_SIZE);
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_ENUM:
            {
                int32_t& value = ref.asInt32();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = static_cast<int32_t>(memberInfo->memberDefault.intDefault.low); // nullptr check on memberInfo is handled by IS_MISSING_TDF_HEADER
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;

#ifndef BLAZE_CLIENT_SDK
                const EA::TDF::TypeDescriptionEnum* enumMap = ref.getTypeDescription().asEnumMap();
                if (enumMap != nullptr)
                {
                    // need to find value in enum map
                    // exists() is similar to findByValue but does not pass back the actual value, just true if found.
                    if (!enumMap->exists(static_cast<int32_t>(tmpValue)))
                    {
                        // in case of invalid value, stop decoding
                        char8_t buf[5];
                        if (memberInfo != nullptr)
                        {
                            Heat2Util::decodeTag(memberInfo->getTag(), buf, 5);
                            BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Error validating enum for field " << buf << " member " << memberInfo->getMemberName() << ". Unexpected value: " << tmpValue << "!");
                        }
                        else
                        {
                            BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Error validating enum type " << enumMap->fullName << ". Unexpected value: " << tmpValue << "!");
                        }
                        mValidationResult = ERR_INVALID_TDF_ENUM_VALUE;
                        return false;
                    }
                }
                else
                {
                    BLAZE_ERR_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Unknown enum.");
                    mValidationResult = ERR_INVALID_TDF_ENUM_VALUE;
                    return false;
                }
#endif
                value = static_cast<int32_t>(tmpValue);
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_STRING:
            {
                if (GET_OPTIONAL_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_STRING))
                {
                    int64_t tmpLen = 0;
                    if (!decodeVarsizeInteger(tmpLen))
                    {
                        // Couldn't decode a valid int
                        return false;
                    }
                    if (tmpLen <= 0)
                    {
                        // Length must be greater than 0 due to addition of nullptr terminator
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: String (" << tag << ") invalid size.");
#endif
                        return false;
                    }
                    size_t length = (size_t)tmpLen;
                    if (mBuffer->datasize() < length)
                    {
                        // The string is 'length' bytes but we don't have that much left in the buffer
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: String (" << tag << ") length requested exceeds buffer size.");
#endif
                        return false;
                    }
#ifndef BLAZE_CLIENT_SDK
                    if (memberInfo != nullptr)
                    {
                        size_t maxLength = memberInfo->additionalValue;
                        if ((maxLength > 0) && (maxLength < length))
                        {
                            char8_t buf[5];
                            Heat2Util::decodeTag(tag, buf, 5);
                            // input exceeds the max length defined for this EA::TDF::TdfString
                            BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Error decoding string for field " << buf << ", member " << memberInfo->getMemberName() << " as incoming string length (" 
                                << (static_cast<uint32_t>(length)) << ") exceeds max length (" << maxLength << ")!");
                            mValidationResult = ERR_TDF_STRING_TOO_LONG;
                            return false;
                        }
                    }
#endif
                    EA::TDF::TdfString& value = ref.asString();
                    // Copy the string out of the buffer.  Length includes nullptr terminator so subtract 1
                    value.set((char8_t*)mBuffer->data(), static_cast<EA::TDF::TdfStringLength>(length) - 1);

                    MARK_TDF_PRIMITIVE_MEMBER_SET();
                    mBuffer->pull(length);
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_VARIABLE:
            {
                if (GET_OPTIONAL_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_VARIABLE))
                {
                    if (mBuffer->datasize() == 0)
                    {
                        // We're at the end of the buffer mid-decode
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Variable (" << tag << ") missing data.");
#endif
                        return false;
                    }
                    uint8_t present = mBuffer->data()[0];
                    mBuffer->pull(1);
                    if (present)
                    {
                        EA::TDF::VariableTdfBase& value = ref.asVariable();
                        // The TDF is present so create and decode it
                        int64_t tempTdfId;
                        EA::TDF::TdfId tdfId;
                        if((decodeVarsizeInteger(tempTdfId)) && tempTdfId >= 0 && tempTdfId <= UINT32_MAX &&
                            (tdfId = (EA::TDF::TdfId)tempTdfId) != EA::TDF::INVALID_TDF_ID)
                        {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                            //don't create a new instance if we are decoding incrementally and
                            // the instance already exists and the tdf ids are the same
                            if (!mOnlyDecodeChanged || !value.isValid() || (value.get()->getTdfId() != tdfId))
#endif
                                value.create(tdfId);
                        }

                        if(!value.isValid())
                        {
#ifndef BLAZE_CLIENT_SDK
                            BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Variable (" << tag << ") using invalid value id " << tempTdfId << ".");
#endif
                            // undefined/unregistered variable tdf
                            // skip the TDF itself
                            if (!getStructTerminator())
                                return (false);

                            break;
                        }

                        EA::TDF::TdfGenericReference tdfRef(*value.get());
                        EA::TDF::TdfVisitContext c(context, tdfRef);
                        if (!visitContext(c))
                            return false;

                        if (!getStructTerminator())
                            return false;
                    }
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE:
            {
                if (GET_OPTIONAL_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_GENERIC_TYPE))
                {
                    if (mBuffer->datasize() == 0)
                    {
                        // We're at the end of the buffer mid-decode
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Generic (" << tag << ") missing data.");
#endif
                        return false;
                    }
                    uint8_t present = mBuffer->data()[0];
                    mBuffer->pull(1);
                    if (present)
                    {
                        EA::TDF::GenericTdfType& value = ref.asGenericType();
                        // The TDF is present so create and decode it
                        int64_t tempTdfId;
                        EA::TDF::TdfId tdfId = EA::TDF::INVALID_TDF_ID;
                        if((decodeVarsizeInteger(tempTdfId)) &&
                            (tdfId = (EA::TDF::TdfId)tempTdfId) != EA::TDF::INVALID_TDF_ID)
                        {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                            //don't create a new instance if we are decoding incrementally and
                            // the instance already exists and the tdf ids are the same
                            if (!mOnlyDecodeChanged || !value.isValid() || (value.get().getTdfId() != tdfId))
#endif
                                value.create(tdfId);
                        }

                        if (!value.isValid())
                        {
#ifndef BLAZE_CLIENT_SDK
                            BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Generic (" << tag << ") using invalid value id " << tempTdfId << ".");
#endif
                            // undefined/unregistered variable tdf
                            // skip the TDF itself
                            if (!getStructTerminator())
                                return (false);

                            break;
                        }

                        EA::TDF::TdfGenericReference tdfRef(value.get());
                        EA::TDF::TdfVisitContext c(context, tdfRef);
                        if (!visitContext(c))
                            return false;

                        if (!getStructTerminator())
                            return false;
                    }
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_BITFIELD:
            {
                EA::TDF::TdfBitfield& value = ref.asBitfield();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value.setBits(static_cast<uint32_t>(memberInfo->memberDefault.intDefault.low)); // nullptr check on memberInfo is handled by IS_MISSING_TDF_HEADER
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;

                value.setBits(static_cast<uint32_t>(tmpValue));
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_BLOB:
            {
                if (GET_OPTIONAL_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_BLOB))
                {
                    int64_t tmpLen = 0;
                    if (!decodeVarsizeInteger(tmpLen))
                    {
                        // Couldn't decode a valid int
                        return false;
                    }
                    if (tmpLen < 0)
                    {
                        // Can't have a negative length
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Blob (" << tag << ") invalid data size.");
#endif
                        return false;
                    }
                    size_t length = (size_t)tmpLen;
                    if (mBuffer->datasize() < length)
                    {
                        // The blob is 'length' bytes but we don't have that much left in the buffer
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Blob (" << tag << ") requesting more data than is in buffer.");
#endif
                        return false;
                    }
                    EA::TDF::TdfBlob& value = ref.asBlob();
                    value.setData(mBuffer->data(), (EA::TDF::TdfSizeVal)length);
                    mBuffer->pull(length);
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_UNION:
            {
                if (GET_OPTIONAL_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_UNION))
                {
                    if (mBuffer->datasize() < 1)
                    {
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Union (" << tag << ") missing data.");
#endif
                        return (false);
                    }

                    uint8_t activeMember = mBuffer->data()[0];
                    mBuffer->pull(1);
                    EA::TDF::TdfUnion& value = ref.asUnion();
                    value.switchActiveMember(activeMember);

                    if (activeMember != EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
                    {
                        if (!value.visitMembers(*this, context))
                            return false;

                        if (value.getActiveMemberIndex() == EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
                        {
                            // Failed to decode the member because the encoded member wasn't recognized by
                            // this decode likely due to mismatched TDF versions between the encode and decode
                            // sides.  Just try and skip the element and move on.
                            if (mBuffer->datasize() < Heat2Util::HEADER_SIZE)
                            {
#ifndef BLAZE_CLIENT_SDK
                                BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: Union (" << tag << ") missing data for header.");
#endif
                                return false;
                            }
                            else
                            {
                                Heat2Util::HeatType skipType
                                    = (Heat2Util::HeatType)mBuffer->data()[Heat2Util::HEADER_TYPE_OFFSET];
                                mBuffer->pull(Heat2Util::HEADER_SIZE);
                                if (!skipElement(skipType))
                                    return false;
                            }
                        }
                    }
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_TDF:
            {
                if (GET_OPTIONAL_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_TDF))
                {
                    EA::TDF::Tdf& value = ref.asTdf();

                    if (!value.visitMembers(*this, context))
                        return false;

                    // NOTE: Only read terminator if not decoding a root tdf
                    const bool decodeTerminator = (parentType != EA::TDF::TDF_ACTUAL_TYPE_UNKNOWN);
                    if (decodeTerminator)
                    {
                        getStructTerminator();
                    }
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_TYPE:
            {
                if (GET_OPTIONAL_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_OBJECT_TYPE))
                {
                    // Decode the two components of a ObjectType
                    int64_t tmpValue = 0;
                    if (!decodeVarsizeInteger(tmpValue))
                        return false;

                    if (tmpValue < 0 || tmpValue > UINT16_MAX)
                    {
                        // ComponentId must not be negative
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: ObjectType (" << tag << ") with invalid component id value.");
#endif
                        return false;
                    }
                    EA::TDF::ObjectType& value = ref.asObjectType();
                    value.component = (ComponentId)tmpValue;

                    if (!decodeVarsizeInteger(tmpValue))
                        return false;
                    if (tmpValue < 0 || tmpValue > UINT16_MAX)
                    {
                        // EntityType must not be negative
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: ObjectType (" << tag << ") with invalid EntityType value.");
#endif
                        return false;
                    }
                    value.type = (EntityType)tmpValue;
                    MARK_TDF_PRIMITIVE_MEMBER_SET();
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_ID:
            {
                if (GET_OPTIONAL_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_OBJECT_ID))
                {
                    // Decode the three components of a ObjectId
                    int64_t tmpValue = 0;
                    if (!decodeVarsizeInteger(tmpValue))
                        return false;
                    if (tmpValue < 0 || tmpValue > UINT16_MAX)
                    {
                        // ComponentId must not be negative nor greater than max
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: ObjectId (" << tag << ") with invalid component id value.");
#endif
                        return false;
                    }
                    EA::TDF::ObjectId& value = ref.asObjectId();
                    value.type.component = (ComponentId)tmpValue;
                    if (!decodeVarsizeInteger(tmpValue))
                        return false;
                    if (tmpValue < 0 || tmpValue > UINT16_MAX)
                    {
                        // EntityType must not be negative nor greater than max
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].visitContext: ObjectId (" << tag << ") with invalid EntityType value.");
#endif
                        return false;
                    }
                    value.type.type = (EntityType)tmpValue;
                    if (!decodeVarsizeInteger(tmpValue))
                        return false;
                    value.id = (EntityId)tmpValue;
                    MARK_TDF_PRIMITIVE_MEMBER_SET();
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE:
            {
                EA::TDF::TimeValue& value = ref.asTimeValue();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = static_cast<int64_t>(static_cast<uint64_t>(memberInfo->memberDefault.intDefault.high) << 32 | static_cast<uint64_t>(memberInfo->memberDefault.intDefault.low));  // nullptr check on memberInfo is handled by IS_MISSING_TDF_HEADER
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;
                value = static_cast<int64_t>(tmpValue);
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_BOOL:
            {
                bool& value = ref.asBool();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = (memberInfo->memberDefault.intDefault.low != 0); // nullptr check on memberInfo is handled by IS_MISSING_TDF_HEADER
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;
                value = (tmpValue != 0);
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT8:
            {
                int8_t& value = ref.asInt8();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = static_cast<int8_t>(memberInfo->memberDefault.intDefault.low); // nullptr check on memberInfo is handled by IS_MISSING_TDF_HEADER
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;
                value = static_cast<int8_t>(tmpValue);
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT8:
            {
                uint8_t& value = ref.asUInt8();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = static_cast<uint8_t>(memberInfo->memberDefault.intDefault.low); // nullptr check on memberInfo is handled by IS_MISSING_TDF_HEADER
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;
                value = static_cast<uint8_t>(tmpValue);
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT16:
            {
                int16_t& value = ref.asInt16();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = static_cast<int16_t>(memberInfo->memberDefault.intDefault.low); // nullptr check on memberInfo is handled by IS_MISSING_TDF_HEADER
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;
                value = static_cast<int16_t>(tmpValue);
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT16:
            {
                uint16_t& value = ref.asUInt16();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = static_cast<uint16_t>(memberInfo->memberDefault.intDefault.low);
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;
                value = static_cast<uint16_t>(tmpValue);
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT32:
            {
                int32_t& value = ref.asInt32();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = static_cast<int32_t>(memberInfo->memberDefault.intDefault.low); // nullptr check on memberInfo is handled by IS_MISSING_TDF_HEADER
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;
                value = static_cast<int32_t>(tmpValue);
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT32:
            {
                uint32_t& value = ref.asUInt32();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = static_cast<uint32_t>(memberInfo->memberDefault.intDefault.low);
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;
                value = static_cast<uint32_t>(tmpValue);
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT64:
            {
                int64_t& value = ref.asInt64();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = static_cast<int64_t>(static_cast<uint64_t>(memberInfo->memberDefault.intDefault.high) << 32 | static_cast<uint64_t>(memberInfo->memberDefault.intDefault.low));  // nullptr check on memberInfo is handled by IS_MISSING_TDF_HEADER
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;
                value = static_cast<int64_t>(tmpValue);
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT64:
            {
                uint64_t& value = ref.asUInt64();
                if (IS_MISSING_TDF_HEADER(tag, Heat2Util::HEAT_TYPE_INTEGER))
                {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
                    if (!mOnlyDecodeChanged)
#endif
                    {
                        value = static_cast<uint64_t>(static_cast<uint64_t>(memberInfo->memberDefault.intDefault.high) << 32 | static_cast<uint64_t>(memberInfo->memberDefault.intDefault.low));  // nullptr check on memberInfo is handled by IS_MISSING_TDF_HEADER
                        MARK_TDF_PRIMITIVE_MEMBER_SET();
                    }
                    break;
                }
                int64_t tmpValue = 0;
                if (!decodeVarsizeInteger(tmpValue))
                    return false;
                value = static_cast<uint64_t>(tmpValue);
                MARK_TDF_PRIMITIVE_MEMBER_SET();
            }
            break;
        }
    }

    return !mFatalError;
}


bool Heat2Decoder::getHeader(uint32_t tag, Heat2Util::HeatType type)
{
    while (mBuffer->datasize() >= Heat2Util::HEADER_SIZE)
    {
        uint8_t* buf = mBuffer->data();

        if (buf[0] == Heat2Util::ID_TERM)
            return false;

        uint32_t bufTag = static_cast<uint32_t>((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8));
        Heat2Util::HeatType bufType = (Heat2Util::HeatType)buf[Heat2Util::HEADER_TYPE_OFFSET];

        mBuffer->pull(Heat2Util::HEADER_SIZE);

        if (EA_UNLIKELY(bufType >= Heat2Util::HEAT_TYPE_MAX))
        {
#ifndef BLAZE_CLIENT_SDK
            BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].getHeader: Failed due to invalid type in buffer.  Fatal error.");
#endif
            // FATAL ERROR: Invalid type or the reserved bits are not all 0.
            // We put the Buffer back where it started (so it'll stay stuck on this invalid type), rather than advancing into unknown buffer values. 
            mBuffer->push(Heat2Util::HEADER_SIZE);
            mFatalError = true;
            return false;
        }

        if (EA_LIKELY(bufTag == tag))
        {
            // This is the tag we are looking for

            if (EA_UNLIKELY(type != bufType))
            {
#ifndef BLAZE_CLIENT_SDK
                BLAZE_TRACE_LOG(Log::FIRE, "[Heat2Decoder].getHeader: Type mismatch - "<< type <<" expected, "<< bufType <<" provided for tag " << tag <<".  Skipping.");
#endif
                // ERROR: Matching tag but mismatched type. Skip the element.
                skipElement(bufType);
                return false;
            }
            return true;
        }

        if (EA_UNLIKELY(bufTag > tag))
        {
            // Tag doesn't exist.  This is not an error condition as it just means that the
            // encoded stream didn't have this element and we'll use the default instead.
            // Push the header back on the stream since we don't actually want to consume it here.
            mBuffer->push(Heat2Util::HEADER_SIZE);
            return false;
        }

        // Unrecognized tag in the buffer so skip it
        if (EA_UNLIKELY(!skipElement(bufType)))
        {
            return false;
        }
    }

#ifndef BLAZE_CLIENT_SDK
    // 0 size, or an ID_TERM just indicates that we're at the end of the message, and no more tdf members were set. (Valid usage)
    if (mBuffer->datasize() > 0 && (mBuffer->data()[0] != Heat2Util::ID_TERM))
    {
        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].getHeader: Failed with " << mBuffer->datasize() << " data remaining.");
    }
#endif

    return false;
}

bool Heat2Decoder::decodeVarsizeInteger(int64_t& value)
{
    uint8_t* buffer = mBuffer->data();
    uint32_t len = (uint32_t)mBuffer->datasize();

    if (EA_UNLIKELY(len == 0))
    {
#ifndef BLAZE_CLIENT_SDK
        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].decodeVarsizeInteger: Failed to decode int - no data in buffer.");
#endif
        value = 0;
        return false;
    }

    uint8_t buf0 = buffer[0];
    bool neg = ((buf0 & Heat2Util::VARSIZE_NEGATIVE) != 0);

    uint64_t v = ((uint64_t)buf0 & (Heat2Util::VARSIZE_NEGATIVE -1));
    if (buf0 & Heat2Util::VARSIZE_MORE)
    {
        int32_t shift = 6;
        uint32_t idx;
        bool more = false;
        for(idx = 1; idx < (uint32_t)len; ++idx)
        {
            uint8_t bufIdx = buffer[idx];
            v |= (((uint64_t)bufIdx & (Heat2Util::VARSIZE_MORE - 1)) << shift);
            more = ((bufIdx & Heat2Util::VARSIZE_MORE) != 0);
            if (!more)
            {
                ++idx;
                break;
            }
            shift += 7;
        }
        if (EA_UNLIKELY(more))
        {
#ifndef BLAZE_CLIENT_SDK
            BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].decodeVarsizeInteger: Failed to decode int - ran out of data in buffer.");
#endif
            // We finished decoding with a MORE byte so the encoding is incomplete.
            value = 0;
            return false;
        }
        mBuffer->pull(idx);   // In the worst case, idx == len (mBuffer->datasize()), which means it won't overflow
    }
    else
    {
        mBuffer->pull(1);
    }

    if (neg)
    {
        value = v ? -(int64_t)v : INT64_MIN;
    }
    else
    {
        value = (int64_t)v;
    }
    return true;
}

bool Heat2Decoder::getStructTerminator()
{
    while (mBuffer->datasize() >= 1)
    {
        uint8_t* buf = mBuffer->data();
        if (buf[0] == Heat2Util::ID_TERM)
        {
            mBuffer->pull(1);
            return true;
        }

        if (mBuffer->datasize() < Heat2Util::HEADER_SIZE)
            break;

        Heat2Util::HeatType bufType = (Heat2Util::HeatType) buf[Heat2Util::HEADER_TYPE_OFFSET];
        mBuffer->pull(Heat2Util::HEADER_SIZE);
        if (!skipElement(bufType))
            break;
    }
#ifndef BLAZE_CLIENT_SDK
    BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].getStructTerminator: Failed with " << mBuffer->datasize() << " data remaining.");
#endif

    return false;
}

bool Heat2Decoder::skipElement(Heat2Util::HeatType type)
{
    bool rc = true;

    if (mCurrentSkipDepth < MAX_SKIP_ELEMENT_DEPTH)
    {
        ++mCurrentSkipDepth;
    }
    else
    {
#ifndef BLAZE_CLIENT_SDK
        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].skipElement: Current skip depth limit of (" << MAX_SKIP_ELEMENT_DEPTH 
            << ") exceeded while skipping element of type(" << type << "). Aborting decode!");
#endif
        return false;
    }

    switch (type)
    {
    case Heat2Util::HEAT_TYPE_INTEGER:
        {
            int64_t v = 0;
            if (!decodeVarsizeInteger(v))
                rc = false;
            break;
        }

    case Heat2Util::HEAT_TYPE_STRING:
    case Heat2Util::HEAT_TYPE_BLOB:
        {
            int64_t length = 0;
            if (!decodeVarsizeInteger(length) || (length < 0)
                || (mBuffer->datasize() < (size_t)length))
            {
                rc = false;
            }
            else
            {
                mBuffer->pull((size_t)length);
            }
            break;
        }

    case Heat2Util::HEAT_TYPE_TDF:
        {
            rc = getStructTerminator();
            break;
        }

    case Heat2Util::HEAT_TYPE_LIST:
        {
            if (mBuffer->datasize() < 1)
            {
                rc = false;
            }
            else
            {
                Heat2Util::HeatType listType = (Heat2Util::HeatType)mBuffer->data()[0];
                mBuffer->pull(1);
                int64_t length;
                if (!decodeVarsizeInteger(length) || (length < 0))
                {
                    rc = false;
                }
                else
                {
                    for(uint32_t idx = 0; (idx < length) && rc; ++idx)
                    {
                        rc = skipElement(listType);
                        if (!rc)
                            break;
                    }
                }
            }
            break;
        }

    case Heat2Util::HEAT_TYPE_MAP:
        {
            if (mBuffer->datasize() < 2)
            {
                rc = false;
            }
            else
            {
                Heat2Util::HeatType keyType = (Heat2Util::HeatType)mBuffer->data()[0];
                Heat2Util::HeatType valueType = (Heat2Util::HeatType)mBuffer->data()[1];
                mBuffer->pull(2);
                int64_t length;
                if (!decodeVarsizeInteger(length) || (length < 0))
                {
                    rc = false;
                }
                else
                {
                    for(uint32_t idx = 0; (idx < length) && rc; ++idx)
                    {
                        rc = skipElement(keyType);
                        if (rc)
                            rc = skipElement(valueType);
                        else
                            break;
                    }
                }
            }
            break;
        }

    case Heat2Util::HEAT_TYPE_UNION:
        {
            if (mBuffer->datasize() < 1)
            {
                rc = false;
            }
            else
            {
                // Grab the active member index
                uint32_t activeMember = (uint32_t)mBuffer->data()[0];
                mBuffer->pull(1);

                if (activeMember != EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
                {
                    if (mBuffer->datasize() < Heat2Util::HEADER_SIZE)
                    {
                        rc = false;
                    }
                    else
                    {
                        // Skip the union member
                        Heat2Util::HeatType unionType
                            = (Heat2Util::HeatType)mBuffer->data()[Heat2Util::HEADER_TYPE_OFFSET];
                        mBuffer->pull(Heat2Util::HEADER_SIZE);
                        rc = skipElement(unionType);
                    }
                }
            }
            break;
        }

    case Heat2Util::HEAT_TYPE_VARIABLE:
    case Heat2Util::HEAT_TYPE_GENERIC_TYPE:
        {
            if (mBuffer->datasize() < 1)
            {
                rc = false;
            }
            else
            {
                // variable/generic TDFs are terminated like a structure if present so just skip to the
                // next terminator.
                uint8_t present = mBuffer->data()[0];
                mBuffer->pull(1);
                if (present)
                {
                    // Pull the TDF ID
                    int64_t tdfId = 0;
                    rc = decodeVarsizeInteger(tdfId);
                    if (rc)
                    {
                        // Skip the TDF itself
                        rc = getStructTerminator();
                    }
                }
            }
            break;
        }

    case Heat2Util::HEAT_TYPE_OBJECT_TYPE:
    {
        int64_t v = 0;
        if (!decodeVarsizeInteger(v)) // value.component
            rc = false;
        else if (!decodeVarsizeInteger(v)) // value.type
            rc = false;
        break;
    }
    case Heat2Util::HEAT_TYPE_OBJECT_ID:
    {
        int64_t v = 0;
        if (!decodeVarsizeInteger(v)) // value.component
            rc = false;
        else if (!decodeVarsizeInteger(v)) // value.type
            rc = false;
        else if (!decodeVarsizeInteger(v)) // value.id
            rc = false;
        break;
    }

    case Heat2Util::HEAT_TYPE_FLOAT:
    {
        if (mBuffer->datasize() < Heat2Util::FLOAT_SIZE)
        {
            rc = false;
        }
        else
        {
            mBuffer->pull(Heat2Util::FLOAT_SIZE);
            rc = true;
        }
        break;
    }

    case Heat2Util::HEAT_TYPE_TIMEVALUE:
    {
        // This code should never be hit, since we encode timevalues as ints in Heat2Encoder::visitContext with TDF_ENCODE_INTEGER.
        rc = false;
        break;
    }
    case Heat2Util::HEAT_TYPE_MAX:
    default:
        rc = false;
        break;
    }

    if (mCurrentSkipDepth > 0)
        --mCurrentSkipDepth;

    if (rc == false)
    {
#ifndef BLAZE_CLIENT_SDK
        BLAZE_WARN_LOG(Log::FIRE, "[Heat2Decoder].skipElement: Failed to decode type " << type << ". Fatal error.");
#endif
        mFatalError = true;
    }

    return rc;
}

} // Blaze

