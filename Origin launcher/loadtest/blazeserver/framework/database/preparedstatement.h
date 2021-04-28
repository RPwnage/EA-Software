

/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PREPAREDSTATEMENT_H
#define BLAZE_PREPAREDSTATEMENT_H

/*** Include files *******************************************************************************/

#include "framework/database/dberrors.h"
#include "EASTL/string.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class DbResult;
class RawBuffer;
class DbConnInterface;
class MySqlConn;
class MySqlAsyncConn;

typedef int32_t PreparedStatementId;
const PreparedStatementId INVALID_PREPARED_STATEMENT_ID = -1;

class PreparedStatementInfo
{
public:
    PreparedStatementInfo(PreparedStatementId id, const char8_t* name, const char8_t* query);
    ~PreparedStatementInfo();

    PreparedStatementId getId() const { return mId; }
    const char8_t* getName() const { return mName; }
    const char8_t* getQuery() const { return mQuery; }

private:
    PreparedStatementId mId;
    char8_t* mName;
    char8_t* mQuery;
};

class PreparedStatement
{
    NON_COPYABLE(PreparedStatement);

    friend class DbConnInterface;
    friend class ThreadedDbConn;
    friend class MySqlConn;
    friend class MySqlAsyncConn;

public:
    PreparedStatement(const PreparedStatementInfo& info);
    virtual ~PreparedStatement();

    const char8_t* getName() const { return mName; }
    const char8_t* getQuery() const { return mQuery; }
    bool getIsRegistered() const { return mRegistered; }

    void setInt64(uint32_t column, int64_t value);
    void setInt32(uint32_t column, int32_t value);
    void setInt16(uint32_t column, int16_t value);
    void setInt8(uint32_t column, int8_t value);
    void setUInt64(uint32_t column, uint64_t value);
    void setUInt32(uint32_t column, uint32_t value);
    void setUInt16(uint32_t column, uint16_t value);
    void setUInt8(uint32_t column, uint8_t value);
    void setString(uint32_t column, const char8_t* value);
    void setFloat(uint32_t column , float value);
    void setDouble(uint32_t column , double value);
    void setBinary(uint32_t column, const uint8_t* value, size_t len);

    const char8_t* toString(eastl::string& buf,
            int32_t maxStringLen = 0, int32_t maxBinaryLen = 0) const;

protected:
    struct ParamData
    {
        ParamData() : mType(TYPE_NONE) { }

        enum
        {
            TYPE_NONE, TYPE_INT8, TYPE_INT16, TYPE_INT32, TYPE_INT64,
            TYPE_UINT8, TYPE_UINT16, TYPE_UINT32, TYPE_UINT64,
            TYPE_STRING, TYPE_DOUBLE, TYPE_FLOAT, TYPE_BINARY
        } mType;
        char mIsNull;
        union
        {
            int8_t mInt8;
            int16_t mInt16;
            int32_t mInt32;
            int64_t mInt64;
            uint8_t mUInt8;
            uint16_t mUInt16;
            uint32_t mUInt32;
            uint64_t mUInt64;
            char8_t* mString;
            double_t mDouble;
            float_t mFloat;
            struct
            {
                uint8_t* data;
                size_t len;
            } mBinary;
        };
        uint8_t mBuffer[32];
    };

    typedef eastl::vector<ParamData> ParamList;

    void setRegistered(bool registered) { mRegistered = registered; }

    PreparedStatementId mId;
    char8_t* mName;
    char8_t* mQuery;
    bool mRegistered;
    ParamList mParams;

private:
    virtual BlazeRpcError execute(DbResult** result, MySqlConn& conn) = 0;
    virtual BlazeRpcError execute(DbResult** result, MySqlAsyncConn& conn, TimeValue timeout = 0) = 0;
    virtual void close(DbConnInterface& conn) = 0;

    void reset();

    ParamData& getColumn(uint32_t column);
};

} // Blaze

#endif // BLAZE_PREPAREDSTATEMENT_H



