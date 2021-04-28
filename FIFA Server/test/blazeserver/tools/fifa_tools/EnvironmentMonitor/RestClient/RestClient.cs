using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net.Http;
using System.Net;
using System.Net.Http.Headers;
using System.Security.Cryptography.X509Certificates;

namespace Rest
{
    public class RestClient : HttpClient
    {
        private Random mRandom = new Random();
        private string mBaseUrl = string.Empty;

        public RestClient(string baseUrl) : base()
        {
            ServicePointManager.ServerCertificateValidationCallback += (sender, cert, chain, sslPolicyErrors) => true;
            mBaseUrl = baseUrl;
        }

        public RestClient(string baseUrl, WebRequestHandler certHandler)
            : base(certHandler)
        {
            ServicePointManager.ServerCertificateValidationCallback += (sender, cert, chain, sslPolicyErrors) => true;
            mBaseUrl = baseUrl;
        }

        public event EventHandler<RestClientFinishedEventArgs> RestClientRequestFinished;

        public async void Execute(RestRequest request, StringBuilder report = null)
        {
            try
            {
                request.AddBaseUrl(mBaseUrl);
                HttpRequestMessage req = request.GetHttpRequest();

                Console.WriteLine(string.Format("[RestClient] - {0}", req.ToString()));

                if (report != null)
                {
                    report.AppendLine(string.Format("[RestClient] - {0}", req.ToString()));
                }

                HttpResponseMessage message = await SendAsync(req);
                string responseBody = await message.Content.ReadAsStringAsync();

                RestClientFinishedEventArgs args = new RestClientFinishedEventArgs(message.StatusCode, message.ReasonPhrase, responseBody, message);                
                RestClientRequestFinished(this, args);
            }
            catch (HttpRequestException ex)
            {
            	RestClientFinishedEventArgs args = new RestClientFinishedEventArgs(HttpStatusCode.InternalServerError, ex.Message, string.Empty, null);
                RestClientRequestFinished(this, args);
            }
        }

    }

    public class RestClientFinishedEventArgs : EventArgs
    {
        private HttpStatusCode mStatusCode;
        private string mResultMessage;
        private string mPayload;
        private HttpResponseMessage mResponse;

        public HttpStatusCode StatusCode
        {
            get { return mStatusCode; }
        }

        public string ResponseMessage
        {
            get { return mResultMessage; }
        }

        public HttpResponseMessage HttpResponse
        {
            get { return mResponse; }
        }

        public string Payload
        {
            get { return mPayload; }
        }

        public RestClientFinishedEventArgs(HttpStatusCode statusCode, string responseMessage, string payload, HttpResponseMessage response)
            : base()
        {
            mStatusCode = statusCode;
            mResultMessage = responseMessage;
            mPayload = payload;
            mResponse = response;
        }
    }
}
