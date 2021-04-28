/*************************************************************************************************/
/*!
\file jwttest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "test/framework/jwttest.h"
#include "framework/blaze.h"
#include "framework/oauth/jwtutil.h"
#include "nucleusconnect/tdf/nucleusconnect.h"

namespace Blaze
{
namespace OAuth
{
    //test JwtUtil::getJwtSegmentInfo with valid and invalid token formats
    TEST_F(JwtTest, getJwtSegmentInfoTest)
    {
        struct TestData
        {
            const char8_t* token;
            const bool result;
            const char8_t* header;
            const uint32_t headerSize;
            const char8_t* payload;
            const uint32_t payloadSize;
            const char8_t* signature;
            const uint32_t signatureSize;
            TestData(const char8_t* t, bool r, const char8_t* h, uint32_t hs, const char8_t* p, uint32_t ps, const char8_t* s, uint32_t ss)
                : token(t), result(r), header(h), headerSize(hs), payload(p), payloadSize(ps), signature(s), signatureSize(ss)
            {
            }
        };
        eastl::vector<TestData> testData;

        //test data, mixed of valid and invalid jwt formats
        const char8_t* token1 = "a.b.c";
        testData.push_back(TestData(token1, true, token1, 1, token1 + 2, 1, token1 + 4, 1));

        const char8_t* token2 = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c";
        uint32_t headerSize2 = strlen("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9");
        uint32_t payloadSize2 = strlen("eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ");
        uint32_t signatureSize2 = strlen("SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c");
        testData.push_back(TestData(token2, true, token2, headerSize2, token2 + headerSize2 + 1, payloadSize2, token2 + headerSize2 + payloadSize2 + 2, signatureSize2));

        const char8_t* token3 = "eyJraWQiOiI3MzlkMzAzZS05NjE5LTRmNmMtYWIwZS1lMWUwYWE5N2Y5NTIiLCJhbGciOiJSUzI1NiJ9.eyJpc3MiOiJhY2NvdW50cy5sdC5lYS5jb20iLCJqdGkiOiJTa0V3T2pJdU1Eb3hMakE2WldJeVlXWXlZMkV0WldFeU9TMDBOVE0xTFdJd09UQXRNamsyTVRBNVl6RmlNR0UzIiwiYXpwIjoiRUFEUF9HU19CTFpERVZfUENfQkxaX1NFUlZFUiIsImlhdCI6MTYwODY4ODQxNiwiZXhwIjoxNjA4NzAyODE2LCJ2ZXIiOjEsIm5leHVzIjp7InJzdmQiOnt9LCJjbGkiOiJFQURQX0dTX0JMWkRFVl9QQ19CTFpfU0VSVkVSIiwic2NvIjoib2ZmbGluZSBzZWFyY2guaWRlbnRpdHkgYmFzaWMuZG9tYWluZGF0YSBiYXNpYy51dGlsaXR5IGJhc2ljLmlkZW50aXR5IGJhc2ljLnBlcnNvbmEgYmFzaWMuZW50aXRsZW1lbnQiLCJwaWQiOiIxMTAwMjYzNjE0NjczIiwicHR5IjoiTlVDTEVVUyIsInVpZCI6IjExMDAyNjM2MTQ2NzMiLCJzdHBzIjoiT0ZGIiwiY250eSI6IjEiLCJhdXNyYyI6IjEwMDAwNCIsImVuYyI6Ik1TWWlQbi9TV2YzOWUyeXhXSUlhKzgxMi9pU3pZZlZDZkluai9ORzgzclBTcjlEWGRPYjdEWnV4d0ZpbUlNcno3UnhoRGdkS2g0cmFMeXhxMUlwd2twNit2c3p0MXdYTGNtZUNWU0U2c0ttRU5BaFRaOURsdDNTbXBONXhsVURCN2ZnUlQ1blBOWG95WlBUK1AyRkVLYkZkKzZxb0ZDV1Nka3pQU3FVS1J3NEoxMkQ0OGN5TDV4dk1RbENEa2dHbDJJU2pVRzI5alV5c0FaVnpzS3BNdUNSK09SNlVFYVpJcWE1WG5rWmZRR0NxdFB5dzdkMjlXOXVGdC9BWlcybXhMbWRYLzZMUXUyT2JVLzcxaFdOc1hvSXRzR28zcHVkajZnMkh2bFE4K2ptK2VkSXZpWFRXa1RNODhmM1dSdTduQVZTTElwYVpaRm90a2d0OWd6UjZMc2FRb1E1SnhlZWlJMTBoWVNXNGlqeEFScGxTMTBIc21WR3dSWktHMG92RmxpZjk2NDJJUWsyUHN4S2lWWXV1UFcxeWVUQzBmTDBkYjRBMVBIK0JRcmw1dkJITG00MjRVRVhLeVNHVlhKUFdYV05TR1NEN2twQnBnWTVDZ2piMFlEdkxFWmdGVlpNeG10c3ZQaldLWXdad1Vrb00yRjFmOENJcU9OMGNSNjVHVldmNm93RHJtRjNCV0tJblhaRlBmcjJhVysvZ29EN2ZTN255WDRHbFNZR1F5dis3TkI1VlJiS3ZPU0k0c2VodlMySHdpMVljUDlvamJ1RWNTcG9jOWJyREJIZUsyZFZDY21HR2N1UmdEZWZmVWthYVFYMkpPZTJ3WmJTUXVYODlodmkyQ2Z3UXRiVmNiTVBEc0cwdEpqTHBuM05uc0pReitmTzd4ZEZzMEtLQWFvUy9ZVEJaVmljbWhLRWRyRWdZRjVaUkNJU0RGTzhPcnNHQWM2Qk9sQW9xOHQ1TmdkVExSbHFMMEhzVnN4ZTJncEJvdGxPNTZkK08rQk5iQmFKcjhWb2d3WkRTVjJyR2ZkeXgyTjN1cFEwOXdaMUJEUmg2b2t6OHJNQW9PejRqdlVLVzk5U3lSeHUyNDV0Z0w0djIwMXdFeDV0RGlUWE9oN0Q5ZTc1NUV2eUoybjV5dmtJeGR0YVN4eWdlV2Z3alBrMHBoejZLb2tYMUR6V21rS2tqNUhZeHVFeGhMVGlGRFdvNG9yZW5NdTQ3SjRraXZORTZCU3UycXNWMXg2S3ZkWE9lRzVUWGpRQnZzUGp3T1YybjFLWXJYdzBQaHpQcE5qK1JVMS9HNnJYd0lZdDBhaDgwbjFFWW5IVlhTSU55dDBFMzZGbmpRVnhublZOU1FNU2c1TlVDWGI1ZU96eVZTalQ3bkFFa1RueERVemhEOWh4a25nbVdwYmdSa3dnPSJ9fQ.KSNJBPF0_kjapEnQgKq_qhiICGJY-TblAPBaKDthHKQbxrncuuoN8Ur0c0oaaIfdRKMrWuekVZIKv4YlRkuWv3-0DEVybejZUk4WXlZAI5L3yF7n2DF06V2sVPWUWqrNh829M2iRHkmHToVFY_PZtJVmHduD23Zhlp-Yh2JZIEqnWbYAIGkX3hqicmIRR3HuazvZ97zQHDeyQEuqk5SbEAbQVlegasXAkG0nWu-VPWNQMJnCx1gUw4kI9VdroqATe7Fitm7zv2RbBqaj44V5Bs9b5t-wG9FkTOyRKQp7tEuPsdMWZWnwCTgQ9DSFh0s4F1b36EI932yuaUNj-T-hdg";
        uint32_t headerSize3 = strlen("eyJraWQiOiI3MzlkMzAzZS05NjE5LTRmNmMtYWIwZS1lMWUwYWE5N2Y5NTIiLCJhbGciOiJSUzI1NiJ9");
        uint32_t payloadSize3 = strlen("eyJpc3MiOiJhY2NvdW50cy5sdC5lYS5jb20iLCJqdGkiOiJTa0V3T2pJdU1Eb3hMakE2WldJeVlXWXlZMkV0WldFeU9TMDBOVE0xTFdJd09UQXRNamsyTVRBNVl6RmlNR0UzIiwiYXpwIjoiRUFEUF9HU19CTFpERVZfUENfQkxaX1NFUlZFUiIsImlhdCI6MTYwODY4ODQxNiwiZXhwIjoxNjA4NzAyODE2LCJ2ZXIiOjEsIm5leHVzIjp7InJzdmQiOnt9LCJjbGkiOiJFQURQX0dTX0JMWkRFVl9QQ19CTFpfU0VSVkVSIiwic2NvIjoib2ZmbGluZSBzZWFyY2guaWRlbnRpdHkgYmFzaWMuZG9tYWluZGF0YSBiYXNpYy51dGlsaXR5IGJhc2ljLmlkZW50aXR5IGJhc2ljLnBlcnNvbmEgYmFzaWMuZW50aXRsZW1lbnQiLCJwaWQiOiIxMTAwMjYzNjE0NjczIiwicHR5IjoiTlVDTEVVUyIsInVpZCI6IjExMDAyNjM2MTQ2NzMiLCJzdHBzIjoiT0ZGIiwiY250eSI6IjEiLCJhdXNyYyI6IjEwMDAwNCIsImVuYyI6Ik1TWWlQbi9TV2YzOWUyeXhXSUlhKzgxMi9pU3pZZlZDZkluai9ORzgzclBTcjlEWGRPYjdEWnV4d0ZpbUlNcno3UnhoRGdkS2g0cmFMeXhxMUlwd2twNit2c3p0MXdYTGNtZUNWU0U2c0ttRU5BaFRaOURsdDNTbXBONXhsVURCN2ZnUlQ1blBOWG95WlBUK1AyRkVLYkZkKzZxb0ZDV1Nka3pQU3FVS1J3NEoxMkQ0OGN5TDV4dk1RbENEa2dHbDJJU2pVRzI5alV5c0FaVnpzS3BNdUNSK09SNlVFYVpJcWE1WG5rWmZRR0NxdFB5dzdkMjlXOXVGdC9BWlcybXhMbWRYLzZMUXUyT2JVLzcxaFdOc1hvSXRzR28zcHVkajZnMkh2bFE4K2ptK2VkSXZpWFRXa1RNODhmM1dSdTduQVZTTElwYVpaRm90a2d0OWd6UjZMc2FRb1E1SnhlZWlJMTBoWVNXNGlqeEFScGxTMTBIc21WR3dSWktHMG92RmxpZjk2NDJJUWsyUHN4S2lWWXV1UFcxeWVUQzBmTDBkYjRBMVBIK0JRcmw1dkJITG00MjRVRVhLeVNHVlhKUFdYV05TR1NEN2twQnBnWTVDZ2piMFlEdkxFWmdGVlpNeG10c3ZQaldLWXdad1Vrb00yRjFmOENJcU9OMGNSNjVHVldmNm93RHJtRjNCV0tJblhaRlBmcjJhVysvZ29EN2ZTN255WDRHbFNZR1F5dis3TkI1VlJiS3ZPU0k0c2VodlMySHdpMVljUDlvamJ1RWNTcG9jOWJyREJIZUsyZFZDY21HR2N1UmdEZWZmVWthYVFYMkpPZTJ3WmJTUXVYODlodmkyQ2Z3UXRiVmNiTVBEc0cwdEpqTHBuM05uc0pReitmTzd4ZEZzMEtLQWFvUy9ZVEJaVmljbWhLRWRyRWdZRjVaUkNJU0RGTzhPcnNHQWM2Qk9sQW9xOHQ1TmdkVExSbHFMMEhzVnN4ZTJncEJvdGxPNTZkK08rQk5iQmFKcjhWb2d3WkRTVjJyR2ZkeXgyTjN1cFEwOXdaMUJEUmg2b2t6OHJNQW9PejRqdlVLVzk5U3lSeHUyNDV0Z0w0djIwMXdFeDV0RGlUWE9oN0Q5ZTc1NUV2eUoybjV5dmtJeGR0YVN4eWdlV2Z3alBrMHBoejZLb2tYMUR6V21rS2tqNUhZeHVFeGhMVGlGRFdvNG9yZW5NdTQ3SjRraXZORTZCU3UycXNWMXg2S3ZkWE9lRzVUWGpRQnZzUGp3T1YybjFLWXJYdzBQaHpQcE5qK1JVMS9HNnJYd0lZdDBhaDgwbjFFWW5IVlhTSU55dDBFMzZGbmpRVnhublZOU1FNU2c1TlVDWGI1ZU96eVZTalQ3bkFFa1RueERVemhEOWh4a25nbVdwYmdSa3dnPSJ9fQ");
        uint32_t signatureSize3 = strlen("KSNJBPF0_kjapEnQgKq_qhiICGJY-TblAPBaKDthHKQbxrncuuoN8Ur0c0oaaIfdRKMrWuekVZIKv4YlRkuWv3-0DEVybejZUk4WXlZAI5L3yF7n2DF06V2sVPWUWqrNh829M2iRHkmHToVFY_PZtJVmHduD23Zhlp-Yh2JZIEqnWbYAIGkX3hqicmIRR3HuazvZ97zQHDeyQEuqk5SbEAbQVlegasXAkG0nWu-VPWNQMJnCx1gUw4kI9VdroqATe7Fitm7zv2RbBqaj44V5Bs9b5t-wG9FkTOyRKQp7tEuPsdMWZWnwCTgQ9DSFh0s4F1b36EI932yuaUNj-T-hdg");
        testData.push_back(TestData(token3, true, token3, headerSize3, token3 + headerSize3 + 1, payloadSize3, token3 + headerSize3 + payloadSize3 + 2, signatureSize3));

        const char8_t* token4 = nullptr;
        testData.push_back(TestData(token4, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token5 = "";
        testData.push_back(TestData(token5, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token6 = "abcdefg";
        testData.push_back(TestData(token6, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token7 = ".abcdefg";
        testData.push_back(TestData(token7, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token8 = "abc.defg";
        testData.push_back(TestData(token8, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token9 = "abcdefg.";
        testData.push_back(TestData(token9, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token10 = "..abcdefg";
        testData.push_back(TestData(token10, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token11 = "abcde..fg";
        testData.push_back(TestData(token11, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token12 = "abcdefg..";
        testData.push_back(TestData(token12, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token13 = ".abcdefg.";
        testData.push_back(TestData(token13, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token14 = ".abcd.efg";
        testData.push_back(TestData(token14, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token15 = "abcd.efg.";
        testData.push_back(TestData(token15, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token16 = ".";
        testData.push_back(TestData(token16, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token17 = "..";
        testData.push_back(TestData(token17, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token18 = "...";
        testData.push_back(TestData(token18, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token19 = "....";
        testData.push_back(TestData(token19, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token20 = "abc.defg.hijk.";
        testData.push_back(TestData(token20, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token21 = "abc.defg.hijk.lmnopq";
        testData.push_back(TestData(token21, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token22 = "abc.defg.hijk.lmnopq.rst";
        testData.push_back(TestData(token22, false, nullptr, 0, nullptr, 0, nullptr, 0));
        const char8_t* token23 = "abcde.fghijklmn.opqrst";
        testData.push_back(TestData(token23, true, token23, 5, token23 + 5 + 1, 9, token23 + 5 + 9 + 2, 6));

        JwtUtil jwtUtil(nullptr);
        for (const auto& data : testData)
        {
            //test case: run each test data and compare actual result to expected
            JwtUtil::JwtSegmentInfo segInfo;
            bool result = jwtUtil.getJwtSegmentInfo(data.token, segInfo);
            EXPECT_TRUE(result == data.result) << "token is: " << data.token;
            if (result)
            {
                EXPECT_TRUE(segInfo.header == data.header) << "token is: " << data.token;
                EXPECT_TRUE(segInfo.headerSize == data.headerSize) << "token is: " << data.token;
                EXPECT_TRUE(segInfo.payload == data.payload) << "token is: " << data.token;
                EXPECT_TRUE(segInfo.payloadSize == data.payloadSize) << "token is: " << data.token;
                EXPECT_TRUE(segInfo.signature == data.signature) << "token is: " << data.token;
                EXPECT_TRUE(segInfo.signatureSize == data.signatureSize) << "token is: " << data.token;
            }
        }
    }

    //test JwtUtil::decodeJwtSegment with valid and invalid jwt segments
    TEST_F(JwtTest, decodeJwtSegmentTest)
    {
        JwtUtil jwtUtil(nullptr);

        //test case: valid jwt header
        {
            /*
            {
              "alg": "algorithm 001",
              "kid": "keyid 002"
            }
            */
            const char8_t* data = "ewogICJhbGciOiAiYWxnb3JpdGhtIDAwMSIsCiAgImtpZCI6ICJrZXlpZCAwMDIiCn0";
            NucleusConnect::JwtHeaderInfo expected;
            expected.setAlgorithm("algorithm 001");
            expected.setKeyId("keyid 002");

            NucleusConnect::JwtHeaderInfo actual;
            BlazeRpcError result = jwtUtil.decodeJwtSegment(data, strlen(data), actual);
            EXPECT_TRUE(result == ERR_OK) << "data is: " << data;
            EXPECT_TRUE(expected.equalsValue(actual)) << "data is: " << data;
        }

        //test case: valid jwt payload
        {
            /*
            {
              "iss": "accounts.int.ea.com",
              "jti": "SkEwOjMuMDoxLjA6NjIxOWQxNmMtM2FmMC00YmUyLTllMmUtYzY2MTg4NzJjOTE2",
              "azp": "EADP_GS_BLZDEV_COM_BLZ_SERVER",
              "iat": 1608702672,
              "exp": 1608717072,
              "ver": 1,
              "nexus": {
                "rsvd": {
                  "efplty": "5"
                },
                "cli": "EADP_GS_BLZDEV_COM_BLZ_SERVER",
                "sco": "gs_qos_coordinator_trusted gs_ccs_client gos_tools_api_user gos_notify_post_event search.identity basic.utility",
                "pid": "1000041899262",
                "pty": "PS3",
                "uid": "1000045199262",
                "psid": 1000041099262,
                "csev": "INT",
                "dvid": "00000007000700c000010181001000005f19ab321388cccab51b71e7ba6679a0",
                "pltyp": "PS4",
                "stps": "OFF",
                "udg": false,
                "cnty": "1",
                "ausrc": "100003",
                "ipgeo": {
                  "ip": "159.153.*.*",
                  "cty": "CA",
                  "reg": "British Columbia",
                  "cit": "Vancouver",
                  "isp": "Electronic Arts  Inc."
                }
              }
            }
            */
            const char8_t* data = "ewogICJpc3MiOiAiYWNjb3VudHMuaW50LmVhLmNvbSIsCiAgImp0aSI6ICJTa0V3T2pNdU1Eb3hMakE2TmpJeE9XUXhObU10TTJGbU1DMDBZbVV5TFRsbE1tVXRZelkyTVRnNE56SmpPVEUyIiwKICAiYXpwIjogIkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwKICAiaWF0IjogMTYwODcwMjY3MiwKICAiZXhwIjogMTYwODcxNzA3MiwKICAidmVyIjogMSwKICAibmV4dXMiOiB7CiAgICAicnN2ZCI6IHsKICAgICAgImVmcGx0eSI6ICI1IgogICAgfSwKICAgICJjbGkiOiAiRUFEUF9HU19CTFpERVZfQ09NX0JMWl9TRVJWRVIiLAogICAgInNjbyI6ICJnc19xb3NfY29vcmRpbmF0b3JfdHJ1c3RlZCBnc19jY3NfY2xpZW50IGdvc190b29sc19hcGlfdXNlciBnb3Nfbm90aWZ5X3Bvc3RfZXZlbnQgc2VhcmNoLmlkZW50aXR5IGJhc2ljLnV0aWxpdHkiLAogICAgInBpZCI6ICIxMDAwMDQxODk5MjYyIiwKICAgICJwdHkiOiAiUFMzIiwKICAgICJ1aWQiOiAiMTAwMDA0NTE5OTI2MiIsCiAgICAicHNpZCI6IDEwMDAwNDEwOTkyNjIsCiAgICAiY3NldiI6ICJJTlQiLAogICAgImR2aWQiOiAiMDAwMDAwMDcwMDA3MDBjMDAwMDEwMTgxMDAxMDAwMDA1ZjE5YWIzMjEzODhjY2NhYjUxYjcxZTdiYTY2NzlhMCIsCiAgICAicGx0eXAiOiAiUFM0IiwKICAgICJzdHBzIjogIk9GRiIsCiAgICAidWRnIjogZmFsc2UsCiAgICAiY250eSI6ICIxIiwKICAgICJhdXNyYyI6ICIxMDAwMDMiLAogICAgImlwZ2VvIjogewogICAgICAiaXAiOiAiMTU5LjE1My4qLioiLAogICAgICAiY3R5IjogIkNBIiwKICAgICAgInJlZyI6ICJCcml0aXNoIENvbHVtYmlhIiwKICAgICAgImNpdCI6ICJWYW5jb3V2ZXIiLAogICAgICAiaXNwIjogIkVsZWN0cm9uaWMgQXJ0cyAgSW5jLiIKICAgIH0KICB9Cn0";
            NucleusConnect::JwtPayloadInfo expected;
            expected.setIssuer("accounts.int.ea.com");
            expected.setTokenId("SkEwOjMuMDoxLjA6NjIxOWQxNmMtM2FmMC00YmUyLTllMmUtYzY2MTg4NzJjOTE2");
            expected.setAuthParty("EADP_GS_BLZDEV_COM_BLZ_SERVER");
            expected.setIssuerTime(1608702672);
            expected.setExpirationTime(1608717072);
            expected.setVersion(1);
            expected.getNexusClaims().setClientId("EADP_GS_BLZDEV_COM_BLZ_SERVER");
            expected.getNexusClaims().setScopes("gs_qos_coordinator_trusted gs_ccs_client gos_tools_api_user gos_notify_post_event search.identity basic.utility");
            expected.getNexusClaims().setPidId("1000041899262");
            expected.getNexusClaims().setPidType("PS3");
            expected.getNexusClaims().setUserId("1000045199262");
            expected.getNexusClaims().setPersonaId(1000041099262);
            expected.getNexusClaims().setConsoleEnv("INT");
            expected.getNexusClaims().setDeviceId("00000007000700c000010181001000005f19ab321388cccab51b71e7ba6679a0");
            expected.getNexusClaims().setPlatformType("PS4");
            expected.getNexusClaims().setStopProcess("OFF");
            expected.getNexusClaims().setIsUnderage(false);
            expected.getNexusClaims().setConnectionType("1");
            expected.getNexusClaims().setAuthSource("100003");
            expected.getNexusClaims().getIpGeoLocation().setIpAddress("159.153.*.*");
            expected.getNexusClaims().getIpGeoLocation().setCountry("CA");
            expected.getNexusClaims().getIpGeoLocation().setRegion("British Columbia");
            expected.getNexusClaims().getIpGeoLocation().setCity("Vancouver");
            expected.getNexusClaims().getIpGeoLocation().setIspProvider("Electronic Arts  Inc.");

            NucleusConnect::JwtPayloadInfo actual;
            BlazeRpcError result = jwtUtil.decodeJwtSegment(data, strlen(data), actual);
            EXPECT_TRUE(result == ERR_OK) << "data is: " << data;
            EXPECT_TRUE(expected.equalsValue(actual)) << "data is: " << data;
        }

        //test case: invalid empty jwt segment
        {
            const char8_t* data = "";

            NucleusConnect::JwtHeaderInfo actual;
            BlazeRpcError result = jwtUtil.decodeJwtSegment(data, strlen(data), actual);
            EXPECT_TRUE(result != ERR_OK) << "data is: " << data;
        }

        //test case: invalid jwt segment
        {
            const char8_t* data = "a";

            NucleusConnect::JwtHeaderInfo actual;
            BlazeRpcError result = jwtUtil.decodeJwtSegment(data, strlen(data), actual);
            EXPECT_TRUE(result != ERR_OK) << "data is: " << data;
        }

        //test case: invalid jwt segment
        {
            const char8_t* data = "fakebase64urlencoding";

            NucleusConnect::JwtHeaderInfo actual;
            BlazeRpcError result = jwtUtil.decodeJwtSegment(data, strlen(data), actual);
            EXPECT_TRUE(result != ERR_OK) << "data is: " << data;
        }

        //test case: jwt header of invalid json format
        {
            /*
            {"alg": "algorithm 001", "kid": "keyid 002"
            */
            const char8_t* data = "eyJhbGciOiAiYWxnb3JpdGhtIDAwMSIsICJraWQiOiAia2V5aWQgMDAyIg";

            NucleusConnect::JwtHeaderInfo actual;
            BlazeRpcError result = jwtUtil.decodeJwtSegment(data, strlen(data), actual);
            EXPECT_TRUE(result != ERR_OK) << "data is: " << data;
        }

        //test case: jwt header of invalid json format
        {
            /*
            {alg = "algorithm 001", "kid": "keyid 002"}
            */
            const char8_t* data = "e2FsZyA9ICJhbGdvcml0aG0gMDAxIiwgImtpZCI6ICJrZXlpZCAwMDIifQ";

            NucleusConnect::JwtHeaderInfo actual;
            BlazeRpcError result = jwtUtil.decodeJwtSegment(data, strlen(data), actual);
            EXPECT_TRUE(result != ERR_OK) << "data is: " << data;
        }
    }

    //test JwtUtil::verifyJwtSignature with valid and invalid sig, msg, mod, exp
    TEST_F(JwtTest, verifyJwtSignatureTest)
    {
        JwtUtil jwtUtil(nullptr);

        //test case: valid sig, msg, mod, exp can be verified successfully
        {
            const char8_t* sig = "aW9Ac9DeZrNszuwt1ARznST-fYU7lDToco4MyZyIZ__9I6gGlwebZmXd3VtwOGtgL1UTq3PIB096R7pBeCnp8xspCRLxfC1KXM5bbZBKPSE3xLfqEb5_ZKYLRzG5Gax7NLrU_5if11PsVeDM7W5RJMmH7GECU9bEVaoDL0V9y9lgsgc-1UbZ512SdIGZ1CQ9XNeiEl-53oxDx_0OLHynrKgUNT1QEi_yT3vxvO9oUz7cpqZV3dtOCQBaE4E5uO_8ib3rq-b-JtB_Slmk2XC_lt2mDr3hr_xAgv2jxiDgDqMl2SGYlo3nCz26x0m2TYoljKru1cAn40qtLCXooAvRPQ";
            const char8_t* msg = "eyJraWQiOiJlMjIwODg4Yy03ZWU0LTQ5ZWUtOWUzMi02YTkzOWRjYmU3M2UiLCJhbGciOiJSUzI1NiJ9.eyJpc3MiOiJhY2NvdW50cy5pbnQuZWEuY29tIiwianRpIjoiU2tFd09qTXVNRG94TGpBNll6STNNVEk0T0RndE5HSm1NeTAwT0dZeUxXSTNPRGt0WmpZNU5XWmtOakE1WVRnMSIsImF6cCI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwiaWF0IjoxNjA4NzEwNDI0LCJleHAiOjE2MDg3MjQ4MjQsInZlciI6MSwibmV4dXMiOnsicnN2ZCI6eyJlZnBsdHkiOiI1In0sImNsaSI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwic2NvIjoiZ3NfcW9zX2Nvb3JkaW5hdG9yX3RydXN0ZWQgZ3NfY2NzX2NsaWVudCBnb3NfdG9vbHNfYXBpX3VzZXIgZ29zX25vdGlmeV9wb3N0X2V2ZW50IHNlYXJjaC5pZGVudGl0eSBiYXNpYy51dGlsaXR5IGJhc2ljLmVudGl0bGVtZW50LndyaXRlIHBsYXllcm93bmVyc2hpcC5yZWFkIGdzX21lc3NhZ2VidXNfcHJvZHVjZXIgYmFzaWMuYWNjZXNzLnRydXN0ZWQgZ29zX2ZyaWVuZHNfZW5kdXNlciBvZmZsaW5lIGJ5dGV2YXVsdCBiYXNpYy54YmxzZXJ2ZXJ0b2tlbiBkcC5pZGVudGl0eS5wc25zZXJ2ZXJ0b2tlbi5yIGRwLnBsYXllcnNldHRpbmdzLnNldHRpbmdzLnIgYmFzaWMucHNudG9rZW4gZ3Nfc3RhdHNfZW5kdXNlciBiYXNpYy54Ymx0b2tlbiBiYXNpYy5wZXJzb25hLndyaXRlIGJhc2ljLmVudGl0bGVtZW50IGJhc2ljLmRvbWFpbmRhdGEgYmFzaWMuaWRlbnRpdHkud3JpdGUgYmFzaWMucGVyc29uYWV4dGVybmFscmVmIGRwLmNvbW1lcmNlLnJlZnJlc2hleHRlbnRpdGxlbWVudC5ydyBiYXNpYy5wcmVmZXJlbmNlLndyaXRlIGdvc19ncm91cF9lbmR1c2VyIGJhc2ljLmFjY2VzcyBzb2NpYWxfcmVjb21tZW5kYXRpb25fdXNlciBiYXNpYy5wcmVmZXJlbmNlIGJhc2ljLmlkZW50aXR5IGJhc2ljLnBlcnNvbmEgZ29zX2dyb3VwX2ludGVncmF0b3IiLCJwaWQiOiIxMDAwMDQxODk5MjYyIiwicHR5IjoiUFMzIiwidWlkIjoiMTAwMDA0NTE5OTI2MiIsInBzaWQiOjEwMDAwNDEwOTkyNjIsImNzZXYiOiJJTlQiLCJkdmlkIjoiMDAwMDAwMDcwMDA3MDBjMDAwMDEwMTgxMDAxMDAwMDA1ZjE5YWIzMjEzODhjY2NhYjUxYjcxZTdiYTY2NzlhMCIsInBsdHlwIjoiUFM0Iiwic3RwcyI6Ik9GRiIsInVkZyI6ZmFsc2UsImNudHkiOiIxIiwiYXVzcmMiOiIxMDAwMDMiLCJpcGdlbyI6eyJpcCI6IjE1OS4xNTMuKi4qIiwiY3R5IjoiQ0EiLCJyZWciOiJCcml0aXNoIENvbHVtYmlhIiwiY2l0IjoiVmFuY291dmVyIiwiaXNwIjoiRWxlY3Ryb25pYyBBcnRzICBJbmMuIn0sImVuYyI6Ik1TWWlQbi9TV2YzOWUyeXhXSUlhKzgxMi9pU3pZZlZDZkluai9ORzgzck0vZVVOSm81YUUzalU0SVVjdTk1RFlnS0ozUk9vdDJxdUNFbGlWVUJIbmRzNnh1NEJ3OUVGRGJKT3pNMFptV3FLUmpkM1ZmRVRMWEtiaWt2QjdhYSs5UjJYdU4xdmVCd2Z4M1lZRUZPUnRPdW9ROWFhVDh2RElmMnNFZzB0SXQ4dGlGazJnT3RlR1pyWjZxRUVYWkZHc2ZMY2ZBZFBRbS9zSjJlVEV1WDRnTnR4cU9naG9zTmM5Y05RcUNmOUVSZ3pNNm9RVFdVb0xVVVZHZDNUY25TMWdJN3JtNHNMTVY2SmNYVGtjbWhOdG5CMkFYYzZxQUpFaS9JcjhuZnVvWjRkUjNNYzQ4YmZ3by9UaEc0WnRIS2xwMHdHVkpPQTNRV1JoTUdIWVNtSGtwMTQxcnB4TGhsKy9SWGdRTnpJQW5OTHRlVThuZEpzOWQ0VjhCcFhYS2tzM0RxdUd5NHhHRWhZRHNCV1RUd3hLU3hxZ3NENTJnbHptd3JiR1E1V2xJU0xhem4xM1YzSGI2NE1qeDh5VzRUVmxUbkI1ZzNxWEU4dE5ESjhucWNIUTltdE9FZG1MS1BLaWhvMjg3amk1QVg5cUpDQ3dXVXFwY2U2S2pYYm1wclAzNW9SWlFoTnZYdzVKZkhWTUk1MEpzZENxMHp4eFRSb0dIMWZ0Z1Q5b3JvZnMvQVJtT2pHQlY2cGs1bFFkcWxodlNBdFBNL3hab3V1TnNXbWVWak9CclJPYkN4dDE0ektrZUxFMGpEc0t3cmhxdWN6RW52WnZTRnlmUXhoeW1BZC9hckdGSTJ3VGliSFZoYU5PeHJoa0tMTVFYYXZJNHRtSytObTFHT3RmNlFoY2xRcjRiTVV3MzBaRGNzb3pTWS9yWCtBSndJZzNSaUVDR1d6dzJ4b2ZCbFVHSWtpa2JvUHJzd25vZnJuclJuVEw3c1RWWUYzcS9iR3NjY0tkNlVZV2VrUFNmL3F2RUg4QS9Rb3NhMXliRHcya3BpdW9lU2RLc3huM3NXeTgrSnF2alR2UjdYRFpBa0oyVjBKVjVNVVJzS0p6MXdBUVV1eG9hc3I2SmJOS0FTU0ZQY0ZFTzlwY1NDR2ZIZDBSK0Q2Zm9sN256K2dPeGNySEFTdm5CRldDMDZ6bXZmWEVFdEJzbnlzanlPakwzMGM3TWJzd0NsMk1vTHJQRkFzdTU5OFNScklpOCt4ZmVDVTM0TDdXTlFNWWhkbEd2Y05TMFByb3ViV0FoM0F5NEtpTXo0cHJpaUo3U2hydXloNFRIeDRJMW9mT2xHR0IrSUhKMWt1aVdHMzBJNHRJVkRQTFZMb1NyNnBxVWlIRkhWZ0s1RVNDaFJUNE5vaVVwRy90dkE1T0dtODNrUzlQdnBrb1Y2TnEwNVFsaFhXVWNqRkRVMzhPRDllZkFxN1g1RlhQc2N5N2xoTVN5SmtTSnJVOVlXU01VOTRUMFc0UytlbVEvR28yR3ZoVDNwaFh3dE45OE1SaitFNEt2MDA3QVcrbXJhR2RWck1RVUVhMnBnNjJyYkNHcEhTaTZzb0YxR1h5cHBhbnBCcVF2MXJ1UERFbHdqNTFYenc2cTlndlB2ZzlxQU5KRTQxZUsva0dDS2djVjN5WHc0SnByVEFNdEdyY1Iwa0lpb3pLK05hdG5ndUdVRFc5bVUrSVRpbHNwUG9nckgrSWVnR0prNEFhYzV6UFgxY09jeXcvOW1MbU5Kc01tYWxaUTQydnhuSFFsM2ZiWmRld0VZQnNNS0M4QkdESlVZb3R0VmFyNXZWNnlGTWxJTU44UTFNNFEvWWNaSjRKbHFXNEVaTUkifX0";
            const char8_t* mod = "ny7FHSTmfpHefqyxDTFkPDJuokxR7y92MtFmf76cNGYYdF1w-sRGMvjuRnZyiiZTD4SAdm6dciLyKaR3BoTX5HuWURuqefZqMkhfduFS8q4kRv-f995hKmIdbnWXPQNDu4HxU4wTha2wCvOaYj0Yx0j-njiMrU7_YeYABRSLlulaVkMes3T32ToPVxBTGoI3lsL58QgQDzzDXHIi4dYH_mr98Rhxz4lDYVzcu5kE702AFowxHIAPWZMZ7N2wAmXgv8xcNlJ3agX9gcVb9_caGvF5BVPUnW2d005TzCnFbt3IUlYXeJocDkVvbIcLCDkMFQpw7IQzmU8d8BXAxbbBLw";
            const char8_t* exp = "AQAB";

            JwtUtil::SignatureVerificationData verification;
            verification.sig = (uint8_t*)sig;
            verification.sigSize = strlen(sig);
            verification.msg = (uint8_t*)msg;
            verification.msgSize = strlen(msg);
            verification.mod = (uint8_t*)mod;
            verification.modSize = strlen(mod);
            verification.exp = (uint8_t*)exp;
            verification.expSize = strlen(exp);
            BlazeRpcError result = jwtUtil.verifyJwtSignature(verification);
            EXPECT_TRUE(result == ERR_OK) << "all valid";
        }

        //test case: invalid msg cannot be verified
        {
            const char8_t* sig = "aW9Ac9DeZrNszuwt1ARznST-fYU7lDToco4MyZyIZ__9I6gGlwebZmXd3VtwOGtgL1UTq3PIB096R7pBeCnp8xspCRLxfC1KXM5bbZBKPSE3xLfqEb5_ZKYLRzG5Gax7NLrU_5if11PsVeDM7W5RJMmH7GECU9bEVaoDL0V9y9lgsgc-1UbZ512SdIGZ1CQ9XNeiEl-53oxDx_0OLHynrKgUNT1QEi_yT3vxvO9oUz7cpqZV3dtOCQBaE4E5uO_8ib3rq-b-JtB_Slmk2XC_lt2mDr3hr_xAgv2jxiDgDqMl2SGYlo3nCz26x0m2TYoljKru1cAn40qtLCXooAvRPQ";
            //payload content is manipulated
            const char8_t* invalidMsg = "eyJraWQiOiJlMjIwODg4Yy03ZWU0LTQ5ZWUtOWUzMi02YTkzOWRjYmU3M2UiLCJhbGciOiJSUzI1NiJ9.eyJpc3MiOiJhY2NvdW50cy5pbnQuZWEuY29tIiwianRpIjoiU2tFd09qTXVNRG94TGpBNll6STNNVEk0T0RndE5HSm1NeTAwT0dZeUxXSTNPRGt0WmpZNU5XWmtOakE1WVRnMSIsImF6cCI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwiaWF0IjoxNjA4NzEwNDI0LCJleHAiOjE2MDk3MjQ4MjQsInZlciI6MSwibmV4dXMiOnsicnN2ZCI6eyJlZnBsdHkiOiI1In0sImNsaSI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwic2NvIjoiZ3NfcW9zX2Nvb3JkaW5hdG9yX3RydXN0ZWQgZ3NfY2NzX2NsaWVudCBnb3NfdG9vbHNfYXBpX3VzZXIgZ29zX25vdGlmeV9wb3N0X2V2ZW50IHNlYXJjaC5pZGVudGl0eSBiYXNpYy51dGlsaXR5IGJhc2ljLmVudGl0bGVtZW50LndyaXRlIHBsYXllcm93bmVyc2hpcC5yZWFkIGdzX21lc3NhZ2VidXNfcHJvZHVjZXIgYmFzaWMuYWNjZXNzLnRydXN0ZWQgZ29zX2ZyaWVuZHNfZW5kdXNlciBvZmZsaW5lIGJ5dGV2YXVsdCBiYXNpYy54YmxzZXJ2ZXJ0b2tlbiBkcC5pZGVudGl0eS5wc25zZXJ2ZXJ0b2tlbi5yIGRwLnBsYXllcnNldHRpbmdzLnNldHRpbmdzLnIgYmFzaWMucHNudG9rZW4gZ3Nfc3RhdHNfZW5kdXNlciBiYXNpYy54Ymx0b2tlbiBiYXNpYy5wZXJzb25hLndyaXRlIGJhc2ljLmVudGl0bGVtZW50IGJhc2ljLmRvbWFpbmRhdGEgYmFzaWMuaWRlbnRpdHkud3JpdGUgYmFzaWMucGVyc29uYWV4dGVybmFscmVmIGRwLmNvbW1lcmNlLnJlZnJlc2hleHRlbnRpdGxlbWVudC5ydyBiYXNpYy5wcmVmZXJlbmNlLndyaXRlIGdvc19ncm91cF9lbmR1c2VyIGJhc2ljLmFjY2VzcyBzb2NpYWxfcmVjb21tZW5kYXRpb25fdXNlciBiYXNpYy5wcmVmZXJlbmNlIGJhc2ljLmlkZW50aXR5IGJhc2ljLnBlcnNvbmEgZ29zX2dyb3VwX2ludGVncmF0b3IiLCJwaWQiOiIxMDAwMDQxODk5MjYyIiwicHR5IjoiUFMzIiwidWlkIjoiMTAwMDA0NTE5OTI2MiIsInBzaWQiOjEwMDAwNDEwOTkyNjIsImNzZXYiOiJJTlQiLCJkdmlkIjoiMDAwMDAwMDcwMDA3MDBjMDAwMDEwMTgxMDAxMDAwMDA1ZjE5YWIzMjEzODhjY2NhYjUxYjcxZTdiYTY2NzlhMCIsInBsdHlwIjoiUFM0Iiwic3RwcyI6Ik9GRiIsInVkZyI6ZmFsc2UsImNudHkiOiIxIiwiYXVzcmMiOiIxMDAwMDMiLCJpcGdlbyI6eyJpcCI6IjE1OS4xNTMuKi4qIiwiY3R5IjoiQ0EiLCJyZWciOiJCcml0aXNoIENvbHVtYmlhIiwiY2l0IjoiVmFuY291dmVyIiwiaXNwIjoiRWxlY3Ryb25pYyBBcnRzICBJbmMuIn0sImVuYyI6Ik1TWWlQbi9TV2YzOWUyeXhXSUlhKzgxMi9pU3pZZlZDZkluai9ORzgzck0vZVVOSm81YUUzalU0SVVjdTk1RFlnS0ozUk9vdDJxdUNFbGlWVUJIbmRzNnh1NEJ3OUVGRGJKT3pNMFptV3FLUmpkM1ZmRVRMWEtiaWt2QjdhYSs5UjJYdU4xdmVCd2Z4M1lZRUZPUnRPdW9ROWFhVDh2RElmMnNFZzB0SXQ4dGlGazJnT3RlR1pyWjZxRUVYWkZHc2ZMY2ZBZFBRbS9zSjJlVEV1WDRnTnR4cU9naG9zTmM5Y05RcUNmOUVSZ3pNNm9RVFdVb0xVVVZHZDNUY25TMWdJN3JtNHNMTVY2SmNYVGtjbWhOdG5CMkFYYzZxQUpFaS9JcjhuZnVvWjRkUjNNYzQ4YmZ3by9UaEc0WnRIS2xwMHdHVkpPQTNRV1JoTUdIWVNtSGtwMTQxcnB4TGhsKy9SWGdRTnpJQW5OTHRlVThuZEpzOWQ0VjhCcFhYS2tzM0RxdUd5NHhHRWhZRHNCV1RUd3hLU3hxZ3NENTJnbHptd3JiR1E1V2xJU0xhem4xM1YzSGI2NE1qeDh5VzRUVmxUbkI1ZzNxWEU4dE5ESjhucWNIUTltdE9FZG1MS1BLaWhvMjg3amk1QVg5cUpDQ3dXVXFwY2U2S2pYYm1wclAzNW9SWlFoTnZYdzVKZkhWTUk1MEpzZENxMHp4eFRSb0dIMWZ0Z1Q5b3JvZnMvQVJtT2pHQlY2cGs1bFFkcWxodlNBdFBNL3hab3V1TnNXbWVWak9CclJPYkN4dDE0ektrZUxFMGpEc0t3cmhxdWN6RW52WnZTRnlmUXhoeW1BZC9hckdGSTJ3VGliSFZoYU5PeHJoa0tMTVFYYXZJNHRtSytObTFHT3RmNlFoY2xRcjRiTVV3MzBaRGNzb3pTWS9yWCtBSndJZzNSaUVDR1d6dzJ4b2ZCbFVHSWtpa2JvUHJzd25vZnJuclJuVEw3c1RWWUYzcS9iR3NjY0tkNlVZV2VrUFNmL3F2RUg4QS9Rb3NhMXliRHcya3BpdW9lU2RLc3huM3NXeTgrSnF2alR2UjdYRFpBa0oyVjBKVjVNVVJzS0p6MXdBUVV1eG9hc3I2SmJOS0FTU0ZQY0ZFTzlwY1NDR2ZIZDBSK0Q2Zm9sN256K2dPeGNySEFTdm5CRldDMDZ6bXZmWEVFdEJzbnlzanlPakwzMGM3TWJzd0NsMk1vTHJQRkFzdTU5OFNScklpOCt4ZmVDVTM0TDdXTlFNWWhkbEd2Y05TMFByb3ViV0FoM0F5NEtpTXo0cHJpaUo3U2hydXloNFRIeDRJMW9mT2xHR0IrSUhKMWt1aVdHMzBJNHRJVkRQTFZMb1NyNnBxVWlIRkhWZ0s1RVNDaFJUNE5vaVVwRy90dkE1T0dtODNrUzlQdnBrb1Y2TnEwNVFsaFhXVWNqRkRVMzhPRDllZkFxN1g1RlhQc2N5N2xoTVN5SmtTSnJVOVlXU01VOTRUMFc0UytlbVEvR28yR3ZoVDNwaFh3dE45OE1SaitFNEt2MDA3QVcrbXJhR2RWck1RVUVhMnBnNjJyYkNHcEhTaTZzb0YxR1h5cHBhbnBCcVF2MXJ1UERFbHdqNTFYenc2cTlndlB2ZzlxQU5KRTQxZUsva0dDS2djVjN5WHc0SnByVEFNdEdyY1Iwa0lpb3pLK05hdG5ndUdVRFc5bVUrSVRpbHNwUG9nckgrSWVnR0prNEFhYzV6UFgxY09jeXcvOW1MbU5Kc01tYWxaUTQydnhuSFFsM2ZiWmRld0VZQnNNS0M4QkdESlVZb3R0VmFyNXZWNnlGTWxJTU44UTFNNFEvWWNaSjRKbHFXNEVaTUkifX0";
            const char8_t* mod = "ny7FHSTmfpHefqyxDTFkPDJuokxR7y92MtFmf76cNGYYdF1w-sRGMvjuRnZyiiZTD4SAdm6dciLyKaR3BoTX5HuWURuqefZqMkhfduFS8q4kRv-f995hKmIdbnWXPQNDu4HxU4wTha2wCvOaYj0Yx0j-njiMrU7_YeYABRSLlulaVkMes3T32ToPVxBTGoI3lsL58QgQDzzDXHIi4dYH_mr98Rhxz4lDYVzcu5kE702AFowxHIAPWZMZ7N2wAmXgv8xcNlJ3agX9gcVb9_caGvF5BVPUnW2d005TzCnFbt3IUlYXeJocDkVvbIcLCDkMFQpw7IQzmU8d8BXAxbbBLw";
            const char8_t* exp = "AQAB";

            JwtUtil::SignatureVerificationData verification;
            verification.sig = (uint8_t*)sig;
            verification.sigSize = strlen(sig);
            verification.msg = (uint8_t*)invalidMsg;
            verification.msgSize = strlen(invalidMsg);
            verification.mod = (uint8_t*)mod;
            verification.modSize = strlen(mod);
            verification.exp = (uint8_t*)exp;
            verification.expSize = strlen(exp);
            BlazeRpcError result = jwtUtil.verifyJwtSignature(verification);
            EXPECT_TRUE(result != ERR_OK) << "invalid msg";
        }

        //test case: invalid sig cannot be verified
        {
            //sig is manipulated
            const char8_t* invalidSig = "aW9Ac9DeZrNszuwt1ARznST-fYU7lDToco4MyZyIZ__9I6gGlwebZmXd3VtwOGtgL1UTq3PIB096R7pBeCnp8xspCRLxfC1KXM5bbZBKPSE3xLfqEb5_ZKYLRzG5Gax7NLrU_5if11PsVeDM7W5RJMmH7GECU9bEVaoDL0V9y9lgsgc-1UbZ512SdIGZ1CQ9XNeiEl-53oxDx_0OLHynrKgUNT1QEi_yT3vxvO9oUz7cpqZV3dtOCQBaE4E5uO_8ib3rq-b-JtB_Slmk2XC_lt2mDr3hr_xAgv2jxiDgDqMl2SGYlo3nCz26x0m2TYoljKru1cAn40qtLCXooAvRP0";
            const char8_t* msg = "eyJraWQiOiJlMjIwODg4Yy03ZWU0LTQ5ZWUtOWUzMi02YTkzOWRjYmU3M2UiLCJhbGciOiJSUzI1NiJ9.eyJpc3MiOiJhY2NvdW50cy5pbnQuZWEuY29tIiwianRpIjoiU2tFd09qTXVNRG94TGpBNll6STNNVEk0T0RndE5HSm1NeTAwT0dZeUxXSTNPRGt0WmpZNU5XWmtOakE1WVRnMSIsImF6cCI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwiaWF0IjoxNjA4NzEwNDI0LCJleHAiOjE2MDg3MjQ4MjQsInZlciI6MSwibmV4dXMiOnsicnN2ZCI6eyJlZnBsdHkiOiI1In0sImNsaSI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwic2NvIjoiZ3NfcW9zX2Nvb3JkaW5hdG9yX3RydXN0ZWQgZ3NfY2NzX2NsaWVudCBnb3NfdG9vbHNfYXBpX3VzZXIgZ29zX25vdGlmeV9wb3N0X2V2ZW50IHNlYXJjaC5pZGVudGl0eSBiYXNpYy51dGlsaXR5IGJhc2ljLmVudGl0bGVtZW50LndyaXRlIHBsYXllcm93bmVyc2hpcC5yZWFkIGdzX21lc3NhZ2VidXNfcHJvZHVjZXIgYmFzaWMuYWNjZXNzLnRydXN0ZWQgZ29zX2ZyaWVuZHNfZW5kdXNlciBvZmZsaW5lIGJ5dGV2YXVsdCBiYXNpYy54YmxzZXJ2ZXJ0b2tlbiBkcC5pZGVudGl0eS5wc25zZXJ2ZXJ0b2tlbi5yIGRwLnBsYXllcnNldHRpbmdzLnNldHRpbmdzLnIgYmFzaWMucHNudG9rZW4gZ3Nfc3RhdHNfZW5kdXNlciBiYXNpYy54Ymx0b2tlbiBiYXNpYy5wZXJzb25hLndyaXRlIGJhc2ljLmVudGl0bGVtZW50IGJhc2ljLmRvbWFpbmRhdGEgYmFzaWMuaWRlbnRpdHkud3JpdGUgYmFzaWMucGVyc29uYWV4dGVybmFscmVmIGRwLmNvbW1lcmNlLnJlZnJlc2hleHRlbnRpdGxlbWVudC5ydyBiYXNpYy5wcmVmZXJlbmNlLndyaXRlIGdvc19ncm91cF9lbmR1c2VyIGJhc2ljLmFjY2VzcyBzb2NpYWxfcmVjb21tZW5kYXRpb25fdXNlciBiYXNpYy5wcmVmZXJlbmNlIGJhc2ljLmlkZW50aXR5IGJhc2ljLnBlcnNvbmEgZ29zX2dyb3VwX2ludGVncmF0b3IiLCJwaWQiOiIxMDAwMDQxODk5MjYyIiwicHR5IjoiUFMzIiwidWlkIjoiMTAwMDA0NTE5OTI2MiIsInBzaWQiOjEwMDAwNDEwOTkyNjIsImNzZXYiOiJJTlQiLCJkdmlkIjoiMDAwMDAwMDcwMDA3MDBjMDAwMDEwMTgxMDAxMDAwMDA1ZjE5YWIzMjEzODhjY2NhYjUxYjcxZTdiYTY2NzlhMCIsInBsdHlwIjoiUFM0Iiwic3RwcyI6Ik9GRiIsInVkZyI6ZmFsc2UsImNudHkiOiIxIiwiYXVzcmMiOiIxMDAwMDMiLCJpcGdlbyI6eyJpcCI6IjE1OS4xNTMuKi4qIiwiY3R5IjoiQ0EiLCJyZWciOiJCcml0aXNoIENvbHVtYmlhIiwiY2l0IjoiVmFuY291dmVyIiwiaXNwIjoiRWxlY3Ryb25pYyBBcnRzICBJbmMuIn0sImVuYyI6Ik1TWWlQbi9TV2YzOWUyeXhXSUlhKzgxMi9pU3pZZlZDZkluai9ORzgzck0vZVVOSm81YUUzalU0SVVjdTk1RFlnS0ozUk9vdDJxdUNFbGlWVUJIbmRzNnh1NEJ3OUVGRGJKT3pNMFptV3FLUmpkM1ZmRVRMWEtiaWt2QjdhYSs5UjJYdU4xdmVCd2Z4M1lZRUZPUnRPdW9ROWFhVDh2RElmMnNFZzB0SXQ4dGlGazJnT3RlR1pyWjZxRUVYWkZHc2ZMY2ZBZFBRbS9zSjJlVEV1WDRnTnR4cU9naG9zTmM5Y05RcUNmOUVSZ3pNNm9RVFdVb0xVVVZHZDNUY25TMWdJN3JtNHNMTVY2SmNYVGtjbWhOdG5CMkFYYzZxQUpFaS9JcjhuZnVvWjRkUjNNYzQ4YmZ3by9UaEc0WnRIS2xwMHdHVkpPQTNRV1JoTUdIWVNtSGtwMTQxcnB4TGhsKy9SWGdRTnpJQW5OTHRlVThuZEpzOWQ0VjhCcFhYS2tzM0RxdUd5NHhHRWhZRHNCV1RUd3hLU3hxZ3NENTJnbHptd3JiR1E1V2xJU0xhem4xM1YzSGI2NE1qeDh5VzRUVmxUbkI1ZzNxWEU4dE5ESjhucWNIUTltdE9FZG1MS1BLaWhvMjg3amk1QVg5cUpDQ3dXVXFwY2U2S2pYYm1wclAzNW9SWlFoTnZYdzVKZkhWTUk1MEpzZENxMHp4eFRSb0dIMWZ0Z1Q5b3JvZnMvQVJtT2pHQlY2cGs1bFFkcWxodlNBdFBNL3hab3V1TnNXbWVWak9CclJPYkN4dDE0ektrZUxFMGpEc0t3cmhxdWN6RW52WnZTRnlmUXhoeW1BZC9hckdGSTJ3VGliSFZoYU5PeHJoa0tMTVFYYXZJNHRtSytObTFHT3RmNlFoY2xRcjRiTVV3MzBaRGNzb3pTWS9yWCtBSndJZzNSaUVDR1d6dzJ4b2ZCbFVHSWtpa2JvUHJzd25vZnJuclJuVEw3c1RWWUYzcS9iR3NjY0tkNlVZV2VrUFNmL3F2RUg4QS9Rb3NhMXliRHcya3BpdW9lU2RLc3huM3NXeTgrSnF2alR2UjdYRFpBa0oyVjBKVjVNVVJzS0p6MXdBUVV1eG9hc3I2SmJOS0FTU0ZQY0ZFTzlwY1NDR2ZIZDBSK0Q2Zm9sN256K2dPeGNySEFTdm5CRldDMDZ6bXZmWEVFdEJzbnlzanlPakwzMGM3TWJzd0NsMk1vTHJQRkFzdTU5OFNScklpOCt4ZmVDVTM0TDdXTlFNWWhkbEd2Y05TMFByb3ViV0FoM0F5NEtpTXo0cHJpaUo3U2hydXloNFRIeDRJMW9mT2xHR0IrSUhKMWt1aVdHMzBJNHRJVkRQTFZMb1NyNnBxVWlIRkhWZ0s1RVNDaFJUNE5vaVVwRy90dkE1T0dtODNrUzlQdnBrb1Y2TnEwNVFsaFhXVWNqRkRVMzhPRDllZkFxN1g1RlhQc2N5N2xoTVN5SmtTSnJVOVlXU01VOTRUMFc0UytlbVEvR28yR3ZoVDNwaFh3dE45OE1SaitFNEt2MDA3QVcrbXJhR2RWck1RVUVhMnBnNjJyYkNHcEhTaTZzb0YxR1h5cHBhbnBCcVF2MXJ1UERFbHdqNTFYenc2cTlndlB2ZzlxQU5KRTQxZUsva0dDS2djVjN5WHc0SnByVEFNdEdyY1Iwa0lpb3pLK05hdG5ndUdVRFc5bVUrSVRpbHNwUG9nckgrSWVnR0prNEFhYzV6UFgxY09jeXcvOW1MbU5Kc01tYWxaUTQydnhuSFFsM2ZiWmRld0VZQnNNS0M4QkdESlVZb3R0VmFyNXZWNnlGTWxJTU44UTFNNFEvWWNaSjRKbHFXNEVaTUkifX0";
            const char8_t* mod = "ny7FHSTmfpHefqyxDTFkPDJuokxR7y92MtFmf76cNGYYdF1w-sRGMvjuRnZyiiZTD4SAdm6dciLyKaR3BoTX5HuWURuqefZqMkhfduFS8q4kRv-f995hKmIdbnWXPQNDu4HxU4wTha2wCvOaYj0Yx0j-njiMrU7_YeYABRSLlulaVkMes3T32ToPVxBTGoI3lsL58QgQDzzDXHIi4dYH_mr98Rhxz4lDYVzcu5kE702AFowxHIAPWZMZ7N2wAmXgv8xcNlJ3agX9gcVb9_caGvF5BVPUnW2d005TzCnFbt3IUlYXeJocDkVvbIcLCDkMFQpw7IQzmU8d8BXAxbbBLw";
            const char8_t* exp = "AQAB";

            JwtUtil::SignatureVerificationData verification;
            verification.sig = (uint8_t*)invalidSig;
            verification.sigSize = strlen(invalidSig);
            verification.msg = (uint8_t*)msg;
            verification.msgSize = strlen(msg);
            verification.mod = (uint8_t*)mod;
            verification.modSize = strlen(mod);
            verification.exp = (uint8_t*)exp;
            verification.expSize = strlen(exp);
            BlazeRpcError result = jwtUtil.verifyJwtSignature(verification);
            EXPECT_TRUE(result != ERR_OK) << "invalid sig";
        }

        //test case: invalid mod cannot be verified
        {
            const char8_t* sig = "aW9Ac9DeZrNszuwt1ARznST-fYU7lDToco4MyZyIZ__9I6gGlwebZmXd3VtwOGtgL1UTq3PIB096R7pBeCnp8xspCRLxfC1KXM5bbZBKPSE3xLfqEb5_ZKYLRzG5Gax7NLrU_5if11PsVeDM7W5RJMmH7GECU9bEVaoDL0V9y9lgsgc-1UbZ512SdIGZ1CQ9XNeiEl-53oxDx_0OLHynrKgUNT1QEi_yT3vxvO9oUz7cpqZV3dtOCQBaE4E5uO_8ib3rq-b-JtB_Slmk2XC_lt2mDr3hr_xAgv2jxiDgDqMl2SGYlo3nCz26x0m2TYoljKru1cAn40qtLCXooAvRPQ";
            const char8_t* msg = "eyJraWQiOiJlMjIwODg4Yy03ZWU0LTQ5ZWUtOWUzMi02YTkzOWRjYmU3M2UiLCJhbGciOiJSUzI1NiJ9.eyJpc3MiOiJhY2NvdW50cy5pbnQuZWEuY29tIiwianRpIjoiU2tFd09qTXVNRG94TGpBNll6STNNVEk0T0RndE5HSm1NeTAwT0dZeUxXSTNPRGt0WmpZNU5XWmtOakE1WVRnMSIsImF6cCI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwiaWF0IjoxNjA4NzEwNDI0LCJleHAiOjE2MDg3MjQ4MjQsInZlciI6MSwibmV4dXMiOnsicnN2ZCI6eyJlZnBsdHkiOiI1In0sImNsaSI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwic2NvIjoiZ3NfcW9zX2Nvb3JkaW5hdG9yX3RydXN0ZWQgZ3NfY2NzX2NsaWVudCBnb3NfdG9vbHNfYXBpX3VzZXIgZ29zX25vdGlmeV9wb3N0X2V2ZW50IHNlYXJjaC5pZGVudGl0eSBiYXNpYy51dGlsaXR5IGJhc2ljLmVudGl0bGVtZW50LndyaXRlIHBsYXllcm93bmVyc2hpcC5yZWFkIGdzX21lc3NhZ2VidXNfcHJvZHVjZXIgYmFzaWMuYWNjZXNzLnRydXN0ZWQgZ29zX2ZyaWVuZHNfZW5kdXNlciBvZmZsaW5lIGJ5dGV2YXVsdCBiYXNpYy54YmxzZXJ2ZXJ0b2tlbiBkcC5pZGVudGl0eS5wc25zZXJ2ZXJ0b2tlbi5yIGRwLnBsYXllcnNldHRpbmdzLnNldHRpbmdzLnIgYmFzaWMucHNudG9rZW4gZ3Nfc3RhdHNfZW5kdXNlciBiYXNpYy54Ymx0b2tlbiBiYXNpYy5wZXJzb25hLndyaXRlIGJhc2ljLmVudGl0bGVtZW50IGJhc2ljLmRvbWFpbmRhdGEgYmFzaWMuaWRlbnRpdHkud3JpdGUgYmFzaWMucGVyc29uYWV4dGVybmFscmVmIGRwLmNvbW1lcmNlLnJlZnJlc2hleHRlbnRpdGxlbWVudC5ydyBiYXNpYy5wcmVmZXJlbmNlLndyaXRlIGdvc19ncm91cF9lbmR1c2VyIGJhc2ljLmFjY2VzcyBzb2NpYWxfcmVjb21tZW5kYXRpb25fdXNlciBiYXNpYy5wcmVmZXJlbmNlIGJhc2ljLmlkZW50aXR5IGJhc2ljLnBlcnNvbmEgZ29zX2dyb3VwX2ludGVncmF0b3IiLCJwaWQiOiIxMDAwMDQxODk5MjYyIiwicHR5IjoiUFMzIiwidWlkIjoiMTAwMDA0NTE5OTI2MiIsInBzaWQiOjEwMDAwNDEwOTkyNjIsImNzZXYiOiJJTlQiLCJkdmlkIjoiMDAwMDAwMDcwMDA3MDBjMDAwMDEwMTgxMDAxMDAwMDA1ZjE5YWIzMjEzODhjY2NhYjUxYjcxZTdiYTY2NzlhMCIsInBsdHlwIjoiUFM0Iiwic3RwcyI6Ik9GRiIsInVkZyI6ZmFsc2UsImNudHkiOiIxIiwiYXVzcmMiOiIxMDAwMDMiLCJpcGdlbyI6eyJpcCI6IjE1OS4xNTMuKi4qIiwiY3R5IjoiQ0EiLCJyZWciOiJCcml0aXNoIENvbHVtYmlhIiwiY2l0IjoiVmFuY291dmVyIiwiaXNwIjoiRWxlY3Ryb25pYyBBcnRzICBJbmMuIn0sImVuYyI6Ik1TWWlQbi9TV2YzOWUyeXhXSUlhKzgxMi9pU3pZZlZDZkluai9ORzgzck0vZVVOSm81YUUzalU0SVVjdTk1RFlnS0ozUk9vdDJxdUNFbGlWVUJIbmRzNnh1NEJ3OUVGRGJKT3pNMFptV3FLUmpkM1ZmRVRMWEtiaWt2QjdhYSs5UjJYdU4xdmVCd2Z4M1lZRUZPUnRPdW9ROWFhVDh2RElmMnNFZzB0SXQ4dGlGazJnT3RlR1pyWjZxRUVYWkZHc2ZMY2ZBZFBRbS9zSjJlVEV1WDRnTnR4cU9naG9zTmM5Y05RcUNmOUVSZ3pNNm9RVFdVb0xVVVZHZDNUY25TMWdJN3JtNHNMTVY2SmNYVGtjbWhOdG5CMkFYYzZxQUpFaS9JcjhuZnVvWjRkUjNNYzQ4YmZ3by9UaEc0WnRIS2xwMHdHVkpPQTNRV1JoTUdIWVNtSGtwMTQxcnB4TGhsKy9SWGdRTnpJQW5OTHRlVThuZEpzOWQ0VjhCcFhYS2tzM0RxdUd5NHhHRWhZRHNCV1RUd3hLU3hxZ3NENTJnbHptd3JiR1E1V2xJU0xhem4xM1YzSGI2NE1qeDh5VzRUVmxUbkI1ZzNxWEU4dE5ESjhucWNIUTltdE9FZG1MS1BLaWhvMjg3amk1QVg5cUpDQ3dXVXFwY2U2S2pYYm1wclAzNW9SWlFoTnZYdzVKZkhWTUk1MEpzZENxMHp4eFRSb0dIMWZ0Z1Q5b3JvZnMvQVJtT2pHQlY2cGs1bFFkcWxodlNBdFBNL3hab3V1TnNXbWVWak9CclJPYkN4dDE0ektrZUxFMGpEc0t3cmhxdWN6RW52WnZTRnlmUXhoeW1BZC9hckdGSTJ3VGliSFZoYU5PeHJoa0tMTVFYYXZJNHRtSytObTFHT3RmNlFoY2xRcjRiTVV3MzBaRGNzb3pTWS9yWCtBSndJZzNSaUVDR1d6dzJ4b2ZCbFVHSWtpa2JvUHJzd25vZnJuclJuVEw3c1RWWUYzcS9iR3NjY0tkNlVZV2VrUFNmL3F2RUg4QS9Rb3NhMXliRHcya3BpdW9lU2RLc3huM3NXeTgrSnF2alR2UjdYRFpBa0oyVjBKVjVNVVJzS0p6MXdBUVV1eG9hc3I2SmJOS0FTU0ZQY0ZFTzlwY1NDR2ZIZDBSK0Q2Zm9sN256K2dPeGNySEFTdm5CRldDMDZ6bXZmWEVFdEJzbnlzanlPakwzMGM3TWJzd0NsMk1vTHJQRkFzdTU5OFNScklpOCt4ZmVDVTM0TDdXTlFNWWhkbEd2Y05TMFByb3ViV0FoM0F5NEtpTXo0cHJpaUo3U2hydXloNFRIeDRJMW9mT2xHR0IrSUhKMWt1aVdHMzBJNHRJVkRQTFZMb1NyNnBxVWlIRkhWZ0s1RVNDaFJUNE5vaVVwRy90dkE1T0dtODNrUzlQdnBrb1Y2TnEwNVFsaFhXVWNqRkRVMzhPRDllZkFxN1g1RlhQc2N5N2xoTVN5SmtTSnJVOVlXU01VOTRUMFc0UytlbVEvR28yR3ZoVDNwaFh3dE45OE1SaitFNEt2MDA3QVcrbXJhR2RWck1RVUVhMnBnNjJyYkNHcEhTaTZzb0YxR1h5cHBhbnBCcVF2MXJ1UERFbHdqNTFYenc2cTlndlB2ZzlxQU5KRTQxZUsva0dDS2djVjN5WHc0SnByVEFNdEdyY1Iwa0lpb3pLK05hdG5ndUdVRFc5bVUrSVRpbHNwUG9nckgrSWVnR0prNEFhYzV6UFgxY09jeXcvOW1MbU5Kc01tYWxaUTQydnhuSFFsM2ZiWmRld0VZQnNNS0M4QkdESlVZb3R0VmFyNXZWNnlGTWxJTU44UTFNNFEvWWNaSjRKbHFXNEVaTUkifX0";
            const char8_t* invalidMod = "ny7FHSTmfpHefqyxDTFkPDJuokxR7y92MtFmf76cNGYYdF1w-sRGMvjuRnZyiiZTD4SAdm6dciLyKaR3BoTX5HuWURuqefZqMkhfduFS8q4kRv-f995hKmIdbnWXPQNDu4HxU4wTha2wCvOaYj0Yx0j-njiMrU7_YeYABRSLlulaVkMes3T32ToPVxBTGoI3lsL58QgQDzzDXHIi4dYH_mr98Rhxz4lDYVzcu5kE702AFowxHIAPWZMZ7N2wAmXgv8xcNlJ3agX9gcVb9_caGvF5BVPUnW2d005TzCnFbt3IUlYXeJocDkVvbIcLCDkMFQpw7IQzmU8d8BXAxbbBL";
            const char8_t* exp = "AQAB";

            JwtUtil::SignatureVerificationData verification;
            verification.sig = (uint8_t*)sig;
            verification.sigSize = strlen(sig);
            verification.msg = (uint8_t*)msg;
            verification.msgSize = strlen(msg);
            verification.mod = (uint8_t*)invalidMod;
            verification.modSize = strlen(invalidMod);
            verification.exp = (uint8_t*)exp;
            verification.expSize = strlen(exp);
            BlazeRpcError result = jwtUtil.verifyJwtSignature(verification);
            EXPECT_TRUE(result != ERR_OK) << "invalid mod";
        }

        //test case: invalid exp cannot be verified
        {
            const char8_t* sig = "aW9Ac9DeZrNszuwt1ARznST-fYU7lDToco4MyZyIZ__9I6gGlwebZmXd3VtwOGtgL1UTq3PIB096R7pBeCnp8xspCRLxfC1KXM5bbZBKPSE3xLfqEb5_ZKYLRzG5Gax7NLrU_5if11PsVeDM7W5RJMmH7GECU9bEVaoDL0V9y9lgsgc-1UbZ512SdIGZ1CQ9XNeiEl-53oxDx_0OLHynrKgUNT1QEi_yT3vxvO9oUz7cpqZV3dtOCQBaE4E5uO_8ib3rq-b-JtB_Slmk2XC_lt2mDr3hr_xAgv2jxiDgDqMl2SGYlo3nCz26x0m2TYoljKru1cAn40qtLCXooAvRPQ";
            const char8_t* msg = "eyJraWQiOiJlMjIwODg4Yy03ZWU0LTQ5ZWUtOWUzMi02YTkzOWRjYmU3M2UiLCJhbGciOiJSUzI1NiJ9.eyJpc3MiOiJhY2NvdW50cy5pbnQuZWEuY29tIiwianRpIjoiU2tFd09qTXVNRG94TGpBNll6STNNVEk0T0RndE5HSm1NeTAwT0dZeUxXSTNPRGt0WmpZNU5XWmtOakE1WVRnMSIsImF6cCI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwiaWF0IjoxNjA4NzEwNDI0LCJleHAiOjE2MDg3MjQ4MjQsInZlciI6MSwibmV4dXMiOnsicnN2ZCI6eyJlZnBsdHkiOiI1In0sImNsaSI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwic2NvIjoiZ3NfcW9zX2Nvb3JkaW5hdG9yX3RydXN0ZWQgZ3NfY2NzX2NsaWVudCBnb3NfdG9vbHNfYXBpX3VzZXIgZ29zX25vdGlmeV9wb3N0X2V2ZW50IHNlYXJjaC5pZGVudGl0eSBiYXNpYy51dGlsaXR5IGJhc2ljLmVudGl0bGVtZW50LndyaXRlIHBsYXllcm93bmVyc2hpcC5yZWFkIGdzX21lc3NhZ2VidXNfcHJvZHVjZXIgYmFzaWMuYWNjZXNzLnRydXN0ZWQgZ29zX2ZyaWVuZHNfZW5kdXNlciBvZmZsaW5lIGJ5dGV2YXVsdCBiYXNpYy54YmxzZXJ2ZXJ0b2tlbiBkcC5pZGVudGl0eS5wc25zZXJ2ZXJ0b2tlbi5yIGRwLnBsYXllcnNldHRpbmdzLnNldHRpbmdzLnIgYmFzaWMucHNudG9rZW4gZ3Nfc3RhdHNfZW5kdXNlciBiYXNpYy54Ymx0b2tlbiBiYXNpYy5wZXJzb25hLndyaXRlIGJhc2ljLmVudGl0bGVtZW50IGJhc2ljLmRvbWFpbmRhdGEgYmFzaWMuaWRlbnRpdHkud3JpdGUgYmFzaWMucGVyc29uYWV4dGVybmFscmVmIGRwLmNvbW1lcmNlLnJlZnJlc2hleHRlbnRpdGxlbWVudC5ydyBiYXNpYy5wcmVmZXJlbmNlLndyaXRlIGdvc19ncm91cF9lbmR1c2VyIGJhc2ljLmFjY2VzcyBzb2NpYWxfcmVjb21tZW5kYXRpb25fdXNlciBiYXNpYy5wcmVmZXJlbmNlIGJhc2ljLmlkZW50aXR5IGJhc2ljLnBlcnNvbmEgZ29zX2dyb3VwX2ludGVncmF0b3IiLCJwaWQiOiIxMDAwMDQxODk5MjYyIiwicHR5IjoiUFMzIiwidWlkIjoiMTAwMDA0NTE5OTI2MiIsInBzaWQiOjEwMDAwNDEwOTkyNjIsImNzZXYiOiJJTlQiLCJkdmlkIjoiMDAwMDAwMDcwMDA3MDBjMDAwMDEwMTgxMDAxMDAwMDA1ZjE5YWIzMjEzODhjY2NhYjUxYjcxZTdiYTY2NzlhMCIsInBsdHlwIjoiUFM0Iiwic3RwcyI6Ik9GRiIsInVkZyI6ZmFsc2UsImNudHkiOiIxIiwiYXVzcmMiOiIxMDAwMDMiLCJpcGdlbyI6eyJpcCI6IjE1OS4xNTMuKi4qIiwiY3R5IjoiQ0EiLCJyZWciOiJCcml0aXNoIENvbHVtYmlhIiwiY2l0IjoiVmFuY291dmVyIiwiaXNwIjoiRWxlY3Ryb25pYyBBcnRzICBJbmMuIn0sImVuYyI6Ik1TWWlQbi9TV2YzOWUyeXhXSUlhKzgxMi9pU3pZZlZDZkluai9ORzgzck0vZVVOSm81YUUzalU0SVVjdTk1RFlnS0ozUk9vdDJxdUNFbGlWVUJIbmRzNnh1NEJ3OUVGRGJKT3pNMFptV3FLUmpkM1ZmRVRMWEtiaWt2QjdhYSs5UjJYdU4xdmVCd2Z4M1lZRUZPUnRPdW9ROWFhVDh2RElmMnNFZzB0SXQ4dGlGazJnT3RlR1pyWjZxRUVYWkZHc2ZMY2ZBZFBRbS9zSjJlVEV1WDRnTnR4cU9naG9zTmM5Y05RcUNmOUVSZ3pNNm9RVFdVb0xVVVZHZDNUY25TMWdJN3JtNHNMTVY2SmNYVGtjbWhOdG5CMkFYYzZxQUpFaS9JcjhuZnVvWjRkUjNNYzQ4YmZ3by9UaEc0WnRIS2xwMHdHVkpPQTNRV1JoTUdIWVNtSGtwMTQxcnB4TGhsKy9SWGdRTnpJQW5OTHRlVThuZEpzOWQ0VjhCcFhYS2tzM0RxdUd5NHhHRWhZRHNCV1RUd3hLU3hxZ3NENTJnbHptd3JiR1E1V2xJU0xhem4xM1YzSGI2NE1qeDh5VzRUVmxUbkI1ZzNxWEU4dE5ESjhucWNIUTltdE9FZG1MS1BLaWhvMjg3amk1QVg5cUpDQ3dXVXFwY2U2S2pYYm1wclAzNW9SWlFoTnZYdzVKZkhWTUk1MEpzZENxMHp4eFRSb0dIMWZ0Z1Q5b3JvZnMvQVJtT2pHQlY2cGs1bFFkcWxodlNBdFBNL3hab3V1TnNXbWVWak9CclJPYkN4dDE0ektrZUxFMGpEc0t3cmhxdWN6RW52WnZTRnlmUXhoeW1BZC9hckdGSTJ3VGliSFZoYU5PeHJoa0tMTVFYYXZJNHRtSytObTFHT3RmNlFoY2xRcjRiTVV3MzBaRGNzb3pTWS9yWCtBSndJZzNSaUVDR1d6dzJ4b2ZCbFVHSWtpa2JvUHJzd25vZnJuclJuVEw3c1RWWUYzcS9iR3NjY0tkNlVZV2VrUFNmL3F2RUg4QS9Rb3NhMXliRHcya3BpdW9lU2RLc3huM3NXeTgrSnF2alR2UjdYRFpBa0oyVjBKVjVNVVJzS0p6MXdBUVV1eG9hc3I2SmJOS0FTU0ZQY0ZFTzlwY1NDR2ZIZDBSK0Q2Zm9sN256K2dPeGNySEFTdm5CRldDMDZ6bXZmWEVFdEJzbnlzanlPakwzMGM3TWJzd0NsMk1vTHJQRkFzdTU5OFNScklpOCt4ZmVDVTM0TDdXTlFNWWhkbEd2Y05TMFByb3ViV0FoM0F5NEtpTXo0cHJpaUo3U2hydXloNFRIeDRJMW9mT2xHR0IrSUhKMWt1aVdHMzBJNHRJVkRQTFZMb1NyNnBxVWlIRkhWZ0s1RVNDaFJUNE5vaVVwRy90dkE1T0dtODNrUzlQdnBrb1Y2TnEwNVFsaFhXVWNqRkRVMzhPRDllZkFxN1g1RlhQc2N5N2xoTVN5SmtTSnJVOVlXU01VOTRUMFc0UytlbVEvR28yR3ZoVDNwaFh3dE45OE1SaitFNEt2MDA3QVcrbXJhR2RWck1RVUVhMnBnNjJyYkNHcEhTaTZzb0YxR1h5cHBhbnBCcVF2MXJ1UERFbHdqNTFYenc2cTlndlB2ZzlxQU5KRTQxZUsva0dDS2djVjN5WHc0SnByVEFNdEdyY1Iwa0lpb3pLK05hdG5ndUdVRFc5bVUrSVRpbHNwUG9nckgrSWVnR0prNEFhYzV6UFgxY09jeXcvOW1MbU5Kc01tYWxaUTQydnhuSFFsM2ZiWmRld0VZQnNNS0M4QkdESlVZb3R0VmFyNXZWNnlGTWxJTU44UTFNNFEvWWNaSjRKbHFXNEVaTUkifX0";
            const char8_t* mod = "ny7FHSTmfpHefqyxDTFkPDJuokxR7y92MtFmf76cNGYYdF1w-sRGMvjuRnZyiiZTD4SAdm6dciLyKaR3BoTX5HuWURuqefZqMkhfduFS8q4kRv-f995hKmIdbnWXPQNDu4HxU4wTha2wCvOaYj0Yx0j-njiMrU7_YeYABRSLlulaVkMes3T32ToPVxBTGoI3lsL58QgQDzzDXHIi4dYH_mr98Rhxz4lDYVzcu5kE702AFowxHIAPWZMZ7N2wAmXgv8xcNlJ3agX9gcVb9_caGvF5BVPUnW2d005TzCnFbt3IUlYXeJocDkVvbIcLCDkMFQpw7IQzmU8d8BXAxbbBLw";
            const char8_t* invalidExp = "AQAC";

            JwtUtil::SignatureVerificationData verification;
            verification.sig = (uint8_t*)sig;
            verification.sigSize = strlen(sig);
            verification.msg = (uint8_t*)msg;
            verification.msgSize = strlen(msg);
            verification.mod = (uint8_t*)mod;
            verification.modSize = strlen(mod);
            verification.exp = (uint8_t*)invalidExp;
            verification.expSize = strlen(invalidExp);
            BlazeRpcError result = jwtUtil.verifyJwtSignature(verification);
            EXPECT_TRUE(result != ERR_OK) << "invalid exp";
        }

        //test case: invalid sig cannot be verified
        {
            const char8_t* invalidSig = "fakebase64urlencoding";
            const char8_t* msg = "eyJraWQiOiJlMjIwODg4Yy03ZWU0LTQ5ZWUtOWUzMi02YTkzOWRjYmU3M2UiLCJhbGciOiJSUzI1NiJ9.eyJpc3MiOiJhY2NvdW50cy5pbnQuZWEuY29tIiwianRpIjoiU2tFd09qTXVNRG94TGpBNll6STNNVEk0T0RndE5HSm1NeTAwT0dZeUxXSTNPRGt0WmpZNU5XWmtOakE1WVRnMSIsImF6cCI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwiaWF0IjoxNjA4NzEwNDI0LCJleHAiOjE2MDg3MjQ4MjQsInZlciI6MSwibmV4dXMiOnsicnN2ZCI6eyJlZnBsdHkiOiI1In0sImNsaSI6IkVBRFBfR1NfQkxaREVWX0NPTV9CTFpfU0VSVkVSIiwic2NvIjoiZ3NfcW9zX2Nvb3JkaW5hdG9yX3RydXN0ZWQgZ3NfY2NzX2NsaWVudCBnb3NfdG9vbHNfYXBpX3VzZXIgZ29zX25vdGlmeV9wb3N0X2V2ZW50IHNlYXJjaC5pZGVudGl0eSBiYXNpYy51dGlsaXR5IGJhc2ljLmVudGl0bGVtZW50LndyaXRlIHBsYXllcm93bmVyc2hpcC5yZWFkIGdzX21lc3NhZ2VidXNfcHJvZHVjZXIgYmFzaWMuYWNjZXNzLnRydXN0ZWQgZ29zX2ZyaWVuZHNfZW5kdXNlciBvZmZsaW5lIGJ5dGV2YXVsdCBiYXNpYy54YmxzZXJ2ZXJ0b2tlbiBkcC5pZGVudGl0eS5wc25zZXJ2ZXJ0b2tlbi5yIGRwLnBsYXllcnNldHRpbmdzLnNldHRpbmdzLnIgYmFzaWMucHNudG9rZW4gZ3Nfc3RhdHNfZW5kdXNlciBiYXNpYy54Ymx0b2tlbiBiYXNpYy5wZXJzb25hLndyaXRlIGJhc2ljLmVudGl0bGVtZW50IGJhc2ljLmRvbWFpbmRhdGEgYmFzaWMuaWRlbnRpdHkud3JpdGUgYmFzaWMucGVyc29uYWV4dGVybmFscmVmIGRwLmNvbW1lcmNlLnJlZnJlc2hleHRlbnRpdGxlbWVudC5ydyBiYXNpYy5wcmVmZXJlbmNlLndyaXRlIGdvc19ncm91cF9lbmR1c2VyIGJhc2ljLmFjY2VzcyBzb2NpYWxfcmVjb21tZW5kYXRpb25fdXNlciBiYXNpYy5wcmVmZXJlbmNlIGJhc2ljLmlkZW50aXR5IGJhc2ljLnBlcnNvbmEgZ29zX2dyb3VwX2ludGVncmF0b3IiLCJwaWQiOiIxMDAwMDQxODk5MjYyIiwicHR5IjoiUFMzIiwidWlkIjoiMTAwMDA0NTE5OTI2MiIsInBzaWQiOjEwMDAwNDEwOTkyNjIsImNzZXYiOiJJTlQiLCJkdmlkIjoiMDAwMDAwMDcwMDA3MDBjMDAwMDEwMTgxMDAxMDAwMDA1ZjE5YWIzMjEzODhjY2NhYjUxYjcxZTdiYTY2NzlhMCIsInBsdHlwIjoiUFM0Iiwic3RwcyI6Ik9GRiIsInVkZyI6ZmFsc2UsImNudHkiOiIxIiwiYXVzcmMiOiIxMDAwMDMiLCJpcGdlbyI6eyJpcCI6IjE1OS4xNTMuKi4qIiwiY3R5IjoiQ0EiLCJyZWciOiJCcml0aXNoIENvbHVtYmlhIiwiY2l0IjoiVmFuY291dmVyIiwiaXNwIjoiRWxlY3Ryb25pYyBBcnRzICBJbmMuIn0sImVuYyI6Ik1TWWlQbi9TV2YzOWUyeXhXSUlhKzgxMi9pU3pZZlZDZkluai9ORzgzck0vZVVOSm81YUUzalU0SVVjdTk1RFlnS0ozUk9vdDJxdUNFbGlWVUJIbmRzNnh1NEJ3OUVGRGJKT3pNMFptV3FLUmpkM1ZmRVRMWEtiaWt2QjdhYSs5UjJYdU4xdmVCd2Z4M1lZRUZPUnRPdW9ROWFhVDh2RElmMnNFZzB0SXQ4dGlGazJnT3RlR1pyWjZxRUVYWkZHc2ZMY2ZBZFBRbS9zSjJlVEV1WDRnTnR4cU9naG9zTmM5Y05RcUNmOUVSZ3pNNm9RVFdVb0xVVVZHZDNUY25TMWdJN3JtNHNMTVY2SmNYVGtjbWhOdG5CMkFYYzZxQUpFaS9JcjhuZnVvWjRkUjNNYzQ4YmZ3by9UaEc0WnRIS2xwMHdHVkpPQTNRV1JoTUdIWVNtSGtwMTQxcnB4TGhsKy9SWGdRTnpJQW5OTHRlVThuZEpzOWQ0VjhCcFhYS2tzM0RxdUd5NHhHRWhZRHNCV1RUd3hLU3hxZ3NENTJnbHptd3JiR1E1V2xJU0xhem4xM1YzSGI2NE1qeDh5VzRUVmxUbkI1ZzNxWEU4dE5ESjhucWNIUTltdE9FZG1MS1BLaWhvMjg3amk1QVg5cUpDQ3dXVXFwY2U2S2pYYm1wclAzNW9SWlFoTnZYdzVKZkhWTUk1MEpzZENxMHp4eFRSb0dIMWZ0Z1Q5b3JvZnMvQVJtT2pHQlY2cGs1bFFkcWxodlNBdFBNL3hab3V1TnNXbWVWak9CclJPYkN4dDE0ektrZUxFMGpEc0t3cmhxdWN6RW52WnZTRnlmUXhoeW1BZC9hckdGSTJ3VGliSFZoYU5PeHJoa0tMTVFYYXZJNHRtSytObTFHT3RmNlFoY2xRcjRiTVV3MzBaRGNzb3pTWS9yWCtBSndJZzNSaUVDR1d6dzJ4b2ZCbFVHSWtpa2JvUHJzd25vZnJuclJuVEw3c1RWWUYzcS9iR3NjY0tkNlVZV2VrUFNmL3F2RUg4QS9Rb3NhMXliRHcya3BpdW9lU2RLc3huM3NXeTgrSnF2alR2UjdYRFpBa0oyVjBKVjVNVVJzS0p6MXdBUVV1eG9hc3I2SmJOS0FTU0ZQY0ZFTzlwY1NDR2ZIZDBSK0Q2Zm9sN256K2dPeGNySEFTdm5CRldDMDZ6bXZmWEVFdEJzbnlzanlPakwzMGM3TWJzd0NsMk1vTHJQRkFzdTU5OFNScklpOCt4ZmVDVTM0TDdXTlFNWWhkbEd2Y05TMFByb3ViV0FoM0F5NEtpTXo0cHJpaUo3U2hydXloNFRIeDRJMW9mT2xHR0IrSUhKMWt1aVdHMzBJNHRJVkRQTFZMb1NyNnBxVWlIRkhWZ0s1RVNDaFJUNE5vaVVwRy90dkE1T0dtODNrUzlQdnBrb1Y2TnEwNVFsaFhXVWNqRkRVMzhPRDllZkFxN1g1RlhQc2N5N2xoTVN5SmtTSnJVOVlXU01VOTRUMFc0UytlbVEvR28yR3ZoVDNwaFh3dE45OE1SaitFNEt2MDA3QVcrbXJhR2RWck1RVUVhMnBnNjJyYkNHcEhTaTZzb0YxR1h5cHBhbnBCcVF2MXJ1UERFbHdqNTFYenc2cTlndlB2ZzlxQU5KRTQxZUsva0dDS2djVjN5WHc0SnByVEFNdEdyY1Iwa0lpb3pLK05hdG5ndUdVRFc5bVUrSVRpbHNwUG9nckgrSWVnR0prNEFhYzV6UFgxY09jeXcvOW1MbU5Kc01tYWxaUTQydnhuSFFsM2ZiWmRld0VZQnNNS0M4QkdESlVZb3R0VmFyNXZWNnlGTWxJTU44UTFNNFEvWWNaSjRKbHFXNEVaTUkifX0";
            const char8_t* mod = "ny7FHSTmfpHefqyxDTFkPDJuokxR7y92MtFmf76cNGYYdF1w-sRGMvjuRnZyiiZTD4SAdm6dciLyKaR3BoTX5HuWURuqefZqMkhfduFS8q4kRv-f995hKmIdbnWXPQNDu4HxU4wTha2wCvOaYj0Yx0j-njiMrU7_YeYABRSLlulaVkMes3T32ToPVxBTGoI3lsL58QgQDzzDXHIi4dYH_mr98Rhxz4lDYVzcu5kE702AFowxHIAPWZMZ7N2wAmXgv8xcNlJ3agX9gcVb9_caGvF5BVPUnW2d005TzCnFbt3IUlYXeJocDkVvbIcLCDkMFQpw7IQzmU8d8BXAxbbBLw";
            const char8_t* exp = "AQAB";

            JwtUtil::SignatureVerificationData verification;
            verification.sig = (uint8_t*)invalidSig;
            verification.sigSize = strlen(invalidSig);
            verification.msg = (uint8_t*)msg;
            verification.msgSize = strlen(msg);
            verification.mod = (uint8_t*)mod;
            verification.modSize = strlen(mod);
            verification.exp = (uint8_t*)exp;
            verification.expSize = strlen(exp);
            BlazeRpcError result = jwtUtil.verifyJwtSignature(verification);
            EXPECT_TRUE(result != ERR_OK) << "invalid sig2";
        }
    }

    //test JwtUtil::verifySignatureRS256 with valid and invalid sig, msg, mod, exp
    TEST_F(JwtTest, verifySignatureRS256Test)
    {
        JwtUtil jwtUtil(nullptr);

        const char8_t* validMsg = "eyJraWQiOiIxZTlnZGs3IiwiYWxnIjoiUlMyNTYifQ.ewogImlzcyI6ICJodHRwOi8vc2VydmVyLmV4YW1wbGUuY29tIiwKICJzdWIiOiAiMjQ4Mjg5NzYxMDAxIiwKICJhdWQiOiAiczZCaGRSa3F0MyIsCiAibm9uY2UiOiAibi0wUzZfV3pBMk1qIiwKICJleHAiOiAxMzExMjgxOTcwLAogImlhdCI6IDEzMTEyODA5NzAsCiAiY19oYXNoIjogIkxEa3RLZG9RYWszUGswY25YeENsdEEiCn0";
        uint32_t validMsgSize = strlen(validMsg);

        //XW6uhdrkBgcGx6zVIrCiROpWURs-4goO1sKA4m9jhJIImiGg5muPUcNegx6sSv43c5DSn37sxCRrDZZm4ZPBKKgtYASMcE20SDgvYJdJS0cyuFw7Ijp_7WnIjcrl6B5cmoM6ylCvsLMwkoQAxVublMwH10oAxjzD6NEFsu9nipkszWhsPePf_rM4eMpkmCbTzume-fzZIi5VjdWGGEmzTg32h3jiex-r5WTHbj-u5HL7u_KP3rmbdYNzlzd1xWRYTUs4E8nOTgzAUwvwXkIQhOh5TPcSMBYy6X3E7-_gr9Ue6n4ND7hTFhtjYs3cjNKIA08qm5cpVYFMFMG6PkhzLQ
        uint8_t validSignature[] = {
            0x5d, 0x6e, 0xae, 0x85, 0xda, 0xe4, 0x06, 0x07, 0x06, 0xc7, 0xac, 0xd5, 0x22, 0xb0, 0xa2, 0x44,
            0xea, 0x56, 0x51, 0x1b, 0x3e, 0xe2, 0x0a, 0x0e, 0xd6, 0xc2, 0x80, 0xe2, 0x6f, 0x63, 0x84, 0x92,
            0x08, 0x9a, 0x21, 0xa0, 0xe6, 0x6b, 0x8f, 0x51, 0xc3, 0x5e, 0x83, 0x1e, 0xac, 0x4a, 0xfe, 0x37,
            0x73, 0x90, 0xd2, 0x9f, 0x7e, 0xec, 0xc4, 0x24, 0x6b, 0x0d, 0x96, 0x66, 0xe1, 0x93, 0xc1, 0x28,
            0xa8, 0x2d, 0x60, 0x04, 0x8c, 0x70, 0x4d, 0xb4, 0x48, 0x38, 0x2f, 0x60, 0x97, 0x49, 0x4b, 0x47,
            0x32, 0xb8, 0x5c, 0x3b, 0x22, 0x3a, 0x7f, 0xed, 0x69, 0xc8, 0x8d, 0xca, 0xe5, 0xe8, 0x1e, 0x5c,
            0x9a, 0x83, 0x3a, 0xca, 0x50, 0xaf, 0xb0, 0xb3, 0x30, 0x92, 0x84, 0x00, 0xc5, 0x5b, 0x9b, 0x94,
            0xcc, 0x07, 0xd7, 0x4a, 0x00, 0xc6, 0x3c, 0xc3, 0xe8, 0xd1, 0x05, 0xb2, 0xef, 0x67, 0x8a, 0x99,
            0x2c, 0xcd, 0x68, 0x6c, 0x3d, 0xe3, 0xdf, 0xfe, 0xb3, 0x38, 0x78, 0xca, 0x64, 0x98, 0x26, 0xd3,
            0xce, 0xe9, 0x9e, 0xf9, 0xfc, 0xd9, 0x22, 0x2e, 0x55, 0x8d, 0xd5, 0x86, 0x18, 0x49, 0xb3, 0x4e,
            0x0d, 0xf6, 0x87, 0x78, 0xe2, 0x7b, 0x1f, 0xab, 0xe5, 0x64, 0xc7, 0x6e, 0x3f, 0xae, 0xe4, 0x72,
            0xfb, 0xbb, 0xf2, 0x8f, 0xde, 0xb9, 0x9b, 0x75, 0x83, 0x73, 0x97, 0x37, 0x75, 0xc5, 0x64, 0x58,
            0x4d, 0x4b, 0x38, 0x13, 0xc9, 0xce, 0x4e, 0x0c, 0xc0, 0x53, 0x0b, 0xf0, 0x5e, 0x42, 0x10, 0x84,
            0xe8, 0x79, 0x4c, 0xf7, 0x12, 0x30, 0x16, 0x32, 0xe9, 0x7d, 0xc4, 0xef, 0xef, 0xe0, 0xaf, 0xd5,
            0x1e, 0xea, 0x7e, 0x0d, 0x0f, 0xb8, 0x53, 0x16, 0x1b, 0x63, 0x62, 0xcd, 0xdc, 0x8c, 0xd2, 0x88,
            0x03, 0x4f, 0x2a, 0x9b, 0x97, 0x29, 0x55, 0x81, 0x4c, 0x14, 0xc1, 0xba, 0x3e, 0x48, 0x73, 0x2d
        };
        uint32_t validSignatureSize = 256;

        //w7Zdfmece8iaB0kiTY8pCtiBtzbptJmP28nSWwtdjRu0f2GFpajvWE4VhfJAjEsOcwYzay7XGN0b-X84BfC8hmCTOj2b2eHT7NsZegFPKRUQzJ9wW8ipn_aDJWMGDuB1XyqT1E7DYqjUCEOD1b4FLpy_xPn6oV_TYOfQ9fZdbE5HGxJUzekuGcOKqOQ8M7wfYHhHHLxGpQVgL0apWuP2gDDOdTtpuld4D2LK1MZK99s9gaSjRHE8JDb1Z4IGhEcEyzkxswVdPndUWzfvWBBWXWxtSUvQGBRkuy1BHOa4sP6FKjWEeeF7gm7UMs2Nm2QUgNZw6xvEDGaLk4KASdIxRQ
        uint8_t validModulus[] = {
            0xc3, 0xb6, 0x5d, 0x7e, 0x67, 0x9c, 0x7b, 0xc8, 0x9a, 0x07, 0x49, 0x22, 0x4d, 0x8f, 0x29, 0x0a,
            0xd8, 0x81, 0xb7, 0x36, 0xe9, 0xb4, 0x99, 0x8f, 0xdb, 0xc9, 0xd2, 0x5b, 0x0b, 0x5d, 0x8d, 0x1b,
            0xb4, 0x7f, 0x61, 0x85, 0xa5, 0xa8, 0xef, 0x58, 0x4e, 0x15, 0x85, 0xf2, 0x40, 0x8c, 0x4b, 0x0e,
            0x73, 0x06, 0x33, 0x6b, 0x2e, 0xd7, 0x18, 0xdd, 0x1b, 0xf9, 0x7f, 0x38, 0x05, 0xf0, 0xbc, 0x86,
            0x60, 0x93, 0x3a, 0x3d, 0x9b, 0xd9, 0xe1, 0xd3, 0xec, 0xdb, 0x19, 0x7a, 0x01, 0x4f, 0x29, 0x15,
            0x10, 0xcc, 0x9f, 0x70, 0x5b, 0xc8, 0xa9, 0x9f, 0xf6, 0x83, 0x25, 0x63, 0x06, 0x0e, 0xe0, 0x75,
            0x5f, 0x2a, 0x93, 0xd4, 0x4e, 0xc3, 0x62, 0xa8, 0xd4, 0x08, 0x43, 0x83, 0xd5, 0xbe, 0x05, 0x2e,
            0x9c, 0xbf, 0xc4, 0xf9, 0xfa, 0xa1, 0x5f, 0xd3, 0x60, 0xe7, 0xd0, 0xf5, 0xf6, 0x5d, 0x6c, 0x4e,
            0x47, 0x1b, 0x12, 0x54, 0xcd, 0xe9, 0x2e, 0x19, 0xc3, 0x8a, 0xa8, 0xe4, 0x3c, 0x33, 0xbc, 0x1f,
            0x60, 0x78, 0x47, 0x1c, 0xbc, 0x46, 0xa5, 0x05, 0x60, 0x2f, 0x46, 0xa9, 0x5a, 0xe3, 0xf6, 0x80,
            0x30, 0xce, 0x75, 0x3b, 0x69, 0xba, 0x57, 0x78, 0x0f, 0x62, 0xca, 0xd4, 0xc6, 0x4a, 0xf7, 0xdb,
            0x3d, 0x81, 0xa4, 0xa3, 0x44, 0x71, 0x3c, 0x24, 0x36, 0xf5, 0x67, 0x82, 0x06, 0x84, 0x47, 0x04,
            0xcb, 0x39, 0x31, 0xb3, 0x05, 0x5d, 0x3e, 0x77, 0x54, 0x5b, 0x37, 0xef, 0x58, 0x10, 0x56, 0x5d,
            0x6c, 0x6d, 0x49, 0x4b, 0xd0, 0x18, 0x14, 0x64, 0xbb, 0x2d, 0x41, 0x1c, 0xe6, 0xb8, 0xb0, 0xfe,
            0x85, 0x2a, 0x35, 0x84, 0x79, 0xe1, 0x7b, 0x82, 0x6e, 0xd4, 0x32, 0xcd, 0x8d, 0x9b, 0x64, 0x14,
            0x80, 0xd6, 0x70, 0xeb, 0x1b, 0xc4, 0x0c, 0x66, 0x8b, 0x93, 0x82, 0x80, 0x49, 0xd2, 0x31, 0x45
        };
        uint32_t validModulusSize = 256;

        //AQAB
        uint8_t validExponent[] = {
            0x01, 0x00, 0x01
        };
        uint32_t validExponentSize = 3;

        //test case: valid sig, msg, mod, exp can be verified successfully
        {
            JwtUtil::SignatureVerificationData verification;
            verification.sig = validSignature;
            verification.sigSize = validSignatureSize;
            verification.msg = (uint8_t*)validMsg;
            verification.msgSize = validMsgSize;
            verification.mod = validModulus;
            verification.modSize = validModulusSize;
            verification.exp = validExponent;
            verification.expSize = validExponentSize;
            bool result = jwtUtil.verifySignatureRS256(verification);
            EXPECT_TRUE(result) << "all valid";
        }

        //test case: invalid msg cannot be verified
        {
            //payload content is manipulated
            const char8_t* invalidMsg = "eyJraWQiOiIxZTlnZGs3IiwiYWxnIjoiUlMyNTYifQ.ewogImlzcyI6ICJodHRwOi8vc2VydmVyLmV4YW1wbGUuY29tIiwKICJzdWIiOiAiMjQ4Mjg5NzYxMDAxIiwKICJhdWQiOiAiczZCaGRSa3F0MyIsCiAibm9uY2UiOiAibi0wUzZfV3pBMk1qIiwKICJleHAiOiAxMzExMzgxOTcwLAogImlhdCI6IDEzMTEyODA5NzAsCiAiY19oYXNoIjogIkxEa3RLZG9RYWszUGswY25YeENsdEEiCn0";
            uint32_t invalidMsgSize = strlen(invalidMsg);

            JwtUtil::SignatureVerificationData verification;
            verification.sig = validSignature;
            verification.sigSize = validSignatureSize;
            verification.msg = (uint8_t*)invalidMsg;
            verification.msgSize = invalidMsgSize;
            verification.mod = validModulus;
            verification.modSize = validModulusSize;
            verification.exp = validExponent;
            verification.expSize = validExponentSize;
            bool result = jwtUtil.verifySignatureRS256(verification);
            EXPECT_TRUE(!result) << "invalid msg";
        }

        //test case: invalid sig cannot be verified
        {
            //signature is manipulated
            uint8_t invalidSignature[] = {
                0x5d, 0x6e, 0xae, 0x85, 0xda, 0xe4, 0x06, 0x07, 0x06, 0xc7, 0xac, 0xd5, 0x22, 0xb0, 0xa2, 0x44,
                0xea, 0x56, 0x51, 0x1b, 0x3e, 0xe2, 0x0a, 0x0e, 0xd6, 0xc2, 0x80, 0xe2, 0x6f, 0x63, 0x84, 0x92,
                0x08, 0x9a, 0x21, 0xa0, 0xe6, 0x6b, 0x8f, 0x51, 0xc3, 0x5e, 0x83, 0x1e, 0xac, 0x4a, 0xfe, 0x37,
                0x73, 0x90, 0xd2, 0x9f, 0x7e, 0xec, 0xc4, 0x24, 0x6b, 0x0d, 0x96, 0x66, 0xe1, 0x93, 0xc1, 0x28,
                0xa8, 0x2d, 0x60, 0x04, 0x8c, 0x70, 0x4d, 0xb4, 0x48, 0x38, 0x2f, 0x60, 0x97, 0x49, 0x4b, 0x47,
                0x32, 0xb8, 0x5c, 0x3b, 0x22, 0x3a, 0x7f, 0xed, 0x69, 0xc8, 0x8d, 0xca, 0xe5, 0xe8, 0x1e, 0x5c,
                0x9a, 0x83, 0x3a, 0xca, 0x50, 0xaf, 0xb0, 0xb3, 0x30, 0x92, 0x84, 0x00, 0xc5, 0x5b, 0x9b, 0x94,
                0xcc, 0x07, 0xd7, 0x4a, 0x00, 0xc6, 0x3c, 0xc3, 0xe8, 0xd1, 0x05, 0xb2, 0xef, 0x67, 0x8a, 0x99,
                0x2c, 0xcd, 0x68, 0x6c, 0x3d, 0xe3, 0xdf, 0xfe, 0xb3, 0x38, 0x78, 0xca, 0x64, 0x98, 0x26, 0xd3,
                0xce, 0xe9, 0x9e, 0xf9, 0xfc, 0xd9, 0x22, 0x2e, 0x55, 0x8d, 0xd5, 0x86, 0x18, 0x49, 0xb3, 0x4e,
                0x0d, 0xf6, 0x87, 0x78, 0xe2, 0x7b, 0x1f, 0xab, 0xe5, 0x64, 0xc7, 0x6e, 0x3f, 0xae, 0xe4, 0x72,
                0xfb, 0xbb, 0xf2, 0x8f, 0xde, 0xb9, 0x9b, 0x75, 0x83, 0x73, 0x97, 0x37, 0x75, 0xc5, 0x64, 0x58,
                0x4d, 0x4b, 0x38, 0x13, 0xc9, 0xce, 0x4e, 0x0c, 0xc0, 0x53, 0x0b, 0xf0, 0x5e, 0x42, 0x10, 0x84,
                0xe8, 0x79, 0x4c, 0xf7, 0x12, 0x30, 0x16, 0x32, 0xe9, 0x7d, 0xc4, 0xef, 0xef, 0xe0, 0xaf, 0xd5,
                0x1e, 0xea, 0x7e, 0x0d, 0x0f, 0xb8, 0x53, 0x16, 0x1b, 0x63, 0x62, 0xcd, 0xdc, 0x8c, 0xd2, 0x88,
                0x03, 0x4f, 0x2a, 0x9b, 0x97, 0x29, 0x55, 0x81, 0x4c, 0x14, 0xc1, 0xba, 0x3e, 0x48, 0x73, 0x00
            };
            uint32_t invalidSignatureSize = 256;

            JwtUtil::SignatureVerificationData verification;
            verification.sig = invalidSignature;
            verification.sigSize = invalidSignatureSize;
            verification.msg = (uint8_t*)validMsg;
            verification.msgSize = validMsgSize;
            verification.mod = validModulus;
            verification.modSize = validModulusSize;
            verification.exp = validExponent;
            verification.expSize = validExponentSize;
            bool result = jwtUtil.verifySignatureRS256(verification);
            EXPECT_TRUE(!result) << "invalid signature";
        }

        //test case: invalid mod cannot be verified
        {
            //modulus is manipulated
            uint8_t invalidModulus[] = {
                0xc3, 0xb6, 0x5d, 0x7e, 0x67, 0x9c, 0x7b, 0xc8, 0x9a, 0x07, 0x49, 0x22, 0x4d, 0x8f, 0x29, 0x0a,
                0xd8, 0x81, 0xb7, 0x36, 0xe9, 0xb4, 0x99, 0x8f, 0xdb, 0xc9, 0xd2, 0x5b, 0x0b, 0x5d, 0x8d, 0x1b,
                0xb4, 0x7f, 0x61, 0x85, 0xa5, 0xa8, 0xef, 0x58, 0x4e, 0x15, 0x85, 0xf2, 0x40, 0x8c, 0x4b, 0x0e,
                0x73, 0x06, 0x33, 0x6b, 0x2e, 0xd7, 0x18, 0xdd, 0x1b, 0xf9, 0x7f, 0x38, 0x05, 0xf0, 0xbc, 0x86,
                0x60, 0x93, 0x3a, 0x3d, 0x9b, 0xd9, 0xe1, 0xd3, 0xec, 0xdb, 0x19, 0x7a, 0x01, 0x4f, 0x29, 0x15,
                0x10, 0xcc, 0x9f, 0x70, 0x5b, 0xc8, 0xa9, 0x9f, 0xf6, 0x83, 0x25, 0x63, 0x06, 0x0e, 0xe0, 0x75,
                0x5f, 0x2a, 0x93, 0xd4, 0x4e, 0xc3, 0x62, 0xa8, 0xd4, 0x08, 0x43, 0x83, 0xd5, 0xbe, 0x05, 0x2e,
                0x9c, 0xbf, 0xc4, 0xf9, 0xfa, 0xa1, 0x5f, 0xd3, 0x60, 0xe7, 0xd0, 0xf5, 0xf6, 0x5d, 0x6c, 0x4e,
                0x47, 0x1b, 0x12, 0x54, 0xcd, 0xe9, 0x2e, 0x19, 0xc3, 0x8a, 0xa8, 0xe4, 0x3c, 0x33, 0xbc, 0x1f,
                0x60, 0x78, 0x47, 0x1c, 0xbc, 0x46, 0xa5, 0x05, 0x60, 0x2f, 0x46, 0xa9, 0x5a, 0xe3, 0xf6, 0x80,
                0x30, 0xce, 0x75, 0x3b, 0x69, 0xba, 0x57, 0x78, 0x0f, 0x62, 0xca, 0xd4, 0xc6, 0x4a, 0xf7, 0xdb,
                0x3d, 0x81, 0xa4, 0xa3, 0x44, 0x71, 0x3c, 0x24, 0x36, 0xf5, 0x67, 0x82, 0x06, 0x84, 0x47, 0x04,
                0xcb, 0x39, 0x31, 0xb3, 0x05, 0x5d, 0x3e, 0x77, 0x54, 0x5b, 0x37, 0xef, 0x58, 0x10, 0x56, 0x5d,
                0x6c, 0x6d, 0x49, 0x4b, 0xd0, 0x18, 0x14, 0x64, 0xbb, 0x2d, 0x41, 0x1c, 0xe6, 0xb8, 0xb0, 0xfe,
                0x85, 0x2a, 0x35, 0x84, 0x79, 0xe1, 0x7b, 0x82, 0x6e, 0xd4, 0x32, 0xcd, 0x8d, 0x9b, 0x64, 0x14,
                0x80, 0xd6, 0x70, 0xeb, 0x1b, 0xc4, 0x0c, 0x66, 0x8b, 0x93, 0x82, 0x80, 0x49, 0xd2, 0x31, 0x00
            };
            uint32_t invalidModulusSize = 256;

            JwtUtil::SignatureVerificationData verification;
            verification.sig = validSignature;
            verification.sigSize = validSignatureSize;
            verification.msg = (uint8_t*)validMsg;
            verification.msgSize = validMsgSize;
            verification.mod = invalidModulus;
            verification.modSize = invalidModulusSize;
            verification.exp = validExponent;
            verification.expSize = validExponentSize;
            bool result = jwtUtil.verifySignatureRS256(verification);
            EXPECT_TRUE(!result) << "invalid modulus";
        }

        //test case: invalid exp cannot be verified
        {
            //exponent is manipulated
            uint8_t invalidExponent[] = {
                0x01, 0x00, 0x00
            };
            uint32_t invalidExponentSize = 3;

            JwtUtil::SignatureVerificationData verification;
            verification.sig = validSignature;
            verification.sigSize = validSignatureSize;
            verification.msg = (uint8_t*)validMsg;
            verification.msgSize = validMsgSize;
            verification.mod = validModulus;
            verification.modSize = validModulusSize;
            verification.exp = invalidExponent;
            verification.expSize = invalidExponentSize;
            bool result = jwtUtil.verifySignatureRS256(verification);
            EXPECT_TRUE(!result) << "invalid exponent";
        }

        //test case: sig from a different jwt cannot be verified
        {
            //dqjc2h_tPrGgeb-XFGTMmzjE5WA0CiqfJo9yOrH6wuYF30IDEOTr8OTqtfr6doIBVQMKWAVXDVOEen9bLIOwv371cUkgoZ396DJhd6j9TeQnGUT2CwxLvwaa4cz-fZPTQqYGw1PX5GFL4jRUyYJslas2PQ7YX0o4SJz9ZVNWr7UUgtgGfwKr4eyC4A1MyL8oCzx7gEoDVXBdYvMFHTxn0-QMX-bMkfQYZTbrnS8pO6GIlRlW6hggdOj3v0aHNDPnVKN4mfXxYi0RhpXoVHYcQ4oIDHY_qtMdgKksqv3RpsBDjFL-sdH-CtHrkl5o2umno9qVnErGuNqtOKoKkiRbdw
            uint8_t signatureOfDifferentToken[] = {
                0x76, 0xa8, 0xdc, 0xda, 0x1f, 0xed, 0x3e, 0xb1, 0xa0, 0x79, 0xbf, 0x97, 0x14, 0x64, 0xcc, 0x9b, 
                0x38, 0xc4, 0xe5, 0x60, 0x34, 0x0a, 0x2a, 0x9f, 0x26, 0x8f, 0x72, 0x3a, 0xb1, 0xfa, 0xc2, 0xe6, 
                0x05, 0xdf, 0x42, 0x03, 0x10, 0xe4, 0xeb, 0xf0, 0xe4, 0xea, 0xb5, 0xfa, 0xfa, 0x76, 0x82, 0x01, 
                0x55, 0x03, 0x0a, 0x58, 0x05, 0x57, 0x0d, 0x53, 0x84, 0x7a, 0x7f, 0x5b, 0x2c, 0x83, 0xb0, 0xbf, 
                0x7e, 0xf5, 0x71, 0x49, 0x20, 0xa1, 0x9d, 0xfd, 0xe8, 0x32, 0x61, 0x77, 0xa8, 0xfd, 0x4d, 0xe4, 
                0x27, 0x19, 0x44, 0xf6, 0x0b, 0x0c, 0x4b, 0xbf, 0x06, 0x9a, 0xe1, 0xcc, 0xfe, 0x7d, 0x93, 0xd3, 
                0x42, 0xa6, 0x06, 0xc3, 0x53, 0xd7, 0xe4, 0x61, 0x4b, 0xe2, 0x34, 0x54, 0xc9, 0x82, 0x6c, 0x95, 
                0xab, 0x36, 0x3d, 0x0e, 0xd8, 0x5f, 0x4a, 0x38, 0x48, 0x9c, 0xfd, 0x65, 0x53, 0x56, 0xaf, 0xb5, 
                0x14, 0x82, 0xd8, 0x06, 0x7f, 0x02, 0xab, 0xe1, 0xec, 0x82, 0xe0, 0x0d, 0x4c, 0xc8, 0xbf, 0x28, 
                0x0b, 0x3c, 0x7b, 0x80, 0x4a, 0x03, 0x55, 0x70, 0x5d, 0x62, 0xf3, 0x05, 0x1d, 0x3c, 0x67, 0xd3, 
                0xe4, 0x0c, 0x5f, 0xe6, 0xcc, 0x91, 0xf4, 0x18, 0x65, 0x36, 0xeb, 0x9d, 0x2f, 0x29, 0x3b, 0xa1, 
                0x88, 0x95, 0x19, 0x56, 0xea, 0x18, 0x20, 0x74, 0xe8, 0xf7, 0xbf, 0x46, 0x87, 0x34, 0x33, 0xe7, 
                0x54, 0xa3, 0x78, 0x99, 0xf5, 0xf1, 0x62, 0x2d, 0x11, 0x86, 0x95, 0xe8, 0x54, 0x76, 0x1c, 0x43, 
                0x8a, 0x08, 0x0c, 0x76, 0x3f, 0xaa, 0xd3, 0x1d, 0x80, 0xa9, 0x2c, 0xaa, 0xfd, 0xd1, 0xa6, 0xc0, 
                0x43, 0x8c, 0x52, 0xfe, 0xb1, 0xd1, 0xfe, 0x0a, 0xd1, 0xeb, 0x92, 0x5e, 0x68, 0xda, 0xe9, 0xa7, 
                0xa3, 0xda, 0x95, 0x9c, 0x4a, 0xc6, 0xb8, 0xda, 0xad, 0x38, 0xaa, 0x0a, 0x92, 0x24, 0x5b, 0x77
            };
            uint32_t signatureOfDifferentTokenSize = 256;

            JwtUtil::SignatureVerificationData verification;
            verification.sig = signatureOfDifferentToken;
            verification.sigSize = signatureOfDifferentTokenSize;
            verification.msg = (uint8_t*)validMsg;
            verification.msgSize = validMsgSize;
            verification.mod = validModulus;
            verification.modSize = validModulusSize;
            verification.exp = validExponent;
            verification.expSize = validExponentSize;
            bool result = jwtUtil.verifySignatureRS256(verification);
            EXPECT_TRUE(!result) << "signature of different token";
        }
    }

} //namespace OAuth
} //namespace Blaze
