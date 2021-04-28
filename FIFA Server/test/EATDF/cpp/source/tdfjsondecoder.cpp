/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class JsonDecoder

    This class provides a mechanism to decode data using the JSON format

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <EATDF/codec/tdfjsondecoder.h>
#include <EATDF/tdfvector.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfvariable.h>
#include <EATDF/tdfbase.h>
#include <EATDF/tdfbitfield.h>
#include <EATDF/tdfblob.h>
#include <EAStdC/EASprintf.h>
#include <EATDF/tdfmemberiterator.h>

namespace EA
{
namespace TDF
{
    const char8_t* JSONDECODER_TAG_COMPONENT = "component";
    const char8_t* JSONDECODER_TAG_TYPE = "type";
    const char8_t* JSONDECODER_TAG_ID = "id";
    const char8_t* JSONDECODER_TAG_BLOB_DATA_SIZE = "count";
    const char8_t* JSONDECODER_TAG_BLOB_ENC_TYPE = "enc";
    const char8_t* JSONDECODER_TAG_BLOB_OBJECT_NAME = "base64";
    const char8_t* JSONDECODER_TAG_BLOB_DATA = "data";
    const char8_t* JSONDECODER_TAG_TDF_ID = "tdfid";
    const char8_t* JSONDECODER_TAG_TDF_CLASS = "tdfclass";
    const char8_t* JSONDECODER_TAG_VALUE = "value";

typedef size_t size_type;

class IStreamToJsonReadStream : public EA::Json::IReadStream
{
    EA_TDF_NON_ASSIGNABLE(IStreamToJsonReadStream);
public:
    IStreamToJsonReadStream(EA::IO::IStream& iStream)
        : mIStream(iStream)
    {
    }

    ~IStreamToJsonReadStream() override {}

    size_type Read(void* pData, size_type nSize) override
    {
        return (size_type)mIStream.Read(pData, nSize);
    }

    EA::IO::IStream& mIStream;
};

bool JsonDecoder::readValue(EA::TDF::TdfGenericReference& ref)
{
    // All JSON values can return nullptr, which we currently handle by just using the default for the value:
    if (mReader.GetEventType() == EA::Json::kETNull)
        return true;

    switch (ref.getType())
    {
        case TDF_ACTUAL_TYPE_UNKNOWN:
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readValue: Type contains unknown member.");
            return false;        
        case TDF_ACTUAL_TYPE_MAP:
            {
                EA::TDF::TdfMapBase& map = ref.asMap();
                map.clearMap();
                return readJsonObject(ref, &JsonDecoder::readMapFields);
            }
        case TDF_ACTUAL_TYPE_UNION:
        case TDF_ACTUAL_TYPE_TDF:
            return readJsonObject(ref, &JsonDecoder::readTdfFields);
        case TDF_ACTUAL_TYPE_VARIABLE:
            return readJsonObject(ref, &JsonDecoder::readVariableFields);
        case TDF_ACTUAL_TYPE_GENERIC_TYPE:
            return readJsonObject(ref, &JsonDecoder::readGenericFields);
        case TDF_ACTUAL_TYPE_LIST:
            return readVector(ref.asList());
        case TDF_ACTUAL_TYPE_FLOAT:
            return readFloat(ref.asFloat());            
        case TDF_ACTUAL_TYPE_ENUM:
             return readEnum(ref.asEnum(), *ref.getTypeDescription().asEnumMap());
        case TDF_ACTUAL_TYPE_STRING:
            return readString(ref.asString());
        case TDF_ACTUAL_TYPE_BITFIELD:
            return readBitfield(ref.asBitfield());
        case TDF_ACTUAL_TYPE_BLOB:
            return readJsonObject(ref, &JsonDecoder::readBlobFields);
        case TDF_ACTUAL_TYPE_OBJECT_TYPE:
            return readJsonObject(ref, &JsonDecoder::readObjectTypeFields);
        case TDF_ACTUAL_TYPE_OBJECT_ID:
            return readJsonObject(ref, &JsonDecoder::readObjectIdFields);
        case TDF_ACTUAL_TYPE_TIMEVALUE:
            return readTimeValue(ref.asTimeValue());
        case TDF_ACTUAL_TYPE_BOOL:
            return readBool(ref.asBool());
        case TDF_ACTUAL_TYPE_INT8:
            return readInteger(ref.asInt8());
        case TDF_ACTUAL_TYPE_UINT8:
            return readInteger(ref.asUInt8());
        case TDF_ACTUAL_TYPE_INT16:
            return readInteger(ref.asInt16());
        case TDF_ACTUAL_TYPE_UINT16:
            return readInteger(ref.asUInt16());
        case TDF_ACTUAL_TYPE_INT32:
            return readInteger(ref.asInt32());
        case TDF_ACTUAL_TYPE_UINT32:
            return readInteger(ref.asUInt32());
        case TDF_ACTUAL_TYPE_INT64:
            return readInteger(ref.asInt64());
        case TDF_ACTUAL_TYPE_UINT64:
            return readInteger(ref.asUInt64());
    }

    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readValue: Type contains an unhandled member type.");
    return false;
}
    
bool JsonDecoder::readString(TdfString& val)
{       
    if (isBasicDataType(mReader.GetEventType()))
    {
        val.set(mReader.GetString(), (EA::TDF::TdfStringLength)mReader.GetValueLength());  // GetStringLength only works for string data types (0 otherwise)
        if (!val.isValidUtf8())
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readString: String contains non-unicode characters.");
            return false;
        }

        return true;
    }
    else if (mReader.GetEventType() == EA::Json::kETNull)
        return true;

    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readString: JSON used an unexpected type to represent the data.");
    return false;
}

bool JsonDecoder::readTimeValue(TimeValue& val)
{
    EA::Json::EventType e = mReader.GetEventType();
    if (e == EA::Json::kETString)
    {
        return val.parseTimeAllFormats(mReader.GetString());
    }
    else if (e == EA::Json::kETInteger)
    {
        val = static_cast<uint64_t>(mReader.GetInteger());    
        return true;
    }
    else if (mReader.GetEventType() == EA::Json::kETNull)
        return true;

    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readTimeValue: JSON used an unexpected type to represent the data.");
    return false;
}

bool JsonDecoder::readEnum(int32_t& val, const TypeDescriptionEnum& enumMap)
{     
    EA::Json::EventType e = mReader.GetEventType();
    bool result = false;
    if (e == EA::Json::kETString)
    {        
        result = enumMap.findByName(mReader.GetString(), val);
        if (!result)
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "ERR_INVALID_TDF_ENUM_VALUE");
    }
    else if (e == EA::Json::kETInteger)
    {
        val = static_cast<int32_t>(mReader.GetInteger());
        result = enumMap.findByValue(val, nullptr); //don't actually care about the name, only if this is a valid value
        if (!result)
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "ERR_INVALID_TDF_ENUM_VALUE");
    }
    else if (mReader.GetEventType() == EA::Json::kETNull)
    {
        return true;
    }
    else
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readEnum: JSON used an unexpected type to represent the data.");
    }

    return result;
}

bool JsonDecoder::readBitfield(TdfBitfield& val)
{
    uint32_t bits = 0;
    if (readInteger(bits))
    {
        val.setBits(bits);
        return true;
    }

    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readBitfield: JSON used an unexpected type to represent the data.");
    return false;
}

bool JsonDecoder::readJsonObject(TdfGenericReference& ref, bool (JsonDecoder::*objValueCallback)(const char8_t*, TdfGenericReference&))
{
    if (mReader.GetEventType() == EA::Json::kETNull)
    {
        return true;
    }

    bool result = true;
    if (mReader.GetEventType() != EA::Json::kETBeginObject)
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readJsonObject: JSON did not start with a begin object event to represent the data.");
        result = false;
    }


    while (result)
    {
        // Read the next element:
        mReader.Read();

        eastl::string name;
        if (mReader.GetEventType() == EA::Json::kETBeginObjectValue)
        {
            name = mReader.GetName();
            mReader.Read();
            result = (this->*objValueCallback)(name.c_str(), ref);
        }
        else if (mReader.GetEventType() == EA::Json::kETEndObject)
        {
            // Early out with success when we reach the end of the object:
            return true;
        }
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readJsonObject: JSON used an unexpected type to represent the data.");
            result = false;
        }
    }

    return result;
}

bool JsonDecoder::readObjectTypeFields(const char8_t* name, TdfGenericReference& ref)
{            
    bool result = false;
    if (EA::StdC::Strcmp(JSONDECODER_TAG_COMPONENT, name) == 0)
    {
        result = readInteger(ref.asObjectType().component);
    }
    else if (EA::StdC::Strcmp(JSONDECODER_TAG_TYPE, name) == 0)
    {         
        result = readInteger(ref.asObjectType().type);
    }
    else if (mReader.GetEventType() == EA::Json::kETNull)
    {
        return true;
    }
    else
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readObjectTypeFields: JSON used an unexpected type to represent the data.");
    }

    return result;
}

bool JsonDecoder::readObjectIdFields(const char8_t* name, TdfGenericReference& ref)
{        
    bool result = false;
    if (EA::StdC::Strcmp(JSONDECODER_TAG_TYPE, name) == 0)
    {
        TdfGenericReference typeRef(ref.asObjectId().type);
        result = readJsonObject(typeRef, &JsonDecoder::readObjectTypeFields);
    }
    else if (EA::StdC::Strcmp(JSONDECODER_TAG_ID, name) == 0)
    {         
        result = readInteger(ref.asObjectId().id);
    }
    else if (mReader.GetEventType() == EA::Json::kETNull)
    {
        return true;
    }
    else
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readObjectIdFields: JSON used an unexpected type to represent the data.");
    }

    return result;
}


bool JsonDecoder::readBlobFields(const char8_t* name, TdfGenericReference& ref)
{       
    bool result = false;

    if (EA::StdC::Strcmp(JSONDECODER_TAG_BLOB_DATA_SIZE, name) == 0)
    {
        result = (mReader.GetEventType() == EA::Json::kETInteger);
    }
    else if (EA::StdC::Strcmp(JSONDECODER_TAG_BLOB_ENC_TYPE, name) == 0)
    {
        result = (mReader.GetEventType() ==  EA::Json::kETString); //ignore the type for now.
    }
    else if (EA::StdC::Strcmp(JSONDECODER_TAG_BLOB_DATA, name) == 0)
    {   
        if (mReader.GetEventType() == EA::Json::kETString)
        {
            //decode string
            ref.asBlob().decodeBase64(mReader.GetString(), (EA::TDF::TdfSizeVal)mReader.GetStringLength());
            result = true;
        }
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readBlobFields: JSON used an unexpected type to represent the blob data.");
        }
    }
    else
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readBlobFields: JSON used an unexpected type to represent the data.");
    }

    return result;
}

// Skips the children of the current node
bool JsonDecoder::skipJsonObjectValue()
{
    EA::Json::EventType e = mReader.GetEventType();
    if (e == EA::Json::kETError || e == EA::Json::kETNone)
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].skipJsonObjectValue: JSON found an error or None type where a value should be.");
        return false;
    }

    // Increment the depthCount whenever we encounter a new object/array, and decrement the count when the object/array is ended
    // exit the loop after visiting all its children or error
    if (e == EA::Json::kETBeginObject || e == EA::Json::kETBeginObjectValue || e == EA::Json::kETBeginArray)
    {
        int32_t depthCount = 1;
            
        while (depthCount > 0 && mReader.Read()) 
        {
            e  = mReader.GetEventType();
            if (e == EA::Json::kETBeginObject || e == EA::Json::kETBeginArray)
            {
                depthCount++;
            }
            else if (e == EA::Json::kETEndObject || e == EA::Json::kETEndArray)
            {
                depthCount--;
            }
            else if (e == EA::Json::kETError)
            {
                EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].skipJsonObjectValue: JSON found an error type where a value should be.");
                return false;
            }
        }
    }
    return true;
}

bool JsonDecoder::readTdfFields(const char8_t* name, TdfGenericReference& ref)
{
    bool result = false;

    EA::TDF::TdfMemberIterator valRef(ref.asTdf());
    if (ref.asTdf().getIteratorByName(name, valRef))
    {
        result = readValue(valRef);
    }
    else
    {
        result = skipJsonObjectValue();
    }

    return result;
}

bool JsonDecoder::readSubfieldObject(TdfGenericReference& ref, uint32_t subFieldTag)
{
    bool result = false;
    EA::TDF::TdfMemberIterator valRef(ref.asTdf());
    if (ref.asTdf().getIteratorByTag(subFieldTag, valRef))
    {
        // We handle the logic differently for primitive types and complex types.
        switch (valRef.getType())
        {
        case TDF_ACTUAL_TYPE_UNKNOWN:
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readSubfieldObject: JSON  was attempting to decode into an unknown data type.");
            return false;
        case TDF_ACTUAL_TYPE_FLOAT:
        case TDF_ACTUAL_TYPE_ENUM:
        case TDF_ACTUAL_TYPE_STRING:
        case TDF_ACTUAL_TYPE_BITFIELD:
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
            {
                if (mReader.GetEventType() == EA::Json::kETBeginObject)
                {
                    mReader.Read();
                    if (mReader.GetEventType() != EA::Json::kETBeginObjectValue) // move to the value of the subfield member then decode to tdf
                    {
                        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readSubfieldObject: JSON was unable to read the object's value.");
                        return false;
                    }

                    // read now, to get ready to parse the value:
                    mReader.Read();
                }
                else
                {
                    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readSubfieldObject: JSON was unable to begin reading the object.");
                    return false;
                }
            }
            break;
        default:
            break; // do nothing for complex types
        }
        result = readValue(valRef);
    }
    else
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readSubfieldObject: Unable to find data for the subfield tag provided.");
    }
    return result;
}

bool JsonDecoder::readMapFields(const char8_t* name, TdfGenericReference& ref)
{
    bool result = false;

    //Read the next member/keyval 
    EA::TDF::TdfMapBase& map = ref.asMap();
    EA::TDF::TdfBlob blobKey(mAllocator);
    TdfGenericValue key(mAllocator);
    if (map.getKeyTypeDesc().isIntegral())
    {
        if (map.getKeyTypeDesc().isEnum())
        {
            //Convert name to enum
            int32_t val = 0;
            if (map.getKeyTypeDesc().asEnumMap()->findByName(name, val))
            {
                key.set(val, *map.getKeyTypeDesc().asEnumMap());
                result = true;
            }
            else
            {
                EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readMapFields:  Unable to lookup enum value by name.");
            }
        }
        else
        {
            char8_t* pEnd = nullptr;
            uint64_t val = EA::StdC::StrtoU64(name, &pEnd, 10);
            if (name != pEnd)
            {
                key.set(val, map.getKeyTypeDesc());
                result = true;
            }
            else
            {
                EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readMapFields:  Unable to convert integer using StrtoU64.");
            }
        }
    }
    else if (map.getKeyType() == TDF_ACTUAL_TYPE_BLOB)
    {
        // Decode the blob: 
        blobKey.decodeBase64(name, (TdfSizeVal)(strlen(name)));
        key.set(blobKey);
        result = true;
    }
    else if (map.getKeyTypeDesc().isString())
    {
        key.set(name);
        if (!key.asString().isValidUtf8())
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readMapFields:  String data was non-Unicode.");
            return false;
        }

        result = true;
    }
    else
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readMapFields: Decode context tagged as map, but key value(%s) not integer or string or blob.", name);
    }

    if (result)
    {
        TdfGenericReference value;
        // avoid add duplicated key
        if (map.insertKeyGetValue(key, value))
        {
            result = readValue(value);
        }
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readMapFields:  Unable to insert key value.");
        }
    }

    return result;
}

bool JsonDecoder::readVariableFields(const char8_t* name, TdfGenericReference& ref)
{                 
    bool result = false;
    if (EA::StdC::Strcmp(JSONDECODER_TAG_TDF_ID, name) == 0)
    {
        if (mReader.GetEventType() == EA::Json::kETInteger)
        {
            // create the tdf based on tdfid
            ref.asVariable().create((TdfId) mReader.GetInteger());
            result = true;
        }
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readVariableFields: JSON used an unexpected type to represent the data.");
        }
    }
    else if (EA::StdC::Strcmp(JSONDECODER_TAG_TDF_CLASS, name) == 0)
    {
        if (mReader.GetEventType() == EA::Json::kETString)
        {
            ref.asVariable().create(mReader.GetString());
            result = true;
        }
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readVariableFields: JSON used an unexpected type to represent the data.");
        }
    }
    else if (EA::StdC::Strcmp(JSONDECODER_TAG_VALUE, name) == 0)
    {   
        if (ref.asVariable().get() != nullptr)
        {
            TdfGenericReference nextRef(*ref.asVariable().get());
            result = readJsonObject(nextRef, &JsonDecoder::readTdfFields);
        }
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readVariableFields: JSON used an unexpected type to represent the data.");
        }
    }
    else
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readVariableFields:  Unexpected data tag name.");
    }

    return result;   
}

bool JsonDecoder::readGenericFields(const char8_t* name, TdfGenericReference& ref)
{                 
    bool result = false;
    if (EA::StdC::Strcmp(JSONDECODER_TAG_TDF_ID, name) == 0)
    {
        if (mReader.GetEventType() == EA::Json::kETInteger)
        {
            ref.asGenericType().create((TdfId) mReader.GetInteger());
            result = true;
        }            
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readGenericFields: JSON used an unexpected type to represent the data.");
        }
    }
    else if (EA::StdC::Strcmp(JSONDECODER_TAG_TDF_CLASS, name) == 0)
    {
        if (mReader.GetEventType() == EA::Json::kETString)
        {
            ref.asGenericType().create(mReader.GetString());
            result = true;
        }       
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readGenericFields: JSON used an unexpected type to represent the data.");
        }
    }
    else if (EA::StdC::Strcmp(JSONDECODER_TAG_VALUE, name) == 0)
    {   
        if (ref.asGenericType().isValid())
        {
            TdfGenericReference nextRef(ref.asGenericType().getValue());
            result = readValue(nextRef);
        }
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readGenericFields: JSON used an unexpected type to represent the data.");
        }
    }
    else
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readGenericFields:  Unexpected data tag name.");
    }

    return result;
}

bool JsonDecoder::readVector(TdfVectorBase& vector)
{
    if (mReader.GetEventType() == EA::Json::kETNull)
    {
        return true;
    }

    vector.clearVector();
    bool result = false;
    if (mReader.GetEventType() == EA::Json::kETBeginArray)
    {
        result = true;
        while (result)
        {
            // Read the element (exit if we're at the end of the list):
            mReader.Read();
            if (mReader.GetEventType() == EA::Json::kETEndArray)
                break;

            TdfGenericReference value;
            vector.pullBackRef(value);
            result = readValue(value);
        }

        // Clear the element that failed to load fully:
        if (!result)
        {
            vector.popBackRef();
        }
    }
    else
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].readVector: JSON used an unexpected type to represent the data.");
    }

    return result;
}
    
bool JsonDecoder::decode(EA::IO::IStream& iStream, Tdf& tdf, const MemberVisitOptions& options)
{
    bool result = false;
    *mErrorMessage = '\0';

    IStreamToJsonReadStream stream(iStream);
    mReader.SetStream(&stream);
    mReader.Read();
    if (mReader.GetEventType() == EA::Json::kETBeginDocument)
    {
        // Read the first element to get started
        mReader.Read();

        TdfGenericReference ref(tdf);
        if (options.subFieldTag == 0)
        {
            if (readJsonObject(ref, &JsonDecoder::readTdfFields))
            {
                // Read the final element (which should end the document)
                if (mReader.Read() == EA::Json::kETEndDocument)
                {
                    result = true;
                }
                else
                {
                    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].decode: JSON read ended before reaching the end of the doc.");
                }
            }
        }
        else
        {
            result = readSubfieldObject(ref, options.subFieldTag);
        }
    }
    else
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].decode: JSON used an unexpected type to represent the data.");
    }

    if (!result && mErrorMessage[0] == '\0')
    {
        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[JsonDecoder].decode: decoding failed near JSON input's line(%d), around value(%s).", mReader.GetLineNumber(), (mReader.GetValue() != nullptr ? mReader.GetValue() : "<<_UNKNOWN_>>"));
    }

    mReader.Reset();
    return result;
}

} //namespace TDF
} //namespace EA

