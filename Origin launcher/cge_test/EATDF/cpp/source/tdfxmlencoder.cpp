
/*************************************************************************************************/
/*!
    \class XmlEncoder

    This class provides a mechanism to encode data using the XML encoding format

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <EATDF/codec/tdfxmlencoder.h>
#include <EATDF/tdfvector.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfunion.h>
#include <EATDF/tdfvariable.h>
#include <EATDF/tdfbase.h>
#include <EATDF/tdfbitfield.h>
#include <EATDF/tdfblob.h>
#include <EAStdC/EASprintf.h>
#include <EAAssert/eaassert.h>
#include <EAStdC/EACType.h>
#include <EAStdC/EAString.h>

namespace EA
{
namespace TDF
{
    const char8_t* XMLENCODER_COLLECTION_ELEMENT = "entry";
    const char8_t* XMLENCODER_MAP_KEY = "key";
    const char8_t* XMLENCODER_VARIABLE_TDF_NAME = "variable";
    const char8_t* XMLENCODER_TAG_TDF_ID = "tdfid";
    const char8_t* XMLENCODER_TAG_TDF_CLASS = "tdfclass";
    const char8_t* XMLENCODER_TAG_BLOB_ENC_TYPE = "enc";
    const char8_t* XMLENCODER_TAG_BLOB_ENC_COUNT = "count";
    const char8_t* XMLENCODER_TAG_BLOB_OBJECT_NAME = "base64";
    const char8_t* XMLENCODER_TAG_UNION_VALU = "valu";

    const int32_t RESPONSE_WORD_LENGTH = 8;
    const int32_t IntegerBufferSize = 22;
    const int32_t FloatBufferSize = 64;

    // convert member name to lower case and truncate "response" keyword
    const char8_t* XmlEncoder::getSanitizedMemberName(const char8_t* objectName, char8_t elementName[MAX_XML_ELEMENT_LENGTH])
    {
        bool useRawNames = (mEncOptions != nullptr && mEncOptions->format == EA::TDF::EncodeOptions::FORMAT_USE_TDF_RAW_NAMES);

        if (useRawNames)
        {
            return objectName;
        }
        else
        {
            // Convert the objectName to lowercase, copying for us as well.
            uint32_t len = 0;                           // Will hold string length after loop finishes
            for (; len < MAX_XML_ELEMENT_LENGTH-1 ; ++len)
            {
                elementName[len] = static_cast<char8_t>(tolower(objectName[len]));
                if (elementName[len] == '\0' )
                {
                    break;      // All done
                }
            }
            elementName[MAX_XML_ELEMENT_LENGTH-1] = '\0';  // Ensure long element names are truncated

            // The element name probably ends in "response" (i.e. DummyResponse).  We don't want
            // the response part to be encoded in the XML.
            if (len > RESPONSE_WORD_LENGTH)              // 8 characters in "response"
            {
                char8_t* response = &(elementName[len-RESPONSE_WORD_LENGTH]);
                if (strcmp(response, "response") == 0 )
                {
                    *response = '\0';   // Remove the response
                }
            }
            return elementName;
        }
    }

    bool XmlEncoder::findAncestorMemberName(char8_t* name, size_t size, const EA::TDF::TdfVisitContextConst &context)
    {
        const EA::TDF::TdfVisitContextConst* parent = context.getParent();
        const TdfMemberInfo* memberInfo = parent->getMemberInfo();
        if (memberInfo == nullptr)
        {
            do 
            {
                parent = parent->getParent();
                if (parent == nullptr)
                    return false;
                memberInfo = parent->getMemberInfo();                                
            } while (memberInfo == nullptr);
        }
        convertMemberToElement(getNameFromMemInfo(memberInfo), name, size);

        return true;
    }

    bool findAncestorType(TdfType &type, const EA::TDF::TdfVisitContextConst &context)
    {
        const EA::TDF::TdfVisitContextConst* parent = context.getParent();
        if (!parent->isRoot())
        {
            do 
            {
                parent = parent->getParent();
                if (parent == nullptr)
                    return false;
                type = parent->getType();
            } while (!parent->isRoot());
        }
        return true;
    }

    const char* XmlEncoder::getNameFromMemInfo(const TdfMemberInfo* memberInfo )
    {
        return memberInfo->getMemberName();
    }

    bool XmlEncoder::getMemNameForGeneric(const EA::TDF::TdfVisitContextConst &context, char8_t memberName[MAX_XML_ELEMENT_LENGTH])
    {
        if (context.getMemberInfo() != nullptr)
        {
            convertMemberToElement(getNameFromMemInfo(context.getMemberInfo()), memberName, MAX_XML_ELEMENT_LENGTH);
            return true;
        }
        else if (findAncestorMemberName(memberName, MAX_XML_ELEMENT_LENGTH, context))
            return true;
        else
            return false;
    }

    bool XmlEncoder::visitValue(const EA::TDF::TdfVisitContextConst& context)
    {
        // Write out our left hand side
        const TdfGenericReferenceConst& ref = context.getValue();
        mStateStack[mStateDepth].preBeginPreamblePosition = mWriter.getOutputStream().GetPosition();
        if (!context.isRoot())
        {
            switch(context.getParentType())
            {
            case TDF_ACTUAL_TYPE_UNION:
                break;
            case TDF_ACTUAL_TYPE_TDF:
                {
                    TdfType type = context.getType();
                    char8_t memName[MAX_XML_ELEMENT_LENGTH];
                    convertMemberToElement(getNameFromMemInfo(context.getMemberInfo()), memName, sizeof(memName));

                    if (type == EA::TDF::TDF_ACTUAL_TYPE_LIST)
                    {
                        const TdfVectorBase& vector = ref.asList();
                        if (vector.vectorSize() > 0)
                        {
                            char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                            mWriter.customBeginElement(getSanitizedMemberName(memName, elementName));
                        }
                        else
                        {
                            return true;
                        }
                    }
                    else if (type == EA::TDF::TDF_ACTUAL_TYPE_MAP)
                    {
                        const TdfMapBase& map = ref.asMap();
                        if (map.mapSize() > 0)
                        {
                            char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                            mWriter.customBeginElement(getSanitizedMemberName(memName, elementName));
                        }
                        else
                        {
                            return true;
                        }
                    }
                    else if (type == EA::TDF::TDF_ACTUAL_TYPE_UNION)
                    {
                        const EA::TDF::TdfUnion& target = ref.asUnion();
                        if (target.getActiveMemberIndex() != EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
                        {
                            char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                            mWriter.customBeginElement(getSanitizedMemberName(memName, elementName));
                        }
                    }
                    else if (type != EA::TDF::TDF_ACTUAL_TYPE_VARIABLE && type != EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE)
                    {
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customBeginElement(getSanitizedMemberName(memName, elementName));
                    }
                    break;
                }
            case TDF_ACTUAL_TYPE_MAP:
                {
                    TdfGenericReferenceConst key = context.getKey();
                    if (key.isTypeString())
                    {
                        mWriter.customBeginElement(XMLENCODER_COLLECTION_ELEMENT);
                        mWriter.AppendAttribute(XMLENCODER_MAP_KEY, key.asString().c_str());
                    }
                    else if (key.isTypeInt())
                    {
                        if (key.getType() != EA::TDF::TDF_ACTUAL_TYPE_ENUM)
                        {
                            //LHS ints need to be strings
                            char8_t keyString[32];
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
                                EA::StdC::Snprintf(mErrorMessage, MAX_ENCODER_ERR_MESSAGE_LEN, "[XmlEncoder].visitValue: Map key is an unsupported int type");
                                return false;
                            }
                            mWriter.customBeginElement(XMLENCODER_COLLECTION_ELEMENT);
                            mWriter.customAppendAttribute(XMLENCODER_MAP_KEY, keyString);
                        }
                        else
                        {
                            const char8_t* enumKey = nullptr;
                            if (key.getTypeDescription().asEnumMap()->findByValue(key.asInt32(), &enumKey))
                            {
                                // if enum, encode the name of the enum
                                mWriter.customBeginElement(XMLENCODER_COLLECTION_ELEMENT);
                                mWriter.customAppendAttribute(XMLENCODER_MAP_KEY, enumKey);
                            }
                            else
                            {
                                EA::StdC::Snprintf(mErrorMessage, MAX_ENCODER_ERR_MESSAGE_LEN, "[XmlEncoder].visitValue: Encode context tagged as map keyed by enums, but key value %d not a valid enum.", context.getKey().asInt32());
                                return false;
                            }
                        }
                    }
                    else if (key.getType() == TDF_ACTUAL_TYPE_BLOB)
                    {
                        const TdfBlob& blobKey = key.asBlob();
                        const char8_t* base64Key= (const char8_t*) blobKey.getData();
                        mWriter.customBeginElement(XMLENCODER_COLLECTION_ELEMENT);
                        mWriter.customAppendAttribute(XMLENCODER_MAP_KEY, base64Key);
                    }
                    else 
                    {
                        EA::StdC::Snprintf(mErrorMessage, MAX_ENCODER_ERR_MESSAGE_LEN, "[XmlEncoder].visitValue: Encode context tagged as map, but key value not integer or string or blob.");
                        return false;
                    }
                    TdfType type = context.getType();
                    if (type == EA::TDF::TDF_ACTUAL_TYPE_TDF)
                    {
                        const Tdf& target = ref.asTdf();
                        const char8_t* fullName = target.getClassName();
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customBeginElement(getSanitizedMemberName(fullName, elementName));
                    }
                    else if (type == EA::TDF::TDF_ACTUAL_TYPE_LIST)
                    {
                        const TdfVectorBase& vector = ref.asList();
                        if (vector.vectorSize() > 0)
                        {
                            char8_t memberName[MAX_XML_ELEMENT_LENGTH];
                            if (findAncestorMemberName(memberName, MAX_XML_ELEMENT_LENGTH, context))
                            {
                                char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                                mWriter.customBeginElement(getSanitizedMemberName(memberName, elementName));
                            }
                            else
                                return false;
                        }
                    }
                    else if (type == EA::TDF::TDF_ACTUAL_TYPE_UNION)
                    {
                        const TdfUnion& target = ref.asUnion();
                        const char8_t* fullName = target.getClassName();
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        uint32_t memberIndex = target.getActiveMemberIndex();
                        if (memberIndex == EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
                        {
                            mWriter.setStateToChars();
                            mWriter.customWriteCloseTag();
                            mWriter.setSimpleElement();
                            mStateStack[mStateDepth].postBeginPreamblePosition = mWriter.getOutputStream().GetPosition();
                            return true; // as a map of union, we don't print union element if it is empty.
                        }
                        mWriter.customBeginElement(getSanitizedMemberName(fullName, elementName));
                    }

                    break;
                }
            case TDF_ACTUAL_TYPE_VARIABLE:
            case TDF_ACTUAL_TYPE_GENERIC_TYPE:
                break;
            case TDF_ACTUAL_TYPE_LIST:
                {
                    TdfType type = context.getType();
                    // print tdf class name as element tag
                    if (type == EA::TDF::TDF_ACTUAL_TYPE_TDF)
                    {
                        const Tdf& target = ref.asTdf();
                        const char8_t* fullName = target.getClassName();
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customBeginElement(getSanitizedMemberName(fullName, elementName));
                    }
                    else if (type == EA::TDF::TDF_ACTUAL_TYPE_UNION)
                    {
                        const TdfUnion& target = ref.asUnion();
                        const char8_t* fullName = target.getClassName();
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customBeginElement(getSanitizedMemberName(fullName, elementName));
                    }
                    else if (type != EA::TDF::TDF_ACTUAL_TYPE_VARIABLE && type != EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE && type != EA::TDF::TDF_ACTUAL_TYPE_MAP)
                    {
                        if (mEncOptions != nullptr && mEncOptions->useCommonEntryName && type != EA::TDF::TDF_ACTUAL_TYPE_BLOB && type != TDF_ACTUAL_TYPE_LIST)
                        {
                            mWriter.customBeginElement(XMLENCODER_COLLECTION_ELEMENT); // only for primitives
                        }
                        else
                        {
                            char8_t memberName[MAX_XML_ELEMENT_LENGTH];
                            if (findAncestorMemberName(memberName, MAX_XML_ELEMENT_LENGTH, context))
                            {
                                char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                                mWriter.customBeginElement(getSanitizedMemberName(memberName, elementName));
                            }
                            else
                                return false;
                        }
                    }
                }
                break;
            default:
                break;
            }
        }

        mStateStack[mStateDepth].postBeginPreamblePosition = mWriter.getOutputStream().GetPosition();

        switch (ref.getType())
        {
        case TDF_ACTUAL_TYPE_UNION:
            {
                const TdfUnion& target = ref.asUnion();
                uint32_t memberIndex = target.getActiveMemberIndex();
                if (memberIndex == EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
                {
                    return true;
                }

                // append attribute to the union tdf class name
                char8_t index[32];
                EA::StdC::Snprintf(index, sizeof(index), "%d", memberIndex);
                mWriter.customAppendAttribute("member", index);

                // write element name for union's field
                mWriter.customBeginElement("valu");
            }
            break;
        case TDF_ACTUAL_TYPE_MAP:
            {
                TdfType parentType = context.getParentType();
                // don't need to print member name if its parent is root. We only need to print if the context is in map of map or further deeper
                if (parentType != TDF_ACTUAL_TYPE_TDF && parentType != TDF_ACTUAL_TYPE_UNKNOWN)
                {
                    const TdfMapBase& map = ref.asMap();
                    if (parentType == TDF_ACTUAL_TYPE_MAP)
                    {
                        if (map.mapSize() == 0) // for case map of map, if the inner map is empty, we skip it. ex:<entry key="a"/>
                        {
                            mWriter.setStateToChars();
                            mWriter.customWriteCloseTagForEmptyElement();
                            mStateStack[mStateDepth].postBeginPreamblePosition = mWriter.getOutputStream().GetPosition();
                            return true;
                        }
                    }
                    else if (parentType == TDF_ACTUAL_TYPE_UNION)
                    {
                        return true;
                    }

                    char8_t memberName[MAX_XML_ELEMENT_LENGTH];
                    if (findAncestorMemberName(memberName, MAX_XML_ELEMENT_LENGTH, context))
                    {
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customBeginElement(getSanitizedMemberName(memberName, elementName));
                    }
                    else
                        return false;
                }
                
            }
            break;
        case TDF_ACTUAL_TYPE_TDF:
        case TDF_ACTUAL_TYPE_LIST:
            break;
        case TDF_ACTUAL_TYPE_VARIABLE:
            {
                if (ref.asVariable().isValid())
                {
                    const Tdf* target = ref.asVariable().get();
                    if (context.getParentType() == TDF_ACTUAL_TYPE_MAP || context.getParentType() == TDF_ACTUAL_TYPE_LIST)
                    {
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customBeginElement(getSanitizedMemberName(target->getClassName(), elementName));
                    }
                    else if (context.getParentType() != TDF_ACTUAL_TYPE_UNION)
                    {
                        char8_t memberName[MAX_XML_ELEMENT_LENGTH] = "";
                        if (!getMemNameForGeneric(context, memberName))
                            return false;
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customBeginElement(getSanitizedMemberName(memberName, elementName));
                    }
                    char8_t idString[32];
                    EA::StdC::Snprintf(idString, sizeof(idString), "%" PRIu32, target->getTdfId());
                    mWriter.customAppendAttribute(XMLENCODER_TAG_TDF_ID, idString);
                    const char8_t* fullName = target != nullptr ? target->getFullClassName() : "";
                    mWriter.customAppendAttribute(XMLENCODER_TAG_TDF_CLASS, fullName);
                }
            }
            break;
        case TDF_ACTUAL_TYPE_GENERIC_TYPE:
            {
                if (ref.asGenericType().isValid())
                {
                    const EA::TDF::TdfGenericValue& target = ref.asGenericType().get();
                    if (context.getParentType() == TDF_ACTUAL_TYPE_MAP || context.getParentType() == TDF_ACTUAL_TYPE_LIST)
                    {
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customBeginElement(getSanitizedMemberName(target.getFullName(), elementName));
                    }
                    else if (context.getParentType() != TDF_ACTUAL_TYPE_UNION)
                    {
                        char8_t memberName[MAX_XML_ELEMENT_LENGTH] = "";
                        if (!getMemNameForGeneric(context, memberName))
                            return false;
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customBeginElement(getSanitizedMemberName(memberName, elementName));
                    }

                    char8_t idString[32];
                    EA::StdC::Snprintf(idString, sizeof(idString), "%" PRIu32, target.getTdfId());
                    mWriter.customAppendAttribute(XMLENCODER_TAG_TDF_ID, idString);
                    mWriter.customAppendAttribute(XMLENCODER_TAG_TDF_CLASS, target.getFullName());
                }
            }
            break;
        case TDF_ACTUAL_TYPE_BITFIELD: 
            writeInteger(static_cast<int64_t>(ref.asBitfield().getBits())); 
            break;
        case TDF_ACTUAL_TYPE_BOOL:
            writeInteger(static_cast<int64_t>(ref.asBool())); 
            break;
        case TDF_ACTUAL_TYPE_INT8:
            writeInteger(static_cast<int64_t>(ref.asInt8()));
            break;
        case TDF_ACTUAL_TYPE_UINT8:
            writeInteger(static_cast<int64_t>(ref.asUInt8()));
            break;
        case TDF_ACTUAL_TYPE_INT16:
            writeInteger(static_cast<int64_t>(ref.asInt16()));
            break;
        case TDF_ACTUAL_TYPE_UINT16:
            writeInteger(static_cast<int64_t>(ref.asUInt16()));
            break;
        case TDF_ACTUAL_TYPE_INT32:
            writeInteger(static_cast<int64_t>(ref.asInt32()));
            break;
        case TDF_ACTUAL_TYPE_UINT32:
            writeInteger(static_cast<int64_t>(ref.asUInt32()));
            break;
        case TDF_ACTUAL_TYPE_INT64:
            writeInteger(static_cast<int64_t>(ref.asInt64()));
            break;
        case TDF_ACTUAL_TYPE_UINT64:
            writeInteger(static_cast<uint64_t>(ref.asUInt64()));
            break;
        case TDF_ACTUAL_TYPE_ENUM:
            {
                bool useValue = mEncOptions != nullptr && (mEncOptions->enumFormat == EA::TDF::EncodeOptions::ENUM_FORMAT_VALUE);
                if (useValue)
                {
                    writeInteger(static_cast<int64_t>(ref.asEnum()));
                }
                else
                {
                    const char8_t* enumName = nullptr;
                    if (ref.getTypeDescription().asEnumMap()->findByValue(ref.asEnum(), &enumName)) 
                    {
                        //write the enum name and break out
                        mWriter.WriteCharData(enumName);
                    }
                    else
                        mWriter.WriteCharData("UNKNOWN");
                }
            }
            break;
        case TDF_ACTUAL_TYPE_STRING:
            {
                writeCharData(ref.asString().c_str());
                // we still want to print close element tag if the string is empty
                if (mWriter.checkValueIsEmpty())
                {
                    mWriter.setStateToChars();
                    mWriter.customWriteCloseTag();
                }
            }
            break;
        case TDF_ACTUAL_TYPE_BLOB:
            {
                size_t bufSize = ref.asBlob().calcBase64EncodedSize() + 1;
                char8_t sizeStr[12];
                EA::StdC::I32toa(int32_t(bufSize - 1), sizeStr, 10);
                mWriter.customAppendAttribute(XMLENCODER_TAG_BLOB_ENC_COUNT, sizeStr);
                mWriter.customAppendAttribute(XMLENCODER_TAG_BLOB_ENC_TYPE, XMLENCODER_TAG_BLOB_OBJECT_NAME);
                char8_t* encodeBuf = (char8_t*) CORE_ALLOC(&mAllocator, bufSize, EA_TDF_DEFAULT_ALLOC_NAME, 0);
                ref.asBlob().encodeBase64(encodeBuf, bufSize);
                writeCharData(encodeBuf);
                // we still want to print close element tag if the blob is empty
                if (mWriter.checkValueIsEmpty())
                {
                    mWriter.setStateToChars();
                    mWriter.customWriteCloseTag();
                }
                CORE_FREE(&mAllocator, encodeBuf);
            }
            break;
        case TDF_ACTUAL_TYPE_OBJECT_TYPE:
            {
                char8_t buf[256];
                ref.asObjectType().toString(buf, sizeof(buf));
                mWriter.WriteCharData(buf);
                break;
            }
        case TDF_ACTUAL_TYPE_OBJECT_ID:
            {
                char8_t buf[256];
                ref.asObjectId().toString(buf, sizeof(buf));
                mWriter.WriteCharData(buf);
                break;
            }
        case TDF_ACTUAL_TYPE_FLOAT:
            writeFloat(static_cast<float>(ref.asFloat()));
            break;
        case TDF_ACTUAL_TYPE_TIMEVALUE:
            writeInteger(ref.asTimeValue().getMicroSeconds());
            break;
        case TDF_ACTUAL_TYPE_UNKNOWN:
            EA::StdC::Snprintf(mErrorMessage, MAX_ENCODER_ERR_MESSAGE_LEN, "[XmlEncoder].visitValue: Invalid info type in tdf member info struct");
            return false;
        }

        return true;
    }

    bool XmlEncoder::postVisitValue(const EA::TDF::TdfVisitContextConst& context)
    {        
        // Write out our right hand side
        const TdfGenericReferenceConst& ref = context.getValue();     
        switch (ref.getType())
        {
        case TDF_ACTUAL_TYPE_UNION:
            {
                const TdfUnion& target = ref.asUnion();
                // Handle the union class
                uint32_t memberIndex = target.getActiveMemberIndex();
                if (memberIndex == EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
                {
                    // this union is empty
                    if (context.getParentType() == TDF_ACTUAL_TYPE_MAP)
                    {
                        mWriter.customEndElement(XMLENCODER_COLLECTION_ELEMENT); //  <entry key="MapKey1"></entry>
                    }
                    else if (context.getParentType() == TDF_ACTUAL_TYPE_LIST)
                    {
                        mWriter.setStateToChars();
                        mWriter.customWriteCloseTagForEmptyElement(); // EX: <stringorint32/>
                    }

                    return true;
                }
                mWriter.customEndElement("valu");
            }
            break;
        case TDF_ACTUAL_TYPE_TDF:
            break;
        case TDF_ACTUAL_TYPE_MAP:
            {
                TdfType parentType = context.getParentType();
                // don't need to print member name if its parent is root. We only need to print if the context is in map of map or further deeper
                if (parentType != TDF_ACTUAL_TYPE_UNKNOWN && parentType != TDF_ACTUAL_TYPE_TDF)
                {
                    const TdfMapBase& map = ref.asMap();
                    if (parentType == TDF_ACTUAL_TYPE_MAP)
                    {
                        if (map.mapSize() == 0) // for case map of map, if the inner map is empty, we skip it. ex:<entry key="a"/>
                        {
                            return true;
                        }
                    }
                    else if (parentType == TDF_ACTUAL_TYPE_UNION)
                    {
                        return true;
                    }

                    char8_t memberName[MAX_XML_ELEMENT_LENGTH] = "";
                    if (findAncestorMemberName(memberName, MAX_XML_ELEMENT_LENGTH, context))
                    {
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customEndElement(getSanitizedMemberName(memberName, elementName));
                    }
                    else
                        return false;
                }
            }
            break;
        case TDF_ACTUAL_TYPE_VARIABLE:
            {
                if (ref.asVariable().isValid())
                {
                    const Tdf* target = ref.asVariable().get();
                    if (context.getParentType() == TDF_ACTUAL_TYPE_MAP || context.getParentType() == TDF_ACTUAL_TYPE_LIST)
                    {
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customEndElement(getSanitizedMemberName(target->getClassName(), elementName));
                    }
                    else if (context.getParentType() != TDF_ACTUAL_TYPE_UNION)
                    {
                        char8_t memberName[MAX_XML_ELEMENT_LENGTH] = "";
                        if (!getMemNameForGeneric(context, memberName))
                            return false;
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customEndElement(getSanitizedMemberName(memberName, elementName));
                    }
                }
            }            
            break;
        case TDF_ACTUAL_TYPE_GENERIC_TYPE:
            {
                if (ref.asGenericType().isValid())
                {
                    const TdfGenericValue& target = ref.asGenericType().get();
                    if (context.getParentType() == TDF_ACTUAL_TYPE_MAP || context.getParentType() == TDF_ACTUAL_TYPE_LIST)
                    {
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customEndElement(getSanitizedMemberName(target.getFullName(), elementName));
                    }
                    else if (context.getParentType() != TDF_ACTUAL_TYPE_UNION)
                    {
                        char8_t memberName[MAX_XML_ELEMENT_LENGTH] = "";
                        if (!getMemNameForGeneric(context, memberName))
                            return false;
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customEndElement(getSanitizedMemberName(memberName, elementName));
                    }
                }
            }
            break;
        case TDF_ACTUAL_TYPE_LIST:
            break;
        default:
            break;
        }


        if (!context.isRoot())
        {
            switch(context.getParentType())
            {
            case TDF_ACTUAL_TYPE_UNION:
                break;
            case TDF_ACTUAL_TYPE_TDF:
                {
                    TdfType type = context.getType();
                    char8_t memName[MAX_XML_ELEMENT_LENGTH];
                    memName[0] = '\0';
                    convertMemberToElement(getNameFromMemInfo(context.getMemberInfo()), memName, sizeof(memName));
                    if (context.getType() == TDF_ACTUAL_TYPE_LIST)
                    {
                        const TdfVectorBase& vector = ref.asList();
                        if (vector.vectorSize() > 0)
                        {
                            char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                            mWriter.customEndElement(getSanitizedMemberName(memName, elementName));
                        }
                    }
                    else if (type == TDF_ACTUAL_TYPE_MAP)
                    {
                        const TdfMapBase& map = ref.asMap();
                        if (map.mapSize() > 0)
                        {
                            char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                            mWriter.customEndElement(getSanitizedMemberName(memName, elementName));
                        }
                    }
                    else if (type == TDF_ACTUAL_TYPE_UNION)
                    {
                        const TdfUnion& target = ref.asUnion();
                        if (target.getActiveMemberIndex() != EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
                        {
                            char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                            mWriter.customEndElement(getSanitizedMemberName(memName, elementName));
                        }
                    }
                    else if (type != EA::TDF::TDF_ACTUAL_TYPE_VARIABLE && type != EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE)
                    {
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customEndElement(getSanitizedMemberName(memName, elementName));
                    }
                    break;
                }
            case TDF_ACTUAL_TYPE_MAP:
                {
                    TdfType type = context.getType();
                    if (type == EA::TDF::TDF_ACTUAL_TYPE_TDF)
                    {
                        const Tdf& target = ref.asTdf();
                        const char8_t* fullName = target.getClassName();
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customEndElement(getSanitizedMemberName(fullName, elementName));
                    }
                    else if (type == EA::TDF::TDF_ACTUAL_TYPE_LIST)
                    {
                        const TdfVectorBase& vector = ref.asList();
                        if (vector.vectorSize() > 0)
                        {
                            char8_t memberName[MAX_XML_ELEMENT_LENGTH] = "";
                            if (findAncestorMemberName(memberName, MAX_XML_ELEMENT_LENGTH, context))
                            {
                                char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                                mWriter.customEndElement(getSanitizedMemberName(memberName, elementName));
                            }
                            else
                                return false;
                        }
                    }
                    else if (type == EA::TDF::TDF_ACTUAL_TYPE_UNION)
                    {
                        const TdfUnion& target = ref.asUnion();
                        const char8_t* fullName = target.getClassName();
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customEndElement(getSanitizedMemberName(fullName, elementName));
                    }

                    mWriter.customEndElement(XMLENCODER_COLLECTION_ELEMENT);
                }
            case TDF_ACTUAL_TYPE_VARIABLE:
            case TDF_ACTUAL_TYPE_GENERIC_TYPE:
                break;
            case TDF_ACTUAL_TYPE_LIST:
                {
                    TdfType type = context.getType();
                    if (type == EA::TDF::TDF_ACTUAL_TYPE_TDF)
                    {
                        const Tdf& target = ref.asTdf();
                        const char8_t* fullName = target.getClassName();
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customEndElement(getSanitizedMemberName(fullName, elementName));
                    }
                    else if (type == EA::TDF::TDF_ACTUAL_TYPE_UNION)
                    {
                        const TdfUnion& target = ref.asUnion();
                        const char8_t* fullName = target.getClassName();
                        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                        mWriter.customEndElement(getSanitizedMemberName(fullName, elementName));
                    }
                    else if (type != EA::TDF::TDF_ACTUAL_TYPE_VARIABLE && type != EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE && type != EA::TDF::TDF_ACTUAL_TYPE_MAP)
                    {
                        if (mEncOptions != nullptr && mEncOptions->useCommonEntryName && type != EA::TDF::TDF_ACTUAL_TYPE_BLOB && type != TDF_ACTUAL_TYPE_LIST)
                        {
                            mWriter.customEndElement(XMLENCODER_COLLECTION_ELEMENT); // only for primitives
                        }
                        else
                        {
                            char8_t memberName[MAX_XML_ELEMENT_LENGTH] = "";
                            if (findAncestorMemberName(memberName, MAX_XML_ELEMENT_LENGTH, context))
                            {
                                char8_t elementName[MAX_XML_ELEMENT_LENGTH];
                                mWriter.customEndElement(getSanitizedMemberName(memberName, elementName));
                            }
                            else
                                return false;
                        }
                    }
                }   
                break;
            default:
                //do nothing 
                break;
            }
        }

        return true;
    }

    bool XmlEncoder::encode(EA::IO::IStream& iStream, const EA::TDF::Tdf& tdf, const EncodeOptions* encOptions, const MemberVisitOptions& options)
    {
        bool result = false;
        mEncOptions = encOptions;        
        mWriter.SetLineEnd(EA::XML::kLineEndUnix); // we want /n as line end
        mWriter.SetOutputStream(&iStream);
        mWriter.WriteXmlHeader();
        char8_t elementName[MAX_XML_ELEMENT_LENGTH];
        const char8_t* className = getSanitizedMemberName(tdf.getClassName(), elementName); // convert to lower case
        mWriter.customBeginElement(className);
        result = tdf.visit(*this, options);
        mWriter.customEndElement(className);
        mWriter.WriteCharData("\n", 1); // add /n to the end of document

        mWriter.SetOutputStream(nullptr);
        mWriter.setIndentLevel(0);
        return result;
    }

    void XmlEncoder::writeInteger(int64_t value)
    {
        char8_t buffer[IntegerBufferSize] = {0};
        int len = EA::StdC::Snprintf(buffer, sizeof(buffer), "%" PRId64, value);
        if (len > 0)
            mWriter.customWriteIntegerData(buffer, len);
    }
    void XmlEncoder::writeInteger(uint64_t value)
    {
        char8_t buffer[IntegerBufferSize] = {0};
        int len = EA::StdC::Snprintf(buffer, sizeof(buffer), "%" PRIu64, value);
        if (len > 0)
            mWriter.customWriteIntegerData(buffer, len);
    }
    void XmlEncoder::writeFloat(float value)
    {
        char8_t buffer[FloatBufferSize] = {0};
        int len = EA::StdC::Snprintf(buffer, sizeof(buffer), "%f", value);
        if (len > 0)
            mWriter.customWriteIntegerData(buffer, len);
    }

/**
  \brief Write a null-terminated string to the output buffer.
  \param s string to write.
*/
void XmlEncoder::printString(const char8_t *s)
{
    printString(s, strlen(s));
}

/**
  \brief Write a string to the output buffer.
  \param s string to write. (optionally null-terminated)
  \param len length of string to write.
*/
void XmlEncoder::printString(const char8_t *s, size_t len)
{
    mWriter.customWriteCharData(s, len);
}


/**
  \brief Write XML characters (PCDATA).

  This method can for efficiency purposes simply write out the data verbatim.

  this method will replace invalid control characters with a question mark, 
  and perform the necessary escaping for ASCII characters that require it (e.g. \& is replaced by \&amp;).

  In addition, bytes with the high-bit set will be validated to ensure they
  are valid UTF-8 sequences, each non-conforming byte will be replaced by a '?'.
  
    UTF8 encoding
    00-7F   0-127       US-ASCII (single byte)
    80-BF   128-191     Second, third, or fourth byte of a multi-byte sequence
    C0-C1   192-193     invalid: Overlong encoding: start of a 2-byte sequence, but code point <= 127
    C2-DF   194-223     Start of 2-byte sequence
    E0-EF   224-239     Start of 3-byte sequence
    F0-F4   240-244     Start of 4-byte sequence (F4 is partially restricted for codepoints above 10FFFF)
    F5-F7   245-247     Restricted by RFC 3629: start of 4-byte sequence for codepoint above 10FFFF
    F8-FB   248-251     Restricted by RFC 3629: start of 5-byte sequence
    FC-FD   252-253     Restricted by RFC 3629: start of 6-byte sequence
    FE-FF   254-255     Invalid: not defined by original UTF-8 specification

    Valid characters in XML:
              \n |  \t |  \r
    Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]

    Invalid characters can be written out as U+#### or &#x####.  We write them out as '?'

  \param characters        characters to write.
*/
void XmlEncoder::writeCharData(const char *chars)
{
    while (*chars != '\0')
    {
        // For efficiency, print runs of chars that don't require special handling all at once,
        // scan through and find batches that can be printed verbatim
        const char8_t* pch = chars;
        while ((*pch >= 0x20) && (*pch < 0x7f))
        {
            ++pch;
        }

        if (pch != chars)
        {
            printString(chars, pch-chars);
            chars = pch;
            if (*chars == '\0')
                return;
        }

        // At this point we've got a char that requires special handling, either control characters, Ascii chars that need escaping,
        // or UTF-8 chars to validate
        uint8_t ch = static_cast<uint8_t>(*chars);

        // The following whitespace characters should be relatively rare, so rather than filtering them above,
        // we'll handle them here, anything other than these at this point is a control character we don't support
        if ((ch & 0x80) == 0)
        {
            if (ch == '\n' || ch == '\r' || ch == '\t')
                printChar(ch);
            else
                printChar('?');
            ++chars;
            continue;
        }
        
        // Now we get into the meat of UTF-8 validation, check the validity of the first char
        if (ch < 0xC2 || ch > 0xF4)
        {
            printChar('?');
            ++chars;
            continue;
        }

        int32_t count = 0;
        if (ch < 0xE0)
            count = 2;
        else if (ch < 0xF0)
            count = 3;
        else if (ch < 0xF5)
            count = 4;

        // And check the validity of the remaining chars...
        bool valid = true;
        for (int32_t j = 1; j < count; ++j)
        {
            uint8_t ch2 = static_cast<uint8_t>(chars[j]);
            if (ch2 < 0x80 || ch2 > 0xBF)   // Invalid UTF-8 encoding check
            {
                valid = false;
                break;
            }

            // Special XML case checks:
            if (ch == 0xF4 && j == 1 && ch2 > 0x8F)
            {
                // Special case for > 0x10FFFF
                valid = false;
                break;
            }
            else if (ch == 0xED && j == 1 && ch2 > 0x9F)
            {
                // Special case for 0xD800 -> 0xDFFF
                valid = false;
                break;
            }
            else if (ch == 0xEF && j == 1 && ch2 == 0xBF)
            { 
                uint8_t ch3 = static_cast<uint8_t>(chars[j+1]);
                if(ch3 == 0xBE || ch3 == 0xBF)
                {
                    // Special case for 0xFFFE and 0xFFFF
                    valid = false;
                    break;
                }
            }
        }

        // If we've found a fully valid UTF-8 representation of a char, print all the bytes now
        if (valid)
        {
            printString(chars, count);
            chars += count;
        }
        else
        {
            // Otherwise replace just the first char with a question mark, we'll try next loop around to see
            // if the subsequent chars form a valid UTF-8 char
            printChar('?');
            ++chars;
        }
    } // for
}

bool TdfXmlWriter::customWriteWhitespace( size_t nChars )
{
    //return mpOutputStream->Fill(nChars);
    const size_t kSpacesCount = 32;
    const char   kSpaces[kSpacesCount + 1] = "                                ";

    // Since nIndent may be > than kSpacesCount, we allow for writing the indent in pieces.
    while (nChars > 0) {
        const size_t nCharsToWrite = ((nChars < kSpacesCount) ? nChars : kSpacesCount);

        if (!mpOutputStream->Write(kSpaces, nCharsToWrite)) 
            return false;

        nChars -= nCharsToWrite;
    }
    return true;
}

bool TdfXmlWriter::customWriteIndent()
{
    if (mbFormatPretty) {
        // Write a newline, regardless of how many spaces follow.
        // Don't write a newline if we have written nothing at all yet to the file.
        if (mnCharCount) {
            if (!mpOutputStream->Write("\n", 1)) 
                return false;
        }

        if (!customWriteWhitespace(mnIndentLevel * mnIndentSpaces))
            return false;
    }
    return true;
}

bool TdfXmlWriter::customCloseCurrentElement()
{
    switch (mnState) {
    case kStateElement:
        if (!mpOutputStream->Write(">", 1)) 
            return false;
        break;

    case kStateProcessingInstruction:
        if (!mpOutputStream->Write("?>", 2))
            return false;
        break;

    case kStateCDATA:
        if (!mpOutputStream->Write("]]>", 3))
            return false;
        break;

    case kStateChars:
        break;
    }

    mnState = kStateChars;
    return true;
}

void TdfXmlWriter::setStateToChars()
{
    mnState = kStateChars;
}

bool TdfXmlWriter::checkValueIsEmpty() const
{
    return (mnState == kStateElement);
}

void TdfXmlWriter::customWriteCloseTag()
{
    mpOutputStream->Write(">", 1);
}

void TdfXmlWriter::customWriteCloseTagForEmptyElement()
{
    mnIndentLevel--; // we want to reduce indent level here since we won't call endElement
    mbSimpleElement = false;
    mpOutputStream->Write("/>", 2);
}

void TdfXmlWriter::customWriteNewline()
{
    mpOutputStream->Write("\n", 1);
}

void TdfXmlWriter::setSimpleElement()
{
    mbSimpleElement = false;
}

bool TdfXmlWriter::customBeginElement( const char* psElementName )
{
    if (customCloseCurrentElement()
        && customWriteIndent()
        && mpOutputStream->Write("<", 1)
        && mpOutputStream->Write(psElementName, EA::StdC::Strlen(psElementName)))
    {
        mnState = kStateElement;
        mbSimpleElement = true;
        mnIndentLevel++;
        return true;
    }
    return false;
}

bool TdfXmlWriter::customEndElement( const char* psElementName )
{
    EA_ASSERT(mnIndentLevel > 0);

    mnIndentLevel--;

    switch (mnState) {
    case kStateProcessingInstruction:
        EA_FAIL_MESSAGE("XmlWriter: Cannot close element because we are in a processing instruction.");
        return false;

    case kStateElement:
        mnState = kStateChars;
        mbSimpleElement = false;
        return mpOutputStream->Write("/>", 2);

    case kStateCDATA:
        if(!customCloseCurrentElement())
            return false;
        // Fall through.

    case kStateChars:
        if (!mbSimpleElement) { // If the current element has no child elements...
            if (!customWriteIndent()) // Then write an indent before writing the element end.
                return false;
        }

        mbSimpleElement = false;

        return (mpOutputStream->Write("</", 2)
            && mpOutputStream->Write(psElementName, EA::StdC::Strlen(psElementName))
            && mpOutputStream->Write(">", 1));
    }

    return false;
}

bool TdfXmlWriter::customAppendAttribute(const char8_t* psAttrName, const char8_t* psValue)
{
    if ((mnState != kStateElement) && (mnState != kStateProcessingInstruction))
        return false;

    const size_t valLength = (psValue != nullptr) ? EA::StdC::Strlen(psValue) : 0;

    return (customWriteText(" ", 1)
        && customWriteText(psAttrName, EA::StdC::Strlen(psAttrName))
        && customWriteText("=\"", 2)
        && customWriteText(psValue, valLength)
        && customWriteText("\"", 1));
}

bool TdfXmlWriter::customAppendAttribute(const char8_t* psAttrName, const char8_t* psValue, size_t valueLength)
{
    if ((mnState != kStateElement) && (mnState != kStateProcessingInstruction))
        return false;

    return (customWriteText(" ", 1)
        && customWriteText(psAttrName, EA::StdC::Strlen(psAttrName))
        && customWriteText("=\"", 2)
        && customWriteText(psValue, valueLength)
        && customWriteText("\"", 1));
}

bool TdfXmlWriter::customWriteCharData(const char8_t* psCharData, size_t nCount)
{
    return customCloseCurrentElement() && WriteEscapedString(psCharData, nCount);
}

bool TdfXmlWriter::customWriteIntegerData( const char8_t* psIntData, size_t nCount )
{
    return customCloseCurrentElement() && mpOutputStream->Write(psIntData, nCount);
}

void TdfXmlWriter::setIndentLevel( size_t num )
{
    mnIndentLevel = num;
}

bool TdfXmlWriter::customWriteText(const char8_t* psCharData, size_t nCount)
{
    mnCharCount += nCount;
    return mpOutputStream->Write(psCharData, nCount);
}

}
}
