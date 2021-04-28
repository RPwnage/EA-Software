using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Net.Http;
using System.Web;
using System.Collections.Specialized;
using System.Net.Http.Headers;

namespace Rest
{
    public class RestRequest
    {
        private NameValueCollection mQueries = new NameValueCollection();
        private Dictionary<string, string> mRequestHeaders = new Dictionary<string, string>();

        private string mAbsoluteQuery = string.Empty;
        private string mRelativePath = string.Empty;
        private string mBaseUrl = string.Empty;
        private string mFullRequestUrl = string.Empty;

        private HttpMethod mMethod = null;
        private HttpContent mContent = null;
                
        public RestRequest()
        {
            mMethod = HttpMethod.Get;
        }

        public RestRequest(HttpMethod method, Uri path)
        {
            mMethod = method;
            if (path.IsAbsoluteUri)
            {
                mFullRequestUrl = path.AbsoluteUri;
            }
            else
            {
                mRelativePath = path.AbsolutePath;
            }
        }

        public RestRequest(HttpMethod method, string path)
        {
            mMethod = method;
            

            if (Uri.IsWellFormedUriString(path, UriKind.Absolute))
            {
                mFullRequestUrl = path;
            }
            else
            {
                mRelativePath = path;
            }
        }

        /// <summary>
        /// Sets the HTTP Method.
        /// </summary>
        /// <param name="method"></param>
        public void SetMethod(HttpMethod method)
        {
            mMethod = method;
        }

        /// <summary>
        /// Sets the absolute uri.  This assumes that the passed in uri defines all query parameters and is a fully well formed url.
        /// This will override the AddBaseUrl, AddQueryParams, FillInUrlSegment fields
        /// </summary>
        /// <param name="uri"></param>
        public void SetAbsoluteRequestUri(Uri uri)
        {
            mFullRequestUrl = uri.AbsoluteUri;
        }

        /// <summary>
        /// Sets the absolute uri.  This assumes that the passed in uri defines all query parameters and is a fully well formed url.
        /// This will override the AddBaseUrl, AddQueryParams, FillInUrlSegment fields
        /// </summary>
        /// <param name="uri"></param>
        public void SetAbsoluteRequestUri(string uri)
        {
            mFullRequestUrl = uri;
        }


        /// <summary>
        /// Adds custom request headers to the request
        /// </summary>
        /// <param name="name">Name of the header field</param>
        /// <param name="value">Value of the header field</param>
        public void AddCustomRequestHeaders(string name, string value)
        {
            mRequestHeaders.Add(name, value);
        }

        /// <summary>
        /// Adds the base url to the existing request.
        /// Only used when an absolute base url hasn't been defined.
        /// </summary>
        /// <param name="url"></param>
        public void AddBaseUrl(string url)
        {
            mBaseUrl = url;
        }


        /// <summary>
        /// Adds query parameters to the uri.
        /// </summary>
        /// <param name="name"></param>
        /// <param name="value"></param>
        public void AddQueryByParams(string name, string value)
        {
            mAbsoluteQuery = string.Empty;
            mQueries.Add(name, value);
        }

        /// <summary>
        /// Adds query parameters to the uri.
        /// </summary>
        /// <param name="name"></param>
        /// <param name="value"></param>
        public void AddQueryByString(string queryString)
        {
            mQueries.Clear();
            mAbsoluteQuery = queryString;
        }

        /// <summary>
        /// If tokens are present in the local url path, this method will search for the string and replace it with the passed in value.
        /// </summary>
        /// <param name="name">name of token to replace</param>
        /// <param name="value">the updated string</param>
        public void FillInUrlSegment(string name, string value)
        {
            mRelativePath = mRelativePath.Replace(string.Format("{{{0}}}", name), value);
        }

        /// <summary>
        /// Adds a content object to the request.
        /// </summary>
        /// <param name="content"></param>
        public void AddContent(HttpContent content)
        {
            mContent = content;
        }

        public void AddDefaultStringContent(string data)
        {
            mContent = new StringContent(data, Encoding.UTF8, "text/plain");
        }
        
        private string ToQueryString(NameValueCollection nvc)
        {
            string[] array = (from key in nvc.AllKeys
                         from value in nvc.GetValues(key)
                         select string.Format("{0}={1}", key, value))
                .ToArray();
            return string.Join("&", array);
        }
        
        public HttpRequestMessage GetHttpRequest()
        {
            // Build Uri
            Uri uri = null;
            if (string.IsNullOrWhiteSpace(mFullRequestUrl))
            {
                Uri baseUrl = new Uri(mBaseUrl);
                uri = new Uri(baseUrl, mRelativePath);
                UriBuilder builder = new UriBuilder(uri);
                builder.Query = ToQueryString(mQueries);
                uri = builder.Uri;
            }
            else
            {
                uri = new Uri(mFullRequestUrl);
            }

            // Build Request Message
            HttpRequestMessage requestMessage = new HttpRequestMessage(mMethod, uri);
            foreach (KeyValuePair<string, string> header in mRequestHeaders)
            {
                requestMessage.Headers.Add(header.Key, header.Value);
            }

            requestMessage.Content = mContent;
            return requestMessage;

        }

    }
}
