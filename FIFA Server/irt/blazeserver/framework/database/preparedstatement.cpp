/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/database/preparedstatement.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{

PreparedStatementInfo::PreparedStatementInfo(
        PreparedStatementId id, const char8_t* name, const char8_t* query)
    : mId(id),
      mName(blaze_strdup(name)),
      mQuery(blaze_strdup(query))
{
}

PreparedStatementInfo::~PreparedStatementInfo()
{
    BLAZE_FREE(mName);
    BLAZE_FREE(mQuery);
}

PreparedStatement::PreparedStatement(const PreparedStatementInfo& info)
    : mId(info.getId()),
      mName(blaze_strdup(info.getName())),
      mQuery(blaze_strdup(info.getQuery())),
      mRegistered(false)
{
    reset();
}

PreparedStatement::~PreparedStatement()
{
    BLAZE_FREE(mName);
    BLAZE_FREE(mQuery);

    reset();
}

void PreparedStatement::setInt64(uint32_t column, int64_t value)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_INT64;
    data.mInt64 = value;
    data.mIsNull = false;
}

void PreparedStatement::setInt32(uint32_t column, int32_t value)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_INT32;
    data.mInt32 = value;
    data.mIsNull = false;
}

void PreparedStatement::setInt16(uint32_t column, int16_t value)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_INT16;
    data.mInt16 = value;
    data.mIsNull = false;
}

void PreparedStatement::setInt8(uint32_t column, int8_t value)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_INT8;
    data.mInt8 = value;
    data.mIsNull = false;
}

void PreparedStatement::setUInt64(uint32_t column, uint64_t value)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_UINT64;
    data.mUInt64 = value;
    data.mIsNull = false;
}

void PreparedStatement::setUInt32(uint32_t column, uint32_t value)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_UINT32;
    data.mUInt32 = value;
    data.mIsNull = false;
}

void PreparedStatement::setUInt16(uint32_t column, uint16_t value)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_UINT16;
    data.mUInt16 = value;
    data.mIsNull = false;
}

void PreparedStatement::setUInt8(uint32_t column, uint8_t value)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_UINT8;
    data.mUInt8 = value;
    data.mIsNull = false;
}

void PreparedStatement::setString(uint32_t column, const char8_t* value)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_STRING;
    if (value == nullptr)
    {
        data.mString = nullptr;
        data.mIsNull = true;
    }
    else
    {
        data.mString = blaze_strdup(value);
        data.mIsNull = false;
    }
}

void PreparedStatement::setFloat(uint32_t column , float value)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_FLOAT;
    data.mFloat = value;
    data.mIsNull = false;
}

void PreparedStatement::setDouble(uint32_t column , double value)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_DOUBLE;
    data.mDouble = value;
    data.mIsNull = false;
}

void PreparedStatement::setBinary(uint32_t column, const uint8_t* value, size_t len)
{
    ParamData& data = getColumn(column);
    data.mType = ParamData::TYPE_BINARY;
    if (value == nullptr)
    {
        data.mBinary.data = nullptr;
        data.mBinary.len = 0;
        data.mIsNull = true;
    }
    else
    {
        data.mBinary.data = (uint8_t*) BLAZE_ALLOC_MGID(len, DEFAULT_BLAZE_MEMGROUP, "PreparedStatement::BinaryData");
        memcpy(data.mBinary.data, value, len);
        data.mBinary.len = len;
        data.mIsNull = false;
    }
}

void PreparedStatement::reset()
{
    ParamList::iterator itr = mParams.begin();
    ParamList::iterator end = mParams.end();
    for (; itr != end; ++itr)
    {
        ParamData& data = *itr;

        if (data.mType == ParamData::TYPE_STRING)
            BLAZE_FREE(data.mString);
        else if (data.mType == ParamData::TYPE_BINARY)
            BLAZE_FREE(data.mBinary.data);
    }

    mParams.clear();
}

PreparedStatement::ParamData& PreparedStatement::getColumn(uint32_t column)
{
    if (column >= mParams.size())
    {
        size_t newElements = column - mParams.size() + 1;
        while (newElements > 0)
        {
            mParams.push_back();
            newElements--;
        }
        return mParams.back();
    }
    return mParams[column];
}

const char8_t* PreparedStatement::toString(
        eastl::string& buffer, int32_t maxStringLen, int32_t maxBinaryLen) const
{
    buffer.append_sprintf("\n    Name: %s\n    Query: %s\n    Registered: %s\n",
            mName, mQuery, mRegistered ? "true" : "false");
    ParamList::const_iterator itr = mParams.begin();
    ParamList::const_iterator end = mParams.end();
    for(int32_t idx = 0; itr != end; ++itr, ++idx)
    {
        const ParamData& data = *itr;

        switch (data.mType)
        {
            case ParamData::TYPE_INT8:
                buffer.append_sprintf("      %d: int8_t: %d\n", idx, data.mInt8);
                break;
            case ParamData::TYPE_INT16:
                buffer.append_sprintf("      %d: int16_t: %d\n", idx, data.mInt16);
                break;
            case ParamData::TYPE_INT32:
                buffer.append_sprintf("      %d: int32_t: %d\n", idx, data.mInt32);
                break;
            case ParamData::TYPE_INT64:
                buffer.append_sprintf("      %d: int64_t: %" PRId64"\n", idx, data.mInt64);
                break;
            case ParamData::TYPE_UINT8:
                buffer.append_sprintf("      %d: uint8_t: %u\n", idx, data.mUInt8);
                break;
            case ParamData::TYPE_UINT16:
                buffer.append_sprintf("      %d: uint16_t: %u\n", idx, data.mUInt16);
                break;
            case ParamData::TYPE_UINT32:
                buffer.append_sprintf("      %d: uint32_t: %u\n", idx, data.mUInt32);
                break;
            case ParamData::TYPE_UINT64:
                buffer.append_sprintf("      %d: uint64_t: %" PRIu64"\n", idx, data.mUInt64);
                break;
            case ParamData::TYPE_STRING:
                buffer.append_sprintf("      %d: string:", idx);
                if (data.mIsNull)
                {
                    buffer.append(" <nullptr>\n");
                    continue;
                }
                if (maxStringLen == 0)
                    buffer.append_sprintf(" '%s'\n", data.mString);
                else
                    buffer.append_sprintf(" '%.*s'\n", maxStringLen, data.mString);
                break;
            case ParamData::TYPE_DOUBLE:
                buffer.append_sprintf("      %d: double: %f\n", idx, data.mDouble);
                break;
            case ParamData::TYPE_FLOAT:
                buffer.append_sprintf("      %d: float: %f\n", idx, data.mFloat);
                break;
            case ParamData::TYPE_BINARY:
            {
                buffer.append_sprintf("      %d: binary:", idx);
                if (data.mIsNull)
                {
                    buffer.append(" <nullptr>\n");
                    continue;
                }
                size_t binLen = data.mBinary.len;
                if ((maxBinaryLen > 0) && (binLen > (size_t)maxBinaryLen))
                    binLen = (size_t)maxBinaryLen;
                for(size_t i = 0; i < binLen; ++i)
                    buffer.append_sprintf(" %02x", data.mBinary.data[i]);
                buffer.append("\n");
                break;
            }
            case ParamData::TYPE_NONE:
                break;
        }
    }
    return buffer.c_str();
}


} // Blaze

