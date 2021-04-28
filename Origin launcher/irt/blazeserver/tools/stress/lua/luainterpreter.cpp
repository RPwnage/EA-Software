/*************************************************************************************************/
/*!
    \file   luainterpreter.cpp

    $Header: //blaze/games/Madden/2010-NG/trunk/component/franchise/stress/luainterpreter.cpp#2 $
    $Change: 44737 $
    $DateTime: 2009/06/04 14:33:39 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LuaInterpreter

    Wraps the Lua interpreter for the purposes of stress testing, and serves as a factory for
    Lua threads.  Note that there will be ONE instance of LuaInterpreter PER OS thread.  This is
    a requirement to maintain thread safety in Lua.

*/
/*************************************************************************************************/

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"
#include "authentication/tdf/authentication.h"
#include "loginmanager.h"
#include "luainterpreter.h"
#include "luathread.h"
#include "luastats.h"
#include "luacommontypes.h"
#include "luastressinstance.h"
#include <EAStdC/EACType.h>


namespace Blaze
{
namespace Stress
{

    //using namespace Stress;


    eastl::string getLuaFormattedName(const char* tdfMemberName)
    {
        eastl::string name = tdfMemberName;
        name[0] = EA::StdC::Toupper(name[0]);
        return name;
    }

    void decodeMember(LuaThread* luaThread, LuaValue luaValue, EA::TDF::TdfGenericReference outValue)
    {
        // luaThread - Owns the LuaState we're using
        // luaValue - The value we're decoding (should not be invalid)
        // outValue - The value we're decoding into (Must be referencing the correct type already)
        if (luaThread == nullptr || luaValue.GetType() == LuaValue::LUATYPE_NONE)
        {
            // If the value is missing, then that's okay.  Not every value has to be parsed.
            return;
        }

        switch (outValue.getType())
        {
            // Integral Types:
        case EA::TDF::TDF_ACTUAL_TYPE_BOOL:         outValue.asBool() = luaValue.GetBool();         break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT8:         outValue.asInt8() = luaValue.GetInt8();               break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT8:        outValue.asUInt8() = luaValue.GetUInt8();              break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT16:        outValue.asInt16() = luaValue.GetInt16();              break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT16:       outValue.asUInt16() = luaValue.GetUInt16();             break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT32:        outValue.asInt32() = luaValue.GetInt();              break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT32:       outValue.asUInt32() = luaValue.GetUInt();             break;
        case EA::TDF::TDF_ACTUAL_TYPE_INT64:        outValue.asInt64() = luaValue.GetInt64();              break;
        case EA::TDF::TDF_ACTUAL_TYPE_UINT64:       outValue.asUInt64() = luaValue.GetUInt64();             break;
        case EA::TDF::TDF_ACTUAL_TYPE_BITFIELD:     outValue.asBitfield().setBits(luaValue.GetUInt());    break;   // No special case for bitfield members as a table
        case EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE:    outValue.asTimeValue().setMicroSeconds(luaValue.GetInt64());  break;  // No special case for time value as a string
        case EA::TDF::TDF_ACTUAL_TYPE_ENUM:
        {
            if (luaValue.GetType() == LuaValue::LUATYPE_STRING)
                outValue.getTypeDescription().asEnumMap()->findByName(luaValue.GetString().c_str(), outValue.asEnum());
            else
                outValue.asEnum() = (luaValue.GetInt());
            break;
        }

        // Float/String/Blob Types:
        case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:        outValue.asFloat() = luaValue.GetFloat();             break;
        case EA::TDF::TDF_ACTUAL_TYPE_STRING:       outValue.asString() = luaValue.GetString().c_str();     break;
        case EA::TDF::TDF_ACTUAL_TYPE_BLOB:         outValue.asBlob().setData(luaValue.GetBlobData(), luaValue.GetBlobSize());  break;

            // Weird, Vestigial, Tiny hardcoded-class Types:
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_TYPE:  outValue.asObjectType() = convertFromLua_Blaze_Object_Type(luaThread, luaValue.GetTable());   break;
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_ID:    outValue.asObjectId() = convertFromLua_Blaze_Object_Id(luaThread, luaValue.GetTable());       break;

            // Templated Types:
        case EA::TDF::TDF_ACTUAL_TYPE_LIST:
        {
            if (luaValue.GetType() == LuaValue::LUATYPE_TABLE)
            {
                LuaTable luaListTable = luaValue.GetTable();
                outValue.asList().clearVector();
                for (LuaTableIterator i(luaThread->GetLuaState(), luaListTable); i.HasNext();)
                {
                    EA::TDF::TdfGenericReference newListValue;
                    outValue.asList().pullBackRef(newListValue);

                    // next() pops the current value, and advances the list. (Not sure why it's not just called popFront()).
                    LuaValue luaListValue = i.Next();
                    decodeMember(luaThread, luaListValue, newListValue);
                }
            }
            break;
        }

        case EA::TDF::TDF_ACTUAL_TYPE_MAP:
        {
            if (luaValue.GetType() == LuaValue::LUATYPE_TABLE)
            {
                LuaTable luaListTable = luaValue.GetTable();
                outValue.asList().clearVector();
                for (LuaTableIterator i(luaThread->GetLuaState(), luaListTable); i.HasNext();)
                {
                    // Decode the Map Key: 
                    EA::TDF::TdfGenericValue key;
                    key.setType(outValue.asMap().getKeyTypeDesc());
                    EA::TDF::TdfGenericReference keyRef(key);

                    LuaKeyValue luaListValue = i.NextKeyValue();
                    decodeMember(luaThread, luaListValue.key, keyRef);

                    // Insert & Decode the Value: 
                    EA::TDF::TdfGenericReference newValueRef;
                    if (!outValue.asMap().insertKeyGetValue(keyRef, newValueRef))
                    {
                        // If there's an error inserting a key, attempt to continue with the rest of the map:
                        continue;
                    }
                    decodeMember(luaThread, luaListValue.value, newValueRef);
                }
            }
            break;
        }

        // Class Types:
        case EA::TDF::TDF_ACTUAL_TYPE_UNION:
        {
            if (luaValue.GetType() == LuaValue::LUATYPE_TABLE)
            {
                const EA::TDF::TypeDescriptionClass* classTypeDesc = outValue.asUnion().getTypeDescription().asClassInfo();

                bool setUnion = false;
                const EA::TDF::TdfMemberInfo* classMemberInfo = nullptr;
                for (uint32_t index = 0; classTypeDesc->getMemberInfoByIndex(index, classMemberInfo); ++index)
                {
                    // For the Union, we need to know what the type is before we can parse the member:
                    eastl::string luaClassMemberName = getLuaFormattedName(classMemberInfo->getMemberName());
                    LuaValue luaMemberValue = luaThread->GetLuaProperty(luaValue.GetTable(), luaClassMemberName.c_str());
                    if (luaMemberValue.GetType() != LuaValue::LUATYPE_NONE)
                    {
                        outValue.asUnion().switchActiveMember(index);
                        setUnion = true;
                        break;
                    }
                }

                if (!setUnion)
                    break;

                EA::TDF::TdfGenericReference memberValue;
                outValue.asUnion().getValueByName(classMemberInfo->getMemberName(), memberValue);

                eastl::string luaMemberName = getLuaFormattedName(classMemberInfo->getMemberName());
                LuaValue luaMemberValue = luaThread->GetLuaProperty(luaValue.GetTable(), luaMemberName.c_str());
                decodeMember(luaThread, luaMemberValue, memberValue);
            }
            break;
        }

        case EA::TDF::TDF_ACTUAL_TYPE_TDF:
        {
            if (luaValue.GetType() == LuaValue::LUATYPE_TABLE)
            {
                const EA::TDF::TypeDescriptionClass* classTypeDesc = outValue.asTdf().getTypeDescription().asClassInfo();

                // Just iterate over all the members and read in their data: 
                const EA::TDF::TdfMemberInfo* classMemberInfo = nullptr;
                for (uint32_t index = 0; classTypeDesc->getMemberInfoByIndex(index, classMemberInfo); ++index)
                {
                    EA::TDF::TdfGenericReference memberValue;
                    outValue.asTdf().getValueByName(classMemberInfo->getMemberName(), memberValue);

                    // Make sure the strings are in the expected format:
                    eastl::string luaMemberName = getLuaFormattedName(classMemberInfo->getMemberName());
                    LuaValue luaMemberValue = luaThread->GetLuaProperty(luaValue.GetTable(), luaMemberName.c_str());
                    decodeMember(luaThread, luaMemberValue, memberValue);
                }
            }
            break;
        }

        // Generic Types:
        case EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE:
        {
            // Basic Logic here: 
            // We parse 2 magic values (TdfId + Tdf), and use those to determine the tdf type & value
            if (luaValue.GetType() == LuaValue::LUATYPE_TABLE)
            {
                LuaTable genericTdfTable = luaValue.GetTable();

                LuaValue tdfIdValue = luaThread->GetLuaProperty(genericTdfTable, "TdfId");
                LuaValue tdfTableValue = luaThread->GetLuaProperty(genericTdfTable, "Tdf");

                if (tdfIdValue.GetType() != LuaValue::LUATYPE_NONE && tdfTableValue.GetType() != LuaValue::LUATYPE_NONE)
                {
                    const EA::TDF::TypeDescription* typeDesc = nullptr;
                    if (tdfIdValue.GetType() == LuaValue::LUATYPE_STRING)
                        typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(tdfIdValue.GetString().c_str());
                    else
                        typeDesc = EA::TDF::TdfFactory::get().getTypeDesc((EA::TDF::TdfId)tdfIdValue.GetUInt());

                    if (typeDesc != nullptr)
                    {
                        outValue.asGenericType().setType(*typeDesc);
                        decodeMember(luaThread, tdfTableValue, EA::TDF::TdfGenericReference(outValue.asGenericType().get()));
                    }
                    else
                    {
                        if (tdfIdValue.GetType() == LuaValue::LUATYPE_STRING)
                        {
                            BLAZE_ERR_LOG(Log::SYSTEM, "TDF ID (" << tdfIdValue.GetString() << ") specified for generic TDF member does not appear to be valid, cannot convert to TDF.");
                        }
                        else
                        {
                            BLAZE_ERR_LOG(Log::SYSTEM, "TDF ID (" << tdfIdValue.GetUInt() << ") specified for generic TDF member does not appear to be valid, cannot convert to TDF.");
                        }
                    }
                }
                else
                {
                    BLAZE_ERR_LOG(Log::SYSTEM, "Unable to create variable type because the following values were missing or the wrong type: " <<
                        ((tdfIdValue.GetType() == LuaValue::LUATYPE_NONE) ? "TdfId" : "") << " " <<
                        ((tdfTableValue.GetType() != LuaValue::LUATYPE_TABLE) ? "Tdf" : "") << ".");
                }
            }
            break;
        }

        case EA::TDF::TDF_ACTUAL_TYPE_VARIABLE:
        {
            // Basic Logic here: 
            // We parse 2 magic values (TdfId + Tdf), and use those to determine the tdf type & value
            if (luaValue.GetType() == LuaValue::LUATYPE_TABLE)
            {
                LuaTable variableTdfTable = luaValue.GetTable();
                LuaValue tdfIdValue = luaThread->GetLuaProperty(variableTdfTable, "TdfId");
                LuaValue tdfTableValue = luaThread->GetLuaProperty(variableTdfTable, "Tdf");

                if (tdfIdValue.GetType() != LuaValue::LUATYPE_NONE && tdfTableValue.GetType() == LuaValue::LUATYPE_TABLE)
                {
                    EA::TDF::TdfPtr tdf = nullptr;
                    if (tdfIdValue.GetType() == LuaValue::LUATYPE_STRING)
                        tdf = EA::TDF::TdfFactory::get().create(tdfIdValue.GetString().c_str());
                    else
                        tdf = EA::TDF::TdfFactory::get().create((EA::TDF::TdfId)tdfIdValue.GetUInt());

                    if (tdf != nullptr)
                    {
                        outValue.asVariable().set(tdf);
                        decodeMember(luaThread, tdfTableValue, EA::TDF::TdfGenericReference(*outValue.asVariable().get()));
                    }
                    else
                    {
                        if (tdfIdValue.GetType() == LuaValue::LUATYPE_STRING)
                        {
                            BLAZE_ERR_LOG(Log::SYSTEM, "TDF ID (" << tdfIdValue.GetString() << ") specified for variable TDF member does not appear to be valid, cannot convert to TDF.");
                        }
                        else
                        {
                            BLAZE_ERR_LOG(Log::SYSTEM, "TDF ID (" << tdfIdValue.GetUInt() << ") specified for variable TDF member does not appear to be valid, cannot convert to TDF.");
                        }
                    }
                }
                else
                {
                    BLAZE_ERR_LOG(Log::SYSTEM, "Unable to create variable type because the following values were missing or the wrong type: " <<
                        ((tdfIdValue.GetType() == LuaValue::LUATYPE_NONE) ? "TdfId" : "") << " " <<
                        ((tdfTableValue.GetType() != LuaValue::LUATYPE_TABLE) ? "Tdf" : "") << ".");
                }
            }
            break;
        }
        default:
            BLAZE_ERR_LOG(Log::SYSTEM, "decodeMember:: Unknown Type " << (outValue.getType()) << " provided.  No Lua conversion was handled.");
            break;
        }

        return;
    }

    void convertFromLuaToTdf(LuaThread* luaThread, LuaTable luaRequest, EA::TDF::TdfPtr request)
    {
        LuaValue luaValue;
        lua_rawgeti(luaThread->GetLuaState(), LUA_REGISTRYINDEX, static_cast<int32_t>(luaRequest));
        luaValue = LuaValue::Pop(luaThread->GetLuaState());

        EA::TDF::TdfGenericReference outValue(*request);
        decodeMember(luaThread, luaValue, outValue);
    }

    void encodeMemberToStack(LuaThread* luaThread, EA::TDF::TdfGenericReferenceConst outValue)
    {
        switch (outValue.getType())
        {
            // Integral Types:
        case EA::TDF::TDF_ACTUAL_TYPE_BOOL:         {LuaValue v; v.SetBool(outValue.asBool()); v.Push(luaThread->GetLuaState());                 break;}
        case EA::TDF::TDF_ACTUAL_TYPE_INT8:         {LuaValue v; v.SetInt8(outValue.asInt8()); v.Push(luaThread->GetLuaState());                 break;}
        case EA::TDF::TDF_ACTUAL_TYPE_UINT8:        {LuaValue v; v.SetUInt8(outValue.asUInt8()); v.Push(luaThread->GetLuaState());               break;}
        case EA::TDF::TDF_ACTUAL_TYPE_INT16:        {LuaValue v; v.SetInt16(outValue.asInt16()); v.Push(luaThread->GetLuaState());               break;}
        case EA::TDF::TDF_ACTUAL_TYPE_UINT16:       {LuaValue v; v.SetUInt16(outValue.asUInt16()); v.Push(luaThread->GetLuaState());             break;}
        case EA::TDF::TDF_ACTUAL_TYPE_INT32:        {LuaValue v; v.SetInt(outValue.asInt32()); v.Push(luaThread->GetLuaState());                 break;}
        case EA::TDF::TDF_ACTUAL_TYPE_UINT32:       {LuaValue v; v.SetUInt(outValue.asUInt32()); v.Push(luaThread->GetLuaState());               break;}
        case EA::TDF::TDF_ACTUAL_TYPE_INT64:        {LuaValue v; v.SetInt64(outValue.asInt64()); v.Push(luaThread->GetLuaState());               break;}
        case EA::TDF::TDF_ACTUAL_TYPE_UINT64:       {LuaValue v; v.SetUInt64(outValue.asUInt64()); v.Push(luaThread->GetLuaState());             break;}
        case EA::TDF::TDF_ACTUAL_TYPE_BITFIELD:     {LuaValue v; v.SetUInt(outValue.asBitfield().getBits()); v.Push(luaThread->GetLuaState());   break;}   // No special case for bitfield members as a table
        case EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE:    {LuaValue v; v.SetInt64(outValue.asTimeValue().getMicroSeconds()); v.Push(luaThread->GetLuaState()); break;}  // No special case for time value as a string
        case EA::TDF::TDF_ACTUAL_TYPE_ENUM:
        {
            const char8_t* enumValueName = "";
            outValue.getTypeDescription().asEnumMap()->findByValue(outValue.asEnum(), &enumValueName);
            LuaValue v; v.SetString(enumValueName); v.Push(luaThread->GetLuaState());
            break;
        }

        // Float/String/Blob Types:
        case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:        {LuaValue v; v.SetFloat(outValue.asFloat()); v.Push(luaThread->GetLuaState());               break;}
        case EA::TDF::TDF_ACTUAL_TYPE_STRING:       {LuaValue v; v.SetString(outValue.asString().c_str()); v.Push(luaThread->GetLuaState());     break;}
        case EA::TDF::TDF_ACTUAL_TYPE_BLOB:         {LuaValue v; v.SetBlob(outValue.asBlob().getData(), outValue.asBlob().getCount()); v.Push(luaThread->GetLuaState());           break;}

            // Weird, Vestigial, Tiny hardcoded-class Types:
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_TYPE:
        {
            // Create the table:
            lua_newtable(luaThread->GetLuaState());
            {LuaValue v; v.SetUInt(outValue.asObjectType().component); v.Push(luaThread->GetLuaState()); }
            lua_setfield(luaThread->GetLuaState(), -2, "Component");
            {LuaValue v; v.SetUInt(outValue.asObjectType().type); v.Push(luaThread->GetLuaState()); }
            lua_setfield(luaThread->GetLuaState(), -2, "Type");
            break;
        }
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_ID:
        {
            lua_newtable(luaThread->GetLuaState());
            {LuaValue v; v.SetInt64(outValue.asObjectId().id); v.Push(luaThread->GetLuaState()); }
            lua_setfield(luaThread->GetLuaState(), -2, "Id");

            {
                // Set the object id portion:
                lua_newtable(luaThread->GetLuaState());
                {LuaValue v; v.SetUInt(outValue.asObjectId().type.component); v.Push(luaThread->GetLuaState()); }
                lua_setfield(luaThread->GetLuaState(), -2, "Component");
                {LuaValue v; v.SetUInt(outValue.asObjectId().type.type); v.Push(luaThread->GetLuaState()); }
                lua_setfield(luaThread->GetLuaState(), -2, "Type");
            }
            lua_setfield(luaThread->GetLuaState(), -2, "Type");
            break;
        }


        // Templated Types:
        case EA::TDF::TDF_ACTUAL_TYPE_LIST:
        {
            // For lists, we just need to create the table, then push the elements to the stack and add them all in:
            lua_newtable(luaThread->GetLuaState());
            for (size_t i = 0; i < outValue.asList().vectorSize(); ++i)
            {
                EA::TDF::TdfGenericReferenceConst outListValue;
                if (outValue.asList().getValueByIndex(i, outListValue))
                {
                    encodeMemberToStack(luaThread, outListValue);
                    lua_rawseti(luaThread->GetLuaState(), -2, static_cast<int32_t>(lua_objlen(luaThread->GetLuaState(), -2)) + 1);   // Append to the end of the list
                }
            }
            break;
        }

        case EA::TDF::TDF_ACTUAL_TYPE_MAP:
        {
            // For maps, we need to create the table, encode the key, and encode the elements:
            lua_newtable(luaThread->GetLuaState());
            for (size_t i = 0; i < outValue.asMap().mapSize(); ++i)
            {
                EA::TDF::TdfGenericReferenceConst outMapKey;
                EA::TDF::TdfGenericReferenceConst outMapValue;
                if (outValue.asMap().getValueByIndex(i, outMapKey, outMapValue))
                {
                    encodeMemberToStack(luaThread, outMapKey);
                    encodeMemberToStack(luaThread, outMapValue);
                    lua_settable(luaThread->GetLuaState(), -3);                          // Set Table (-3 on stack) to have key (-2) with value (-1)
                }
            }
            break;
        }

        // Class Types:
        case EA::TDF::TDF_ACTUAL_TYPE_UNION:
        {
            // For unions, encode the member that is used, and that's it.
            lua_newtable(luaThread->GetLuaState());

            uint32_t memberIndex = outValue.asUnion().getActiveMemberIndex();
            if (memberIndex != EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
            {
                const EA::TDF::TdfMemberInfo* memberInfo = nullptr;
                outValue.asUnion().getMemberInfoByIndex(memberIndex, memberInfo);
                eastl::string luaClassMemberName = getLuaFormattedName(memberInfo->getMemberName());

                EA::TDF::TdfGenericReferenceConst memberRef;
                outValue.asUnion().getValueByTag(memberInfo->getTag(), memberRef);
                encodeMemberToStack(luaThread, memberRef);

                lua_setfield(luaThread->GetLuaState(), -2, luaClassMemberName.c_str());  // Encode the member 
            }
            break;
        }

        case EA::TDF::TDF_ACTUAL_TYPE_TDF:
        {
            // For classes, encode all members.
            lua_newtable(luaThread->GetLuaState());

            uint32_t index = 0;
            const EA::TDF::TdfMemberInfo* classMemberInfo = nullptr;
            while (outValue.asTdf().getMemberInfoByIndex(index++, classMemberInfo))
            {
                eastl::string luaClassMemberName = getLuaFormattedName(classMemberInfo->getMemberName());

                EA::TDF::TdfGenericReferenceConst memberRef;
                outValue.asTdf().getValueByTag(classMemberInfo->getTag(), memberRef);
                encodeMemberToStack(luaThread, memberRef);

                lua_setfield(luaThread->GetLuaState(), -2, luaClassMemberName.c_str());  // Encode the member 
            }
            break;
        }

        // Generic Types:
        case EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE:
        {
            // For Generic Tdfs, handle it like the Object Ids -
            lua_newtable(luaThread->GetLuaState());

            if (outValue.asGenericType().isValid())
            {
                // Set the id:
                EA::TDF::TdfId tdfId = outValue.asGenericType().get().getTdfId();
                LuaValue v; v.SetUInt(tdfId); v.Push(luaThread->GetLuaState());
                lua_setfield(luaThread->GetLuaState(), -2, "TdfId");

                // Encode the value:
                EA::TDF::TdfGenericReferenceConst variableValueRef(outValue.asGenericType().get());
                encodeMemberToStack(luaThread, variableValueRef);
                lua_setfield(luaThread->GetLuaState(), -2, "Tdf");
            }
            break;

        }

        case EA::TDF::TDF_ACTUAL_TYPE_VARIABLE:
        {
            // For Variable Tdfs, handle it like the Object Ids -
            lua_newtable(luaThread->GetLuaState());

            if (outValue.asVariable().isValid())
            {
                // Set the id:
                EA::TDF::TdfId tdfId = outValue.asVariable().get()->getTdfId();
                LuaValue v; v.SetUInt(tdfId); v.Push(luaThread->GetLuaState());
                lua_setfield(luaThread->GetLuaState(), -2, "TdfId");

                // Encode the value:
                EA::TDF::TdfGenericReferenceConst variableValueRef(*outValue.asVariable().get());
                encodeMemberToStack(luaThread, variableValueRef);
                lua_setfield(luaThread->GetLuaState(), -2, "Tdf");
            }
            break;
        }
        default:
            BLAZE_ERR_LOG(Log::SYSTEM, "encodeMemberToStack:: Unknown Type " << (outValue.getType()) << " provided.  No Lua conversion was handled.");
            break;
        }
    }

    LuaTable convertFromTdfToLua(LuaThread* luaThread, EA::TDF::TdfPtr response)
    {
        // Create the Table on the Stack:
        EA::TDF::TdfGenericReferenceConst responseRef(response);
        encodeMemberToStack(luaThread, responseRef);

        // Pop/return the table from the stack:
        return (LuaTable)luaL_ref(luaThread->GetLuaState(), LUA_REGISTRYINDEX);
    }

    int32_t luaBlazeRpcFunction(lua_State *lua)
    {
        eastl::string name = lua_tostring(lua, lua_upvalueindex(1)); // Lua is 1-indexed
        ComponentId componentId = (ComponentId)lua_tointeger(lua, lua_upvalueindex(2));
        CommandId commandId = (CommandId)lua_tointeger(lua, lua_upvalueindex(3));

        const ComponentData* componentData = ComponentData::getComponentData(componentId);
        const CommandInfo* commandInfo = componentData->getCommandInfo(commandId);

        EA::TDF::TdfPtr request = commandInfo->createRequest("Lua Rpc Request");
        EA::TDF::TdfPtr response = commandInfo->createResponse("Lua Rpc Response");
        EA::TDF::TdfPtr errResponse = commandInfo->createErrorResponse("Lua Rpc Err Response");

        LuaThread *thread = LuaThread::GetInstanceArgument(lua);
        BlazeRpcError err = ERR_OK;
        LuaStressInstance *instance = static_cast<LuaStressInstance *>(thread->GetContainer());

        LuaTable luaResponse = 0;
        if (instance)
        {
            if (request != nullptr)
            {
                if (lua_gettop(lua) >= 1)
                {
                    lua_pushvalue(lua, -1);
                    LuaTable luaRequest = (LuaTable)luaL_ref(lua, LUA_REGISTRYINDEX);
                    convertFromLuaToTdf(thread, luaRequest, request);
                    luaL_unref(lua, LUA_REGISTRYINDEX, static_cast<int32_t>(luaRequest));
                }
            }

            TimeValue start(TimeValue::getTimeOfDay());

            Component* componentProxy = ((Component*)instance->getProxy(componentId));
            err = componentProxy->sendRequest(*commandInfo, request, response, errResponse, RpcCallOptions());
            Lua::LuaStats::GetInstance()->ReportError(name, err);
            Lua::LuaStats::GetInstance()->ReportTime(name, start);

            if (response != nullptr)
            {
                luaResponse = convertFromTdfToLua(thread, response);
            }
        }
        else
        {
            BLAZE_ERR(Log::SYSTEM, "LuaThread was null in RPC command [%s]", name.c_str());
            err = ERR_SYSTEM;
        }

        printf("%s\n", name.c_str());

        // push the error  (Passed as a double, since the underlying type is uint, and we don't want negative values)
        lua_pushnumber(lua, static_cast<double>(err));

        if (response != nullptr)
        {
            // then the response structure (table in Lua)
            lua_rawgeti(lua, LUA_REGISTRYINDEX, static_cast<int32_t>(luaResponse));
            luaL_unref(lua, LUA_REGISTRYINDEX, static_cast<int32_t>(luaResponse));
        }
        else
        {
            lua_pushnil(lua);
        }

        return Lua::LuaStats::GetInstance()->GetYieldAll() ? lua_yield(lua, 2) : 2;
    }

    void registerLuaToRpcFunctions(lua_State *lua)
    {
        // Add all of the RPCs mapped to components:
        size_t numComps = ComponentData::getComponentCount();
        for (size_t i = 0; i < numComps; ++i)
        {
            const ComponentData* compData = ComponentData::getComponentDataAtIndex(i);
            if (Component::isMasterComponentId(compData->id))
                continue;

            ComponentId componentId = compData->id;
            eastl::string componentNameCapitalized = compData->loggingName;        // remove the (0x0000) at the end:
            componentNameCapitalized.erase(componentNameCapitalized.end() - 8, componentNameCapitalized.end());
            for (auto& command : compData->commands)
            {
                CommandId commandId = command.first;
                const CommandInfo* commandInfo = command.second;
                const char8_t* commandName = commandInfo->name;

                // Only One function name form:  
                //   Achievement_expressLogin
                eastl::string commandNameCapitalized(commandName);
                commandNameCapitalized[0] = EA::StdC::Toupper(commandNameCapitalized[0]);

                eastl::string combinedName;
                combinedName.sprintf("%s_%s", componentNameCapitalized.c_str(), commandName);

                lua_pushstring(lua, combinedName.c_str());        // Component+Command Name
                lua_pushinteger(lua, componentId);                // Component Index  (Okay to pass as int, no sign extension from uint16)
                lua_pushinteger(lua, commandId);                  // Command Id       (Okay to pass as int, no sign extension from uint16)
                lua_pushcclosure(lua, luaBlazeRpcFunction, 3);    // Lua -> C Function
                lua_setglobal(lua, combinedName.c_str());         // Set the Global to this
            }
        }
    }


/******************************************************************************/
/*! LuaInterpreter :: AppendTimeToFilename

    \brief  Appends date-time at end of filename before extension

    \param  fileName - [Put the description of the parameter 'fileName' here]
    \return          - [Describe the 'char8_t *' return result here.]

    Note : Caller needs pass enough big buffer to fit date-time string appended to filename.
*/
/******************************************************************************/
char8_t *LuaInterpreter::AppendTimeToFilename(char8_t * fileName)
{
    char8_t tempFileName[MAX_FILENAME_SIZE];
    char8_t timeBuffer[128];
    char8_t fileExt[16];

    blaze_strnzcpy(tempFileName, fileName, sizeof(tempFileName));
    char8_t* fileExtPtr = strrchr(tempFileName, '.');
    if (fileExtPtr != nullptr)
    {
        // save of the file extension
        blaze_strnzcpy(fileExt, fileExtPtr, sizeof(fileExt));
    }

    TimeValue startTime = TimeValue::getTimeOfDay();

    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
    uint32_t millis;

    TimeValue::getLocalTimeComponents(startTime, &year, &month, &day, &hour, &minute, &second, &millis);
    blaze_snzprintf(timeBuffer, sizeof(timeBuffer), "-%d%02d%02d_%02d%02d%02d", year, month, day, hour, minute, second);

    if (fileExtPtr != nullptr)
    {
        // append the timebuffer just before file-ext starts,
        blaze_strnzcpy(fileExtPtr, timeBuffer, sizeof(timeBuffer));
        // append the original file extension back to the buffer
        blaze_strnzcat(tempFileName, fileExt, sizeof(tempFileName)-1);
    }
    else
    {
        // otherwise just append date-time string
        blaze_strnzcat(tempFileName, timeBuffer, sizeof(timeBuffer)-1);
    }

    // copy new filename back to in-parameter
    blaze_strnzcpy(fileName, tempFileName, sizeof(tempFileName)-1);
    return fileName;
}

LuaInterpreter::LuaInterpreter(const char8_t *rpcCallStatsFile, const char8_t *rpcErrorStatsFile) : mState(0)
{   
    Lua::LuaStats::GetInstance()->SetStartTime(TimeValue::getTimeOfDay());

    char8_t filename[MAX_FILENAME_SIZE];
    blaze_strnzcpy(filename, rpcCallStatsFile, sizeof(filename));
    AppendTimeToFilename(filename);
    Lua::LuaStats::GetInstance()->SetStatsFilename(filename);
    blaze_strnzcpy(filename, rpcErrorStatsFile, sizeof(filename));
    AppendTimeToFilename(filename);
    Lua::LuaStats::GetInstance()->SetErrorsFilename(filename);
}

//! GetState() returns the underlying lua_State instance.
lua_State *LuaInterpreter::GetState()
{
    return mState;
}

//! CreateThread() creates a new lua 'fiber' and initializes it.
LuaThread *LuaInterpreter::CreateThread(void *userContainer)
{
    LuaThread *thread = nullptr;
    if (mState != nullptr)
    {
        thread = BLAZE_NEW LuaThread(*this);
        thread->SetContainer(userContainer);
    }
    return thread;
}

namespace Lua
{
    //! Called from Lua as 'setYieldAll(b)' with a value b true or false.  Sets whether every RPC explicitly yields after being called.
    static int32_t SetYieldAll(lua_State *lua)
    {
        // the second needs to be our argument
        luaL_checktype(lua, -1, LUA_TBOOLEAN);
        LuaStats::GetInstance()->SetYieldAll(lua_toboolean(lua, -1) != 0);
        return 0;
    }

    //! Called from Lua as 'setDelay(instance, n)' with a delay n in milliseconds.  Sets the default delay between each RPC call.
    static int32_t SetDelay(lua_State *lua)
    {
        // first parameter needs to be our lua stress instance
        luaL_checktype(lua, -2, LUA_TTABLE);
        // the second needs to be our time to delay in ms
        luaL_checktype(lua, -1, LUA_TNUMBER);

        LuaThread *thread = LuaThread::GetInstanceArgument(lua);
        LuaStressInstance *instance = static_cast<LuaStressInstance *>(thread->GetContainer());

        if (instance)
        {
            int32_t delay = static_cast<int32_t>(lua_tonumber(lua, -1));
            instance->SetDelay(delay);
        }

        return 0;
    }

    //! Called from Lua as 'delay(instance, n)' with a delay n in milliseconds.  Explicitly delays for n milliseconds.
    static int32_t Delay(lua_State *lua)
    {
        // first parameter needs to be our lua stress instance
        luaL_checktype(lua, -2, LUA_TTABLE);
        // the second needs to be our time to delay in ms
        luaL_checktype(lua, -1, LUA_TNUMBER);

        LuaThread *thread = LuaThread::GetInstanceArgument(lua);
        LuaStressInstance *instance = static_cast<LuaStressInstance *>(thread->GetContainer());

        if (instance)
        {
            int32_t delay = static_cast<int32_t>(lua_tonumber(lua, -1));
            instance->Delay(delay);
        }

        return 0;
    }

    //! Called from Lua as 'delayWithPing(instance, n)' with a delay n in milliseconds.  Explicitly delays for n milliseconds.
    static int32_t DelayWithPing(lua_State *lua)
    {
        // first parameter needs to be our lua stress instance
        luaL_checktype(lua, -2, LUA_TTABLE);
        // the second needs to be our time to delay in ms
        luaL_checktype(lua, -1, LUA_TNUMBER);

        LuaThread *thread = LuaThread::GetInstanceArgument(lua);
        LuaStressInstance *instance = static_cast<LuaStressInstance *>(thread->GetContainer());

        if (instance)
        {
            int32_t delay = static_cast<int32_t>(lua_tonumber(lua, -1));
            instance->sleep(delay);
        }

        return 0;
    }

    // ! Called from Lua as 'getBlazeId(instance)'.  Returns an integer with the blaze id.
    static int32_t GetBlazeId(lua_State *lua)
    {
        LuaThread *thread = LuaThread::GetInstanceArgument(lua);
        LuaStressInstance *instance = static_cast<LuaStressInstance *>(thread->GetContainer());

        int64_t value = -1;
        if (instance)
        {
            Login *login = instance->getLogin();
            const Authentication::UserLoginInfo *userLoginInfo = login->getUserLoginInfo();
            value = userLoginInfo->getBlazeId();
        }
        lua_pushinteger(lua, value);
        return 1;
    }

    // ! Called from Lua as 'getRpcTime()'.  Returns an integer with the call time in microseconds.
    static int32_t GetLatestRpcTime(lua_State *lua)
    {
        // first parameter needs to be our lua stress instance
        luaL_checktype(lua, -2, LUA_TTABLE);
        // the second needs to be our rpc name
        luaL_checktype(lua, -1, LUA_TSTRING);

        LuaThread *thread = LuaThread::GetInstanceArgument(lua);
        LuaStressInstance *instance = static_cast<LuaStressInstance *>(thread->GetContainer());

        int value = -1;
        if (instance)
        {
            value = static_cast<int>(LuaStats::GetInstance()->GetLatestRpcTime(lua_tostring(lua, -1)).getMicroSeconds());
        }
        lua_pushinteger(lua, value);    // Passed as a int, since we want -1 to stay negative.
        return 1;
    }

    // ! Called from Lua as 'errorName(n)' with a BlazeRpcError n expressed as a number.  Returns a string describing the error.
    static int32_t ErrorName(lua_State *lua)
    {
        luaL_checktype(lua, -1, LUA_TNUMBER);
        lua_pushstring(lua,
            ErrorHelp::getErrorName(
                static_cast<BlazeRpcError>(lua_tointeger(lua, -1))));
        return 1;
    }

    // ! Called from Lua as 'getPersona(instance)'. Returns a string with the persona. 
    static int32_t GetPersona(lua_State *lua)
    {
        LuaThread *thread = LuaThread::GetInstanceArgument(lua);
        LuaStressInstance *instance = static_cast<LuaStressInstance *>(thread->GetContainer());

        eastl::string value;
        if (instance)
        {
            Login *login = instance->getLogin();
            const Authentication::UserLoginInfo *userLoginInfo = login->getUserLoginInfo();
            value = userLoginInfo->getPersonaDetails().getDisplayName();
        }
        lua_pushstring(lua, value.c_str());
        return 1;
    }

    // ! Called from Lua as 'getEmail(instance)'. Returns a string with the email associated with the session. 
    static int32_t GetEmail(lua_State *lua)
    {
        LuaThread *thread = LuaThread::GetInstanceArgument(lua);
        LuaStressInstance *instance = static_cast<LuaStressInstance *>(thread->GetContainer());

        eastl::string value;
        if (instance)
        {
            Login *login = instance->getLogin();
            const Authentication::UserLoginInfo *userLoginInfo = login->getUserLoginInfo();
            value = userLoginInfo->getEmail();
        }
        lua_pushstring(lua, value.c_str());
        return 1;
    }

} //namespace Lua


//! RunFile() initializes, runs the lua script and sets LuaStats::SetLuaFile(..)
bool LuaInterpreter::RunFile(const char8_t *filename)
{
    if (filename == nullptr)
    {
        return false;
    }

    Lua::LuaStats::GetInstance()->SetLuaFile(filename);

    mState = luaL_newstate(); 

    // load the libs  
    luaL_openlibs(mState);  

    lua_register(mState, "delay", Lua::Delay);
    lua_register(mState, "delayWithPing", Lua::DelayWithPing);
    lua_register(mState, "setDelay", Lua::SetDelay);
    lua_register(mState, "setYieldAll", Lua::SetYieldAll);
    lua_register(mState, "getBlazeId", Lua::GetBlazeId);
    lua_register(mState, "getPersona", Lua::GetPersona);
    lua_register(mState, "getEmail", Lua::GetEmail);
    lua_register(mState, "getRpcTime", Lua::GetLatestRpcTime);
    lua_register(mState, "errorName", Lua::ErrorName);

    // Blaze::Stress::registerLuaFunctions(mState);
    registerLuaToRpcFunctions(mState);

    // Sets up symbols in global table for each TDF that has a tdf id.
    // (Could expand this to handle enum values if we want)
    // For example: BLAZE_DUMMY_DUMMYENTRY_TDFID
    const auto& tdfRegistry = EA::TDF::TdfFactory::get().getRegistry();
    for (const auto& registryEntry : tdfRegistry)
    {
        const EA::TDF::TypeDescription& typeDesc = static_cast<const EA::TDF::TypeRegistrationNode&>(registryEntry).info;
        if (typeDesc.getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_TDF || typeDesc.getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_UNION)
        {
            eastl::string convertedName;
            for (const char8_t* curChar = typeDesc.getFullName(); *curChar != '\0'; ++curChar)
            {
                if (*curChar == ':')
                {
                    convertedName.push_back('_');
                    ++curChar; // skip the next ':'
                    if (*curChar == '\0')
                        break;
                }
                else
                {
                    convertedName.push_back(EA::StdC::Toupper(*curChar));
                }
            }
            convertedName.append("_TDFID");

            BLAZE_TRACE(Log::SYSTEM, "LuaVariableTdfHandler registering TDF ID %s = %d",
                convertedName.c_str(), typeDesc.getTdfId());
            lua_pushinteger(mState, typeDesc.getTdfId());    
            lua_setglobal(mState, convertedName.c_str());
        }
    }

    if (luaL_dofile(mState, filename) != 0)
    {
        printf("Error: %s\n",lua_tostring(mState, -1));
        lua_pop(mState, lua_gettop(mState));
        //what should I do if the script is invalid?
        //how do I terminate the stress test tool?
        ASSERT(false);
        return false;
    }
    return true;
}

LuaInterpreter::~LuaInterpreter()
{
    if (mState != nullptr)
    {
        // Verify the stack
        ASSERT(lua_gettop(mState) == 0);
        lua_close(mState);  
    }
}

} // Stress
} // Blaze
