/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_ERRORS_H
#define BLAZE_REDIS_ERRORS_H

/*** Include files *******************************************************************************/

#include "blazerpcerrors.h"
#include "framework/util/shared/stringbuilder.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define REDIS_ERROR(error, description) ((gRedisManager->setLastError(error) << description).get() ? error : REDIS_ERR_SYSTEM)

namespace Blaze
{

enum RedisErrorVal
{
    REDIS_ERR_OK,
    REDIS_ERR_SYSTEM,
    REDIS_ERR_NOT_FOUND,
    REDIS_ERR_INVALID_CONFIG,
    REDIS_ERR_TIMEOUT,
    REDIS_ERR_NOT_CONNECTED,
    REDIS_ERR_ALREADY_CONNECTED,
    REDIS_ERR_DNS_LOOKUP_FAILED,
    REDIS_ERR_CONNECT_FAILED,
    REDIS_ERR_SEND_COMMAND_FAILED,
    REDIS_ERR_SERVER_AUTH_FAILED,
    REDIS_ERR_UNEXPECTED_TYPE,
    REDIS_ERR_COMMAND_FAILED,
    REDIS_ERR_OBJECT_NOT_FOUND,
    REDIS_ERR_NO_EXPIRY,
    REDIS_ERR_MOVED,
    REDIS_ERR_HIREDIS_IO,
    REDIS_ERR_HIREDIS_EOF,
    REDIS_ERR_HIREDIS_PROTOCOL,
    REDIS_ERR_HIREDIS_OOM,
    REDIS_ERR_ASK,
    REDIS_ERR_CROSSSLOT,
    REDIS_ERR_LOADING,
    REDIS_ERR_TRYAGAIN,
    REDIS_ERR_CLUSTERDOWN,
    REDIS_ERR_NO_MORE_RETRIES
};

struct RedisError
{
    RedisError()
    {
        error = REDIS_ERR_OK;
    }

    RedisError(const RedisError& redisError)
    {
        error = redisError.error;
    }

    RedisError(const RedisErrorVal &redisError)
    {
        error = redisError;
    }

    //template<typename T>
    //inline bool operator==(T)
    //{
    //    EA_COMPILETIME_ASSERT(false);
    //}

    //template<>
    inline bool operator==(const RedisError &redisError)
    {
        return error == redisError.error;
    }

    //template<>
    inline bool operator==(const RedisErrorVal &redisError)
    {
        return error == redisError;
    }

    //template<typename T>
    //inline bool operator!=(T)
    //{
    //    EA_COMPILETIME_ASSERT(false);
    //}

    //template<>
    inline bool operator!=(const RedisError &redisError)
    {
        return error != redisError.error;
    }

    //template<>
    inline bool operator!=(const RedisErrorVal &redisError)
    {
        return error != redisError;
    }

    RedisErrorVal error;
};

class RedisErrorHelper
{
public:
    RedisError getLastError() const { return mLastError; }
    const char8_t *getLastErrorDescription() const { return mLastErrorDescription.get(); }

    StringBuilder &setLastError(RedisError lastError)
    {
        mLastError = lastError;
        return mLastErrorDescription.reset();
    }

    static RedisError convertError(BlazeRpcError blazeError)
    {
        switch (blazeError)
        {
        case ERR_OK:        return REDIS_ERR_OK;
        case ERR_SYSTEM:    return REDIS_ERR_SYSTEM;
        case ERR_TIMEOUT:   return REDIS_ERR_TIMEOUT;
        default:            return REDIS_ERR_SYSTEM;
        }
    }

    static const char8_t* getName(RedisError err)
    {
        switch (err.error)
        {
        case REDIS_ERR_OK: return "REDIS_ERR_OK";
        case REDIS_ERR_SYSTEM: return "REDIS_ERR_SYSTEM";
        case REDIS_ERR_NOT_FOUND: return "REDIS_ERR_NOT_FOUND";
        case REDIS_ERR_INVALID_CONFIG: return "REDIS_ERR_INVALID_CONFIG";
        case REDIS_ERR_TIMEOUT: return "REDIS_ERR_TIMEOUT";
        case REDIS_ERR_NOT_CONNECTED: return "REDIS_ERR_NOT_CONNECTED";
        case REDIS_ERR_ALREADY_CONNECTED: return "REDIS_ERR_ALREADY_CONNECTED";
        case REDIS_ERR_DNS_LOOKUP_FAILED: return "REDIS_ERR_DNS_LOOKUP_FAILED";
        case REDIS_ERR_CONNECT_FAILED: return "REDIS_ERR_CONNECT_FAILED";
        case REDIS_ERR_SEND_COMMAND_FAILED: return "REDIS_ERR_SEND_COMMAND_FAILED";
        case REDIS_ERR_SERVER_AUTH_FAILED: return "REDIS_ERR_SERVER_AUTH_FAILED";
        case REDIS_ERR_UNEXPECTED_TYPE: return "REDIS_ERR_UNEXPECTED_TYPE";
        case REDIS_ERR_COMMAND_FAILED: return "REDIS_ERR_COMMAND_FAILED";
        case REDIS_ERR_OBJECT_NOT_FOUND: return "REDIS_ERR_OBJECT_NOT_FOUND";
        case REDIS_ERR_NO_EXPIRY: return "REDIS_ERR_NO_EXPIRY";
        case REDIS_ERR_MOVED: return "REDIS_ERR_MOVED";
        case REDIS_ERR_HIREDIS_IO: return "REDIS_ERR_HIREDIS_IO";
        case REDIS_ERR_HIREDIS_EOF: return "REDIS_ERR_HIREDIS_EOF";
        case REDIS_ERR_HIREDIS_PROTOCOL: return "REDIS_ERR_HIREDIS_PROTOCOL";
        case REDIS_ERR_HIREDIS_OOM: return "REDIS_ERR_HIREDIS_OOM";
        case REDIS_ERR_ASK: return "REDIS_ERR_ASK";
        case REDIS_ERR_CROSSSLOT: return "REDIS_ERR_CROSSSLOT";
        case REDIS_ERR_LOADING: return "REDIS_ERR_LOADING";
        case REDIS_ERR_TRYAGAIN: return "REDIS_ERR_TRYAGAIN";
        case REDIS_ERR_CLUSTERDOWN: return "REDIS_ERR_CLUSTERDOWN";
        case REDIS_ERR_NO_MORE_RETRIES: return "REDIS_ERR_NO_MORE_RETRIES";
        };

        return "unknown";
    }

private:
    friend class RedisManager;
    RedisErrorHelper() : mLastError(REDIS_ERR_OK) {};

private:
    RedisError mLastError;
    StringBuilder mLastErrorDescription;
};

} // Blaze

#endif // BLAZE_REDIS_ERRORS_H
