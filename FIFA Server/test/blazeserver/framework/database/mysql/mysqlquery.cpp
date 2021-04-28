/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class MySqlQuery

    <Describe the responsibility of the class>

    \notes
        <Any additional detail including implementation notes and references.  Delete this
        section if there are no notes.>

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/database/mysql/mysqlquery.h"
#include "framework/database/mysql/mysqlconn.h"
#include "framework/database/mysql/blazemysql.h"
#include "EATDF/time.h"

namespace Blaze
{

/*** Private Members *****************************************************************************/

Query::SqlKeywordList MySqlQuery::sDestructiveMySqlKeywords = {
    "ALTER",
    "AUTO_INCREMENT",
    "CHANGE",
    "CREATE",
    "DATABASE",
    "DELETE",
    "DESCRIBE",
    "DROP",
    "GRANT",
    "INSERT",
    "KEYS",
    "KILL",
    "LOAD",
    "LOCK",
    "MASTER_SERVER_ID",
    "PRIVILEGES",
    "PROCEDURE",
    "PURGE",
    "RENAME",
    "REPLACE",
    "RESTRICT",
    "SET",
    "TABLE",
    "TABLES",
    "UNLOCK",
    "UPDATE",
    "USAGE",
    "WRITE"
};

/*** Public Methods ******************************************************************************/

void MySqlQuery::releaseInternal()
{
    delete this;
}

/*** Protected Methods ***************************************************************************/

const Query::SqlKeywordList& MySqlQuery::getDestructiveKeywords() const
{
    return sDestructiveMySqlKeywords;
}

eastl::string MySqlQuery::getEscapedString(const char8_t* str, size_t len)
{
    if (str == nullptr)
        return "";

    if (len == 0)
        len = strlen(str);

    eastl::string escapedStr;
    escapedStr.reserve((len * 2) + 1);

    mysql_real_escape_string(
        &mConn, escapedStr.data(), str, (unsigned long)len);

    return escapedStr;
}

bool MySqlQuery::escapeString(const char8_t* str, size_t len)
{
    if (str == nullptr)
        return true;

    if (len == 0)
        len = strlen(str);

    if (!grow((len * 2) + 1))
    {
        mCount = 0;
        return false;
    }

    char8_t* escapedStr = mBuffer + mCount;

    mCount += mysql_real_escape_string(
        &mConn, escapedStr, str, (unsigned long)len);

    return true;
}

bool MySqlQuery::escapeLikeQueryString(const char8_t* str)
{
    if (str == nullptr)
        return true;

    StringBuilder sb;
    for (uint32_t i = 0; str[i] != '\0'; ++i)
    {
        if (str[i] == '%' || str[i] == '_' || str[i] == '\\')
            sb << '\\';
        sb << str[i];
    }

    return escapeString(sb.c_str());
}

bool MySqlQuery::encodeDateTime(const TimeValue& time)
{
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
    uint32_t millis;
    int32_t wrote;

    if (!grow(64))
    {
        mCount = 0;
        return false;
    }

    TimeValue::getLocalTimeComponents(time, &year, &month, &day, &hour, &minute, &second, &millis);

    wrote = blaze_snzprintf(mBuffer + mCount, mSize - mCount,
            "DATE_FORMAT('%04d-%02d-%02d %02d:%02d:%02d','%%Y-%%m-%%d %%H:%%i:%%s')",
            year, month, day, hour, minute, second);

    if (wrote <= 0)
    {
        return false;
    }

    mCount += wrote;
    return true;
}

} // namespace Blaze

