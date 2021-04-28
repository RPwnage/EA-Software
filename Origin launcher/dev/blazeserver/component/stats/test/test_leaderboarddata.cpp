/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include <time.h>
#include "framework/blaze.h"
#include "framework/test/blazeunittest.h"


#define DEFAULT_SUB_ARRAY_SIZE 8

#include "stats/leaderboarddata.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
using namespace Blaze;
using namespace Blaze::Stats;

/*** Test Classes ********************************************************************************/
class TestLeaderboardData
{
public:

// TBD FIX UP TAABS!!!

    // Test the edge case of a subarray of size 1, there is no need
    // to test a subarray of size 0 as the caller should never constrcut
    // such an object
    void testTinySubArray()
    {
        SubArray<int32_t> tiny(1);
        Entry<int32_t> entry1; entry1.entityId = 1; entry1.value = 1;
        Entry<int32_t> entry2; entry2.entityId = 2; entry2.value = 2;
        tiny.insertHead(entry1);
        BUT_ASSERT(entry1 == tiny.getEntry(0));
        BUT_ASSERT(entry1 == tiny.popHead());
        tiny.insertHead(entry2);
        BUT_ASSERT(entry2 == tiny.getEntry(0));
        BUT_ASSERT(entry2 == tiny.popTail());
        tiny.insertTail(entry1);
        BUT_ASSERT(entry1 == tiny.getEntry(0));
        BUT_ASSERT(entry1 == tiny.popHead());
        tiny.insertTail(entry2);
        BUT_ASSERT(entry2 == tiny.getEntry(0));
        BUT_ASSERT(entry2 == tiny.popTail());
        tiny.insert(0, entry1);
        BUT_ASSERT(entry1 == tiny.getEntry(0));
        tiny.update(0, entry2);
        BUT_ASSERT(entry2 == tiny.getEntry(0));
        tiny.remove(0);
        tiny.insert(0, entry1);
        BUT_ASSERT(entry1 == tiny.getEntry(0));
    }

    // Test the basic operations on a subarray that has more than one element
    // to test that various insert, update, and remove operations have the
    // intended effects.  This test case exercises the public API, but makes
    // no attempt to use our knowledge ot the internal implementation details,
    // and thus does not necessarily exercise every internal code path.
    void testBasicSubArray()
    {
        SubArray<int32_t> subArray(4);
        Entry<int32_t> entry1; entry1.entityId = 1; entry1.value = 1;
        Entry<int32_t> entry2; entry2.entityId = 2; entry2.value = 2;
        Entry<int32_t> entry3; entry3.entityId = 3; entry3.value = 3;
        Entry<int32_t> entry4; entry4.entityId = 4; entry4.value = 4;
        Entry<int32_t> entry5; entry5.entityId = 5; entry5.value = 5;
        Entry<int32_t> entry6; entry6.entityId = 6; entry6.value = 6;

        // Normally when populating an array on the slave we would be
        // pushing everything in on the tail
        subArray.insertTail(entry1);
        subArray.insertTail(entry2);
        subArray.insertTail(entry3);
        subArray.insertTail(entry4);

        BUT_ASSERT(entry1 == subArray.getEntry(0));
        BUT_ASSERT(entry2 == subArray.getEntry(1));
        BUT_ASSERT(entry3 == subArray.getEntry(2));
        BUT_ASSERT(entry4 == subArray.getEntry(3));

        BUT_ASSERT(entry1 == subArray.popHead());
        BUT_ASSERT(entry2 == subArray.popHead());
        BUT_ASSERT(entry3 == subArray.popHead());
        BUT_ASSERT(entry4 == subArray.popHead());

        // Now test a mix of ways to populate and depopulate the array
        // this should populate the list in order with 1,2,3,4
        subArray.insertTail(entry2);
        subArray.insert(1, entry4);
        subArray.insert(1, entry3);
        subArray.insertHead(entry1);
        
        BUT_ASSERT(entry1 == subArray.popHead());
        BUT_ASSERT(entry4 == subArray.popTail());
        BUT_ASSERT(entry3 == subArray.getEntry(1));
        subArray.remove(1);
        BUT_ASSERT(entry2 == subArray.popHead());

        // Now let's test out the remaining couple of methods, updating
        // an entry in place, and update together with move
        subArray.insertTail(entry1);
        subArray.insertTail(entry3);
        subArray.insertTail(entry4);
        subArray.insertTail(entry6);

        subArray.update(0, entry2);
        subArray.update(3, entry5);

        BUT_ASSERT(entry2 == subArray.getEntry(0));
        BUT_ASSERT(entry3 == subArray.getEntry(1));
        BUT_ASSERT(entry4 == subArray.getEntry(2));
        BUT_ASSERT(entry5 == subArray.getEntry(3));

        // At this point we should ahve 2,3,4,5 in the list,
        // now bump the first entry down to be 1, and that leaves a gap
        // so that we can update the last entry from value 5 to 2, and
        // slide it in behind the first entry, which should leave the list
        // at 1,2,3,4
        subArray.update(0, entry1);
        subArray.update(3, 1, entry2);

        BUT_ASSERT(entry4 == subArray.popTail());
        BUT_ASSERT(entry3 == subArray.popTail());
        BUT_ASSERT(entry2 == subArray.popTail());
        BUT_ASSERT(entry1 == subArray.popTail());
    }

    // Depending on the state of a subarray, an individual insert, update, or
    // remove may shift memory torwards the head or tail, or do a shift of
    // elements in the middle.  Some moves are simple shifts, some involve wraparound,
    // so this test case aims to use our current knowledge as to the internal implementation
    // details in order to exercise each internal code path.
    void testSubArrayInternals()
    {
        SubArray<int32_t> subArray(16);
        Entry<int32_t> entry1; entry1.entityId = 1; entry1.value = 1;
        Entry<int32_t> entry2; entry2.entityId = 2; entry2.value = 2;
        Entry<int32_t> entry3; entry3.entityId = 3; entry3.value = 3;
        Entry<int32_t> entry4; entry4.entityId = 4; entry4.value = 4;
        Entry<int32_t> entry5; entry5.entityId = 5; entry5.value = 5;
        Entry<int32_t> entry6; entry6.entityId = 6; entry6.value = 6;
        Entry<int32_t> entry7; entry7.entityId = 7; entry7.value = 7;
        Entry<int32_t> entry8; entry8.entityId = 8; entry8.value = 8;
        Entry<int32_t> entry9; entry9.entityId = 9; entry9.value = 9;
        Entry<int32_t> entry10; entry10.entityId = 10; entry10.value = 10;
        Entry<int32_t> entry11; entry11.entityId = 11; entry11.value = 11;
        Entry<int32_t> entry12; entry12.entityId = 12; entry12.value = 12;
        Entry<int32_t> entry13; entry13.entityId = 13; entry13.value = 13;
        Entry<int32_t> entry14; entry14.entityId = 14; entry14.value = 14;
        Entry<int32_t> entry15; entry15.entityId = 15; entry15.value = 15;
        Entry<int32_t> entry16; entry16.entityId = 16; entry16.value = 16;

        // First setup entries 5 - 12 in order, with 5 being the head,
        // and 12 being the tail, entries 5 through 8 should be at the
        // end of the physical array and 9 through 12 at the beginning
        subArray.insertTail(entry9);
        subArray.insertTail(entry10);
        subArray.insertTail(entry11);
        subArray.insertTail(entry12);
        subArray.insertHead(entry8);
        subArray.insertHead(entry7);
        subArray.insertHead(entry6);
        subArray.insertHead(entry5);

        // Now we want to exercise inserts that insert at the head, at the tail,
        // and near the head and tail to exercise shifts in all directions
        subArray.insert(0, entry1);
        subArray.insert(1, entry4);
        subArray.insert(1, entry2);
        subArray.insert(2, entry3);
        subArray.insert(12, entry16);
        subArray.insert(12, entry13);
        subArray.insert(13, entry15);
        subArray.insert(13, entry14);

        // All 16 entries should now be in order, wrapping around in the
        // physical array right in the middle
        BUT_ASSERT(entry1 == subArray.getEntry(0));
        BUT_ASSERT(entry2 == subArray.getEntry(1));
        BUT_ASSERT(entry3 == subArray.getEntry(2));
        BUT_ASSERT(entry4 == subArray.getEntry(3));
        BUT_ASSERT(entry5 == subArray.getEntry(4));
        BUT_ASSERT(entry6 == subArray.getEntry(5));
        BUT_ASSERT(entry7 == subArray.getEntry(6));
        BUT_ASSERT(entry8 == subArray.getEntry(7));
        BUT_ASSERT(entry9 == subArray.getEntry(8));
        BUT_ASSERT(entry10 == subArray.getEntry(9));
        BUT_ASSERT(entry11 == subArray.getEntry(10));
        BUT_ASSERT(entry12 == subArray.getEntry(11));
        BUT_ASSERT(entry13 == subArray.getEntry(12));
        BUT_ASSERT(entry14 == subArray.getEntry(13));
        BUT_ASSERT(entry15 == subArray.getEntry(14));
        BUT_ASSERT(entry16 == subArray.getEntry(15));

        // Now removes some entries, exercising both the remove from
        // head and tail special cases, as well as shifting towards
        // tail and head
        subArray.remove(0);
        subArray.remove(2);
        subArray.remove(1);
        subArray.remove(0);
        subArray.remove(11);
        subArray.remove(8);
        subArray.remove(8);
        subArray.remove(8);

        // Now add a couple entries back in leaving gaps so we can
        // test some updates with moves
        subArray.insertHead(entry2);
        subArray.insertHead(entry1);
        subArray.insertTail(entry15);
        subArray.insertTail(entry16);

        // We should now have entries 1,2,5,6,7,8,9,10,11,12,15,16
        // in order, and we want to yank 15 and move it to become 4,
        // then yank 2 and move it to be 13, and we should keep the
        // list in order
        subArray.update(10, 2, entry4);
        subArray.update(1, 10, entry13);

        // Now do a couple of nearby moves, we should be starting with
        // 1,4,5,6,7,8,9,10,11,12,13,16 and move 5 to become 2, and
        // 12 to become 15
        subArray.update(2, 1, entry2);
        subArray.update(9, 10, entry15);

        // Check again that we're in good shape
        BUT_ASSERT(entry1 == subArray.getEntry(0));
        BUT_ASSERT(entry2 == subArray.getEntry(1));
        BUT_ASSERT(entry4 == subArray.getEntry(2));
        BUT_ASSERT(entry6 == subArray.getEntry(3));
        BUT_ASSERT(entry7 == subArray.getEntry(4));
        BUT_ASSERT(entry8 == subArray.getEntry(5));
        BUT_ASSERT(entry9 == subArray.getEntry(6));
        BUT_ASSERT(entry10 == subArray.getEntry(7));
        BUT_ASSERT(entry11 == subArray.getEntry(8));
        BUT_ASSERT(entry13 == subArray.getEntry(9));
        BUT_ASSERT(entry15 == subArray.getEntry(10));
        BUT_ASSERT(entry16 == subArray.getEntry(11));

        // Finally we want to test some updates across the wraparound of the
        // physical array, break should still be between 8 and 9, first add a couple
        // more elements to make the count nice and big to allow us to do inner shifts
        subArray.insert(2, entry3);
        subArray.insert(4, entry5);

        // Pluck out entry with value 10
        subArray.remove(9);
        BUT_ASSERT(entry11 == subArray.getEntry(9));

        // Now move entry with value 7 to become the new entry with value 10, but remember
        // because this removes an entry earlier in the list, we insert at index 8 now
        // rather than index 9 where we removed entry10 from in the first place
        subArray.update(6, 8, entry10);
        BUT_ASSERT(entry8 == subArray.getEntry(6));
        BUT_ASSERT(entry9 == subArray.getEntry(7));
        BUT_ASSERT(entry10 == subArray.getEntry(8));
        BUT_ASSERT(entry11 == subArray.getEntry(9));

        // Now pluck the entry with value 13 and update it to value 7 and move it
        // to the right place
        subArray.update(10, 6, entry7);
        BUT_ASSERT(entry7 == subArray.getEntry(6));
        BUT_ASSERT(entry8 == subArray.getEntry(7));
        BUT_ASSERT(entry9 == subArray.getEntry(8));
        BUT_ASSERT(entry10 == subArray.getEntry(9));
        BUT_ASSERT(entry11 == subArray.getEntry(10));
        BUT_ASSERT(entry15 == subArray.getEntry(11));
        BUT_ASSERT(entry16 == subArray.getEntry(12));

        // One last big check, pop all elements off using various head and tail pops
        // to ensure that everything is still as expected
        BUT_ASSERT(entry1 == subArray.popHead());
        BUT_ASSERT(entry2 == subArray.popHead());
        BUT_ASSERT(entry3 == subArray.popHead());
        BUT_ASSERT(entry4 == subArray.popHead());
        BUT_ASSERT(entry5 == subArray.popHead());
        BUT_ASSERT(entry6 == subArray.popHead());
        BUT_ASSERT(entry16 == subArray.popTail());
        BUT_ASSERT(entry15 == subArray.popTail());
        BUT_ASSERT(entry11 == subArray.popTail());
        BUT_ASSERT(entry10 == subArray.popTail());
        BUT_ASSERT(entry9 == subArray.popTail());
        BUT_ASSERT(entry8 == subArray.popTail());
        BUT_ASSERT(entry7 == subArray.popTail());
    }

    // Test the slave scenario where a leaderboard is populated
    // on startup on the slave, and then modified from that point
    void testLeaderboardDataPopulate()
    {
        // Create a leaderboard that will be split over a few subarrays
        // and have a smaller final subarray due to configured rows
        // and extra rows not summing up to a number evenly divisible
        // by number of rows
        int32_t configuredSize = 25;
        int32_t extraRows = 5;
        int32_t totalRows = configuredSize + extraRows;
        LeaderboardData<int32_t> data(configuredSize, extraRows, false);

        // Populate the entire leaderboard including extra rows
        for (int32_t i = 0; i < totalRows; ++i)
        {
            Entry<int32_t> entry;
            entry.entityId = i + 1;
            entry.value = totalRows - i;
            data.populate(entry);
            if (i < configuredSize)
            {
                BUT_ASSERT(static_cast<uint32_t>(i + 1) == data.getCount());
            }
            else
            {
                BUT_ASSERT(static_cast<uint32_t>(configuredSize) == data.getCount());
            }
        }

        // Check that the leaderboard is in proper order
        for (int32_t i = 0; i < totalRows; ++i)
        {
            Entry<int32_t> entry;
            entry.entityId = i + 1;
            entry.value = totalRows - i;

            // Remember, only the configured size of the leaderboard is valid ranks
            if (i < configuredSize)
            {
                BUT_ASSERT(entry == data.getEntryByRank(i + 1));
                BUT_ASSERT(static_cast<uint32_t>(i + 1) == data.getRank(entry.entityId));
            }
            else
            {
                BUT_ASSERT(0 == data.getRank(entry.entityId));
            }
        }

        // Remove all the extra rows, and then double-check that removing the last ranked entry
        // properly updates the count to configured size - 1
        for (int32_t i = configuredSize; i < totalRows; ++i)
        {
            data.remove(i + 1);
        }
        data.remove(configuredSize);
        BUT_ASSERT(static_cast<uint32_t>(configuredSize - 1) == data.getCount());
    }

    void testLeaderboardDataReal()
    {
        // Create a leaderboard that will be split over a few subarrays
        // and have a smaller final subarray due to configured rows
        // and extra rows not summing up to a number evenly divisible
        // by number of rows
        int32_t configuredSize = 25;
        int32_t extraRows = 5;
        //int32_t totalRows = configuredSize + extraRows;
        LeaderboardData<int32_t> data(configuredSize, extraRows, false);

        // To truly put the leaderboard data code through it's paces,
        // we need to perform a variety of updates and removes, and in order
        // to verify that we actually get everything right, we need to keep
        // track of what data should be in the leaderboard.  We will use a
        // simple hash map to check the validity for any given entity as to what
        // stat value it should own in the leaderboard, and to validate the overall
        // ordering of the leaderboard we may simply iterate through each element
        // comparing it to the previous one
        typedef eastl::hash_map<EntityId, int32_t> DataMap;
        DataMap dataMap;
        Entry<int32_t> entry;

        // Add a whole bunch of entries to fill up the configured size of the
        // leaderboard, with a number of updates to existing entries along the
        // way, and periodically check the count is accurate
        entry.entityId = 1; entry.value = 100;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 2; entry.value = -1;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 3; entry.value = 75;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 4; entry.value = 50;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 1; entry.value = 16;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 5; entry.value = 0;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        BUT_ASSERT(data.getCount() == 5);
        entry.entityId = 6; entry.value = 101;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 7; entry.value = 33;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 8; entry.value = 25;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 8; entry.value = 100;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 9; entry.value = 88;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 3; entry.value = 99;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 10; entry.value = 77;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        BUT_ASSERT(data.getCount() == 10);
        entry.entityId = 11; entry.value = 75;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 12; entry.value = 10;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 7; entry.value = 100;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 13; entry.value = 5;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 13; entry.value = 25;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 14; entry.value = 25;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 15; entry.value = 16;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        BUT_ASSERT(data.getCount() == 15);
        entry.entityId = 16; entry.value = 99;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 16; entry.value = -2;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 17; entry.value = 102;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 17; entry.value = -3;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 18; entry.value = 49;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 19; entry.value = 50;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 6; entry.value = 64;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 20; entry.value = 22;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        BUT_ASSERT(data.getCount() == 20);
        entry.entityId = 21; entry.value = 44;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 22; entry.value = -2;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 23; entry.value = 64;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 21; entry.value = 55;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 24; entry.value = 32;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 25; entry.value = 1;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 1; entry.value = 42;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        BUT_ASSERT(data.getCount() == 25);

        // Validate the data in the leaderboard has the correct values for each entity,
        // and that the data is also sorted in proper rank order
        for (int32_t rank = 1; rank <= configuredSize; ++rank)
        {
            Entry<int32_t> currEntry = data.getEntryByRank(rank);
            BUT_ASSERT(currEntry.value == dataMap.find(currEntry.entityId)->second);
            if (rank != configuredSize)
            {
                Entry<int32_t> nextEntry = data.getEntryByRank(rank + 1);
                BUT_ASSERT(currEntry.value > nextEntry.value || (currEntry.value == nextEntry.value && currEntry.entityId > nextEntry.entityId));
            }
        }

        // Now add enough additional entries to fill up the extra rows as well
        entry.entityId = 26; entry.value = 97;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 27; entry.value = 0;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 28; entry.value = 10;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 29; entry.value = 50;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 30; entry.value = 68;
        data.update(entry); dataMap[entry.entityId] = entry.value;

        // Count tracks only configured size, not extra rows, so should remain at 25
        BUT_ASSERT(data.getCount() == 25);

        // Validate the data in the leaderboard has the correct values for each entity,
        // and that the data is also sorted in proper rank order
        for (int32_t rank = 1; rank <= configuredSize; ++rank)
        {
            Entry<int32_t> currEntry = data.getEntryByRank(rank);
            BUT_ASSERT(currEntry.value == dataMap.find(currEntry.entityId)->second);
            if (rank != configuredSize)
            {
                Entry<int32_t> nextEntry = data.getEntryByRank(rank + 1);
                BUT_ASSERT(currEntry.value > nextEntry.value || (currEntry.value == nextEntry.value && currEntry.entityId > nextEntry.entityId));
            }
        }

        // Now update a few entries to have crappy values, which should pull
        // the better ones out of extra rows and into the main leaderboard
        entry.entityId = 4; entry.value = -6;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 9; entry.value = -7;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 14; entry.value = -10;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 19; entry.value = -10;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 24; entry.value = -100;
        data.update(entry); dataMap[entry.entityId] = entry.value;

        // Validate the data in the leaderboard has the correct values for each entity,
        // and that the data is also sorted in proper rank order
        for (int32_t rank = 1; rank <= configuredSize; ++rank)
        {
            Entry<int32_t> currEntry = data.getEntryByRank(rank);
            BUT_ASSERT(currEntry.value == dataMap.find(currEntry.entityId)->second);
            if (rank != configuredSize)
            {
                Entry<int32_t> nextEntry = data.getEntryByRank(rank + 1);
                BUT_ASSERT(currEntry.value > nextEntry.value || (currEntry.value == nextEntry.value && currEntry.entityId > nextEntry.entityId));
            }
        }

        // Now add a few more entries which should shove the worst entries right
        // out of the bottom
        entry.entityId = 31; entry.value = 111;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 32; entry.value = 23;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 33; entry.value = 49;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 34; entry.value = -4;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 35; entry.value = 2;
        data.update(entry); dataMap[entry.entityId] = entry.value;

        // Validate the data in the leaderboard has the correct values for each entity,
        // and that the data is also sorted in proper rank order
        for (int32_t rank = 1; rank <= configuredSize; ++rank)
        {
            Entry<int32_t> currEntry = data.getEntryByRank(rank);
            BUT_ASSERT(currEntry.value == dataMap.find(currEntry.entityId)->second);
            if (rank != configuredSize)
            {
                Entry<int32_t> nextEntry = data.getEntryByRank(rank + 1);
                BUT_ASSERT(currEntry.value > nextEntry.value || (currEntry.value == nextEntry.value && currEntry.entityId > nextEntry.entityId));
            }
        }

        // Now modify some existing entries, keeping some exactly the same, causing
        // others to get new values that don't change their ranks enough for them to move,
        // and finally others change a lot to jump up or down
        entry.entityId = 2; entry.value = 125;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 3; entry.value = 99;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 7; entry.value = 34;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 8; entry.value = 0;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 12; entry.value = 97;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 13; entry.value = 20;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 17; entry.value = -3;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 18; entry.value = 51;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 22; entry.value = 13;
        data.update(entry); dataMap[entry.entityId] = entry.value;
        entry.entityId = 23; entry.value = 27;
        data.update(entry); dataMap[entry.entityId] = entry.value;

        // Validate the data in the leaderboard has the correct values for each entity,
        // and that the data is also sorted in proper rank order
        for (int32_t rank = 1; rank <= configuredSize; ++rank)
        {
            Entry<int32_t> currEntry = data.getEntryByRank(rank);
            BUT_ASSERT(currEntry.value == dataMap.find(currEntry.entityId)->second);
            if (rank != configuredSize)
            {
                Entry<int32_t> nextEntry = data.getEntryByRank(rank + 1);
                BUT_ASSERT(currEntry.value > nextEntry.value || (currEntry.value == nextEntry.value && currEntry.entityId > nextEntry.entityId));
            }

            // By carefully working out the end results by hand, we know that the following
            // 10 entity ids should not be in the top 25, the first 5 should be the extra rows,
            // the latter 5 should not even be in the ranking list at all
            BUT_ASSERT(currEntry.entityId != 8);
            BUT_ASSERT(currEntry.entityId != 5);
            BUT_ASSERT(currEntry.entityId != 16);
            BUT_ASSERT(currEntry.entityId != 17);
            BUT_ASSERT(currEntry.entityId != 34);
            BUT_ASSERT(currEntry.entityId != 24);
            BUT_ASSERT(currEntry.entityId != 19);
            BUT_ASSERT(currEntry.entityId != 14);
            BUT_ASSERT(currEntry.entityId != 9);
            BUT_ASSERT(currEntry.entityId != 4);
        }

        // One final verification that the number of entries is still 25
        BUT_ASSERT(data.getCount() == 25);
    }
};

// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_LEADERBOARDDATA\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestLeaderboardData testLeaderboardData;
    testLeaderboardData.testTinySubArray();
    testLeaderboardData.testBasicSubArray();
    testLeaderboardData.testSubArrayInternals();
    testLeaderboardData.testLeaderboardDataPopulate();
    testLeaderboardData.testLeaderboardDataReal();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

