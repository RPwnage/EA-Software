/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class MySqlRow
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "EASTL/algorithm.h"
#include "framework/database/mysql/mysqlrow.h"
#include "framework/database/mysql/mysqlresult.h"
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
const char8_t* MySqlRow::getString(const char8_t* columnName) const
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
const char8_t* MySqlRow::getString(uint32_t columnIndex) const
{
    if (columnIndex >= mResult->mNumFields)
        return nullptr;
    return mRow[columnIndex];
}

/*************************************************************************************************/
/*!
    \brief <getFloat>

    Returns the float data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

float_t MySqlRow::getFloat(const char8_t* columnName) const
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

float_t MySqlRow::getFloat(uint32_t columnIndex) const
{
    if ((columnIndex >= mResult->mNumFields) || (mRow[columnIndex] == nullptr))
        return 0;
    return (float_t)strtod(mRow[columnIndex], nullptr);
}

/*************************************************************************************************/
/*!
    \brief <getDouble>

    Returns the double data of a column for a given column name.
    \param[in]  <columnName> - name of the column for which data is requested.

    \return - data of a column in a row

*/
/*************************************************************************************************/

double_t MySqlRow::getDouble(const char8_t* columnName) const
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

double_t MySqlRow::getDouble(uint32_t columnIndex) const
{
    if ((columnIndex >= mResult->mNumFields) || (mRow[columnIndex] == nullptr))
        return 0;
    return strtod(mRow[columnIndex], nullptr);
}

const uint8_t* MySqlRow::getBinary(const char8_t* columnName, size_t* len) const
{
    return getBinary(getColumnIndex(columnName), len);
}

const uint8_t* MySqlRow::getBinary(uint32_t columnIndex, size_t* len) const
{
    if (columnIndex >= mResult->mNumFields)
        return nullptr;
    if (len != nullptr)
        *len = (size_t)mDatasizes[columnIndex];
    return (uint8_t*)mRow[columnIndex];
}

uint32_t MySqlRow::getColumnIndex(const char8_t* columnName) const
{
    //get the index for the given column name
    if ((columnName != nullptr) && (columnName[0] != '\0'))
    {
        MySqlResultBase::FieldCompare fieldCompare(mResult->mFields);
        const MySqlResultBase::FieldLookup& lookup = mResult->mFieldLookup;
        MySqlResultBase::FieldLookup::const_iterator it = 
            eastl::binary_search_i(lookup.begin(), lookup.end(), columnName, fieldCompare);
        if (it != lookup.end())
        {
            return *it;
        }
    }

    BLAZE_WARN_LOG(Log::DB, "[MySqlRow].getColumnIndex: "
        << "@ { "<< Fiber::getCurrentContext() << " } could not find column name: " << columnName);
    return UINT32_MAX;
}

const char8_t* MySqlRow::toString(eastl::string& buf, int32_t maxStringLen, int32_t maxBinaryLen) const
{
    for(uint32_t idx = 0; idx < mResult->mNumFields; ++idx)
    {
        buf.append_sprintf("%s(%d)=", mResult->mFields[idx].name, idx);
        if ((mResult->mFields[idx].type == MYSQL_TYPE_VAR_STRING)
                || (mResult->mFields[idx].type == MYSQL_TYPE_STRING)
                || (mResult->mFields[idx].type == MYSQL_TYPE_VARCHAR))
        {
            if (maxStringLen == 0)
                buf.append_sprintf("'%s' ", mRow[idx]);
            else
                buf.append_sprintf("'%.*s' ", maxStringLen, mRow[idx]);
        }
        else if ((mResult->mFields[idx].type == MYSQL_TYPE_TINY_BLOB)
                || (mResult->mFields[idx].type == MYSQL_TYPE_MEDIUM_BLOB)
                || (mResult->mFields[idx].type == MYSQL_TYPE_LONG_BLOB)
                || (mResult->mFields[idx].type == MYSQL_TYPE_BLOB))
        {
            if (mRow[idx] == nullptr)
                buf.append_sprintf("(null) ");
            else
            {
                size_t len = (size_t)mDatasizes[idx];
                if ((maxBinaryLen != 0) && ((size_t)maxBinaryLen < len))
                    len = maxBinaryLen;
                for(size_t i = 0; i < len; ++i)
                    buf.append_sprintf("%02x ", mRow[idx][i]);
            }
        }
        else
        {
            buf.append_sprintf("%s ", mRow[idx]);
        }
    }
    return buf.c_str();
}

int8_t MySqlRow::getChar(const char8_t* columnName) const { return getValByName<int8_t>(columnName); }
int8_t MySqlRow::getChar(uint32_t columnIndex) const { return getVal<int8_t>(columnIndex); }
uint8_t MySqlRow::getUChar(const char8_t* columnName) const { return getValByName<uint8_t>(columnName); }
uint8_t MySqlRow::getUChar(uint32_t columnIndex) const { return getVal<uint8_t>(columnIndex); }
int16_t MySqlRow::getShort(const char8_t* columnName) const { return getValByName<int16_t>(columnName); }
int16_t MySqlRow::getShort(uint32_t columnIndex) const { return getVal<int16_t>(columnIndex); }
uint16_t MySqlRow::getUShort(const char8_t* columnName) const { return getValByName<uint16_t>(columnName); }
uint16_t MySqlRow::getUShort(uint32_t columnIndex) const { return getVal<uint16_t>(columnIndex); }
int32_t MySqlRow::getInt(const char8_t* columnName) const { return getValByName<int32_t>(columnName); }
int32_t MySqlRow::getInt(uint32_t columnIndex) const { return getVal<int32_t>(columnIndex); }
uint32_t MySqlRow::getUInt(const char8_t* columnName) const { return getValByName<uint32_t>(columnName); }
uint32_t MySqlRow::getUInt(uint32_t columnIndex) const { return getVal<uint32_t>(columnIndex); }
int64_t MySqlRow::getInt64(const char8_t* columnName) const { return getValByName<int64_t>(columnName); }
int64_t MySqlRow::getInt64(uint32_t columnIndex) const { return getVal<int64_t>(columnIndex); }
uint64_t MySqlRow::getUInt64(const char8_t* columnName) const { return getValByName<uint64_t>(columnName); }
uint64_t MySqlRow::getUInt64(uint32_t columnIndex) const { return getVal<uint64_t>(columnIndex); }

// MySQL BOOLEAN columns: TINYINT(1), (value == 0) => false and (value != 0) => true
bool MySqlRow::getBool(const char8_t* columnName) const { return (getChar(columnName) != 0); }
bool MySqlRow::getBool(uint32_t columnIndex) const { return (getChar(columnIndex) != 0); }

// Timestamp values retrieved via UNIX_TIMESTAMP(someColumn) AS columnName
EA::TDF::TimeValue MySqlRow::getUnixTimestamp(const char8_t* columnName) const
{
    EA::TDF::TimeValue unixTimestamp;
    unixTimestamp.setSeconds(getInt64(columnName));
    return unixTimestamp;
}
EA::TDF::TimeValue MySqlRow::getUnixTimestamp(uint32_t columnIndex) const
{
    EA::TDF::TimeValue unixTimestamp;
    unixTimestamp.setSeconds(getInt64(columnIndex));
    return unixTimestamp;
}

// MySQL TIMESTAMP / DATETIME values retrieved as string values
EA::TDF::TimeValue MySqlRow::getTimeValue(const char8_t* columnName) const
{
    const char8_t* dbDateTime = getString(columnName);
    return Blaze::DbUtil::dateTimeToTimeValue(dbDateTime);
}
EA::TDF::TimeValue MySqlRow::getTimeValue(uint32_t columnIndex) const
{
    const char8_t* dbDateTime = getString(columnIndex);
    return Blaze::DbUtil::dateTimeToTimeValue(dbDateTime);
}

template<typename T>
T MySqlRow::getVal(uint32_t columnIndex) const
{
    T retval = 0;
    if ((columnIndex >= mResult->mNumFields) || (mRow[columnIndex] == nullptr))
        return retval;

    if (EA_UNLIKELY(blaze_str2int(mRow[columnIndex], &retval) == mRow[columnIndex]))
    {
        BLAZE_WARN_LOG(Log::DB, "[MySqlRow].getVal: Warning, truncated value "
            << mRow[columnIndex] << " for column " << mResult->mFields[columnIndex].name);
    }

    return retval;
}

template<typename T>
T MySqlRow::getValByName(const char8_t* columnName) const
{
    return getVal<T>(getColumnIndex(columnName));
}

}// Blaze
