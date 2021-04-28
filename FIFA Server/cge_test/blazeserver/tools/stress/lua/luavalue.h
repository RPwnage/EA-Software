/*************************************************************************************************/
/*!
    \file   luavalue.h

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luavalue.h#2 $
    $Change: 44737 $
    $DateTime: 2009/06/04 14:33:39 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

#ifndef _LUAVALUE_H
#define _LUAVALUE_H

struct lua_State;

#include "luainterpreter.h"

typedef int32_t LuaTable;

namespace Blaze
{
namespace Stress
{

/*************************************************************************************************/
/*!
    \class LuaValue

    Provides a variant-type class for easily handling Lua values in C++ having types that
    may not be known.

*/
/*************************************************************************************************/
class LuaValue
{
public:

    enum Type { LUATYPE_NONE, LUATYPE_BOOL, LUATYPE_INT64, LUATYPE_UINT64,
                LUATYPE_NUMBER, LUATYPE_STRING, LUATYPE_TABLE, LUATYPE_BLOB };

    inline LuaValue() : mType(LUATYPE_NONE), mLuaState(nullptr) {}
   
    LuaValue(const LuaValue &other);
    ~LuaValue();

    LuaValue &operator=(const LuaValue &other);

    inline Type          GetType() const { return mType; }

    void                 SetTable(LuaTable value);
    LuaTable             GetTable() const;
    void                 SetString(const eastl::string &value);
    const eastl::string &GetString() const;
    void                 SetInt(int32_t value);
    int32_t              GetInt() const;
    void                 SetUInt(uint32_t value);
    uint32_t             GetUInt() const;
    void                 SetNumber(double value);
    double               GetNumber() const;
    void                 SetFloat(float value);
    float                GetFloat() const;
    void                 SetInt64(int64_t value);
    int64_t              GetInt64() const;
    void                 SetUInt64(uint64_t value);
    uint64_t             GetUInt64() const;
    void                 SetInt16(int16_t value);
    int16_t              GetInt16() const;
    void                 SetUInt16(uint16_t value);
    uint16_t             GetUInt16() const;
    void                 SetInt8(int8_t value);
    int8_t               GetInt8() const;
    void                 SetUInt8(uint8_t value);
    uint8_t              GetUInt8() const;
    void                 SetBool(bool value);
    bool GetBool() const;

    void SetBlob(const uint8_t* data, uint32_t length);
    const uint8_t* GetBlobData() const;
    uint32_t GetBlobSize() const;

    // Thread, Function, UserData types could easily be added, if desired

    void                 Push(lua_State *lua) const;
    static LuaValue      Pop(lua_State *lua);

    eastl::string        ToString(lua_State *lua) const;

    inline static LuaValue Table(LuaTable value)              { LuaValue lua; lua.SetTable(value);  return lua; }
    inline static LuaValue Int(int32_t value)                 { LuaValue lua; lua.SetInt(value);    return lua; }
    inline static LuaValue UInt(uint32_t value)               { LuaValue lua; lua.SetUInt(value);   return lua; }
    inline static LuaValue Int64(int64_t value)               { LuaValue lua; lua.SetInt64(value);  return lua; }
    inline static LuaValue UInt64(uint64_t value)             { LuaValue lua; lua.SetUInt64(value); return lua; }
    inline static LuaValue Int16(int16_t value)               { LuaValue lua; lua.SetInt16(value);  return lua; }
    inline static LuaValue UInt16(uint16_t value)             { LuaValue lua; lua.SetUInt16(value); return lua; }
    inline static LuaValue Int8(int8_t value)                 { LuaValue lua; lua.SetInt8(value);  return lua; }
    inline static LuaValue UInt8(uint8_t value)               { LuaValue lua; lua.SetUInt8(value); return lua; }
    inline static LuaValue Bool(bool value)                   { LuaValue lua; lua.SetBool(value);   return lua; }
    inline static LuaValue Number(double value)               { LuaValue lua; lua.SetNumber(value); return lua; }
    inline static LuaValue Float(float value)                 { LuaValue lua; lua.SetFloat(value);  return lua; }
    inline static LuaValue String(const char8_t *value)       { LuaValue lua; lua.SetString(value); return lua; }
    inline static LuaValue String(const eastl::string &value) { LuaValue lua; lua.SetString(value); return lua; }
    inline static LuaValue Blob(const uint8_t *value, uint32_t length) { LuaValue lua; lua.SetBlob(value, length);   return lua; }
    inline static LuaValue None()                             { LuaValue lua; return lua; }

    static const char* GetTypeName(Type);

private:

    bool Verify(Type type) const;

    Type          mType;
    union
    {
        bool      mBool;
        double    mNumber;
        int64_t   mInt64;
        LuaTable  mTable;
        char8_t   mData[sizeof(double)]; // mNumber is the largest, mData is for copying
    };
    eastl::string mString;
    lua_State* mLuaState;
        
    enum { MAX_INT64_STRING_SIZE = 128 };

    static const eastl::string EMPTY;
};

/*************************************************************************************************/
/*!
    \class LuaKeyValue

    A simple key-value pair of LuaValues, especially useful for associative array (map/hash table)
    traversal.

*/
/*************************************************************************************************/
struct LuaKeyValue
{
    LuaValue key,
             value;
};

/*************************************************************************************************/
/*!
    \class LuaTableIterator

    Provides an easy to use iterator for LuaTables.  Is able to iterate over just values (for a
    list-type table) or key-value pairs (as an associative table).

*/
/*************************************************************************************************/
class LuaTableIterator
{
public:

    LuaTableIterator(lua_State *state, LuaTable table);
    ~LuaTableIterator();
    
    bool        HasNext() const;
    LuaValue    Next();
    LuaKeyValue NextKeyValue();

private:

    lua_State  *mState;
    bool        mHasNext;
};

} // Stress
} // Blaze

#endif // _LUAVALUE_H
