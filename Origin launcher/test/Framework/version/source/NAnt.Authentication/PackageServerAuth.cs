


namespace NAnt.Authentication
{
	using System;
	using System.Collections.Generic;
	using System.Linq;
	using System.Text;
	using System.Configuration;
	using System.Threading.Tasks;
	using System.Net.Http;
	using System.Web;
    using Newtonsoft.Json;
    using System.IO;

	public class PackageServerAuth
    {

	    private static string AuthEndpoint = "https://packages.ea.com/api/v2/auth/";
	    private static string TestEndpoint = "https://packages.ea.com/api/v2/packages/";
		private static string RefreshedTokenHeader = "authorization";
		private static string ConfigOption = "UseV1";
		private static string TokenFormat = "Bearer ";
		/// <summary>
		/// Validates token and returns a refreshed token
		/// </summary>
		/// <param name="token"></param>
		/// <param name="endpoint"></param>
		/// <returns></returns>
		public async Task<string> ValidateToken(string token, string endpoint = "")
		{
			HttpClient m_httpClient = new HttpClient();
			if (string.IsNullOrEmpty(endpoint))
			{
				endpoint = TestEndpoint;
			};
			m_httpClient.DefaultRequestHeaders.Add("Authorization", $"Bearer {token}");
			UriBuilder builder = new UriBuilder(endpoint);
			var query = HttpUtility.ParseQueryString(builder.Query);
			builder.Query = query.ToString();
			HttpRequestMessage request = new HttpRequestMessage(HttpMethod.Get, builder.ToString());
			var response = await m_httpClient.SendAsync(request);
			if (response.IsSuccessStatusCode)
			{
				string newToken = response.Headers.GetValues(RefreshedTokenHeader).FirstOrDefault();
				if (!string.IsNullOrEmpty(newToken))
				{
					newToken = newToken.Replace(TokenFormat, "");
					return newToken;
				}
				// no refreshed token returned?
				return token;
			}
			return null;
		}


		public async Task<string> AuthenticateAsync(string bearer)
		{
			HttpClient m_httpClient = new HttpClient();
			m_httpClient.DefaultRequestHeaders.Add("Authorization", bearer);
			UriBuilder builder = new UriBuilder(AuthEndpoint);
			var query = HttpUtility.ParseQueryString(builder.Query);
			builder.Query = query.ToString();
			Stream receiveStream;
			StreamReader readStream;
			HttpRequestMessage request = new HttpRequestMessage(HttpMethod.Post, builder.ToString());
			var response = await m_httpClient.SendAsync(request);
			if (response.IsSuccessStatusCode)
			{
				receiveStream = response.Content.ReadAsStreamAsync().Result;
				using (receiveStream)
				{
					readStream = new StreamReader(receiveStream, Encoding.UTF8);
					using (readStream)
					{
						try
						{
							var responseText = readStream.ReadToEndAsync().Result;
							var unpacked = JsonConvert.DeserializeObject<PackageServerResponse>(responseText);
							return unpacked.Token;
						}
						catch (Exception e)
						{
							throw new Exception($"Error deserialising response from package server - {e}");
						}
					}
				}						
			}
			else if (response.StatusCode == System.Net.HttpStatusCode.Unauthorized)
			{
				throw new Exception($"Received response: Unauthorized from package server at {AuthEndpoint}, make sure username & password are correct");
			}
			else if (response.StatusCode == System.Net.HttpStatusCode.NotFound)
			{
				throw new Exception($"Received response: Not Found from package server at {AuthEndpoint}, make sure URL is correct.");
			}
			else
			{
				receiveStream = response.Content.ReadAsStreamAsync().Result;
				using (receiveStream)
				{
					readStream = new StreamReader(receiveStream, Encoding.UTF8);
					using (readStream)
					{						
						throw new Exception($"Received unexpected response from {AuthEndpoint} of {response.StatusCode} : {readStream.ReadToEndAsync().Result}. Please ensure sure email and password are correct");
					}
				}
			}			
		}

		public static bool UseV1PackageServer()
		{
			string option = ConfigurationManager.AppSettings[ConfigOption];
			bool useV1 = true;
			try
			{
				useV1 = bool.Parse(option);
			}
			catch
			{

			}
			return useV1;
		}
		private class PackageServerResponse
		{
			public string Email { get; set; }
			public string Name { get; set; }
			public string Token { get; set; }
		}
	}
}
