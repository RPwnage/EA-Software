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
#include "framework/util/shared/stringbuilder.h"
#include "framework/redis/redisresponse.h"
#include "framework/redis/redismanager.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

RedisResponse::RedisResponse()
    : mRefCount(0),
      mReplyErr(REDIS_OK),
      mReply(nullptr),
      mOwned(false),
      mCompleted(false)
{
}

RedisResponse::RedisResponse(const RedisResponse &obj)
    : mRefCount(0),
      mReplyErr(obj.mReplyErr),
      mReply(obj.mReply),
      mOwned(false),
      mCompleted(false)
{
}

RedisResponse::RedisResponse(redisReply *reply, bool owned)
    : mRefCount(0),
      mReplyErr(REDIS_OK),
      mReply(reply),
      mOwned(owned),
      mCompleted(false)
{
}

RedisResponse::~RedisResponse()
{
    if (mReply != nullptr)
    {
        if (mOwned)
            freeReplyObject(mReply);
        mReply = nullptr;
    }
}

void RedisResponse::setRedisReplyInfo(const redisAsyncContext *context, redisReply *reply, bool owned)
{
    mReplyErr = context->err;
    mReplyErrStr = context->errstr;
    mReply = reply;
    mOwned = owned;
}

RedisResponse RedisResponse::getArrayElement(uint32_t index) const
{
    EA_ASSERT(index < mReply->elements);
    return RedisResponse(mReply->element[index], false);
}

uint64_t RedisResponse::getResponseLength() const
{
    return getSizeOfRedisReply(mReply);
}

uint64_t RedisResponse::getSizeOfRedisReply(redisReply *reply) const
{
    uint64_t totalLength = 0;
    if (reply != nullptr)
    {
        switch (reply->type)
        {
        case TYPE_NONE:
        case TYPE_ERROR:
        case TYPE_STRING:
        case TYPE_STATUS:
            totalLength += reply->len;
            break;
        case TYPE_INTEGER:
            totalLength += sizeof(reply->integer);
            break;
        case TYPE_ARRAY:
            for (size_t a = 0; a < reply->elements; ++a)
                totalLength += getSizeOfRedisReply(reply->element[a]);
            break;
        }
    }
    return totalLength;
}

BlazeRpcError RedisResponse::extractInto(EA::TDF::Tdf &tdf) const
{
    BlazeRpcError rc = ERR_SYSTEM;

    if (getType() == TYPE_STRING)
    {
        RawBuffer rawBuffer((uint8_t*)mReply->str, mReply->len, false);
        rawBuffer.put(mReply->len);

        EA::TDF::TdfId tdfId;
        if (rawBuffer.datasize() >= sizeof(tdfId))
        {
            memcpy(&tdfId, rawBuffer.data(), sizeof(tdfId));
            rawBuffer.pull(sizeof(tdfId));

            if (tdf.getTdfId() == tdfId)
            {
                Heat2Decoder decoder;
                rc = decoder.decode(rawBuffer, tdf);
            }
        }
    }

    return rc;
}

BlazeRpcError RedisResponse::extractTdf(EA::TDF::Tdf *&tdf, TdfDecoder &decoder, MemoryGroupId memGroupId)
{
    BlazeRpcError rc = ERR_SYSTEM;
    tdf = nullptr;

    if (getType() == TYPE_STRING)
    {
        RawBuffer rawBuffer((uint8_t*)mReply->str, mReply->len, false);
        rawBuffer.put(mReply->len);

        EA::TDF::TdfId tdfId;
        if (rawBuffer.datasize() >= sizeof(tdfId))
        {
            memcpy(&tdfId, rawBuffer.data(), sizeof(tdfId));
            rawBuffer.pull(sizeof(tdfId));

            EA::TDF::Tdf* created = EA::TDF::TdfFactory::get().create(tdfId, *Blaze::Allocator::getAllocator(memGroupId), "RedisResponse::extractTdf");
            if (created != nullptr)
            {
                tdf = created; // REDIS_TODO: replace tdf with smart ptr to make sure it gets deleted, such as in the case where the decode fails
                rc = decoder.decode(rawBuffer, *tdf);
            }
        }
    }

    return rc;
}

BlazeRpcError RedisResponse::extractNotification(const NotificationInfo *&info, EA::TDF::Tdf *&payload, TdfDecoder &decoder, MemoryGroupId memGroupId, const char8_t *memGroupName)
{
    BlazeRpcError rc = ERR_SYSTEM;
    info = nullptr;
    payload = nullptr;

    if (getType() == TYPE_STRING)
    {
        RawBuffer rawBuffer((uint8_t*)mReply->str, mReply->len, false);
        rawBuffer.put(mReply->len);

        ComponentId componentId;
        NotificationId notificationId;
        if (rawBuffer.datasize() >= (sizeof(componentId) + sizeof(notificationId)))
        {
            memcpy(&componentId, rawBuffer.data(), sizeof(componentId));
            rawBuffer.pull(sizeof(componentId));

            memcpy(&notificationId, rawBuffer.data(), sizeof(notificationId));
            rawBuffer.pull(sizeof(notificationId));

            info = ComponentData::getNotificationInfo(componentId, notificationId);
            if ((info != nullptr) && (info->payloadTypeDesc != nullptr))
            {
                payload = info->payloadTypeDesc->createInstance(*Allocator::getAllocator(memGroupId), memGroupName);
                if (payload != nullptr)
                    rc = decoder.decode(rawBuffer, *payload);
            }
        }
    }

    return rc;
}

RedisError RedisResponse::waitForReply(TimeValue absoluteTimeout)
{
    RedisError err = REDIS_ERR_OK;
    if (!mCompleted)
    {
        err = RedisErrorHelper::convertError(Fiber::getAndWait(mCompletionEvent, "RedisResponse::waitForReply", absoluteTimeout));
        if (err != REDIS_ERR_TIMEOUT)
            err = hiredisErrorToRedisError(mReplyErr);

        if (err != REDIS_ERR_OK)
            err = REDIS_ERROR(err, "RedisResponse.waitForReply: Wait operation failed : err(" << err << ")");
    }

    return err;
}

void RedisResponse::signalCompletion(BlazeRpcError err)
{
    mCompleted = true;
    if (mCompletionEvent.isValid())
        Fiber::delaySignal(mCompletionEvent, err);
}

void RedisResponse::encode(eastl::string& dest) const
{
    RedisResponseType type = getType();
    dest.append("{ ");
    switch (type)
    {
    case TYPE_NONE:
        dest.append_sprintf("(none), err: %d, errstr: '%s'", mReplyErr, mReplyErrStr.c_str());
        break;
    case TYPE_ERROR:
        dest.append("(err) ");
        dest.append(this->getString());
        break;
    case TYPE_STATUS:
        dest.append("(status) ");
        dest.append(this->getString());
        break;
    case TYPE_INTEGER:
        dest.append("(int) ");
        dest.append_sprintf("%" PRIi64, this->getInteger());
        break;
    case TYPE_STRING:
        dest.append("(string) ");
        dest.append(this->getString());
        break;
    case TYPE_ARRAY:
        dest.append("(array) ");
        for (uint32_t i = 0, sz = getArraySize(); i < sz; ++i)
        {
            getArrayElement(i).encode(dest);
            dest.append(", ");
        }
        break;
    case TYPE_EMPTY:
        dest.append("(empty)");
        break;
    }
    dest.append(" }");
}

// static
RedisError RedisResponse::hiredisErrorToRedisError(int hiredisErr)
{
    switch (hiredisErr)
    {
    case REDIS_OK:
        return REDIS_ERR_OK;
    case REDIS_ERR_IO:
        return REDIS_ERR_HIREDIS_IO;
    case REDIS_ERR_EOF:
        return REDIS_ERR_HIREDIS_EOF;
    case REDIS_ERR_PROTOCOL:
        return REDIS_ERR_HIREDIS_PROTOCOL;
    case REDIS_ERR_OOM:
        return REDIS_ERR_HIREDIS_OOM;
    default:
        return REDIS_ERR_SYSTEM;
    }
}

} // Blaze
