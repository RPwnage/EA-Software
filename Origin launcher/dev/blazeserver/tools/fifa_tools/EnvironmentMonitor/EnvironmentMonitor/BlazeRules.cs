using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Shapes;
using System.Net;
using System.Xml;

using Rest;
using System.Net.Http;
using System.Xml.Serialization;
using System.Reflection;
using System.ComponentModel;


namespace EnvironmentMonitor
{
/*
 * BlazeServerExistenceRule
 * Make a getServerInstanceHttp call attempt to get <Blaze server name>
 *      Success: Payload with host IP and port 
 *      Failure: server response with error code
 */
    class BlazeServerExistenceRule :Rule
    {
        public BlazeServerExistenceRule(MonitorModule module, RuleCallbackInterface callbackObj) : base(module, callbackObj) {}

        public string StringIntToIP(String sDecIP)
        {
            uint ip = Convert.ToUInt32(sDecIP);
            var a = ip >> 24 & 0xFF;
            var b = ip >> 16 & 0xFF;
            var c = ip >> 8 & 0xFF;
            var d = ip & 0xFF;

            return String.Format("{0}.{1}.{2}.{3}", a, b, c, d);
        }

        protected override void onEvaluationComplete(object state)
        {
            mModule.updateStatus();
            
            mCallbackObj.HandleRuleEvaluationComplete(!mbSatisfied);            
        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        { 
            // Make an async call to GOS redirector to get back the Hostname, IP and Port of the Blaze server instance
            //RestClient client = new RestClient("https://winter15.gosredirector.stest.ea.com:42230/");
            RestClient client = new RestClient(EnvironmentConfigParam.getInstance().mBlazeParam.mRedirevtorURL+":42230");
            client.RestClientRequestFinished += this.responseCallback;

            BlazeModuleParam blazeParam = EnvironmentConfigParam.getInstance().mBlazeParam;
            RestRequest request = new RestRequest(HttpMethod.Post, "/redirector/getServerInstance");

            string xmlPayload = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><serverinstancerequest><blazesdkversion>15.1.1.0.1</blazesdkversion><blazesdkbuilddate>Jun 26 2015 15: 41:05</blazesdkbuilddate><clientname>" + blazeParam.mGameTitle + blazeParam.mGameYear + "</clientname><clienttype>CLIENT_TYPE_GAMEPLAY_USER</clienttype><clientplatform>" + blazeParam.mPlatform + "</clientplatform><clientskuid>FIFAPC</clientskuid><clientversion>198532</clientversion><dirtysdkversion>15.1.1.0.1</dirtysdkversion><environment>" + blazeParam.mGOSEnv + "</environment><firstpartyid member=\"2\"><valu>apoopoo0200</valu></firstpartyid><clientlocale>1701724993</clientlocale><name>" + blazeParam.mBlazeServerName + "</name><platform>" + blazeParam.mPlatform + "</platform><connectionprofile>standardSecure_v4</connectionprofile><istrial>0</istrial></serverinstancerequest>";

            StringContent content = new StringContent(xmlPayload, Encoding.UTF8, "application/xml");
            request.AddContent(content);      

            client.Execute(request, mReport);            
        }
                
        public void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0} - {1} ", e.StatusCode.ToString(), e.Payload));
            mReport.AppendFormat("Rest Client Finished! - {0} - {1}\n", e.StatusCode.ToString(), e.Payload);

            if (e.StatusCode == HttpStatusCode.OK)
            {

                BlazeModuleParam blazeParam = EnvironmentConfigParam.getInstance().mBlazeParam;

                XmlDocument doc = new XmlDocument();

                doc.LoadXml(e.Payload);
                                               
                if (doc.DocumentElement.GetElementsByTagName("serverinstanceerror").Count == 0)                
                {
                    XmlNodeList nodelist = doc.DocumentElement.GetElementsByTagName("hostname");
                    blazeParam.mBlazeHostName = nodelist.Item(0).InnerText;

                    nodelist = doc.DocumentElement.GetElementsByTagName("ip");
                    blazeParam.mBlazeServerIP = StringIntToIP(nodelist.Item(0).InnerText);

                    nodelist = doc.DocumentElement.GetElementsByTagName("port");
                    blazeParam.mBlazePort = Convert.ToString(Convert.ToInt32(nodelist.Item(0).InnerText) + 1);

                    mbSatisfied = true;
                }
                else
                {
                    mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Redirector responsed with serverinstanceerror, possible cause is incorrect Blaze server name - {1} or the Blaze server is currently down. Please copy the log here and contact GOS Ops and GameTeam Online SE for further investigation",
                        this.GetType().ToString(), EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerName));
                }
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Request to {1} has failed, the GOS redirector could be down. This rule is crucial to all of online connectivity as it serves as the first entry point for all service, please copy the log here and contact GOS Ops and GameTeam Online SE for further investigation",
                    this.GetType().ToString(), (EnvironmentConfigParam.getInstance().mBlazeParam.mRedirevtorURL+":42230")));
            }

            uiContext.Post(onEvaluationComplete, this);                    

        }
    }

    class BlazePreAuthRule : Rule
    {
        public BlazePreAuthRule(MonitorModule module, RuleCallbackInterface callbackObj) : base(module, callbackObj) { }

        public void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));
            mReport.AppendLine(String.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));

            if (e.StatusCode == HttpStatusCode.OK)
            {
                XmlDocument doc = new XmlDocument();

                doc.LoadXml(e.Payload);

                XmlNodeList nodelist = doc.DocumentElement.SelectNodes("config/config/entry");

                BlazeModuleParam blazeParam = EnvironmentConfigParam.getInstance().mBlazeParam;
                NucleusModuleParam nucleusParam = EnvironmentConfigParam.getInstance().mNucleusParam;

                foreach (XmlNode node in nodelist)
                {
                    switch (node.Attributes["key"].Value)
                    {
                        case ("nucleusConnect"): nucleusParam.mNucleusConnect = node.InnerText; break;
                        case ("nucleusConnectTrusted"): nucleusParam.mNucleusConnectTrusted = node.InnerText; break;
                        case ("nucleusProxy"): nucleusParam.mNucleusProxy = node.InnerText; break;
                        case ("nucleusRedirect"): nucleusParam.mNucleusRedirect = node.InnerText; break;
                        case ("identityRedirectUri"): nucleusParam.mNucleusRedirect = node.InnerText; break;
                        case ("blazeSdkClientId"): nucleusParam.mBlazeSDKClientId = node.InnerText; break;
                        case ("blazeSdkClientSecret"): nucleusParam.mBlazeSDKClientSecret = node.InnerText; break;
                        case ("blazeServerClientId"): nucleusParam.mBlazeServerClientId = node.InnerText; break;

                    }
                }

                blazeParam.mBlazeServerVersion = doc.DocumentElement.SelectSingleNode("serverversion").InnerText;

                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is {1} Blaze server is currently down. This rule is crucial to all of online connectivity as it serves as the first entry point for all service, , please copy the log here and contact GOS Ops and GameTeam Online SE for further investigation",
                    this.GetType().ToString(), EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerName));
            }

            uiContext.Post(onEvaluationComplete, this);

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            // Make the PreAuth RPC Web call directly to the Blaze server
            BlazeModuleParam blazeParam = EnvironmentConfigParam.getInstance().mBlazeParam;
            string hostURL = "http://" + blazeParam.mBlazeServerIP + ":" + blazeParam.mBlazePort;
            RestClient client = new RestClient(hostURL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "/util/preAuth");
            request.AddQueryByParams("cdat|iito", "0");
            request.AddQueryByParams("cdat|lang", "1701726018");
            request.AddQueryByParams("cdat|svcn", blazeParam.mBlazeServerName);
            request.AddQueryByParams("cdat|type", "CLIENT_TYPE_GAMEPLAY_USER");
            request.AddQueryByParams("cinf|bsdk", "15.1.1.0.1");
            request.AddQueryByParams("cinf|btim", "Aug 13 2015 01:33:08");
            request.AddQueryByParams("cinf|clnt", blazeParam.mGameTitle + blazeParam.mGameYear);
            request.AddQueryByParams("cinf|cpft", "20");
            request.AddQueryByParams("cinf|csku", "FIFAPC");
            request.AddQueryByParams("cinf|cver", "251601");
            request.AddQueryByParams("cinf|dsdk", "15.1.1.0.1");
            request.AddQueryByParams("cinf|env", blazeParam.mGOSEnv);
            request.AddQueryByParams("cinf|loc", "1701726018");
            request.AddQueryByParams("fccr|cfid", "BlazeSDK");
            request.AddQueryByParams("ladd", "2868981002");

            client.Execute(request, mReport);
        }
    }

 
    class BlazeFetchConfigRule : Rule
    {
        public BlazeFetchConfigRule(MonitorModule module, RuleCallbackInterface callbackObj) : base(module, callbackObj) { }

        public void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));
            mReport.AppendLine(String.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));

            if (e.StatusCode == HttpStatusCode.OK)
            {
                XmlDocument doc = new XmlDocument();

                doc.LoadXml(e.Payload);

                XmlNodeList nodelist = doc.DocumentElement.SelectNodes("config/entry");
                                              
                CMSModuleParam cmsParam = EnvironmentConfigParam.getInstance().mCMSParam;
                EASFCModuleParam easfcParam = EnvironmentConfigParam.getInstance().mEASFCParam;
                FUTModuleParam futParam = EnvironmentConfigParam.getInstance().mFUTParam;
                
                foreach (XmlNode node in nodelist)
                {
                    switch (node.Attributes["key"].Value)
                    {
                        case ("OSDK_EASW_AUTH_URL"): cmsParam.mOSDK_EASW_AUTH_URL = node.InnerText; break;
                        case ("OSDK_EASW_GF_FILE_URL"): cmsParam.mOSDK_EASW_GF_FILE_URL = node.InnerText; break;
                        case ("CMS_BASE_URL"): cmsParam.mCMS_BASE_URL = node.InnerText; break;
                        case ("CMS_API_KEY"): cmsParam.mCMS_API_KEY = node.InnerText; break;
                        case ("CMS_HOSP_SKU_KC"): cmsParam.mCMS_HOSP_SKU_KC = node.InnerText; break;
                        case ("CMS_SKU"): cmsParam.mCMS_SKU = node.InnerText; break;
                        case ("CMS_UPLOAD_PERMISSION_URL"): cmsParam.mCMS_UPLOAD_PERMISSION_URL = node.InnerText; break;
                        case ("CMS_UPLOAD_PERMISSION_PARAMS"): cmsParam.mCMS_UPLOAD_PERMISSION_PARAMS = node.InnerText; break;
                        case ("CONTENT_URL"): cmsParam.mCONTENT_URL = node.InnerText; break;
                        case ("ROUTINGCFGFILE_URL"): cmsParam.mROUTINGCFGFILE_URL = node.InnerText; break;

                        case ("FIFA_POW_URL"): easfcParam.mFIFA_POW_URL= node.InnerText; break;
                        case ("FIFA_POW_CONTENT_SERVER_URL"): easfcParam.mFIFA_POW_CONTENT_SERVER_URL = node.InnerText; break;
                        case ("FIFA_POW_NUCLEUS_PROXY_URL"): easfcParam.mFIFA_POW_NUCLEUS_PROXY_URL = node.InnerText; break;
                        case ("FIFA_POW_MMM_URI"): easfcParam.mFIFA_POW_MMM_URI = node.InnerText; break;
                        case ("FIFA_RS4_URL"): easfcParam.mFIFA_RS4_URL = node.InnerText; break;                        
                        
                        case ("FUT_RS4_ADMIN_URL"): futParam.mFUT_RS4_ADMIN_URL = node.InnerText; break;                        
                        case ("FUT_RS4_BASE_URL"): futParam.mFUT_RS4_BASE_URL = node.InnerText; break;                        
                        case ("FUTBOOTCFGFILE_URL"): futParam.mFUTBOOTCFGFILE_URL = node.InnerText; break;
                    }
                }
                                
                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is {1} Blaze server is currently down. This rule is crucial to all of online connectivity as it serves as the first entry point for all service, please copy the log here and contact GOS Ops and GameTeam Online SE for further investigation",
                    this.GetType().ToString(), EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerName));
            }

            uiContext.Post(onEvaluationComplete, this);

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            // Make the FetchClientConfig RPC Web call directly to the Blaze server
            string hostURL = "http://" + EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerIP + ":" + EnvironmentConfigParam.getInstance().mBlazeParam.mBlazePort;
            RestClient client = new RestClient(hostURL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "/util/fetchClientConfig");
            request.AddQueryByParams("cfid", "OSDK_CORE");

            client.Execute(request, mReport);
        }
    }

    public class BlazeLoginRule : Rule
    {
        public class PersonaDetails
        {
            public string displayname = string.Empty;
            public int lastauthenticated = 0;
            public long personaid = 0;
            public string clientplatform = string.Empty;
            public string status = string.Empty;
            public long extid = 0;

            public PersonaDetails() { }
        }

        public class UserLoginInfo
        {
            public int isfirstconsolelogin = 0;
            public long blazeuserid = 0;
            public int isfirstlogin = 0;
            public string sessionkey = string.Empty;
            public long lastlogindatetime = 0;
            public string email = string.Empty;
            public PersonaDetails personadetails = new PersonaDetails();
            public long userid = 0;

            public UserLoginInfo() { }
        }

        [XmlRoot("login")]
        public class LoginResponse
        {
            public int isanonymous = 0;
            public int needslegaldoc = 0;
            public UserLoginInfo userlogininfo = new UserLoginInfo();
            public int isoflegalcontactage = 0;
            public int isunderage = 0;

            public LoginResponse() { }
        }

        public BlazeLoginRule(MonitorModule module, RuleCallbackInterface callbackObj) : base(module, callbackObj) { }

        public void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));
            mReport.AppendLine(String.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));

            if (e.StatusCode == HttpStatusCode.OK)
            {
                LoginResponse response = DeserializeXmlResponse(e.Payload);
                if (response != null)
                {
                    EnvironmentConfigParam.getInstance().mBlazeUserParam.mBlazeId = response.userlogininfo.blazeuserid;
                    EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId = response.userlogininfo.userid;
                    EnvironmentConfigParam.getInstance().mBlazeUserParam.mPersonaId = response.userlogininfo.personadetails.personaid;
                    EnvironmentConfigParam.getInstance().mBlazeUserParam.mSessionKey = response.userlogininfo.sessionkey;
                    EnvironmentConfigParam.getInstance().mBlazeUserParam.mEmail = response.userlogininfo.email;
                    EnvironmentConfigParam.getInstance().mBlazeUserParam.mDisplayName = response.userlogininfo.personadetails.displayname;
                    EnvironmentConfigParam.getInstance().mBlazeUserParam.mClientPlatform = response.userlogininfo.personadetails.clientplatform;

                    mbSatisfied = true;
                }
                else
                {
                    mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is incorrect login account or login credential. Please copy the log here and contact GameTeam Online SE and EADP Identity team for further investigation",
                        this.GetType().ToString()));
                }
                
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is {1} Blaze server is currently down. Please copy the log here and contact GOS Ops and GameTeam Online SE for further investigation",
                    this.GetType().ToString(), EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerName));
            }

            uiContext.Post(onEvaluationComplete, this);

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            string hostURL = "http://" + EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerIP + ":" + EnvironmentConfigParam.getInstance().mBlazeParam.mBlazePort;
            RestClient client = new RestClient(hostURL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "/authentication/login");
            request.AddQueryByParams("auth", EnvironmentConfigParam.getInstance().mNucleusParam.mServerAuthCode);
            request.AddQueryByParams("exti", "0");


            client.Execute(request, mReport);
        }

        private LoginResponse DeserializeXmlResponse(string response)
        {
            LoginResponse loginResponse = new LoginResponse();
            using (StringReader stringReader = new StringReader(response))
            {
                using (XmlTextReader xmlReader = new XmlTextReader(stringReader))
                {
                    XmlSerializer xmlSerializer = new XmlSerializer(loginResponse.GetType());
                    object settingsObject;

                    if (xmlSerializer.CanDeserialize(xmlReader))
                    {
                        settingsObject = xmlSerializer.Deserialize(xmlReader);
                        Type type = loginResponse.GetType();

                        FieldInfo[] fields = type.GetFields();
                        foreach (FieldInfo field in fields)
                        {
                            if (field.IsPublic)
                            {
                                field.SetValue(loginResponse, field.GetValue(settingsObject));
                            }
                        }
                    }
                }
            }
            return loginResponse;
            
        }
    }

    public class BlazeGameReportRule : Rule
    {
        public BlazeGameReportRule(MonitorModule module, RuleCallbackInterface callbackObj) : base(module, callbackObj) { }

        public void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));
            mReport.AppendLine(String.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));

            if (e.StatusCode == HttpStatusCode.OK)
            {
                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is {1} Blaze server is currently down. Please copy the log here and contact GOS Ops and GameTeam Online SE for further investigation",
                    this.GetType().ToString(), EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerName));
            }

            uiContext.Post(onEvaluationComplete, this);

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            string hostURL = "http://" + EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerIP + ":" + EnvironmentConfigParam.getInstance().mBlazeParam.mBlazePort;
            RestClient client = new RestClient(hostURL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "/gamereporting/submitOfflineGameReport/" + EnvironmentConfigParam.getInstance().mBlazeUserParam.mSessionKey);
            request.AddQueryByParams("auth", EnvironmentConfigParam.getInstance().mNucleusParam.mServerAuthCode);
            request.AddQueryByParams("exti", "0");


            client.Execute(request, mReport);
        }
    }
}
