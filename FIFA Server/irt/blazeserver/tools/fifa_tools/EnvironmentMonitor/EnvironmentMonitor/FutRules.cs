using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Web;
using System.Web.Script.Serialization;
using Rest;

namespace EnvironmentMonitor
{
    public class FutHealthCheckRule : Rule
    {
        private FUTModuleParam mFUTParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mFUTParam;

        public FutHealthCheckRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mFUTParam.mFUT_RS4_BASE_URL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "ut/healthcheck");
            client.Execute(request, mReport);
        }

        private void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            string output = string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload);
            Console.WriteLine(output);
            mReport.AppendLine(output);

            if (e.StatusCode == HttpStatusCode.OK)
            {
                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible failure point can be FUT server downtime or incorrect API usage. Please copy the log here and contact Lion Ops/Dev and GameTeam FUT SE for further investigation",
                    this.GetType().ToString()));
            }
            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class FutAuthRule : Rule
    {
        public class IdentificationJson
        {
            public IdentificationJson(string authCode, string fat, string tid)
            {
                this.authCode = authCode;
                this.fat = fat;
                this.tid = tid;

            }
            public IdentificationJson()
            {
            }
            public string authCode { get; set; }
            public string user { get; set; }
            public string password { get; set; }
            public string fat { get; set; }
            public string tid { get; set; }
        }

        public class PreAuthPayloadJson
        {
            public PreAuthPayloadJson()
            {
                identification = new IdentificationJson();
            }
 
            public bool isReadOnly { get; set; }
            public int priorityLevel { get; set; }
            public string sku { get; set; }
            public string nucleusPersonaPlatform { get; set; }
            public int clientVersion { get; set; }
            public long nuc { get; set; }
            public long nucleusPersonaId { get; set; }
            public string nucleusPersonaDisplayName { get; set; }
            public string locale { get; set; }
            public string deviceId { get; set; }
            public string macAddress { get; set; }
            public string method { get; set; }
            public IdentificationJson identification { get; set; }
            public string regionCode { get; set; }
        }

        public class FutAuthResponsePayload
        {
            public FutAuthResponsePayload()
            {

            }

            public string protocol { get; set; }
            public string ipPort { get; set; }
            public string serverTime { get; set; }
            public string lastOnlineTime { get; set; }
            public string sid { get; set; }
        }

        private FUTModuleParam mFUTParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mFUTParam;

        public FutAuthRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            PreAuthPayloadJson payload = fillPreAuthJsonPayload();

            RestClient client = new RestClient(mFUTParam.mFUT_RS4_BASE_URL);
            client.RestClientRequestFinished += this.responseCallback;
            
            JavaScriptSerializer jsSerializer = new JavaScriptSerializer();
            string payloadJson = jsSerializer.Serialize(payload);

            StringContent content = new StringContent(payloadJson, Encoding.UTF8, "application/json");
            
            content.Headers.Add("X-FOS-SKIP-NUC-AUTH", "true");
            content.Headers.Add("X-FOS-DEBUG-SKIP-XUID-VALIDATION", "true");
            content.Headers.Add("X-FOS-SKIP-MS-PRIVACY-CHECK", "true");
            content.Headers.Add("X-FOS-SKIP-SECURITY-CHECK", "true");
            content.Headers.Add("X-FOS-SKIP-SECURITY-VALIDATION", "true");
            content.Headers.Add("X-FOS-SKIP-CAPTCHA", "true");
            content.Headers.Add("X-FOS-SKIP-WHITE-LIST-VALIDATION", "true");
            content.Headers.Add("X-FOS-DEBUG-TWO-FACTOR-OVERRIDE-DEVICE-ID", EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId.ToString());
            content.Headers.Add("X-FOS-DEBUG-DEVICE-LIMITS-OVERRIDE-DEVICE-ID", EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId.ToString());
            
            RestRequest request = new RestRequest(HttpMethod.Post, "ut/auth");
            request.AddContent(content);
            client.Execute(request, mReport);
        }

        private PreAuthPayloadJson fillPreAuthJsonPayload()
        {
            PreAuthPayloadJson json = new PreAuthPayloadJson();
            json.isReadOnly = false;
            json.priorityLevel = 6;
            json.sku = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam.mCMS_SKU;
            json.nuc = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId;
            json.nucleusPersonaId = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mPersonaId;
            json.nucleusPersonaDisplayName = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mDisplayName;
            json.locale = "en-US";
            json.deviceId = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId.ToString();
            json.method = "nuc";
            json.clientVersion = 1;
            json.regionCode = "no";

            json.identification.authCode = "";
            json.identification.password = "1Q2w3e4r";
            json.identification.user = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mEmail;

            if(json.sku.Contains("XBO"))
            {
                json.nucleusPersonaPlatform = "capilano";
                json.identification.fat = "XBL3.0 x=1007972276776590282;eyJlbmMiOiJBMTI4Q0JDK0hTMjU2IiwiYWxnIjoiUlNBLU9BRVAiLCJjdHkiOiJKV1QiLCJ6aXAiOiJERUYiLCJ4NXQiOiI4cW5QSUktLWZHSFIzVDd2NmRyNnI4WDFLTHMifQ.lQ-HhTw02m38HWPWKrkEgDPYxisKvgTty692OQuOy3jpSWgXJeb-7hjygxvvLrm7mrsETtp8BTpXbV6I0iL7kf-eMgFi7IItL9Obd2IPDsfWDOYp4-WUjUWp_PV1Dqv4oGlwinMecFDRq9y1g_92u-mWA8yesKWcfssmb6D0USfivIlB3c_QM48q_ZzreBZ9AuK-aovr5jwFPf8zoj_UJjxI37ww3Ls83vBjaQxYzAdWg3j81eMFllFrhjv9TahGfj9-E_AmLfa871Uu7XHxHmd-mUNeVZNJ8RTGiN9P-iaN5ZdY2FD9-ABUzbcnR3AvIrt9o2yDXHvvhj80N0xT4w.2_ZUT2qh8zbXX1JzixlyBA.T4yO8iUYb1xWbkAdP_pEGipnpu2Has1CIIJmpbwqgHoIbgT6Ft_gpnGT9nTqhX3kokQ4eDMc9ggugFVc8K4h3dkkuN7BKowZ4tMtryxHQzb7zxPgkbxwslNo1kgp35waevCMizo6x72vzl5w_4BVkdk_617Z-39-I9W7YuaoPvwDxK-_8ID0IwSSlaIlUImdlMDjEnChHfBbzof70ePubrx0ijaqZlPGlBW2wzoEyZ_GHRNAqfq17sSWB5Q2SM9AFJNRo0Q0RLXaz_61E1Yw7Xcos3mTeYJLVSqOj9RJqGKvr3BwjZpRFGyyrkHuqforFbPvyjKalDgFEgGEdaDAH2xlqZKUZFxJf9TOXFzzV2qL9Yl5G8hSvs3nbQp92SHC2EawiUDW4T6bOssQ3QpsJj2ooyMx42OLi_IPSqNzvb26IP4NSGmRYrcPEOrN28xlPLC1ffL1uXKdcP-y6YVjZsS7C0EZNLvSuZuiLAD5H-s7bgElfauNa7ldKWYDLNrQBAWTTYdLYC_aRqaleX0arZKkWq9wQA2P6LP14Vxp_4dQkmeRTX1GyNoevK6qlyBmGjQ5Fk493A8Ktz1V3zQgtdrrHsfJwWqJNZkW50FizCsIfLpcSYG4UypYGvBFZJAQUlVynvIPYjtS_cD1CRiRhwr8zhUUDyXPLUXXo_GgigjqEieKRqVZHlBHimOVTH0IShY6OIVRi_LcdxhMA_1tlkr9baP5GCtF4FTPrOFdK477UCgj-8oawxQgpG-JxbM2-KAtf4VnIfJ68GHyKePFh9Se6V4LqYgtxP7EH1Ko9rMmBonmrNOeryXCC4zhplzOm7JxMc15-_Zb9qLLSaf7K-n1LaPYBVnKKCT38QCoE2h6tXEBejbITyV3z4JVWbMrJWK-v4ZQvA2md81fOQYvvIauaVNpxwDB2cJVM58m4_mbQOHJZp6h4rC500b4oEQ1Us4oPYG5HqMsZiqB3FeouKXongZyBX_2tdjaySPHHDmy44UHFC9bYNo1HWChunc_5oGpldcz56YsNoGKRVNFH7wRYRSqHAORnzvYtAn_VEVstUzJM0Q6WGSczpve-pryX6duIjFc9n7zoyYa9watHMKTxF8CenXfP4gPiiy2YwEzx-pq5aDo5WiEjlGxHM7-d2CV9rzqmwVg2pmA2M6tAdrzzASHrUY7mpviVNXQ6OeITgAloqpS3WQZCrDXViw_gzSkqtWWGL40kKeTuuFiXwo1R5T4rh0C-1B178maotkMBoyNqSYmd6I7sbaNRpx7xGpJA_tydY-rl65UYzubbQL12pTNKjeiBZ7sbdiwYpnWy07UXHWm3kvn92Sfeo10IZXHlI1lo5vB_uFLeizPbBh839vvcyJFNoDAA4lvqJTm757GmCrzBA-aJDBFFmLq9Od3d18Q3SqmMVWw7jl9B4kCAiAD8OGD5kjh0rykHuM41pTLaf9fws0hxxh91nqp6FQly-QfBFFwGJTkg4KZqBFPmZ8Va7hXMPkChSOzQl6b2Rgs35yqeoNHUUpBmPyL7Fw7pZBqbTHrtvVzIKo_MiqOTuqW6mmxzimrsd63kJPjwwcBezOlAZpnEHvY2jykIdOSqomRvPYklwkRPGZPaOJTH789V_2FUV_smOWkmHukH1_R0wIpjBl2t62Yv8TW.Mwocj8WI-P3fsqZB6ZYFwCBEQzrCZDQg3pMzsa8fBO0";
                json.identification.tid = "EARW.1";
                json.macAddress = "b9:ab:57:20:cf:63";
            }
            else if (json.sku.Contains("PS4"))
            {
                json.nucleusPersonaPlatform = "kettle";
                json.identification.fat = "ckmQO6";
                json.identification.tid = "CUSA00128_00";
                json.macAddress = "15:20:0b:5f:3f:25";
            }
            else
            {
                json.nucleusPersonaPlatform = "pc";
                json.macAddress = "05:77:0b:05:51:53";
            }
             
            return json;
        }

        private void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            string output = string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload);
            Console.WriteLine(output);
            mReport.AppendLine(output);

            if (e.StatusCode == HttpStatusCode.OK)
            {
                JavaScriptSerializer jsSerializer = new JavaScriptSerializer();
                FutAuthResponsePayload payloadJson = jsSerializer.Deserialize<FutAuthResponsePayload>(e.Payload);
                EnvironmentMonitor.EnvironmentConfigParam.getInstance().mFUTParam.mFutSessionId = payloadJson.sid;

                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible failure point can be FUT server downtime or incorrect API usage. Please copy the log here and contact Lion Ops/Dev and GameTeam FUT SE for further investigation",
                    this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class FutGetUserInfoRule : Rule
    {
        private FUTModuleParam mFUTParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mFUTParam;

        public FutGetUserInfoRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mFUTParam.mFUT_RS4_BASE_URL);
            client.RestClientRequestFinished += this.responseCallback;

            string fifaYear = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerName.Contains("2016") ? "fifa16" : "fifa15";

            RestRequest request = new RestRequest(HttpMethod.Get, "ut/game/{gameName}/user");
            request.FillInUrlSegment("gameName", fifaYear);
            request.AddCustomRequestHeaders("X-Purchased", "false");
            request.AddCustomRequestHeaders("X-UT-SID", mFUTParam.mFutSessionId);

            client.Execute(request, mReport);
        }

        private void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            string output = string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload);
            Console.WriteLine(output);
            mReport.AppendLine(output);

            if (e.StatusCode == HttpStatusCode.OK)
            {
                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible failure point can be FUT server downtime or incorrect API usage. Please copy the log here and contact Lion Ops/Dev and GameTeam FUT SE for further investigation",
                    this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class FutLogoutRule : Rule
    {
        private FUTModuleParam mFUTParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mFUTParam;

        public FutLogoutRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mFUTParam.mFUT_RS4_BASE_URL);
            client.RestClientRequestFinished += this.responseCallback;

            string fifaYear = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerName.Contains("2016") ? "fifa16" : "fifa15";

            RestRequest request = new RestRequest(HttpMethod.Delete, "ut/auth");
            request.AddCustomRequestHeaders("X-UT-SID", mFUTParam.mFutSessionId);

            client.Execute(request, mReport);
        }

        private void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            string output = string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload);
            Console.WriteLine(output);
            mReport.AppendLine(output);

            if (e.StatusCode == HttpStatusCode.OK)
            {
                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible failure point can be FUT server downtime or incorrect API usage. Please copy the log here and contact Lion Ops/Dev and GameTeam FUT SE for further investigation",
                    this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

}
