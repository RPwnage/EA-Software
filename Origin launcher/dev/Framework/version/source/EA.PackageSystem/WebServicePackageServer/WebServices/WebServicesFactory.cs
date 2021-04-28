// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Net;
using System.Net.Http;
using NAnt.Authentication;
using NAnt.Core.Logging;

using PackageServer = EA.PackageSystem.PackageCore.Services;

namespace EA.PackageSystem.PackageCore.Services
{
	public static class WebServicesFactory
	{
		private static ICredentials m_credentials = null;
		private static int DefaultTimeoutSec = 30;
		private static bool TokenValidated = false;

		public static IWebServices Generate(Log log, string credentialStoreFile, bool useV1 = false, string token = null)
		{
			if (useV1 == false)
			{
				return GenerateNew(log, credentialStoreFile, token);
			}
			else
			{
#pragma warning disable CS0618 // Type or member is obsolete
				return GenerateOld(log, credentialStoreFile);
#pragma warning restore CS0618 // Type or member is obsolete
			}
		}

		public static IWebServices Generate(Log log, string credentialStoreFile)
		{
#pragma warning disable CS0618 // Type or member is obsolete
			return GenerateOld(log, credentialStoreFile);
#pragma warning restore CS0618 // Type or member is obsolete
		}

		public static Credential GetCredentialsForServer(string server, string email, string password, PackageServerAuth ps)
		{
			if (string.IsNullOrEmpty(email) || string.IsNullOrEmpty(password))
			{
				Console.WriteLine("Enter credential for {0}:", server);
			}
			if (string.IsNullOrEmpty(email))
			{
				email = GetEmailInteractively();
			}
			if (string.IsNullOrEmpty(password))
			{
				password = GetPasswordInteractively();
			}
			var credentials = Convert.ToBase64String(System.Text.Encoding.UTF8.GetBytes($"{email}:{password}"));
			// lets replace the stored password with the receieved token
			password = ps.AuthenticateAsync($"Basic {credentials}").Result;
			Credential cred = new Credential(email, password);
			return cred;
		}

		public static Credential PromptForCredentialsWithForm(Log log, string server)
		{
			Credential userInputCredential = CredentialStore.Prompt(log, server);
			return userInputCredential;
		}

		internal static Credential GetGenericCredential(Log log, string credentialStoreFile)
		{
			Credential genericCredential = CredentialStore.Retrieve(log, CredentialStore.FrameworkGenericNetLoginKey, credentialStoreFile);
			return genericCredential;
		}


		[ObsoleteAttribute("This Method uses a version of the API that is not secure and is soon to be deprecated, please start to use V2 - message the package-server slack channel for more information")]
		private static IWebServices GenerateOld(Log log, string credentialStoreFile)
		{
#if NETFRAMEWORK
			var serv = new WebServices();
			serv.Proxy = new WebProxy();
			var genericCredential = GetGenericCredential(log, credentialStoreFile);
			serv.Credentials = GetCredentials(log, genericCredential);
			serv.Url = PackageServer.WebServicesURL.GetWebServicesURL(true);
#else
			var serv = new WebServices(PackageServer.WebServicesURL.GetWebServicesURL(false));
			// These creds imo aren't needed / never work / are untestable
			var genericCredential = GetGenericCredential(log, credentialStoreFile);
			serv.Credentials.Windows.ClientCredential = GetCredentials(log, genericCredential) as NetworkCredential;
#endif
			return serv;
		}

		private static IWebServices GenerateNew(Log log, string credentialStoreFile, string packageServerToken = "")
		{
			HttpClient client = new HttpClient();
			var url = WebServicesURL.GetWebServicesURL(false);
			var configTimeout = System.Configuration.ConfigurationManager.AppSettings["WebRequestTimeoutMs"];
			TimeSpan timeout = TimeSpan.FromSeconds(DefaultTimeoutSec);
			if (configTimeout != null)
			{
				try
				{
					int result = Int32.Parse(configTimeout);
					timeout = TimeSpan.FromSeconds(result);
				}
				catch
				{
					log.Warning.WriteLine(
						$"value WebRequestTimeoutMs could not be read using default of {DefaultTimeoutSec}");
					// do nothing
				}
			}

			PackageServerRestV2Api api = String.IsNullOrEmpty(url) ? new PackageServerRestV2Api() : new PackageServerRestV2Api(url);
			PackageServerAuth ps = new PackageServerAuth();
			if (String.IsNullOrEmpty(credentialStoreFile))
			{
				credentialStoreFile = CredentialStore.GetDefaultCredStoreFilePath();
			}

			Credential credential;
			if (String.IsNullOrEmpty(packageServerToken))
			{
				// get token
				credential = RetrieveCredentialsFromStore(credentialStoreFile, url, log);
				if (credential == null)
				{
					credential = PromptForCredential(log, url);
					StoreCredential(log, url, credential, credentialStoreFile);
				}
				if (!TokenValidated)
				{
					var newToken = ps.ValidateToken(credential.Password).Result;
					TokenValidated = true;
					if (String.IsNullOrEmpty(newToken))
					{
						log.Warning.WriteLine(
							$"Token for package server {url} may be invalid - please enter credentials again");
						// token may be out of date, prompt for a new one
						credential = PromptForCredential(log, url);	
						StoreCredential(log, url, credential, credentialStoreFile);
					}
					else
					{					
						var newCred = new Credential(credential.Name, newToken);
						StoreCredential(log, url, newCred, credentialStoreFile);
					}
				}
			}
			else
			{
				string auth = null;
				if (!TokenValidated)
				{
					TokenValidated = true;
					auth = ps.ValidateToken(packageServerToken).Result;
				}
				if (String.IsNullOrEmpty(auth))
				{
					credential = PromptForCredential(log, url);
				}
				else
				{
					credential = new Credential("", packageServerToken);
				}				
			}

			// add the token
			client.DefaultRequestHeaders.Add("Authorization", $"Bearer {credential.Password}");

			// create client for future requests
			var services = new WebServicesHttps(client, timeout, api);
			return services;
		}

		private static Credential RetrieveCredentialsFromStore(string credStoreFile, string server, Log log)
		{
			return CredentialStore.Retrieve(log, server, credStoreFile);
		}

		private static void StoreCredential(Log log, string server, Credential credential, string credStoreFile)
		{
			CredentialStore.Store(log, server, credential, credStoreFile);
		}

		private static Credential PromptForCredential(Log log, string url)
		{
			var credential = PromptForCredentialsWithForm(log, url);
			if (credential == null)
			{
				throw new Exception($"Credentials are incorrect for package server at {url}. To rectify this either" +
				$"\ncall  \"eapm credstore -use-package-server\" within fblcli or " +
				"\npass a correct token to nant using -packageservertoken:<yourtoken}");
			}
			return credential;
		}

		private static string GetEmailInteractively()
		{
			Console.Write("\tEmail: ");
			return Console.ReadLine();
		}

		private static string GetPasswordInteractively()
		{
			Console.Write("\tPassword: ");

			string password = "";
			ConsoleKeyInfo keyinfo = new ConsoleKeyInfo();
			while (keyinfo.Key != ConsoleKey.Enter)
			{
				keyinfo = Console.ReadKey(true);
				if (keyinfo.Key == ConsoleKey.Backspace && password.Length > 0)
				{
					password = password.Substring(0, (password.Length - 1));
					Console.Write("\b \b");
				}
				else if (keyinfo.Key != ConsoleKey.Backspace && keyinfo.Key != ConsoleKey.Enter)
				{
					password += keyinfo.KeyChar;
					Console.Write("*");
				}
			}
			Console.WriteLine();

			return password;
		}


		internal static ICredentials GetCredentials(Log log, Credential genericCredential)
		{
			if (m_credentials != null)
			{
				return m_credentials;
			}

			// default windows credentials
			m_credentials = CredentialCache.DefaultCredentials;


			if (genericCredential != null)
			{
				string user = null;
				string domain = null;
				AuthenticationUtil.SplitDomainAndUser(genericCredential.Name, out user, out domain);
				m_credentials = new NetworkCredential(user, genericCredential.Password, domain);
				return m_credentials;
			}

			return m_credentials;
		}

	}
}
