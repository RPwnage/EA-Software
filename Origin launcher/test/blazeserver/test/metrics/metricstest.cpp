/*************************************************************************************************/
/*!
\file metricstest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "metrics/metricstest.h"

#include "EATDF/time.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace Metrics
{
    namespace Tag
    {
        TagInfo<const char*>* tag1 = BLAZE_NEW TagInfo<const char*>("tag1");
        TagInfo<const char*>* tag2 = BLAZE_NEW TagInfo<const char*>("tag2");
        TagInfo<const char*>* tag3 = BLAZE_NEW TagInfo<const char*>("tag3");
    }
}
}

namespace BlazeServerTest
{
namespace Metrics 
{

    TEST_F(MetricsTest, FiltersCheck)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::OperationalInsightsExportConfig oiConfig;

        // Build some includes and excludes:
        Blaze::MetricFilterList* list = &oiConfig.getInclude();
        list->pull_back()->setRegex("^uptime");
        list->pull_back()->setRegex("^fibers.cpuUsageForProcessPercent");
        list->pull_back()->setRegex("^util.stt*");

        list = &oiConfig.getExclude();
        list->pull_back()->setRegex("^uptime.whatsthat");   // Exclude specific value
        list->pull_back()->setRegex("^fibers");             // Excludes all fibers values including cpuUsageForProcessPercent
        list->pull_back()->setRegex("^util.sttTimeToGo*");  // Excludes all util values sttTimeToGo
        
        Blaze::Metrics::Counter testCounter1(collection, "uptime");
        Blaze::Metrics::Counter testCounter2(collection, "uptime.whatsthat");
        Blaze::Metrics::Counter testCounter3(collection, "downtime");
        Blaze::Metrics::Counter testCounter4(collection, "fibers");
        Blaze::Metrics::Counter testCounter5(collection, "fibers.cpuUsageForProcessPercent");
        Blaze::Metrics::Counter testCounter6(collection, "util");
        Blaze::Metrics::Counter testCounter7(collection, "util.stt");
        Blaze::Metrics::Counter testCounter8(collection, "util.sttEventCount");
        Blaze::Metrics::Counter testCounter9(collection, "util.sttTimeToGo");
        Blaze::Metrics::Counter testCounter10(collection, "util.sttTimeToGoHome");

        // Clear export interval so that we don't actually try to export, just update the export flag.  (exportMetrics doesn't actually check the oiConfig for this)
        const_cast<EA::TDF::TimeValue&>( Blaze::gController->getFrameworkConfigTdf().getMetricsLoggingConfig().getOperationalInsightsExport().getExportInterval() ).setSeconds(0);
        collection.configure(oiConfig);
        collection.exportMetrics();

        EXPECT_EQ(true,  testCounter1.isExported());
        EXPECT_EQ(false, testCounter2.isExported());
        EXPECT_EQ(false, testCounter3.isExported());
        EXPECT_EQ(false, testCounter4.isExported());
        EXPECT_EQ(false, testCounter5.isExported());
        EXPECT_EQ(false, testCounter6.isExported());
        EXPECT_EQ(true,  testCounter7.isExported());
        EXPECT_EQ(true,  testCounter8.isExported());
        EXPECT_EQ(false, testCounter9.isExported());
        EXPECT_EQ(false, testCounter10.isExported());
    }

    TEST_F(MetricsTest, BasicCounter)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::Metrics::Counter testCounter(collection, "testCounter");

        testCounter.increment(1);
        EXPECT_EQ(1, testCounter.getTotal());

        testCounter.increment(5);
        EXPECT_EQ(6, testCounter.getTotal());
    }


    TEST_F(MetricsTest, BasicGauge)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::Metrics::Gauge testGauge(collection, "testGauge");

        testGauge.set(35);
        EXPECT_EQ(35, testGauge.get());

        testGauge.increment(5);
        EXPECT_EQ(40, testGauge.get());

        testGauge.decrement(10);
        EXPECT_EQ(30, testGauge.get());
    }


    TEST_F(MetricsTest, BasicPolledGauge)
    {
        Blaze::Metrics::MetricsCollection collection;
        eastl::set<uint32_t> tempSet;

        Blaze::Metrics::PolledGauge testGauge(collection, "testPolledGauge", [&tempSet]() { return tempSet.size(); });

        tempSet.insert(5);
        tempSet.insert(20);

        EXPECT_EQ(2, testGauge.get());
    }

    TEST_F(MetricsTest, BasicTimer)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::Metrics::Timer testTimer(collection, "testTimer");

        EA::TDF::TimeValue time1(1000);
        EA::TDF::TimeValue time2(500);

        testTimer.record(time1);
        testTimer.record(time2);

        EXPECT_EQ(500, testTimer.getTotalMin());
        EXPECT_EQ(1000, testTimer.getTotalMax());
        EXPECT_EQ(2, testTimer.getTotalCount());
        EXPECT_EQ(1500, testTimer.getTotalTime());
        EXPECT_EQ(750, testTimer.getTotalAverage());
    }

    TEST_F(MetricsTest, BasicTaggedCounter)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::Metrics::TaggedCounter<const char*> oneTagCounter (collection, "onTagCounter", Blaze::Metrics::Tag::tag1);
        Blaze::Metrics::TaggedCounter<const char*, const char*, const char*> threeTagCounter(collection, "threeTaggedCounter", Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag2, Blaze::Metrics::Tag::tag3);

        oneTagCounter.increment(1, "foo");
        oneTagCounter.increment(2, "bar");

        EXPECT_EQ(3, oneTagCounter.getTotal());
        EXPECT_EQ(1, oneTagCounter.getTotal({ { Blaze::Metrics::Tag::tag1, "foo" } }));
        EXPECT_EQ(2, oneTagCounter.getTotal({ { Blaze::Metrics::Tag::tag1, "bar" } }));

        threeTagCounter.increment(1, "a", "foo", "alpha");
        threeTagCounter.increment(2, "a", "bar", "beta");
        threeTagCounter.increment(3, "a", "foo", "delta");

        EXPECT_EQ(6, threeTagCounter.getTotal());
        EXPECT_EQ(6, threeTagCounter.getTotal({ { Blaze::Metrics::Tag::tag1, "a" } }));
        EXPECT_EQ(4, threeTagCounter.getTotal({ { Blaze::Metrics::Tag::tag1, "a"}, { Blaze::Metrics::Tag::tag2, "foo"} }));
        EXPECT_EQ(2, threeTagCounter.getTotal({ { Blaze::Metrics::Tag::tag1, "a"}, { Blaze::Metrics::Tag::tag2, "bar"}, { Blaze::Metrics::Tag::tag3, "beta" } }));
    }

    TEST_F(MetricsTest, BasicIterate)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::Metrics::TaggedCounter<const char*> oneTagCounter(collection, "onTagCounter", Blaze::Metrics::Tag::tag1);
        Blaze::Metrics::TaggedCounter<const char*, const char*, const char*> threeTagCounter(collection, "threeTaggedCounter", Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag2, Blaze::Metrics::Tag::tag3);

        oneTagCounter.increment(1, "foo");
        oneTagCounter.increment(2, "bar");

        oneTagCounter.iterate([](const Blaze::Metrics::TagPairList& tagList, const Blaze::Metrics::Counter& testCounter) {
            if (testCounter.getTotal() == 1)
            {
                EXPECT_STREQ("foo", tagList[0].second.c_str());
            }
            else if (testCounter.getTotal() == 2)
            {
                EXPECT_STREQ("bar", tagList[0].second.c_str());
            }
        });

        threeTagCounter.increment(1, "a", "foo", "alpha");
        threeTagCounter.increment(2, "a", "bar", "beta");
        threeTagCounter.increment(3, "a", "foo", "delta");

        threeTagCounter.iterate([](const Blaze::Metrics::TagPairList& tagList, const Blaze::Metrics::Counter& testCounter) {
            if (testCounter.getTotal() == 1)
            {
                EXPECT_STREQ("a", tagList[0].second.c_str());
                EXPECT_STREQ("foo", tagList[1].second.c_str());
                EXPECT_STREQ("alpha", tagList[2].second.c_str());
            }
            else if (testCounter.getTotal() == 2)
            {
                EXPECT_STREQ("a", tagList[0].second.c_str());
                EXPECT_STREQ("bar", tagList[1].second.c_str());
                EXPECT_STREQ("beta", tagList[2].second.c_str());
            }
            else if (testCounter.getTotal() == 3)
            {
                EXPECT_STREQ("a", tagList[0].second.c_str());
                EXPECT_STREQ("foo", tagList[1].second.c_str());
                EXPECT_STREQ("delta", tagList[2].second.c_str());
            }
            else
            {
                // unknown value returned
                GTEST_FAIL() << "unknown value returned " << testCounter.getTotal();
            }
        });
    }

    TEST_F(MetricsTest, BasicSumAggregate)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::Metrics::TaggedCounter<const char*, const char*, const char*> threeTagCounter(collection, "threeTaggedCounter", Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag2, Blaze::Metrics::Tag::tag3);

        threeTagCounter.increment(1, "a", "foo", "alpha");
        threeTagCounter.increment(2, "a", "bar", "beta");
        threeTagCounter.increment(3, "a", "foo", "delta");

        Blaze::Metrics::SumsByTagValue sums;
        threeTagCounter.aggregate({ Blaze::Metrics::Tag::tag2 }, sums);

        for (auto& item : sums)
        {
            for (auto& tagValue : item.first)
            {
                if (blaze_strcmp("foo", tagValue) == 0)
                {
                    EXPECT_EQ(4, item.second);
                }
                else if (blaze_strcmp("bar", tagValue) == 0)
                {
                    EXPECT_EQ(2, item.second);
                }
                else
                {
                    GTEST_FAIL() << "unknown Tag value " << tagValue;
                }
            }
        }
    }

    TEST_F(MetricsTest, BasicTaggedGauge)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::Metrics::TaggedGauge<const char*> oneTagGauge(collection, "onTagGauge", Blaze::Metrics::Tag::tag1);
        Blaze::Metrics::TaggedGauge<const char*, const char*, const char*> threeTagGauge(collection, "threeTaggedGauge", Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag2, Blaze::Metrics::Tag::tag3);
        Blaze::Metrics::TaggedGauge<const char*, const char*, const char*> threeTagGaugeGroups(collection, "threeTaggedGaugeGroups", Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag2, Blaze::Metrics::Tag::tag3);

        oneTagGauge.set(5, "foo");
        oneTagGauge.set(10, "bar");

        EXPECT_EQ(15, oneTagGauge.get());
        EXPECT_EQ(5, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "foo" } }));
        EXPECT_EQ(10, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "bar" } }));

        oneTagGauge.increment(2, "foo");
        oneTagGauge.increment(2, "bar");

        EXPECT_EQ(19, oneTagGauge.get());
        EXPECT_EQ(7, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "foo" } }));
        EXPECT_EQ(12, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "bar" } }));

        oneTagGauge.decrement(2, "foo");
        oneTagGauge.decrement(2, "bar");

        EXPECT_EQ(15, oneTagGauge.get());
        EXPECT_EQ(5, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "foo" } }));
        EXPECT_EQ(10, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "bar" } }));

        threeTagGauge.set(5, "a", "foo", "alpha");
        threeTagGauge.set(10, "a", "bar", "beta");
        threeTagGauge.set(15, "a", "foo", "delta");

        EXPECT_EQ(30, threeTagGauge.get());
        EXPECT_EQ(30, threeTagGauge.get({ { Blaze::Metrics::Tag::tag1, "a" } }));
        EXPECT_EQ(10, threeTagGauge.get({ { Blaze::Metrics::Tag::tag1, "a" }, { Blaze::Metrics::Tag::tag2, "bar"} }));
        EXPECT_EQ(15, threeTagGauge.get({ { Blaze::Metrics::Tag::tag1, "a" }, { Blaze::Metrics::Tag::tag2, "foo" }, { Blaze::Metrics::Tag::tag3, "delta"} }));



        threeTagGaugeGroups.defineTagGroups({
            { Blaze::Metrics::Tag::tag1 },
            { Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag3 },
            { Blaze::Metrics::Tag::tag2 },
            { Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag2, Blaze::Metrics::Tag::tag3 } });

        threeTagGaugeGroups.increment(1, "a", "a", "a");
        threeTagGaugeGroups.increment(1, "a", "b", "a");
        threeTagGaugeGroups.increment(1, "a", "b", "b");
        threeTagGaugeGroups.increment(1, "a", "b", "c");
        threeTagGaugeGroups.increment(1, "c", "a", "a");
        threeTagGaugeGroups.increment(1, "a", "a", "c");
        threeTagGaugeGroups.increment(1, "b", "b", "b");

        EXPECT_EQ(7, threeTagGaugeGroups.get());

        EXPECT_EQ(5, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag1, "a" } }));
        EXPECT_EQ(1, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag1, "b" } }));
        EXPECT_EQ(1, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag1, "c" } }));

        EXPECT_EQ(2, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag1, "a" }, { Blaze::Metrics::Tag::tag3, "a"} }));
        EXPECT_EQ(1, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag1, "a" }, { Blaze::Metrics::Tag::tag3, "b"} }));
        EXPECT_EQ(2, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag1, "a" }, { Blaze::Metrics::Tag::tag3, "c"} }));
        EXPECT_EQ(1, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag1, "c" }, { Blaze::Metrics::Tag::tag3, "a"} }));
        EXPECT_EQ(1, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag1, "b" }, { Blaze::Metrics::Tag::tag3, "b"} }));

        EXPECT_EQ(3, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag2, "a" } }));
        EXPECT_EQ(4, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag2, "b" } }));

        EXPECT_EQ(1, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag1, "a" }, { Blaze::Metrics::Tag::tag2, "b" }, { Blaze::Metrics::Tag::tag3, "c"} }));
        EXPECT_EQ(1, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag1, "a" }, { Blaze::Metrics::Tag::tag2, "a" }, { Blaze::Metrics::Tag::tag3, "a"} }));
        EXPECT_EQ(1, threeTagGaugeGroups.get({ { Blaze::Metrics::Tag::tag1, "b" }, { Blaze::Metrics::Tag::tag2, "b" }, { Blaze::Metrics::Tag::tag3, "b"} }));

    }

    TEST_F(MetricsTest, BasicTaggedPolledGauge)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::Metrics::TaggedPolledGauge<const char*> oneTagGauge(collection, "onTagGauge", Blaze::Metrics::Tag::tag1);
        Blaze::Metrics::TaggedPolledGauge<const char*, const char*, const char*> threeTagGauge(collection, "threeTaggedGauge", Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag2, Blaze::Metrics::Tag::tag3);

        uint64_t oneTagGaugeIncrement = 0;
        oneTagGauge.startOrReconfigure([&oneTagGaugeIncrement]() { return 5 + oneTagGaugeIncrement; }, "foo");
        oneTagGauge.startOrReconfigure([&oneTagGaugeIncrement]() { return 10 + oneTagGaugeIncrement; }, "bar");

        EXPECT_EQ(15, oneTagGauge.get());
        EXPECT_EQ(5, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "foo" } }));
        EXPECT_EQ(10, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "bar" } }));

        oneTagGaugeIncrement = 2;

        EXPECT_EQ(19, oneTagGauge.get());
        EXPECT_EQ(7, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "foo" } }));
        EXPECT_EQ(12, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "bar" } }));

        oneTagGauge.startOrReconfigure([&oneTagGaugeIncrement]() { return 15 + oneTagGaugeIncrement; }, "foo");

        EXPECT_EQ(29, oneTagGauge.get());
        EXPECT_EQ(17, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "foo" } }));
        EXPECT_EQ(12, oneTagGauge.get({ { Blaze::Metrics::Tag::tag1, "bar" } }));

        threeTagGauge.startOrReconfigure([]() { return 5; }, "a", "foo", "alpha");
        threeTagGauge.startOrReconfigure([]() { return 10; }, "a", "bar", "beta");
        threeTagGauge.startOrReconfigure([]() { return 15; }, "a", "foo", "delta");

        EXPECT_EQ(30, threeTagGauge.get());
        EXPECT_EQ(30, threeTagGauge.get({ { Blaze::Metrics::Tag::tag1, "a" } }));
        EXPECT_EQ(10, threeTagGauge.get({ { Blaze::Metrics::Tag::tag1, "a" }, { Blaze::Metrics::Tag::tag2, "bar"} }));
        EXPECT_EQ(15, threeTagGauge.get({ { Blaze::Metrics::Tag::tag1, "a" }, { Blaze::Metrics::Tag::tag2, "foo" }, { Blaze::Metrics::Tag::tag3, "delta"} }));

    }

    TEST_F(MetricsDeathTest, TaggedPolledGaugeDisallowsDefiningTagGroups)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::Metrics::TaggedPolledGauge<const char*, const char*, const char*> threeTagGauge(collection, "threeTaggedGauge", Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag2, Blaze::Metrics::Tag::tag3);

        EXPECT_DEATH(
            threeTagGauge.defineTagGroups({
                { Blaze::Metrics::Tag::tag1 },
                { Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag3 },
                { Blaze::Metrics::Tag::tag2 },
                { Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag2, Blaze::Metrics::Tag::tag3 } }),
                "");
    }

    TEST_F(MetricsDeathTest, TaggedPolledGaugeDisallowsDisablingTags)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::Metrics::TaggedPolledGauge<const char*, const char*, const char*> threeTagGauge(collection, "threeTaggedGauge", Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag2, Blaze::Metrics::Tag::tag3);

        EXPECT_DEATH(
            threeTagGauge.disableTags({ Blaze::Metrics::Tag::tag1, Blaze::Metrics::Tag::tag3 }),
            "");
    }

    TEST_F(MetricsTest, BasicTaggedTimer)
    {
        Blaze::Metrics::MetricsCollection collection;
        Blaze::Metrics::TaggedTimer<const char*> oneTagTimer(collection, "oneTagTimer", Blaze::Metrics::Tag::tag1);

        EA::TDF::TimeValue time1(1000);
        EA::TDF::TimeValue time2(500);
        EA::TDF::TimeValue time3(200);
        EA::TDF::TimeValue time4(100);

        oneTagTimer.record(time1, "foo");
        oneTagTimer.record(time2, "foo");
        oneTagTimer.record(time3, "bar");
        oneTagTimer.record(time4, "bar");

        oneTagTimer.iterate([](const Blaze::Metrics::TagPairList& tagList, const Blaze::Metrics::Timer& testTimer) {
            if (blaze_strcmp("foo", tagList[0].second.c_str()) == 0)
            {
                EXPECT_EQ(500, testTimer.getTotalMin());
                EXPECT_EQ(1000, testTimer.getTotalMax());
                EXPECT_EQ(2, testTimer.getTotalCount());
                EXPECT_EQ(1500, testTimer.getTotalTime());
                EXPECT_EQ(750, testTimer.getTotalAverage());
            }
            else if (blaze_strcmp("bar", tagList[0].second.c_str()) == 0)
            {
                EXPECT_EQ(100, testTimer.getTotalMin());
                EXPECT_EQ(200, testTimer.getTotalMax());
                EXPECT_EQ(2, testTimer.getTotalCount());
                EXPECT_EQ(300, testTimer.getTotalTime());
                EXPECT_EQ(150, testTimer.getTotalAverage());
            }
        });
    }


}
}