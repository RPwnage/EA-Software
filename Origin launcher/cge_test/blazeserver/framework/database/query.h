/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_QUERY_H
#define BLAZE_QUERY_H

/*** Include files *******************************************************************************/

#include "framework/util/shared/stringbuilder.h"
#include "framework/system/frameworkresource.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#define DB_QUERY_RESET(query) query->reset(__FILEANDLINE__);

namespace Blaze
{


class DbConnBase;

class QueryBase : protected StringBuilder, public FrameworkResource
{
    NON_COPYABLE(QueryBase);

    friend class DbConnBase;

public:    
    bool append(const char8_t* format, ...) override;

    void setQueryBuffer(char8_t* buffer);

    // Enable public access to protected virtual StringBuilder functions
    StringBuilder& operator<<(int8_t value) override { return StringBuilder::operator<<(value); }
    StringBuilder& operator<<(int16_t value) override { return StringBuilder::operator<<(value); }
    StringBuilder& operator<<(int32_t value) override { return StringBuilder::operator<<(value); }
    StringBuilder& operator<<(int64_t value) override { return StringBuilder::operator<<(value); }
    StringBuilder& operator<<(uint8_t value) override { return StringBuilder::operator<<(value); }
    StringBuilder& operator<<(uint16_t value) override { return StringBuilder::operator<<(value); }
    StringBuilder& operator<<(uint32_t value) override { return StringBuilder::operator<<(value); }
    StringBuilder& operator<<(uint64_t value) override { return StringBuilder::operator<<(value); }
    StringBuilder& operator<<(double value) override { return StringBuilder::operator<<(value); }

    // Append the given string to the buffer in it's escaped form.
    // For MySQL, characters encoded are \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z).
    StringBuilder& operator<<(const char8_t* value) override;
    StringBuilder& operator+=(const char8_t* value) override { return operator<<(value); }

    // Append the given date/time value to the buffer in DB appropriate format.
    StringBuilder& operator<<(const EA::TDF::TimeValue& value) override;

    StringBuilder& operator<<(const EA::TDF::TdfBlob& value);

    // Append the given string to the buffer for LIKE query in it's escaped form.
    // For MySQL, characters encoded are % (percentage), _ (underscore), and \ (backslash) to avoid unexpected matches.
    // Also calls escapeString to escape other special characters.
    bool appendLikeQueryString(const char8_t* value);

    // Append the given string to the buffer in it's original form.
    bool appendVerbatimString(const char8_t* value, bool skipStringChecks = false, bool allowDestructiveKeywords = false);

    const char8_t* getTypeName() const override { return "QueryBase"; }
    const char8_t* getName() const { return mName; }

    // overload for 'StringBuilder& reset()' function
    void reset(const char8_t* fileAndLine);

    // override non-virtual StringBuilder functions
    bool isEmpty() const;

    // Enable public access to protected non-virtual StringBuilder functions
    size_t length() const { return StringBuilder::length(); }
    const char8_t* c_str() const { return StringBuilder::c_str(); }
    const char8_t* get() const { return StringBuilder::get(); }
    void trim(size_t bytes) { StringBuilder::trim(bytes); }

    // Returns the list of destructive keywords i.e. for logging purposes
    typedef eastl::list<eastl::string> SqlKeywordList;
    virtual const SqlKeywordList& getDestructiveKeywords() const = 0;

    // Searches a string for destructive keywords that could cause data or schema changes
    bool hasDestructiveKeyword(const char8_t* str, eastl::string& keywordOut) const;

protected:
    QueryBase(const char8_t* fileAndLine);
    ~QueryBase() override;

    void assignInternal() override {}
    void releaseInternal() override = 0;

    // Generates an escaped copy of the string without appending anything to the buffer.
    // For MySQL, characters encoded are \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z).
    virtual eastl::string getEscapedString(const char8_t* str, size_t len = 0) = 0;

    // Append the given string to the buffer in it's escaped form.
    // For MySQL, characters encoded are \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z).
    virtual bool escapeString(const char8_t* str, size_t len = 0) = 0;

    // Append the given string to the buffer for LIKE query in it's escaped form.
    // For MySQL, characters encoded are % (percentage), _ (underscore), and \ (backslash) to avoid unexpected matches.
    // Also calls escapeString to escape other special characters.
    virtual bool escapeLikeQueryString(const char8_t* str) = 0;

    // Append the given date/time value to the buffer in DB appropriate format.
    virtual bool encodeDateTime(const EA::TDF::TimeValue& time) = 0;

private:
    bool isValidVerbatimString(const char8_t* value, bool skipStringChecks, bool allowDestructiveKeywords);

    bool mIsEmpty;

    const char8_t* mName;
};

//  QueryPtr interface definition
typedef FrameworkResourcePtr<QueryBase> QueryPtr;

//  Deprecated Query interface.
class Query : public QueryBase
{
    NON_COPYABLE(Query);

public:
    void release() { FrameworkResource::release(); }

protected:
    Query(const char8_t* fileAndLine) : QueryBase(fileAndLine) {}
    void releaseInternal() override = 0;
};


} // namespace Blaze

#endif // BLAZE_QUERY_H
