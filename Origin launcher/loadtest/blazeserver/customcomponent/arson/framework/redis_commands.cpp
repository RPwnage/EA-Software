#include "framework/blaze.h"
#include "arson/rpc/arsonslave/redis_stub.h"
#include "arson/tdf/arson.h"
#include "framework/usersessions/usersession.h"
#include "arson/arsonslaveimpl.h"

namespace Blaze
{

namespace Arson
{

class RedisCommand : public RedisCommandStub
{
public:
    RedisCommand(Message* message, RedisRequest *req, ArsonSlaveImpl* component)
        : RedisCommandStub(message, req),
          mComponent(component)
    {
    }

    ArsonSlaveImpl* mComponent;

    ~RedisCommand() override { }

    RedisCommandStub::Errors execute() override
    {
        BlazeRpcError rc = Blaze::ERR_OK;

        //switch (mRequest.getOp())
        //{
        //    case RedisRequest::REDIS_OP_GET:
        //    {
        //        if (mRequest.getUseTdf())
        //        {
        //            rc = mComponent->getRedis().get(mRequest.getKey(), mResponse.getTdf());
        //        }
        //        else
        //        {
        //            char8_t* value = nullptr;
        //            size_t valueLen = 0;
        //            rc = mComponent->getRedis().get(mRequest.getKey(), value, valueLen);
        //            if (rc != Blaze::ERR_OK)
        //                break;

        //            RedisEntry* entry = mResponse.getResults().pull_back();
        //            entry->setKey(mRequest.getKey());

        //            if (valueLen > 0)
        //            {
        //                eastl::string val; // copy value into our own nullptr-terminated buffer
        //                val.sprintf("%.*s", valueLen, value);
        //                entry->setValue(val.c_str());
        //            }

        //            delete value;
        //        }
        //        break;
        //    }
        //    case RedisRequest::REDIS_OP_SET:
        //    {
        //        if (mRequest.getUseTdf())
        //        {
        //            rc = mComponent->getRedis().set(mRequest.getKey(),
        //                    mRequest.getTdf(), mRequest.getExpiry());
        //        }
        //        else
        //        {
        //            rc = mComponent->getRedis().set(mRequest.getKey(),
        //                    mRequest.getValue(), strlen(mRequest.getValue()), mRequest.getExpiry());
        //        }
        //        break;
        //    }
        //    case RedisRequest::REDIS_OP_REMOVE:
        //    {
        //        rc = mComponent->getRedis().del(mRequest.getKey());
        //        break;
        //    }
        //}

        return (RedisCommandStub::Errors)rc;
    }
};

RedisCommandStub *RedisCommandStub::create(
    Message* message, RedisRequest *req, ArsonSlave* componentImpl)
{
    return BLAZE_NEW RedisCommand(message, req, (ArsonSlaveImpl*)componentImpl);
}

} // namespace Dummy
} // namespace Blaze

