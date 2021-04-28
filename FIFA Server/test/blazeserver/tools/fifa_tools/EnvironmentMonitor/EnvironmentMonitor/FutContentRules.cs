using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using Rest;

namespace EnvironmentMonitor
{
    public class FutDownloadDreamSquadJsonRule : Rule
    {
        private FUTModuleParam mFUTParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mFUTParam;

        public FutDownloadDreamSquadJsonRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            string hostURL = EnvironmentConfigParam.getInstance().mCMSParam.mCONTENT_URL;
            RestClient client = new RestClient(hostURL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, "fut/packs/dreamsquad/dreamsquadpacklist.json");
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
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is FUT Dream Squad content does not exist at {1}fut/packs/dreamsquad/dreamsquadpacklist.json. Please copy the log here and contact GameTeam FUT SE for further investigation",
                    this.GetType().ToString(), EnvironmentConfigParam.getInstance().mCMSParam.mCONTENT_URL));
            }
            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class FutDownloadSquadsRule : Rule
    {
        private FUTModuleParam mFUTParam = EnvironmentMonitor.EnvironmentConfigParam.getInstance().mFUTParam;

        public FutDownloadSquadsRule(MonitorModule module, RuleCallbackInterface callback)
            : base(module, callback)
        {

        }

        protected override void evaluate(object sender, DoWorkEventArgs e)
        {
            string hostURL = EnvironmentConfigParam.getInstance().mCMSParam.mCONTENT_URL;
            RestClient client = new RestClient(hostURL);
            client.RestClientRequestFinished += this.responseCallback;

            RestRequest request = new RestRequest(HttpMethod.Get, mFUTParam.mFutDBLoc);
            client.Execute(request, mReport);
        }

        private void responseCallback(object sender, RestClientFinishedEventArgs e)
        {
            string output = string.Format("Rest Client Finished! - {0}", e.StatusCode.ToString());
            Console.WriteLine(output);
            mReport.AppendLine(output);

            if (e.StatusCode == HttpStatusCode.OK)
            {
                mbSatisfied = true;
            }
            else
            {
                mRecommend.AppendLine(String.Format("\n ***** [{0}] Failed - Possible cause is FUT DB content does not exist at {1}. Please copy the log here and contact GameTeam FUT SE for further investigation",
                    this.GetType().ToString(), EnvironmentConfigParam.getInstance().mCMSParam.mCONTENT_URL + mFUTParam.mFutDBLoc));
            }
            uiContext.Post(onEvaluationComplete, this);
        }
    }

}
