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
using System.Windows.Forms;
using EA.PackageSystem.PackageCore.Services;
using NAnt.Authentication;

namespace CredentialManager
{
	public class Program
	{
		private static void PrintHelp()
		{
			Console.WriteLine("NAnt Credential Manager (C) 2018 Electronic Arts");
			Console.WriteLine("");
			Console.WriteLine("Usage: CrendentialManager [server] [timeout]");
			Console.WriteLine("");
			Console.WriteLine("The Credential Manager is a GUI application for prompting a user " +
				"to enter their perforce or package server credentials and saving them to a credential store.");
		}

		public static void Main(string[] args)
		{
			string server = "";
			int timeoutSeconds = CredentialForm.NO_TIMEOUT;
			bool usePackageServer = false;
			Credential credential;
			if (args.Length > 0)
			{
				if (args[0] == "help" || args[0] == "-help")
				{
					PrintHelp();
					return;
				}
				server = args[0];
				if (server.Equals(WebServicesURL.GetWebServicesURL(false)) || server.Contains("packages.ea.com") || server.ToLower().Replace("-", "").Contains("packageserver"))
				{
					usePackageServer = true;
					server = WebServicesURL.GetWebServicesURL(false);
				}
			}
			if (args.Length > 1)
			{
				timeoutSeconds = int.Parse(args[1]);
			}

			CredentialForm credentialForm = new CredentialForm(server, timeoutSeconds, usePackageServer);
			DialogResult result = credentialForm.ShowDialog();

			if (result == DialogResult.Cancel) return;

			if (string.IsNullOrEmpty(server))
			{
				server = credentialForm.GetSelectedServer();
			}
			if (usePackageServer)
			{
				PackageServerAuth ps = new PackageServerAuth();
				var oauth = Convert.ToBase64String(System.Text.Encoding.UTF8.GetBytes($"{credentialForm.Username}:{credentialForm.Password}"));
				// validate the the token then create the credential using 
				var token = ps.AuthenticateAsync($"Basic {oauth}").Result;
				credential = new Credential(credentialForm.Username, token);
			}
			else
			{
				credential = new Credential(credentialForm.Username, credentialForm.Password);
			}
			
			
			if (credential != null)
			{
				// if user gave valid credential, update store on disk
				CredentialStore credStore = CredentialStore.LoadDecrypt(null, null);
				credStore.Add(server, credential);
				credStore.SaveEncrypt();
			}
		}
	}
}