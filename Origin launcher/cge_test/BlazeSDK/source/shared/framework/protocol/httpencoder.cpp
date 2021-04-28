/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "framework/blaze.h"
#include "framework/util/shared/base64.h"
#include "framework/util/shared/bytearrayinputstream.h"
#include "framework/util/shared/bytearrayoutputstream.h"
#include "framework/protocol/shared/httpencoder.h"
#include "framework/protocol/shared/heat2util.h"
#include "framework/protocol/shared/httpprotocolutil.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

//SDK does not use the standard "EA_ASSERT" so we redefine it here as "BlazeAssert"
#if !defined(EA_ASSERT) && defined(BlazeAssert)
#define EA_ASSERT(X) BlazeAssert(X)
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief HttpEncoder

    Construct a new XML encoder that encodes into the given buffer

    \param[in]  buffer - the buffer to encode into

*/
/*************************************************************************************************/
HttpEncoder::HttpEncoder()
    : mStateDepth(0),
      sSafeChars("-._~:?[]@!$&'()*,;=")  // These chars are always safe for URLs.  We don't include '+', which is encoded as "%2B", because the HTTP decoder assumes '+' == ' '.
{
}

/*************************************************************************************************/
/*!
    \brief ~HttpEncoder

    Destructor for HttpEncoder class.
*/
/*************************************************************************************************/
HttpEncoder::~HttpEncoder()
{
}

/*************************************************************************************************/
/*!
    \brief setBuffer

    Set the raw buffer that the XML response will be encoded into

    \param[in]  buffer - the buffer to decode from
*/
/*************************************************************************************************/
void HttpEncoder::setBuffer(RawBuffer* buffer)
{
    mBuffer = buffer;
    mStateDepth = 0;
    mKey[0] = '\0';
    mStateStack[0].state = STATE_NORMAL;


}

bool HttpEncoder::visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue)
{
    EA_ASSERT(mStateDepth == 0);

    tdf.visit(*this, tdf, tdf); 

    // trim the last char '&'
    if (*(mBuffer->tail() - 1) == '&')
    {
        *(mBuffer->tail() - 1) = '\0';
        mBuffer->trim(1);
    }
 
    if (isOk() == false)
    {
        ++mErrorCount;
    }

    return (mErrorCount == 0);
}


bool HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue)
{
    bool result = true;

    if (value.isValid())
    {
        pushKey(tag);
        pushStack(STATE_VARIABLE);

        mStateStack[mStateDepth].parseVariableTdfInfo = true;
        EA::TDF::TdfId tdfId = value.get()->getTdfId();      // Which variable TDF we're encoding
        pushNumericKey(tdfId);
        mStateStack[mStateDepth].parseVariableTdfInfo = false;

        result = visit(rootTdf, parentTdf, tag, *value.get(), *value.get());

        popStack();
        popKey();
    }

    return result;
}

bool HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue)
{
    bool result = true;

    if (value.isValid())
    {
        pushKey(tag);
        pushStack(STATE_VARIABLE);

        mStateStack[mStateDepth].parseVariableTdfInfo = true;
        EA::TDF::TdfId tdfId = value.get().getTdfId();      // Which generic TDF we're encoding
        pushNumericKey(tdfId);
        mStateStack[mStateDepth].parseVariableTdfInfo = false;

        visitReference(rootTdf, parentTdf, tag, value.get(), &value.get());

        popStack();
        popKey();
    }

    return result;
}

bool HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
{

    pushKey(tag);

    pushStack(STATE_NORMAL);

    value.visit(*this, rootTdf, referenceValue);

    popStack();
    popKey();

    if (isOk() == false)
    {
        ++mErrorCount;
    }

    return (mErrorCount == 0);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
{

    State parentState = mStateStack[mStateDepth].state;

    pushStack(STATE_ARRAY);

    mStateStack[mStateDepth].sizes = (uint32_t) value.vectorSize();
    mStateStack[mStateDepth].counts = 0;

    // First push the tag name of the array onto the key
    //  note: if this is a nested list, don't duplicate the tag.   proper fixup will occur in the final popKey() in this method.
    if (parentState == STATE_NORMAL)
    {
        pushTagKey(tag);
    }

    // Then start the index of the first dimension at 0
    pushNumericKey(0);

    value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);

    EA_ASSERT(mStateStack[mStateDepth].state == STATE_ARRAY);   

    // Ends the dimensions
    popRawKey();

    popStack();

    //  Ends the array or advances the parent array dimension.
    //  for STATE_ARRAY, this updates the most recent dimention inside the key value
    popKey();

    return;
}


void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
{
    State parentState = mStateStack[mStateDepth].state;

    pushStack(STATE_MAP);

    mStateStack[mStateDepth].sizes = static_cast<uint32_t>(value.mapSize());    // So we know how many elements to expect
    mStateStack[mStateDepth].counts = 0;                             // So far, no encoded elements
    mStateStack[mStateDepth].mapKey.clear();                                      // Next element encoded will be the key

    //  find all items with tag and generate map key list.  simplest way to do this
    //  is iterate through all parameters and pick out the ones needed.
    //  note: if this is a nested map, don't duplicate the tag.   proper fixup will occur in the final popKey() in this method.
    if (parentState == STATE_NORMAL)
    {
        pushTagKey(tag);
    }
     
    value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);

    EA_ASSERT(mStateStack[mStateDepth].state == STATE_MAP);
    EA_ASSERT(mStateStack[mStateDepth].counts == mStateStack[mStateDepth].sizes);
    EA_ASSERT(mStateStack[mStateDepth].mapKey.empty());


    // Update the stack
    popStack();

    //  Ends the array or advances the parent array dimension.
    //  for STATE_NORMAL, this reverses the pushTagKey() call at the head of this function
    //  for STATE_ARRAY and STATE_MAP, this updates the most recent dimention inside the key value
    popKey(); 
}

bool HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue)
{
    EA_ASSERT(mStateStack[mStateDepth].state != STATE_UNION);          // Can't have a union in a union

    State parentState = mStateStack[mStateDepth].state;

    pushStack(STATE_UNION);

    //  find all items with tag and generate map key list.  simplest way to do this
    //  is iterate through all parameters and pick out the ones needed.
    //  note: if this is a nested map, don't duplicate the tag.   proper fixup will occur in the final popKey() in this method.
    if (parentState == STATE_NORMAL)
    {
        pushTagKey(tag);
    }
       
    size_t initkeylen = strlen(mKey);
    const char8_t* memberName = nullptr;
    value.getMemberNameByIndex(value.getActiveMemberIndex(), memberName);
    size_t keylen = initkeylen + blaze_snzprintf(mKey + initkeylen, sizeof(mKey)-initkeylen, "%c%s", getNestDelim(), memberName);
    mKey[keylen] = '\0';

    value.visit(*this, rootTdf, value);

    EA_ASSERT(mStateStack[mStateDepth].state == STATE_UNION);

    mKey[initkeylen] = '\0';

    popStack();

    popRawKey();

    return isOk();
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%u", value ? 1 : 0);
    writePrimitive(tag, mChars);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%c", value);
    writePrimitive(tag, mChars);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%d", value);
    writePrimitive(tag, mChars);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%u", value);
    writePrimitive(tag, mChars);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%d", value);
    writePrimitive(tag, mChars);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%u", value);
    writePrimitive(tag, mChars);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%d", value);
    writePrimitive(tag, mChars);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue)
{
    uint32_t bits = value.getBits();
    visit(rootTdf, parentTdf, tag, bits, bits, 0);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%u", value);
    writePrimitive(tag, mChars);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%" PRIi64, value);
    writePrimitive(tag, mChars);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%" PRIu64, value);
    writePrimitive(tag, mChars);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%f", value);
    writePrimitive(tag, mChars);
}


void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
{
    writePrimitive(tag, value.c_str());
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue)
{
    size_t encodedSize = value.calcBase64EncodedSize();
    char8_t* encodeBuf = (char8_t*) BLAZE_ALLOC_MGID(encodedSize + 1, MEM_GROUP_FRAMEWORK_HTTP, "HTTP_BLOB_BUFFER");
    value.encodeBase64(encodeBuf, encodedSize + 1);
    writePrimitive(tag, encodeBuf);      
    BLAZE_FREE_MGID(MEM_GROUP_FRAMEWORK_HTTP, encodeBuf);
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue)
{
    bool useIdentifier = true;
    
#ifndef BLAZE_CLIENT_SDK
    if (mFrame->enumFormat == RpcProtocol::ENUM_FORMAT_VALUE)
    {
        useIdentifier = false;        
    }
#endif

    const char8_t *identifier = nullptr;
   
    if (EA_LIKELY(useIdentifier))
    {
        if (EA_LIKELY((enumMap != nullptr) && enumMap->findByValue(value, &identifier)))
            writePrimitive(tag, identifier);
        else
            writePrimitive(tag, "UNKNOWN");
    }
    else
    {
        // use standard int32_t visitor
        visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
    }
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue)
{
    writePrimitive(tag, value.toString().c_str());
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue)
{
    writePrimitive(tag, value.toString().c_str());
}

void HttpEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue &referenceValue, const TimeValue defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%" PRIi64, value.getMicroSeconds());
    writePrimitive(tag, mChars);
}

/*** Private Methods *****************************************************************************/

/*************************************************************************************************/
/*!
    \brief writePrimitive

    Called by any of the type-specific method to encode primitive values (ints or strings).

    \param[in] value - the tag of the value being encoded
    \param[in] len - the value being encoded as a human readable string

    \return - true if successful, false if any error occurred
*/
/*************************************************************************************************/
bool HttpEncoder::writePrimitive(uint32_t tag, const char8_t* value)
{

    if (mStateStack[mStateDepth].state == STATE_MAP)
    {
        // This is a map, it gets specially encoded because the key and value are 
        // passed in as two separate values.  We need to keep a little state to keep 
        // track of when we've got both the key and value before starting the element.
        if (mStateStack[mStateDepth].mapKey.empty())
        {
            // We're getting the key.  Save it so the next run through just has to spit 
            // out the data associated with the key.

            // Encode the key (specifically the next delimiter) so that we don't have to do it again.
            // (We don't encode anything else, since printString will handle the other characters)
            HttpProtocolUtil::urlencode(value, mStateStack[mStateDepth].mapKey);

            size_t len = strlen(mKey);
            size_t avail = sizeof(mKey) - len;

            int32_t written = (int32_t)avail;

            written = blaze_snzprintf(&(mKey[len]), avail, "%c%s%c", 
                getNestDelim(), 
                mStateStack[mStateDepth].mapKey.c_str(),
                '\0');

            if (written >= (int32_t)avail)
                return false;

            return true;
        }
    }
    
    if (pushKey(tag))
    {
        if (mKey[0] != '\0')
        {
            // We have already encoded the key at this point, and we don't want to encode the nestDelimiter character '|'.
            printString(mKey, true);
            printChar('=');
            printString(value);
            printChar('&');
        }

        popKey();
    }


    return (isOk());
}

/*************************************************************************************************/
/*!
    \brief isOk

    Utility method to check if the previous operation was successful.  Since all we're doing is
    writing XML data to a buffer, success is defined as there still being room in the buffer
    to write more data (it is assumed that this method will be called in all cases _except_ the
    last write to the buffer - that being the case even if the most recent write did not overflow
    the buffer it is still a failure if the buffer is just full because we can safely assume
    that more data needs to be written).

    \return - true if the buffer has room remaining, false otherwise
*/
/*************************************************************************************************/
bool HttpEncoder::isOk()
{
    if (mBuffer->tailroom() == 0)
    {
        ++mErrorCount;
        return false;
    }
    return true;
}

/*************************************************************************************************/
/*!
    \brief popStack

    Updates the state stack and sets whether we are back at the top level

    \return - true if state depth updated, false if we're already at the top level
*/
/*************************************************************************************************/
bool HttpEncoder::popStack()
{
    if (mStateDepth > 0)
    {
        --mStateDepth;

        return true;
    }
    else
    {
        return false;
    }
}

/*************************************************************************************************/
/*!
    \brief pushStack

    Pushes a new state onto the encoding stack.
*/
/*************************************************************************************************/
void HttpEncoder::pushStack(State state)
{
    ++mStateDepth;
    if (mStateDepth >= MAX_STATE_DEPTH)
    {
        //  perhaps we should extend the stack?  better than overwriting memory.
        EA_ASSERT(mStateDepth < MAX_STATE_DEPTH);
        mStateDepth = MAX_STATE_DEPTH-1;
        return;
    }

    mStateStack[mStateDepth].state = state;
    mStateStack[mStateDepth].parseVariableTdfInfo = false;
}


/*************************************************************************************************/
/*!
    \brief pushKey

    Push a tag onto the buffer that maintains the current param name.

    \param[in]  tag - the tag to push (in packed form)

    \return - true if successful, false if any error occurred
*/
/*************************************************************************************************/
bool HttpEncoder::pushKey(uint32_t tag)
{
    // Push the tag for normal types, but for arrays do nothing as the initial work
    // is done when setting up a dimension, and the index incrementing is handled in popKey
    if (mStateStack[mStateDepth].state == STATE_NORMAL)
    {
        return pushTagKey(tag);
    }
    else if (mStateStack[mStateDepth].state == STATE_MAP)
    {
        //  trying to read more elements than available - fail
        if (mStateStack[mStateDepth].counts == mStateStack[mStateDepth].sizes)
            return false;
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief pushNumericKey

    Push a numeric key onto the buffer that maintains the current param name.

    \param[in]  key - the numeric key

    \return - true if successful, false if any error occurred
*/
/*************************************************************************************************/
bool HttpEncoder::pushNumericKey(uint32_t key)
{
    size_t len = strlen(mKey);
    size_t avail = sizeof(mKey) - len;

    int32_t written = (int32_t)avail;

    if (mStateStack[mStateDepth].state == STATE_ARRAY)
    {
        // We assume when pushing an index key that since it inside an array there must
        // already be some data in mKey and hence it is safe to always precede it with 
        // the separator char
        written = blaze_snzprintf(&(mKey[len]), avail, "%c%" PRIu32 "%c", getNestDelim(), key, '\0');
    }
    else if (mStateStack[mStateDepth].state == STATE_VARIABLE)
    {
        // When pushing a TdfId key, we assume it's safe to precede it with the 
        // separator char because the associated variable tdf's tag should
        // already be in mKey
        written = blaze_snzprintf(&(mKey[len]), avail, "%c%" PRIu32 "", getNestDelim(), key);
    }

    //  if neither state if block was hit above, error out (unhandled state!)
    if (written >= (int32_t)avail)
        return false;

    return true;
}

/*************************************************************************************************/
/*!
    \brief pushTagKey

    Push the string representation of a tag onto the buffer that maintains the current param name.

    \param[in]  tag - the tag to push (in packed form)

    \return - true if successful, false if any error occurred

*/
/*************************************************************************************************/
bool HttpEncoder::pushTagKey(uint32_t tag)
{
    size_t len = strlen(mKey);

    // Need at least 6 chars: 1 for separator, 4 for tag, 1 for null-terminator
    if ((sizeof(mKey) - len) < 6)
    {
        ++mErrorCount;
        return false;
    }

    if (len != 0)
        mKey[len++] = getNestDelim();
    Heat2Util::decodeTag(tag, &(mKey[len]), (uint32_t)(sizeof(mKey) - len), true);

    return true;
}

/*************************************************************************************************/
/*!
    \brief popKey

    Pop the last key off of the buffer that maintains the current param name.

    \return - true if successful, false if any error occurred

*/
/*************************************************************************************************/
bool HttpEncoder::popKey()
{    
    int32_t effectiveStateDepth = mStateDepth;
    bool parseVariableTdfInfo = false;
    if (mStateStack[effectiveStateDepth].state == STATE_VARIABLE && effectiveStateDepth > 0)
    {
        parseVariableTdfInfo = mStateStack[effectiveStateDepth].parseVariableTdfInfo;
        --effectiveStateDepth;
    }

    switch (mStateStack[effectiveStateDepth].state)
    {
    case STATE_ARRAY:
        {
            if (!parseVariableTdfInfo)
            {
                // At this point we are popping an array index, so remove the previous index,
                // and push the new one onto the key
                popRawKey();
                mStateStack[effectiveStateDepth].counts++;
                return pushNumericKey(mStateStack[effectiveStateDepth].counts);
            }
            return true;
        }
    case STATE_MAP:
        {            
            if (!parseVariableTdfInfo)
            {
                if (!mStateStack[effectiveStateDepth].mapKey.empty())
                {
                    ++mStateStack[effectiveStateDepth].counts;
                    mStateStack[effectiveStateDepth].mapKey.clear();
                }
                return popRawKey();
            }
            return true;
        }
    default:
        return popRawKey();
    }
}

/*************************************************************************************************/
/*!
    \brief popRawKey

    Pop the last key off of the buffer that maintains the current param name.

    \return - true if successful, false if any error occurred

*/
/*************************************************************************************************/
bool HttpEncoder::popRawKey()
{
    State effectiveState = mStateStack[mStateDepth].state;
    if (effectiveState == STATE_VARIABLE && mStateDepth > 0)
    {
        effectiveState = mStateStack[mStateDepth-1].state;
    }

    char8_t* last = nullptr;
    switch (effectiveState)
    {
    case STATE_ARRAY:
    case STATE_MAP:
    case STATE_NORMAL:
        last = strrchr(mKey, getNestDelim());
        break;
    default:
        break;
    }

    if (last == nullptr)
    {
        // If we can't find a '|' then we're trying to pop the only key,
        // but if there is no key to pop at all then that's an error
        if (mKey[0] == '\0')
        {
            ++mErrorCount;
            return false;
        }
        else
        {
            last = mKey;
        }
    }
    *last = '\0';
    return true;
}

/**
  \brief Write a null-terminated string to the output buffer.
  \param s string to write.
*/
void HttpEncoder::printString(const char *s, bool skipSafeCheck)
{
    printString(s, strlen(s), skipSafeCheck);
}

/**
  \brief Write a string to the output buffer.
  \param s string to write.
  \param len length of string to write.
*/
void HttpEncoder::printString(const char *s, size_t len, bool skipSafeCheck)
{
    while(len-- > 0)
    {
        printChar(*(s++), skipSafeCheck);
    }
}


/**
  \brief Write one character to the output buffer. (Convert to %-format if not a safe character)
  \param c character to write.
*/
void HttpEncoder::printChar(unsigned char c, bool skipSafeCheck)
{
    // Copied from HttpProtocolUtil::urlencode (which we don't use directly because it doesn't build a buffer)
    if (skipSafeCheck || isalnum(c) ||
        sSafeChars.find(c) < sSafeChars.length())
    {
        //Char is safe because it is alphanumeric, safe, or one of the not-to-encode chars.
        uint8_t* t = mBuffer->acquire(2);
        if (t != nullptr)
        {
            t[0] = c;
            t[1] = '\0';

            mBuffer->put(1);
        }
    }
    else
    {
        uint8_t* t = mBuffer->acquire(4);
        if (t != nullptr)
        {
            t[0] = '%';

            // Basically, we do a quick binary to character hex conversion here.
            uint8_t nTemp = (uint8_t)c >> 4;  // Get first hex digit
            nTemp = '0' + nTemp;       // Shift it to be starting at '0' instead of 0.
            if(nTemp > '9')            // If digit needs to be in 'A' to 'F' range.
                nTemp += 7;                     // Shift it to the 'A' to 'F' range.
            t[1] = (char8_t)nTemp;

            nTemp = (uint8_t)c & 0x0f;
            nTemp = '0' + nTemp;
            if(nTemp > '9')
                nTemp += 7;
            t[2] = (char8_t)nTemp;
            t[3] = '\0';

            mBuffer->put(3);
        }
    }
}

} // Blaze


