/*************************************************************************************************/
/*!
\file externalservicemetricstest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "test/metrics/externalservicemetricstest.h"
#include "framework/metrics/externalservicemetrics.h"

namespace BlazeServerTest
{
    namespace Metrics
    {
        TEST_F(ExternalServiceMetricsTest, IncCounters)
        {
            Blaze::Metrics::MetricsCollection collection;
            Blaze::ExternalServiceMetrics externalMetrics(collection);
            Blaze::ComponentStatus status;
            Blaze::ComponentStatus::InfoMap& infoMap = status.getInfo();

            externalMetrics.incCallsStarted("name", "uri");
            externalMetrics.incCallsFinished("name", "uri");
            externalMetrics.incRequestsSent("name", "uri");
            externalMetrics.incRequestsFailed("name", "uri");
            externalMetrics.incResponseCount("name", "uri", "OK");
            externalMetrics.recordResponseTime("name", "uri", "OK", 100000);

            externalMetrics.getStatusInfo(status);
            
            ASSERT_STREQ("1", infoMap["ExtService_name_uri_STARTED"].c_str());
            ASSERT_STREQ("1", infoMap["ExtService_name_uri_FINISHED"].c_str());
            ASSERT_STREQ("1", infoMap["ExtService_name_uri_REQSENT"].c_str());
            ASSERT_STREQ("1", infoMap["ExtService_name_uri_REQFAILED"].c_str());
            ASSERT_STREQ("1", infoMap["ExtService_name_uri_OK_COUNT"].c_str());
            ASSERT_STREQ("100000", infoMap["ExtService_name_uri_OK_TIME"].c_str());
            

            externalMetrics.incCallsStarted("name", "uri");
            externalMetrics.incCallsFinished("name", "uri");
            externalMetrics.incRequestsSent("name", "uri");
            externalMetrics.incRequestsFailed("name", "uri");
            externalMetrics.incResponseCount("name", "uri", "OK");
            externalMetrics.recordResponseTime("name", "uri", "OK", 100000);

            externalMetrics.getStatusInfo(status);

            ASSERT_STREQ("2", infoMap["ExtService_name_uri_STARTED"].c_str());
            ASSERT_STREQ("2", infoMap["ExtService_name_uri_FINISHED"].c_str());
            ASSERT_STREQ("2", infoMap["ExtService_name_uri_REQSENT"].c_str());
            ASSERT_STREQ("2", infoMap["ExtService_name_uri_REQFAILED"].c_str());
            ASSERT_STREQ("2", infoMap["ExtService_name_uri_OK_COUNT"].c_str());
            ASSERT_STREQ("200000", infoMap["ExtService_name_uri_OK_TIME"].c_str());
            
            externalMetrics.incCallsStarted("name", "new-uri");
            externalMetrics.incCallsFinished("name", "new-uri");
            externalMetrics.incRequestsSent("name", "new-uri");
            externalMetrics.incRequestsFailed("name", "new-uri");
            externalMetrics.incResponseCount("name", "new-uri", "OK");
            externalMetrics.recordResponseTime("name", "new-uri", "OK", 100000);

            externalMetrics.getStatusInfo(status);

            ASSERT_STREQ("1", infoMap["ExtService_name_new-uri_STARTED"].c_str());
            ASSERT_STREQ("1", infoMap["ExtService_name_new-uri_FINISHED"].c_str());
            ASSERT_STREQ("1", infoMap["ExtService_name_new-uri_REQSENT"].c_str());
            ASSERT_STREQ("1", infoMap["ExtService_name_new-uri_REQFAILED"].c_str());
            ASSERT_STREQ("1", infoMap["ExtService_name_new-uri_OK_COUNT"].c_str());
            ASSERT_STREQ("100000", infoMap["ExtService_name_new-uri_OK_TIME"].c_str());
        }
    } //namespace Metrics
} //namespace BlazeServerTest
