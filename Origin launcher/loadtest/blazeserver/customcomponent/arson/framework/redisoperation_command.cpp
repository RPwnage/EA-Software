#include "framework/blaze.h"
#include "arson/rpc/arsonslave/redisoperation_stub.h"
#include "arson/tdf/arson.h"
#include "framework/redis/rediskeyfieldmap.h"
#include "framework/usersessions/usersession.h"
#include "framework/redis/redismanager.h"
#include "arson/arsonslaveimpl.h"


namespace Blaze
{

    namespace Arson
    {
        typedef RedisKeyFieldMap<uint32_t, eastl::string, eastl::string> TestRedisMap;

        class RedisOperationCommand : public RedisOperationCommandStub
        {
        public:
            RedisOperationCommand(Message* message, RedisMapData *req, ArsonSlaveImpl* component)
                : RedisOperationCommandStub(message, req),
                mComponent(component)
            {
            }

            ArsonSlaveImpl* mComponent;

            ~RedisOperationCommand() override { }

            RedisOperationCommandStub::Errors execute() override
            {
                Blaze::RedisError rc = Blaze::REDIS_ERR_OK;
                TestRedisMap testMap(RedisId(RedisManager::UNITTESTS_NAMESPACE, "arsontestMap"));
                
                TestRedisMap::Value val("");

                switch (mRequest.getOpType())
                {
                case RedisMapData::HGET:
                    rc = testMap.find(mRequest.getKey(), mRequest.getField(), val);
                    if (rc == REDIS_ERR_OK)
                    {
                        TRACE_LOG("[RedisOperationComman].execute: GetValue: "<< val);
                        mResponse.setValue(val.c_str());
                        mResponse.setField(mRequest.getField());
                        mResponse.setKey(mRequest.getKey());
                    }
                    break;
                case RedisMapData::HSET:
                    rc = testMap.upsert(mRequest.getKey(),mRequest.getField(), mRequest.getValue()); 
                    mResponse.setField(mRequest.getField());
                    mResponse.setKey(mRequest.getKey());
                    mResponse.setValue(mRequest.getValue());
                    break;
                default:
                    TRACE_LOG("[RedisOperationComman].execute: Operation Type(" << mRequest.getOpType() << ") is invalid");
                    return ERR_SYSTEM;
                    break;
                }
       
                return arsonErrorFromRedisError(rc);
            }

            RedisOperationCommandStub::Errors  arsonErrorFromRedisError(Blaze::RedisError redisError)
            {
                Errors err = ERR_SYSTEM;
                switch(redisError.error)
                {
                    case Blaze::REDIS_ERR_OK: err = ERR_OK; break;
                    case Blaze::REDIS_ERR_SYSTEM: err = ERR_SYSTEM; break;
                    case Blaze::REDIS_ERR_NOT_FOUND: err = ARSON_REDIS_ERR_NOT_FOUND; break;
                    case Blaze::REDIS_ERR_INVALID_CONFIG: err = ARSON_REDIS_ERR_INVALID_CONFIG; break;
                    case Blaze::REDIS_ERR_TIMEOUT: err = ARSON_REDIS_ERR_TIMEOUT; break;
                    case Blaze::REDIS_ERR_NOT_CONNECTED: err = ARSON_REDIS_ERR_NOT_CONNECTED; break;
                    case Blaze::REDIS_ERR_ALREADY_CONNECTED: err = ARSON_REDIS_ERR_ALREADY_CONNECTED; break;
                    case Blaze::REDIS_ERR_DNS_LOOKUP_FAILED: err = ARSON_REDIS_ERR_DNS_LOOKUP_FAILED; break;
                    case Blaze::REDIS_ERR_CONNECT_FAILED: err = ARSON_REDIS_ERR_CONNECT_FAILED; break;
                    case Blaze::REDIS_ERR_SEND_COMMAND_FAILED: err = ARSON_REDIS_ERR_COMMAND_FAILED; break;
                    case Blaze::REDIS_ERR_SERVER_AUTH_FAILED: err = ARSON_REDIS_ERR_SERVER_AUTH_FAILED; break;
                    case Blaze::REDIS_ERR_UNEXPECTED_TYPE: err = ARSON_REDIS_ERR_UNEXPECTED_TYPE; break;
                    case Blaze::REDIS_ERR_COMMAND_FAILED: err = ARSON_REDIS_ERR_COMMAND_FAILED; break;
                    case Blaze::REDIS_ERR_OBJECT_NOT_FOUND: err = ARSON_REDIS_ERR_OBJECT_NOT_FOUND; break;
                    default:
                    {
                        //got an error not defined in rpc definition, log it
                        TRACE_LOG("[RedisOperationComman].arsonErrorFromRedisError: unexpected error(" << SbFormats::HexLower(redisError.error) << "): return as ERR_SYSTEM");
                        break;
                    }
                }

                return err;
            }
        };

        RedisOperationCommandStub *RedisOperationCommandStub::create(
            Message* message, RedisMapData *req, ArsonSlave* componentImpl)
        {
            return BLAZE_NEW RedisOperationCommand(message, req, (ArsonSlaveImpl*)componentImpl);
        }

    } // namespace Dummy
} // namespace Blaze

