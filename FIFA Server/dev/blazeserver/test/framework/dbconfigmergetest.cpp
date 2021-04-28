/*************************************************************************************************/
/*!
\file dbconfigmergetest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/config/configmerger.h"
#include "framework/config/config_file.h"
#include "framework/config/configdecoder.h"
#include "test/framework/dbconfigmergetest.h"

namespace BlazeServerTest
{
namespace frameworktest
{

    using ::testing::AtLeast;
    using ::testing::Return;
    using ::testing::NiceMock;
    using ::testing::_;
    using namespace Blaze;

    static const char8_t* INITIAL_CONFIG = R"__CONFIG(
        databaseConnections = {
            main = {
                vaultPath = { path = "vault_path/db_name/master" }
                hostname = "db_master_host"
                port = 3306
                database = "main"
                username = "rw_dbuser"
                password = "rw_dbpass"
                obfuscator = "db_obfuscator"
                maxConnCount = 5
                maxPingTime = "10s"
                instanceOverrides = {
                    coreSlave = {
                        maxConnCount = 1
                    }
                }
                slaves = [
                    {
                        vaultPath = { path = "vault_path/db_name/slave" }
                        hostname = "db_slave_host"
                        port = 3306
                        database = "main"
                        username = "ro_dbuser"
                        password = "ro_dbpass"
                        obfuscator = "db_obfuscator"
                        maxConnCount = 5
                        maxPingTime = "10s"
                    }
                ]
            }
        }
    )__CONFIG";

    // deliberate change to validate correct handling of ConfigMerger handling of overriding fields marked reconfigurable=false
    static const char8_t* UPDATED_CONFIG = R"__CONFIG(
        databaseConnections = {
            main = {
                vaultPath = { path = "vault_path/db_name/master" }
                hostname = "db_master_host"
                port = 3306
                database = "main"
                username = "rw_dbuser"
                password = "rw_dbpass"
                obfuscator = "db_obfuscator"
                maxConnCount = 5
                maxPingTime = "10s"
                instanceOverrides = {
                    coreSlave = {
                        maxConnCount = 1
                    }
                }
                slaves = [
                    {
                        vaultPath = { path = "vault_path/db_name/slave" }
                        hostname = "db_slave_host"
                        port = 3306
                        database = "main"
                        username = "ro_dbuser"
                        password = "ro_dbpass"
                        obfuscator = "db_obfuscator"
                        maxConnCount = 5
                        maxPingTime = "10s"
                    },
                    {
                        vaultPath = { path = "vault_path/db_name/slave" }
                        hostname = "another_db_slave_host"
                        port = 3306
                        database = "main"
                        username = "ro_dbuser"
                        password = "ro_dbpass"
                        obfuscator = "db_obfuscator"
                        maxConnCount = 5
                        maxPingTime = "10s"
                    }
                ]
            }
        }
    )__CONFIG";


    DbConfigMergeTest::DbConfigMergeTest()
    {

    }

    void DbConfigMergeTest::SetUp()
    {

    }

    DbConfigMergeTest::~DbConfigMergeTest()
    {
        
    }

    // The purpose of this test is to mirror/validate the behavior of the blaze DbScheduler::reconfigure flow 
    // when new db slave configs are added see(https://eadpjira.ea.com/browse/GOS-33106) 
    TEST_F(DbConfigMergeTest, testReconfigureDbSlaveWithNonReconfigurableValues)
    {
        DatabaseConfiguration initialConfig;
        {
            auto* configFile = ConfigFile::createFromString(INITIAL_CONFIG, false);
            EXPECT_NE(configFile, nullptr);
            ConfigDecoder decoder(*configFile, true /*strictParsing*/, "databaseConnections");
            EXPECT_TRUE(decoder.decode(&initialConfig));
            delete configFile;
        }

        DatabaseConfiguration newConfig;
        {
            auto* configFile = ConfigFile::createFromString(UPDATED_CONFIG, false);
            EXPECT_NE(configFile, nullptr);
            ConfigDecoder decoder(*configFile, true /*strictParsing*/, "databaseConnections");
            EXPECT_TRUE(decoder.decode(&newConfig));
            delete configFile;
        }

        DatabaseConfiguration updatedConfig;
        EA::TDF::MemberVisitOptions copyOpts;
        copyOpts.onlyIfSet = true;
        initialConfig.copyInto(updatedConfig, copyOpts);

        ConfigMerger merger;
        EXPECT_TRUE(merger.merge(updatedConfig, newConfig));

        auto refDbConnMapItr = initialConfig.getDatabaseConnections().begin();
        for (auto dbConnMapItr = updatedConfig.getDatabaseConnections().begin(), dbConnMapEnd = updatedConfig.getDatabaseConnections().end(); dbConnMapItr != dbConnMapEnd; ++dbConnMapItr, ++refDbConnMapItr)
        {
            // Copy the ref master DB config
            DbConnConfig tempDbConnConfig;
            refDbConnMapItr->second->copyInto(tempDbConnConfig);

            // Now copy the new config's slaves.
            // We will later validate the slaves by first creating a DbConfig for the master and then iterating through the 
            // master's slaves (see below). We need to preserve change tracking info for the slaves because when we create 
            // the master DbConfig, this info is used to determine which properties a slave DbConfig needs to inherit from the master.
            EA::TDF::MemberVisitOptions opts;
            opts.onlyIfSet = true;
            dbConnMapItr->second->getSlaves().copyInto(tempDbConnConfig.getSlaves(), opts);

            // Given the above configurations we expect this check to always pass because non-reconfigurable members of a tdf (DbConnConfig) added to a reconfigurable 
            // collection (databaseConnecions["main"].slaves[1]) are already written by the ConfigMerger; their 'isset' bits are likewise all expected to be written as 'set'.
            EXPECT_TRUE(dbConnMapItr->second->getSlaves().equalsValue(tempDbConnConfig.getSlaves()));
        }
    }

} // namespace
} // namespace BlazeServerTest