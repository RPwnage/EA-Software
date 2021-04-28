/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EA_TDF_PRINTENCODER_H
#define EA_TDF_PRINTENCODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/tdfbase.h>
#include <EATDF/tdfvisit.h>
#include <EAStdC/EAString.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace EA
{
namespace TDF
{
 

class EATDF_API PrintHelper
{

public:
    virtual ~PrintHelper() {}

    virtual bool pushChar(const char8_t c) { return push(1, "%c", c); }
    virtual bool pushString(const char8_t* buf, int bufLen = -1) { return push(bufLen > -1 ? bufLen : EA_TDF_STRLEN(buf), "%s", buf); }
    virtual bool push(size_t size, const char8_t* format, ...)
    {
        va_list args;
        va_start(args, format);
        bool result = pushVar(size, format, args);
        va_end(args);

        return result;
    }

    virtual bool pushVar(size_t size, const char8_t* format, va_list args) = 0;
};

class EATDF_API TdfPrintHelperCharBuf : public PrintHelper
{
public:
    TdfPrintHelperCharBuf(char8_t* buf, size_t bufSize);

    void reset();
    bool pushVar(size_t size, const char8_t* format, va_list args) override;

protected:
    char8_t* mBuf;
    char8_t* mCurPos;
    size_t mSize;
    size_t mBufLeft;
};

class EATDF_API TdfPrintHelperString : public PrintHelper
{
public:
    TdfPrintHelperString();

    void reset();
    bool pushChar(const char8_t c) override;
    bool pushVar(size_t size, const char8_t* format, va_list args) override;
    bool pushString(const char8_t* buf, int bufLen = -1) override;
    EATDFEastlString getString() { return mString; }

protected:
    EATDFEastlString mString;
};

class EATDF_API PrintEncoder
{
public:
    static const uint32_t MAX_OUTPUT_BUFFER_SIZE = 512*1024;

    PrintEncoder(int32_t indent = 0, bool terse = false);
    ~PrintEncoder();
    void setMaxEncodeLength(uint32_t length) { mMaxEncodeLength = length; }
    
    // Default encoder for print for logging:  Outputs Tdf class name before value
    bool print(PrintHelper& printer, const Tdf& tdf, int32_t indent = 0, bool terse = false);

    // Default encoder for string creation: Does not output type name before value 
    bool print(PrintHelper& printer, TdfGenericReferenceConst& ref, int32_t indent = 0, bool terse = true, char8_t entrySeperator = ' ');

    bool visitValue(TdfGenericReferenceConst& ref, const TdfMemberInfo* memberInfo = nullptr);
    bool visitTdfBase(const Tdf &value);
    bool visitTdf(const Tdf &value);
    bool visitTdfUnion(const TdfUnion &value);

    static int32_t printBinary(char8_t* dst, size_t len, const uint8_t* src, uint32_t srclen, int32_t indent = 0, char8_t entrySeparator = '\n');

protected:

    int32_t mIndent;
    bool mFull;
    const char8_t* mObjectName;
   
    typedef enum { STATE_NORMAL, STATE_ARRAY, STATE_MAP, STATE_UNION } StateEnum;
    struct State
    {
        StateEnum state;
        int32_t elementNum;
        uint32_t arrayIndex;
        bool outputType;
    };

    State* mCurrentState;
    PrintHelper* mPrinter;
    bool mTerse;
    char8_t mEntrySeparator;
    uint32_t mMaxEncodeLength;

    bool indent();
    bool addName(const TdfMemberIteratorConst& memberIter);
    bool outputData(const TdfMemberInfo* memberInfo, int32_t size, const char8_t* type, const char8_t* format, ...) EA_TDF_ATTRIBUTE_PRINTF_CHECK(5, 6);

    bool outputLine(bool skip = false);
    bool pushChar(const char8_t c);
    bool pushString(const char8_t* buf, int bufLen = -1);
};

} //namespace TDF
} //namespace EA


#endif // EA_TDF_PRINTENCODER_H

