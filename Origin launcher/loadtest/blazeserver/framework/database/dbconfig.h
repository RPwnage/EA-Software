/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_DBCONFIG_H
#define BLAZE_DBCONFIG_H
/*** Include files *******************************************************************************/
#include "EASTL/vector.h"
#include "EASTL/hash_set.h"

#include "framework/tdf/frameworkconfigtypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

// Filenames for the blaze database dynamically loaded libraries
#if defined(EA_PLATFORM_WINDOWS)
#define BLAZE_MYSQL_DLL "blazedb_mysql.dll"
#elif defined(EA_PLATFORM_LINUX)
#define BLAZE_MYSQL_DLL "libblazedb_mysql.so"
#endif


class DbConfig
{
public:
    typedef eastl::vector<DbConfig*> SlaveList;

    DbConfig(const char8_t* name, const DbConnConfig& config);

    ~DbConfig();

    const char8_t *getName() const { return mName; }
    uint32_t getMaxConnCount() const {return mConfig.getMaxConnCount();}
    uint32_t getSummaryMaxConnCount() const;
    bool isHealthCheckEnabled() const {return mConfig.getHealthcheck();}
    const char8_t* getPassword() const {return mConfig.getPassword();}
    const char8_t* getUser() const {return mConfig.getUsername();}
    const char8_t* getDatabase() const {return mConfig.getDatabase();}
    const char8_t* getHost() const {return mConfig.getHostname();}
    uint16_t getPort() const {return mConfig.getPort();}
    DbConnConfig::DbClientType getDbClient() const {return mConfig.getClient();}
    const char8_t* getCharset() const { return mConfig.getCharset(); }
    bool useCompression() const { return mConfig.getCompression(); }
    const char8_t* getObfuscator() const { return mConfig.getObfuscator(); }

    EA::TDF::TimeValue getMinQueryRuntime() const { return mConfig.getMinQueryTime(); }
    EA::TDF::TimeValue getMaxQueryRuntime() const { return mConfig.getMaxQueryTime(); }
    EA::TDF::TimeValue getMaxConnPingtime() const { return mConfig.getMaxPingTime(); }
    EA::TDF::TimeValue getMaxConnKilltime() const { return mConfig.getMaxKillTime(); }

    const SlaveList& getSlaves() const { return mSlaves; }

    const char8_t* getIdentity() const { return mIdentity; }

    void setMaxConnCount(uint32_t maxConnCount) { mConfig.setMaxConnCount(maxConnCount); }
    void setTimeouts(const DbConnConfig& config)
    {
        mConfig.setMinQueryTime(config.getMinQueryTime());
        mConfig.setMaxQueryTime(config.getMaxQueryTime());
        mConfig.setMaxPingTime(config.getMaxPingTime());
        mConfig.setMaxKillTime(config.getMaxKillTime());
    }

    const DbConnConfig& getDbConnConfig() const { return mConfig;  }

private:
    void reset();
    friend class DbConnPool;
    friend class DbInstancePool;

    DbConfig(const char8_t* name, const DbConnConfig& config,
        const DbConfig& parent);

    DbConfig(const DbConfig& orig);

    void decodePassword();

    SlaveList& getSlaves() { return mSlaves; }

    DbConfig& addSlaveConfig(const char8_t *name, const DbConnConfig& dbConnConfig);

    // used when creating the db names in addSlaveConfig()
    int mDbIndex;
    char8_t mName[256];
    DbConnConfig mConfig;

    typedef eastl::hash_set<eastl::string, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> BoundServerMap;
    BoundServerMap mBoundServerTypeMap;

    SlaveList mSlaves;
    char8_t mIdentity[1024];
};

}// Blaze
#endif // BLAZE_DBCONFIG_H
