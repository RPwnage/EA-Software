#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include "framework/blaze.h"
#include "framework/protocol/shared/heat2util.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/redis/redisid.h"
#include "EATDF/tdf.h"
#include "EASTL/hash_set.h"
#include "EASTL/hash_map.h"
#include "EAStdC/EAString.h"

#include "hiredis/hiredis.h"

namespace redisinspector
{

static const char* gRedisHost = "localhost";
static int32_t gRedisPort = 6370;
static redisContext *gRedisContext = nullptr;

static int usage(const char* exeName)
{
    printf("Usage: %s <command>\n", exeName);
    printf("    get <key> [<field>]\n");
    return 1;
}

void logBinary(const char *buf, int len)
{
    char bytes[256];
    char ascii[256];
    char num[8];
    bool flushed;

    bytes[0] = '\0';
    ascii[0] = '\0';
    flushed = true;
    for(int i=0; i<len; i++)
    {
        if ( (i % 16) == 0 )
        {
            if ( bytes[0] != '\0' )
            {
                printf("%-50s  %s", bytes, ascii);
                flushed = true;
            }
            strcpy(bytes, "  ");
            memset(ascii, 0, 256);
            ascii[0] = '\0';
        }
        sprintf(num, "%2.2x ", buf[i] & 0xff);
        strcat(bytes, num);
        ascii[i%16] = (buf[i] < 32) ? '.' : buf[i];
        flushed = false;
    }

    if ( !flushed )
        printf("%-50s  %s", bytes, ascii);
    printf("\n");
}

static bool parseCommonArgs(int32_t argc, char** argv, int32_t& idx)
{
    for(idx = 1; idx < argc; ++idx)
    {
        if (argv[idx][0] != '-')
            return true;

        if (strcmp(argv[idx], "-h") == 0)
        {
            ++idx;
            if (idx == argc)
            {
                printf("-h parameter missing redis hostname\n");
                return false;
            }
            gRedisHost = argv[idx];
        }
        else if (strcmp(argv[idx], "-p") == 0)
        {
            ++idx;
            if (idx == argc)
            {
                printf("-p parameter missing redis port\n");
                return false;
            }
            gRedisPort = EA::StdC::AtoI32(argv[idx]);
        }
    }
    return true;
}


static bool connectToRedis()
{
    gRedisContext = redisConnect(gRedisHost, gRedisPort);
    if (gRedisContext->err)
    {
        printf("Connection error: %s\n", gRedisContext->errstr);
        return false;
    }
    return true;
}

static int fetchEntry(int32_t numArgs, const char** args, const size_t *argsLen)
{
    redisReply* reply = (redisReply*)redisCommandArgv(gRedisContext, numArgs, args, argsLen);
    if (reply == nullptr)
    {
        printf("Unable to fetch entry; reply == nullptr\n");
        return 1;
    }
    if (reply->len == 0)
    {
        printf("No entry found.\n");
        return 1;
    }
    EA::TDF::Tdf* tdf = nullptr;
    if ((size_t)reply->len >= sizeof(EA::TDF::TdfId))
    {
        EA::TDF::TdfId tdfId = *(EA::TDF::TdfId*)reply->str;
        tdf = EA::TDF::TdfFactory::get().create(tdfId);
        if (tdf != nullptr)
        {
            Blaze::RawBuffer* raw = BLAZE_NEW Blaze::RawBuffer((uint8_t*)(reply->str + sizeof(EA::TDF::TdfId)), reply->len - sizeof(EA::TDF::TdfId));
            eastl::intrusive_ptr<Blaze::RawBuffer> ptr(raw);
            raw->put(reply->len - sizeof(EA::TDF::TdfId));
            Blaze::Heat2Decoder decoder;
            Blaze::BlazeRpcError rc = decoder.decode(*raw, *tdf);
            if (rc != Blaze::ERR_OK)
            {
                printf("Failed to decode TDF (tdfId=%d)\n", tdfId);
                return 1;
            }
            Blaze::StringBuilder s;
            tdf->print(s, 1);
            printf("%s\n", s.get());
        }
    }
    if (tdf == nullptr)
    {
        bool isString = true;
        for(int idx = 0; (idx < reply->len) && isString; ++idx)
        {
            if (!isascii(reply->str[idx]))
                isString = false;
        }
        if (isString)
        {
            char* str = BLAZE_NEW_ARRAY(char, reply->len+1);
            memcpy(str, reply->str, reply->len);
            str[reply->len] = '\0';
            // Probably a nullptr terminated string
            printf("%s\n", str);
            delete[] str;
        }
        else
        {
            logBinary(reply->str, reply->len);
        }
    }
    return 0;
}

static size_t unescapeKey(const char* inKey, char* outKey)
{
    const char* in = inKey;
    char* out = outKey;
    while (in[0] != '\0')
    {
        if ((in[0] == '\\') && (in[1] != '\0') && (in[1] == 'x') && (in[2] != '\0') && (in[3] != '\0'))
        {
            char hex[3];
            hex[0] = in[2];
            hex[1] = in[3];
            hex[2] = '\0';
            out[0] = (char)strtol(hex, nullptr, 16);
            in += 3;
        }
        else
        {
            out[0] = in[0];
        }
        ++in;
        ++out;
    }
    return (out-outKey);
}

static char* escapeKey(const char* in, size_t len, char* out)
{
    char *o = out;
    for(size_t idx = 0; idx < len; ++idx, ++o)
    {
        if (in[idx] == '\0')
        {
            o[0] = '\\';
            o[1] = 'x';
            o[2] = '0';
            o[3] = '0';
            o += 3;
        }
        else
            o[0] = in[idx];
    }
    o[0] = '\0';
    return out;
}

static int get(int32_t argc, char** argv)
{
    if (!connectToRedis())
        return 1;

    char key[1024];
    size_t keyLen = unescapeKey(argv[0], key);

    const char* args[16];
    size_t argsLen[16];
    args[0] = "TYPE";
    argsLen[0] = strlen(args[0]);
    args[1] = key;
    argsLen[1] = keyLen;
    redisReply* typeReply = (redisReply*)redisCommandArgv(gRedisContext, 2, args, argsLen);
    if (typeReply == nullptr)
    {
        printf("Unable to fetch key type.\n");
        return 1;
    }

    int rc;
    if (strcmp(typeReply->str, "string") == 0)
    {
        args[0] = "GET";
        argsLen[0] = strlen(args[0]);
        args[1] = key;
        argsLen[1] = keyLen;
        rc = fetchEntry(2, args, argsLen);
    }
    else if (strcmp(typeReply->str, "hash") == 0)
    {
        if (argc != 2)
        {
            printf("Attempting to fetch hash member but no field key provided.\n");
            rc =  1;
        }
        else
        {
            args[0] = "HGET";
            argsLen[0] = strlen(args[0]);
            args[1] = key;
            argsLen[1] = keyLen;
            args[2] = argv[1];
            argsLen[2] = strlen(args[2]);
            rc = fetchEntry(3, args, argsLen);
        }
    }
    else
    {
        printf("Fetching type '%s' not supported.\n", typeReply->str);
        rc = 1;
    }
    freeReplyObject(typeReply);
    return rc;
}

static int schema()
{
    if (!connectToRedis())
        return 1;

    Blaze::RedisId schemaId("framework", "blaze_schema");

    char key[1024];
    size_t keyLen = sprintf(key, "%s%c%s%c0%c", schemaId.getNamespace(), '\0', schemaId.getName(), '\0', '\0');

    const char* args[16];
    size_t argsLen[16];
    args[0] = "HGETALL";
    argsLen[0] = strlen(args[0]);
    args[1] = key;
    argsLen[1] = keyLen;
    redisReply* reply = (redisReply*)redisCommandArgv(gRedisContext, 2, args, argsLen);
    if (reply == nullptr)
    {
        printf("Unable to fetch schema.\n");
        return 1;
    }

    printf("%-25s %s\n", "SchemaKey", "FullName");
    for(uint32_t idx = 0; idx < reply->elements; idx += 2)
    {
        char abbr[1024];
        char full[1024];
        printf("%-25s %s\n", escapeKey(reply->element[idx]->str, reply->element[idx]->len, abbr),
                escapeKey(reply->element[idx+1]->str, reply->element[idx+1]->len, full));
    }

    freeReplyObject(reply);
    return 0;
}

int redisinspector_main(int argc, char** argv)
{
    if (argc < 2)
        return usage(argv[0]);

    int32_t idx = 1;
    if (!parseCommonArgs(argc, argv, idx))
        return usage(argv[0]);

    int rc = 0;
    if (strcmp(argv[idx] , "get") == 0)
    {
        ++idx;
        rc = get(argc - idx, &argv[idx]);
    }
    else if (strcmp(argv[idx] , "schema") == 0)
    {
        ++idx;
        rc = schema();
    }
    else
    {
        rc = usage(argv[0]);
    }

    return rc;
}

}
