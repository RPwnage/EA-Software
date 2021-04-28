/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_COMMAND_H
#define BLAZE_REDIS_COMMAND_H

/*** Include files *******************************************************************************/

#include "blazerpcerrors.h"
#include "EATDF/tdf.h"
#include "EASTL/fixed_vector.h"
#include "EASTL/bitvector.h"
#include "EASTL/utility.h"
#include "EASTL/string.h"
#include "hiredis/hiredis.h"
#include "framework/redis/redisid.h"
#include "framework/redis/redissliver.h"
#include "framework/redis/redisdecoder.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

struct NotificationInfo;
class RedisCommand;
class StringBuilder;

/*! *****************************************************************************/
/*! \class 
    \brief Used to build a Redis command.

    A RedisCommand is passed to the various methods found in RedisCluster and
    RedisConn when sending a command to the Redis server.
*********************************************************************************/
class RedisCommand
{
    NON_COPYABLE(RedisCommand);

    static const size_t DEFAULT_BUFFER_SIZE = (1024);
    static const size_t DEFAULT_PARAM_COUNT = (32);

public:
    RedisCommand(size_t bufferSize = DEFAULT_BUFFER_SIZE);
    virtual ~RedisCommand();

    void reset();

    RedisCommand& begin();
    RedisCommand& beginKey();
    RedisCommand& end();

    // Various overloaded methods for adding arguments to this RedisCommand.
    RedisCommand& add(const RedisId& value);
    RedisCommand& add(const RedisSliver& value);
    template<typename T, typename Allocator>
    RedisCommand& add(const eastl::basic_string<T, Allocator>& value)      { appendBuffer((const uint8_t*)value.c_str(), value.size(), false, mEscaping!=0); return (*this); }
    RedisCommand& add(const std::string& value)        { appendBuffer((const uint8_t*)value.data(), value.size(), false, mEscaping != 0); return (*this); }
    RedisCommand& add(const char8_t* value)            { appendBuffer((const uint8_t*)value, strlen(value), false, mEscaping!=0); return (*this); }
    RedisCommand& add(const char8_t& value)            { appendBuffer((const uint8_t*)&value, 1, false, mEscaping!=0); return (*this); }
    RedisCommand& add(int8_t value)                    { appendSignedInt(value, false); return (*this); }
    RedisCommand& add(int16_t value)                   { appendSignedInt(value, false); return (*this); }
    RedisCommand& add(int32_t value)                   { appendSignedInt(value, false); return (*this); }
    RedisCommand& add(int64_t value)                   { appendSignedInt(value, false); return (*this); }
    RedisCommand& add(uint8_t value)                   { appendUnsignedInt(value, false); return (*this); }
    RedisCommand& add(uint16_t value)                  { appendUnsignedInt(value, false); return (*this); }
    RedisCommand& add(uint32_t value)                  { appendUnsignedInt(value, false); return (*this); }
    RedisCommand& add(uint64_t value)                  { appendUnsignedInt(value, false); return (*this); }
    RedisCommand& add(double value)                    { appendDouble(value, false); return (*this); }

    RedisCommand& add(const EA::TDF::TdfBlob& blob)    { appendBuffer(blob.getData(), blob.getCount(), false, mEscaping!=0); return (*this); }
    RedisCommand& add(const RawBuffer& value)          { appendBuffer(value.data(), value.datasize(), false, mEscaping!=0); return (*this); }
    RedisCommand& add(const uint8_t* valueBuf, size_t valueBufSize) { appendBuffer(valueBuf, valueBufSize, false, mEscaping!=0); return (*this); }
    RedisCommand& add(const NotificationInfo& info, const EA::TDF::Tdf* payload) { appendNotification(info, payload, false); return (*this); }
    RedisCommand& add(const EA::TDF::Tdf& value)       { appendTdf(value, false); return (*this); }
    RedisCommand& add(const RedisCommand& value)       { appendRedisCommand(value); return (*this); }
    RedisCommand& add(const BlazeObjectId& value)      { appendObjectId(value, false); return (*this); }

    // Helper method to specifically add keys to the command.  Can be used instead of beginKey().add(myKey).end();
    template<typename T, typename Allocator>
    RedisCommand& addKey(const eastl::basic_string<T, Allocator>& value)   { appendBuffer((const uint8_t*)value.c_str(), value.size(), true, mEscaping!=0); return (*this); }
    RedisCommand& addKey(const std::string& value)     { appendBuffer((const uint8_t*)value.data(), value.size(), true, mEscaping != 0); return (*this); }
    RedisCommand& addKey(const char8_t* value)         { appendBuffer((const uint8_t*)value, strlen(value), true, mEscaping != 0); return (*this); }
    RedisCommand& addKey(const char8_t& value)         { appendBuffer((const uint8_t*)&value, 1, true, mEscaping!=0); return (*this); }
    RedisCommand& addKey(int8_t value)                 { appendSignedInt(value, true); return (*this); }
    RedisCommand& addKey(int16_t value)                { appendSignedInt(value, true); return (*this); }
    RedisCommand& addKey(int32_t value)                { appendSignedInt(value, true); return (*this); }
    RedisCommand& addKey(int64_t value)                { appendSignedInt(value, true); return (*this); }
    RedisCommand& addKey(uint8_t value)                { appendUnsignedInt(value, true); return (*this); }
    RedisCommand& addKey(uint16_t value)               { appendUnsignedInt(value, true); return (*this); }
    RedisCommand& addKey(uint32_t value)               { appendUnsignedInt(value, true); return (*this); }
    RedisCommand& addKey(uint64_t value)               { appendUnsignedInt(value, true); return (*this); }
    RedisCommand& addKey(double value)                 { appendDouble(value, true); return (*this); }
    RedisCommand& addKey(const BlazeObjectId& value)   { appendObjectId(value, true); return (*this); }

    template<typename A, typename B>
    RedisCommand &add(const eastl::pair<A, B>& value)
    {
        beginEscaping();
        add(value.first);
        appendBuffer((const uint8_t*)&REDIS_PAIR_DELIMITER, 1, false, false);
        add(value.second);
        endEscaping();
        return (*this);
    }

    size_t getParamCount() const { return mParams.size(); }
    int32_t getKeyCount() const { return mKeyCount; }
    size_t getDataSize() const { return mRawBuffer.datasize(); }
    bool getParam(size_t index, const char8_t*& param, size_t& paramLen) const;

    uint16_t getHashSlot() const;
    void encode(eastl::string& dest) const;

    void setNamespace(const char8_t* namespaceName) { mNamespace = namespaceName; }
    const char8_t* getNamespace() const { return mNamespace; }

protected:
    friend class RedisConn;

    RedisCommand& beginInternal(bool isKey);

    RedisCommand& beginEscaping() { ++mEscaping; return begin(); }
    RedisCommand& endEscaping() { --mEscaping; return end(); }

    void appendRedisCommand(const RedisCommand& sourceCommand);
    void appendBuffer(const uint8_t* valueBuf, size_t valueBufSize, bool isKey, bool escape);
    void appendSignedInt(int64_t value, bool isKey);
    void appendUnsignedInt(uint64_t value, bool isKey);
    void appendDouble(double value, bool isKey);
    void appendTdf(const EA::TDF::Tdf& value, bool isKey);
    void appendNotification(const NotificationInfo& info, const EA::TDF::Tdf* payload, bool isKey);
    void appendObjectId(const BlazeObjectId& id, bool isKey);

    const char8_t** getParams() const;
    const size_t* getParamLengths() const { return &mParamLengths.front(); }

    void updateParamPointers() const;

private:
    void pushParam(const char8_t* ptr, size_t len, bool isKey);

private:
    RawBuffer mRawBuffer;

    typedef eastl::fixed_vector<const char8_t*, DEFAULT_PARAM_COUNT> ParamVector;
    mutable ParamVector mParams; // We mark this mutable because it can be modified in the const getParams() method.

    typedef eastl::fixed_vector<size_t, DEFAULT_PARAM_COUNT> ParamLengthVector;
    ParamLengthVector mParamLengths;

    typedef eastl::bitvector<> ParamKeysVector;
    ParamKeysVector mParamKeys;

    int32_t mKeyCount;

    int32_t mEscaping;
    int32_t mAppendingParam;
    bool mIsAppendingKey;   // NOTE this is only used for EA_ASSERT'ing.  It has no other use other
                            //      than to make sure someone doesn't mistakenly call, for example,
                            //      cmd.begin(), then later call cmd.beginKey() without first ending the
                            //      previous begin() call with an call to end().
    const char8_t* mNamespace;
};

} // Blaze

#endif // BLAZE_REDIS_COMMAND_H

