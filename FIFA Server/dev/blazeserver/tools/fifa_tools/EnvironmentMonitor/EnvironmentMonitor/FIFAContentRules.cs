using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Windows.Controls;

using System.Net;
using System.Xml;
using Rest;
using System.Net.Http;
using System.Windows.Shapes;
using System.Threading;
using System.ComponentModel;

namespace EnvironmentMonitor
{
    class RosterVersionData
    {
        public String mdbMajor;
        public String mdbMajorCRC;
        public String mdbMajorLoc;

        public String mdbMinor;
        public String mdbMinorCRC;
        public String mdbMinorLoc;

        public String mdbSchemaCRC;

        public String mdbFUTVer;
        public String mdbFUTCRC;
        public String mdbFUTLoc;
        public String mImagepathbase;
    }

    class FIFAContentModule : MonitorModule
    {
        private List<RosterVersionData> mRosterList = new List<RosterVersionData>();

        public FIFAContentModule(ModuleParam param, Ellipse statusControl, TextBox operationTextBox, String name) : base(param, statusControl, operationTextBox, name) { }

        public List<RosterVersionData> getRosterList() { return mRosterList; }

        public void resetRosterList() { mRosterList.Clear(); }
    }

    class RosterVersionRule : Rule
    {        
        public RosterVersionRule(MonitorModule module, RuleCallbackInterface callbackObj) : base(module, callbackObj) { }

        public void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            Console.WriteLine(string.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));
            mReport.AppendLine(String.Format("Rest Client Finished! - {0} - {1}", e.StatusCode.ToString(), e.Payload));

            if (e.StatusCode == HttpStatusCode.OK)
            {
                XmlDocument doc = new XmlDocument();

                doc.LoadXml(e.Payload);

                
                XmlNodeList nodelist = doc.DocumentElement.SelectNodes("squadInfoSet/squadInfo");
                
                BlazeModuleParam blazeParam = EnvironmentConfigParam.getInstance().mBlazeParam;              
                CMSModuleParam cmsParam = EnvironmentConfigParam.getInstance().mCMSParam;
                EASFCModuleParam easfcParam = EnvironmentConfigParam.getInstance().mEASFCParam;
                FUTModuleParam futParam = EnvironmentConfigParam.getInstance().mFUTParam;

                String platform = blazeParam.mPlatform;

                if (String.Compare(platform, "PC", true) == 0)
                {
                    platform = "pc64";
                }

                foreach (XmlNode node in nodelist)
                {
                    if(String.Compare(node.Attributes["platform"].Value,platform, true)==0)
                    {
                        RosterVersionData roster = new RosterVersionData();

                        roster.mdbMajor = (node.SelectSingleNode("dbMajor")).InnerText;
                        roster.mdbMinor = (node.SelectSingleNode("dbMinor")).InnerText;
                        roster.mdbMajorCRC = (node.SelectSingleNode("dbMajorCRC")).InnerText;
                        roster.mdbMinorCRC = (node.SelectSingleNode("dbMinorCRC")).InnerText;
                        roster.mdbMajorLoc = (node.SelectSingleNode("dbMajorLoc")).InnerText;
                        roster.mdbMinorLoc = (node.SelectSingleNode("dbMinorLoc")).InnerText;
                        roster.mdbSchemaCRC = (node.SelectSingleNode("dbSchemaCRC")).InnerText;
                        roster.mdbFUTVer = (node.SelectSingleNode("dbFUTVer")).InnerText;
                        roster.mdbFUTCRC = (node.SelectSingleNode("dbFUTCRC")).InnerText;
                        roster.mdbFUTLoc = (node.SelectSingleNode("dbFUTLoc")).InnerText;
                        roster.mImagepathbase = (node.SelectSingleNode("imagepathbase")).InnerText;

                        ((FIFAContentModule)mModule).getRosterList().Add(roster);

                        futParam.mFutDBCRC = roster.mdbFUTCRC;
                        futParam.mFutDBLoc = roster.mdbFUTLoc;
                        futParam.mFutDBVer = roster.mdbFUTVer;
                    }
                }                
             
                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is incorrect API or the roster update file does not exist at {1}fifa/fifalive/gen4title/rosterupdate.xml. Please copy the log here and contact CTS Ops, DC&L and GameTeam Online SE for further investigation", 
                    this.GetType().ToString(), EnvironmentConfigParam.getInstance().mCMSParam.mCONTENT_URL));
            }

            uiContext.Post(onEvaluationComplete, this);
        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            ((FIFAContentModule)mModule).getRosterList().Clear();

            string hostURL = EnvironmentConfigParam.getInstance().mCMSParam.mCONTENT_URL;
            RestClient client = new RestClient(hostURL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "fifa/fifalive/gen4title/rosterupdate.xml");

            client.Execute(request, mReport);
        }
    }

    class DownContentParam
    {
        public String mURL;
        public String mContentPath;
        public String mVersion;
    }

    class DownloadContentRule : Rule
    {
        private DownContentParam mParam;

        public DownloadContentRule(MonitorModule module, RuleCallbackInterface callbackObj, DownContentParam parm, String name) : base(module, callbackObj, name) 
        {
            mParam = parm;
            this.uiContext = ((MultipleRostersRule)callbackObj).getUiContext();
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

            mReport.AppendLine(String.Format("Performing - {0}", mRuleName));
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
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is roster content does not exist at {1}. Please copy the log here and contact CTS Ops, DC&L and GameTeam Online SE for further investigation",
                    this.GetType().ToString(), mParam.mURL+mParam.mContentPath));
            }

            uiContext.Post(onEvaluationComplete, this);
        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            if (mParam.mURL != null && mParam.mContentPath != null)
            {
                RestClient client = new RestClient(mParam.mURL);
                client.RestClientRequestFinished += this.responseCallback;

                RestRequest request = new RestRequest(HttpMethod.Get, mParam.mContentPath);

                Console.WriteLine("Downloading - {0}{1}", mParam.mURL, mParam.mContentPath);
                mReport.AppendLine(String.Format("Downloading - {0}{1}", mParam.mURL, mParam.mContentPath));

                client.Execute(request, mReport);
            }
        }
    }

    class MultipleRostersRule : Rule, RuleCallbackInterface
    {
        private List<DownloadContentRule> mDownloadContentRules;

        private int mDownloadContentRuleIndex;

        public SynchronizationContext getUiContext() { return uiContext; }

        public MultipleRostersRule(MonitorModule module, RuleCallbackInterface callbackObj) : base(module, callbackObj) 
        {
            mDownloadContentRules = new List<DownloadContentRule>();
        }

        public override void PerformEvaluation()
        {
            uiContext = SynchronizationContext.Current;

            mbSatisfied = false;
            mbGiveRecommendation = true;

            workerThread = new BackgroundWorker();
            workerThread.DoWork += new DoWorkEventHandler(evaluate);
            workerThread.RunWorkerAsync(uiContext);
        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            // MultipleRostersRule goes through the list of rosters, create a DownloadContent rule and download each roster one by one via the RosterContentRule

            FIFAContentModule fifaContentModule = (FIFAContentModule) mModule;
                       

            // for each roster version, download the FIFA Roster Major, FIFA Roster Minor, FUT DB
            foreach (RosterVersionData roster in fifaContentModule.getRosterList())
            {
                // Minor roster update rule
                DownContentParam paramMinor = new DownContentParam();
                paramMinor.mURL = EnvironmentConfigParam.getInstance().mCMSParam.mCONTENT_URL;
                paramMinor.mContentPath = roster.mdbMinorLoc;
                paramMinor.mVersion = roster.mdbMajor + "." + roster.mdbMinor;
                mDownloadContentRules.Add(new DownloadContentRule(mModule, this, paramMinor, "FIFAFixture"+paramMinor.mVersion));

                
                // Major roster update rule
                DownContentParam paramMajor = new DownContentParam();
                paramMajor.mURL = EnvironmentConfigParam.getInstance().mCMSParam.mCONTENT_URL;
                paramMajor.mContentPath = roster.mdbMajorLoc;
                paramMajor.mVersion = roster.mdbMajor + "." + roster.mdbMinor;
                mDownloadContentRules.Add(new DownloadContentRule(mModule, this, paramMajor,"FIFASquad"+paramMinor.mVersion));
                /*
                // FUT DB roster rule
                DownContentParam paramFUT = new DownContentParam();
                paramFUT.mURL = EnvironmentConfigParam.getInstance().mCMSParam.mCONTENT_URL;
                paramFUT.mContentPath = roster.mdbFUTLoc;
                paramFUT.mVersion = roster.mdbFUTVer;
                mDownloadContentRules.Add(new DownloadContentRule(mModule, this, paramFUT, "FUTDB" + roster.mdbFUTLoc));
                */
            }
            
            mDownloadContentRuleIndex = 0;

            mDownloadContentRules[mDownloadContentRuleIndex].PerformEvaluation();
        }

        public void HandleRuleEvaluationComplete(bool bCriticalError = false)
        {
            // increment mCurrentRosterIndex, if mCurrentRosterindex is less than total roster then download next content, if not flag our own callback listener

            mDownloadContentRuleIndex++;

            if (mDownloadContentRuleIndex < mDownloadContentRules.Count)
            {
                // start next content download
                mDownloadContentRules[mDownloadContentRuleIndex].PerformEvaluation();
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
}
