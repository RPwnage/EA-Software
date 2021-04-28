/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RedisCommand

    Provides the ability to build a Redis command.  Each call to the add() methods adds a new
    parameter to the command.  Typically, the first parameter will always be the Redis command
    that you would like to send to the Redis server.  Additional parameters that are added are
    typically the arguments for the command.  See http://redis.io/commands for the documentation
    on each available command.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/redis/redis.h"
#include "framework/util/shared/stringbuilder.h"
#include "framework/redis/rediscommand.h"
#include "framework/protocol/shared/heat2encoder.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

RedisCommand::RedisCommand(size_t bufferSize)
    : mRawBuffer(bufferSize),
      mKeyCount(0),
      mEscaping(0),
      mAppendingParam(0),
      mIsAppendingKey(false),
      mNamespace("")
{
}

RedisCommand::~RedisCommand()
{
    EA_ASSERT(mAppendingParam == 0);
}

void RedisCommand::reset()
{
    mRawBuffer.reset();
    mParams.clear();
    mParamLengths.clear();
    mParamKeys.clear();
    mKeyCount = 0;
}

const char **RedisCommand::getParams() const
{
    if (mParams.empty())
        return nullptr;

    updateParamPointers();

    return &mParams.front();
}

void RedisCommand::updateParamPointers() const
{
    if (!mParams.empty())
    {
        // Check to see if we need to update our list of pointers.  This can
        // happen, for example, if the RawBuffer was realloc'd.
        if ((const char8_t*)mRawBuffer.head() != mParams.front())
        {
            mParams.at(0) = (const char8_t*)mRawBuffer.head();
            for (size_t a = 1; a < mParams.size(); ++a)
            {
                mParams.at(a) = mParams[a - 1] + mParamLengths[a - 1];
            }
        }
    }
}

bool RedisCommand::getParam(size_t index, const char8_t*& param, size_t& paramLen) const
{
    if (index >= mParams.size())
        return false;

    updateParamPointers();
    param = mParams.at(index);
    paramLen = mParamLengths.at(index);
    return true;
}

RedisCommand &RedisCommand::beginInternal(bool isKey)
{
    pushParam((char8_t*)mRawBuffer.tail(), 0, isKey);
    if ((mAppendingParam > 0) && mIsAppendingKey)
    {
        // NOTE this is only used for EA_ASSERT'ing.  It has no other use other
        //      than to make sure someone doesn't mistakenly call, for example,
        //      cmd.begin(), then later call cmd.beginKey() without first ending the
        //      previous begin() call with an call to end().
        EA_ASSERT(!isKey);
    }
    mIsAppendingKey = isKey;
    mAppendingParam++;
    return (*this);
}

RedisCommand &RedisCommand::begin()
{
    return beginInternal(false);
}

RedisCommand &RedisCommand::beginKey()
{
    return beginInternal(true);
}

RedisCommand &RedisCommand::end()
{
    EA_ASSERT(mAppendingParam > 0);
    if (--mAppendingParam == 0)
        mIsAppendingKey = false;
    return (*this);
}

void RedisCommand::pushParam(const char *ptr, size_t len, bool isKey)
{
    if (mAppendingParam == 0)
    {
        mParams.push_back(ptr);
        mParamLengths.push_back(len);
        mParamKeys.push_back(isKey);
        if (isKey)
            ++mKeyCount;
    }
    else
    {
        mParamLengths.back() += len;
    }
}

RedisCommand &RedisCommand::add(const RedisId &value)
{
    return add(value.mSchemaKeyPair);
}

RedisCommand& RedisCommand::add(const RedisSliver& value)
{
    if (value.isTagged)
    {
        return begin().add('{').add(value.sliverId).add('}').end();
    }
    return add(value.sliverId);
}

void RedisCommand::appendRedisCommand(const RedisCommand &sourceCommand)
{
    sourceCommand.updateParamPointers();

    for (size_t a = 0; a < sourceCommand.getParamCount(); ++a)
    {
        appendBuffer((const uint8_t *)sourceCommand.mParams[a], sourceCommand.mParamLengths[a], sourceCommand.mParamKeys[a], mEscaping!=0);
    }
}

void RedisCommand::appendBuffer(const uint8_t *valueBuf, size_t valueBufSize, bool isKey, bool escape)
{
    if (escape)
    {
        const uint8_t* substringStart = valueBuf;
        size_t substringLength = 0;
        int32_t escapedChars = 0;

        for (size_t i = 0; i < valueBufSize; ++i)
        {
            if ((*valueBuf == REDIS_PAIR_DELIMITER_ESCAPE) || (*valueBuf == REDIS_PAIR_DELIMITER))
            {
                mRawBuffer.acquire(substringLength + 1);
                memcpy(mRawBuffer.tail(), substringStart, substringLength);
                mRawBuffer.put(substringLength);
                *mRawBuffer.tail() = REDIS_PAIR_DELIMITER_ESCAPE;
                mRawBuffer.put(1);

                substringLength = 0;
                substringStart = valueBuf;
                ++escapedChars;
            }

            ++substringLength;
            ++valueBuf;
        }

        mRawBuffer.acquire(substringLength);
        memcpy(mRawBuffer.tail(), substringStart, substringLength);
        mRawBuffer.put(substringLength);

        pushParam((char8_t*)mRawBuffer.tail() - (valueBufSize + escapedChars), valueBufSize + escapedChars, isKey);
    }
    else
    {
        uint8_t *buf = mRawBuffer.acquire(valueBufSize);
        memcpy(buf, valueBuf, valueBufSize);
        mRawBuffer.put(valueBufSize);

        pushParam((char8_t*)buf, valueBufSize, isKey);
    }
}

void RedisCommand::appendSignedInt(int64_t value, bool isKey)
{
    char8_t* buf = (char8_t*)mRawBuffer.acquire(64);
    EA::StdC::I64toa(value, buf, 10);
    size_t len = strlen(buf);
    mRawBuffer.put(len);
    pushParam((char8_t*)buf, len, isKey);
}

void RedisCommand::appendUnsignedInt(uint64_t value, bool isKey)
{
    char8_t* buf = (char8_t*)mRawBuffer.acquire(64);
    EA::StdC::U64toa(value, buf, 10);
    size_t len = strlen(buf);
    mRawBuffer.put(len);
    pushParam((char8_t*)buf, len, isKey);
}

void RedisCommand::appendDouble(double value, bool isKey)
{
    // We only get 16 decimal places back from redis for doubles, so we don't need to give it more precision than that
    // This prints all doubles in the form: "-1.7976931348623157e+308", "3.1400000000000001e+00", "inf", "-inf" or "nan".
    // "nan" however will return an error if used for a sorted set score, because nan != nan.

    char8_t* buf = (char8_t*)mRawBuffer.acquire(25);        // ex: -1.7976931348623157e+308 + \0
    size_t len = static_cast<size_t>(blaze_snzprintf(buf, 25, "%.16e", value));
    mRawBuffer.put(len);
    pushParam(buf, len, isKey);
}

void RedisCommand::appendTdf(const EA::TDF::Tdf &value, bool isKey)
{
    EA_ASSERT_MSG(mEscaping == 0, "Escaping Tdf serialization is not supported");
    // Grab the current head and tail so we can recalculate sizes in case of realloc
    uint8_t* oldHead = mRawBuffer.head();
    uint8_t* oldTail = mRawBuffer.tail();

    // First store the TdfId
    EA::TDF::TdfId tdfId = value.getTdfId();
    uint8_t *buf = mRawBuffer.acquire(sizeof(tdfId));
    memcpy(buf, &tdfId, sizeof(tdfId));
    mRawBuffer.put(sizeof(tdfId));

    // Then encode the Tdf into the RawBuffer
    Heat2Encoder encoder;
    bool success = encoder.encode(mRawBuffer, value);

    // Recalculate the tail in case the buffer was realloc'd
    oldTail += (mRawBuffer.head() - oldHead);

    if (EA_LIKELY(success))
    {
        pushParam((char8_t*)oldTail, mRawBuffer.tail() - oldTail, isKey);
    }
    else
    {
        BLAZE_ERR_LOG(Log::REDIS, "[RedisCommand].add(Tdf): Failed to encode the '" << value.getClassName() << "' Tdf.");
        // We failed, so trim back to where we started
        mRawBuffer.trim(mRawBuffer.tail() - oldTail);
    }
}

void RedisCommand::appendNotification(const NotificationInfo &info, const EA::TDF::Tdf *payload, bool isKey)
{
    EA_ASSERT_MSG(mEscaping == 0, "Escaping notification serialization is not supported");
    uint8_t* oldHead = mRawBuffer.head();
    uint8_t* oldTail = mRawBuffer.tail();

    uint8_t *buf = mRawBuffer.acquire(sizeof(info.componentId));
    memcpy(buf, &info.componentId, sizeof(info.componentId));
    mRawBuffer.put(sizeof(info.componentId));

    buf = mRawBuffer.acquire(sizeof(info.notificationId));
    memcpy(buf, &info.notificationId, sizeof(info.notificationId));
    mRawBuffer.put(sizeof(info.notificationId));

    bool success = true;
    if (payload != nullptr)
    {
        Heat2Encoder encoder;
        success = encoder.encode(mRawBuffer, *payload); 
    }

    // Recalculate the tail in case the buffer was realloc'd
    oldTail += (mRawBuffer.head() - oldHead);

    if (EA_LIKELY(success))
    {
        pushParam((char8_t*)oldTail, mRawBuffer.tail() - oldTail, isKey);
    }
    else
    {
        BLAZE_ERR_LOG(Log::REDIS, "[RedisCommand].add(NotificationInfo,Tdf): Failed to encode the '" << payload->getClassName() << "' Tdf.");
        // We failed, so trim back to where we started
        mRawBuffer.trim(mRawBuffer.tail() - oldTail);
    }
}

uint16_t RedisCommand::getHashSlot() const
{
    uint16_t hashSlot = UINT16_MAX;

    updateParamPointers();

    for (size_t a = 0; a < mParamKeys.size(); ++a)
    {
        if (EA_UNLIKELY(mParamKeys[a]))
        {
            uint16_t hs = RedisCluster::computeHashSlot(mParams[a], mParamLengths[a]);
            if (hashSlot == UINT16_MAX)
                hashSlot = hs;
            else if (hashSlot != hs)
                return UINT16_MAX;
        }
    }

    return hashSlot;
}

void RedisCommand::encode(eastl::string& dest) const
{
    for (uint32_t i = 0, sz = (uint32_t)mParams.size(); i < sz; ++i)
    {
        // TODO: This does not handle blobs or binary formatted data
        // we need to add a bitset similar to ParamBinVector
        // that marks elements as binary so that
        // we can encode them correctly!
        dest.append(mParams[i], mParamLengths[i]);
        dest += ' ';
    }
    if (!dest.empty())
        dest.pop_back();
}

void RedisCommand::appendObjectId(const BlazeObjectId& objectId, bool isKey)
{
    beginInternal(isKey);
    add(objectId.type.component);
    add(":");
    add(objectId.type.type);
    add(":");
    add(objectId.id);
    end();
}

} // Blaze
