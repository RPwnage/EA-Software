/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_DBROW_H
#define BLAZE_DBROW_H
/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class DbRow
{
public:
    virtual ~DbRow() { }

    virtual bool getBool(const char8_t* columnName) const = 0;
    virtual bool getBool(uint32_t columnIndex) const = 0;
    virtual int8_t getChar(const char8_t* columnName) const =0;
    virtual int8_t getChar(uint32_t columnIndex) const =0;
    virtual uint8_t getUChar(const char8_t* columnName) const =0;
    virtual uint8_t getUChar(uint32_t columnIndex) const =0;
    virtual int16_t getShort(const char8_t* columnName) const =0;
    virtual int16_t getShort(uint32_t columnIndex) const =0;
    virtual uint16_t getUShort(const char8_t* columnName) const =0;
    virtual uint16_t getUShort(uint32_t columnIndex) const =0;
    virtual int32_t getInt(const char8_t* columnName) const =0;
    virtual int32_t getInt(uint32_t columnIndex) const =0;
    virtual uint32_t getUInt(const char8_t* columnName) const =0;
    virtual uint32_t getUInt(uint32_t columnIndex) const =0;
    virtual int64_t getInt64(const char8_t* columnName) const =0;
    virtual int64_t getInt64(uint32_t columnIndex) const =0;
    virtual uint64_t getUInt64(const char8_t* columnName) const =0;
    virtual uint64_t getUInt64(uint32_t columnIndex) const =0;
    virtual float_t getFloat(const char8_t* columnName) const =0;
    virtual float_t getFloat(uint32_t columnIndex) const =0;
    virtual double_t getDouble(const char8_t* columnName) const =0;
    virtual double_t getDouble(uint32_t columnIndex) const =0;
    virtual const uint8_t* getBinary(const char8_t* columnName, size_t* len) const =0;
    virtual const uint8_t* getBinary(uint32_t columnIndex, size_t* len) const =0;
    virtual const char8_t* getString(const char8_t* columnName) const =0;
    virtual const char8_t* getString(uint32_t columnIndex) const =0;
    virtual const char8_t* toString(eastl::string& buf, int32_t maxStringLen = 0, int32_t maxBinaryLen = 0) const = 0;
    virtual EA::TDF::TimeValue getUnixTimestamp(const char8_t* columnName) const = 0;
    virtual EA::TDF::TimeValue getUnixTimestamp(uint32_t columnIndex) const = 0;
    virtual EA::TDF::TimeValue getTimeValue(const char8_t* columnName) const = 0;
    virtual EA::TDF::TimeValue getTimeValue(uint32_t columnIndex) const = 0;

    //Others types to follow
};

}// Blaze
#endif // BLAZE_DBROW_H

