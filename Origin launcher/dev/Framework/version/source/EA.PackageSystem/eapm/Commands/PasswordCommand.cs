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
using System.Linq;

using NAnt.Authentication;
using NAnt.Core.Logging;

namespace EA.PackageSystem.ConsolePackageManager
{
    [Command("password")]
	internal class PasswordCommand : Command
	{
        internal override string Summary
		{
			get { return "Set package server password."; }
		}

        internal override string Usage
		{
			get { return "<username> <password> <domain>"; }
		}

        internal override string Remarks
		{
			get
			{
				return @"Description
	Stores the user's package server credentials in encrypted credential store file.

Examples
	Prompts the user to enter their username, password and domain for 
	package server authentication. When typing the password the text is 
	hidden.
	> eapm password

	Creates environment variables for username, password and domain. 
	The password is encrypted and stored.
	> eapm password <username> <password> <domain>";
			}
		}

        internal override void Execute()
		{
			string user = null;
			string domain = null;
			string password = null;
			if (!Arguments.Any())
			{
				user = GetUsernameInteractively();
				domain = GetDomainInteractively();
				password = GetPasswordInteractively();
			}
			else if (Arguments.Count() == 3)
			{
                user = Arguments.ElementAt(0);
				password = Arguments.ElementAt(1);
                domain = Arguments.ElementAt(2);
            }
			else
			{
				throw new InvalidCommandArgumentsException(this);
			}

			if (user.Contains('\0'))
			{
				throw new InvalidCommandArgumentsException(this, 
					"Provided username contains an invalid null terminator character. This could be caused by the incorrect character being sent via a program like TightVNC.");
			}
			if (password.Contains('\0'))
			{
				throw new InvalidCommandArgumentsException(this,
					"Provided password contains an invalid null terminator character. This could be caused by the incorrect character being sent via a program like TightVNC.");
			}
			if (domain.Contains('\0'))
			{
				throw new InvalidCommandArgumentsException(this,
					"Provided domain contains an invalid null terminator character. This could be caused by the incorrect character being sent via a program like TightVNC.");
			}

			user = !String.IsNullOrWhiteSpace(domain) ? domain + '\\' + user : user;

			CredentialStore.Store(Log.Default, CredentialStore.FrameworkGenericNetLoginKey, new Credential(user, password));
		}

		private string GetUsernameInteractively()
		{
			Console.Write("Enter Username (without domain): ");
			return Console.ReadLine();
		}

		private string GetDomainInteractively()
		{
			Console.Write("Enter Domain: ");
			return Console.ReadLine();
		}

		private string GetPasswordInteractively()
		{
			Console.Write("Enter Password: ");

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
