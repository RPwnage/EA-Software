/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_SCRIPT_REGISTERY_H
#define BLAZE_REDIS_SCRIPT_REGISTERY_H

/*** Include files *******************************************************************************/

#include "EASTL/intrusive_list.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class RedisConn;

typedef int64_t RedisScriptId;

struct RedisScript : public eastl::intrusive_list_node
{
public:
    static const RedisScriptId INVALID_SCRIPT_ID = 0;

public:
    RedisScript(int32_t numberOfKeys, const char8_t *scriptTextOrFilepath, bool isFilepath = false);
    RedisScriptId getId() const { return mScriptId; }
    int32_t getNumberOfKeys() const { return mNumberOfKeys; }
    const char8_t *getScript() const { return mScript.c_str(); }

private:
    friend class RedisScriptRegistry;
    RedisScriptId mScriptId = INVALID_SCRIPT_ID;
    int32_t mNumberOfKeys = 0;
    eastl::string mScriptFilepath;
    eastl::string mScript;
    eastl::string mScriptLoadError;
};



class RedisScriptRegistry
{
public:
    static RedisError loadRegisteredScripts(RedisConn *conn);

private:
    friend struct RedisScript;
    void registerScript(RedisScript &script);
    static RedisScriptRegistry& getRegistrySingleton();

    typedef eastl::intrusive_list<RedisScript> RedisScriptList;
    RedisScriptList mScriptList;
    RedisScriptId mIdCounter = 0;

};

} // Blaze

#endif // BLAZE_REDIS_SCRIPT_REGISTERY_H
