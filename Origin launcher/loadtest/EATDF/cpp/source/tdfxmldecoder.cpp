/*************************************************************************************************/
/*!
    \class XmlDecoder

    This class provides a mechanism to decode data using the XML encoding format

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <EATDF/codec/tdfxmldecoder.h>
#include <EATDF/tdfvector.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfvariable.h>
#include <EATDF/tdfbase.h>
#include <EATDF/tdfbitfield.h>
#include <EATDF/tdfblob.h>
#include <EATDF/tdfobjectid.h>
#include <EATDF/tdfunion.h>
#include <EAStdC/EASprintf.h>
#include <EATDF/tdfmemberiterator.h>
#include <EASTL/fixed_string.h>
#include <EAStdC/EACType.h> // for Isspace in isInWhiteSpace()

namespace EA
{
namespace TDF
{
    typedef eastl::fixed_string<char8_t, 256, false> EATDFixedString;

    const char8_t* XMLDECODER_COLLECTION_ELEMENT = "entry";
    const char8_t* XMLDECODER_MAP_KEY = "key";
    const char8_t* XMLDECODER_VARIABLE_TDF_NAME = "variable";
    const char8_t* XMLDECODER_TAG_TDF_ID = "tdfid";
    const char8_t* XMLDECODER_TAG_TDF_CLASS = "tdfclass";
    const char8_t* XMLDECODER_TAG_BLOB_ENC_TYPE = "enc";
    const char8_t* XMLDECODER_TAG_BLOB_ENC_COUNT = "count";
    const char8_t* XMLDECODER_TAG_BLOB_OBJECT_NAME = "base64";
    const char8_t* XMLDECODER_TAG_UNION_VALU = "valu";

    bool XmlDecoder::decode(EA::IO::IStream& iStream, EA::TDF::Tdf& tdf, const MemberVisitOptions&)
    {
        bool result = false;
        *mErrorMessage = '\0';

        EA::XML::XmlReader::ssize_t length = (EA::XML::XmlReader::ssize_t)iStream.GetSize();
        if (length == 0) // iStream is empty
            return true;

        mReader.PushInputStream(&iStream, EA::XML::kReadEncodingUTF8, nullptr, length);

        // use fixed string to cache the element name here since XmlReader's XmlTokenBuffer will potentially point to different memory location 
        // after reader reuses internal memory for reading next page of content
        EATDFixedString elementName;
        if (readSkipNewLine(true))
        {
            if (mReader.GetValueLength() > elementName.max_size()) // we have a class name larger than 256 char
            {
                mReader.Reset();
                EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].decode:  Element name larger than 256 char.");
                return false;
            }

            if (mReader.IsEmptyElement())
            {
                mReader.Reset();
                return true;
            }
            elementName = mReader.GetValue();
        }

        TdfGenericReference ref(tdf);
        if (tdf.getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_UNION)
        {
            result = readUnion(ref);
        }
        else
        {
            result = readXmlObject(elementName.c_str(), ref);
        }

        if (!result && mErrorMessage[0] == '\0')
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].decode: decoding failed near XML input's line(%d), around value(%s).", mReader.GetLineNumber(), (mReader.GetValue() != nullptr ? mReader.GetValue() : "<<_UNKNOWN_>>"));
        }

        mReader.Reset();
        return result;
    }

    bool XmlDecoder::beginValue()
    {
        bool result = false;
        if (mReader.GetNodeType() == EA::XML::XmlReader::Element)
        {
            if (!readSkipNewLine())
                return false;
        }

        if (mReader.GetNodeType() == EA::XML::XmlReader::CharacterData)
        {
            result = true;
        }
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].beginValue:  Unable to find CharacterData.");
        }

        return result;
    }

    bool XmlDecoder::endValue()
    {
        if (!readSkipNewLine())
            return false;

        if (mReader.GetNodeType() == EA::XML::XmlReader::EndElement)
            return true;
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].endValue:  Unable to find EndElement.");
            return false;
        }
    }

    template <typename T>
    bool XmlDecoder::readInteger(T& val)
    {
        if (readSkipNewLine())
        {
            if (beginValue())
            {
                val = static_cast<T>(EA::StdC::AtoU64(mReader.GetValue()));
                return endValue();
            }
        }
        return false;
    }

    bool XmlDecoder::readBool(bool& val)
    {
        if (readSkipNewLine())
        {
            if (beginValue())
            {
                if (EA::StdC::Strcmp(mReader.GetValue(), "true") == 0 
                    || EA::StdC::Strcmp(mReader.GetValue(), "1") == 0)
                    val = 1;
                else if (EA::StdC::Strcmp(mReader.GetValue(), "false") == 0 
                    || EA::StdC::Strcmp(mReader.GetValue(), "0") == 0)
                    val = 0;

                return endValue();
            }
        }
        return false;
    }

    bool XmlDecoder::readFloat(float& val)
    {
        if (readSkipNewLine())
        {
            if (beginValue())
            {
                val = EA::StdC::AtoF32(mReader.GetValue());
                return endValue();
            }
        }
        return false;
    }

    bool XmlDecoder::readValue(EA::TDF::TdfGenericReference& ref)
    {
        switch (ref.getType())
        {
        case TDF_ACTUAL_TYPE_UNKNOWN:
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readValue: Type contains unknown member.");
            return false;        
        case TDF_ACTUAL_TYPE_MAP:
            return readMapFields(ref);
        case TDF_ACTUAL_TYPE_UNION:
                return readUnion(ref);
        case TDF_ACTUAL_TYPE_TDF:
            {
                const char8_t* name = mReader.GetValue();
                return readXmlObject(name, ref);
            }
        case TDF_ACTUAL_TYPE_VARIABLE:
            return readVariableFields(ref);
        case TDF_ACTUAL_TYPE_GENERIC_TYPE:
            return readGenericFields(ref);
        case TDF_ACTUAL_TYPE_LIST:
            return readVector(ref.asList());
        case TDF_ACTUAL_TYPE_FLOAT:
            return readFloat(ref.asFloat());            
        case TDF_ACTUAL_TYPE_ENUM:
            {
                bool result = readEnum(ref.asEnum(), *ref.getTypeDescription().asEnumMap());
                if (!result)
                    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "ERR_INVALID_TDF_ENUM_VALUE");
                return result;
            }
        case TDF_ACTUAL_TYPE_STRING:
            return readString(ref.asString());
        case TDF_ACTUAL_TYPE_BITFIELD:
            return readBitfield(ref.asBitfield());
        case TDF_ACTUAL_TYPE_BLOB:
            return readBlobFields(ref.asBlob());
        case TDF_ACTUAL_TYPE_OBJECT_TYPE:
            return readObjectTypeFields(ref.asObjectType());
        case TDF_ACTUAL_TYPE_OBJECT_ID:
            return readObjectIdFields(ref.asObjectId());
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

        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readValue: Unexpected data type (%d).", ref.getType());
        return false;
    }

    bool XmlDecoder::readTdfFields(const char8_t* name, TdfGenericReference& ref)
    {
        bool result = false;

        EA::TDF::TdfMemberIterator valRef(ref.asTdf());
        if (ref.asTdf().getIteratorByName(name, valRef))
        { 
            result = readValue(valRef);
        }
        else
        {
            result = skip();
        }

        return result;
    }

    bool XmlDecoder::readUnion(TdfGenericReference& ref)
    {
        bool result = false;
        
        // skip the begin tag if it isn't for union
        EATDFixedString unionTdfName = mReader.GetValue(); // used for list of union, which union does not have member attribute
        if (!mReader.HasAttributes() || mReader.GetAttributeValue("member") == nullptr)
        {
            if (!readSkipNewLine())
                return false;

            EATDFixedString unionFieldName = mReader.GetValue();
            if (!readTdfFields(unionFieldName.c_str(), ref))
                return false;

            if (endValue()) // this endValue() will move to the end of union value
            {
                if (EA::StdC::Strcmp(unionTdfName.c_str(), mReader.GetValue()) == 0)
                {
                    result = true;
                }
            }
        }
        
        if (mReader.HasAttributes() && mReader.GetAttributeValue("member") != nullptr)
        {
            char8_t* pEnd = nullptr;
            uint32_t val = EA::StdC::StrtoU32(mReader.GetAttributeValue("member"), &pEnd, 10);
            EA::TDF::TdfUnion* target = &ref.asUnion();
            const char8_t* memberName = nullptr;
            target->getMemberNameByIndex(val, memberName);
            if (memberName != nullptr && readSkipNewLine(true))
            {
                if (!readTdfFields(memberName, ref))
                    return false;

                if (endValue() && mReader.GetNodeType() == EA::XML::XmlReader::EndElement) // this endValue() will move to the end of union value
                {
                    if (EA::StdC::Strcmp(unionTdfName.c_str(), mReader.GetValue()) == 0)
                    {
                        return true;
                    }
                }
            }
        }

        if (!result)
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readUnion: Failed to read to end of union.");

        return result;
    }

    bool XmlDecoder::readMapFields(TdfGenericReference& ref)
    {
        EA::TDF::TdfMapBase& map = ref.asMap();
        map.clearMap(); // always clear the map container before decoding

        bool result = false;
        // step into the next line if the current map is inner map. We will grab the map name from its value.
        if (EA::StdC::Stricmp(XMLDECODER_COLLECTION_ELEMENT, mReader.GetValue()) == 0)
        {
            if(!readSkipNewLine())
                return false;
        }

        // store map name
        EATDFixedString mapName = mReader.GetName();
        if (mReader.GetNodeType() == EA::XML::XmlReader::EndElement)
        {
            // the map is empty, skip it
            if (EA::StdC::Stricmp(XMLDECODER_COLLECTION_ELEMENT, mReader.GetValue()) == 0)
                return true;
        }
        else if (mReader.GetNodeType() == EA::XML::XmlReader::Element && readSkipNewLine(true))
        {
            result = false;
            do
            {
                bool innerMapTagExist = false;
                if (mReader.GetValue() != nullptr && mReader.GetValue()[0] == '\n')
                {
                    result = readSkipNewLine();
                }
                if (EA::StdC::Stricmp(XMLDECODER_COLLECTION_ELEMENT, mReader.GetValue()) != 0)
                {
                    result = readSkipNewLine();
                    if (result)
                    {
                        if (EA::StdC::Strcmp(XMLDECODER_COLLECTION_ELEMENT, mReader.GetValue()) == 0)
                            result = true;
                        else
                            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readMapFields: 'entry' field not found after newline.");
                    }

                    innerMapTagExist = true;
                    if (!result)
                        return false;
                }

                //Read the next member/keyval
                TdfGenericValue key(mAllocator);
                const char8_t* keyname = nullptr;
                if (mReader.GetAttributeCount() > 0)
                {
                    if (EA::StdC::Strcmp(XMLDECODER_MAP_KEY, mReader.GetAttributeName(0)) == 0)
                    {
                        keyname = mReader.GetAttributeValue(0);
                    }
                    else
                    {
                        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readMapFields: Map key value is not equal to '%s'.", XMLDECODER_MAP_KEY);
                        break;
                    }
                }
                else
                {
                    if (!result)
                        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readMapFields: No Attribute count was 0.  No map key.");
                    break;
                }

                if (map.getKeyTypeDesc().isIntegral())
                {
                    if (map.getKeyTypeDesc().isEnum())
                    {
                        //Convert name to enum
                        int32_t val = 0;
                        if (map.getKeyTypeDesc().asEnumMap()->findByName(keyname, val))
                        {
                            key.set(val, *map.getKeyTypeDesc().asEnumMap());
                        }
                        else
                        {
                            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readMapFields: Map key value is enum('%s'), but not found in KeyEnumMap.", keyname);
                            break;
                        }
                    }
                    else
                    {
                        char8_t* pEnd = nullptr;
                        uint64_t val = EA::StdC::StrtoU64(keyname, &pEnd, 10);
                        if (keyname != pEnd)
                        {
                            key.set(val, map.getKeyTypeDesc());
                        }
                        else
                        {
                            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readMapFields: Map key value('%s') cannot be convert to integer.", keyname);
                            break;
                        }
                    }                            
                }
                else if (map.getKeyType() == TDF_ACTUAL_TYPE_BLOB)
                {
                    EA::TDF::TdfBlob blobKey(mAllocator);
                    blobKey.setData(reinterpret_cast<const uint8_t*>(keyname), (TdfSizeVal) (strlen(keyname) + 1));
                    key.set(blobKey);
                }
                else if (map.getKeyTypeDesc().isString())
                {
                    key.set(keyname);
                    if (!key.asString().isValidUtf8())
                    {
                        EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readMapFields: Key string is not valid Utf8.");
                        return false;
                    }
                }
                else
                {
                    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readMapFields: Decode context tagged as map, but key value(%s) not integer or string or blob.", keyname);
                    break;
                }

                TdfGenericReference value;
                // avoid add duplicated key
                if (map.insertKeyGetValue(key, value))
                {
                    // if a map value is a tdf, it will have class name around the tdf. Ex: <foo><int>123</int></foo>
                    TdfType valueType = map.getValueType();
                    if (valueType == TDF_ACTUAL_TYPE_TDF)
                    {
                        if (!readSkipNewLine())
                            return false;
                    }
                    else if (valueType == TDF_ACTUAL_TYPE_LIST || valueType == TDF_ACTUAL_TYPE_MAP)
                    {
                        if (mReader.IsEmptyElement() && (EA::StdC::Strcmp(XMLDECODER_COLLECTION_ELEMENT, mReader.GetValue()) == 0))
                        {
                            result = readSkipNewLine();
                            if (EA::StdC::Strcmp(mapName.c_str(), mReader.GetValue()) == 0)
                            {
                                break;
                            }
                            else
                                continue;
                        }
                        else
                        {
                            // remove the wrapper begin element 
                            result = readSkipNewLine();
                        }
                    }

                    if (valueType == TDF_ACTUAL_TYPE_UNION)
                    {
                        if (readSkipNewLine(true) && mReader.GetNodeType() == EA::XML::XmlReader::EndElement && (EA::StdC::Strcmp(XMLDECODER_COLLECTION_ELEMENT, mReader.GetValue()) == 0))
                        {
                            // map contains empty union
                            // exit if the map member tag is reached
                            result = readSkipNewLine();
                            if (EA::StdC::Strcmp(mapName.c_str(), mReader.GetValue()) == 0)
                            {
                                break;
                            }
                            else
                            {
                                continue;
                            }
                        }
                    }
                    result = readValue(value);
                }

                // Handle empty map case
                if (mReader.GetNodeType() == EA::XML::XmlReader::EndElement && EA::StdC::Strcmp(XMLDECODER_COLLECTION_ELEMENT, mReader.GetValue()) == 0)
                {
                    result = readSkipNewLine();
                }
                else if (endValue()) // normal case
                {
                    if (innerMapTagExist)
                        result = endValue();

                    // handle entry tag
                    if (mReader.GetNodeType() == EA::XML::XmlReader::EndElement && EA::StdC::Strcmp(XMLDECODER_COLLECTION_ELEMENT, mReader.GetValue()) == 0)
                        result = readSkipNewLine();
                }
                else
                {
                    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readMapFields: Unable to find end value for map.");
                    return false;
                }

                // exit if the map member tag is reached
                if (EA::StdC::Strcmp(mapName.c_str(), mReader.GetValue()) == 0)
                {
                    break;
                }
            }
            while (result);
        }        

        result = result && (mReader.GetNodeType() == EA::XML::XmlReader::EndElement);
        if (!result && mErrorMessage[0] == '\0')
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readMapFields: Unable to find EndElement for map.");

        return result;
    }

    bool XmlDecoder::readVariableFields(TdfGenericReference& ref)
    {
        const char8_t* name = nullptr;

        bool result = false;
        if (mReader.GetNodeType() == EA::XML::XmlReader::Element)
        {
            if (EA::StdC::Strcmp(XMLDECODER_COLLECTION_ELEMENT, mReader.GetName()) == 0)
                if(!readSkipNewLine())
                    return false;

            if (mReader.GetAttributeCount() > 0)
            {
                // beginElement is 'variable', followed by 2 attributes
                if (mReader.GetAttributeCount() == 2)
                {
                    if (EA::StdC::Strcmp(XMLDECODER_TAG_TDF_CLASS, mReader.GetAttributeName(1)) == 0)
                    {
                        ref.asVariable().create(mReader.GetAttributeValue(1));
                        name = mReader.GetValue();
                        result = true;
                    }
                }
                // beginElement is map entry, followed by 3 attributes: k, tdfid, tdfclass
                else if (mReader.GetAttributeCount() == 3)
                {
                    if (EA::StdC::Strcmp(XMLDECODER_TAG_TDF_CLASS, mReader.GetAttributeName(2)) == 0)
                    {
                        ref.asVariable().create(mReader.GetAttributeValue(2));
                        name = mReader.GetValue();
                        result = true;
                    }
                }

                if (result)
                {
                    result = false;
                    TdfGenericReference nextRef(*ref.asVariable().get());
                    if (readXmlObject(name, nextRef))
                    {
                        result = (mReader.GetNodeType() == EA::XML::XmlReader::EndElement);
                        if (!result)
                            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readVariableFields: Unable to find EndElement.");
                    }
                }
                    
            }
        }
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readVariableFields: Unable to find Element.");
        }

        
        return result;
    }

    bool XmlDecoder::readGenericFields(TdfGenericReference& ref)
    {
        bool result = false;
        if (mReader.GetNodeType() == EA::XML::XmlReader::Element)
        {
            if (EA::StdC::Strcmp(XMLDECODER_COLLECTION_ELEMENT, mReader.GetName()) == 0)
                if(!readSkipNewLine())
                    return false;

            if (mReader.GetAttributeCount() > 0)
            {
                // beginElement is 'variable', followed by 2 attributes
                if (mReader.GetAttributeCount() == 2)
                {
                    if (EA::StdC::Strcmp(XMLDECODER_TAG_TDF_CLASS, mReader.GetAttributeName(1)) == 0)
                    {
                        ref.asGenericType().create(mReader.GetAttributeValue(1));
                        result = true;
                    }
                }
                // beginElement is map entry, followed by 3 attributes: k, tdfid, tdfclass
                else if (mReader.GetAttributeCount() == 3)
                {
                    if (EA::StdC::Strcmp(XMLDECODER_TAG_TDF_CLASS, mReader.GetAttributeName(2)) == 0)
                    {
                        ref.asGenericType().create(mReader.GetAttributeValue(2));
                        result = true;
                    }
                }

                if (result)
                {
                    result = false;
                    TdfGenericReference nextRef(ref.asGenericType().getValue());
                    if (readValue(nextRef))
                    {
                        result = (mReader.GetNodeType() == EA::XML::XmlReader::EndElement);
                        if (!result)
                            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readGenericFields: Unable to find EndElement.");
                    }
                }

            }
        }
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readGenericFields: Unable to find Element.");
        }

        return result;
    }

    bool XmlDecoder::readVector(TdfVectorBase& vector)
    {
        vector.clearVector();
        bool result = false;
        EATDFixedString vectorName = mReader.GetName();
        if (readSkipNewLine(true) && mReader.GetNodeType() == EA::XML::XmlReader::Element)
        {
            do
            {
                TdfGenericReference value;
                vector.pullBackRef(value);

                if (!mReader.IsEmptyElement())
                {
                    result = readValue(value);

                    if (endValue())
                    {
                        // exit if we reach the end of vector
                        if (EA::StdC::Strcmp(vectorName.c_str(), mReader.GetValue()) == 0)
                            break;
                    }
                }
                else
                {
                    // when current element is empty, we will skip it and continue to the next element
                    result = readSkipNewLine(); 
                    if (mReader.GetNodeType() == EA::XML::XmlReader::EndElement)
                    {
                        // exit if we reach the end of vector
                        if (EA::StdC::Strcmp(vectorName.c_str(), mReader.GetValue()) == 0)
                            break;
                    }
                }
            }
            while (result);
        }
        
        result = (mReader.GetNodeType() == EA::XML::XmlReader::EndElement);
        if (!result)
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readVector: Unable to find EndElement.");
        }
        return result;
    }

    bool XmlDecoder::readString(TdfString& val)
    {
        // to allow this field to include a newline, don't call readSkipNewLine()
        if (mReader.Read())
        {
            if (mReader.GetNodeType() == EA::XML::XmlReader::EndElement) // Empty string
                return true;
            if (beginValue())
            {
                val.set(mReader.GetValue(), (EA::TDF::TdfStringLength)mReader.GetValueLength());
                
                if (!val.isValidUtf8())
                {
                    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readString: Value is not valid utf8.");
                    return false;
                }

                return endValue();
            }
        }
        else
        {
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readString: Unable to Read.");
        }
        return false;
    }

    bool XmlDecoder::readTimeValue(TimeValue& val)
    {
        uint64_t microseconds;
        if (readInteger(microseconds))
        {
            val = microseconds;
            return true;
        }
        return false;
    }

    bool XmlDecoder::readEnum(int32_t& val, const TypeDescriptionEnum& enumMap)
    {
        bool result = false;
        if (beginValue())
        {
            result = enumMap.findByName(mReader.GetValue(), val);
            if (!result)
            {
                val = EA::StdC::AtoI32(mReader.GetValue());
                result = enumMap.findByValue(val, nullptr); // don't actually care about the name, only if this is a valid value
            }

            if (!result)
            {
                EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readEnum: Unable to find mapping by name or value.");
            }
        }

        return (result && endValue());
    }

    bool XmlDecoder::readBitfield(TdfBitfield& val)
    {
        uint32_t bits = 0;
        if (readInteger(bits))
        {
            val.setBits(bits);
            return true;
        }

        return false;
    }

    // This is the wrapper function for XmlReader read function.
    // Skip the \n and extra whitespace, since UTFXML does not filter it out.
    bool XmlDecoder::readSkipNewLine(bool skipErrorLog)
    {
        XML::XmlReader::NodeType prevNodeType = mReader.GetNodeType();

        bool result = mReader.Read();

        if (mReader.GetValue() != nullptr && mReader.GetValue()[0] == '\n')
        {
            result = mReader.Read();
            if (!skipErrorLog && !result && mErrorMessage[0] == '\0')
                EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readSkipNewLine: Failed to skip new line ('\n' found).");
        }
        else if ((prevNodeType == XML::XmlReader::Element || prevNodeType == XML::XmlReader::EndElement) &&
            (mReader.GetNodeType() == EA::XML::XmlReader::CharacterData))
        {
            // Skip trailing whitespace between XML element tags.
            if (isInWhiteSpace())
            {
                result = mReader.Read();
                if (!skipErrorLog && !result && mErrorMessage[0] == '\0')
                    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readSkipNewLine: Failed to skip trailing whitespace.");
            }
        }

        if (!skipErrorLog && !result && mErrorMessage[0] == '\0')
            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readSkipNewLine: Failed to skip new line.");
        return result;
    }

    bool XmlDecoder::readXmlObject(const char8_t* name, TdfGenericReference& ref)
    {
        while (!mReader.IsEof()) 
        {
            if (readSkipNewLine())
            {
                switch (mReader.GetNodeType())
                {
                case EA::XML::XmlReader::Element:
                    if (!readTdfFields(mReader.GetName(), ref))
                    {
                        if (mErrorMessage[0] == '\0')
                            EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readXmlObject:  Unable to read TdfFields for (%s).", mReader.GetName());
                        return false;
                    }
                    break;
                case EA::XML::XmlReader::EndElement:
                    if (EA::StdC::Strcmp(name, mReader.GetValue()) == 0)
                        return true;
                    break;
                default: 
                    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].readXmlObject:  Unexpected node type (%d).", mReader.GetNodeType());
                    return false;
                }
            }
            else
                return false;
        }

        return true;
    }

    bool XmlDecoder::readObjectTypeFields(ObjectType& type)
    {
        if (readSkipNewLine())
        {
            if (beginValue())
            {
                type = ObjectType::parseString(mReader.GetValue());
                return endValue();
            }
        }
        return false;
    }
    bool XmlDecoder::readObjectIdFields(ObjectId& id)
    {
        if (readSkipNewLine())
        {
            if (beginValue())
            {
                id = ObjectId::parseString(mReader.GetValue());
                return endValue();
            }
        }
        return false;
    }
    bool XmlDecoder::readBlobFields(TdfBlob& blob)
    {
        if (readSkipNewLine(true))
        {
            if (beginValue())
            {
                //decode string
                blob.decodeBase64(mReader.GetValue(), (EA::TDF::TdfSizeVal)mReader.GetValueLength());
                return endValue();
            }
        }

        return true;
    }
    
    bool XmlDecoder::skip()
    {
        if ((mReader.GetNodeType() == EA::XML::XmlReader::Element) && !mReader.IsEmptyElement())
        {
            int32_t nElementDepth = mReader.GetDepth();

            while (readSkipNewLine())
            {
                int32_t depth = mReader.GetDepth();
                if (depth == nElementDepth)
                {
                    if (mReader.GetNodeType() == EA::XML::XmlReader::EndElement)
                        return true;

                    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XmlDecoder].skip: Unable to find EndElement.");
                    return false;
                }
                
                if(depth < nElementDepth)
                {
                    EA::StdC::Snprintf(mErrorMessage, MAX_DECODER_ERR_MESSAGE_LEN, "[XMLDecoder].Skip: Depth error in XML element(%s) and value(%s).", mReader.GetName(), mReader.GetValue());
                    return false;
                }
            }
            return false;
        }

        return true;
    }

    bool XmlDecoder::isInWhiteSpace() const
    {
        if (mReader.GetValue() == nullptr)
            return false;

        for (size_t i = 0, valLen = mReader.GetValueLength(); i < valLen; ++i)
        {
            const char8_t c = mReader.GetValue()[i];
            if (!EA::StdC::Isspace(c))
            {
                return false;
            }
        }
        return true;
    }

}
}
