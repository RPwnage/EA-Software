/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class MySqlPreparedStatementRow
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/mysql/mysqlpreparedstatementrow.h"
#include "framework/database/dbutil.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief <getString>

    Returns the char8_t* type data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row, nullptr is returned if a column with the
    name is not found in row.

*/
/*************************************************************************************************/
const char8_t* MySqlPreparedStatementRow::getString(const char8_t* columnName) const
{
    return getString(getColumnIndex(columnName));
}

/*************************************************************************************************/
/*!
    \brief <getString>

    Returns the char8_t* type data of a column for a given column index.
    \param[in]  <columnIndex> - index of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/
const char8_t* MySqlPreparedStatementRow::getString(uint32_t columnIndex) const
{
    if (columnIndex >= mFields.size())
        return nullptr;
    const ResultData& data = mRow[columnIndex];
    if (!data.isStringType() || data.mIsNull)
        return nullptr;
    return (const char8_t*)data.u.mData;
}

/*************************************************************************************************/
/*!
    \brief <getChar>

    Returns the int8 data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

int8_t MySqlPreparedStatementRow::getChar(const char8_t* columnName) const
{
    return getChar(getColumnIndex(columnName));
}

/*************************************************************************************************/
/*!
    \brief <getChar>

    Returns the int8 data of a column for a given column index.
    \param[in]  <columnIndex> - index of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

int8_t MySqlPreparedStatementRow::getChar(uint32_t columnIndex) const
{
    if (columnIndex >= mFields.size())
        return 0;
    const ResultData& data = mRow[columnIndex];
    if (data.mType != MYSQL_TYPE_TINY)
        return 0;
    return data.u.mInt8;
}

/*************************************************************************************************/
/*!
    \brief <getUChar>

    Returns the unsigned int8 data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

uint8_t MySqlPreparedStatementRow::getUChar(const char8_t* columnName) const
{
    return getUChar(getColumnIndex(columnName));
}

/*************************************************************************************************/
/*!
    \brief <getUChar>

    Returns the unsigned int8 data of a column for a given column index.
    \param[in]  <columnIndex> - index of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

uint8_t MySqlPreparedStatementRow::getUChar(uint32_t columnIndex) const
{
    if (columnIndex >= mFields.size())
        return 0;
    const ResultData& data = mRow[columnIndex];
    if (data.mType != MYSQL_TYPE_TINY)
        return 0;
    return (uint8_t)data.u.mInt8;
}

/*************************************************************************************************/
/*!
    \brief <getShort>

    Returns the int16 data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

int16_t MySqlPreparedStatementRow::getShort(const char8_t* columnName) const
{
    return getShort(getColumnIndex(columnName));
}

/*************************************************************************************************/
/*!
    \brief <getShort>

    Returns the int16 data of a column for a given column index.
    \param[in]  <columnIndex> - index of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

int16_t MySqlPreparedStatementRow::getShort(uint32_t columnIndex) const
{
    if (columnIndex >= mFields.size())
        return 0;
    const ResultData& data = mRow[columnIndex];
    if (data.mType != MYSQL_TYPE_SHORT)
        return 0;
    return data.u.mInt16;
}

/*************************************************************************************************/
/*!
    \brief <getUShort>

    Returns the unsigned int16 data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

uint16_t MySqlPreparedStatementRow::getUShort(const char8_t* columnName) const
{
    return getUShort(getColumnIndex(columnName));
}

/*************************************************************************************************/
/*!
    \brief <getUShort>

    Returns the unsigned int16 data of a column for a given column index.
    \param[in]  <columnIndex> - index of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

uint16_t MySqlPreparedStatementRow::getUShort(uint32_t columnIndex) const
{
    if (columnIndex >= mFields.size())
        return 0;
    const ResultData& data = mRow[columnIndex];
    if (data.mType != MYSQL_TYPE_SHORT)
        return 0;
    return (uint16_t)data.u.mInt16;
}

/*************************************************************************************************/
/*!
    \brief <getInt>

    Returns the int data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

int32_t MySqlPreparedStatementRow::getInt(const char8_t* columnName) const
{
    return getInt(getColumnIndex(columnName));
}

/*************************************************************************************************/
/*!
    \brief <getInt>

    Returns the int data of a column for a given column index.
    \param[in]  <columnIndex> - index of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

int32_t MySqlPreparedStatementRow::getInt(uint32_t columnIndex) const
{
    if (columnIndex >= mFields.size())
        return 0;
    const ResultData& data = mRow[columnIndex];
    if (data.mType != MYSQL_TYPE_LONG)
        return 0;
    return data.u.mInt32;
}

/*************************************************************************************************/
/*!
    \brief <getUInt>

    Returns the unsigned int data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

uint32_t MySqlPreparedStatementRow::getUInt(const char8_t* columnName) const
{
    return getUInt(getColumnIndex(columnName));
}

/*************************************************************************************************/
/*!
    \brief <getUInt>

    Returns the unsigned int data of a column for a given column index.
    \param[in]  <columnIndex> - index of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

uint32_t MySqlPreparedStatementRow::getUInt(uint32_t columnIndex) const
{
    if (columnIndex >= mFields.size())
        return 0;
    const ResultData& data = mRow[columnIndex];
    if (data.mType != MYSQL_TYPE_LONG)
        return 0;
    return (uint32_t)data.u.mInt32;
}

/*************************************************************************************************/
/*!
    \brief <getInt64>

    Returns the int64 data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

int64_t MySqlPreparedStatementRow::getInt64(const char8_t* columnName) const
{
    return getInt64(getColumnIndex(columnName));
}

/*************************************************************************************************/
/*!
    \brief <getInt64>

    Returns the int64 data of a column for a given column index.
    \param[in]  <columnIndex> - index of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

int64_t MySqlPreparedStatementRow::getInt64(uint32_t columnIndex) const
{
    if (columnIndex >= mFields.size())
        return 0;

    const ResultData& data = mRow[columnIndex];
    if (data.mType != MYSQL_TYPE_LONGLONG)
        return 0;
    return data.u.mInt64;
}

/*************************************************************************************************/
/*!
    \brief <getUInt64>

    Returns the unsigned int64 data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

uint64_t MySqlPreparedStatementRow::getUInt64(const char8_t* columnName) const
{
    return getUInt64(getColumnIndex(columnName));
}

/*************************************************************************************************/
/*!
    \brief <getUInt64>

    Returns the unsigned int64 data of a column for a given column index.
    \param[in]  <columnIndex> - index of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

uint64_t MySqlPreparedStatementRow::getUInt64(uint32_t columnIndex) const
{
    if (columnIndex >= mFields.size())
        return 0;

    const ResultData& data = mRow[columnIndex];
    if (data.mType != MYSQL_TYPE_LONGLONG)
        return 0;
    return (uint64_t)data.u.mInt64;
}

/*************************************************************************************************/
/*!
    \brief <getFloat>

    Returns the float data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

float_t MySqlPreparedStatementRow::getFloat(const char8_t* columnName) const
{
    return getFloat(getColumnIndex(columnName));
}

/*************************************************************************************************/
/*!
    \brief <getFloat>

    Returns the float data of a column for a given column index.
    \param[in]  <columnIndex> - index of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

float_t MySqlPreparedStatementRow::getFloat(uint32_t columnIndex) const
{
    if (columnIndex >= mFields.size())
        return 0;
    const ResultData& data = mRow[columnIndex];
    if (data.mType != MYSQL_TYPE_FLOAT)
        return 0;
    return data.u.mFloat;
}

/*************************************************************************************************/
/*!
    \brief <getDouble>

    Returns the double data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

double_t MySqlPreparedStatementRow::getDouble(const char8_t* columnName) const
{
    return getDouble(getColumnIndex(columnName));
}

/*************************************************************************************************/
/*!
    \brief <getDouble>

    Returns the double data of a column for a given column index.
    \param[in]  <columnIndex> - index of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

double_t MySqlPreparedStatementRow::getDouble(uint32_t columnIndex) const
{
    if (columnIndex >= mFields.size())
        return 0;
    const ResultData& data = mRow[columnIndex];
    if (data.mType != MYSQL_TYPE_DOUBLE)
        return 0;
    return data.u.mDouble;
}

// MySQL BOOLEAN columns: TINYINT(1), (value == 0) => false and (value != 0) => true
bool MySqlPreparedStatementRow::getBool(const char8_t* columnName) const { return (getChar(columnName) != 0); }
bool MySqlPreparedStatementRow::getBool(uint32_t columnIndex) const { return (getChar(columnIndex) != 0); }

// Timestamp values retrieved via UNIX_TIMESTAMP(someColumn) AS columnName
EA::TDF::TimeValue MySqlPreparedStatementRow::getUnixTimestamp(const char8_t* columnName) const
{
    EA::TDF::TimeValue unixTimestamp;
    unixTimestamp.setSeconds(getInt64(columnName));
    return unixTimestamp;
}
EA::TDF::TimeValue MySqlPreparedStatementRow::getUnixTimestamp(uint32_t columnIndex) const
{
    EA::TDF::TimeValue unixTimestamp;
    unixTimestamp.setSeconds(getInt64(columnIndex));
    return unixTimestamp;
}
// MySQL TIMESTAMP / DATETIME values retrieved as string values
EA::TDF::TimeValue MySqlPreparedStatementRow::getTimeValue(const char8_t* columnName) const
{
    const char8_t* dbDateTime = getString(columnName);
    return Blaze::DbUtil::dateTimeToTimeValue(dbDateTime);
}
EA::TDF::TimeValue MySqlPreparedStatementRow::getTimeValue(uint32_t columnIndex) const
{
    const char8_t* dbDateTime = getString(columnIndex);
    return Blaze::DbUtil::dateTimeToTimeValue(dbDateTime);
}


const uint8_t* MySqlPreparedStatementRow::getBinary(const char8_t* columnName, size_t* len) const
{
    return getBinary(getColumnIndex(columnName), len);
}

const uint8_t* MySqlPreparedStatementRow::getBinary(uint32_t columnIndex, size_t* len) const
{
    if (len != nullptr)
        *len = 0;

    if (columnIndex >= mFields.size())
        return nullptr;

    const ResultData& data = mRow[columnIndex];
    if ((!data.isBinaryType() && !data.isStringType()) || data.mIsNull)
        return nullptr;
    if (len != nullptr)
        *len = (size_t)data.mLength;
    return (data.mLength != 0) ? data.u.mData : nullptr;
}

uint32_t MySqlPreparedStatementRow::getColumnIndex(const char8_t* columnName) const
{
    if ((columnName == nullptr) || (columnName[0] == '\0'))
        return UINT32_MAX;

    //get the index for the given column name
    FieldNameList::const_iterator itr = mFields.begin();
    FieldNameList::const_iterator end = mFields.end();
    for(uint32_t idx = 0; itr != end; ++itr, ++idx)
    {
        if (blaze_stricmp(columnName, *itr) == 0)
            return idx;
    }
    return UINT32_MAX;
}
/*************************************************************************************************/
/*!
    \brief Constructor
*/
/*************************************************************************************************/
MySqlPreparedStatementRow::MySqlPreparedStatementRow(
        const ResultDataList& row, MYSQL_FIELD* fields, uint32_t numFields)
    : mRow(row)
{
    mFields.reserve(numFields);
    size_t bufLen = 0;
    for(uint32_t idx = 0; idx < numFields; ++idx)
        bufLen += strlen(fields[idx].name) + 1;
    mFieldsBuffer = BLAZE_NEW_ARRAY(char8_t, bufLen);
    char8_t* field = mFieldsBuffer;
    for(uint32_t idx = 0; idx < numFields; ++idx)
    {
        strcpy(field, fields[idx].name);
        mFields.push_back(field);
        field += strlen(fields[idx].name) + 1;
    }
}

MySqlPreparedStatementRow::~MySqlPreparedStatementRow()
{
    delete[] mFieldsBuffer;
}

const char8_t* MySqlPreparedStatementRow::toString(
        eastl::string& buf, int32_t maxStringLen, int32_t maxBinaryLen) const
{
    ResultDataList::const_iterator itr = mRow.begin();
    ResultDataList::const_iterator end = mRow.end();
    for(uint32_t idx = 0; itr != end; ++itr, ++idx)
    {
        buf.append_sprintf("%s(%d)=", mFields[idx], idx);
        const ResultData& data = *itr;
        if (data.isStringType())
        {
            if (maxStringLen == 0)
                buf.append_sprintf("'%s' ", (char8_t*)data.u.mData);
            else
                buf.append_sprintf("'%.*s' ", maxStringLen, (char8_t*)data.u.mData);
        }
        else if (data.isBinaryType())
        {
            size_t len = data.mLength;
            if ((maxBinaryLen != 0) && ((size_t)maxBinaryLen < len))
                len = maxBinaryLen;
            for(size_t i = 0; i < len; ++i)
                buf.append_sprintf("%02x ", data.u.mData[i]);
        }
        else if (data.mType == MYSQL_TYPE_TINY)
            buf.append_sprintf("%d ", data.u.mInt8);
        else if (data.mType == MYSQL_TYPE_SHORT)
            buf.append_sprintf("%d ", data.u.mInt16);
        else if (data.mType == MYSQL_TYPE_LONG)
            buf.append_sprintf("%d ", data.u.mInt32);
        else if (data.mType == MYSQL_TYPE_LONGLONG)
            buf.append_sprintf("%" PRIi64 " ", data.u.mInt64);
        else if (data.mType == MYSQL_TYPE_DOUBLE)
            buf.append_sprintf("%f ", data.u.mDouble);
        else if (data.mType == MYSQL_TYPE_FLOAT)
            buf.append_sprintf("%f ", data.u.mFloat);
        else
            buf.append_sprintf("<unknown type> ");
    }
    return buf.c_str();
}

}// Blaze
