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
#include "framework/util/lrucache.h"
#include "framework/system/identity.h"
#include "framework/test/blazeunittest.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
using namespace Blaze;

/*** Test Classes ********************************************************************************/
class TestLruCache
{
public:
    class Value
    {
    public:
        Value(int32_t val) : mValue(val) { }

        int32_t getValue() { return mValue; }

    private:
        int32_t mValue;
    };

    class DeleteValue
    {
    public:
        DeleteValue(int32_t val) : mValue(val), mDeleted(false) { }

        int32_t getValue() { return mValue; }

        void* operator new(size_t size)
        {
            return malloc(size);
        }

        void operator delete(void* ptr, size_t size)
        {
            DeleteValue* v = reinterpret_cast<DeleteValue*>(ptr);
            v->mDeleted = true;
        }

        void release()
        {
            free(this);
        }

        bool isDeleted() const
        {
            return mDeleted;
        }

    private:
        int32_t mValue;
        bool mDeleted;
    };
    
    TestLruCache()
    {
    }

    // Test to see if pushing elements onto the cache beyond the max size will invalidate
    // old entries
    void test1()
    {
        mTestRunning = true;
        typedef LruCache<int32_t, Value> ValueCache;
        ValueCache cache(32, ValueCache::RemoveCb(this, &TestLruCache::test1_removeCb));

        for(int32_t i = 0; i < 40; i++)
            cache.add(i, new Value(i));
        mTestRunning = false;
    }

    // Test if accessing items will push them to the head of the accessed queue and prevent them
    // from being expired
    void test2()
    {
        mTestRunning = true;
        typedef LruCache<int32_t, Value> ValueCache;
        ValueCache cache(20, ValueCache::RemoveCb(this, &TestLruCache::test2_removeCb));

        for(int32_t i = 0; i < 20; i++)
            cache.add(i, new Value(i));

        // Access some cache elements from the middle the ensure they are bumped to the head
        for(int32_t i = 5; i < 10; i++)
        {
            const Value* v = cache.get(i);
            BUT_ASSERT_MSG("test2", v != nullptr);
        }

        // Now add more elements to push the old ones off.  Items 5 to 10 should not be bumped
        for(int32_t i = 20; i < 35; i++)
        {
            cache.add(i, new Value(i));
        }
        mTestRunning = false;
    }

    // Test explictly removing elements from the cache
    void test3()
    {
        mTestRunning = true;
        typedef LruCache<int32_t, Value> ValueCache;
        ValueCache cache(20, ValueCache::RemoveCb(this, &TestLruCache::test3_removeCb));

        for(int32_t i = 0; i < 20; i++)
            cache.add(i, new Value(i));

        bool result;

        // Remove an element in the middle
        mRemoveElement = 5;
        result = cache.remove(mRemoveElement);
        BUT_ASSERT_MSG("test3", result);

        // Remove the last element
        mRemoveElement = 0;
        result = cache.remove(mRemoveElement);
        BUT_ASSERT_MSG("test3", result);

        // Remove the first element
        mRemoveElement = 19;
        result = cache.remove(mRemoveElement);
        BUT_ASSERT_MSG("test3", result);

        mTestRunning = false;
    }

    // Test that a cache can be created without a remove callback and that the objects will be
    // deleted properly
    void test4()
    {
        typedef LruCache<int32_t, DeleteValue> ValueCache;
        ValueCache cache(20);

        DeleteValue* v1 = new DeleteValue(0);
        BUT_ASSERT_MSG("test4", !v1->isDeleted());
        cache.add(v1->getValue(), v1);
        cache.remove(v1->getValue());
        BUT_ASSERT_MSG("test4", v1->isDeleted());
        v1->release();
    }


private:
    bool mTestRunning;
    int32_t mRemoveElement;

    void test1_removeCb(const int32_t &key, const Value* value)
    {
        if (mTestRunning)
            BUT_ASSERT_MSG("test1", key < 8);
        delete value;
    }

    void test2_removeCb(const int32_t &key, const Value* value)
    {
        if (mTestRunning)
            BUT_ASSERT_MSG("test2", (key < 5) || (key >= 10));
        delete value;
    }

    void test3_removeCb(const int32_t &key, const Value* value)
    {
        if (mTestRunning)
        {
            BUT_ASSERT_MSG("test3", key == mRemoveElement);
        }
        else
        {
            // Fail if any of the removed elements still exist when the cache is destroyed
            BUT_ASSERT_MSG("test3", (key != 5) && (key != 0) && (key != 19));
        }
        delete value;
    }
};



// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_LRUCACHE\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestLruCache testLruCache;
    testLruCache.test1();
    testLruCache.test2();
    testLruCache.test3();
    testLruCache.test4();
     
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

