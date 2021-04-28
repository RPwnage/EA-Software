/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MYSQLPREPRAREDSTATEMENTROW_H
#define BLAZE_MYSQLPREPRAREDSTATEMENTROW_H

/*** Include files *******************************************************************************/

#include "framework/database/mysql/blazemysql.h"
#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "framework/database/dbrow.h"
#include "framework/logger.h"
#include "framework/util/shared/blazestring.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class MySqlPreparedStatementRow : public DbRow
{
public:
    // MySQL BOOLEAN columns: TINYINT(1), (value == 0) => false and (value != 0) => true
    bool getBool(const char8_t* columnName) const override; 
    bool getBool(uint32_t columnIndex) const override; 
    const char8_t* getString(const char8_t* columnName) const override;
    const char8_t* getString(uint32_t columnIndex) const override;
    int8_t getChar(const char8_t* columnName) const override;
    int8_t getChar(uint32_t columnIndex) const override;
    uint8_t getUChar(const char8_t* columnName) const override;
    uint8_t getUChar(uint32_t columnIndex) const override;
    int16_t getShort(const char8_t* columnName) const override;
    int16_t getShort(uint32_t columnIndex) const override;
    uint16_t getUShort(const char8_t* columnName) const override;
    uint16_t getUShort(uint32_t columnIndex) const override;
    int32_t getInt(const char8_t* columnName) const override;
    int32_t getInt(uint32_t columnIndex) const override;
    uint32_t getUInt(const char8_t* columnName) const override;
    uint32_t getUInt(uint32_t columnIndex) const override;
    int64_t getInt64(const char8_t* columnName) const override;
    int64_t getInt64(uint32_t columnIndex) const override;
    uint64_t getUInt64(const char8_t* columnName) const override;
    uint64_t getUInt64(uint32_t columnIndex) const override;
    float_t getFloat(const char8_t* columnName) const override;
    float_t getFloat(uint32_t columnIndex) const override;
    double_t getDouble(const char8_t* columnName) const override;
    double_t getDouble(uint32_t columnIndex) const override;
    const uint8_t* getBinary(const char8_t* columnName, size_t* len) const override;
    const uint8_t* getBinary(uint32_t columnIndex, size_t* len) const override;
    EA::TDF::TimeValue getUnixTimestamp(const char8_t* columnName) const override;
    EA::TDF::TimeValue getUnixTimestamp(uint32_t columnIndex) const override;
    EA::TDF::TimeValue getTimeValue(const char8_t* columnName) const override;
    EA::TDF::TimeValue getTimeValue(uint32_t columnIndex) const override;

    const char8_t* toString(
            eastl::string& buf, int32_t maxStringLen = 0, int32_t maxBinaryLen = 0) const override;

    struct ResultData
    {
        ResultData()
            : mType(MYSQL_TYPE_NULL),
              mIsNull(false),
              mError(false),
              mLength(0)
        {
            u.mData = nullptr;
        }

        ~ResultData()
        {
            if (isStringType() || isBinaryType())
                delete[] u.mData;
        }

        ResultData(const ResultData& rhs)
        {
            u.mData = nullptr;
            operator=(rhs);
        }

        ResultData& operator=(const ResultData& rhs)
        {
            mType = rhs.mType;
            mIsNull = rhs.mIsNull;
            mLength = rhs.mLength;
            mError = rhs.mError;
            if (isStringType())
            {
                // See allocData(), we copy up to mLength + 1 bytes due to our extra hidden byte
                allocData(mLength);
                blaze_strnzcpy((char8_t*)u.mData, (char8_t*)rhs.u.mData, mLength + 1);
            }
            else if (isBinaryType())
            {
                allocData(mLength);
                memcpy(u.mData, rhs.u.mData, mLength);
            }
            else
                memcpy(&u, &rhs.u, sizeof(u));
            return *this;
        }

        void allocData(uint32_t len)
        {
            delete[] u.mData;
            if (isStringType())
            {
                // MySQL client will place null-terminated strings in the buffer we provide
                // assuming the null terminator fits.  However, should the null terminator not fit,
                // it will simply write the full string without any null-terminator, and it will not
                // report data truncation.  In order to deal with this, we allocate a hidden extra byte
                // and intialize it to null.
                u.mData = BLAZE_NEW_ARRAY(uint8_t, len + 1);
                u.mData[len] = '\0';
            }
            else
            {
                u.mData = BLAZE_NEW_ARRAY(uint8_t, len);
            }
        }

        bool isStringType() const
        {
            return ((mType == MYSQL_TYPE_VAR_STRING)
                    || (mType == MYSQL_TYPE_STRING)
                    || (mType == MYSQL_TYPE_VARCHAR));
        }

        bool isBinaryType() const
        {
            return ((mType == MYSQL_TYPE_TINY_BLOB)
                    || (mType == MYSQL_TYPE_MEDIUM_BLOB) \
                    || (mType == MYSQL_TYPE_LONG_BLOB) \
                    || (mType == MYSQL_TYPE_BLOB));
        }

        enum_field_types mType;
        my_bool mIsNull;
        my_bool mError;
        union
        {
            int8_t mInt8;
            int16_t mInt16;
            int32_t mInt32;
            int64_t mInt64;
            double_t mDouble;
            float_t mFloat;
            uint8_t* mData;
        } u;
        unsigned long mLength;
    };

    typedef eastl::vector<ResultData> ResultDataList;

    MySqlPreparedStatementRow(const ResultDataList& row, MYSQL_FIELD* fields, uint32_t numFields);
    ~MySqlPreparedStatementRow() override;

private:

    ResultDataList mRow;
    typedef eastl::vector<const char8_t*> FieldNameList;
    FieldNameList mFields;
    char8_t* mFieldsBuffer;

    uint32_t getColumnIndex(const char8_t* columnName) const;
};

} // Blaze

#endif // BLAZE_MYSQLPREPRAREDSTATEMENTROW_H

