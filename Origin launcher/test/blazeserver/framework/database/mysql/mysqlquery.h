/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MYSQLQUERY_H
#define BLAZE_MYSQLQUERY_H

/*** Include files *******************************************************************************/

#include "framework/database/query.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

// JACOB: Why does a Query need to be a framework resource? It can be a simple refcounted object. Currently it stores MYSQL state reference, it can also reference its owning connection(if the MYSQL.charset) const pointer is whats required to make it work, but in general if it needs to be a member of a connection it can be, the connection is the resource, mysql query is not a resource itself, nor should it be. In fact the connection shouldn't really be managing QueryPtr objects.
// The other thing we can do is make a query a simple StringBuffer that gets passed up to the conn. That way the ownership of the buffer can remain on the stack. A given connection probably only needs one query buffer anyway, so the connection can very well return a Query& it owns internally...
// Option: Get rid of query escape string function on the mysql query object entirely, we can escape the whole thing before we execute the query, because escaping depends on the connection the query is going to be executed against.


struct st_mysql;
typedef st_mysql MYSQL;

namespace Blaze
{

class MySqlConn;
class MySqlAsyncConn;

class MySqlQuery : public Query
{
    NON_COPYABLE(MySqlQuery);

public:
    MySqlQuery(MYSQL& conn, const char8_t* fileAndLine) : Query(fileAndLine), mConn(conn) { }

    void releaseInternal() override;

protected:
    const Query::SqlKeywordList& getDestructiveKeywords() const override;

    eastl::string getEscapedString(const char8_t* str, size_t len = 0) override;
    bool escapeString(const char8_t* str, size_t len = 0) override;
    bool escapeLikeQueryString(const char8_t* str) override;
    bool encodeDateTime(const EA::TDF::TimeValue& time) override;

private:
    friend class MySqlConn;
    friend class MySqlAsyncConn;

    static Query::SqlKeywordList sDestructiveMySqlKeywords;

    MYSQL& mConn;

};

} // namespace Blaze

#endif // BLAZE_MYSQLQUERY_H

