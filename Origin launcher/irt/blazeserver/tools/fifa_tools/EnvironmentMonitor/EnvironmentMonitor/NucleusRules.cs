using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.ComponentModel;
using Rest;

namespace EnvironmentMonitor
{
    public class GoodAuthRepsonseJson
    {
        public string access_token { get; set; }
        public string origin_access_token { get; set; }
        public string token_type { get; set; }
        public string expires_in { get; set; }
        public string refresh_token { get; set; }
        public string origin_refresh_token { get; set; }
        public string id_token { get; set; }
        public string origin_id_token { get; set; }
        public List<string> additional_access_tokens { get; set; }

        public GoodAuthRepsonseJson()
        {
            access_token = string.Empty;
            origin_access_token = string.Empty;
            token_type = string.Empty;
            expires_in = string.Empty;
            refresh_token = string.Empty;
            origin_refresh_token = string.Empty;
            id_token = string.Empty;
            origin_id_token = string.Empty;
            additional_access_tokens = new List<string>();
        }
    }

    public class NucleusClientAuthRule : Rule
    {
        private NucleusModuleParam mNucleusModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam;
        private const string PC_TOKEN_ISSUER_CLIENT_ID = "test_token_issuer";
        private const string PC_TOKEN_ISSUER_CLIENT_SECRET = "test_token_issuer_secret";
        private const string PC_TEST_ENV_PERSONA_ID = "1000144953451";

        private const string CONSOLE_TOKEN_ISSUER_CLIENT_ID = "FIFA-16-Test-Client";
        private const string CONSOLE_TOKEN_ISSUER_CLIENT_SECRET = "MZRtcNCisFW2cypWHplyczwcv4QsVY47ABEhSwUtUUNJPtKm9oQIi6iANLnfpColPZAr98cmd3GMBOS0";

        private const string LOCALE = "en_US";

        
        public NucleusClientAuthRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {
        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = null;
            RestRequest request = null;

            if (mNucleusModuleParam.mBlazeSDKClientId.Contains("PC"))
            {
                client = new RestClient(mNucleusModuleParam.mNucleusConnect);
                client.RestClientRequestFinished += this.responsePCPreAuthTokenCallback;
                request = new RestRequest(HttpMethod.Post, "/connect/token");

                request.AddQueryByParams("grant_type", "client_credentials");
                request.AddQueryByParams("client_id", PC_TOKEN_ISSUER_CLIENT_ID);
                request.AddQueryByParams("client_secret", PC_TOKEN_ISSUER_CLIENT_SECRET);
                request.AddQueryByParams("pidId", PC_TEST_ENV_PERSONA_ID);

                request.AddCustomRequestHeaders("enable-client-cert-auth", "true");

                StringContent content = new StringContent("", Encoding.UTF8, "application/x-www-form-urlencoded");
                request.AddContent(content);
            }
            else
            {
                WebRequestHandler handler = new WebRequestHandler();
                handler.AllowAutoRedirect = false;

                client = new RestClient(mNucleusModuleParam.mNucleusConnect, handler);
                client.RestClientRequestFinished += this.responseConsoleCallback;
                request = new RestRequest(HttpMethod.Get, "/connect/auth");

                request.AddQueryByParams("response_type", "code");
                request.AddQueryByParams("client_id", CONSOLE_TOKEN_ISSUER_CLIENT_ID);
                request.AddQueryByParams("redirect_uri", mNucleusModuleParam.mNucleusRedirect);
                request.AddQueryByParams("local", LOCALE);
                request.AddQueryByParams("scope", "basic.identity basic.persona signin offline");
                request.AddQueryByParams("prompt", "none");
                request.AddQueryByParams("isgen4", "true");

                if (mNucleusModuleParam.mBlazeSDKClientId.Contains("PS4") || mNucleusModuleParam.mBlazeSDKClientId.Contains("Ket"))
                {
                    string deviceId = "psn_doomtastico";
                    request.AddQueryByParams("psn_ticket", deviceId);
                }
                else if (mNucleusModuleParam.mBlazeSDKClientId.Contains("XB1") || mNucleusModuleParam.mBlazeSDKClientId.Contains("CAP"))
                {
                    string deviceId = "xbox_doomtastico";
                    request.AddQueryByParams("xtoken", deviceId);
                }
            }

            client.Execute(request, mReport);
        }

        public void responseConsoleCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));
            mReport.AppendLine(String.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));

            if (e.StatusCode == HttpStatusCode.Redirect)
            {
                HttpResponseHeaders headers = e.HttpResponse.Headers;
                if (headers.Location.Query.Contains("code"))
                {
                    string[] query = headers.Location.Query.Split('=');
                    EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mClientAuthCode = query[1];
                    Console.WriteLine(string.Format("AuthCode: {0}",query[1]));
                    mReport.AppendFormat("AuthCode: {0}", query[1]);
                    mbSatisfied = true;
                }
                
            }

            uiContext.Post(onEvaluationComplete, this);
        }

        

        private void responsePCPreAuthTokenCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));
            mReport.AppendLine(String.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));

            RestClient client = sender as RestClient;
            client.RestClientRequestFinished -= this.responsePCPreAuthTokenCallback;

            if (e.StatusCode == HttpStatusCode.OK)
            {
                JavaScriptSerializer serializer = new JavaScriptSerializer();
                GoodAuthRepsonseJson authResponse = serializer.Deserialize<GoodAuthRepsonseJson>(e.Payload);
                EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mClientAuthToken = authResponse.access_token;
                Console.WriteLine(string.Format("PC Access_Token: {0}", authResponse.access_token));
                mReport.AppendFormat("PC Access_Token: {0}", authResponse.access_token);
                
                mbSatisfied = true;
            }

            uiContext.Post(onEvaluationComplete, this);
        }

    }

    public class NucleusClientTokenRule : Rule
    {
        private const string CONSOLE_TOKEN_ISSUER_CLIENT_ID = "FIFA-16-Test-Client";
        private const string CONSOLE_TOKEN_ISSUER_CLIENT_SECRET = "MZRtcNCisFW2cypWHplyczwcv4QsVY47ABEhSwUtUUNJPtKm9oQIi6iANLnfpColPZAr98cmd3GMBOS0";

        private NucleusModuleParam mNucleusModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam;

        public NucleusClientTokenRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {
        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = null;
            RestRequest request = null;

            if (!mNucleusModuleParam.mBlazeSDKClientId.Contains("PC"))
            {
                client = new RestClient(mNucleusModuleParam.mNucleusConnect);
                client.RestClientRequestFinished += this.responseCallback;

                request = new RestRequest(HttpMethod.Post, "/connect/token");
                request.AddQueryByParams("grant_type", "authorization_code");
                request.AddQueryByParams("client_id", CONSOLE_TOKEN_ISSUER_CLIENT_ID);
                request.AddQueryByParams("client_secret", CONSOLE_TOKEN_ISSUER_CLIENT_SECRET);
                request.AddQueryByParams("redirect_uri", mNucleusModuleParam.mNucleusRedirect);
                request.AddQueryByParams("code", mNucleusModuleParam.mClientAuthCode);

                StringContent content = new StringContent("", Encoding.UTF8, "application/x-www-form-urlencoded");
                request.AddContent(content);

                client.Execute(request, mReport);

            }
            else
            {
                Thread workerThread = new Thread(new ThreadStart(PcComplete));
                workerThread.Start();
            }
        }

        public void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));
            mReport.AppendLine(String.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));

            if (e.StatusCode == HttpStatusCode.OK)
            {
                JavaScriptSerializer serializer = new JavaScriptSerializer();
                GoodAuthRepsonseJson authResponse = serializer.Deserialize<GoodAuthRepsonseJson>(e.Payload);

                Console.WriteLine(string.Format("AuthToken: {0}", authResponse.access_token));
                mReport.AppendFormat("AuthToken: {0}", authResponse.access_token);

                EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mClientAuthToken = authResponse.access_token;


                mbSatisfied = true;
            }

            uiContext.Post(onEvaluationComplete, this);
        }

        private void PcComplete()
        {
            Console.WriteLine(string.Format("Skip this rule for PC"));
            mReport.AppendFormat("Skip this rule for PC");

            mbSatisfied = true;

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class NucleusServerAuthRule : Rule
    {
        private NucleusModuleParam mNucleusModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam;


        public NucleusServerAuthRule(MonitorModule module, RuleCallbackInterface callbackObj)
            : base(module, callbackObj)
        {
        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            WebRequestHandler handler = new WebRequestHandler();
            handler.AllowAutoRedirect = false;

            RestClient client = new RestClient(mNucleusModuleParam.mNucleusConnect, handler);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "/connect/auth");
            request.AddQueryByParams("response_type", "code");
            request.AddQueryByParams("client_id", mNucleusModuleParam.mBlazeServerClientId);
            request.AddQueryByParams("redirect_uri", mNucleusModuleParam.mNucleusRedirect);
            request.AddQueryByParams("prompt", "none");

            request.AddQueryByParams("access_token", mNucleusModuleParam.mClientAuthToken);

            client.Execute(request, mReport);

        }

        public void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));
            mReport.AppendLine(String.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));

            if (e.StatusCode == HttpStatusCode.Redirect)
            {
                HttpResponseHeaders headers = e.HttpResponse.Headers;
                if (headers.Location.Query.Contains("code"))
                {
                    string[] query = headers.Location.Query.Split('=');
                    EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mServerAuthCode = query[1];
                    Console.WriteLine(string.Format("AuthCode: {0}", query[1]));
                    mReport.AppendFormat("AuthCode: {0}", query[1]);
                    mbSatisfied = true;
                }

            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class NucleusLionAuthCodeRule : Rule
    {
        private NucleusModuleParam mNucleusModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam;
        private const string LION_CLIENT_ID = "FOS-SERVER";
        private const string LOCALE = "en_US";

        public NucleusLionAuthCodeRule(MonitorModule module, RuleCallbackInterface callbackObj)
            : base(module, callbackObj)
        {
        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            WebRequestHandler handler = new WebRequestHandler();
            handler.AllowAutoRedirect = false;

            RestClient client = new RestClient(mNucleusModuleParam.mNucleusConnect, handler);
            client.RestClientRequestFinished += this.responseCallback;
            
            RestRequest request = new RestRequest(HttpMethod.Get, "/connect/auth");

            request.AddQueryByParams("response_type", "code");
            request.AddQueryByParams("client_id", LION_CLIENT_ID);
            request.AddQueryByParams("local", LOCALE);
            request.AddQueryByParams("scope", "basic.identity+basic.persona+offline");
            request.AddQueryByParams("redirect_uri", mNucleusModuleParam.mNucleusConnect + "/");

            request.AddQueryByParams("access_token", mNucleusModuleParam.mClientAuthToken);

            client.Execute(request, mReport);

        }

        private void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));
            mReport.AppendLine(String.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));

            if (e.StatusCode == HttpStatusCode.Redirect)
            {
                HttpResponseHeaders headers = e.HttpResponse.Headers;
                if (headers.Location.Query.Contains("code"))
                {
                    string[] query = headers.Location.Query.Split('=');
                    EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mLionAuthCode = query[1];
                    Console.WriteLine(string.Format("AuthCode: {0}", query[1]));
                    mReport.AppendFormat("AuthCode: {0}", query[1]);
                    mbSatisfied = true;
                }

            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }
}
