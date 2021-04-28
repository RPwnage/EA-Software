using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using Rest;

namespace EnvironmentMonitor
{
    public class PowHealthCheckRule : Rule
    {
        private EASFCModuleParam mEASFCParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mEASFCParam;

        public PowHealthCheckRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mEASFCParam.mFIFA_POW_URL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "pow/healthcheck");
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
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible failure point can be EASFC server downtime or incorrect API usage. Please copy the log here and contact Lion Ops/Dev and GameTeam EASFC SE for further investigation",
                    this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class PowAuthRule : Rule
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
            public int clientVersion { get; set; }
            public long nuc { get; set; }
            public long nucleusPersonaId { get; set; }
            public string nucleusPersonaDisplayName { get; set; }
            public string locale { get; set; }
            public string macAddress { get; set; }
            public string method { get; set; }
            public IdentificationJson identification { get; set; }
        }

        public class PowAuthResponsePayload
        {
            public PowAuthResponsePayload()
            {

            }

            public string protocol { get; set; }
            public string ipPort { get; set; }
            public string serverTime { get; set; }
            public string lastOnlineTime { get; set; }
            public string sid { get; set; }
        }

        private EASFCModuleParam mEASFCParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mEASFCParam;

        public PowAuthRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            PreAuthPayloadJson payload = fillPreAuthJsonPayload();

            RestClient client = new RestClient(mEASFCParam.mFIFA_POW_URL);
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

            RestRequest request = new RestRequest(HttpMethod.Post, "pow/auth");
            request.AddContent(content);
            client.Execute(request, mReport);
        }

        private PreAuthPayloadJson fillPreAuthJsonPayload()
        {
            PreAuthPayloadJson json = new PreAuthPayloadJson();
            json.isReadOnly = false;
            json.priorityLevel = 0;
            json.sku = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam.mCMS_SKU;
            json.nuc = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId;
            json.nucleusPersonaId = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mPersonaId;
            json.nucleusPersonaDisplayName = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mDisplayName;
            json.locale = "en-US";
            json.method = "nuc";
            json.clientVersion = 2;

            json.identification.authCode = "";
            json.identification.password = "1Q2w3e4r";
            json.identification.user = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mEmail;

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
                PowAuthResponsePayload payloadJson = jsSerializer.Deserialize<PowAuthResponsePayload>(e.Payload);
                EnvironmentMonitor.EnvironmentConfigParam.getInstance().mEASFCParam.mEasfcSessionId = payloadJson.sid;

                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible failure point can be EASFC server downtime or incorrect login account or credential. Please copy the log here and contact Lion Ops/Dev and GameTeam EASFC SE for further investigation",
                    this.GetType().ToString()));
            }
            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class PowGetTierWeightGameTitleRule : Rule
    {
        private EASFCModuleParam mEASFCParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mEASFCParam;

        public PowGetTierWeightGameTitleRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mEASFCParam.mFIFA_POW_URL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "pow/lvl/weight/tiergp/businessunit/tiertp/fifa");
            request.AddCustomRequestHeaders("X-POW-SID", mEASFCParam.mEasfcSessionId);
            request.AddQueryByParams("gametitle", "fifa16");
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
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible failure point can be EASFC server downtime or incorrect API usage. Please copy the log here and contact Lion Ops/Dev and GameTeam EASFC SE for further investigation",
                    this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class PowGetTierWeightSelfRule : Rule
    {
        private EASFCModuleParam mEASFCParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mEASFCParam;

        public PowGetTierWeightSelfRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mEASFCParam.mFIFA_POW_URL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "pow/lvl/weight/tiergp/businessunit/tiertp/fifa");
            request.AddCustomRequestHeaders("X-POW-SID", mEASFCParam.mEasfcSessionId);
            request.AddQueryByParams("self", "true");
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
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible failure point can be EASFC server downtime or incorrect API usage. Please copy the log here and contact Lion Ops/Dev and GameTeam EASFC SE for further investigation",
                    this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class PowGetChallengeDefinitionRule : Rule
    {
        private EASFCModuleParam mEASFCParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mEASFCParam;

        public PowGetChallengeDefinitionRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mEASFCParam.mFIFA_POW_URL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "/pow/v2/chal");
            request.AddCustomRequestHeaders("X-POW-SID", mEASFCParam.mEasfcSessionId);
            client.Execute(request, mReport);
        }



        private void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            string output = string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload);
            Console.WriteLine(output);
            mReport.AppendLine(output);

            if (e.StatusCode == HttpStatusCode.OK)
            {
                if (e.Payload.Length > 15)
                {
                    JavaScriptSerializer jsSerializer = new JavaScriptSerializer();
                    ChallengeDefinition payloadJson = jsSerializer.Deserialize<ChallengeDefinition>(e.Payload);

                    mEASFCParam.mChallengeData = payloadJson;
                }
                else
                {
                    mEASFCParam.mChallengeData = null;
                }

                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible failure point can be EASFC server downtime or incorrect API usage. Please copy the log here and contact Lion Ops/Dev and GameTeam EASFC SE for further investigation",
                    this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class DownloadChallengeRule : Rule
    {
        private Challenge mParam;

        public DownloadChallengeRule(MonitorModule module, RuleCallbackInterface callbackObj, Challenge parm, String name)
            : base(module, callbackObj, name)
        {
            mParam = parm;
            this.uiContext = ((MultipleChallengesRule)callbackObj).getUiContext();
        }

        /*
          * PerformEvaluation
          * PerformEvaluation override to skip the uiContext assign from current thread, 
          * because this DownloadContentRule is triggered by the MultipleRosterRule which is not a UIThread
         */
        public override void PerformEvaluation()
        {            
            mbSatisfied = false;
            mbGiveRecommendation = true;

            workerThread = new BackgroundWorker();
            workerThread.DoWork += new DoWorkEventHandler(evaluate);
            workerThread.RunWorkerAsync(uiContext);
        }

        public void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0}", e.StatusCode.ToString()));
            mReport.AppendLine(String.Format("Rest Client Finished! - {0}", e.StatusCode.ToString()));

            if (e.StatusCode == HttpStatusCode.OK)
            {
                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible failure point can be Content server downtime or missing content. Please copy the log here and contact Lion Ops/Dev and GameTeam EASFC SE for further investigation",
                    this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            string baseUrl = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mEASFCParam.mFIFA_POW_CONTENT_SERVER_URL;
            RestClient client = new RestClient(baseUrl);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, mParam.customAction);

            Console.WriteLine("Downloading - {0}{1}", baseUrl, mParam.customAction);
            mReport.AppendLine(String.Format("Downloading - {0}{1}", baseUrl, mParam.customAction));

            client.Execute(request, mReport);

        }
    }

    public class MultipleChallengesRule : Rule, RuleCallbackInterface
    {
        private EASFCModuleParam mEASFCParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mEASFCParam;
        private List<DownloadChallengeRule> mDownloadChallengeRule;

        private int mDownloadChallengeIndex;

        public SynchronizationContext getUiContext() { return uiContext; }

        public MultipleChallengesRule(MonitorModule module, RuleCallbackInterface callbackObj)
            : base(module, callbackObj)
        {
            mDownloadChallengeRule = new List<DownloadChallengeRule>();
        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            // MultipleRostersRule goes through the list of rosters, create a DownloadContent rule and download each roster one by one via the RosterContentRule

            if (mEASFCParam.mChallengeData != null)
            {
                // for each roster version, download the FIFA Roster Major, FIFA Roster Minor, FUT DB
                ChallengeDefinition challengeDef = mEASFCParam.mChallengeData as ChallengeDefinition;
                foreach (Challenge challenge in challengeDef.chals)
                {
                    mDownloadChallengeRule.Add(new DownloadChallengeRule(mModule, this, challenge, challenge.name));
                }

                mDownloadChallengeIndex = 0;

                mDownloadChallengeRule[mDownloadChallengeIndex].PerformEvaluation();
            }
            else
            {
                HandleRuleEvaluationComplete();
            }
        }

        public void HandleRuleEvaluationComplete(bool bCriticalError = false)
        {
            // increment mCurrentRosterIndex, if mCurrentRosterindex is less than total roster then download next content, if not flag our own callback listener

            mDownloadChallengeIndex++;

            if (mDownloadChallengeIndex < mDownloadChallengeRule.Count)
            {
                // start next content download
                mDownloadChallengeRule[mDownloadChallengeIndex].PerformEvaluation();
            }
            else
            {
                mbSatisfied = true;
                uiContext.Post(onEvaluationComplete, this);
            }
        }

        public void HandleRecommendationUpdate(object sender, PropertyChangedEventArgs e)
        {
            mCallbackObj.HandleRecommendationUpdate(sender, e);
        }
    }

    public class PowLogoutRule : Rule
    {
        private EASFCModuleParam mEASFCParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mEASFCParam;

        public PowLogoutRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mEASFCParam.mFIFA_POW_URL);
            client.RestClientRequestFinished += this.responseCallback;

            string fifaYear = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerName.Contains("2016") ? "fifa16" : "fifa15";

            RestRequest request = new RestRequest(HttpMethod.Delete, "pow/auth");
            request.AddCustomRequestHeaders("X-POW-SID", mEASFCParam.mEasfcSessionId);

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
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible failure point can be POW server downtime or incorrect API usage. Please copy the log here and contact Lion Ops/Dev and GameTeam EASFC SE for further investigation",
                    this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }

    }


#region Helper Classes
    public class ChallengeDefinition
    {
        public ChallengeDefinition() 
        {
            chals = new Challenge[10];
        }
        public Challenge[] chals {get;set;}
    }

    public class Challenge
    {
        public Challenge()
        {
            challengeActivities = new ChallengeActivity[10];
        }

        public string name { get; set; }
        public string gameTitle { get; set; }
        public int challengeId { get; set; }
        public string desc { get; set; }
        public string imgURL { get; set; }
        public string imgURLFull { get; set; }
        public string customAction { get; set; }
        public string activeFrom { get; set; }
        public string activeUntil { get; set; }
        public string activeType { get; set; }
        public int totalActivity { get; set; }
        public string skuMode {get; set;}
        public ChallengeActivity[] challengeActivities{get; set;}
        
    }

    public class ChallengeActivity
    {
        public ChallengeActivity()
        {
            difficultyExpOffsets = new int[4];
            difficultyFundOffsets = new int[4];
        }

        public int challengeActivityId { get; set; }
        public bool isMandatory { get; set; }
        public string name { get; set; }
        public int baseExp { get; set; }
        public int[] difficultyExpOffsets { get; set; }
        public int baseFunds { get; set; }
        public int[] difficultyFundOffsets { get; set; }
        public string currencyName { get; set; }
    }
#endregion
}
