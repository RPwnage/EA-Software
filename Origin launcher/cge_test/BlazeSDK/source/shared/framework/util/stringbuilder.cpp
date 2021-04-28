/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class StringBuilder

    This class represents a String.  It handles formatting
    data types based on the underlying DB implementation.  One primary goal of the class is to
    ensure that string parameters are properly escaped to prevent bogus string data from either
    causing the query to fail or cause unindented side affects (eg. SQL injection attacks).
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/util/shared/stringbuilder.h"
#include "framework/util/shared/rawbuffer.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#include "framework/util/timer.h"
#include "framework/redis/rediserrors.h"
#include "framework/redis/redisid.h"
#include "framework/controller/controller.h"
#endif

#include "EATDF/printencoder.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

StringBuilder::StringBuilder(const char8_t* str)
    : mBuffer(nullptr),
      mSize(DEFAULT_GROW_SIZE),
      mCount(0)
{
    mInternalBuffer[0] = '\0';
    mBuffer = &mInternalBuffer[0];

    // Prefix the query with a comment.  Use the provided comment if present and fall back on the
    // component and command name if available.
    if (str != nullptr)
    {
        append(str);
    }
}

StringBuilder::~StringBuilder()
{
    if (mBuffer != mInternalBuffer)
    {
        BLAZE_FREE_MGID(MEM_GROUP_FRAMEWORK_DEFAULT, mBuffer);
    }
}

/*************************************************************************************************/
/*!
     \brief append
 
      Append text to this string.  This function acts like printf.
  
     \param[in] format - Format string to append.
     \param[in] ...    - 0 or more arguments corresponding to substitution characters in the format
 
     \return - true on success; false on failure

     \notes If this function fails, the entire query will be truncated to zero length.  This is
            done to prevent invalid queries from being formed and executed if return values to
            this function are not honoured.
*/
/*************************************************************************************************/
bool StringBuilder::append(const char8_t* format, ...)
{
    if (format == nullptr || format[0] == '\0')
        return true;

    int32_t n = 0;
    bool overflow;
    do {
        overflow = false;
        size_t length = mSize - mCount;


        // Windows vsnprintf can't distinguish overflow from other err. Allow on windows StringBuilder to 'grow'
        // only on vsnprintf overflow of our designated max length for the StringBuilder by detecting whether
        // it overwrote the tail byte reserved for null below. Pre: mBuffer has mSize + 1 allocated.
        mBuffer[mSize] = '\0';
        const size_t tryWriteLength = length + 1;

        va_list args;
        va_start(args, format);
        n = vsnprintf(mBuffer + mCount, tryWriteLength, format, args);
        va_end(args);

        // overflow: on linux if (n over length), on windows if (mBuffer[mSize] != '\0')
        if ((n > (int32_t)length) || (mBuffer[mSize] != '\0'))
        {
            grow(mSize);
            overflow = true;
        }
        else if (n < 0)
        {
            mBuffer[mCount] = '\0';
            return false;
        }

    } while (overflow);

    mCount += n;
    return true;
}

bool StringBuilder::appendV(const char8_t* format, va_list args, size_t size)
{
    if (format == nullptr || format[0] == '\0')
        return true;

    // vsnprintf modifies the args values on Linux so we decide if we need to
    // grow the memory based on the size provided by caller
    size_t length = mSize - mCount;
    if (size >= length)
    {
        // grow by doubling the current size or the size of the string to append
        grow(mSize >= size ? mSize : size);
        length = mSize - mCount;
    }

    mBuffer[mSize] = '\0';
    const size_t tryWriteLength = length + 1;

    int32_t n = vsnprintf(mBuffer + mCount, tryWriteLength, format, args);

    if ((n > (int32_t)length) || (mBuffer[mSize] != '\0') || n < 0)
    {
        mBuffer[mCount] = '\0';
        return false;
    }

    mCount += n;
    return true;
}

void StringBuilder::appendN(const char8_t* buffer, size_t count)
{
    grow(count);
    strncpy(mBuffer + mCount, buffer, count);
    mCount += count;
    *(mBuffer + mCount) = '\0';
}


#define OPERATOR_IMPL(Type, Formatter) \
    StringBuilder& StringBuilder::operator<<(Type value) \
    { \
        if ((mSize - mCount) < 64) \
        { \
            if (!grow(64)) \
            { \
                mCount = 0; \
                return *this; \
            } \
        } \
        mCount += blaze_snzprintf(mBuffer + mCount, mSize - mCount, "%" Formatter, value); \
        return *this; \
    }

OPERATOR_IMPL(int8_t, PRId8);
OPERATOR_IMPL(int16_t, PRIi16);
OPERATOR_IMPL(int32_t, PRIi32);
OPERATOR_IMPL(int64_t, PRId64);
OPERATOR_IMPL(uint8_t, PRIu8);
OPERATOR_IMPL(uint16_t, PRIu16);
OPERATOR_IMPL(uint32_t, PRIu32);
OPERATOR_IMPL(uint64_t, PRIu64);
OPERATOR_IMPL(double, "f");
OPERATOR_IMPL(const void *, "p");

#if defined(EA_PLATFORM_IPHONE) || defined(EA_PLATFORM_OSX)
OPERATOR_IMPL(size_t, PRIu64);
#endif

StringBuilder& StringBuilder::operator<<(bool value)
{
    return (*this) << (value ? "true" : "false");
}

StringBuilder& StringBuilder::operator<<(char8_t value)
{
    if ((mSize - mCount) < 1) 
    { 
        if (!grow(1)) 
        {
            mCount = 0;             
            return *this; 
        } 
    } 

    mBuffer[mCount++] = value;
    mBuffer[mCount] = '\0';
    return *this;
}

StringBuilder& StringBuilder::operator<<(const SbFormats::TdfObject& value)
{
    EA::TDF::PrintEncoder enc;
    enc.setMaxEncodeLength(EA::TDF::PrintEncoder::MAX_OUTPUT_BUFFER_SIZE);

    EA::TDF::TdfGenericReferenceConst ref(value.mTdfObject);
    if (ref.getType() == EA::TDF::TDF_ACTUAL_TYPE_TDF)
        enc.print(*this, ref.asTdf(), value.mIndent, value.mTerse);          // This prints multi-line, same as before.
    else
        enc.print(*this, ref, value.mIndent, value.mTerse, value.mSeparator);    // Defaults to single line printing

    return (*this);
}

StringBuilder& StringBuilder::operator+(const char8_t* value)
{
    if (!value)
        value = "";
    size_t len = strlen(value);
    if ((mSize - mCount) < len)
    {
        if (!grow(len))
        {
            mCount = 0;
            return *this;
        }
    }
    memcpy(mBuffer + mCount, value, len);
    mCount += len;
    mBuffer[mCount] = '\0';

    return *this;
}

bool StringBuilder::pushString(const char8_t* buf, int bufLen /*= -1*/)
{
    if (bufLen < 0)
        (*this) << buf;
    else
        appendN(buf, bufLen);
    return true;
}

#ifndef BLAZE_CLIENT_SDK
StringBuilder& StringBuilder::operator<<(const RedisError& value)
{
    return (*this) << RedisErrorHelper::getName(value);
}

StringBuilder& StringBuilder::operator<<(const RedisErrorHelper& value)
{
    return (*this) << RedisErrorHelper::getName(value.getLastError()) << ": " << value.getLastErrorDescription();
}

StringBuilder& StringBuilder::operator<<(const RoutingOptions& value)
{
    switch (value.getType())
    {
    case RoutingOptions::INVALID:
        return (*this) << "invalid";
    case RoutingOptions::INSTANCE_ID:
        if (value.getAsInstanceId() == INVALID_INSTANCE_ID)
            return (*this) << "(invalid instance)";
        else if (value.getAsInstanceId() == gController->getInstanceId())
            return (*this) << "local:" << gController->getInstanceName();
        else
        {
            const RemoteServerInstance* instance = gController->getRemoteServerInstance(value.getAsInstanceId());
            if (instance != nullptr)
                return (*this) << "peer:" << instance->getInstanceName();
            else
                return (*this) << "peer:" << value.getAsInstanceId();
        }
    case RoutingOptions::SLIVER_IDENTITY:
        return (*this) << "sliver:" << GetSliverIdFromSliverIdentity(value.getAsSliverIdentity()) << "/" << BlazeRpcComponentDb::getComponentNameById(GetSliverNamespaceFromSliverIdentity(value.getAsSliverIdentity()));
    }

    return (*this) << "(unknown type)";
}

StringBuilder& StringBuilder::operator<<(const BlazeError& value)
{
    return (*this) << ErrorHelp::getErrorName(value);
}

StringBuilder& StringBuilder::operator<<(const Timer& value)
{
    char8_t intervalStr[64] = "";
    return (*this) << value.getInterval().toIntervalString(intervalStr, sizeof(intervalStr));
}

StringBuilder& StringBuilder::operator<<(const InetAddress& value)
{
    return (*this) << value.getIpAsString() << ":" << value.getPort(InetAddress::HOST);
}

StringBuilder& StringBuilder::operator<<(const SbFormats::InetAddress& value)
{
    char buf[256];
    value.mAddr.toString(buf, sizeof(buf), value.mFormat);
    return operator+(buf);
}

#endif

StringBuilder& StringBuilder::operator<<(const SbFormats::Raw& value)
{
    if ((mSize - mCount) < 64)
    {
        if (!grow(64))
        {
            mCount = 0;
            return *this;
        }
    }
    mCount += blaze_snzprintf(mBuffer + mCount, mSize - mCount, value.mFormat, value.mValue);
    return *this;
}

StringBuilder& StringBuilder::operator<<(const SbFormats::String& value)
{
    const char8_t* str = value.getFormattedString();
    return operator+(str);
}

StringBuilder& StringBuilder::operator<<(const SbFormats::StringAppender& value)
{
    value.append(*this);
    return (*this);
}

StringBuilder& StringBuilder::operator<<(const EA::TDF::TimeValue& value)
{
    char8_t timeStr[64] = "";
    return (*this) << value.toString(timeStr, sizeof(timeStr));
}

StringBuilder& StringBuilder::operator<<(const EA::TDF::ObjectId& value)
{
    EA::TDF::PrintEncoder enc;
    enc.setMaxEncodeLength(EA::TDF::PrintEncoder::MAX_OUTPUT_BUFFER_SIZE);
    EA::TDF::TdfGenericReferenceConst ref(value);
    enc.print(*this, ref);

    return (*this);
}

StringBuilder& StringBuilder::operator<<(const EA::TDF::ObjectType& value)
{
    EA::TDF::PrintEncoder enc;
    enc.setMaxEncodeLength(EA::TDF::PrintEncoder::MAX_OUTPUT_BUFFER_SIZE);
    EA::TDF::TdfGenericReferenceConst ref(value);
    enc.print(*this, ref);

    return (*this);
}

StringBuilder& StringBuilder::operator<<(const EA::TDF::TdfBitfield& value)
{
    return (*this) << value.getBits();
}

StringBuilder& StringBuilder::operator<<(const EA::TDF::TdfObject& value)
{
    EA::TDF::PrintEncoder enc(0, false);
    enc.setMaxEncodeLength(EA::TDF::PrintEncoder::MAX_OUTPUT_BUFFER_SIZE);

    EA::TDF::TdfGenericReferenceConst ref(value);
    if (ref.getType() == EA::TDF::TDF_ACTUAL_TYPE_TDF)
        enc.print(*this, ref.asTdf());          // This prints multi-line, same as before.
    else
        enc.print(*this, ref);    // Defaults to single line printing

    return (*this);
}

/*************************************************************************************************/
/*!
     \brief reset
 
     Clear the query buffer.
*/
/*************************************************************************************************/
StringBuilder& StringBuilder::reset()
{
    if (mBuffer != nullptr)
        mBuffer[0] = '\0';
    mCount = 0;

    return *this;
}

/*************************************************************************************************/
/*!
     \brief trim
 
     Remove the last n bytes off the end of the query buffer.
  
     \param[in] bytes - Number of bytes to remove from the end of the query.
*/
/*************************************************************************************************/
void StringBuilder::trim(size_t bytes)
{
    if (mBuffer == nullptr)
        return;

    if (bytes > mCount)
    {
        mCount = 0;
        mBuffer[0] = '\0';
    }
    else
    {
        mCount -= bytes;
        mBuffer[mCount] = '\0';
    }
}

StringBuilder& StringBuilder::trimIfEndsWith(char8_t ch)
{
    if (mBuffer != nullptr)
    {
        if (mCount > 0 && mBuffer[mCount - 1] == ch)
        {
            trim(1);
        }
    }
    return *this;
}

bool StringBuilder::grow(size_t bytesNeeded)
{
    if ((mSize - mCount) > bytesNeeded)
        return true;

    if (bytesNeeded < DEFAULT_GROW_SIZE)
        bytesNeeded = DEFAULT_GROW_SIZE;
    size_t newSize = mSize + bytesNeeded;
    char8_t* newBuf = (char8_t*) BLAZE_ALLOC_MGID(newSize + 1, MEM_GROUP_FRAMEWORK_DEFAULT, "StringBuilderBuf");

    if (newBuf == nullptr)
        return false;
    if (mBuffer != nullptr)
    {
        memcpy(newBuf, mBuffer, mCount);
        if (mBuffer != mInternalBuffer)
        {
            BLAZE_FREE_MGID(MEM_GROUP_FRAMEWORK_DEFAULT, mBuffer);
        }
    }
    mSize = newSize;
    mBuffer = newBuf;
    mBuffer[mCount] = '\0';
    return true;
}

void SbFormats::PrefixAppender::append(StringBuilder& sb) const
{
    const char8_t* pos = mText;
    while (*pos != '\0')
    {
        sb << mPrefix;

        size_t lineLen = strcspn(pos, "\r\n");
        sb.appendN(pos, lineLen);
        sb << '\n';

        pos += lineLen;
        pos += strspn(pos, "\r\n");
    }
}

} // namespace Blaze

