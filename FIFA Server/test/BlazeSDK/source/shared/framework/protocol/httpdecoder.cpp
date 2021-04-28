/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class HttpDecoder

    This class provides a mechanism to decode data from an HTTP request, typically the caller
    will use this class to decode an HTTP request into a TDF object.  The HTTP request may pass
    in parameters either in the query string after the URL for a GET request, or in the body
    for a POST request.
    
    Each HTTP parameter represents one value in the TDF object.  As TDF tags lose any
    case-sensitivity when converted into a packed 3-byte form, all comparisons are case-insensitive
    and a caller is free to use any case for the HTTP parameter names.  At the top level of a TDF
    a tag name for a primitive member (string, integer) matches one-to-one to an HTTP parameter
    name.  
    
    Arrays and structs are handled by using the pipe character "|" in the HTTP parameter
    to separate tags and/or array indices.  This is best illustrated in an example.

    Given the following two classes defined in a tdf file:

    class Nested
    {
        [tag="leaf", default="2"] int32_t mLeaf;
    }

    class Request
    {
        [tag="num", default="1"] int32_t mNum;
        [tag="text"] string(256) mText;
        [tag="ints", default="2"] int32_t mInts[3][3];
        [tag="nest"] Nested mNested[2];
    }

    One would format a URL as follows in order to call an API which takes in an argument of
    type Request:

    http://<webhost>/<component>/<command>?
    num=64&text=bob&ints|0|0=1&ints|0|1=2&ints|0|2=3&ints|1|0=4&ints|1|1=5&ints|1|2=6&
    ints|2|0=7&ints|2|1=8&ints|2|2=9&nest|0|leaf=20&nest|1|leaf=45

    Notice that top-level primitives "num" and "text" have HTTP parameters which simply match
    the tags.  The two-dimensional array has keys of the form "ints|<1st dim>|<2nd dim>".
    And the one-dimensional array of structs has keys of the form "nest|<index>|leaf".

    For maps, the map item key is concatenated with the tag, and the map item value of the 
    resulting parameter.

    class Request
    {
        [tag="smap"] map<int32_t, string> mStringMap;
    }

    For example the "smap" element in Request above is of type "map" containing string values
    referenced by an int32_t key.   The URL snippet below defines this map.

    http://<webhost>/<component>/<command>?smap|2=str1&smap|0=str2&smap|12=str3

    The map contains three value pairs:
        <2, str1>
        <0, str2>
        <12, str3>

    Unions can be specified by using a similar syntax to the nested structures.
    However you can only assign a value to one leaf member for each union.

    union TestUnion
    {
        [tag="yyy"] int32_t mSomeInt32;
        [tag="xxx"] uint32_t mSomeUInt32;
        [tag="zzz"] int64_t mSomeInt64;
    }

    class TestClass
    {
        [tag="u1"] TestUnion mUnion1;
        [tag="u2"] TestUnion mUnion2;
    }

    To set the mInt64 member of u1 and the int32 member of u2 would be:

    http://<webhost>/<component>/<resource>?u1|someInt64=1024&u2|someInt32=2048

    IMPORTANT NOTES ABOUT UNIONS:
    - If you try to set more than one member of the same union in a single request, the first one 
      found is the one that will be set.  This doesn't mean the first one specified on the URL
      since the parameters are stored in a hash_map.  It's the first one found while iterating over
      that hash_map.  If that sounds confusing, the take away is don't  use the same union more
      than once in a request!
    - Unions can only be specified as a sub-object of a class.  You cannot have a 
      top-level union.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "framework/blaze.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/protocol/shared/httpdecoder.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/protocol/shared/heat2util.h"
#include "framework/util/shared/bytearrayinputstream.h"
#include "framework/util/shared/bytearrayoutputstream.h"
#include "framework/util/shared/base64.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

//SDK does not use the standard "EA_ASSERT" so we redefine it here as "BlazeAssert"
#if !defined(EA_ASSERT) && defined(BlazeAssert)
#define EA_ASSERT(X) BlazeAssert(X)
#endif

#include "EASTL/string.h"
#include "EASTL/sort.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
const uint32_t HTTP_INCOMING_VECTOR_SIZE_LIMIT = 65536;


/*** Public Methods ******************************************************************************/
/*************************************************************************************************/
/*!
    \brief HttpDecoder

    Construct a new Http decoder that decodes from the given buffer

    \param[in]  buffer - the buffer to decode from

*/
/*************************************************************************************************/
HttpDecoder::HttpDecoder() :
#ifdef BLAZE_CLIENT_SDK
    mParamMap(MEM_GROUP_FRAMEWORK, "HttpDecoder::mParamMap"),
#else
    mParamMap(BlazeStlAllocator("HttpDecoder::mParamMap")),
#endif
    mBuffer(nullptr),
    mErrorCount(0),
    mValidationError(Blaze::ERR_OK)
{    
    *mUri = '\0';
}

/*************************************************************************************************/
/*!
    \brief ~HttpDecoder

    Destructor for HttpDecoder class.
*/
/*************************************************************************************************/
HttpDecoder::~HttpDecoder()
{
}

bool HttpDecoder::visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue)
{
    // Initialize our state
    mErrorCount = 0;
    mUri[0] = '\0';
    mParamMap.clear();
    mValidationError = Blaze::ERR_OK;

    // Parse the HTTP request into a URI and map of parameters
    HttpProtocolUtil::HttpMethod method;
    HttpProtocolUtil::HttpReturnCode rc = HttpProtocolUtil::parseRequest(
        *mBuffer, method, mUri, sizeof(mUri), mParamMap);

    if (rc != HttpProtocolUtil::HTTP_OK)
    {
        ++mErrorCount;
        return false;
    }

    parseTdf(tdf);

    return (mErrorCount == 0);
}

void HttpDecoder::parseTdf(EA::TDF::Tdf &tdf)
{
    // For each pair in the mParamMap: 
    for (HttpParamMap::const_iterator it = mParamMap.begin(); it != mParamMap.end(); ++it)
    {

#ifdef BLAZE_CLIENT_SDK
        BLAZE_EASTL_LIST<BLAZE_EASTL_STRING> tags(MEM_GROUP_FRAMEWORK, "HttpDecoder::tags");
#else
        BLAZE_EASTL_LIST<BLAZE_EASTL_STRING> tags;
#endif

        // Parse the key into a list of tags: 
        BLAZE_EASTL_STRING::size_type startPos = 0;
        while (startPos != BLAZE_EASTL_STRING::npos)
        {
            // The format here looks for delimiters (| or []().) and gets the tags in between.
            // The format skips multiple delimiters, so something like  tag||tag||||tag  is valid.
            BLAZE_EASTL_STRING::size_type endPos = it->first.find_first_of(getNestDelim(), startPos);
            if (endPos != startPos)
            {
                if (endPos == BLAZE_EASTL_STRING::npos)
                {
                    tags.push_back(it->first.substr(startPos));
                    break;
                }
                tags.push_back(it->first.substr(startPos, endPos - startPos));
            }
            startPos = endPos + 1;
        } 
        
        EA::TDF::TdfGenericReference tdfRef(tdf);
        getValueByTags(tags, it->second.c_str(), tdfRef, nullptr);
    }
}

void HttpDecoder::getValueByTags(BLAZE_EASTL_LIST<BLAZE_EASTL_STRING>& tags, const char8_t* keyValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo)
{
#ifndef BLAZE_CLIENT_SDK
    const char8_t* memberName = memberInfo ? memberInfo->getMemberName() : "(inside a map/list/generic)";
#endif

    if (tags.empty())
    {
        // This was the last tag: 
        switch (outValue.getType())
        {
            // Simple types:
            case EA::TDF::TDF_ACTUAL_TYPE_INT8:
            {   // Special case to support a special way to read char8_t:
                if (outValue.getTypeDescription().isIntegralChar())  { outValue.asInt8() = keyValue[0];                     return; }
                                                               blaze_str2int(keyValue, &outValue.asInt8());                 return;
            }

            case EA::TDF::TDF_ACTUAL_TYPE_INT16:               blaze_str2int(keyValue, &outValue.asInt16());                return; 
            case EA::TDF::TDF_ACTUAL_TYPE_INT32:               blaze_str2int(keyValue, &outValue.asInt32());                return; 
            case EA::TDF::TDF_ACTUAL_TYPE_INT64:               blaze_str2int(keyValue, &outValue.asInt64());                return; 
            case EA::TDF::TDF_ACTUAL_TYPE_UINT8:               blaze_str2int(keyValue, &outValue.asUInt8());                return; 
            case EA::TDF::TDF_ACTUAL_TYPE_UINT16:              blaze_str2int(keyValue, &outValue.asUInt16());               return; 
            case EA::TDF::TDF_ACTUAL_TYPE_UINT32:              blaze_str2int(keyValue, &outValue.asUInt32());               return; 
            case EA::TDF::TDF_ACTUAL_TYPE_UINT64:              blaze_str2int(keyValue, &outValue.asUInt64());               return; 

            case EA::TDF::TDF_ACTUAL_TYPE_BITFIELD:            blaze_str2int(keyValue, &outValue.asBitfield().getBits());   return;   // Could expand this to support config syntax (bit0|bit2|bit3)

            case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:               blaze_str2flt(keyValue, outValue.asFloat());                 return;
            case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_TYPE:         outValue.asObjectType() = EA::TDF::ObjectType::parseString(keyValue);   return;
            case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_ID:           outValue.asObjectId() = EA::TDF::ObjectId::parseString(keyValue);       return;

            case EA::TDF::TDF_ACTUAL_TYPE_BOOL:
            {
                int8_t tempVal = 0; 
                blaze_str2int(keyValue, &tempVal);
                outValue.asBool() = (tempVal != 0);
                return; 
            }

            case EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE:            // It would be nice to expand this to support GMTime like config decoder does
            {
                int64_t microseconds = 0;
                blaze_str2int(keyValue, &microseconds);
                outValue.asTimeValue() = TimeValue(microseconds);
                return;
            }

            case EA::TDF::TDF_ACTUAL_TYPE_BLOB:
            {
                size_t keyValueLength = strlen(keyValue);
                outValue.asBlob().resize((EA::TDF::TdfSizeVal)keyValueLength);
                ByteArrayInputStream input((const uint8_t *) keyValue, (uint32_t) keyValueLength);
                ByteArrayOutputStream output(outValue.asBlob().getData(), outValue.asBlob().getSize());
                uint32_t size = Base64::decode(&input, &output);
                outValue.asBlob().setCount(size);
                return;
            }


            case EA::TDF::TDF_ACTUAL_TYPE_STRING:
            {
                uint32_t keyValueLength = static_cast<uint32_t>(strlen(keyValue));

                uint32_t maxLength = memberInfo ? memberInfo->additionalValue : 0;
                if ((maxLength > 0) && (maxLength < keyValueLength))
                {
#ifndef BLAZE_CLIENT_SDK
                    // input exceeds the max length defined for this EA::TDF::TdfString
                    BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding string for field " << memberName <<
                        " as incoming string length (" << keyValueLength << ") exceeds max length (" << maxLength << ")!");
#endif
                    INC_ERROR_COUNT;
                    mValidationError = ERR_TDF_STRING_TOO_LONG;
                    return;
                }

                outValue.asString() = keyValue;

                if (!outValue.asString().isValidUtf8())
                {
#ifndef BLAZE_CLIENT_SDK
                    BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding string for field " << memberName <<
                        " as incoming string is non-Unicode!");
#endif
                    INC_ERROR_COUNT;
                    mValidationError = ERR_TDF_STRING_NOT_UTF8;
                    return;
                }
                return; 
            }

            case EA::TDF::TDF_ACTUAL_TYPE_ENUM:
            {
                const EA::TDF::TdfEnumMap *enumMap = outValue.getTypeDescription().asEnumMap();
                if ((enumMap == nullptr) || !enumMap->findByName(keyValue, outValue.asEnum()))
                {
                    // if value is not enum constant identifier
                    // try and see if it's number (backwards compatibility)
                    if (*blaze_str2int(keyValue, &outValue.asEnum()) != '\0')
                    {
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding enum for field " << memberName <<
                            ". Value is not in enum, and is not an int!");
#endif
                        INC_ERROR_COUNT;
                        mValidationError = ERR_INVALID_TDF_ENUM_VALUE;
                        return;
                    }
                }
                return; 
            }


            // Special case for Variable BACKWARDS-COMPATIBLE SYNTAX:
            case EA::TDF::TDF_ACTUAL_TYPE_VARIABLE:
            {
#ifndef BLAZE_CLIENT_SDK
                BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: Use of deprecated variable tdf syntax for field " << memberName);
#endif
                INC_ERROR_COUNT;
                return;
            }

            // More backwards compatible syntax:
            case EA::TDF::TDF_ACTUAL_TYPE_LIST:
            {
                if (isdigit(keyValue[0]) == 0)
                {
#ifndef BLAZE_CLIENT_SDK
                    BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding list for field " << memberName <<
                        " as incoming list size (" << keyValue << ") does not appear to be a number.");
#endif
                    INC_ERROR_COUNT;
                    return;
                }

                size_t listSize = 0;
                blaze_str2int(keyValue, &listSize);

                if (listSize >= HTTP_INCOMING_VECTOR_SIZE_LIMIT)
                {
                    // Don't allocate giant lists. This tends to just cause crashes.
#ifndef BLAZE_CLIENT_SDK
                    BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].getValueByTags: error decoding list for field " << memberName <<
                        " as incoming list size (" << listSize << ") is obviously too big and probably from a malformed request.");
#endif
                    INC_ERROR_COUNT;
                    return;
                }

                if (listSize > outValue.asList().vectorSize())
                {
                    // Because we don't process URI params in order (and because they can be specified out of order in the URI string)
                    // we don't want to clear any list items (partial or complete) that we may have already processed
                    outValue.asList().resizeVector(listSize, false);
                }
                return;
            }

            default:
            {
#ifndef BLAZE_CLIENT_SDK
                BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder] No additional tags found when parsing " << memberName << ". This type ("
                              << outValue.getFullName() <<") cannot be the final part of a key.");
#endif
                INC_ERROR_COUNT;
                return;
            } 
        }
    }
    else
    {
        // More tags exist: 
        switch (outValue.getType())
        {
            case EA::TDF::TDF_ACTUAL_TYPE_VARIABLE:
            {
                // Two options:
                BLAZE_EASTL_STRING tdfIdStr = tags.front();
                EA::TDF::TdfId tdfId = EA::TDF::INVALID_TDF_ID;
                blaze_str2int(tdfIdStr.c_str(), &tdfId);
                if (outValue.asVariable().isValid())
                {
                    // This logic will have issues with tags that start with a number:
                    if (outValue.asVariable().get()->getTdfId() == tdfId)
                    {
                        tags.pop_front();       // This means we're either the same tag|#|blah = value, or we don't have a #.
                    }
                    else if (tdfId != EA::TDF::INVALID_TDF_ID)
                    {
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding tdf variable (new syntax) for field " << memberName <<
                            " as incoming tdf id (" << tdfId << ") did not match the existing variable's TdfId (" << outValue.asVariable().get()->getTdfId() << ") (and was not a invalid value (a non-int tag))!");
#endif
                        INC_ERROR_COUNT;
                        return;
                    }
                }
                else
                {
                    outValue.asVariable().create(tdfId);        // Get the value as a TdfId, and use it to create the VariableTdf.
                    if (!outValue.asVariable().isValid())
                    {
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding tdf variable (new syntax) for field " << memberName <<
                            " as incoming tdf id (" << tdfId << ") could not be created!");
#endif
                        INC_ERROR_COUNT;
                        return;
                    }
                    tags.pop_front();
                }
                            
                EA::TDF::TdfGenericReference outValueRef(*outValue.asVariable().get());
                getValueByTags(tags, keyValue, outValueRef, nullptr);
                return;
            }
            case EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE:
            {
                // Parse the key as a TdfId:
                BLAZE_EASTL_STRING tdfIdStr = tags.front();
                tags.pop_front();

                EA::TDF::TdfId tdfId = EA::TDF::INVALID_TDF_ID;
                blaze_str2int(tdfIdStr.c_str(), &tdfId);

                // If the generic already is valid, make sure the types match: 
                if (outValue.asGenericType().isValid())
                {
                    if (outValue.asGenericType().get().getTdfId() != tdfId)
                    {
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding tdf generic for field " << memberName <<
                            " as incoming tdf id (" << tdfId << ") did not match the existing variable's TdfId (" << outValue.asGenericType().get().getTdfId() << ")!" );
#endif
                        INC_ERROR_COUNT;
                        return;
                    }
                }
                else
                {
                    outValue.asGenericType().create(tdfId);        // Get the value as a TdfId, and use it to create the VariableTdf.
                    if (!outValue.asGenericType().isValid())
                    {
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding tdf generic for field " << memberName <<
                            " as incoming tdf id (" << tdfId << ") could not be created!");
                        switch (tdfId)
                        {
                        case EA::TDF::TDF_ACTUAL_TYPE_UNKNOWN:       BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: The Unknown type (0) is not a valid type to create."); break;
                        case EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE:  BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: Do not specify the generic type (1) as the type to create, instead use the type id or tdf id of the type you want to create."); break;
                        case EA::TDF::TDF_ACTUAL_TYPE_MAP:           BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: The base map type (2) is not a valid type to create. Instead, use the tdf id for the specific map type you want to create."); break;
                        case EA::TDF::TDF_ACTUAL_TYPE_LIST:          BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: The base list type (3) is not a valid type to create. Instead, use the tdf id for the specific list type you want to create. (Ex. 'list<string>' = 2806890242)"); break;
                        case EA::TDF::TDF_ACTUAL_TYPE_ENUM:          BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: The base enum type (5) is not a valid type to create. Instead, use the tdf id for the specific enum type you want to create."); break;
                        case EA::TDF::TDF_ACTUAL_TYPE_BITFIELD:      BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: The base bitfield type (8) is not a valid type to create. Instead, use the tdf id for the specific bitfield type you want to create. (Ex. 'Blaze::GameManager::GameSettings' = 0xff925d23)"); break;
                        case EA::TDF::TDF_ACTUAL_TYPE_UNION:         BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: The base union type (10) is not a valid type to create. Instead, use the tdf id for the specific union type you want to create."); break;
                        case EA::TDF::TDF_ACTUAL_TYPE_TDF:           BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: The base tdf type (11) is not a valid type to create. Instead, use the tdf id for the specific tdf type you want to create."); break;
                        default:    break;
                        }
#endif
                        INC_ERROR_COUNT;
                        return;
                    }
                }

                EA::TDF::TdfGenericReference outValueRef(outValue.asGenericType().get());
                getValueByTags(tags, keyValue, outValueRef, nullptr);
                return;
            }

            case EA::TDF::TDF_ACTUAL_TYPE_LIST:          
            {
                BLAZE_EASTL_STRING indexString = tags.front();
                tags.pop_front();

                if (isdigit(indexString[0]) == 0)
                {
                    // This is the old list syntax (where we passed up the list size).
                    if (indexString[0] == '\0')
                    {
                        getValueByTags(tags, keyValue, outValue, nullptr);
                        return;
                    }

#ifndef BLAZE_CLIENT_SDK
                    BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding list for field " << memberName <<
                        " as incoming list index (" << indexString << ") does not appear to be a number.");
#endif
                    INC_ERROR_COUNT;
                    return;
                }
                uint32_t indexValue = 0;
                blaze_str2int(indexString.c_str(), &indexValue);

                if (indexValue >= HTTP_INCOMING_VECTOR_SIZE_LIMIT) // Config driven? Gotta limit this somewhere.
                {
                    // Don't allocate giant lists. This tends to just cause crashes.
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding list for field " << memberName <<
                            " as incoming list index (" << indexValue << ") is obviously too big and probably from a malformed request.");
#endif
                    INC_ERROR_COUNT;
                    return;
                }

                EA::TDF::TdfGenericReference newListValue;
                if (indexValue < outValue.asList().vectorSize())
                {
                    outValue.asList().getReferenceByIndex(indexValue, newListValue);
                }
                else if (indexValue == outValue.asList().vectorSize())
                {
                    // 99% of the time we're going to go [0] -> [1] -> [2]
                    outValue.asList().pullBackRef(newListValue);
                }
                else 
                {
                    // Here we expand the list to support the new size (and then just grab the index)
                    outValue.asList().resizeVector(indexValue+1, false);
                    outValue.asList().getReferenceByIndex(indexValue, newListValue);
                }

                getValueByTags(tags, keyValue, newListValue, nullptr);
                return;
            }

            case EA::TDF::TDF_ACTUAL_TYPE_MAP:
            {
                char8_t keyBuffer[128];
                BLAZE_EASTL_STRING keyString = tags.front();
                tags.pop_front();

                // Parse the key into the correct type: 
                EA::TDF::TdfGenericValue mapKey;
                if (outValue.asMap().getKeyTypeDesc().isString())
                {
                    // Special case: The key may be encoded ("Foo Bar" is "Foo%20Bar" in HTTP)
                    HttpProtocolUtil::urlDecode(keyBuffer, sizeof(keyBuffer), keyString.c_str(), keyString.size(), true);
                    mapKey.set(keyBuffer);
                }
                else if (outValue.asMap().getKeyTypeDesc().isIntegral())
                {
                    int64_t keyInt = 0;
                    blaze_str2int(keyString.c_str(), &keyInt);
                    mapKey.set(keyInt);
                }
                else
                {
#ifndef BLAZE_CLIENT_SDK
                    BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding map for field " << memberName <<
                        " because the map does not support string or intregal keys.");
#endif
                    INC_ERROR_COUNT;
                    return;
                }

                // insert/get the reference: 
                EA::TDF::TdfGenericReference mapValueRef;
                outValue.asMap().insertKeyGetValue(mapKey, mapValueRef);

                getValueByTags(tags, keyValue, mapValueRef, nullptr);
                return;
            }

            case EA::TDF::TDF_ACTUAL_TYPE_UNION:
            {
                BLAZE_EASTL_STRING tagString = tags.front();
                tags.pop_front();

                const EA::TDF::TdfMemberInfo* tdfMemberInfo;
                EA::TDF::TdfGenericReference tdfValueRef;
                uint32_t memberIndex = EA::TDF::TdfUnion::INVALID_MEMBER_INDEX;

                bool foundMember = outValue.asUnion().getMemberInfoByName(tagString.c_str(), tdfMemberInfo) &&
                                   outValue.asUnion().getMemberIndexByInfo(tdfMemberInfo, memberIndex);

                if (foundMember)
                {
                    // If the member is complex, we may have partially decoded it while processing an earlier tag array.
                    // Don't set the active member again because this will clear the union's value.
                    if (outValue.asUnion().getActiveMemberIndex() == EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
                    {
                        outValue.asUnion().switchActiveMember(memberIndex);
                    }
                    else if (outValue.asUnion().getActiveMemberIndex() != memberIndex)
                    {
#ifndef BLAZE_CLIENT_SDK
                        const char8_t* curMemberName;
                        outValue.asUnion().getMemberNameByIndex(outValue.asUnion().getActiveMemberIndex(), curMemberName);
                        BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding union for field " << memberName <<
                            " with tag member name " << tagString << ". Union already has a different active member: " << curMemberName);
#endif
                        INC_ERROR_COUNT;
                        return;
                    }

                    // Now that we're sure the active member is set correctly, we can get its value.
                    // We can't do this sooner because getValueByName won't update tdfValueRef if the member
                    // whose value we're fetching is not the active member.
                    foundMember = outValue.asUnion().getValueByName(tagString.c_str(), tdfValueRef);
                }

                if (!foundMember)
                {
#ifndef BLAZE_CLIENT_SDK
                    BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding union for field " << memberName <<
                        " with tag member name " << tagString << ". Probably not a member of the union.");
#endif
                    INC_ERROR_COUNT;
                    return;
                }

                getValueByTags(tags, keyValue, tdfValueRef, tdfMemberInfo);
                return;
            }
            case EA::TDF::TDF_ACTUAL_TYPE_TDF:
            {
                BLAZE_EASTL_STRING tagString = tags.front();
                tags.pop_front();

                const EA::TDF::TdfMemberInfo* tdfMemberInfo;
                EA::TDF::TdfGenericReference tdfValueRef;
                if (useMemberName())
                {
                    if (!outValue.asTdf().getMemberInfoByName(tagString.c_str(), tdfMemberInfo) || 
                        !outValue.asTdf().getValueByName(tagString.c_str(), tdfValueRef) )
                    {
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding tdf for field " << memberName <<
                            " with tag member name " << tagString << " (not the tag). Probably not a member of the tdf.");
#endif
                        INC_ERROR_COUNT;
                        return;
                    }
                }
                else
                {
                    uint32_t tag = Heat2Util::makeTag(tagString.c_str());
                    if (!outValue.asTdf().getValueByTag(tag, tdfValueRef, &tdfMemberInfo, nullptr))
                    {
#ifndef BLAZE_CLIENT_SDK
                        BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder].visit: error decoding tdf for field " << memberName <<
                            " with tag member tag " << tagString << " (not the name). Probably not a member of the tdf.");
#endif
                        INC_ERROR_COUNT;
                        return;
                    }
                }

                getValueByTags(tags, keyValue, tdfValueRef, tdfMemberInfo);
                return;
            }
            default:
            {
#ifndef BLAZE_CLIENT_SDK
                BLAZE_WARN_LOG(Log::HTTP, "[HttpDecoder] Additional tags (starting with '" << tags.front().c_str() << "') found when parsing " << memberName << ". This type (" 
                    << outValue.getFullName() << ") does not support additional tags.");
#endif
                INC_ERROR_COUNT
                return;
            } 
        }
    }
}




} // Blaze

