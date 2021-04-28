/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class QueryBase

    This class represents an SQL query to be executed by the DB framework.  It handles formatting
    data types based on the underlying DB implementation.  One primary goal of the class is to
    ensure that string parameters are properly escaped to prevent bogus string data from either
    causing the query to fail or cause unintended side affects (e.g. SQL injection attacks).
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/database/query.h"
#include "framework/util/shared/blazestring.h"
#include "framework/database/dbscheduler.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

QueryBase::QueryBase(const char8_t* fileAndLine)
    : StringBuilder(nullptr),
    mIsEmpty(true),
    mName(nullptr)
{
    reset(fileAndLine);
}

QueryBase::~QueryBase()
{
}

/*************************************************************************************************/
/*!
     \brief append
 
      Append text to this query.  This function acts much like printf except that the formating
      characters are different.  The following substitutions are possible:

      $d - int8_t, int16_t, or int32_t
      $u - uint8_t, uint16_t, or uint32_t
      $D - int64_t
      $U - uint64_t
      $f - float/double
      $s - String (escaped appropriately for the underlying database implementation)
      $S - [Deprecated, use appendVerbatimString instead] Verbatim string (string is substituted as-is)
      $t - Time/data representation (pass TimeValue* as an argument for the format)
      $b - blob (pass EA::TDF::TdfBlob* as an argument for the format)
      $L - LIKE query (in a query such as "LIKE '$L%'" pass string as an argument for the format)
  
     \param[in] format - Format string to append.
     \param[in] ...    - 0 or more arguments corresponding to substitution characters in the format
 
     \return - true on success; false on failure

     \notes If this function fails, the entire query will be truncated to zero length.  This is
            done to prevent invalid queries from being formed and executed if return values to
            this function are not honored.
*/
/*************************************************************************************************/
bool QueryBase::append(const char8_t* format, ...)
{
    if (format == nullptr)
        return true;

    if (mBuffer == nullptr)
        grow(1);

    bool allowVerbatimStringFormatSpecifier = false;
#ifdef BLAZE_ENABLE_DEPRECATED_VERBATIM_STRING_FORMAT_SPECIFIER_IN_DATABASE_QUERY
    allowVerbatimStringFormatSpecifier = true;
#endif

    va_list args;
    va_start(args, format);

    char8_t ch;
    while ((ch = *format++) != '\0')
    {
        if (mCount == mSize)
        {
            // Ensure there is enough space for at least this character
            if (!grow(1))
            {
                mCount = 0;
                return false;
            }
        }

        if (ch == '$')
        {
            ch = *format++;
            if (ch == '\0')
            {
                // Unexpected end of format string
                mCount = 0;
                return false;
            }
            switch (ch)
            {
                case 'd': // int8_t, int16_t, int32_t
                    StringBuilder::operator<<(va_arg(args, int32_t));
                    break;

                case 'u': // uint8_t, uint16_t, uint32_t
                    StringBuilder::operator<<(va_arg(args, uint32_t));
                    break;

                case 'D': // int64_t
                    StringBuilder::operator<<(va_arg(args, int64_t));
                    break;

                case 'U': // uint64_t
                    StringBuilder::operator<<(va_arg(args, uint64_t));
                    break;

                case 'f': // float/double
                    StringBuilder::operator<<(va_arg(args, double));
                    break;

                case 'S': // verbatim string
                    if (!allowVerbatimStringFormatSpecifier)
                    {
                        EA_FAIL_FORMATTED(("Unallowed usage of deprecated verbatim string format specifier in query: %s", mName));
                        mCount = 0;
                        return false;
                    }

                    // Allowing destructive keywords (to preserve legacy behavior)
                    appendVerbatimString(va_arg(args, const char8_t*), false /*skipStringChecks*/, true /*allowDestructiveKeywords*/);
                    break;

                case 's': // string
                    *this << va_arg(args, const char8_t*);
                    break;

                case 'L': // string for LIKE query
                    appendLikeQueryString(va_arg(args, const char8_t*));
                    break;

                case 't': // date/time
                    *this << *va_arg(args, const TimeValue*);
                    break;

                case 'b': // blob
                    *this << *va_arg(args, const EA::TDF::TdfBlob*);
                    break;

                default:
                    mBuffer[mCount++] = ch;
                    break;
            }
        }
        else
        {
            mBuffer[mCount++] = ch;
        }
    }
    va_end(args);
    mBuffer[mCount] = '\0';
    mIsEmpty = false;
    return true;
}

bool QueryBase::isEmpty() const
{
    if (StringBuilder::isEmpty())
    {
        return true;
    }
    return mIsEmpty;
}

void QueryBase::reset(const char8_t* fileAndLine)
{
    mName = fileAndLine;
    if (strstr(fileAndLine, gDbScheduler->getBuildPath().c_str()) == fileAndLine)
        mName += gDbScheduler->getBuildPath().length() + 1;

    // Special handling because it is called in the constructor before
    // the MySqlQuery derived class has been constructed which causes
    // appendVerbatimString ($S) to hit 'pure virtual' runtime errors.
    append("/* ");
    StringBuilder::operator+(mName);
    append(" */ ");

    // Even though a comment has been appended, we consider the query empty
    // as it would not execute anything on the DB.  So mIsEmpty remains true;
    mIsEmpty = true;

    StringBuilder::reset();
}

StringBuilder& QueryBase::operator<<(const char8_t* value)
{
    if (!escapeString(value))
    {
        mCount = 0;
        return *this;
    }
    mIsEmpty = false;
    return *this;
}

StringBuilder& QueryBase::operator<<(const TimeValue& value)
{
    if (!encodeDateTime(value))
    {
        mCount = 0;
        return *this;
    }
    mIsEmpty = false;
    return *this;
}

StringBuilder& QueryBase::operator<<(const EA::TDF::TdfBlob& value)
{
    if (!escapeString(reinterpret_cast<const char8_t*>(value.getData()), value.getCount()))
    {
        mCount = 0;
        return *this;
    }
    mIsEmpty = false;
    return *this;
}

bool QueryBase::appendLikeQueryString(const char8_t* value)
{
    if (!escapeLikeQueryString(value))
    {
        mCount = 0;
        return false;
    }
    mIsEmpty = false;
    return true;
}

bool QueryBase::appendVerbatimString(const char8_t* value, bool skipStringChecks, bool allowDestructiveKeywords)
{
    if (gDbScheduler->getVerbatimQueryStringChecksEnabled() &&
        !isValidVerbatimString(value, skipStringChecks, allowDestructiveKeywords))
    {
        mCount = 0;
        return false;
    }
    StringBuilder::operator+(value);
    mIsEmpty = false;
    return true;
}

bool QueryBase::isValidVerbatimString(const char8_t* value, bool skipStringChecks, bool allowDestructiveKeywords)
{
    bool isValid = true;

    if (!skipStringChecks)
    {
        eastl::string escapedString = getEscapedString(value);
        if (blaze_strcmp(value, escapedString.c_str()) == 0)
        {
            BLAZE_WARN_LOG(Log::DB, "[QueryBase].isValidVerbatimString: escaped string for value(" << value << ") is identical; consider changing $S to $s to mitigate SQL injection risk in query: " << mName);
        }
        else
        {
            BLAZE_TRACE_LOG(Log::DB, "[QueryBase].isValidVerbatimString: verbatim string is likely required for value(" << value << ") because it is escaped to value(" << escapedString << ") in query: " << mName);
        }

        eastl::string destructiveKeyword;
        if (hasDestructiveKeyword(value, destructiveKeyword))
        {
            if (!allowDestructiveKeywords)
            {
                isValid = false;
                BLAZE_ERR_LOG(Log::DB, "[QueryBase].isValidVerbatimString: rejecting query because destructive keyword(" << destructiveKeyword << ") in value(" << value << ") in query: " << mName);
            }
            else
            {
                BLAZE_WARN_LOG(Log::DB, "[QueryBase].isValidVerbatimString: appended string parameter contains destructive keyword(" << destructiveKeyword << ") in value(" << value << ") in query: " << mName);
            }
        }
    }
    else
    {
        BLAZE_TRACE_LOG(Log::DB, "[QueryBase].isValidVerbatimString: skipping string check for value(" << value << ") in query: " << mName);
    }

    return isValid;
}

bool QueryBase::hasDestructiveKeyword(const char8_t* str, eastl::string& keywordOut) const
{
    bool hasKeyword = false;
    keywordOut = "";

    const Query::SqlKeywordList& destructiveKeywords = getDestructiveKeywords();

    char8_t* queryStr = blaze_strdup(str);

    char8_t* savePtr = nullptr;
    for (char8_t* queryPart = blaze_strtok(queryStr, " ", &savePtr);
        queryPart != nullptr && hasKeyword == false;
        queryPart = blaze_strtok(nullptr, " ", &savePtr))
    {
        for (const eastl::string& reservedWord : destructiveKeywords)
        {
            if (blaze_stricmp(queryPart, reservedWord.c_str()) == 0)
            {
                keywordOut = reservedWord;
                hasKeyword = true;
                break;
            }
        }
    }

    BLAZE_FREE(queryStr);

    return hasKeyword;
}

/*************************************************************************************************/
/*!
     \brief setQueryBuffer
 
     Replace the existing query buffer with the given buffer.  This query buffer will take
     ownership of the memory.
  
     \param[in] buffer - Buffer to replace the eixsting query buffer with.
 
*/
/*************************************************************************************************/
void QueryBase::setQueryBuffer(char8_t* buffer)
{
    if (mBuffer != nullptr)
        BLAZE_FREE(mBuffer);
    mBuffer = buffer;
    mCount = strlen(buffer);
    mSize = mCount + 1;
    mIsEmpty = false;
}

} // namespace Blaze

