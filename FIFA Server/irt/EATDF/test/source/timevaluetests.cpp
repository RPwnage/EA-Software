
#include <test.h>
#include <test/tdf/alltypes.h>

TEST(TimeValueTests, ParsingIntervalStrings)
{
    // Test parsing interval strings
    const char* durations[] = { "y", "d", "h", "m", "s", "ms" };
    uint32_t numDurations = (uint32_t)sizeof(durations) / sizeof(durations[0]);
    uint32_t numIterations = 1 << numDurations;
    for (uint32_t idx = 1; idx < numIterations; ++idx)
    {
        uint32_t bits = idx;
        char buffer[256] = "";
        for (uint32_t i = 0; (i < numDurations) && (bits != 0); ++i)
        {
            if (bits & 1)
            {
                if (buffer[0] != '\0')
                    strcat(buffer, ":");
                strcat(buffer, "1");
                strcat(buffer, durations[i]);
            }
            bits = bits >> 1;
        }
        EA::TDF::TimeValue t(buffer, EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
        char buf2[256];
        t.toIntervalString(buf2, sizeof(buf2));
        EXPECT_STREQ(buf2, buffer);

        EA::TDF::TimeValue t2;
        EXPECT_TRUE(t2.parseTimeInterval(buf2))
            << "string = " << buf2;
        EXPECT_EQ(t, t2);
    }
}


TEST(TimeValueTests, NegativeIntervalStrings)
// Test negative time interval string
{
    const char* buffer = "-1m:30s";
    EA::TDF::TimeValue t(buffer, EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
    char buf2[256];
    t.toIntervalString(buf2, sizeof(buf2));
    EXPECT_STREQ(buf2, buffer);
    EA::TDF::TimeValue t2;
    EXPECT_TRUE(t2.parseTimeInterval(buf2))
        << "string = " << buf2;
    EXPECT_EQ(t, t2);
}

TEST(TimeValueTests, parseLocalTime)
// Test parseLocalTime() for prepending of current local date + time string supplied
{
    // get NOW() to compare
    EA::TDF::TimeValue now = EA::TDF::TimeValue::getTimeOfDay();
    uint32_t nowYear, nowMonth, nowDay; 
    nowYear = nowMonth = nowDay = 0;
    EA::TDF::TimeValue::getLocalTimeComponents(now, &nowYear, &nowMonth, &nowDay);

    // test data - supposed to prepend current LOCAL date + time string given
    EA::TDF::TimeValue test;
    bool success = test.parseLocalTime("14:25:36");

    uint32_t testYear, testMonth, testDay, testHour, testMinute, testSecond;
    testYear = testMonth = testDay = testHour = testMinute = testSecond = 0;
    EA::TDF::TimeValue::getLocalTimeComponents(test, &testYear, &testMonth, &testDay, &testHour, &testMinute, &testSecond);

    // the final parsed local time should be the current LOCAL date + time(14:25:36)
    if (success && testYear == nowYear && testMonth == nowMonth && testDay == nowDay && testHour == 14 && testMinute == 25 && testSecond == 36)
    { 
        // no-op - success
    }
    else
    {
        // failed (in case we are just so lucky to be rolled over the next day, get NOW again and compare)
        now = EA::TDF::TimeValue::getTimeOfDay();
        nowYear = nowMonth = nowDay = 0; 
        EA::TDF::TimeValue::getLocalTimeComponents(now, &nowYear, &nowMonth, &nowDay);

        // the final parsed local time should be the current LOCAL date + time(14:25:36)
        EXPECT_TRUE(success);
        EXPECT_EQ(testHour, (uint32_t)14);
        EXPECT_EQ(testMinute, (uint32_t)25);
        EXPECT_EQ(testSecond, (uint32_t)36);
        EXPECT_EQ(testYear, nowYear);
        EXPECT_EQ(testMonth, nowMonth);
        EXPECT_EQ(testDay, nowDay);
    }       
}    

TEST(TimeValueTests, parseGmTime)
// Test parseGmTime() for prepending of current GMT date + time string supplied
{
    // get NOW() to compare
    EA::TDF::TimeValue now = EA::TDF::TimeValue::getTimeOfDay();
    uint32_t nowYear, nowMonth, nowDay; 
    nowYear = nowMonth = nowDay = 0;
    EA::TDF::TimeValue::getGmTimeComponents(now, &nowYear, &nowMonth, &nowDay);

    // test data - supposed to prepend current GMT date + time string given
    EA::TDF::TimeValue test;
    bool success = test.parseGmTime("14:25:36");

    uint32_t testYear, testMonth, testDay, testHour, testMinute, testSecond;
    testYear = testMonth = testDay = testHour = testMinute = testSecond = 0;
    EA::TDF::TimeValue::getGmTimeComponents(test, &testYear, &testMonth, &testDay, &testHour, &testMinute, &testSecond);

    // the final parsed GMT time should be the current GMT date + time(14:25:36)
    if (success && testYear == nowYear && testMonth == nowMonth && testDay == nowDay && testHour == 14 && testMinute == 25 && testSecond == 36)
    { 
        // no-op - success
    }
    else
    {
        // failed (in case we are just so lucky to be rolled over the next day, get NOW again and compare)
        now = EA::TDF::TimeValue::getTimeOfDay();
        nowYear = nowMonth = nowDay = 0; 
        EA::TDF::TimeValue::getGmTimeComponents(now, &nowYear, &nowMonth, &nowDay);

        // the final parsed GMT time should be the current GMT date + time(14:25:36)
        EXPECT_TRUE(success);
        EXPECT_EQ(testHour, (uint32_t)14);
        EXPECT_EQ(testMinute, (uint32_t)25);
        EXPECT_EQ(testSecond, (uint32_t)36);
        EXPECT_EQ(testYear, nowYear);
        EXPECT_EQ(testMonth, nowMonth);
        EXPECT_EQ(testDay, nowDay);
         
    }       
}

TEST(TimeValueTests, dstDateTimeParsing)
// Test Daylight Savings Time date+time parsing
{
    // "2020/03/08-01:49:00" is an invalid time if DST is in effect. 
    EA::TDF::TimeValue t1("2020/03/08-01:49:00", EA::TDF::TimeValue::TIME_FORMAT_GMDATE); 
    EA::TDF::TimeValue t2("2020/03/08-02:49:00", EA::TDF::TimeValue::TIME_FORMAT_GMDATE);  // With DST, 2:49 is skipped  (1:59 -> 3:00).  GMT has no DST.
    EA::TDF::TimeValue t3("2020/03/08-03:49:00", EA::TDF::TimeValue::TIME_FORMAT_GMDATE);

    EA::TDF::TimeValue t4("2020/11/01-00:49:00", EA::TDF::TimeValue::TIME_FORMAT_GMDATE);
    EA::TDF::TimeValue t5("2020/11/01-01:49:00", EA::TDF::TimeValue::TIME_FORMAT_GMDATE);  // With DST, 1:49 is happens twice  (1:59 -> 1:00).  GMT has no DST.
    EA::TDF::TimeValue t6("2020/11/01-02:49:00", EA::TDF::TimeValue::TIME_FORMAT_GMDATE);

    EXPECT_TRUE(t1 != t2);
    EXPECT_TRUE(t1 != t3);
    EXPECT_TRUE(t2 != t3);

    EXPECT_TRUE(t4 != t5);
    EXPECT_TRUE(t4 != t6);
    EXPECT_TRUE(t5 != t6);

    EA::TDF::TimeValue lt1; lt1.parseLocalDateTime("2020/03/08-01:49:00");
    EA::TDF::TimeValue lt2; lt2.parseLocalDateTime("2020/03/08-02:49:00");  // With DST, 2:49 is skipped  (1:59 -> 3:00).  GMT has no DST.
    EA::TDF::TimeValue lt3; lt3.parseLocalDateTime("2020/03/08-03:49:00");

    EA::TDF::TimeValue lt4; lt4.parseLocalDateTime("2020/11/01-00:49:00");
    EA::TDF::TimeValue lt5; lt5.parseLocalDateTime("2020/11/01-01:49:00");  // With DST, 1:49 is happens twice  (1:59 -> 1:00).  GMT has no DST.
    EA::TDF::TimeValue lt6; lt6.parseLocalDateTime("2020/11/01-02:49:00");

    EXPECT_TRUE(lt1 != lt2);
    EXPECT_TRUE(lt1 != lt3);
    EXPECT_TRUE(lt2 != lt3);

    EXPECT_TRUE(lt4 != lt5);
    EXPECT_TRUE(lt4 != lt6);
    EXPECT_TRUE(lt5 != lt6);

    tm tM;
    EA::TDF::TimeValue::gmTime(&tM, t1.getMicroSeconds()); int64_t v1 = EA::TDF::TimeValue::mkgmTime(&tM, true);
    EA::TDF::TimeValue::gmTime(&tM, t2.getMicroSeconds()); int64_t v2 = EA::TDF::TimeValue::mkgmTime(&tM, true);
    EA::TDF::TimeValue::gmTime(&tM, t3.getMicroSeconds()); int64_t v3 = EA::TDF::TimeValue::mkgmTime(&tM, true);
    EA::TDF::TimeValue::gmTime(&tM, t4.getMicroSeconds()); int64_t v4 = EA::TDF::TimeValue::mkgmTime(&tM, true);
    EA::TDF::TimeValue::gmTime(&tM, t5.getMicroSeconds()); int64_t v5 = EA::TDF::TimeValue::mkgmTime(&tM, true);
    EA::TDF::TimeValue::gmTime(&tM, t6.getMicroSeconds()); int64_t v6 = EA::TDF::TimeValue::mkgmTime(&tM, true);

    EXPECT_TRUE(v1 != t1.getMicroSeconds());
    EXPECT_TRUE(v2 != t2.getMicroSeconds());
    EXPECT_TRUE(v3 != t3.getMicroSeconds());
    EXPECT_TRUE(v4 != t4.getMicroSeconds());
    EXPECT_TRUE(v5 != t5.getMicroSeconds());
    EXPECT_TRUE(v6 != t6.getMicroSeconds());

    EA::TDF::TimeValue::gmTime(&tM, lt1.getMicroSeconds()); int64_t lv1 = EA::TDF::TimeValue::mkgmTime(&tM, true);
    EA::TDF::TimeValue::gmTime(&tM, lt2.getMicroSeconds()); int64_t lv2 = EA::TDF::TimeValue::mkgmTime(&tM, true);
    EA::TDF::TimeValue::gmTime(&tM, lt3.getMicroSeconds()); int64_t lv3 = EA::TDF::TimeValue::mkgmTime(&tM, true);
    EA::TDF::TimeValue::gmTime(&tM, lt4.getMicroSeconds()); int64_t lv4 = EA::TDF::TimeValue::mkgmTime(&tM, true);
    EA::TDF::TimeValue::gmTime(&tM, lt5.getMicroSeconds()); int64_t lv5 = EA::TDF::TimeValue::mkgmTime(&tM, true);
    EA::TDF::TimeValue::gmTime(&tM, lt6.getMicroSeconds()); int64_t lv6 = EA::TDF::TimeValue::mkgmTime(&tM, true);

    EXPECT_TRUE(lv1 != lt1.getMicroSeconds());
    EXPECT_TRUE(lv2 != lt2.getMicroSeconds());
    EXPECT_TRUE(lv3 != lt3.getMicroSeconds());
    EXPECT_TRUE(lv4 != lt4.getMicroSeconds());
    EXPECT_TRUE(lv5 != lt5.getMicroSeconds());
    EXPECT_TRUE(lv6 != lt6.getMicroSeconds());

}

TEST (TimeValueTests, dateTimeParsing)
// Test date+time parsing
{
    struct FormatInfo
    {
        const char* str;
        bool valid;
        int32_t components;
    };
    const FormatInfo timeFormats[] = {
        { "00", true, 1 },
        { "23", true, 1 },
        { "00:00", true, 2 },
        { "23:59", true, 2 },
        { "00:00:00", true, 3 },
        { "23:59:59", true, 3 },
        { "0", true, 1 },
        { "0:0", true, 2 },
        { "00:0", true, 2 },
        { "0:00:00", true, 3 },
        { "00:0:00", true, 3 },
        { "00:00:0", true, 3 },

        { "24", false, 1 },
        { "23:60", false, 2 },
        { "24:59", false, 2 },
        { "24:00:00", false, 3 },
        { "00:60:00", false, 3 },
        { "00:00:62", false, 3 } // 60 and 61 are valid for leap seconds apparently
    };

    const FormatInfo dateFormats[] = {

        // Regular YMD should pass, any fewer components should fail
        { "2014/01/01", true, 3 },
        { "2014/01", false, 2 },
        { "2014", false, 1 },

        // Validate shorter years also pass
        { "111/01/01", true, 3 },
        { "11/01/01", true, 3 },
        { "1/01/01", true, 3 },

        // Validate months/days do not require leading 0's
        { "1111/1/1", true, 3 },

        // Invalid months
        { "1111/111/01", false, 3 },
        { "1111/00/01", false, 3 },
        { "1111/13/01", false, 3 },

        // Invalid days
        { "1111/01/0", false, 3 },
        { "1111/01/32", false, 3 },
        { "1111/01/111", false, 3 }
    };

    for(size_t i = 0; i < sizeof(timeFormats) / sizeof(timeFormats[0]); ++i)
    {
        EA::TDF::TimeValue t1;
        EXPECT_TRUE(t1.parseGmTime(timeFormats[i].str) == timeFormats[i].valid)
            << "time = " << timeFormats[i].str;
            
        EA::TDF::TimeValue t2;
        EXPECT_TRUE(t2.parseLocalTime(timeFormats[i].str) == timeFormats[i].valid)
            << "time = " << timeFormats[i].str;
    }

    for(size_t i = 0; i < sizeof(dateFormats) / sizeof(dateFormats[0]); ++i)
    {
        EA::TDF::TimeValue t;
        EXPECT_TRUE(t.parseGmDateTime(dateFormats[i].str) == dateFormats[i].valid)
            << "date = " << dateFormats[i].str;

        EXPECT_TRUE(t.parseGmDateTime(dateFormats[i].str) == dateFormats[i].valid)
            << "date = " << dateFormats[i].str;

        if (dateFormats[i].components == 3)
        {
            for(size_t j = 0; j < sizeof(timeFormats) / sizeof(timeFormats[0]); ++j)
            {
                char format[256];
                EA::StdC::Snprintf(format, sizeof(format), "%s-%s", dateFormats[i].str, timeFormats[j].str);

                EXPECT_TRUE(t.parseGmDateTime(format) == (dateFormats[i].valid && timeFormats[j].valid))
                    << "format = " << format;

                EXPECT_TRUE(t.parseLocalDateTime(format) == (dateFormats[i].valid && timeFormats[j].valid))
                    << "format = " << format;
            }
        }
    }
}

