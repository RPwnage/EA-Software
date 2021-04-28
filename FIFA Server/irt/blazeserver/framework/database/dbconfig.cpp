/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DbConfig
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"
#include "framework/database/dbconfig.h"
#include "framework/database/dbobfuscator.h"
#include "framework/database/dbscheduler.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


/*************************************************************************************************/
/*!
    \brief Constructor
*/
/*************************************************************************************************/
DbConfig::DbConfig(const char8_t* name, const DbConnConfig& config)
    : mDbIndex(0)
{
    reset();

    blaze_strnzcpy(mName, name, sizeof(mName));
    config.copyInto(mConfig);

    decodePassword();

    blaze_snzprintf(mIdentity, sizeof(mIdentity), "(name=%s;host=%s;port=%d;user=%s;db=%s)",
        mName, getHost(), getPort(), getUser(), getDatabase());

    BLAZE_INFO_LOG(Log::DB, "Added DB config " << name << " (Host: " << getHost() << ":" << getPort() << ", Schema: " << getDatabase() << ", UserName: " << getUser() << ")");


    const auto& serverConfigs = gProcessController->getBootConfigTdf().getServerConfigs();

    for (auto& slaveConfig : config.getSlaves())
    {
        if (!slaveConfig->getBoundToServerConfigs().empty())
        {
            bool match = false;
            // If there are bound server types (slave types) configured, then check to see
            // if this current server (e.g. gController) is in the list.  If not, then don't add
            // this db read slave to the list of available db slaves.
            DbConnConfig::StringBoundToServerConfigsList::const_iterator boundItr = slaveConfig->getBoundToServerConfigs().begin();
            DbConnConfig::StringBoundToServerConfigsList::const_iterator boundEnd = slaveConfig->getBoundToServerConfigs().end();
            for ( ; boundItr != boundEnd; ++boundItr)
            {
                if (serverConfigs.find(boundItr->c_str()) == serverConfigs.end())
                {
                    BLAZE_INFO_LOG(Log::DB, "Db slave '" << mName << "' bound to undefined server config '" << boundItr->c_str() << "'");
                }

                if (blaze_stricmp(boundItr->c_str(), gController->getBaseName()) == 0)
                {
                    match = true;
                }
            }

            if (!match)
                continue;
        }

        addSlaveConfig(name, *slaveConfig);
    }
    
    if (!config.getSlaves().empty() && mSlaves.empty())
    {
        BLAZE_INFO_LOG(Log::DB, "No db read slaves have been assigned to this server instance (" << gController->getInstanceName() << ")");
    }
}

DbConfig::DbConfig(const char8_t* name, const DbConnConfig& config, const DbConfig& parent)
    : mDbIndex(0)
{
    reset();
    
    blaze_strnzcpy(mName, name, sizeof(mName));
    config.copyInto(mConfig);

    //We have to go over each parent attrib and copy into here if we don't specify it.
    // we use the config parameter instead of mConfig because the bitfield that keeps track of which fields
    // are set gets completely set when doing the copyInto above.  Both objects contain the same data.
    mConfig.setClient(parent.getDbClient());
    if (!config.isHostnameSet())
        mConfig.setHostname(parent.getHost());
    if (!config.isPortSet())
        mConfig.setPort(parent.getPort());
    if (!config.isDatabaseSet())
        mConfig.setDatabase(parent.getDatabase());
    if (!config.isUsernameSet())
        mConfig.setUsername(parent.getUser());
    if (!config.isPasswordSet())
        mConfig.setPassword(parent.getPassword());
    if (!config.isObfuscatorSet())
        mConfig.setObfuscator(parent.getObfuscator());
    if (!config.isMaxConnCountSet())
        mConfig.setMaxConnCount(parent.getMaxConnCount());
    if (!config.isMinQueryTimeSet())
        mConfig.setMinQueryTime(parent.getMinQueryRuntime());
    if (!config.isHealthcheckSet())
        mConfig.setHealthcheck(parent.isHealthCheckEnabled());
    if (!config.isCharsetSet())
        mConfig.setCharset(parent.getCharset());
    if (!config.isCompressionSet())
        mConfig.setCompression(parent.useCompression());

    // Can't have both sync and async conns in the same pool, since all conns in a pool are created by the same MySql(Async)Admin
    if (parent.getDbConnConfig().getAsyncDbConns() != config.getAsyncDbConns())
        BLAZE_WARN_LOG(Log::DB, "Ignoring configured 'asyncDbConns' setting for DB slave config " << name << "; using value '" << parent.getDbConnConfig().getAsyncDbConns() << "' from DB master config " << parent.getName() << " (DB connection pools must be either entirely synchronous, or entirely asynchronous)");
    mConfig.setAsyncDbConns(parent.getDbConnConfig().getAsyncDbConns());

    decodePassword();

    blaze_snzprintf(mIdentity, sizeof(mIdentity), "(name=%s;host=%s;port=%d;user=%s;db=%s)",
        mName, getHost(), getPort(), getUser(), getDatabase());

    BLAZE_INFO_LOG(Log::DB, "Added DB slave config " << name << " (Host: " << getHost() << ":" << getPort() << ", Schema: " << getDatabase() << ", UserName: " << getUser() << ")");
}

// Copy constructor for DbInstancePools' copies
DbConfig::DbConfig(const DbConfig& orig)
    : mDbIndex(orig.mDbIndex)
{
    reset();
    
    blaze_strnzcpy(mName, orig.mName, sizeof(mName));
    orig.mConfig.copyInto(mConfig);
    mBoundServerTypeMap = orig.mBoundServerTypeMap;
    blaze_strnzcpy(mIdentity, orig.mIdentity, sizeof(mIdentity));
    // copying mSlaves isn't currently required for DbInstancePools
}

DbConfig::~DbConfig()
{
    SlaveList::iterator itr = mSlaves.begin();
    SlaveList::iterator end = mSlaves.end();
    for(; itr != end; ++itr)
        delete *itr;
}

DbConfig& DbConfig::addSlaveConfig(const char8_t *name, const DbConnConfig& dbConnConfig)
{
    char8_t slaveName[128];
    blaze_snzprintf(slaveName, sizeof(slaveName), "%s-slv%d", name, ++mDbIndex);
    DbConfig* slaveConfig = BLAZE_NEW DbConfig(slaveName, dbConnConfig, *this);
    mSlaves.push_back(slaveConfig);

    return *slaveConfig;
}

uint32_t DbConfig::getSummaryMaxConnCount() const
{
    uint32_t count = getMaxConnCount();
    SlaveList::const_iterator itr = mSlaves.begin();
    SlaveList::const_iterator end = mSlaves.end();
    for(; itr != end; ++itr)
    {
        count += (*itr)->getMaxConnCount();
    }
    return count;
}

void DbConfig::decodePassword()
{
    // check if password was obfuscated 
    if (*mConfig.getObfuscator() != '\0')
    {
        const size_t DECODED_BUFFER_SIZE = 256;
        char8_t buffer[DECODED_BUFFER_SIZE];

        size_t len = DbObfuscator::decode(mConfig.getObfuscator(), mConfig.getPassword(), buffer, DECODED_BUFFER_SIZE);

        if (len < DECODED_BUFFER_SIZE)
        {
            mConfig.setPassword(buffer);
        }
        else
        {
            BLAZE_ERR_LOG(Log::DB, "Could not decode database password: insufficient buffer size.");
        }
    }
}


void DbConfig::reset()
{
    mName[0] = '\0';
    mIdentity[0] = '\0';
}

}// Blaze
