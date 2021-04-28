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

using System.IO;
using System.Linq;
using System.Xml.Linq;
using System;

using NAnt.Authentication;
using NAnt.Core.Logging;
using System.Collections.Generic;
using EA.PackageSystem.PackageCore.Services;

namespace EA.PackageSystem.ConsolePackageManager
{
	[Command("credstore")]
	class CredStoreCommand : Command
	{
		internal override string Summary
		{
			get { return "Stores credentials in encrypted file for use by Framework's ondemand package functions."; }
		}

		internal override string Usage
		{
			get { return "[-credentialfile:<path>] [-user:<username>] [-password:<password>] [-output-to-file:<filepath>] [-packageserver-token:<token>] <server|masterconfig> [-use-package-server] [-output-to-console]"; }
		}

		internal override string Remarks
		{
			get
			{
				return @"Description
	Stores the user's package server credentials in encrypted credential store file.

Options
    server|masterconfig                     Provide the server or masterconfig to store the credentials for.
    -credentialfile:<credential_file_path>  Optional. Packages installed from p4 uri protocol or web package 
                                            server may require credentials. If a credential file is used to 
                                            authenticate against these services then this parameter can be 
                                            used to specify the location of the credential file (if it is 
                                            not in default location).
	
    -user:<username>                        Optional. Provide a username to log into the server or full all of the masterconfig
    -password:<password>                    Optional. Provide a password to log into the server or full all of the masterconfig
	-packageserver-token:<password>			Store the provided token to the credstore (for package server only)
	-output-to-file:<filepath>				Optional. If using package server for credentials, output the token to a file. (requires -use-package-server)
	-use-package-server						Optional - use package server for credentials - default is false, if set the 'server' will use the version from the PackageServerURLv2 config value.
	-output-to-console						Optional. If using package server for credentials, output the token to the console rather than writing to the credstore (requires -use-package-server)

Examples
	Prompts the user to enter their username and password (or ticket) for 
	a particular perforce server. When typing the password the text is 
	hidden. These credentials are stored in the default credential store
	file, located in the current users home directory in a file called
	nant_credstore.dat (%USERPROFILE%\nant_credstore_keys.dat on Windows,
	~/nant_credstore_keys.dat on MacOS or Linux)
	> eapm credstore my.perforce.server:1999

	Used for Package Server Authentication, -use-package-server, user is prompted to put in their email and password,
	auth token will be received from the package server and stored in the credstore file (either where specified or at default user location C:\Users\<user>\nant_credstore_keys.dat)
	Token can be output to file or console out for use for generating tokens for automation also. 
	> eapm credstore packages.ea.com -use-package-server -output-to-console
	
	Stores credentials in a user specified store file. Will prompt user
	to enter credentials for all servers referenced in a masterconfig
	that does not currently have an entry in credential store.
	> eapm credstore -credentialfile:C:\temp\credstore.dat masterconfig.xml";
			}
		}

		private static string[] s_protocolsRequiringAuthentication = new string[] { "p4" };

		internal override void Execute()
		{
			Log log = Log.Default;
			IEnumerable<string> positionalArgs = Arguments.Where(arg => !arg.StartsWith("-"));
			IEnumerable<string> optionalArgs = Arguments.Where(arg => arg.StartsWith("-"));		


			string credStoreFile = null;
			string username = null;
			string password = null;
			string outputFileLocation = null;
			string tokenToStore = null;
			bool outputTokenToConsole = false;
			bool usePackageServer = false;
			foreach (string arg in optionalArgs)
			{


				if (arg.StartsWith("-credentialfile:"))
				{
					credStoreFile = arg.Substring("-credentialfile:".Length);
				}
				else if (arg.StartsWith("-user:"))
				{
					username = arg.Substring("-user:".Length);
				}
				else if (arg.StartsWith("-password:"))
				{
					password = arg.Substring("-password:".Length);
				}
				else if (arg.StartsWith("-output-to-file:"))
				{
					outputFileLocation = arg.Substring("-output-to-file:".Length);
				}
				else if (arg.Contains("-use-package-server"))
				{
					usePackageServer = true;
				}
				else if (arg.Equals("-output-to-console"))
				{
					outputTokenToConsole = true;
				}
				else if (arg.StartsWith("-store-packageserver-token:"))
				{
					tokenToStore = arg.Substring("-store-packageserver-token:".Length);
					usePackageServer = true;
				}
			}

			if (positionalArgs.Count() != 1)
			{
				if (!usePackageServer)
				{
					throw new InvalidCommandArgumentsException(this);
				}
			}

			string credTarget = usePackageServer ? WebServicesURL.GetWebServicesURL(false) :  positionalArgs.First();


			// credential target is the only positional argument

			if (credStoreFile == null)
			{
				credStoreFile = CredentialStore.GetDefaultCredStoreFilePath();
			}

			if (!string.IsNullOrEmpty(tokenToStore))
			{
				// validate
				PackageServerAuth psAuth = new PackageServerAuth();
				var authorizedToken = psAuth.ValidateToken(tokenToStore).Result;
				
				if (String.IsNullOrEmpty(authorizedToken))
				{
					Console.WriteLine($"Warning - Credential provided is not valid for package server, will not be stored");
					return;
				}

				//store
				CredentialStore.Store(log, credTarget, new Credential("", tokenToStore), credStoreFile);
				return;
			}




			if (string.IsNullOrEmpty(username))
			{
				username = GetUsernameInteractively();
			}
			if (string.IsNullOrEmpty(password))
			{
				password = GetPasswordInteractively();
			}

			if (Path.GetExtension(credTarget.ToLowerInvariant()) == ".xml")
			{
				// if path to an xml file assume masterconfig
				// get the server list needed
				var servers = GetCredentialsNeededForMasterconfig(credTarget);

				foreach (var server in servers)
				{
					CredStoreOptions credStoreOptions = new CredStoreOptions(usePackageServer, credStoreFile, outputFileLocation, outputTokenToConsole, server, username, password);
					GetCredentialsForServer(log, credStoreOptions, true);
				}

				Console.WriteLine();
				Console.WriteLine("Masterconfig file {0} processed", credTarget);
				Console.WriteLine("All credentials stored in {0}.", credStoreFile);
				Console.WriteLine();
				Console.WriteLine("Please note that if this file is being created for automation farms that NAnt");
				Console.WriteLine("by default looks in the home directory for a file called nant_credstore.dat, for");
				Console.WriteLine("example %HOMEPATH%\\nant_credstore.dat or ~/nant_credstore.dat");
				Console.WriteLine();
				Console.WriteLine("You can override this location by setting the nant.credentialstore property when");
				Console.WriteLine("invoking your build.");
			}
			else
			{
				CredStoreOptions credStoreOptions = new CredStoreOptions(usePackageServer, credStoreFile, outputFileLocation, outputTokenToConsole, credTarget, username, password);
				// all else assume this is a particular server
				GetCredentialsForServer(log, credStoreOptions, false);
			}
		}

		private List<string> GetCredentialsNeededForMasterconfig(string masterconfig)
		{
			List<string> servers = new List<string>();
			foreach (Uri uri in XDocument.Load(masterconfig).Root.Element("masterversions").Descendants("package")
				.Select(xElem => xElem.Attribute("uri"))
				.Where(attr => attr != null)
				.Select(attr => new Uri(attr.Value))
				.GroupBy(uri => uri.Authority) // distinct authorities, we only need to check once per authority
				.Select(group => group.First()))
			{
				if (s_protocolsRequiringAuthentication.Contains(uri.Scheme))
				{
					servers.Add(uri.Authority);
				}
			}
			return servers;
		}

		private void GetCredentialsForServer(Log log, CredStoreOptions credstoreOptions, bool onlyIfMissing)
		{
			Credential oldCredentials = CredentialStore.Retrieve(log, credstoreOptions.Server, credstoreOptions.PathToCredstoreFile);
			string password = credstoreOptions.Password;
			if (!onlyIfMissing || oldCredentials == null)
			{
				if (credstoreOptions.Username.Contains('\0'))
				{
					throw new InvalidCommandArgumentsException(this,
						"Provided username contains an invalid null terminator character. This could be caused by the incorrect character being sent via a program like TightVNC.");
				}
				if (credstoreOptions.Password.Contains('\0'))
				{
					throw new InvalidCommandArgumentsException(this,
						"Provided password contains an invalid null terminator character. This could be caused by the incorrect character being sent via a program like TightVNC.");
				}
				if (credstoreOptions.UsePackageServer)
				{
					PackageServerAuth ps = new PackageServerAuth();
					var oauth = Convert.ToBase64String(System.Text.Encoding.UTF8.GetBytes($"{credstoreOptions.Username}:{credstoreOptions.Password}"));
					// lets replace the stored password with the received token
					password = ps.AuthenticateAsync($"Basic {oauth}").Result;
					if (credstoreOptions.OutputTokenToConsole)
					{
						Console.WriteLine($"Auth token");
						Console.WriteLine($"{password}");
					}
					if (!String.IsNullOrEmpty(credstoreOptions.OutputFilepath))
					{
						if (File.Exists(credstoreOptions.OutputFilepath))
						{
							Console.WriteLine($"Overwriting {credstoreOptions.OutputFilepath} with token");
						}
						string directoryPath = Path.GetDirectoryName(credstoreOptions.OutputFilepath);
						if (!Directory.Exists(directoryPath))
						{
							Directory.CreateDirectory(directoryPath);
						}
						File.WriteAllText(credstoreOptions.OutputFilepath, password);
					}
				}

				CredentialStore.Store(log, credstoreOptions.Server, new Credential(credstoreOptions.Username, password), credstoreOptions.PathToCredstoreFile);
				if (oldCredentials == null)
				{
					Console.WriteLine("Credential for {0} added.", credstoreOptions.Server);
				}
				else if (oldCredentials.Name == credstoreOptions.Username && oldCredentials.Password == password)
				{
					Console.WriteLine("Credential for {0} already up-to-date.", credstoreOptions.Server);
				}
				else if (oldCredentials.Name == credstoreOptions.Username)
				{
					Console.WriteLine("Credential for {0} password updated.", credstoreOptions.Server);
				}
				else if (oldCredentials.Password == password)
				{
					Console.WriteLine("Credential for {0} user updated.", credstoreOptions.Server);
				}
				else
				{
					Console.WriteLine("Credential for {0} updated.", credstoreOptions.Server);
				}
			}
			else
			{
				Console.WriteLine("Credstore already contains entry for server '{0}'. If you want to update this credential run \"eapm credstore {0}\".", credstoreOptions.Server);
			}
		}

		private string GetUsernameInteractively()
		{
			Console.Write("\tUsername (or email if using package server): ");
			return Console.ReadLine();
		}

		private string GetPasswordInteractively()
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
	}
}
