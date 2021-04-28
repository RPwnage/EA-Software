/*************************************************************************************************/
/*!
    \file   luavalue.cpp

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luavalue.cpp#4 $
    $Change: 44939 $
    $DateTime: 2009/06/05 14:42:23 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LuaValue

    Provides a variant-type class for easily handling Lua values in C++ having types that
    may not be known.

*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"
#include "luainterpreter.h"
#include "luavalue.h"

#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#define strtoll  _strtoi64
#define strtoull _strtoui64
#endif

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


namespace Blaze
{
namespace Stress
{

#define BLOB_SENTINEL_SIZE 6

static const char8_t* luaValueTypeNames[] =
{
    "NONE",
    "BOOL",
    "INT64",
    "UINT64",
    "NUMBER",
    "STRING",
    "TABLE",
    "BLOB"
};

const eastl::string LuaValue::EMPTY("");

const char8_t* LuaValue::GetTypeName(LuaValue::Type type)
{
    if(type >= 0 && type <= LUATYPE_BLOB)
    {
        return luaValueTypeNames[type];
    }
    else
    {
        return "UNDEFINED TYPE";
    }
}

LuaValue::LuaValue(const LuaValue &other) : mType(other.mType), mString(other.mString), mLuaState(other.mLuaState)
{
    const_cast<LuaValue &>(other).mLuaState = nullptr;  // Take ownership of the table reference

    memcpy(mData, other.mData, sizeof(mData));
}

LuaValue::~LuaValue()
{
    // Unref any references that came from the 'Pop' call. 
    if (mLuaState != nullptr && mType == LUATYPE_TABLE)
    {
        luaL_unref(mLuaState, LUA_REGISTRYINDEX, mTable);
    }
}

LuaValue &LuaValue::operator=(const LuaValue &other)
{
    if (this != &other)
    {
        mType   = other.mType;
        mString = other.mString;
        mLuaState = other.mLuaState;
        memcpy(mData, other.mData, sizeof(mData));

        const_cast<LuaValue &>(other).mLuaState = nullptr;  // Take ownership of the table reference
    }
    return *this;
}

bool LuaValue::Verify(Type type) const
{
    if (mType != type)
    {
        EA_ASSERT_FORMATTED(mType == type, ("Specified lua value type [%s] is not in its expected type [%s], check module inputs to RPC calls.", GetTypeName(mType), GetTypeName(type)));
    }
    return mType == type;
}

void LuaValue::SetTable(LuaTable value)
{ 
    mTable = value;
    mType = LUATYPE_TABLE;
}

LuaTable LuaValue::GetTable() const
{
    if (!Verify(LUATYPE_TABLE))
    {
        return LUA_NOREF;
    }
    return mTable;
}

void LuaValue::SetString(const eastl::string &value)
{
    mString = value;
    mType = LUATYPE_STRING;
}

const eastl::string &LuaValue::GetString() const
{
    if (!Verify(LUATYPE_STRING))
    {
        return EMPTY;
    }
    return mString;
}

void LuaValue::SetInt(int32_t value)
{
    mInt64  = static_cast<int64_t>(value);
    mType   = LUATYPE_INT64;
}

int32_t LuaValue::GetInt() const
{
    return static_cast<int32_t>(GetInt64());
}

void LuaValue::SetUInt(uint32_t value)
{
    mInt64  = static_cast<int64_t>(value);
    mType   = LUATYPE_INT64;
}

uint32_t LuaValue::GetUInt() const
{
    return static_cast<uint32_t>(GetInt64());
}

void LuaValue::SetNumber(double value)
{
    mNumber = value;
    mType = LUATYPE_NUMBER;
}

double LuaValue::GetNumber() const
{
    if (!Verify(LUATYPE_NUMBER))
    {
        return 0.0;
    }
    return mNumber;
}

void LuaValue::SetFloat(float value)
{
    mNumber = static_cast<double>(value);
    mType = LUATYPE_NUMBER;
}

float LuaValue::GetFloat() const
{
    if (!Verify(LUATYPE_NUMBER))
    {
        return 0.0f;
    }
    return static_cast<float>(mNumber);
}

void LuaValue::SetBool(bool value)
{ 
    mBool = value;
    mType = LUATYPE_BOOL;
}

bool LuaValue::GetBool() const
{
    if (!Verify(LUATYPE_BOOL))
    {
        return false;
    }
    return mBool;
}

static bool IsInt64(const char8_t *text)
{
    return strncmp(text, "#i64:", 5) == 0;
}

static bool IsUInt64(const char8_t *text)
{
    return strncmp(text, "#u64:", 5) == 0;
}

static bool IsBlob(const char8_t *text)
{
    return strncmp(text, "#blob:", BLOB_SENTINEL_SIZE) == 0;
}

static bool StringToInt64(const char8_t *text, int64_t &value)
{
    if (!IsInt64(text))
    {
        return false;
    }
    const int32_t DECIMAL_RADIX = 10;
    text += 5; // get rid of the type prefix
    const char8_t *ends = text;
    value = (int64_t)strtoll(text, const_cast<char8_t **>(&ends), DECIMAL_RADIX);
    return (ends != nullptr && ends != text);
}

static bool StringToUInt64(const char8_t *text, uint64_t &value)
{
    if (!IsUInt64(text))
    {
        return false;
    }
    const int32_t DECIMAL_RADIX = 10;
    text += 5; // get rid of the type prefix
    const char8_t *ends = text;
    value = (uint64_t)strtoull(text, const_cast<char8_t **>(&ends), DECIMAL_RADIX);
    return (ends != nullptr && ends != text);
}

static void UInt64ToString(uint64_t value, char8_t *buffer, size_t bufferLen)
{
#ifdef WIN32
    blaze_snzprintf(buffer, bufferLen, "#u64:%I64u", value);
#else
    blaze_snzprintf(buffer, bufferLen, "#u64:%llu",  (long long unsigned int)value);
#endif
}

void LuaValue::SetInt64(int64_t value)
{
    mInt64 = value;
    mType = LUATYPE_INT64;
}

int64_t LuaValue::GetInt64() const
{
    if (mType == LUATYPE_NUMBER)
    {
        return static_cast<int64_t>(mNumber);
    }
    else if (mType == LUATYPE_INT64)
    {
        return mInt64;
    }
    else
    {
        int64_t value = 0;
        if (/*Verify(LUATYPE_INT64) &&*/ !StringToInt64(mString.c_str(), value))
        {
            EA_FAIL_MSG("value is not an int64!");
        }
        return value;
    }
}

void LuaValue::SetUInt64(uint64_t value)
{
    char8_t buffer[MAX_INT64_STRING_SIZE];
    UInt64ToString(value, buffer, sizeof(buffer));
    mString = buffer;
    mType = LUATYPE_UINT64;
}

uint64_t LuaValue::GetUInt64() const
{
    if (mType == LUATYPE_NUMBER)
    {
        return static_cast<uint64_t>(mNumber);
    }
    else if (mType == LUATYPE_INT64)
    {
        return static_cast<uint64_t>(mInt64);
    }
    else
    {
        uint64_t value = 0;
        if (!StringToUInt64(mString.c_str(), value))
        {
            EA_FAIL_MSG("value is not a uint64!");
        }
        return value;
    }
}

void LuaValue::SetInt16(int16_t value)
{
    SetInt(value);
}

int16_t LuaValue::GetInt16() const
{
    return (int16_t) GetInt();
}

void LuaValue::SetUInt16(uint16_t value)
{
    SetUInt(value);
}

uint16_t LuaValue::GetUInt16() const
{
    return (uint16_t) GetUInt();
}

void LuaValue::SetInt8(int8_t value)
{
    SetInt(value);
}

int8_t LuaValue::GetInt8() const
{
    return (int8_t) GetInt();
}

void LuaValue::SetUInt8(uint8_t value)
{
    SetUInt(value);
}

uint8_t LuaValue::GetUInt8() const
{
    return (uint8_t) GetUInt();
}

const uint8_t* LuaValue::GetBlobData() const
{
    if (!Verify(LUATYPE_BLOB))
    {
        return nullptr;
    }
    return (const uint8_t*)(&mString[BLOB_SENTINEL_SIZE]);
}

uint32_t LuaValue::GetBlobSize() const
{
    if(!Verify(LUATYPE_BLOB))
    {
        return 0;
    }
    return mString.size()-BLOB_SENTINEL_SIZE;
}

void LuaValue::SetBlob(const uint8_t* data, uint32_t length)
{
    mString = "#blob:";
    mString.resize(length+BLOB_SENTINEL_SIZE, 0);
    memcpy(&mString[BLOB_SENTINEL_SIZE], data, length);
    mType = LUATYPE_BLOB;
}

eastl::string LuaValue::ToString(lua_State *lua) const
{
    Push(lua);
    const char8_t *str = lua_tostring(lua, -1);
    lua_pop(lua, 1);
    return eastl::string(str == nullptr ? "" : str);
}

void LuaValue::Push(lua_State *lua) const
{
    switch (mType)
    {
    case LUATYPE_NONE:
        lua_pushnil(lua);
        break;
    case LUATYPE_BOOL:
        lua_pushboolean(lua, mBool ? 1 : 0);
        break;
    case LUATYPE_INT64:
        lua_pushinteger(lua, mInt64);
        break;
    case LUATYPE_UINT64:
    case LUATYPE_STRING:
        lua_pushstring(lua, mString.c_str());
        break;
    case LUATYPE_BLOB:   
        lua_pushlstring(lua, mString.data(), mString.size());
        break;
    case LUATYPE_NUMBER:
        lua_pushnumber(lua, mNumber);
        break;
    case LUATYPE_TABLE:
        lua_rawgeti(lua, LUA_REGISTRYINDEX, static_cast<int32_t>(mTable));
        break;
    default:
        // $TODO assert/error
        break;
    }
}

LuaValue LuaValue::Pop(lua_State *lua)
{
    LuaValue value; // starts off null
    switch (lua_type(lua, -1))
    {
    case LUA_TNONE:
        // assert/error here?
        break;
    case LUA_TNIL:
        lua_pop(lua, 1);
        break;
    case LUA_TNUMBER:
        if (lua_isinteger(lua, -1))
            value.SetInt64(lua_tointeger(lua, -1));
        else
            value.SetNumber(lua_tonumber(lua, -1));
        lua_pop(lua, 1);
        break;
    case LUA_TBOOLEAN:
        value.SetBool(lua_toboolean(lua, -1) != 0);
        lua_pop(lua, 1);
        break;
    case LUA_TSTRING:
        {
            size_t textlen;
            const char8_t *text = lua_tolstring(lua, -1, &textlen);
            uint64_t       uint64 = 0;
            int64_t        int64 = 0;
            if (*text == '#' && IsUInt64(text) && StringToUInt64(text, uint64))
            {
                value.SetUInt64(uint64);
            }
            else if (*text == '#' && IsInt64(text) && StringToInt64(text, int64))
            {
                value.SetInt64(int64);
            }
            else if (*text == '#' && IsBlob(text))
            {
                value.SetBlob((const uint8_t*)(text)+BLOB_SENTINEL_SIZE, textlen-BLOB_SENTINEL_SIZE);
            }
            else
            {
                value.SetString(text);
            }
            lua_pop(lua, 1);
        }
        break;
    case LUA_TTABLE:
        // This adds a reference to the table, which we own until the LuaValue is destroyed (at which point the table may be destroyed, if no new references exist for it).
        value.SetTable((LuaTable)luaL_ref(lua, LUA_REGISTRYINDEX));
        value.mLuaState = lua;
        break;
    default:
        // we don't support the other types here
        // $TODO assert/error
        break;
    }
    return value;
}

/*************************************************************************************************/
/*!
    \class LuaTableIterator

    Provides an easy to use iterator for LuaTables.  Is able to iterate over just values (for a
    list-type table) or key-value pairs (as an associative table).

*/
/*************************************************************************************************/

LuaTableIterator::LuaTableIterator(lua_State *state, LuaTable table) : mState(state), mHasNext(true)
{        
    lua_rawgeti(mState, LUA_REGISTRYINDEX, (int)table); // here we need to push the table onto the stack (at -2)
    lua_pushnil(mState);                                // this is required to prime the iteration       (at -1)
    mHasNext = lua_next(mState, -2) != 0;
}

LuaTableIterator::~LuaTableIterator()
{
    //cleanup for nested tables
    if (!mHasNext) 
    {
        lua_pop(mState, 1);
    }
}


bool LuaTableIterator::HasNext() const
{
    return mHasNext;    
}

LuaValue LuaTableIterator::Next()
{
    LuaValue value(LuaValue::Pop(mState));
    mHasNext = lua_next(mState, -2) != 0;

    return value;
}

LuaKeyValue LuaTableIterator::NextKeyValue()
{
    LuaKeyValue keyValue;
    keyValue.value = LuaValue::Pop(mState);
    lua_pushvalue(mState, -1); // duplicate the top element
    keyValue.key = LuaValue::Pop(mState);
    mHasNext = lua_next(mState, -2) != 0;

    return keyValue;
}

} // Stress
} // Blaze
