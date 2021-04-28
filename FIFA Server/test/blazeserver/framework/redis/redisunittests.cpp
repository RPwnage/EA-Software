/*************************************************************************************************/
/*!
\file

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "framework/redis/redismanager.h"
#include "framework/redis/rediskeyset.h"
#include "framework/redis/rediskeymap.h"
#include "framework/redis/rediskeylist.h"
#include "framework/redis/rediskeyfieldmap.h"
#include "framework/redis/rediskeysortedset.h"
#include "framework/redis/rediskeyhyperloglog.h"
#include "framework/redis/redistimer.h"
#include "framework/redis/redisuniqueidmanager.h"
#include "framework/tdf/frameworkredistypes_server.h"

namespace Blaze
{

    void testRedisKeySetRegistration()
    {
        RedisId testKeySetIdBig(RedisManager::UNITTESTS_NAMESPACE, "testKeySetBig");
        typedef RedisKeySet<eastl::string, eastl::string> TestKeySet;
        TestKeySet testKeySetBig(testKeySetIdBig);

        bool result;
        result = testKeySetBig.registerCollection();
        EA_ASSERT(result);
        result = testKeySetBig.registerCollection();
        EA_ASSERT(result);

        RedisId testKeySetIdBigger(RedisManager::UNITTESTS_NAMESPACE, "testKeySetBigger");
        typedef RedisKeySet<int32_t, eastl::string> TestKeySet2;
        TestKeySet2 testKeySetBigger(testKeySetIdBigger);

        result = testKeySetBigger.registerCollection();
        EA_ASSERT(result); // no issue because we no loner use the abbreviated names
    }

    void testRedisKeyFieldMap()
    {
        RedisError err;

        typedef RedisKeyFieldMap<eastl::string, eastl::string, eastl::string> TestKeyFieldMap;
        RedisId collId(RedisManager::UNITTESTS_NAMESPACE, "testKeyFieldMap");
        TestKeyFieldMap testKeyFieldMap(collId);

        int64_t result = 0;

        TestKeyFieldMap::Key k = "mykey";

        err = testKeyFieldMap.exists(k, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 0);

        TestKeyFieldMap::Field field;
        TestKeyFieldMap::Value value;
        const uint32_t MAX_FIELDS = 15;
        for (uint32_t i = 0; i < MAX_FIELDS; ++i)
        {
            field.sprintf("myfield%u", i);
            value.sprintf("myvalue%u", i);
            err = testKeyFieldMap.insert(k, field, value, &result);
            EA_ASSERT(err == REDIS_ERR_OK && result == 1);
        }

        err = testKeyFieldMap.exists(k, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        EA::TDF::TimeValue timeLeft = EA::TDF::TimeValue();
        err = testKeyFieldMap.getTimeToLive(k, timeLeft);
        EA_ASSERT(err == REDIS_ERR_NO_EXPIRY);

        EA::TDF::TimeValue in20Seconds("20s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
        err = testKeyFieldMap.expire(k, in20Seconds);
        EA_ASSERT(err == REDIS_ERR_OK);

        err = testKeyFieldMap.getTimeToLive(k, timeLeft);
        EA_ASSERT(err == REDIS_ERR_OK && timeLeft.getSec() >= 18);

        EA::TDF::TimeValue in30Seconds("30s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
        err = testKeyFieldMap.pexpire(k, in30Seconds);
        EA_ASSERT(err == REDIS_ERR_OK);

        err = testKeyFieldMap.getTimeToLive(k, timeLeft);
        EA_ASSERT(err == REDIS_ERR_OK && timeLeft.getSec() >= 28);

        TestKeyFieldMap::Value mapValue("");
        err = testKeyFieldMap.find(k, "myfield0", mapValue);
        EA_ASSERT(err == REDIS_ERR_OK);
        err = testKeyFieldMap.find(k, "myfield20", mapValue);
        EA_ASSERT(err == REDIS_ERR_NOT_FOUND);

        // scan out all the values stored in the keyfieldmap
        RedisResponsePtr scanResp;
        uint32_t count = 0;
        const char8_t* scanCursor = "0"; // cursor returned by SCAN is always a string-formatted int
        do
        {
            uint32_t pageSizeHint = 10;
            err = testKeyFieldMap.scan(k, scanResp, pageSizeHint);
            EA_ASSERT(err == REDIS_ERR_OK);
            scanCursor = scanResp->getArrayElement(REDIS_SCAN_RESULT_CURSOR).getString();
            const RedisResponse& scanArray = scanResp->getArrayElement(REDIS_SCAN_RESULT_ARRAY);
            for (uint32_t vidx = 1, len = scanArray.getArraySize(); vidx < len; vidx += 2)
            {
                bool ok;
                TestKeyFieldMap::Field fld("");
                ok = testKeyFieldMap.extractFieldArrayElement(scanArray, vidx - 1, fld);
                EA_ASSERT(ok);
                TestKeyFieldMap::Value valu("");
                ok = testKeyFieldMap.extractValueArrayElement(scanArray, vidx, valu);
                EA_ASSERT(ok);
                ++count;
            }
        } while (strcmp(scanCursor, "0") != 0);
        EA_ASSERT(count == MAX_FIELDS);

        err = testKeyFieldMap.upsert(k, "myfield0", "testvalue1");
        EA_ASSERT(err == REDIS_ERR_OK);

        TestKeyFieldMap::FieldList fieldList = { "myfield0", "myfield1" };
        err = testKeyFieldMap.upsert(k, fieldList, "testvalue2");
        EA_ASSERT(err == REDIS_ERR_OK);

        TestKeyFieldMap::FieldValueMap fieldValueMap =
        {
            { "myfield0", "testvalue3" },
            { "myfield1", "testvalue4" }
        };
        err = testKeyFieldMap.upsert(k, fieldValueMap);
        EA_ASSERT(err == REDIS_ERR_OK);
    }

    /**
    The purpose of this function is to exercise the eastl::pair<> templates and to validate the compile
    time compatibility of the RedisKeySet collection with various primitive and composite types.
    */
    void testRedisKeySet()
    {
        RedisId SomeCollection(RedisManager::UNITTESTS_NAMESPACE, "UpperAndLowerCaseMember");
        RedisId allPlayerMap(RedisManager::UNITTESTS_NAMESPACE, "allPlayerMap1");
        RedisId lowcasemember(RedisManager::UNITTESTS_NAMESPACE, "lowcasemember1");
        RedisId mLocalUserSessionCountByClientTypeAndState(RedisManager::UNITTESTS_NAMESPACE, "mLocalUserSessionCountByClientTypeAndState");

        RedisError err;
        int64_t result = 0;

        {
            typedef RedisKeySet<int32_t, eastl::string> KeySet;
            RedisId allPlayerSetId(RedisManager::UNITTESTS_NAMESPACE, "allPlayerSet1");
            KeySet allPlayerSet1(allPlayerSetId);

            KeySet::Key k(10);
            KeySet::Value v("stuff");

            err = allPlayerSet1.add(k, v);
            EA_ASSERT(err == REDIS_ERR_OK);
        }

        {
            typedef eastl::pair<int32_t, eastl::string> KeyPair;
            typedef RedisKeySet<KeyPair, int32_t> KeySet;
            RedisId allPlayerSetId(RedisManager::UNITTESTS_NAMESPACE, "allPlayerSet2");
            KeySet allPlayerSet2(allPlayerSetId);

            KeySet::Key k(25, "morestuff");
            KeySet::Value v(10);

            err = allPlayerSet2.exists(k, &result);
            EA_ASSERT(err == REDIS_ERR_OK && result == 0);

            err = allPlayerSet2.exists(k, v, &result);
            EA_ASSERT(err == REDIS_ERR_OK && result == 0);

            err = allPlayerSet2.add(k, v);
            EA_ASSERT(err == REDIS_ERR_OK);

            err = allPlayerSet2.exists(k, &result);
            EA_ASSERT(err == REDIS_ERR_OK && result == 1);

            err = allPlayerSet2.exists(k, v, &result);
            EA_ASSERT(err == REDIS_ERR_OK && result == 1);

            KeySet::Value v2(11);
            err = allPlayerSet2.add(k, v2);
            EA_ASSERT(err == REDIS_ERR_OK);

            EA::TDF::TimeValue timeLeft = EA::TDF::TimeValue();
            err = allPlayerSet2.getTimeToLive(k, timeLeft);
            EA_ASSERT(err == REDIS_ERR_NO_EXPIRY);

            EA::TDF::TimeValue in20Seconds("20s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
            err = allPlayerSet2.expire(k, in20Seconds);
            EA_ASSERT(err == REDIS_ERR_OK);

            err = allPlayerSet2.getTimeToLive(k, timeLeft);
            EA_ASSERT(err == REDIS_ERR_OK && timeLeft.getSec() >= 18);

            EA::TDF::TimeValue in30Seconds("30s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
            err = allPlayerSet2.pexpire(k, in30Seconds);
            EA_ASSERT(err == REDIS_ERR_OK);

            err = allPlayerSet2.getTimeToLive(k, timeLeft);
            EA_ASSERT(err == REDIS_ERR_OK && timeLeft.getSec() >= 28);

            eastl::vector<int32_t> members;
            err = allPlayerSet2.getMembers(k, members);
            EA_ASSERT(err == REDIS_ERR_OK);

            KeySet::Value v3;
            err = allPlayerSet2.pop(k, v3);
            EA_ASSERT(err == REDIS_ERR_OK);
        }

        {
            typedef RedisKeySet<eastl::pair<int32_t, eastl::string>, int32_t> KeySet;
            RedisId allPlayerSetId(RedisManager::UNITTESTS_NAMESPACE, "allPlayerSet3");
            KeySet allPlayerSet3(allPlayerSetId);

            KeySet::Key k(25, "morestuff");
            KeySet::Value v(10);

            err = allPlayerSet3.add(k, v);
            EA_ASSERT(err == REDIS_ERR_OK);
        }
        {
            typedef eastl::pair<int32_t, eastl::string> NestedKey;
            typedef RedisKeySet<eastl::pair<NestedKey, eastl::string>, int32_t> KeySet;
            RedisId allPlayerSetId(RedisManager::UNITTESTS_NAMESPACE, "allPlayerSet4");
            KeySet allPlayerSet4(allPlayerSetId);

            KeySet::Key k(NestedKey(25, "key2"), "key3");
            KeySet::Value v(10);

            err = allPlayerSet4.add(k, v);
            EA_ASSERT(err == REDIS_ERR_OK);
        }

        {
            typedef eastl::pair<int32_t, eastl::string> NestedKey;
            typedef RedisKeySet<eastl::pair<NestedKey, eastl::string>, int32_t> KeySet;
            RedisId allPlayerSetId(RedisManager::UNITTESTS_NAMESPACE, "allPlayerSet5");
            KeySet allPlayerSet5(allPlayerSetId);

            KeySet::Key k(NestedKey(25, "key2"), "key3");
            KeySet::Value v(10);

            err = allPlayerSet5.add(k, v);
            EA_ASSERT(err == REDIS_ERR_OK);

            k.second = "key4";
            err = allPlayerSet5.add(k, v);
            EA_ASSERT(err == REDIS_ERR_OK);

            k.second = "key5";
            err = allPlayerSet5.add(k, v);
            EA_ASSERT(err == REDIS_ERR_OK);
        }
    }

    void testRedisKeyHyperLogLog()
    {
        RedisError err;

        typedef RedisKeyHyperLogLog<eastl::string, eastl::string> KeyLog;
        RedisId hyperLogLogId(RedisManager::UNITTESTS_NAMESPACE, "hyperLog");
        KeyLog hyperLog(hyperLogLogId);

        // If we want to be able to merge keys in a multi-node cluster, those keys have to hash to the same keyslot.
        // We can force keys to hash to the same keyslot by matching the text inside {} for each key.
        // All text outside of {} will be ignored when calculating the keyslot.
        KeyLog::Key testKey1("notHashed{HLLUnitTest}one");    // HLLUnitTest keyslot/node
        KeyLog::Key testKey2("random123{HLLUnitTest}two");    // HLLUnitTest keyslot/node
        KeyLog::Key destKey("destkeyXYZ{HLLUnitTest}stuff");  // HLLUnitTest keyslot/node

        int64_t result = 0;

        // delete all 3 keys before we begin
        err = hyperLog.erase(testKey1, &result);
        EA_ASSERT(err == REDIS_ERR_OK);

        err = hyperLog.erase(testKey2, &result);
        EA_ASSERT(err == REDIS_ERR_OK);

        err = hyperLog.erase(destKey, &result);
        EA_ASSERT(err == REDIS_ERR_OK);

        // check that testKey1 doesn't exist
        err = hyperLog.exists(testKey1, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 0);

        // count members of testKey1 (doesn't exist)
        err = hyperLog.count(testKey1, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 0);   // it's okay, it just doesn't have any entries

        // create testKey1 and add entry to it
        err = hyperLog.add(testKey1, "Log 1, item 1", &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        // check that testKey1 exists
        err = hyperLog.exists(testKey1, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        // check that testKey1 doesn't have a ttl
        EA::TDF::TimeValue ttl;
        err = hyperLog.getTimeToLive(testKey1, ttl);
        EA_ASSERT(err == REDIS_ERR_NO_EXPIRY);

        // set 20s ttl on testKey1
        EA::TDF::TimeValue in20Seconds = EA::TDF::TimeValue("20s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
        err = hyperLog.expire(testKey1, in20Seconds);
        EA_ASSERT(err == REDIS_ERR_OK);

        // confirm testKey1 has ttl close to 20s
        err = hyperLog.getTimeToLive(testKey1, ttl);
        EA_ASSERT(err == REDIS_ERR_OK && ttl.getSec() > 18 && ttl.getSec() < 21);

        // set 30s ttl on testKey1
        EA::TDF::TimeValue in30Seconds = EA::TDF::TimeValue("30s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
        err = hyperLog.pexpire(testKey1, in30Seconds);
        EA_ASSERT(err == REDIS_ERR_OK);

        // confirm testKey1 has ttl close to 30s
        err = hyperLog.getTimeToLive(testKey1, ttl);
        EA_ASSERT(err == REDIS_ERR_OK && ttl.getSec() > 28 && ttl.getSec() < 31);

        // add another entry to testKey1
        err = hyperLog.add(testKey1, "Log 1, item 2", &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        // count entries in testKey1 (should be 2, but not strictly guaranteed)
        err = hyperLog.count(testKey1, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 2);

        // add entry to testKey2
        err = hyperLog.add(testKey2, "Log 2, item 1", &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        // count entries in testKey2 (should be 1, but not strictly guaranteed)
        err = hyperLog.count(testKey2, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        // count entries in destKey (must be 0, as it doesn't exist)
        err = hyperLog.count(destKey, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 0);

        // add ~1000 elements to testKey1 and testKey2
        for (int i = 0; i < 1000; i++)
        {
            err = hyperLog.add(testKey1, eastl::string(eastl::string::CtorSprintf(), "Log 1, item %d", i), &result);
            err = hyperLog.add(testKey2, eastl::string(eastl::string::CtorSprintf(), "Log 2, item %d", i), &result);
        }

        // count entries in testKey1 (1000, give or take 0.81%)
        err = hyperLog.count(testKey1, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result > 980 && result < 1020);    // 1004 testing locally

        // count entries in testKey2 (1000, give or take 0.81%)
        err = hyperLog.count(testKey2, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result > 980 && result < 1020);    // 995 testing locally

        // merge testKey1 and testKey2 into destKey
        err = hyperLog.merge(destKey, testKey1, testKey2, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        // count entries in destKey (2000, give or take 0.81%)
        err = hyperLog.count(destKey, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result > 1960 && result < 2040);   // 2004 testing locally
    }

    void testRedisKeySortedSet()
    {
        RedisError err;

        typedef RedisKeySortedSet<eastl::string, eastl::string> KeySet;
        RedisId sortedSetId(RedisManager::UNITTESTS_NAMESPACE, "sortedSetId");
        KeySet sortedSet(sortedSetId);
        KeySet::Key testKey("SortedSetUnitTest");

        int64_t result = 0;

        err = sortedSet.erase(testKey, &result);       // erase just in case
        EA_ASSERT(err == REDIS_ERR_OK);

        err = sortedSet.exists(testKey, &result);      // confirm key doesn't exist
        EA_ASSERT(err == REDIS_ERR_OK && result == 0);

        double powerLevel = 9000;
        KeySet::Value goku("Goku");
        err = sortedSet.add(testKey, goku, powerLevel, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        EA::TDF::TimeValue ttl;
        err = sortedSet.getTimeToLive(testKey, ttl);
        EA_ASSERT(err == REDIS_ERR_NO_EXPIRY);

        EA::TDF::TimeValue in20Seconds = EA::TDF::TimeValue("20s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
        EA::TDF::TimeValue in30Seconds = EA::TDF::TimeValue("30s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);

        err = sortedSet.expire("NotAKey", in30Seconds, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 0);   // 0 = key not found

        err = sortedSet.pexpire("NotAKey", in30Seconds, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 0);   // 0 = key not found

        err = sortedSet.expire(testKey, in20Seconds, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);   // 1 = ttl was updated

        err = sortedSet.getTimeToLive(testKey, ttl);
        EA_ASSERT(err == REDIS_ERR_OK && ttl.getSec() > 18);

        err = sortedSet.pexpire(testKey, in30Seconds, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);   // 1 = ttl was updated

        err = sortedSet.getTimeToLive(testKey, ttl);
        EA_ASSERT(err == REDIS_ERR_OK && ttl.getSec() > 28);

        err = sortedSet.incrementScoreBy(testKey, goku, 500, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        int insertions = 10;
        for (int i = 0; i < insertions; ++i)
        {
            eastl::string member(eastl::string::CtorSprintf(), "Member_%d", i);
            double score = static_cast<double>(i);

            err = sortedSet.add(testKey, member, score, &result);
            EA_ASSERT(err == REDIS_ERR_OK && result == 1);
        }

        int64_t setSize = 0;
        err = sortedSet.size(testKey, &setSize);
        EA_ASSERT(err == REDIS_ERR_OK && setSize == insertions + 1);

        KeySet::ValueList members;
        eastl::vector<double> scores;

        const bool ascendingOrder = false;
        const bool descendingOrder = true;
        double inf = std::numeric_limits<double>::infinity();

        // get first 4 members with non-negative scores (0 to +inf ascending order)
        err = sortedSet.getRangeByScore(testKey, 0.0, inf, ascendingOrder, 0, 4, members, &scores);
        EA_ASSERT(err == REDIS_ERR_OK && members.size() == 4);
        EA_ASSERT(members[0] == "Member_0" && scores[0] == 0.0);
        EA_ASSERT(members[1] == "Member_1" && scores[1] == 1.0);
        EA_ASSERT(members[2] == "Member_2" && scores[2] == 2.0);
        EA_ASSERT(members[3] == "Member_3" && scores[3] == 3.0);

        // same search, but descending order this time (largest 4 vs smallest 4)
        err = sortedSet.getRangeByScore(testKey, 0.0, inf, descendingOrder, 0, 4, members, &scores);
        EA_ASSERT(err == REDIS_ERR_OK && members.size() == 4);
        EA_ASSERT(members[0] == "Goku" && scores[0] == 9500.0);
        EA_ASSERT(members[1] == "Member_9" && scores[1] == 9.0);
        EA_ASSERT(members[2] == "Member_8" && scores[2] == 8.0);
        EA_ASSERT(members[3] == "Member_7" && scores[3] == 7.0);

        // everything between 0.0 and 4.0 (exclusive of min and max scores)
        err = sortedSet.getRangeByScore(testKey, 0.0, 4.0, ascendingOrder, 0, 4, members, &scores, true, true);
        EA_ASSERT(err == REDIS_ERR_OK && members.size() == 3);
        EA_ASSERT(members[0] == "Member_1" && scores[0] == 1.0);
        EA_ASSERT(members[1] == "Member_2" && scores[1] == 2.0);
        EA_ASSERT(members[2] == "Member_3" && scores[2] == 3.0);

        // remove 5 highest scores (Member_6, Member_7, Member_8, Member_9 and Goku)
        int64_t numToRemove = 5;
        err = sortedSet.removeRange(testKey, -numToRemove, -1, &result);   // negative starts at end of set (-1 == last item, -5 == 5 from end)
        EA_ASSERT(err == REDIS_ERR_OK && result == numToRemove);

        err = sortedSet.size(testKey, &setSize);
        EA_ASSERT(err == REDIS_ERR_OK && setSize == (insertions + 1) - numToRemove);

        err = sortedSet.add(testKey, "Positive Infinity", inf, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        err = sortedSet.add(testKey, "Negative Infinity", -inf, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        double max = std::numeric_limits<double>::max();
        err = sortedSet.add(testKey, "Maximum Positive", max, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        err = sortedSet.add(testKey, "Maximum Negative", -max, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        double min = std::numeric_limits<double>::min();
        err = sortedSet.add(testKey, "Minimum Positive", min, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        err = sortedSet.add(testKey, "Minimum Negative", -min, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        err = sortedSet.add(testKey, "Approximate pi", (22.0 / 7.0), &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        // get the rank of a member from low to high (0 based)
        err = sortedSet.rank(testKey, "Member_0", ascendingOrder, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 3);

        // get the rank of a member from high to low (0 based)
        err = sortedSet.rank(testKey, "Member_0", descendingOrder, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 9);

        // get the rank of a member that doesn't exist
        err = sortedSet.rank(testKey, "Nothing", ascendingOrder, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == -1);  // -1 doesn't exist

        // get the rank of a key that doesn't exist
        err = sortedSet.rank("NotAKey", "Nothing", ascendingOrder, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == -1);  // -1 doesn't exist

        double scoreFromRedis = 0.0;
        err = sortedSet.getScore(testKey, "Minimum Positive", scoreFromRedis);
        EA_ASSERT(err == REDIS_ERR_OK && scoreFromRedis == min);

        err = sortedSet.getScore(testKey, "Approximate pi", scoreFromRedis);
        EA_ASSERT(err == REDIS_ERR_OK && scoreFromRedis > 3.14 && scoreFromRedis < 3.15);

        err = sortedSet.getScore(testKey, "Negative Infinity", scoreFromRedis);
        EA_ASSERT(err == REDIS_ERR_OK && scoreFromRedis == -inf);

        // request score for member that doesn't exist
        err = sortedSet.getScore(testKey, "Nothing", scoreFromRedis);
        EA_ASSERT(err == REDIS_ERR_NOT_FOUND);

        // request score for key that doesn't exist
        err = sortedSet.getScore("NotAKey", "Nothing", scoreFromRedis);
        EA_ASSERT(err == REDIS_ERR_NOT_FOUND);

        // the only invalid score for a sorted set member is nan (should log an error when attempting to add)
        double nan = std::numeric_limits<double>::quiet_NaN();
        err = sortedSet.add(testKey, "Not a number", nan, &result);
        EA_ASSERT(err == REDIS_ERR_COMMAND_FAILED);

        // get 10 members with lowest scores (ascending order) WITH scores
        err = sortedSet.getRange(testKey, 0, 9, ascendingOrder, members, &scores);
        EA_ASSERT(err == REDIS_ERR_OK && members.size() == 10 && scores.size() == 10);

        EA_ASSERT(members[0] == "Negative Infinity" && scores[0] == -inf);
        EA_ASSERT(members[1] == "Maximum Negative" && scores[1] == -max);
        EA_ASSERT(members[2] == "Minimum Negative" && scores[2] == -min);
        EA_ASSERT(members[3] == "Member_0" && scores[3] == 0.0);
        EA_ASSERT(members[4] == "Minimum Positive" && scores[4] == min);
        EA_ASSERT(members[5] == "Member_1" && scores[5] == 1.0);
        EA_ASSERT(members[6] == "Member_2" && scores[6] == 2.0);
        EA_ASSERT(members[7] == "Member_3" && scores[7] == 3.0);
        EA_ASSERT(members[8] == "Approximate pi" && scores[8] > 3.14 && scores[8] < 3.15);
        EA_ASSERT(members[9] == "Member_4" && scores[9] == 4.0);

        // get 10 members with highest scores (descending order) WITHOUT scores
        err = sortedSet.getRange(testKey, 0, 9, descendingOrder, members);
        EA_ASSERT(err == REDIS_ERR_OK && members.size() == 10);

        EA_ASSERT(members[0] == "Positive Infinity");
        EA_ASSERT(members[1] == "Maximum Positive");
        EA_ASSERT(members[2] == "Member_5");
        EA_ASSERT(members[3] == "Member_4");
        EA_ASSERT(members[4] == "Approximate pi");
        EA_ASSERT(members[5] == "Member_3");
        EA_ASSERT(members[6] == "Member_2");
        EA_ASSERT(members[7] == "Member_1");
        EA_ASSERT(members[8] == "Minimum Positive");
        EA_ASSERT(members[9] == "Member_0");

        // remove Member_0
        err = sortedSet.remove(testKey, "Member_0", &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        // attempt to remove Member_0 again (result = 0 since it no longer exists)
        err = sortedSet.remove(testKey, "Member_0", &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 0);

        // remove previously fetched list of members
        err = sortedSet.remove(testKey, members, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 9);  // only removed 9 because Member_0 no longer existed

        // delete the set
        err = sortedSet.erase(testKey, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        // verify set no longer exists
        err = sortedSet.exists(testKey, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 0);
    }

    void testRedisKeyMap()
    {
        RedisError err;

        {
            typedef RedisKeyMap<eastl::string, int32_t> KeyMap;
            RedisId allPlayerMapId(RedisManager::UNITTESTS_NAMESPACE, "allPlayerMap1");
            KeyMap allPlayerMap1(allPlayerMapId, false);

            KeyMap::Key k("stuff");

            int64_t result = 0;

            err = allPlayerMap1.exists(k, &result);
            EA_ASSERT(err == REDIS_ERR_OK && result == 0);

            err = allPlayerMap1.incr(k, &result);
            EA_ASSERT(err == REDIS_ERR_OK && result == 1);

            err = allPlayerMap1.exists(k, &result);
            EA_ASSERT(err == REDIS_ERR_OK && result == 1);

            err = allPlayerMap1.incr(k, &result);
            EA_ASSERT(err == REDIS_ERR_OK && result == 2);

            err = allPlayerMap1.decr(k, &result);
            EA_ASSERT(err == REDIS_ERR_OK && result == 1);

            //err = allPlayerMap1.decr(k, &result);
            //EA_ASSERT(err == REDIS_ERR_OK && result == 0);

            bool removed = false;
            err = allPlayerMap1.decrRemoveWhenZero(k, &removed);
            EA_ASSERT(err == REDIS_ERR_OK && removed);

            KeyMap::Value v(0);
            err = allPlayerMap1.find(k, v);
            EA_ASSERT(err == REDIS_ERR_NOT_FOUND);

            k = "morestuff";
            v = 1;

            err = allPlayerMap1.insert(k, v);
            EA_ASSERT(err == REDIS_ERR_OK);

            EA::TDF::TimeValue timeLeft = EA::TDF::TimeValue();
            err = allPlayerMap1.getTimeToLive(k, timeLeft);
            EA_ASSERT(err == REDIS_ERR_NO_EXPIRY);

            EA::TDF::TimeValue in20Seconds("20s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
            err = allPlayerMap1.expire(k, in20Seconds);
            EA_ASSERT(err == REDIS_ERR_OK);

            err = allPlayerMap1.getTimeToLive(k, timeLeft);
            EA_ASSERT(err == REDIS_ERR_OK && timeLeft.getSec() >= 18);

            EA::TDF::TimeValue in30Seconds("30s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
            err = allPlayerMap1.pexpire(k, in30Seconds);
            EA_ASSERT(err == REDIS_ERR_OK);

            err = allPlayerMap1.getTimeToLive(k, timeLeft);
            EA_ASSERT(err == REDIS_ERR_OK && timeLeft.getSec() >= 28);


            KeyMap::Value v2(0);
            err = allPlayerMap1.find(k, v2);
            EA_ASSERT(err == REDIS_ERR_OK && v2 == 1);

            err = allPlayerMap1.erase(k);
            EA_ASSERT(err == REDIS_ERR_OK);

            v2 = 0;
            err = allPlayerMap1.find(k, v2);
            EA_ASSERT(err == REDIS_ERR_NOT_FOUND);
        }
    }

    void testRedisKeyList()
    {
        RedisError err;

        typedef RedisKeyList<eastl::string, eastl::string> KeyList;
        RedisId fruitsListId(RedisManager::UNITTESTS_NAMESPACE, "fruitsList");
        KeyList fruitsList(fruitsListId);

        KeyList::Key k("fruits");
        int64_t result = 0;

        err = fruitsList.exists(k, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 0);  // 0: fruits doesn't exist

        eastl::vector<eastl::string> newFruit;
        newFruit.push_back("banana");
        newFruit.push_back("cherry");

        err = fruitsList.pushFront(k, newFruit, &result);    // banana, cherry
        EA_ASSERT(err == REDIS_ERR_OK && result == 2);

        eastl::vector<eastl::string> results;
        err = fruitsList.getRange(k, 0, 1, results);
        EA_ASSERT(err == REDIS_ERR_OK && results.size() == 2 && results[0].compare("banana") == 0 && results[1].compare("cherry") == 0);

        err = fruitsList.exists(k, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);  // 1: fruits exists

        err = fruitsList.pushBack(k, "date", &result);  // banana, cherry, date
        EA_ASSERT(err == REDIS_ERR_OK && result == 3);

        err = fruitsList.pushFront(k, "apple", &result);  // apple, banana, cherry, date
        EA_ASSERT(err == REDIS_ERR_OK && result == 4);

        EA::TDF::TimeValue timeLeft = EA::TDF::TimeValue();
        err = fruitsList.getTimeToLive(k, timeLeft);
        EA_ASSERT(err == REDIS_ERR_NO_EXPIRY);

        EA::TDF::TimeValue in20Seconds("20s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
        err = fruitsList.expire(k, in20Seconds);
        EA_ASSERT(err == REDIS_ERR_OK);

        err = fruitsList.getTimeToLive(k, timeLeft);
        EA_ASSERT(err == REDIS_ERR_OK && timeLeft.getSec() >= 18);

        EA::TDF::TimeValue ttl("10s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
        err = fruitsList.pexpire(k, ttl);
        EA_ASSERT(err == REDIS_ERR_OK);

        err = fruitsList.getTimeToLive(k, timeLeft);
        EA_ASSERT(err == REDIS_ERR_OK && timeLeft.getSec() >= 8 && ttl.getSec() < 11);

        eastl::string fruit;
        err = fruitsList.get(k, 1, fruit);
        EA_ASSERT(err == REDIS_ERR_OK && fruit.compare("banana") == 0);

        err = fruitsList.set(k, 1, "blueberry", &result);  // apple, blueberry, cherry, date
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        err = fruitsList.get(k, 1, fruit);
        EA_ASSERT(err == REDIS_ERR_OK && fruit.compare("blueberry") == 0);

        err = fruitsList.get(k, 12, fruit);        // out of bounds
        EA_ASSERT(err == REDIS_ERR_NOT_FOUND);

        err = fruitsList.getRange(k, 0, 1, results);  // get 0 through 1
        EA_ASSERT(err == REDIS_ERR_OK && results.size() == 2 && results[0].compare("apple") == 0 && results[1].compare("blueberry") == 0);

        err = fruitsList.pushBack(k, "elderberry", &result);  // apple, blueberry, cherry, date, elderberry
        EA_ASSERT(err == REDIS_ERR_OK && result == 5);

        err = fruitsList.trim(k, 1, 3, &result);    // trim everything except index 1 through 3 (blueberry, cherry, date)
        EA_ASSERT(err == REDIS_ERR_OK && result == 1);

        err = fruitsList.getRange(k, 0, -1, results);    // get all items (-1 is the last item)
        EA_ASSERT(err == REDIS_ERR_OK && results.size() == 3 && results[0].compare("blueberry") == 0 &&
            results[1].compare("cherry") == 0 && results[2].compare("date") == 0);

        err = fruitsList.popFront(k, fruit);  // cherry, date
        EA_ASSERT(err == REDIS_ERR_OK && fruit.compare("blueberry") == 0);

        err = fruitsList.popBack(k, fruit);  // cherry
        EA_ASSERT(err == REDIS_ERR_OK && fruit.compare("date") == 0);

        err = fruitsList.popBack(k, fruit);  // empty
        EA_ASSERT(err == REDIS_ERR_OK && fruit.compare("cherry") == 0);

        err = fruitsList.popFront(k, fruit);
        EA_ASSERT(err == REDIS_ERR_NOT_FOUND);

        err = fruitsList.popBack(k, fruit);
        EA_ASSERT(err == REDIS_ERR_NOT_FOUND);

        err = fruitsList.exists(k, &result);
        EA_ASSERT(err == REDIS_ERR_OK && result == 0);  // 0: fruits doesn't exist

        err = fruitsList.getTimeToLive(k, timeLeft);
        EA_ASSERT(err == REDIS_ERR_NOT_FOUND);  // if an item is pushed now, the list won't have the previous ttl (it will be a new list)
    }

    static const int TEST_VALUE = 77;

    void timerHandler2(int value)
    {
        EA_ASSERT(value == TEST_VALUE);
    }

    void testRedisTimer()
    {
        RedisTimerId timerId;
        EA::TDF::TimeValue expireAt;
        EA::TDF::TimeValue offset("5s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
        RedisId myTimerHandler(RedisManager::UNITTESTS_NAMESPACE, "MyTimer2");
        typedef RedisTimer<int> MyTimer;
        MyTimer timer(myTimerHandler, false);
        MyTimer::TimerCb callback(&timerHandler2);

        bool result = timer.registerHandler(callback);
        EA_ASSERT(result);
        result = timer.syncTimers();
        EA_ASSERT(result);

        expireAt = EA::TDF::TimeValue::getTimeOfDay() + offset;
        timerId = timer.scheduleTimer(expireAt, TEST_VALUE);
        EA_ASSERT(timerId != INVALID_REDIS_TIMER_ID);

        BlazeRpcError rc;
        rc = gSelector->sleep(offset + 6000000LL);
        EA_ASSERT(rc == ERR_OK);

        // schedule one more timer that we will not let expire
        expireAt = EA::TDF::TimeValue::getTimeOfDay() + offset;
        timerId = timer.scheduleTimer(expireAt, TEST_VALUE);
        EA_ASSERT(timerId != INVALID_REDIS_TIMER_ID);

        result = timer.deregisterHandler();
        EA_ASSERT(result);
    }

    void testRedisParseValue()
    {
        // FIXME: This is broken now, because string encode uses ':' delimiter while decodeValue() uses '\0' delimiters
        // The solution is to create a separate RedisEncoder that can be used to write data to a IStream/RawBuffer and test with that instead of StringBuilder
#if 0
        StringBuilder sb;
        eastl::pair<int, int64_t> redisPair(50, 75777);
        sb << redisPair;

        eastl::pair<int, int64_t> redisPair2(0, 0);
        RedisDecoder::decodeValue(sb.get(), sb.get() + sb.length(), redisPair2);

        typedef eastl::pair<int, int64_t> X;
        typedef eastl::pair<int, eastl::string> Y;
        typedef eastl::pair<eastl::string, Y> Z;
        typedef eastl::pair<X, Z> A;

        sb.reset();

        A a(X(1, 2), Z("str_i", Y(3, "str_ii")));
        sb << a;
        A a2(X(0, 0), Z("", Y(0, "")));
        RedisDecoder::decodeValue(sb.get(), sb.get() + sb.length(), a2);
#endif
    }


    void testRedisInsertTdfValue()
    {
        RedisError err;

        typedef RedisKeyMap<eastl::string, Blaze::RedisClusterStatus> KeyMap;
        RedisId redisClusterMapId(RedisManager::UNITTESTS_NAMESPACE, "redisClusterMap");
        KeyMap redisClusterMap(redisClusterMapId, false);
        KeyMap::Key k("foo");

        KeyMap::Value v;
        err = redisClusterMap.find(k, v);
        EA_ASSERT(err == REDIS_ERR_NOT_FOUND);
        v.setBytesRecv(42);
        v.setBytesSent(42);
        err = redisClusterMap.insert(k, v);
        EA_ASSERT(err == REDIS_ERR_OK);
        KeyMap::Value v2;
        err = redisClusterMap.find(k, v2);
        EA_ASSERT(err == REDIS_ERR_OK);
        err = redisClusterMap.erase(k);
        EA_ASSERT(err == REDIS_ERR_OK);
        err = redisClusterMap.find(k, v2);
        EA_ASSERT(err == REDIS_ERR_NOT_FOUND);
    }

    void testRedisUniqueId()
    { 
        RedisId myUniqueId1(RedisManager::UNITTESTS_NAMESPACE, "MyUniqueId1");
        RedisId myUniqueId2(RedisManager::UNITTESTS_NAMESPACE, "MyUniqueId2");
        RedisUniqueIdManager& uidMgr = gRedisManager->getUniqueIdManager();
        bool result = uidMgr.registerUniqueId(myUniqueId1);
        EA_ASSERT(result);
        result = uidMgr.registerUniqueId(myUniqueId2);
        EA_ASSERT(result);
        const uint64_t offset = 10;
        for (RedisUniqueId i = 1; i <= offset; ++i)
        {
            RedisUniqueId uid = uidMgr.getNextUniqueId(myUniqueId1);
            EA_ASSERT(i == uid);
        }

        for (RedisUniqueId i = offset + 1; i <= RedisUniqueIdManager::ID_RESERVATION_RANGE; ++i)
        {
            RedisUniqueId uid1 = uidMgr.getNextUniqueId(myUniqueId1);
            RedisUniqueId uid2 = uidMgr.getNextUniqueId(myUniqueId2);
            EA_ASSERT(i == uid1 && i == uid2 + offset);
        }

        result = uidMgr.deregisterUniqueId(myUniqueId1);
        EA_ASSERT(result);
        RedisUniqueId uid = uidMgr.getNextUniqueId(myUniqueId1);
        EA_ASSERT(uid == INVALID_REDIS_UNIQUE_ID);
        uid = uidMgr.getNextUniqueId(myUniqueId2);
        EA_ASSERT(uid == (RedisUniqueIdManager::ID_RESERVATION_RANGE - offset + 1));
    }

    static const uint32_t LEVEL_NAME_SIZE = 32;
    static const uint32_t NUM_LEVELS = 4;

    struct ScanDataLevel
    {
        int count;
        int index;
        const char8_t *prefix;
        char8_t buf[LEVEL_NAME_SIZE];
    };


    void testMassiveKeyPopulation()
    {
        // This populates 10M records into Redis, takes about 7 min,
        // NOTE: The server must be started with --maxheap 10G, using 1G caused a crash:
        // after inserting 7.63M keys:
        // VirtualAlloc/COWAlloc fail!
        //    [716] 17 Dec 10:35:37.327 # Out Of Memory allocating 16 bytes!

        ScanDataLevel levels[NUM_LEVELS] = { { 10, 0, "level0.%d", "" },{ 10, 0, "level1.%d", "" },{ 1000, 0, "level2.%d", "" },{ 100, 0, "level3.%d", "" } };

        RedisCommand cmd;

        int32_t level = 0;
        int32_t count = 0;
        int32_t batchsize = 10000;

        int32_t totalCount = 1;
        for (int32_t i = 0; i < (int32_t)NUM_LEVELS; ++i)
            totalCount *= levels[i].count;

        EA::TDF::TimeValue startTime = EA::TDF::TimeValue::getTimeOfDay();
        EA::TDF::TimeValue endTime = startTime;
        BLAZE_INFO_LOG(Log::SYSTEM, "MSET begin");
        cmd.add("MSET");
        for (; levels[level].index < levels[level].count; levels[level].index++)
        {
            blaze_snzprintf(levels[level].buf, LEVEL_NAME_SIZE, levels[level].prefix, levels[level].index);
            level++;
            for (; levels[level].index < levels[level].count; levels[level].index++)
            {
                blaze_snzprintf(levels[level].buf, LEVEL_NAME_SIZE, levels[level].prefix, levels[level].index);
                level++;
                for (; levels[level].index < levels[level].count; levels[level].index++)
                {
                    blaze_snzprintf(levels[level].buf, LEVEL_NAME_SIZE, levels[level].prefix, levels[level].index);
                    level++;
                    for (; levels[level].index < levels[level].count; levels[level].index++)
                    {
                        blaze_snzprintf(levels[level].buf, LEVEL_NAME_SIZE, levels[level].prefix, levels[level].index);
                        cmd.begin();
                        for (int32_t i = 0; i < (int32_t)NUM_LEVELS; ++i)
                        {
                            cmd.add(levels[i].buf);
                            cmd.add('\0');
                        }
                        cmd.end();
                        cmd.add(count++);
                        cmd.setNamespace(RedisManager::UNITTESTS_NAMESPACE);
                        if (count % batchsize == 0)
                        {
                            RedisResponsePtr resp;
                            RedisError rc = gRedisManager->exec(cmd, resp);
                            EA_ASSERT(rc == REDIS_ERR_OK);
                            cmd.reset();
                            cmd.add("MSET");
                            EA::TDF::TimeValue now = EA::TDF::TimeValue::getTimeOfDay();
                            double delta = (now - startTime).getMicroSeconds() / 1000000.0f;
                            double eta = (totalCount - count)*(delta / count);
                            BLAZE_INFO_LOG(Log::SYSTEM, "MSET count = " << count << ", run time = " << delta << " sec, eta = " << eta / 60 << " min.");
                            endTime = EA::TDF::TimeValue::getTimeOfDay();
                        }
                    }
                    levels[level--].index = 0;
                }
                levels[level--].index = 0;
            }
            levels[level--].index = 0;
        }
        endTime = EA::TDF::TimeValue::getTimeOfDay();
        BLAZE_INFO_LOG(Log::SYSTEM, "MSET end, total time = " << (endTime - startTime).getMicroSeconds() / 1000000.0f / 60.0f << " min, records written = " << count);
    }

    void testMassiveKeyScan()
    {
        // This reads back 1000 matching elements from the 10M records populated earlier, it takes about 10s with a COUNT parameter set to 100000 and each request taking about 0.08s

        ScanDataLevel levels[NUM_LEVELS] = { { 10, 9, "level0.%d", "" },{ 10, 6, "level1.%d", "" },{ 1000, 0, "level2.*", "" },{ 100, 55, "level3.%d", "" } };
        RedisCommand cmd;
        RedisResponsePtr resp;
        const char8_t* cursor = "0"; // NOTE: Cursor returned by SCAN is always a string-formatted int
        uint32_t countElements = 0;
        uint32_t countReq = 0;
        EA::TDF::TimeValue startTime = EA::TDF::TimeValue::getTimeOfDay();
        EA::TDF::TimeValue endTime = startTime;
        BLAZE_INFO_LOG(Log::SYSTEM, "SCAN begin");
        do
        {
            cmd.reset();
            cmd.add("SCAN");
            cmd.add(cursor);
            cmd.add("MATCH");

            int32_t level = 0;
            cmd.begin();
            blaze_snzprintf(levels[level].buf, LEVEL_NAME_SIZE, levels[level].prefix, levels[level].index);
            cmd.add(levels[level].buf);
            cmd.add('\0');
            level++;
            blaze_snzprintf(levels[level].buf, LEVEL_NAME_SIZE, levels[level].prefix, levels[level].index);
            cmd.add(levels[level].buf);
            cmd.add('\0');
            level++;
            blaze_snzprintf(levels[level].buf, LEVEL_NAME_SIZE, levels[level].prefix);
            cmd.add(levels[level].buf);
            cmd.add('\0');
            level++;
            blaze_snzprintf(levels[level].buf, LEVEL_NAME_SIZE, levels[level].prefix, levels[level].index);
            cmd.add(levels[level].buf);
            cmd.add('\0');
            cmd.end();
            // NOTE: Count is a *hint* that governs how much 'work' the 'stride' length for each redis scan request, 
            // if the amount of data returned is 0, it is best to double the COUNT adaptively (until about 100000 elements)
            // to reduce the number of requests to redis and to enable each request to scan more elements before returning
            // any results. In other words, COUNT is *not* the number of elements to return, but the number of elements to 'visit'.
            // Example: when selecting a 1000 subset of 10M elements [populated with testMassiveKeyPopulation()] using the COUNT = 100000 
            // returns the results in 100 requests, which takes about 10 seconds. 
            // Without using COUNT, the default COUNT value of (10) is used which results in 1M requests
            // which takes about 11 minutes, to return the same 1000 elements!
            cmd.add("COUNT");
            cmd.add(100000);


            endTime = EA::TDF::TimeValue::getTimeOfDay();
            RedisError rc = gRedisManager->exec(cmd, resp);
            countReq++;
            if (rc == REDIS_ERR_OK && resp->isArray() && resp->getArraySize() == 2)
            {
                const RedisResponse& arr = resp->getArrayElement(1);
                if (!arr.isArray())
                    return;
                uint32_t arraySize = arr.getArraySize();
                countElements += arraySize;
                const RedisResponse& cursorResp = resp->getArrayElement(0);
                EA_ASSERT(cursorResp.isString());
                cursor = cursorResp.getString();
                EA::TDF::TimeValue now = EA::TDF::TimeValue::getTimeOfDay();
                BLAZE_INFO_LOG(Log::SYSTEM, "SCAN fetched = " << arraySize << ", next_cursor = " << cursor << ", fetch time = " << (now - endTime).getMicroSeconds() / 1000000.0f << " sec");
                endTime = now;
            }
            else
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "ERROR! ");
                break;
            }
        } while (strcmp(cursor, "0") != 0);

        endTime = EA::TDF::TimeValue::getTimeOfDay();
        BLAZE_INFO_LOG(Log::SYSTEM, "SCAN end, total time = " << (endTime - startTime).getMicroSeconds() / 1000000.0f << " sec, records read = " << countElements << ", requests = " << countReq);
    }

    void testRedis()
    {

#if 0
        testRedisUniqueId();
        testRedisInsertTdfValue();
        testRedisParseValue();
        testRedisKeySetRegistration();
        testRedisKeyFieldMap();
        testRedisKeyMap();
        testRedisKeyList();
        testRedisKeySet();
        testRedisKeyHyperLogLog();
        testRedisKeySortedSet();
        testRedisTimer();
        testMassiveKeyPopulation();
        testMassiveKeyScan();
#endif

    }

} // Blaze