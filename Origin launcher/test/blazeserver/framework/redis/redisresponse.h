/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_RESPONSE_H
#define BLAZE_REDIS_RESPONSE_H

/*** Include files *******************************************************************************/

#include "framework/protocol/shared/heat2decoder.h"
#include "blazerpcerrors.h"
#include "EATDF/tdf.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "framework/redis/redisdecoder.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

struct NotificationInfo;
class StringBuilder;
class RedisCluster;
class RedisConn;
class RedisResponse;
typedef eastl::intrusive_ptr<RedisResponse> RedisResponsePtr;
typedef eastl::vector<RedisResponsePtr> RedisResponsePtrList;

class RedisResponse
{
public:
    RedisResponse();
    RedisResponse(const RedisResponse &obj);
    virtual ~RedisResponse();

    enum RedisResponseType
    {
        TYPE_NONE = 0,
        TYPE_ERROR = REDIS_REPLY_ERROR,
        TYPE_STATUS = REDIS_REPLY_STATUS,
        TYPE_INTEGER = REDIS_REPLY_INTEGER,
        TYPE_STRING = REDIS_REPLY_STRING,
        TYPE_ARRAY = REDIS_REPLY_ARRAY,
        TYPE_EMPTY = REDIS_REPLY_NIL
    };

    RedisResponseType getType() const { return mReply ? (RedisResponseType)mReply->type : TYPE_NONE; }
    bool isError() const { return (getType() == TYPE_ERROR); }
    bool isStatus() const { return (getType() == TYPE_STATUS); }
    bool isInteger() const { return (getType() == TYPE_INTEGER); }
    bool isString() const { return (getType() == TYPE_STRING); }
    bool isArray() const { return (getType() == TYPE_ARRAY); }
    bool isEmpty() const { return (getType() == TYPE_EMPTY); }

    const char8_t *getString() const { return (mReply ? mReply->str : ""); }
    int32_t getStringLen() const { return (mReply ? mReply->len : 0); }

    int64_t getInteger() const { return (mReply ? mReply->integer : 0); }

    uint32_t getArraySize() const { return (mReply ? (uint32_t)mReply->elements : 0); }
    template<typename T>
    bool extractArrayElement(uint32_t index, T& element) const;
    RedisResponse getArrayElement(uint32_t index) const;
    uint64_t getResponseLength() const;

    BlazeRpcError getValue(eastl::string &value) const;

    BlazeRpcError getValue(int8_t &value) const;
    BlazeRpcError getValue(int16_t &value) const;
    BlazeRpcError getValue(int32_t &value) const;
    BlazeRpcError getValue(int64_t &value) const;
    BlazeRpcError getValue(uint8_t &value) const;
    BlazeRpcError getValue(uint16_t &value) const;
    BlazeRpcError getValue(uint32_t &value) const;
    BlazeRpcError getValue(uint64_t &value) const;
    BlazeRpcError getValue(RawBuffer &value) const;

    BlazeRpcError getValue(EA::TDF::Tdf &value) const;

    BlazeRpcError extractInto(EA::TDF::Tdf &tdf) const;

    BlazeRpcError extractTdf(EA::TDF::Tdf *&tdf, TdfDecoder &decoder, MemoryGroupId memGroupId = DEFAULT_BLAZE_MEMGROUP);
    BlazeRpcError extractNotification(const NotificationInfo *&info, EA::TDF::Tdf *&payload, TdfDecoder &decoder, MemoryGroupId memGroupId = DEFAULT_BLAZE_MEMGROUP, const char8_t *memGroupName = BLAZE_DEFAULT_ALLOC_NAME);

    RedisError waitForReply(EA::TDF::TimeValue absoluteTimeout = EA::TDF::TimeValue());

    inline void AddRef()
    {
        mRefCount++;
    }

    inline void Release()
    {
        if (--mRefCount == 0)
            delete this;
    }
    void encode(eastl::string& dest) const;

private:
    friend class RedisCluster;
    friend class RedisConn;

    RedisResponse(redisReply *reply, bool owned);

    static RedisError hiredisErrorToRedisError(int hiredisErr);

    uint64_t getSizeOfRedisReply(redisReply *reply) const;

    void setRedisReplyInfo(const redisAsyncContext *context, redisReply *reply, bool owned);
    void signalCompletion(BlazeRpcError err);

private:
    int32_t mRefCount;

    int mReplyErr;
    eastl::string mReplyErrStr;
    redisReply* mReply;
    bool mOwned;

    bool mCompleted;
    Fiber::EventHandle mCompletionEvent;
};

template<typename T>
bool RedisResponse::extractArrayElement(uint32_t index, T& value) const
{
    bool result = false;
    struct redisReply * reply = mReply->element[index];
    if (reply->type == TYPE_STRING)
    {
        const char8_t* begin = reply->str;
        const char8_t* end = begin + reply->len;
        RedisDecoder rdecoder;
        result = (rdecoder.decodeValue(begin, end, value) != nullptr);
    }
    return result;
}

} // Blaze

#endif // BLAZE_REDIS_RESPONSE_H
