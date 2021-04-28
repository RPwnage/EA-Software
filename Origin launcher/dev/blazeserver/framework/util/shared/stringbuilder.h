/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRING_BUILDER_H
#define BLAZE_STRING_BUILDER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "framework/eastl_containers.h"

#ifndef BLAZE_CLIENT_SDK
#include "framework/connection/inetaddress.h"
#include "EASTL/utility.h"
#endif
#include "EASTL/string.h"
#include "EATDF/printencoder.h"
#include "EATDF/tdfobjectid.h"
#include "EATDF/time.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA { namespace TDF { class Tdf; class TdfObject; } }

namespace Blaze
{
class StringBuilder;

#ifndef BLAZE_CLIENT_SDK
struct RoutingOptions;
struct Timer;
struct BlazeError;
class RedisErrorHelper;
struct RedisError;
struct RedisId;
#endif

namespace SbFormats
{
    class BLAZESDK_API Raw
    {
    friend class Blaze::StringBuilder;
    public:
        Raw(const char8_t *format, int64_t value)
            : mFormat(format), mValue(value) { }
    private:
        const char8_t *mFormat;
        int64_t mValue;
    };

    class BLAZESDK_API HexUpper : public Raw
    {
    public:
        HexUpper(int64_t value) : Raw("0x%" PRIX64, value) { }
        HexUpper(int32_t value) : Raw("0x%" PRIX32, value) { }
        HexUpper(int16_t value) : Raw("0x%" PRIX16, value) { }
        HexUpper(int8_t value) : Raw("0x%" PRIX8, value) { }

        HexUpper(uint64_t value) : Raw("0x%" PRIX64, (int64_t)value) { }
        HexUpper(uint32_t value) : Raw("0x%" PRIX32, value) { }
        HexUpper(uint16_t value) : Raw("0x%" PRIX16, value) { }
        HexUpper(uint8_t value) : Raw("0x%" PRIX8, value) { }
    };

    class BLAZESDK_API HexLower : public Raw
    {
    public:
        HexLower(int64_t value) : Raw("0x%" PRIx64, value) { }
        HexLower(int32_t value) : Raw("0x%" PRIx32, value) { }
        HexLower(int16_t value) : Raw("0x%" PRIx16, value) { }
        HexLower(int8_t value) : Raw("0x%" PRIx8, value) { }

        HexLower(uint64_t value) : Raw("0x%" PRIx64, (int64_t)value) { }
        HexLower(uint32_t value) : Raw("0x%" PRIx32, value) { }
        HexLower(uint16_t value) : Raw("0x%" PRIx16, value) { }
        HexLower(uint8_t value) : Raw("0x%" PRIx8, value) { }
    };

    class BLAZESDK_API TdfObject
    {
    public:
        const EA::TDF::TdfObject& mTdfObject;
        bool mTerse = false;
        char8_t mSeparator = ' ';
        uint32_t mIndent = 0;

        TdfObject(const EA::TDF::TdfObject& obj, bool terse, uint32_t indent = 0, char8_t separator = ' ')
            : mTdfObject(obj), mTerse(terse), mSeparator(separator), mIndent(indent)
        {
        }
    };

#ifndef BLAZE_CLIENT_SDK
    class InetAddress
    {
    NON_COPYABLE(InetAddress);
    friend class Blaze::StringBuilder;
    public:
        InetAddress(const Blaze::InetAddress& addr, Blaze::InetAddress::AddressFormat format)
            : mAddr(addr),
              mFormat(format)
        {
        }

    private:
        const Blaze::InetAddress& mAddr;
        Blaze::InetAddress::AddressFormat mFormat;
    };
#endif

    class BLAZESDK_API String
    {
    public:
        virtual ~String() { }

        virtual const char8_t* getFormattedString() const = 0;
    };

    class BLAZESDK_API StringLiteral : String
    {
    public:
        StringLiteral(const char8_t* listeralStr) : mLiteralStr(listeralStr) {}
        const char8_t* getFormattedString() const override { return mLiteralStr; }
    private:
        const char8_t* mLiteralStr;
    };

    class BLAZESDK_API StringAppender
    {
    public:
        virtual ~StringAppender() { }

        virtual void append(StringBuilder& sb) const = 0;
    };

    class BLAZESDK_API PrefixAppender : public StringAppender
    {
    public:
        PrefixAppender(const char8_t* prefix, const char8_t* text) : mPrefix(prefix), mText(text) {}
        ~PrefixAppender() override { }

        void append(StringBuilder& sb) const override;
    private:
        const char8_t* mPrefix;
        const char8_t* mText;
    };
}

class BLAZESDK_API StringBuilder : public EA::TDF::PrintHelper
{
    NON_COPYABLE(StringBuilder);

public:
    StringBuilder(const char8_t* str = nullptr);
    ~StringBuilder() override;

    virtual bool append(const char8_t* format, ...);
    virtual bool appendV(const char8_t* format, va_list args, size_t size);
    virtual void appendN(const char8_t* buffer, size_t count);

    const char8_t* c_str() const { return mBuffer; }
    const char8_t* get() const { return c_str(); }
    size_t length() const { return mCount; }
    StringBuilder& reset();

    bool empty() const { return (mCount == 0); }
    bool isEmpty() const { return empty(); }

    // Trim the given number of types off the end of the buffer
    void trim(size_t bytes);
    StringBuilder& trimIfEndsWith(char8_t ch);

    // Base Types:
    virtual StringBuilder& operator<<(const char8_t* value) { return operator+(value); }
    virtual StringBuilder& operator<<(const BLAZE_EASTL_STRING& value) { return operator+(value.c_str()); }
    virtual StringBuilder& operator<<(bool value);
    virtual StringBuilder& operator<<(char8_t value);
    virtual StringBuilder& operator<<(int8_t value);
    virtual StringBuilder& operator<<(int16_t value);
    virtual StringBuilder& operator<<(int32_t value);
    virtual StringBuilder& operator<<(int64_t value);
    virtual StringBuilder& operator<<(uint8_t value);
    virtual StringBuilder& operator<<(uint16_t value);
    virtual StringBuilder& operator<<(uint32_t value);
    virtual StringBuilder& operator<<(uint64_t value);
    virtual StringBuilder& operator<<(double value);
    virtual StringBuilder& operator<<(const void * value);
    virtual StringBuilder& operator<<(const StringBuilder& value) { return (*this << value.get()); }

#if defined(EA_PLATFORM_IPHONE) || defined(EA_PLATFORM_OSX)
    // IPhone requires size_t to be defined differently from the unsigned long long (uint64)
    virtual StringBuilder& operator<<(size_t value);
#endif

#ifndef BLAZE_CLIENT_SDK
    // Server Only types:
    virtual StringBuilder& operator<<(const BlazeError& value);
    virtual StringBuilder& operator<<(const RoutingOptions& value);
    virtual StringBuilder& operator<<(const Timer& value);
    virtual StringBuilder& operator<<(const InetAddress& value);
    virtual StringBuilder& operator<<(const SbFormats::InetAddress& value);

    StringBuilder& operator<<(const RedisError& value);
    StringBuilder& operator<<(const RedisErrorHelper& value);
    template<typename A, typename B>
    StringBuilder& operator<<(const eastl::pair<A,B>& value)
    {
        operator<<(value.first);
        operator<<(':');
        operator<<(value.second);
        return *this;
    }
    virtual StringBuilder& operator<<(const Blaze::FixedString8& value) { return operator+(value.c_str()); }
    virtual StringBuilder& operator<<(const Blaze::FixedString16& value) { return operator+(value.c_str()); }
    virtual StringBuilder& operator<<(const Blaze::FixedString32& value) { return operator+(value.c_str()); }
    virtual StringBuilder& operator<<(const Blaze::FixedString64& value) { return operator+(value.c_str()); }
    virtual StringBuilder& operator<<(const Blaze::FixedString128& value) { return operator+(value.c_str()); }
    virtual StringBuilder& operator<<(const Blaze::FixedString256& value) { return operator+(value.c_str()); }

#endif

    virtual StringBuilder& operator<<(const SbFormats::Raw& value);
    virtual StringBuilder& operator<<(const SbFormats::String& value);
    virtual StringBuilder& operator<<(const SbFormats::StringAppender& value);
    virtual StringBuilder& operator<<(const SbFormats::TdfObject& value);

    // EA:TDF Types:
    virtual StringBuilder& operator<<(const EA::TDF::TimeValue& value);
    virtual StringBuilder& operator<<(const EA::TDF::ObjectId& value);
    virtual StringBuilder& operator<<(const EA::TDF::ObjectType& value);
    virtual StringBuilder& operator<<(const EA::TDF::TdfBitfield& value);

    virtual StringBuilder& operator<<(const EA::TDF::TdfObject& value);
    virtual StringBuilder& operator<<(const EA::TDF::TdfObject* value) { return (value != nullptr) ? (*this) << *value : (*this); }
    
    template <class T>
    StringBuilder& operator<<(const EA::TDF::tdf_ptr<T>& value) { return (value.get() != nullptr) ? (*this) << *value : (*this); }

    virtual StringBuilder& operator+(const char8_t* value);
    virtual StringBuilder& operator+=(const char8_t* value) { return operator+(value); }

    //tdf print helper interface
    bool pushChar(const char8_t c) override { (*this) << c; return true; }
    bool pushString(const char8_t* buf, int bufLen = -1) override;
    bool push(size_t size, const char8_t* format, ...) override
    {
        va_list args;
        va_start(args, format);
        bool result = appendV(format, args, size);
        va_end(args);

        return result;
    }

    bool pushVar(size_t size, const char8_t* format, va_list args) override { return appendV(format, args, size); }


protected:
    static const unsigned int DEFAULT_GROW_SIZE = 1024;

    char8_t* mBuffer;
    char8_t mInternalBuffer[DEFAULT_GROW_SIZE + 1];
    size_t mSize;
    size_t mCount;

    bool grow(size_t bytesNeeded = DEFAULT_GROW_SIZE);

};

} // namespace Blaze

#endif // BLAZE_STRING_BUILDER_H4
