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
using System.Linq;
using System.Net;
using System.Security.Principal;

using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace NAnt.Authentication
{
	public static class AuthenticationUtil
	{
		// return true if split was successful
		public static bool SplitDomainAndUser(string userAndDomain, out string user, out string domain)
		{
			int index = userAndDomain.IndexOf('\\');
			if (index == -1)
			{
				user = userAndDomain;
				domain = null;
				return false;
			}
			user = userAndDomain.Substring(index+1);
			domain = userAndDomain.Substring(0, index);
			return true;
		}

		// get list of potential names for the local user, name closer to the top of the list are more likely to
		// be definitive, if include undomained variants is true user names which contain a domain will be included
		// again at the end of the list without a domain
		public static string[] GetLocalUserNames(bool includeUndomainedVariants = false)
		{
			// credential cache
			List<string> userSet = new List<string>();
			string defaultUserName = CredentialCache.DefaultNetworkCredentials.UserName;
			string defaultUserDomain = CredentialCache.DefaultNetworkCredentials.Domain;
			if (!String.IsNullOrWhiteSpace(defaultUserName))
			{
				if (!String.IsNullOrWhiteSpace(defaultUserDomain))
				{
					userSet.Add(String.Format("{0}\\{1}", defaultUserDomain, defaultUserName));
				}
				else
				{
					userSet.Add(defaultUserName);
				}
			}

			// windows identity
			if (PlatformUtil.IsWindows)
			{
				userSet.Add(GetWindowsIdentityCurrentName());
			}

			// environment
			userSet.Add(GetEnvironmentUserName());

			// strip out nulls and duplicates from list
			userSet = userSet.Where(u => !String.IsNullOrWhiteSpace(u)).Distinct().ToList();

			// add undomained version at the end in same order
			if (includeUndomainedVariants)
			{
				foreach (string userAndDomain in userSet.ToArray())
				{
					string user = null;
					string domain = null;
					if (SplitDomainAndUser(userAndDomain, out user, out domain))
					{
						userSet.Add(user);
					}	
				}
			}

			return userSet.Distinct().ToArray();
		}

		public static string GetWindowsIdentityCurrentName()
		{
			// This is wrapped inside a function so that caller function don't see the "WindowsIdentity".  If caller function
			// see this class, the type will get loaded before the function is executed and when the code is executed under
			// mono, it will crash because this class is not implemented.
			return WindowsIdentity.GetCurrent().Name;
		}

		// returns a local user name suitable for reporting purposes
		public static string GetReportingLocalUserName(Log log, string credentialFile = null)
		{
			// use WindowsIdentity on windows
			if (PlatformUtil.IsWindows)
			{
				string identityName = GetWindowsIdentityReportingName();
				if (identityName != null)
				{
					return identityName;
				}
			}

			// use credential store generic login if available
			Credential genericLogonCredential = CredentialStore.Retrieve(log, CredentialStore.FrameworkGenericNetLoginKey, credentialFile);
			if (genericLogonCredential != null)
			{
				return genericLogonCredential.Name;
			}

			return GetEnvironmentUserName() ?? "unknwown";
		}

		public static bool IsSystemAccount()
		{
			if (PlatformUtil.IsWindows)
			{
				return GetIsWindowsSystemAccount();
			}
			return false;
		}

		private static string GetEnvironmentUserName()
		{
			string name = Environment.UserName;
			if (!String.IsNullOrWhiteSpace(name))
			{
				string domain = Environment.UserDomainName;
				if (!String.IsNullOrEmpty(domain))
				{
					name = domain + "\\" + name;
				}
				return name;
			}
			return null;
		}

		// these functions are extracted from the single places
		// they are used and wrapped with platform checks, we 
		// want to avoid mono trying to resolve WindowsIdentity
		// type
		private static bool GetIsWindowsSystemAccount()
		{
			return WindowsIdentity.GetCurrent().IsSystem;
		}

		private static string GetWindowsIdentityReportingName()
		{
			string user = null;
			WindowsIdentity identity = WindowsIdentity.GetCurrent();
			user = identity.Name;
			if (user != null)
			{
				if (identity.IsSystem)
				{
					user = Environment.MachineName + "\\" + user;
				}
				return user;
			}
			return null;
		}
	}
}