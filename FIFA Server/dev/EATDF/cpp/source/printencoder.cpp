/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class PrintEncoder

    This encoder generates a human-readable text format.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <EATDF/tdfbitfield.h>
#include <EATDF/tdfblob.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfunion.h>
#include <EATDF/tdfbasetypes.h>
#include <EATDF/tdfvariable.h>
#include <EATDF/tdfvector.h>
#include <EATDF/printencoder.h>
#include <EAAssert/eaassert.h>

#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAHashString.h>


namespace EA
{
namespace TDF
{
 

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#if EA_TDF_INCLUDE_EXTENDED_TAG_INFO
static const char8_t* CENSORED_STR = "<CENSORED>";
#endif
static const int32_t INDENT_WIDTH = 2;
static const int32_t MAX_INDENT_LENGTH = 256;
static const char8_t INDENT_BUFFER[MAX_INDENT_LENGTH + 1] = {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    '\0'
};

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief PrintEncoder

    Construct a new print encoder that encodes into the given buffer

    \param[in]  buffer - the buffer to encode into

*/
/*************************************************************************************************/
PrintEncoder::PrintEncoder(int32_t indentLength, bool terse /* = false */)
    : mIndent(indentLength),  
      mFull(false),
      mObjectName(nullptr),
      mCurrentState(nullptr),
      mPrinter(nullptr),
      mTerse(terse),
      mEntrySeparator(terse ? ' ' : '\n'),
      mMaxEncodeLength((uint32_t)-1)
{
}

/*************************************************************************************************/
/*!
    \brief ~PrintEncoder

    Destructor for PrintEncoder class.
*/
/*************************************************************************************************/
PrintEncoder::~PrintEncoder()
{
}

bool PrintEncoder::print(PrintHelper& printer, const Tdf& tdf, int32_t indent, bool terse)
{
    mPrinter = &printer;
    mIndent = indent;
    mTerse = terse;
    mEntrySeparator = (terse ? ' ' : '\n');

    return visitTdfBase(tdf);
}

bool PrintEncoder::print(PrintHelper& printer, TdfGenericReferenceConst& ref, int32_t indent, bool terse, char8_t entrySeperator)
{
    mPrinter = &printer;
    mIndent = indent;
    mTerse = terse;
    mEntrySeparator = entrySeperator;

    return visitValue(ref);
}

static const size_t MAX_TAG_LEN = 4;
static int32_t decodeTag(uint32_t tag, char *buf, uint32_t len, const bool convertToLowercase = false)
{
    if (buf == nullptr || len < 4)
        return 0;

    int32_t size = 4;
    tag >>= 8;
    for (int32_t i = 3; i >= 0; --i)
    {
        uint32_t sixbits = tag & 0x3f;
        if (sixbits)
            buf[i] = (char)(sixbits + 32);
        else
        {
            buf[i] = '\0';
            size = i;
        }
        tag >>= 6;
    }
    buf[4] = '\0';

    if (convertToLowercase)
    {
        for (uint32_t i = 0; i < len; ++i)
        {
            buf[i] = static_cast<char>(tolower(buf[i]));
        }
    }

    return size;
}


bool PrintEncoder::outputData(const TdfMemberInfo* memberInfo, int32_t size, const char8_t* type, const char8_t* format, ...)
{
    int32_t dataSize = size;

    if ((uint32_t)dataSize > mMaxEncodeLength)
        dataSize = mMaxEncodeLength;

#if EA_TDF_INCLUDE_EXTENDED_TAG_INFO
    if (memberInfo != nullptr)
    {
        if (memberInfo->printFormat == TdfMemberInfo::CENSOR)
        {
            return pushString(CENSORED_STR, (int32_t) EA_TDF_STRLEN(CENSORED_STR));
        }
        else if (memberInfo->printFormat == TdfMemberInfo::HASH)
        {
            char8_t originalData[512];
            char8_t hash[20]; //64BIT HEX

            va_list args;
            va_start(args, format);
            EA::StdC::Vsnprintf(originalData, sizeof(originalData), format, args);
            va_end(args);

            uint64_t hashVal = EA::StdC::FNV64_String8(originalData);
            int32_t len = EA::StdC::Snprintf(hash, sizeof(hash), "0x%016" PRIX64, hashVal);

            return pushString(hash, len);
        }
        else if (memberInfo->printFormat == TdfMemberInfo::LOWER)
        {
            char8_t originalData[512];

            va_list args;
            va_start(args, format);
            int32_t len = EA::StdC::Vsnprintf(originalData, sizeof(originalData), format, args);
            va_end(args);

            EA::StdC::Strlwr(originalData);
            return pushString(originalData, len);
        }
        else
        {
            EA_ASSERT_MSG(memberInfo->printFormat == TdfMemberInfo::NORMAL, "Unhandled format type.");
        }
    }
#endif

    va_list args;
    va_start(args, format);
    mFull = !mPrinter->pushVar(dataSize + 1, format, args);
    va_end(args);

    return !mFull;
}

bool PrintEncoder::visitValue(TdfGenericReferenceConst& ref, const TdfMemberInfo* memberInfo)
{
#if EA_TDF_INCLUDE_EXTENDED_TAG_INFO
    if (memberInfo && memberInfo->printFormat == TdfMemberInfo::CENSOR)
        return pushString(CENSORED_STR, (int32_t)EA_TDF_STRLEN(CENSORED_STR));
#endif

    switch (ref.getType())
    {
    case TDF_ACTUAL_TYPE_TDF:
        return visitTdf(ref.asTdf());
    case TDF_ACTUAL_TYPE_UNION:
        return visitTdfUnion(ref.asUnion());

    case TDF_ACTUAL_TYPE_BOOL:
    {
        static const int32_t MAX_BOOL_STRING = 16;
        return outputData(memberInfo, MAX_BOOL_STRING, "bool", "%s", ref.asBool() ? "true" : "false");
    }
    case TDF_ACTUAL_TYPE_INT8:
    {
        static const int32_t MAX_INT8_STRING = 12;   // Example: -127 (0x80)
        return outputData(memberInfo, MAX_INT8_STRING, "int8_t", mTerse ? "%d" : "%d (0x%02X)", ref.asInt8(), ref.asInt8());
    }
    case TDF_ACTUAL_TYPE_UINT8:
    {
        static const int32_t MAX_UINT8_STRING = 11;  // Example: 255 (0xFF)
        return outputData(memberInfo, MAX_UINT8_STRING, "uint8_t", mTerse ? "%u" : "%u (0x%02X)", ref.asUInt8(), ref.asUInt8());
    }
    case TDF_ACTUAL_TYPE_INT16:
    {
        static const int32_t MAX_INT16_STRING = 16; // Example: -32768 (0x8000)
        return outputData(memberInfo, MAX_INT16_STRING, "int16_t", mTerse ? "%d" : "%d (0x%04X)", ref.asInt16(), ref.asInt16());
    }
    case TDF_ACTUAL_TYPE_UINT16:
    {
        static const int32_t MAX_UINT16_STRING = 15; // Example: 65535 (0xFFFF)
        return outputData(memberInfo, MAX_UINT16_STRING, "uint16_t", mTerse ? "%u" : "%u (0x%04X)", ref.asUInt16(), ref.asUInt16());
    }
    case TDF_ACTUAL_TYPE_BITFIELD:
    {
        uint32_t bits = ref.asBitfield().getBits();
        static const int32_t MAX_UINT32_STRING = 24;    // Example: 4294967295 (0xFFFFFFFF)
        return outputData(memberInfo, MAX_UINT32_STRING, "bitfield", mTerse ? "%u" : "%u (0x%08X)", bits, bits);
    }
    case TDF_ACTUAL_TYPE_INT32:
    {
        static const int32_t MAX_INT32_STRING = 25; // Example: -2147483648 (0x80000000)
        return outputData(memberInfo, MAX_INT32_STRING, "int32_t", mTerse ? "%d" : "%d (0x%08X)", ref.asInt32(), ref.asInt32());
    }
    case TDF_ACTUAL_TYPE_UINT32:
    {
        static const int32_t MAX_UINT32_STRING = 24;    // Example: 4294967295 (0xFFFFFFFF)
        return outputData(memberInfo, MAX_UINT32_STRING, "uint32_t", mTerse ? "%u" : "%u (0x%08X)", ref.asUInt32(), ref.asUInt32());
    }
    case TDF_ACTUAL_TYPE_INT64:
    {
        static const int32_t MAX_INT64_STRING = 42; // Example: -9223372036854775808 (0x8000000000000000)
        return outputData(memberInfo, MAX_INT64_STRING, "int64_t", mTerse ? "%" PRId64 : "%" PRId64 " (0x%016" PRIX64 ")", ref.asInt64(), ref.asInt64());
    }
    case TDF_ACTUAL_TYPE_UINT64:
    {
        static const int32_t MAX_UINT64_STRING = 42;    // Example: 18446744073709551615 (0xFFFFFFFFFFFFFFFF)
        return outputData(memberInfo, MAX_UINT64_STRING, "uint64_t", mTerse ? "%" PRIu64 : "%" PRIu64 " (0x%016" PRIX64 ")", ref.asUInt64(), ref.asUInt64());
    }
    case TDF_ACTUAL_TYPE_FLOAT:
    {
        static const int32_t MAX_FLOAT_STRING = 50;    // Example: -3.40282E+38
        return outputData(memberInfo, MAX_FLOAT_STRING, "float", "%.8f", ref.asFloat()); // Arson has dependencies on this format
    }
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:
    {
        char8_t buf[256];
        ref.asObjectType().toString(buf, sizeof(buf));
        return outputData(memberInfo, 256, "BlazeObjectType", "%s", buf);
    }
    case TDF_ACTUAL_TYPE_OBJECT_ID:
    {
        char8_t buf[256];
        ref.asObjectId().toString(buf, sizeof(buf));
        return outputData(memberInfo, 256, "BlazeObjectId", "%s", buf);
    }
    case TDF_ACTUAL_TYPE_TIMEVALUE:
    {
        static const int32_t MAX_TIMEVALUE_STRING = 42; // Example: -9223372036854775808 (0x8000000000000000)
        return outputData(memberInfo, MAX_TIMEVALUE_STRING, "TimeValue", mTerse ? "%" PRId64 : "%" PRId64 " (0x%016" PRIX64 ")", ref.asTimeValue().getMicroSeconds(), ref.asTimeValue().getMicroSeconds());
    }
    case TDF_ACTUAL_TYPE_STRING:
    {
        TdfMemberInfo::PrintFormat format = TdfMemberInfo::NORMAL;
#if EA_TDF_INCLUDE_EXTENDED_TAG_INFO
        if (memberInfo)
            format = memberInfo->printFormat;
#endif
        bool addQuotes = ((format == TdfMemberInfo::NORMAL) || (format == TdfMemberInfo::LOWER));
        // wrap the data value with quotes if the print format is neither hash nor censor
        if (addQuotes)
            return outputData(memberInfo, (int32_t)(1 + ref.asString().length() + 1), "string", "\"%s\"", ref.asString().c_str());

        return outputData(memberInfo, (int32_t)(ref.asString().length()), "string", "%s", ref.asString().c_str());
    }

    case TDF_ACTUAL_TYPE_ENUM:
    {
        const TypeDescriptionEnum* enumMap = ref.getTypeDescription().asEnumMap();

        // need to find value in enum map
        char8_t buf[16];
        const char8_t *enumValueString = nullptr;
        if (EA_LIKELY(enumMap != nullptr))
            enumMap->findByValue(ref.asEnum(), &enumValueString);
        else
        {
            EA::StdC::Snprintf(buf, sizeof(buf), "%" PRIi32, ref.asEnum());
            enumValueString = buf;
        }
        int32_t len = (int32_t)EA_TDF_STRLEN(enumValueString) + 32; // Include space for the int formats
        return outputData(memberInfo, len, "enumeration", mTerse ? "%s/%d" : "%s (%d) (0x%08X)", enumValueString, ref.asEnum(), ref.asEnum());

    }

    case TDF_ACTUAL_TYPE_LIST:
    {
        auto& listValue = ref.asList();
        auto listSize = listValue.vectorSize();
        if (listSize == 0)
            return pushString("[]", 2);

        // "[\n"
        if (!pushChar('[') || !outputLine())
            return false;

        ++mIndent;
        for (size_t index = 0; index < listSize; ++index)
        {
            TdfGenericReferenceConst listMemberRef;
            if (!listValue.getValueByIndex(index, listMemberRef))
                return false;

            char buf[32];
            int len = EA::StdC::Snprintf(buf, sizeof(buf), "[%d] = ", index);

            //   [#] = (value)\n
            if (!indent() || !pushString(buf, len) || !visitValue(listMemberRef, memberInfo) || !outputLine())
                return false;
        }
        --mIndent;

        // "  ]"
        if (!indent() || !pushChar(']'))
            return false;

        return true;
    }

    case TDF_ACTUAL_TYPE_MAP:
    {
        auto& mapValue = ref.asMap();
        auto mapSize = mapValue.mapSize();
        if (mapSize == 0)
            return pushString("[]", 2);

        // "[\n"
        if (!pushChar('[') || !outputLine())
            return false;

        ++mIndent;
        for (size_t index = 0; index < mapSize; ++index)
        {
            TdfGenericReferenceConst mapKeyRef;
            TdfGenericReferenceConst mapValueRef;
            if (!mapValue.getValueByIndex(index, mapKeyRef, mapValueRef))
                return false;

            // "  (key, value"
            if (!indent() || !pushChar('(') || !visitValue(mapKeyRef, memberInfo) || !pushString(", ", 2) || !visitValue(mapValueRef, memberInfo))
                return false;

            //  ")\n"
            if (!pushChar(')') || !outputLine())
                return false;
        }
        --mIndent;

        // "  ]"
        if (!indent() || !pushChar(']'))
            return false;

        return true;
    }

    case TDF_ACTUAL_TYPE_VARIABLE:
    {
        auto& variableType = ref.asVariable();
        if (!variableType.isValid())
            return pushString("{}", 2);

        //  "{\n"
        if (!pushChar('{') || !outputLine())
            return false;

        auto& tdfValue = *variableType.get();
        const char8_t *objectName = tdfValue.getClassName();

        ++mIndent;
        //   "className = (tdf)"  
        if (!indent() || !pushString(objectName) || !pushString(" = ", 3) || !visitTdf(tdfValue) || !outputLine())
            return false;
        --mIndent;

        //   "}"
        if (!indent() || !pushChar('}'))
            return false;

        return true;
    }

    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
    {
        auto& genericType = ref.asGenericType();
        if (!genericType.isValid())
            return pushString("{}", 2);

        TdfGenericReferenceConst genericRef(genericType.getValue());
        return visitValue(genericRef, memberInfo);
    }

    case TDF_ACTUAL_TYPE_BLOB:
    {
        auto& blobValue = ref.asBlob();
        auto blobBytes = blobValue.getCount();
        if (blobBytes == 0)
            return pushString("{}", 2);

        // "{\n"
        if (!pushString("{") || !outputLine())
            return false;

        uint8_t* valueData = blobValue.getData();
        ++mIndent;

        // Outputting 16 characters per line. 
        // 4 bytes are consumed in the line buffer for each character:
        //  * 2 bytes to hold the ASCII representation of the corresponding 2-digit hex value
        //  * 1 byte to hold the ASCII blank space between the 2-digit hex values
        //  * 1 byte to hold the ASCII representation of the character
        uint32_t maxLogSize = eastl::min<uint32_t>((uint32_t)blobBytes, (uint32_t)(mMaxEncodeLength / 4.25f)); // We need to divide the max size because we acquire 68 bytes for every 16 we encode
        for (uint32_t idx = 0; idx < maxLogSize; idx += 16)
        {
            char8_t buf[(4 * 16) + 4];
            int32_t count = (int32_t) (maxLogSize - idx);
            if (count > 16)
                count = 16;

            count = printBinary(buf, sizeof(buf), &valueData[idx], count, 0, mEntrySeparator);

            //   "(binary)\n"
            if (!indent() || !pushString(buf, count))
                return false;
        }
        mIndent--;

        // "}"
        if (!indent() || !pushChar('}'))
            return false;

        return true;
    }
    default:
        break;
    }
    return false;
}

bool PrintEncoder::addName(const TdfMemberIteratorConst& memberIter)
{
    const char8_t* memberName = memberIter.getInfo()->getMemberName();
    if (memberName != nullptr)
    {
        return pushString(memberName);
    }

    char8_t tagBuf[MAX_TAG_LEN + 1];
    decodeTag(memberIter.getInfo()->getTag(), tagBuf, sizeof(tagBuf));
    return pushString(tagBuf);
}

// Return value == Fatal print encoding error (ex. Buffer full)
bool PrintEncoder::visitTdfBase(const Tdf &value)
{
    // "tdfName = "
    if (!outputLine(mTerse) || !indent() || !pushString(value.getClassName()) || !pushString(" = ", 3) || !visitTdf(value) || !outputLine(mTerse))
        return false;
    return true;
}

bool PrintEncoder::visitTdf(const Tdf &value)
{
    // "{\n"
    if (!pushChar('{') || !outputLine())
        return false;

    ++mIndent;
    TdfMemberIteratorConst iter(value);  // Note: Iterator starts at -1, so we have to call next() to get a valid value
    while (iter.next())
    {
        // "  memberName = (value)"
        if (!indent() || !addName(iter) || !pushString(" = ", 3) || !visitValue(iter, iter.getInfo()) || !outputLine())
            return false;
    }
    mIndent--;

    // "}\n"
    if (!indent() || !pushChar('}'))
        return false;

    return true;
}


bool PrintEncoder::visitTdfUnion(const TdfUnion &value)
{
    EA_ASSERT(value.getActiveMemberIndex() <= INT8_MAX);

    int32_t len = 0;
    char8_t buf[32];
    if (value.getActiveMemberIndex() == TdfUnion::INVALID_MEMBER_INDEX)
    {
        len = EA::StdC::Snprintf(buf, sizeof(buf), "(union : UNSET) = {}");
        return pushString(buf, len);
    }

    len = EA::StdC::Snprintf(buf, sizeof(buf), "(union : %d) = {", value.getActiveMemberIndex());

    // " (union : #) = {\n"
    if (!pushString(buf, len) || !outputLine())
        return false;

    if (value.getActiveMemberIndex() != TdfUnion::INVALID_MEMBER_INDEX)
    {
        TdfMemberIteratorConst iter(value);
        iter.moveTo(value.getActiveMemberIndex());

        ++mIndent;
        // "  memberName = (value)"
        if (!indent() || !addName(iter) || !pushString(" = ", 3) || !visitValue(iter, iter.getInfo()) || !outputLine())
            return false;

        mIndent--;
    }

    // "}\n"
    if (!indent() || !pushChar('}'))
        return false;

    return true;
}


bool PrintEncoder::indent()
{
    if (mTerse)
        return true;

    int32_t indentLen = mIndent * INDENT_WIDTH;
    if (indentLen > MAX_INDENT_LENGTH)
        indentLen = MAX_INDENT_LENGTH;

    if (indentLen == 0)
        return true;

    return pushString(INDENT_BUFFER, indentLen);
}



bool PrintEncoder::outputLine(bool skip)
{
    return skip ? true : pushChar(mEntrySeparator);
}

bool PrintEncoder::pushChar(const char8_t c)
{
    mFull = !mPrinter->pushChar(c);
    return !mFull;
}

bool PrintEncoder::pushString(const char8_t* buf, int bufLen)
{
    mFull = !mPrinter->pushString(buf, bufLen);
    return !mFull;
}

//  inline function to convert an 8-bit value to its high and low hex ascii chars.
static inline void _char2hex(char8_t &h, char8_t &l, char8_t ch)
{
    static const char8_t* HEX = "0123456789abcdef";
    h = HEX[((uint8_t)ch) >> 4];
    l = HEX[((uint8_t)ch) & 0xf];
}

int32_t PrintEncoder::printBinary(char8_t* dst, size_t len, const uint8_t* src, uint32_t srclen, int32_t indent, char8_t entrySeparator)
{
    static const int32_t MAX_INDENT = 32;
    static const int32_t BYTES_PER_LINE = 16;
    char8_t line[64 + (MAX_INDENT * 2) + 2]; // binary data + 32 levels of indent (+2 for '\n' and '\0')
    char8_t* ascii;
    char8_t* bin;
    int32_t out = 0;

    // Add indentation to the line buffer and setup the binary and ascii output pointers
    if (indent > 0)
    {       
        if (indent > MAX_INDENT)
            indent = MAX_INDENT;
        memset(line, ' ', static_cast<size_t>(indent * 2)); 
        bin = line + (indent * 2);
    }
    else
    {
        bin = line + (indent * 2);
    }
    ascii = bin + (BYTES_PER_LINE * 3);

    // Add fixed whitespace to the line buffer
    int32_t lineIdx = 0;
    for(lineIdx = 0; lineIdx < BYTES_PER_LINE; lineIdx++)
    {
        bin[(lineIdx * 3) + 2] = ' ';
        ascii[lineIdx] = ' ';
    }
    lineIdx = 0;
    ascii[BYTES_PER_LINE] = entrySeparator;
    ascii[BYTES_PER_LINE + 1] = '\0';

    for(uint32_t srcIdx = 0; srcIdx < srclen; srcIdx++)
    {
        // Output byte as 2-digit hex
        _char2hex(bin[(lineIdx * 3)], bin[(lineIdx * 3) + 1], src[srcIdx]);

        // Only output ascii if it is a printable character
        ascii[lineIdx] = isprint(src[srcIdx]) ? src[srcIdx] : '.';

        lineIdx++;
        if (lineIdx == BYTES_PER_LINE)
        {
            // Output a line
            lineIdx = 0;
            int32_t numWritten = EA::StdC::Snprintf(dst + out, len - out, "%s", line);
            if (numWritten > (int32_t) len - out)
            {
                //we're full up, bail
                return (int32_t)len;
            }
            else
            {
                out += numWritten;
            }
        }
    }

    // IF the final line hasn't been output yet
    if (lineIdx > 0)
    {
        for(; lineIdx < BYTES_PER_LINE; lineIdx++)
        {
            bin[(lineIdx * 3)] = ' ';
            bin[(lineIdx * 3) + 1] = ' ';
            ascii[lineIdx] = ' ';
        }
        int32_t numWritten = EA::StdC::Snprintf(dst + out, len - out, "%s", line);
        if (numWritten > (int32_t) len - out)
        {
            //we're full up, bail
            return (int32_t)len;
        }
        else
        {
            out += numWritten;
        }
    }
    return out;
}

TdfPrintHelperCharBuf::TdfPrintHelperCharBuf(char8_t* buf, size_t bufSize) : mBuf(buf),
mSize(bufSize)
{

    reset();
}

bool TdfPrintHelperCharBuf::pushVar(size_t size, const char8_t* format, va_list args)
{
    // Vsnprintf will always add a '\0' at the end, but may return > mBufLeft, or a negative number.
    int numPrinted = EA::StdC::Vsnprintf(mCurPos, mBufLeft, format, args);
    if (numPrinted < 0)
        return false;
    
    if ((size_t)numPrinted <= mBufLeft)
    {
        mBufLeft -= (size_t)numPrinted;
        mCurPos += numPrinted;
    }
    else
    {
        mCurPos += mBufLeft;
        mBufLeft = 0;
    }

    return (mBufLeft > 0);
}

void TdfPrintHelperCharBuf::reset()
{
    mBuf[0] = '\0';        // Ensure that the buffer is empty
    mCurPos = mBuf;
    mBufLeft = mSize;
}

TdfPrintHelperString::TdfPrintHelperString()
{
    reset();
}

bool TdfPrintHelperString::pushVar(size_t size, const char8_t* format, va_list args)
{
    mString.append_sprintf_va_list(format, args);

    return true;
}

bool TdfPrintHelperString::pushString(const char8_t* buf, int bufLen /*= -1*/)
{
    if (bufLen < 0)
        mString.append(buf);
    else
        mString.append(buf, buf + bufLen);
    return true;
}

void TdfPrintHelperString::reset()
{
    mString.clear();        // Ensure that the buffer is empty
}

bool TdfPrintHelperString::pushChar(const char8_t c)
{
    mString += c;
    return true;
}

} //namespace TDF
} //namespace EA


