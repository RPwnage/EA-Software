/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_MySqlRow_H
#define BLAZE_MySqlRow_H
/*** Include files *******************************************************************************/
#include "framework/database/mysql/blazemysql.h"
#include "EASTL/string.h"
#include "framework/database/dbrow.h"
#include "framework/logger.h"
#include "framework/util/less_ptr.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class MySqlResultBase;

/*! *****************************************************************************/
/*! \class  MySqlRow
    \brief  MYSQL-specific implementation of DbRow class
    \note   IMPORTANT! MySqlResult stores a vector of MySqlRow objects to minimize allocations.
            Consequently, it is imperative that MySqlRow objects have a valid trivial copy 
            constructor and assignment operator.
            i.e.: MySqlRow objects shall NEVER own any memory or resources!
 */
class MySqlRow : public DbRow
{
public:
    MySqlRow(MySqlResultBase* result, MYSQL_ROW row, unsigned long *datasizes)
        : mResult(result), mRow(row), mDatasizes(datasizes) {}

    const char8_t* getString(const char8_t* columnName) const override;
    const char8_t* getString(uint32_t columnIndex) const override;
    bool getBool(const char8_t* columnName) const override;
    bool getBool(uint32_t columnIndex) const override;
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
    const char8_t* toString(eastl::string& buf, int32_t maxStringLen = 0, int32_t maxBinaryLen = 0) const override;
    EA::TDF::TimeValue getUnixTimestamp(const char8_t* columnName) const override;
    EA::TDF::TimeValue getUnixTimestamp(uint32_t columnIndex) const override;
    EA::TDF::TimeValue getTimeValue(const char8_t* columnName) const override;
    EA::TDF::TimeValue getTimeValue(uint32_t columnIndex) const override;

private:
    MySqlResultBase* mResult; // not owned
    MYSQL_ROW mRow; // not owned
    unsigned long* mDatasizes; // not owned
    
    uint32_t getColumnIndex(const char8_t* columnName) const;

    template<typename T>
    T getVal(uint32_t columnIndex) const;

    template<typename T>
    T getValByName(const char8_t* columnName) const;
};

}// Blaze
#endif // BLAZE_MySqlRow_H

