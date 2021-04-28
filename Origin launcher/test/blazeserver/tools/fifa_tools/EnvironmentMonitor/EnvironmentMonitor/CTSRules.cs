using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using Rest;
using System.Security.Cryptography;
using System.Web;
using System.Collections.Specialized;
using System.Web.Script.Serialization;
using XmsHdHelperClasses;
using System.ComponentModel;
using System.Reflection;
using System.Net.Http.Headers;
using System.Xml;

namespace EnvironmentMonitor
{
   
    public class CtsEaswAuthRule : Rule
    {
        private CMSModuleParam mCmsModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam;

        public CtsEaswAuthRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mCmsModuleParam.mOSDK_EASW_AUTH_URL);            
            client.RestClientRequestFinished += this.responseCallback;

            string blazeServerClientId = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mBlazeServerClientId;

            List<KeyValuePair<string, string>> nameValueCollection = new List<KeyValuePair<string, string>>();
            if (!blazeServerClientId.Contains("PC"))
            {
                nameValueCollection.Add(new KeyValuePair<string, string>("accesstoken", EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mClientAuthToken));
                nameValueCollection.Add(new KeyValuePair<string, string>("skuid", GetGameSkuAndSecret(blazeServerClientId)[0]));
                nameValueCollection.Add(new KeyValuePair<string, string>("locale", "en_US"));
            }
            else
            {
                nameValueCollection.Add(new KeyValuePair<string, string>("nat", EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mClientAuthToken));
                nameValueCollection.Add(new KeyValuePair<string, string>("namespace", "cem_ea_id"));
                nameValueCollection.Add(new KeyValuePair<string, string>("display_name", EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mDisplayName));
                nameValueCollection.Add(new KeyValuePair<string, string>("authType", "ENGINE"));
                nameValueCollection.Add(new KeyValuePair<string, string>("skuid", GetGameSkuAndSecret(blazeServerClientId)[0]));
                nameValueCollection.Add(new KeyValuePair<string, string>("locale", "en_US"));
            }

            string path = blazeServerClientId.Contains("PC") ? "/v2/authenticationNucleusPersona" : "/nv2/auth";
            RestRequest request = new RestRequest(HttpMethod.Post, path);

            // Create SHA1 Hash
            byte[] headerBytes = Encoding.ASCII.GetBytes(string.Format("POST {0}", path));
            byte[] gameSecretBytes = Encoding.ASCII.GetBytes(GetGameSkuAndSecret(blazeServerClientId)[1]);

            SHA1 sha1 = SHA1.Create();
            sha1.TransformBlock(headerBytes, 0, headerBytes.Length, headerBytes, 0);
            sha1.TransformFinalBlock(gameSecretBytes, 0, gameSecretBytes.Length);
            string base64ReqHash = HttpUtility.UrlEncode(Convert.ToBase64String(sha1.Hash));
            sha1 = null;

            string strContent = ToQueryString(nameValueCollection);
            mReport.AppendFormat("Content - {0}\n", strContent);

            sha1 = SHA1.Create();
            byte[] contentBytes = Encoding.ASCII.GetBytes(strContent);
            sha1.TransformBlock(contentBytes, 0, contentBytes.Length, contentBytes, 0);
            sha1.TransformFinalBlock(gameSecretBytes, 0, gameSecretBytes.Length);
            string base64ContentHash = HttpUtility.UrlEncode(Convert.ToBase64String(sha1.Hash));

            request.AddCustomRequestHeaders("EASW-Version", "2.0.5.0");
            request.AddCustomRequestHeaders("EASW-Request-Signature", base64ReqHash);
            request.AddCustomRequestHeaders("EASW-Content-Signature", base64ContentHash);

            FormUrlEncodedContent content = new FormUrlEncodedContent(nameValueCollection);

            request.AddContent(content);
            client.Execute(request, mReport);
        }

        private string ToQueryString(List<KeyValuePair<string, string>> nameValueCollection)
        {
            string returnVal = string.Empty;
            foreach (KeyValuePair<string, string> kvp in nameValueCollection)
            {
                if (!string.IsNullOrEmpty(returnVal))
                {
                    returnVal += "&";
                }
                returnVal += string.Format("{0}={1}", kvp.Key, kvp.Value);
            }

            returnVal = HttpUtility.UrlEncode(returnVal);
            return returnVal;
        }

        private string[] GetGameSkuAndSecret(string serverClientId)
        {
            string[] gameSkuSecretPair = new string[2];

            if (EnvironmentConfigParam.getInstance().mBlazeParam.mGameYear == "17")
            {
                if (serverClientId.Contains("PC"))
                {
                    gameSkuSecretPair[0] = "FFA16PCC";
                    gameSkuSecretPair[1] = "8QGEYVM4DDHJE79";
                }
                else if (serverClientId.Contains("PS4"))
                {
                    gameSkuSecretPair[0] = "FFA17PS4";
                    gameSkuSecretPair[1] = "R3N2PCTK5TEYVMX";
                }
                else
                {
                    gameSkuSecretPair[0] = "FFA17XBO";
                    gameSkuSecretPair[1] = "J2LQFX2S29NYPFK";
                }
            }
            else if (EnvironmentConfigParam.getInstance().mBlazeParam.mGameYear == "16")
            {
                if (serverClientId.Contains("PC"))
                {
                    gameSkuSecretPair[0] = "FFA16PCC";
                    gameSkuSecretPair[1] = "8QGEYVM4DDHJE79";
                }
                else if (serverClientId.Contains("PS4"))
                {
                    gameSkuSecretPair[0] = "FFA16PS4";
                    gameSkuSecretPair[1] = "R3N2PCTK5TEYVMX";
                }
                else
                {                 
                    gameSkuSecretPair[0] = "FFA16XBO";
                    gameSkuSecretPair[1] = "J2LQFX2S29NYPFK";
                }              
            }
            else if (EnvironmentConfigParam.getInstance().mBlazeParam.mGameYear == "15")
            {
                if (serverClientId.Contains("PC"))
                {
                    gameSkuSecretPair[0] = "FFA15PCC";
                    gameSkuSecretPair[1] = "0E2BYXDEALEAM3S";
                }
                else if (serverClientId.Contains("KETTLE"))
                {
                    gameSkuSecretPair[0] = "FFA15PS4";
                    gameSkuSecretPair[1] = "RJ73HIAB2H2DGCN";
                }
                else
                {
                    gameSkuSecretPair[0] = "FFA15XBO";
                    gameSkuSecretPair[1] = "J9L1J41XNXHW5WM";
                }
            }

            mReport.AppendFormat("SKU: {0} and Secret: {1}", gameSkuSecretPair[0], gameSkuSecretPair[1]);
            return gameSkuSecretPair;
        }

        protected virtual void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            string output = string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload);
            Console.WriteLine(output);
            mReport.AppendLine(output);

            if (e.StatusCode == HttpStatusCode.Created)
            {
                HttpResponseMessage response = e.HttpResponse;
                IEnumerable<string> values = new List<string>().AsEnumerable();
                bool isHeaderAvailable = response.Headers.TryGetValues("EASW-Token", out values);

                if (isHeaderAvailable)
                {
                    Console.WriteLine("CTS EASW Token: " + values.FirstOrDefault());
                    EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam.mCTSAuthCode = values.FirstOrDefault();
                    mReport.AppendFormat("CTS EASW Token: {0}", values.FirstOrDefault());
                }

                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is {1} EASW server is currently down or incorrect login info. Please copy the log here and contact CTS Ops and GameTeam Online SE for further investigation",
                    this.GetType().ToString(), mCmsModuleParam.mOSDK_EASW_AUTH_URL));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    #region XMS HD Rules
    public class CtsXmsHdPublishRule : Rule
    {
        private CMSModuleParam mCmsModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam;
        private const string MULTIPART_BOUNDARY = "-31415926535897932384626";

        public CtsXmsHdPublishRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mCmsModuleParam.mOSDK_EASW_AUTH_URL);
            client.RestClientRequestFinished += this.responseCallback;

            string blazeServerClientId = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mBlazeServerClientId;

            RestRequest request = new RestRequest(HttpMethod.Post, "xmshd_collector/v1/file");
            
            // Fill in query params
            request.AddQueryByParams("apikey", mCmsModuleParam.mCMS_API_KEY);
            request.AddQueryByParams("auth", mCmsModuleParam.mCTSAuthCode);
            request.AddQueryByParams("sku", mCmsModuleParam.mCMS_SKU);
            request.AddQueryByParams("type", "name");
            request.AddQueryByParams("user", EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId.ToString());

            // Create metadata json object
            UploadMetadata data = new UploadMetadata();
            data.name = "FifaEnvTest";
            data.value = "TestRun";
            data.type = "string";
            data.searchable = true;

            UploadMetaDataList mdList = new UploadMetaDataList();
            mdList.metadata.Add(data);

            JavaScriptSerializer jsSerializer = new JavaScriptSerializer();
            string metaDataJson = jsSerializer.Serialize(mdList);

            StringContent metadataContent = new StringContent(metaDataJson, Encoding.UTF8, "application/json");
            metadataContent.Headers.ContentDisposition = new ContentDispositionHeaderValue("form-data");
            metadataContent.Headers.ContentDisposition.Name = "metadata";
            metadataContent.Headers.Add("charset", "ISO-8859-1");

            // Load file into local byte aray
            string sampleDataPath = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location) + @"\SampleData\fifa_online_vpro.bin";
            byte[] sampleByteArray = System.IO.File.ReadAllBytes(sampleDataPath);

            ByteArrayContent sampleDataContent = new ByteArrayContent(sampleByteArray);
            sampleDataContent.Headers.ContentDisposition = new ContentDispositionHeaderValue("form-data");
            sampleDataContent.Headers.ContentDisposition.Name = "fifa_online_vpro.bin";
            sampleDataContent.Headers.ContentDisposition.FileName = "fifa_online_vpro.bin";
            sampleDataContent.Headers.ContentType = new MediaTypeHeaderValue("text/plain");
            sampleDataContent.Headers.Add("charset", "ISO-8859-1");

            MultipartContent content = new MultipartContent("mixed", MULTIPART_BOUNDARY);
            content.Add(metadataContent);
            content.Add(sampleDataContent);
            request.AddContent(content);

            client.Execute(request, mReport);
        }

        private void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            string output = string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload);
            Console.WriteLine(output);
            mReport.AppendLine(output);

            if (e.StatusCode == HttpStatusCode.Created || e.StatusCode == HttpStatusCode.Accepted)
            {
                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is {1} EASW XMS HD module is currently down or incorrect API. Please copy the log here and contact CTS Ops and GameTeam Online SE for further investigation",
                    this.GetType().ToString(), mCmsModuleParam.mOSDK_EASW_AUTH_URL));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class CtsXmsHdUpdateRule : Rule
    {
        private CMSModuleParam mCmsModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam;
        private const string MULTIPART_BOUNDARY = "-31415926535897932384626";

        public CtsXmsHdUpdateRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mCmsModuleParam.mOSDK_EASW_AUTH_URL);
            client.RestClientRequestFinished += this.responseCallback;

            string blazeServerClientId = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mBlazeServerClientId;

            RestRequest request = new RestRequest(HttpMethod.Post, "xmshd_collector/v1/file/{filename}");
            request.FillInUrlSegment("filename", "fifa_online_vpro.bin");

            // Fill in query params
            request.AddQueryByParams("apikey", mCmsModuleParam.mCMS_API_KEY);
            request.AddQueryByParams("auth", mCmsModuleParam.mCTSAuthCode);
            request.AddQueryByParams("sku", mCmsModuleParam.mCMS_SKU);
            request.AddQueryByParams("type", "name");
            request.AddQueryByParams("user", EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId.ToString());

            // Create metadata json object
            UploadMetadata data = new UploadMetadata();
            data.name = "FifaEnvTest";
            data.value = "TestRun";
            data.type = "string";
            data.searchable = true;

            UploadMetaDataList mdList = new UploadMetaDataList();
            mdList.metadata.Add(data);

            JavaScriptSerializer jsSerializer = new JavaScriptSerializer();
            string metaDataJson = jsSerializer.Serialize(mdList);

            StringContent metadataContent = new StringContent(metaDataJson, Encoding.UTF8, "application/json");
            metadataContent.Headers.ContentDisposition = new ContentDispositionHeaderValue("form-data");
            metadataContent.Headers.ContentDisposition.Name = "metadata";
            metadataContent.Headers.Add("charset", "ISO-8859-1");

            // Load file into local byte aray
            string sampleDataPath = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location) + @"\SampleData\fifa_online_vpro.bin";
            byte[] sampleByteArray = System.IO.File.ReadAllBytes(sampleDataPath);

            ByteArrayContent sampleDataContent = new ByteArrayContent(sampleByteArray);
            sampleDataContent.Headers.ContentDisposition = new ContentDispositionHeaderValue("form-data");
            sampleDataContent.Headers.ContentDisposition.Name = "fifa_online_vpro.bin";
            sampleDataContent.Headers.ContentDisposition.FileName = "fifa_online_vpro.bin";
            sampleDataContent.Headers.ContentType = new MediaTypeHeaderValue("text/plain");
            sampleDataContent.Headers.Add("charset", "ISO-8859-1");

            MultipartContent content = new MultipartContent("mixed", MULTIPART_BOUNDARY);
            content.Add(metadataContent);
            content.Add(sampleDataContent);
            request.AddContent(content);

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
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is incorrect API or the content to be desired does not exist. Please copy the log here and contact CTS Ops and GameTeam Online SE for further investigation", this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }

    }

    public class CtsXmsHdGetFileInfoRule : Rule
    {
        private CMSModuleParam mCmsModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam;
        private const string MULTIPART_BOUNDARY = "-31415926535897932384626";

        public CtsXmsHdGetFileInfoRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mCmsModuleParam.mOSDK_EASW_AUTH_URL);
            client.RestClientRequestFinished += this.responseCallback;

            string blazeServerClientId = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mBlazeServerClientId;

            RestRequest request = new RestRequest(HttpMethod.Get, "xmshd_collector/v1/file/{filename}/status");
            request.FillInUrlSegment("filename", "fifa_online_vpro.bin");

            // Fill in query params
            request.AddQueryByParams("apikey", mCmsModuleParam.mCMS_API_KEY);
            request.AddQueryByParams("auth", mCmsModuleParam.mCTSAuthCode);
            request.AddQueryByParams("sku", mCmsModuleParam.mCMS_SKU);
            request.AddQueryByParams("type", "name");
            request.AddQueryByParams("user", EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId.ToString());

            client.Execute(request, mReport);
        }

        private void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            string output = string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload);
            Console.WriteLine(output);
            mReport.AppendLine(output);

            if (e.StatusCode == HttpStatusCode.OK || e.StatusCode == HttpStatusCode.NotFound)
            {
                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is incorrect API or the content to be desired does not exist. Please copy the log here and contact CTS Ops and GameTeam Online SE for further investigation", this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class CtsXmsHdSearchRule : Rule
    {
        private CMSModuleParam mCmsModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam;

        public CtsXmsHdSearchRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mCmsModuleParam.mOSDK_EASW_AUTH_URL);
            client.RestClientRequestFinished += this.responseCallback;

            string blazeServerClientId = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mBlazeServerClientId;

            RestRequest request = new RestRequest(HttpMethod.Post, "xmshd_collector/v1/search");

            // Fill in query params
            request.AddQueryByParams("apikey", mCmsModuleParam.mCMS_API_KEY);
            request.AddQueryByParams("auth", mCmsModuleParam.mCTSAuthCode);
            request.AddQueryByParams("sku", mCmsModuleParam.mCMS_SKU);
            request.AddQueryByParams("style", "summary");
            request.AddQueryByParams("pagesize", "20");
            request.AddQueryByParams("page", "1");
                 
            // Create search json object
            SearchRequestJson searchJson = new SearchRequestJson();
            searchJson.filename.Add("fifa_online_vpro.bin");
            searchJson.user.Add(EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId.ToString());

            JavaScriptSerializer jsSerializer = new JavaScriptSerializer();
            string searchJsonStr = jsSerializer.Serialize(searchJson);

            StringContent searchPayload = new StringContent(searchJsonStr, Encoding.UTF8, "text/json");
            request.AddContent(searchPayload);

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
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is incorrect API ", this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class CtsXmsHdDownloadRule : Rule
    {
        private CMSModuleParam mCmsModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam;
        
        public CtsXmsHdDownloadRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mCmsModuleParam.mOSDK_EASW_AUTH_URL);
            client.RestClientRequestFinished += this.responseCallback;

            string blazeServerClientId = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mBlazeServerClientId;

            RestRequest request = new RestRequest(HttpMethod.Get, "xmshd_collector/v1/file/{filename}");
            request.FillInUrlSegment("filename", "fifa_online_vpro.bin");

            // Fill in query params
            request.AddQueryByParams("apikey", mCmsModuleParam.mCMS_API_KEY);
            request.AddQueryByParams("auth", mCmsModuleParam.mCTSAuthCode);
            request.AddQueryByParams("sku", mCmsModuleParam.mCMS_SKU);
            request.AddQueryByParams("type", "name");
            request.AddQueryByParams("user", EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId.ToString());
                 
            // Create search json object
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
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is incorrect API or the content to be desired does not exist. Please copy the log here and contact CTS Ops and GameTeam Online SE for further investigation", this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class CtsXmsHdDeleteRule : Rule
    {
        private CMSModuleParam mCmsModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam;
        
        public CtsXmsHdDeleteRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mCmsModuleParam.mOSDK_EASW_AUTH_URL);
            client.RestClientRequestFinished += this.responseCallback;

            string blazeServerClientId = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mBlazeServerClientId;

            RestRequest request = new RestRequest(HttpMethod.Delete, "xmshd_collector/v1/file/{filename}");
            request.FillInUrlSegment("filename", "fifa_online_vpro.bin");

            // Fill in query params
            request.AddQueryByParams("apikey", mCmsModuleParam.mCMS_API_KEY);
            request.AddQueryByParams("auth", mCmsModuleParam.mCTSAuthCode);
            request.AddQueryByParams("sku", mCmsModuleParam.mCMS_SKU);
            request.AddQueryByParams("type", "name");
            request.AddQueryByParams("user", EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId.ToString());
                 
            // Create search json object
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
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is incorrect API or the content to be deleted does not exist. Please copy the log here and contact CTS Ops and GameTeam Online SE for further investigation", this.GetType().ToString()));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }
    #endregion // Xms HD Rules

    #region GameFace Rules
    public class CtsGetGameFaceRecipeRule : Rule
    {
        private CMSModuleParam mCmsModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam;
        
        public CtsGetGameFaceRecipeRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mCmsModuleParam.mOSDK_EASW_AUTH_URL);
            client.RestClientRequestFinished += this.responseCallback;

            string blazeServerClientId = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mNucleusParam.mBlazeServerClientId;

            RestRequest request = new RestRequest(HttpMethod.Get, "gameface/gameface_recipe");

            // Fill in query params
            request.AddQueryByParams("nucleus_id", EnvironmentMonitor.EnvironmentConfigParam.getInstance().mBlazeUserParam.mNucleusId.ToString());
            request.AddQueryByParams("sku_id", mCmsModuleParam.mCMS_SKU);

            client.Execute(request, mReport);
        }

        private void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            string output = string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload);
            Console.WriteLine(output);
            mReport.AppendLine(output);

            if (e.StatusCode == HttpStatusCode.OK || e.StatusCode == HttpStatusCode.Accepted)
            {
                
                XmlDocument doc = new XmlDocument();

                doc.LoadXml(e.Payload);
                XmlNode node = doc.DocumentElement.SelectSingleNode("/EASWAvatar/AvatarLikeness3D/RenderAppGameRuntime/Head/OutputBinaries/AssetBinary");

                if(node != null)
                {
                    if (!string.IsNullOrWhiteSpace(node.Attributes["location"].Value))
                    {
                        EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam.mGameFaceBinaryPath = node.Attributes["location"].Value;
                        mbSatisfied = true;
                    }
                }
                else
                {
                    mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Please check if the user has a GameFace uploaded on {1}. Please copy the log here and contact CTS Ops and GameTeam Online SE for further investigation", this.GetType().ToString(), mCmsModuleParam.mOSDK_EASW_AUTH_URL));
                }

            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Request to {1} has failed, based on the error type, this could mean connectivity issue, authentication problem, or mis-matching API. Please copy the log here and contact CTS and GameTeam Online SE who owns GameFace for futher investigation",
                    this.GetType().ToString(), mCmsModuleParam.mOSDK_EASW_AUTH_URL));
            }
            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class CtsGetGameFaceBinaryRule : Rule
    {
        private CMSModuleParam mCmsModuleParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mCMSParam;

        public CtsGetGameFaceBinaryRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            RestClient client = new RestClient(mCmsModuleParam.mOSDK_EASW_GF_FILE_URL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, mCmsModuleParam.mGameFaceBinaryPath);
            client.Execute(request, mReport);
        }

        private void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            string output = string.Format("Rest Client Finished! - {0}", e.StatusCode.ToString());
            Console.WriteLine(output);
            mReport.AppendLine(output);

            if (e.StatusCode == HttpStatusCode.OK || e.StatusCode == HttpStatusCode.Accepted)
            {
                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Please check if the user has a GameFace uploaded on {1}. Please copy the log here and contact CTS Ops and GameTeam Online SE for further investigation", this.GetType().ToString(), mCmsModuleParam.mOSDK_EASW_AUTH_URL));
            }

            uiContext.Post(onEvaluationComplete, this);
        }
    }
    #endregion // GameFace Rules

}

namespace XmsHdHelperClasses
{
    public class CreatedDate
    {
    }

    public class LastAccessed
    {
    }

    public class LastModified
    {
    }

    public class UploadMetaDataList
    {
        public List<UploadMetadata> metadata { get; set; }
        public UploadMetaDataList()
        {
            metadata = new List<UploadMetadata>();
        }
    }

    public class UploadMetadata
    {
        public UploadMetadata(string n, string t, string v)
        {
            name = n;
            type = t;
            value = v;

        }
        public UploadMetadata()
        {
            name = "";
            type = "";
            value = "";
            searchable = true;
        }
        public string name { get; set; }
        public string value { get; set; }
        public string type { get; set; }
        public bool searchable { get; set; }
    }

    public class SearchMetadata
    {
        public SearchMetadata(string n, string t, string v, string m)
        {
            name = n;
            type = t;
            value = v;
            match = m;

        }
        public SearchMetadata()
        {
            name = "";
            type = "";
            value = "";
            match = "";
        }
        public string name { get; set; }
        public string value { get; set; }
        public string type { get; set; }
        public string match { get; set; }
    }

    public class SearchRequestJson
    {
        public SearchRequestJson()
        {
            created_date = new CreatedDate();
            last_accessed = new LastAccessed();
            last_modified = new LastModified();
            metadata = new List<SearchMetadata>();
            sku = new List<string>();
            user = new List<string>();
            filename = new List<string>();
        }

        public CreatedDate created_date { get; set; }
        public LastAccessed last_accessed { get; set; }
        public LastModified last_modified { get; set; }
        public List<SearchMetadata> metadata { get; set; }
        public List<string> sku { get; set; }
        public List<string> user { get; set; }
        public List<string> filename { get; set; }
    }

    public class ResponseMetadata
    {
        public string name { get; set; }
        public string value { get; set; }
        public string type { get; set; }
        public bool searchable { get; set; }
    }

    public class File
    {
        public int id { get; set; }
        public string sku { get; set; }
        public string name { get; set; }
        public string user_id { get; set; }
        public string status { get; set; }
        public string link { get; set; }
        public string created_date { get; set; }
        public string modified_date { get; set; }
        public string accessed_date { get; set; }
        public int bytes_received { get; set; }
        public List<ResponseMetadata> metadata { get; set; }
        public string content_type { get; set; }
    }

    public class SearchResponseObject
    {
        public int count { get; set; }
        public List<File> files { get; set; }
    }
}
