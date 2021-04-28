// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
//
//
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)
// Scott Hernandez (ScottHernandez@hotmail.com)

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Xml.Linq;

using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Authentication
{
	// credential store for all Framework's authentication needs, we're not putting any effort
	// towards being secure other than making sure the on disk representation is human 
	// unreadable and not leaking human readable passwords via error messages

	// threading: credential store instances have no internal thread safety however
	// the only public method of interaction is through static methods, these static methods
	// use a single global lock (we're assuming only one credential file is really ever 
	// going to be accessed per program instance) to stop multiple threads trying to 
	// modify this file 
	//
	// a global lock is also preferred as we can prompt the user for credentials
	// and we only want one prompt at a time

	public class CredentialStore
	{
		public const string FrameworkGenericNetLoginKey = "EAT_FRAMEWORK_LEGACY_NET_LOGIN"; // cred store key for legacy login methods
		public const string CredStoreFileName = "nant_credstore_keys.dat"; // default file name
  

        public const int DefaultPromptTimeoutSeconds = 60; // default timeout for prompt using for user to enter password, if user does not enter in this time, assume automated build
														   // and fall back to other methods

		public readonly static string UnixProfileFilePath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".eat_framework_net_profile");

		private readonly Dictionary<string, Credential> m_credentialMap = new Dictionary<string, Credential>();

		private readonly static object s_globalStoreLock = new object(); // lock for static methods

        private string m_encryptionKey = null;


        private CredentialStore()
		{
		}

		private CredentialStore(string credStoreText, string encryptionKey)
		{
			try
			{
				XDocument document = XDocument.Parse(credStoreText);
				foreach (XElement credential in document.Root.Elements("credential"))
				{
					string address = credential.Attribute("address").Value;
					string username = credential.Attribute("username").Value;
					string password = credential.Attribute("password").Value;

					m_credentialMap[address] = new Credential(username, password);
				}
			}
			catch // swallow all exceptions, don't want any exceptions leaking file contents
			{
				throw new FormatException("Credential store corrupted. Please repopulate credential store.");
			}

            m_encryptionKey = encryptionKey;
        }

        public static void Store(Log log, string server, Credential credential, string filename = null)
		{
			lock (s_globalStoreLock)
			{
				// load, update, save
				CredentialStore credStore = LoadDecrypt(log, filename);
				credStore.Add(server, credential);
				credStore.SaveEncrypt(filename);
			}
		}

		public static Credential Retrieve(Log log, string server, string filename = null)
		{
			lock (s_globalStoreLock)
			{
				CredentialStore credStore = LoadDecrypt(log, filename);
				return credStore.Get(server);
			}
		}

		public static Credential Prompt(Log log, string server, int timeoutSeconds = DefaultPromptTimeoutSeconds, string filename = null)
		{
			lock (s_globalStoreLock)
			{
				Credential credential = GetCredentialFromUserPrompt(server, timeoutSeconds);
				if (credential != null)
				{
					// if user gave valid credential, update store on disk
					CredentialStore credStore = LoadDecrypt(log, filename);
					credStore.Add(server, credential);
					credStore.SaveEncrypt();
				}
				return credential;
			}
		}

		public static bool Invalidate(Log log, string server, string fileName = null)
		{
			lock (s_globalStoreLock)
			{
				CredentialStore credStore = LoadDecrypt(log, fileName);
				if (credStore.Remove(server))
				{
					credStore.SaveEncrypt(fileName);
					return true;
				}
				return false;
			}
		}

		public static void Clear(string filename = null)
		{
			lock (s_globalStoreLock)
			{
				CredentialStore credentialStore = new CredentialStore();
				credentialStore.SaveEncrypt(filename);
			}
		}

		public static string GetDefaultCredStoreFilePath()
		{
			return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile, Environment.SpecialFolderOption.Create), CredStoreFileName);
		}

		public void Add(string server, Credential credential)
		{
			if (server == null)
			{
				throw new ArgumentNullException("server");
			}
			if (credential == null || credential.Name == null || credential.Password == null)
			{
				throw new ArgumentNullException("credential");
			}
			m_credentialMap[server] = credential;
		}

		private Credential Get(string server)
		{
			Credential credential = null;
			m_credentialMap.TryGetValue(server, out credential);
			return credential;
		}

		private bool Remove(string server)
		{
			return m_credentialMap.Remove(server);
		}

		public void SaveEncrypt(string fileName = null)
		{
			fileName = fileName ?? GetDefaultCredStoreFilePath();
			string plainText = ToXDoc().ToString();

            if (string.IsNullOrEmpty(m_encryptionKey))
            {
                m_encryptionKey = AesEncryptionUtil.GetRandomKey(64);
            }

            byte[] encryptedBytes = AesEncryptionUtil.EncryptBuffer(Encoding.Unicode.GetBytes(plainText), m_encryptionKey);
            byte[] encryptedKey = DataProtection.EncryptBuffer(Encoding.Unicode.GetBytes(m_encryptionKey));

			string dirPath = Path.GetDirectoryName(fileName);
			if (!string.IsNullOrWhiteSpace(dirPath) && !Directory.Exists(dirPath))
			{
				Directory.CreateDirectory(dirPath);
			}

            XDocument xDoc = new XDocument(
                new XElement("credentialStore",
                    new XElement("key", Convert.ToBase64String(encryptedKey)),
                    new XElement("credentials", Convert.ToBase64String(encryptedBytes))
                    )
                );
            File.WriteAllText(fileName, xDoc.ToString());
        }

		private XDocument ToXDoc()
		{
			XDocument xDoc = new XDocument();
			xDoc.Add(new XElement("credentials")); // document root
			foreach (KeyValuePair<string, Credential> serverCredPair in m_credentialMap.OrderBy(serverCredPair => serverCredPair.Key))
			{
				xDoc.Root.Add
				(
					new XElement
					(
						"credential",
						new XAttribute("address", serverCredPair.Key),
						new XAttribute("username", serverCredPair.Value.Name),
						new XAttribute("password", serverCredPair.Value.Password)
					)
				);
			}
			return xDoc;
		}

        public static CredentialStore LoadDecrypt(Log log, string fileName = null)
		{
			CredentialStore credStore = null;
			fileName = fileName ?? GetDefaultCredStoreFilePath();
			try
			{
				if (File.Exists(fileName))
				{
					string xDocString = File.ReadAllText(fileName);
                    XDocument xDoc = XDocument.Parse(xDocString);

                    byte[] encryptedKey = Convert.FromBase64String(xDoc.Root.Element("key").Value);
                    byte[] encryptedCredentials = Convert.FromBase64String(xDoc.Root.Element("credentials").Value);

                    string key = Encoding.Unicode.GetString(DataProtection.DecryptBuffer(encryptedKey));

                    var decryptedBuffer = AesEncryptionUtil.DecryptBuffer(encryptedCredentials, key);
                    if (decryptedBuffer != null)
                    {
                        string plainText = Encoding.Unicode.GetString(decryptedBuffer);
                        credStore = new CredentialStore(plainText, key);
                    }
				}
			}
			catch (Exception ex)
			{
				// throw new exception that contains file path in error message
				credStore = null;
                Console.WriteLine(String.Format("Error reading credential store file at '{0}'. {1} File will be regenerated when credentials get added.", fileName, ex.Message));
			}

			// if credstore was unset by above code, create new empty object
			credStore = credStore ?? new CredentialStore();

			return credStore;
		}

		/// <summary>
		/// Prompts user for username and password. On Windows it displays a modal dialog box,
		/// and on Unix it will display a command line prompt.
		/// </summary>
		/// <param name="server">The server name to mention in the prompt so the user knows which credentials to provide</param>
		/// <param name="timeoutSeconds">The timeout length used to skip the prompt if the build is running under automation</param>
		/// <returns>A credential object with a username and password or null if no credentials are provided</returns>
		private static Credential GetCredentialFromUserPrompt(string server, int timeoutSeconds, bool newPackageServer = false)
		{
			if (PlatformUtil.IsWindows)
			{
				// we don't want to prompt if we are not under user interactive mode or else it will fail
				if (!Environment.UserInteractive)
				{
					Console.Write("It appears that you are running in an automated environment and we were not able to find valid credentials for server '{0}'. ", server);
					Console.Write("You can setup perforce credentials on this machine using the eapm tool found in the Framework bin directory. ");
					Console.WriteLine("Use the command \"eapm.exe credstore {0}\" and you will then be prompted to enter credentials. ", server);
					return null;
				}

				Console.WriteLine("Prompting user for credentials...");

				string program = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "CredentialManager.exe");
				if (!File.Exists(program))
				{
					// Second try relative to our unit test executable
					program = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "..", "..", "..", "bin",  "CredentialManager.exe");
				}

				ProcessStartInfo startInfo = new ProcessStartInfo(program, server + " " + timeoutSeconds);
				startInfo.CreateNoWindow = true;
				startInfo.UseShellExecute = false;

				using (Process p = new Process())
				{
					p.StartInfo = startInfo;

					p.Start();

					// wait a little longer than the timeout given to the application to take into acount start up and shutdown time
					int processTimeout = 5000 + timeoutSeconds * 1000;
					if (!p.WaitForExit(processTimeout))
					{
						Console.WriteLine("Timeout value of {0}ms exceeded, killing process.", processTimeout);
						p.Kill();
					}
				}

				// The subprocess should have written to the credential store
				// so try to load the value they would have entered
				Credential credential = null;
				CredentialStore credStore = LoadDecrypt(null);
				if (credStore != null)
				{
					credential = credStore.Get(server);
				}
				return credential;
			}
			else
			{
				Console.WriteLine("Could not determine credentials automatically");
				string messageTemplate = "(press any key to enter credentials manually, {0} seconds remaining)";
				int time = timeoutSeconds;

				// display a timer to give user a chance to enter the credentials so that it skips this step in automated systems
				while (!Console.KeyAvailable && time > 0)
				{
					string message = String.Format(messageTemplate, time);
					Console.Write(message);
					time--;

					Thread.Sleep(1000);

					for (int i = 0; i < message.Length; ++i)
					{
						Console.Write("\b");
					}
				}
				if (time == 0)
				{
					return null;
				}
				// captures the key pressed to end the timer and removes it from the buffer so that it doesn't get added to the username
				Console.ReadKey(true);

				Console.WriteLine();
				Console.WriteLine("Server: " + server);
				Console.Write("Enter Username: ");
				string username = Console.ReadLine();

				// Prompt user to enter password, and obscure the password with stars as they type
				Console.Write("Enter Password: ");
				string password = "";
				ConsoleKeyInfo keyInfo = new ConsoleKeyInfo();
				while (keyInfo.Key != ConsoleKey.Enter)
				{
					keyInfo = Console.ReadKey(true);
					if (keyInfo.Key == ConsoleKey.Backspace && password.Length > 0)
					{
						password = password.Substring(0, (password.Length - 1));
						Console.Write("\b \b");
					}
					else if (keyInfo.Key != ConsoleKey.Backspace && keyInfo.Key != ConsoleKey.Enter)
					{
						password += keyInfo.KeyChar;
						Console.Write("*");
					}
				}
				Console.WriteLine();
				if (newPackageServer)
				{
					PackageServerAuth auth = new PackageServerAuth();

				}
				return new Credential(username, password);
			}
		}
	}
}
