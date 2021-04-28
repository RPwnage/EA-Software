/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class JsonEncoder

    This class provides a mechanism to encode data using the JSON encoding format

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <EATDF/codec/tdfjsonencoder.h>
#include <EATDF/tdfvector.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfvariable.h>
#include <EATDF/tdfbase.h>
#include <EATDF/tdfbitfield.h>
#include <EATDF/tdfblob.h>
#include <EAStdC/EASprintf.h>
#include <EAAssert/eaassert.h>
#include <EATDF/tdffactory.h>
#include <EATDF/tdfmemberinfo.h>

namespace EA
{
namespace TDF
{
    const static int32_t MAX_JSON_ELEMENT_LENGTH = 128;
    const char8_t* JSONENCODER_TAG_COMPONENT = "component";
    const char8_t* JSONENCODER_TAG_TYPE = "type";
    const char8_t* JSONENCODER_TAG_ID = "id";
    const char8_t* JSONENCODER_TAG_BLOB_DATA_SIZE = "count";
    const char8_t* JSONENCODER_TAG_BLOB_ENC_TYPE = "enc";
    const char8_t* JSONENCODER_TAG_BLOB_OBJECT_NAME = "base64";
    const char8_t* JSONENCODER_TAG_BLOB_DATA = "data";
    const char8_t* JSONENCODER_TAG_TDF_ID = "tdfid";
    const char8_t* JSONENCODER_TAG_TDF_CLASS = "tdfclass";
    const char8_t* JSONENCODER_TAG_VALUE = "value";

    typedef size_t size_type;

    class IStreamToJsonWriteStream : public EA::Json::IWriteStream
    {
        EA_TDF_NON_ASSIGNABLE(IStreamToJsonWriteStream);
    public:
        IStreamToJsonWriteStream(EA::IO::IStream& iStream)
            : mIStream(iStream)
        {
        }

        ~IStreamToJsonWriteStream() override {}

        bool Write(const void* pData, size_type nSize) override
        {
            return mIStream.Write(pData, nSize);
        }

        EA::IO::IStream& mIStream;
    };

#define CHECK(func) {if (func == false) return false;}

bool JsonEncoder::visitValue(const TdfVisitContextConst& context)
{
    // Write out our left hand side
    uint32_t subFieldTag = 0;
    {
        const TdfVisitContextConst* parent = context.getParent();
        if (parent != nullptr && parent->isRoot())                  // Only use the subfield tag for the root: (avoids issue where a member of the subfield uses the same tag)
            subFieldTag = context.getVisitOptions().subFieldTag;
    }

    const TdfGenericReferenceConst& ref = context.getValue();
    mStateStack[mStateDepth].preBeginPreamblePosition = mWriter.getStream().mIStream.GetPosition();
    if (!context.isRoot())
    {
        switch(context.getParentType())
        {
        case TDF_ACTUAL_TYPE_UNION:
        case TDF_ACTUAL_TYPE_TDF:
            {
                TdfType type = context.getType();
                if (type != EA::TDF::TDF_ACTUAL_TYPE_VARIABLE && type != EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE)
                {
                    char8_t elementName[MAX_JSON_ELEMENT_LENGTH];
                    convertMemberToElement(getMemNameFromMemInfo(context), elementName, sizeof(elementName));
                    if (subFieldTag == 0)
                    {
                        CHECK(mWriter.BeginObjectValue(elementName));
                    }
                    else
                    {
                        if (context.getMemberInfo()->getTag() != subFieldTag)
                        {
                            CHECK(mWriter.BeginObjectValue(elementName));
                        }
                        else
                        {
                            if (context.getParent()->isRoot() && type != TDF_ACTUAL_TYPE_TDF && type != TDF_ACTUAL_TYPE_MAP) // only print element name if the subfield member is primitive type
                            {
                                CHECK(mWriter.BeginObjectValue(elementName));
                            }
                            else if (!context.getParent()->isRoot()) // this is the case where we are visiting the children of the subfield tdf member, and the tag name happens to the same
                            {
                                CHECK(mWriter.BeginObjectValue(elementName));
                            }
                        }
                    }
                }
            }
            break;
        case TDF_ACTUAL_TYPE_MAP:
            {
                if (context.getKey().isTypeString())
                {
                    if (context.getKey().asString().isValidUtf8())
                    {
#if EA_TDF_INCLUDE_EXTENDED_TAG_INFO
                        // Navigate up the context chain until we find a valid memberInfo to check
                        const TdfVisitContextConst* parent = context.getParent();
                        while (!parent->isRoot() && parent->getMemberInfo() == nullptr)
                            parent = parent->getParent();
                        if (parent->getMemberInfo() != nullptr && parent->getMemberInfo()->printFormat == TdfMemberInfo::LOWER)
                        {
                            char8_t* strLwr = EA::StdC::Strdup(context.getKey().asString().c_str());
                            EA::StdC::Strlwr(strLwr);
                            CHECK(mWriter.BeginObjectValue(strLwr));
                            EA::StdC::Strdel(strLwr);
                        }
                        else
#endif
                        {
                            CHECK(mWriter.BeginObjectValue(context.getKey().asString().c_str()));
                        }
                    }
                    else
                    {
                        // Encode non-Unicode strings as Base64:
                        size_t encodedSize = context.getKey().asString().calcBase64EncodedSize();
                        char8_t* encodeBuf = (char8_t*)CORE_ALLOC(&mAllocator, encodedSize + 1, EA_TDF_DEFAULT_ALLOC_NAME, 0);
                        context.getKey().asString().encodeBase64(encodeBuf, encodedSize + 1);
                        CHECK(mWriter.BeginObjectValue(encodeBuf));
                        CORE_FREE(&mAllocator, encodeBuf);
                    }

                }
                else if (context.getKey().isTypeInt())
                {
                    if (context.getKey().getType() != EA::TDF::TDF_ACTUAL_TYPE_ENUM)
                    {
                        //LHS ints need to be strings
                        char keyString[32];
                        switch (context.getKey().getType())
                        {
                        case EA::TDF::TDF_ACTUAL_TYPE_INT8:
                            EA::StdC::Snprintf(keyString, sizeof(keyString), "%" PRId8, context.getKey().asInt8());
                            break;
                        case EA::TDF::TDF_ACTUAL_TYPE_INT16:
                            EA::StdC::Snprintf(keyString, sizeof(keyString), "%" PRId16, context.getKey().asInt16());
                            break;
                        case EA::TDF::TDF_ACTUAL_TYPE_INT32:
                            EA::StdC::Snprintf(keyString, sizeof(keyString), "%" PRId32, context.getKey().asInt32());
                            break;
                        case EA::TDF::TDF_ACTUAL_TYPE_INT64:
                            EA::StdC::Snprintf(keyString, sizeof(keyString), "%" PRId64, context.getKey().asInt64());
                            break;
                        case EA::TDF::TDF_ACTUAL_TYPE_UINT8:
                            EA::StdC::Snprintf(keyString, sizeof(keyString), "%" PRIu8, context.getKey().asUInt8());
                            break;
                        case EA::TDF::TDF_ACTUAL_TYPE_UINT16:
                            EA::StdC::Snprintf(keyString, sizeof(keyString), "%" PRIu16, context.getKey().asUInt16());
                            break;
                        case EA::TDF::TDF_ACTUAL_TYPE_UINT32:
                            EA::StdC::Snprintf(keyString, sizeof(keyString), "%" PRIu32, context.getKey().asUInt32());
                            break;
                        case EA::TDF::TDF_ACTUAL_TYPE_UINT64:
                            EA::StdC::Snprintf(keyString, sizeof(keyString), "%" PRIu64, context.getKey().asUInt64());
                            break;
                        default:
                            EA::StdC::Snprintf(mErrorMessage, MAX_ENCODER_ERR_MESSAGE_LEN, "[JsonEncoder].visitValue: Map key is an unsupported int type");
                            return false;
                        }
                        CHECK(mWriter.BeginObjectValue(keyString));
                    }
                    else
                    {
                        const char8_t* enumKey = nullptr;
                        if (context.getKey().getTypeDescription().asEnumMap()->findByValue(context.getKey().asInt32(), &enumKey))
                        {
                            // if enum, encode the name of the enum
                            CHECK(mWriter.BeginObjectValue(enumKey));
                        }
                        else
                        {
                            EA::StdC::Snprintf(mErrorMessage, MAX_ENCODER_ERR_MESSAGE_LEN, "[JsonEncoder].visitValue: Encode context tagged as map keyed by enums, but key value %d not a valid enum.", context.getKey().asInt32());
                            return false;
                        }
                    }
                }
                else if (context.getKey().getType() == TDF_ACTUAL_TYPE_BLOB)
                {
                    const TdfBlob& blobKey = context.getKey().asBlob();
                    size_t encodedSize = blobKey.calcBase64EncodedSize();
                    char8_t* encodeBuf = (char8_t*)CORE_ALLOC(&mAllocator, encodedSize + 1, EA_TDF_DEFAULT_ALLOC_NAME, 0);
                    blobKey.encodeBase64(encodeBuf, encodedSize + 1);
                    CHECK(mWriter.BeginObjectValue(encodeBuf));
                    CORE_FREE(&mAllocator, encodeBuf);
                }
                else 
                {
                    EA::StdC::Snprintf(mErrorMessage, MAX_ENCODER_ERR_MESSAGE_LEN, "[JsonEncoder].visitValue: Encode context tagged as map, but key value not integer or string or blob.");
                    return false;
                }
            }
            break;
        case TDF_ACTUAL_TYPE_VARIABLE:
        case TDF_ACTUAL_TYPE_GENERIC_TYPE:
            break;
        default:
            //do nothing - No LHS inside an array
            break;
        }
    }

    mStateStack[mStateDepth].postBeginPreamblePosition = mWriter.getStream().mIStream.GetPosition();

    switch (ref.getType())
    {
    case TDF_ACTUAL_TYPE_UNION:
    case TDF_ACTUAL_TYPE_MAP:
    case TDF_ACTUAL_TYPE_TDF:
        {
            if (subFieldTag == 0)
            {
                CHECK(mWriter.BeginObject());
            }
            else
            {
                if (context.getMemberInfo() != nullptr && context.getMemberInfo()->getTag() == subFieldTag)
                    break;

                CHECK(mWriter.BeginObject());
            }
            mStateStack[mStateDepth].postBeginPreamblePosition = mWriter.getStream().mIStream.GetPosition();
        }
        break;
    case TDF_ACTUAL_TYPE_LIST:
        CHECK(mWriter.BeginArray());
        mStateStack[mStateDepth].postBeginPreamblePosition = mWriter.getStream().mIStream.GetPosition();
        break;
    case TDF_ACTUAL_TYPE_VARIABLE:
        {
            //Encode information about the variable class. The child visit will take care of the rest.
            if (ref.asVariable().isValid())
            {
                CHECK(writeGenericObjValue(context));
                if (mIncludeTDFMetaData)
                {
                    const Tdf* target = ref.asVariable().get();
                    CHECK(mWriter.BeginObject());
                    CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_TDF_ID, strlen(JSONENCODER_TAG_TDF_ID)));
                    CHECK(mWriter.Integer(static_cast<int64_t>(target->getTdfId())));
                    CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_TDF_CLASS, strlen(JSONENCODER_TAG_TDF_CLASS)));
                    CHECK(mWriter.String(target->getFullClassName()));
                    //We use a stock LHS name for variable values.
                    CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_VALUE, strlen(JSONENCODER_TAG_VALUE)));
                }
            }
        }
        break;
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
        {
            //Encode information about the generic class. The child visit will take care of the rest.
            if (ref.asGenericType().isValid())
            {
                CHECK(writeGenericObjValue(context));
                if (mIncludeTDFMetaData)
                {
                    const EA::TDF::TdfGenericValue& target = ref.asGenericType().get();
                    CHECK(mWriter.BeginObject());
                    CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_TDF_ID, strlen(JSONENCODER_TAG_TDF_ID)));
                    CHECK(mWriter.Integer(static_cast<int64_t>(target.getTdfId())));
                    CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_TDF_CLASS, strlen(JSONENCODER_TAG_TDF_CLASS)));
                    CHECK(mWriter.String(target.getFullName()));
                    //We use a stock LHS name for variable values.
                    CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_VALUE, strlen(JSONENCODER_TAG_VALUE)));
                }
            }
        }
        break;
    case TDF_ACTUAL_TYPE_BITFIELD: 
        CHECK(mWriter.Integer(ref.asBitfield().getBits())); 
        break;
    case TDF_ACTUAL_TYPE_BOOL:
        CHECK(mWriter.Bool(ref.asBool())); 
        break;
    case TDF_ACTUAL_TYPE_INT8:
        if (ref.getTypeDescription().isIntegralChar() && (((char8_t)ref.asInt8()) == '\0'))
        {
            // This is a bit of a work around.  We can't fully support JSON NULLs due to our
            // inability to set objects to nullptr in TDF infrastructure.
            // Special case the null character to write out a JSON null.
            CHECK(mWriter.Null());
        }
        else
        {
            CHECK(mWriter.Integer(static_cast<int64_t>(ref.asInt8())));
        }
        break;
    case TDF_ACTUAL_TYPE_UINT8:
        CHECK(mWriter.Integer(static_cast<int64_t>(ref.asUInt8())));
        break;
    case TDF_ACTUAL_TYPE_INT16:
        CHECK(mWriter.Integer(static_cast<int64_t>(ref.asInt16())));
        break;
    case TDF_ACTUAL_TYPE_UINT16:
        CHECK(mWriter.Integer(static_cast<int64_t>(ref.asUInt16())));
        break;
    case TDF_ACTUAL_TYPE_INT32:
        CHECK(mWriter.Integer(static_cast<int64_t>(ref.asInt32())));
        break;
    case TDF_ACTUAL_TYPE_UINT32:
        CHECK(mWriter.Integer(static_cast<int64_t>(ref.asUInt32())));
        break;
    case TDF_ACTUAL_TYPE_INT64:
        CHECK(mWriter.Integer(static_cast<int64_t>(ref.asInt64())));
        break;
    case TDF_ACTUAL_TYPE_UINT64:
        CHECK(mWriter.Integer(static_cast<int64_t>(ref.asUInt64())));
        break;
    case TDF_ACTUAL_TYPE_ENUM:
        const char8_t* enumName;
        if (ref.getTypeDescription().asEnumMap()->findByValue(ref.asEnum(), &enumName)) 
        {
            //write the enum name and break out
            CHECK(mWriter.String(enumName));
        }
        else
        {
            CHECK(mWriter.Integer(static_cast<int64_t>(ref.asEnum())));
        }
        break;
    case TDF_ACTUAL_TYPE_STRING:
        if (ref.asString().isValidUtf8())
        {
#if EA_TDF_INCLUDE_EXTENDED_TAG_INFO
            if (context.getMemberInfo() != nullptr && context.getMemberInfo()->printFormat == TdfMemberInfo::LOWER)
            {
                char8_t* strLwr = EA::StdC::Strdup(ref.asString().c_str());
                EA::StdC::Strlwr(strLwr);
                CHECK(mWriter.String(strLwr, ref.asString().length()));
                EA::StdC::Strdel(strLwr);
            }
            else
#endif
            {
                CHECK(mWriter.String(ref.asString().c_str(), ref.asString().length()));
            }
        }
        else
        {
            // Encode non-Unicode strings as Base64:
            // NOTE: Unlike Blobs, we do not use a separate object for the non-Unicode strings
            size_t encodedSize = ref.asString().calcBase64EncodedSize();
            char8_t* encodeBuf = (char8_t*)CORE_ALLOC(&mAllocator, encodedSize + 1, EA_TDF_DEFAULT_ALLOC_NAME, 0);
            ref.asString().encodeBase64(encodeBuf, encodedSize + 1);
            CHECK(mWriter.String(encodeBuf));
            CORE_FREE(&mAllocator, encodeBuf);
        }
        break;
    case TDF_ACTUAL_TYPE_BLOB:
        {
            CHECK(mWriter.BeginObject());
            CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_BLOB_DATA_SIZE, strlen(JSONENCODER_TAG_BLOB_DATA_SIZE)));
            size_t encodedSize = ref.asBlob().calcBase64EncodedSize();
            CHECK(mWriter.Integer(static_cast<int64_t>(encodedSize)));
            CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_BLOB_ENC_TYPE, strlen(JSONENCODER_TAG_BLOB_ENC_TYPE)));
            CHECK(mWriter.String("base64", strlen("base64")));
            CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_BLOB_DATA, strlen(JSONENCODER_TAG_BLOB_DATA)));
            char8_t* encodeBuf = (char8_t*) CORE_ALLOC(&mAllocator, encodedSize + 1, EA_TDF_DEFAULT_ALLOC_NAME, 0);
            ref.asBlob().encodeBase64(encodeBuf, encodedSize + 1);
            CHECK(mWriter.String(encodeBuf));
            CHECK(mWriter.EndObject());
            CORE_FREE(&mAllocator, encodeBuf);
        }
        break;
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:
        CHECK(writeObjectType(ref.asObjectType()));
        break;
    case TDF_ACTUAL_TYPE_OBJECT_ID:
        CHECK(mWriter.BeginObject());
        CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_TYPE, strlen(JSONENCODER_TAG_TYPE)));
        CHECK(writeObjectType(ref.asObjectId().type));
        CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_ID, strlen(JSONENCODER_TAG_ID)));
        CHECK(mWriter.Integer(static_cast<int64_t>(ref.asObjectId().id)));
        CHECK(mWriter.EndObject());
        break;
    case TDF_ACTUAL_TYPE_FLOAT:
        CHECK(mWriter.Double(static_cast<double>(ref.asFloat())));
        break;
    case TDF_ACTUAL_TYPE_TIMEVALUE:
        CHECK(mWriter.Integer(ref.asTimeValue().getMicroSeconds()));
        break;
    case TDF_ACTUAL_TYPE_UNKNOWN:
        EA::StdC::Snprintf(mErrorMessage, MAX_ENCODER_ERR_MESSAGE_LEN, "[JsonEncoder].visitValue: Invalid info type in tdf member info struct");
        return false;
    }

    return true;
}

bool JsonEncoder::postVisitValue(const EA::TDF::TdfVisitContextConst& context)
{
    uint32_t subFieldTag = 0;
    const TdfVisitContextConst* parent = context.getParent();
    if (parent != nullptr && parent->isRoot())                  // Only use the subfield tag for the root: (avoids issue where a member of the subfield uses the same tag)
        subFieldTag = context.getVisitOptions().subFieldTag;

    const TdfGenericReferenceConst& ref = context.getValue();
    switch (ref.getType())
    {
    case TDF_ACTUAL_TYPE_UNION:
    case TDF_ACTUAL_TYPE_MAP:
    case TDF_ACTUAL_TYPE_TDF:
        {
            TdfType type = context.getType();
            if (type != EA::TDF::TDF_ACTUAL_TYPE_VARIABLE && type != EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE)
            {
                if (subFieldTag == 0)
                {
                    CHECK(mWriter.EndObject());
                }
                else
                {
                    if (context.getMemberInfo() != nullptr && context.getMemberInfo()->getTag() == subFieldTag)
                        break;

                    CHECK(mWriter.EndObject());
                }
            }
        }
        break;
    case TDF_ACTUAL_TYPE_VARIABLE:
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
        break;
    case TDF_ACTUAL_TYPE_LIST:
        CHECK(mWriter.EndArray());
        break;
    default:
        break;
    }

    if (!context.isRoot() && mIncludeTDFMetaData)
    {
        switch(context.getParentType())
        {
        case TDF_ACTUAL_TYPE_VARIABLE:
        case TDF_ACTUAL_TYPE_GENERIC_TYPE:
            {
                if (ref.isValid())
                {
                    CHECK(mWriter.EndObject());
                }
            }
            break;
        default:
            break;
        }
    }

    return true;
}

bool JsonEncoder::writeObjectType(const ObjectType& objType)
{
    CHECK(mWriter.BeginObject());
    CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_COMPONENT, strlen(JSONENCODER_TAG_COMPONENT)));
    CHECK(mWriter.Integer(static_cast<int64_t>(objType.component)));
    CHECK(mWriter.BeginObjectValue(JSONENCODER_TAG_TYPE, strlen(JSONENCODER_TAG_TYPE)));
    CHECK(mWriter.Integer(static_cast<int64_t>(objType.type)));
    CHECK(mWriter.EndObject());
    return true;
}

bool JsonEncoder::encode(EA::IO::IStream& iStream, const Tdf& tdf, const EncodeOptions* encOptions, const MemberVisitOptions& options)
{
    IStreamToJsonWriteStream stream(iStream);
    mWriter.SetStream(&stream);

    for (int32_t i = 0; i < EA::Json::JsonWriter::kFormatOptionCount; ++i)
        mWriter.SetFormatOption(i, mFormatOptions.options[i]);

    bool result = tdf.visit(*this, options);
    mWriter.Reset();

    return result;
}

bool JsonEncoder::writeGenericObjValue( const TdfVisitContextConst &context )
{
    TdfType type = context.getParentType();
    if (type == TDF_ACTUAL_TYPE_UNION || type == TDF_ACTUAL_TYPE_TDF)
    {
        char8_t elementName[MAX_JSON_ELEMENT_LENGTH];
        convertMemberToElement(getMemNameFromMemInfo(context), elementName, sizeof(elementName));
        CHECK(mWriter.BeginObjectValue(elementName));
        return true;
    }
    return true;
}

#undef CHECK

} //namespace TDF
} //namespace EA
