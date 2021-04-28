/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/redis/redis.h"
#include "framework/redis/redisscriptregistry.h"
#include "EAAssert/eaassert.h"
#include "framework/redis/redismanager.h"
#include "EAIO/EAFileUtil.h"
#include <stdio.h>

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

RedisScript::RedisScript(int32_t numberOfKeys, const char8_t *scriptTextOrFilepath, bool isFilepath)
    : mNumberOfKeys(numberOfKeys)
{
    if (isFilepath)
    {
        mScriptFilepath = scriptTextOrFilepath;        
        if (EA::IO::File::Exists(scriptTextOrFilepath))
        {
            FILE* f = fopen(mScriptFilepath.c_str(), "r");
            if (f != nullptr)
            {
                size_t readcount = 0;
                size_t totalread = 0;
                eastl::string buffer;
                buffer.reserve(1000);
                while ((readcount = fread(buffer.begin() + buffer.size(), 1, buffer.capacity() - buffer.size(), f)) > 0)
                {
                    totalread += readcount;
                    buffer.force_size(buffer.size() + readcount);
                    buffer[totalread] = '\0'; // terminate the string provisionally
                    if (buffer.capacity() - buffer.size() < 100)
                        buffer.reserve(buffer.capacity() * 2);
                }
                mScript.swap(buffer);
                fclose(f);
            }
            else
            {
                mScriptLoadError.sprintf("failed to open script file for reading from path(%s)", mScriptFilepath.c_str());
            }
        }
        else
        {
            mScriptLoadError.sprintf("script file not found in path(%s)", mScriptFilepath.c_str());
        }
    }
    else
        mScript = scriptTextOrFilepath;
    
    RedisScriptRegistry::getRegistrySingleton().registerScript(*this);
}

void RedisScriptRegistry::registerScript(RedisScript &script)
{
    EA_ASSERT_MSG(script.mScriptId == RedisScript::INVALID_SCRIPT_ID, "The RedisScriptId for this script already has a value, which means it is already registered, or someone has misused the API.");
    EA_ASSERT_MSG(gRedisManager == nullptr, "The RedisScriptRegistry does not currently support registering scripts at runtime, only during static initialization at program startup.");

    script.mScriptId = ++mIdCounter;
    mScriptList.push_back(script);
}

Blaze::RedisScriptRegistry& RedisScriptRegistry::getRegistrySingleton()
{
    static RedisScriptRegistry registry;
    return registry;
}

RedisError RedisScriptRegistry::loadRegisteredScripts(RedisConn *conn)
{
    RedisError err = REDIS_ERR_OK;

    auto& registrySingleton = getRegistrySingleton();
    for (auto& redisScript : registrySingleton.mScriptList)
    {
        if (!redisScript.mScriptLoadError.empty())
        {
            BLAZE_ERR_LOG(Log::REDIS, "[RedisScriptRegistry].loadRegisteredScripts: Error " << redisScript.mScriptLoadError << ".");
            err = REDIS_ERR_SYSTEM;
            break;
        }
        err = conn->loadScript(redisScript);
        if (err != REDIS_ERR_OK)
            break;
    }

    return err;
}

/*** Private Methods *****************************************************************************/

} // Blaze
