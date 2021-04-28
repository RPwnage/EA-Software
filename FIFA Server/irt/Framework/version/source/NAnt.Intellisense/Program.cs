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
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Reflection;
using System.Security.Permissions;
using System.Diagnostics;
using System.Threading;

namespace NAnt.Intellisense
{
	class Program
	{
		static private bool SupressError = false;
		static private bool PrintLogo = false;
		static private bool Help = false;

		static private int ret = 0;

		static int Main(string[] args)
		{
			try
			{
				ParseCommandLine(args);

				if (PrintLogo)
				{
					ShowBanner();
				}
				else if (Help)
				{
					PrintHelp();
				}
				else
				{
					Install();
				}
			}
			catch (Exception ex)
			{
				SetError(ex);
			}

			return ret;
		}

		private static void Install()
		{
			string tempSchemaLocation = "../build/framework3.xsd";

			NAntSchemaTask.Install(tempSchemaLocation, new List<string>());
			NAntSchemaInstallerTask.Install(tempSchemaLocation);

			Console.WriteLine();
			Console.WriteLine("Intellisense Schema Copied Successfully");

			Thread.Sleep(1500);
		}

		private static void ParseCommandLine(string[] args)
		{
			string src = null;
			foreach (var arg in args)
			{
				if (arg[0] == '-' || arg[0] == '/')
				{
					ProcessOptions(arg.Skip(1).GetEnumerator());
				}
			}
			if (src != null)
			{
				throw new ArgumentException(String.Format("Invalid command line: source '{0}' does not have corresponding destination", src));
			}
		}

		private static void ProcessOptions(IEnumerator<char> argCharEnumerator)
		{
			while (argCharEnumerator.MoveNext())
			{
				switch (argCharEnumerator.Current)
				{
					case 'v':
						PrintLogo = true;
						break;
					case 'h':
						Help = true;
						break;
					default:
						Console.WriteLine("NAnt.Intellisense: unknown option '-{0}'", argCharEnumerator.Current);
						break;
				}
			}
		}

		private static void SetError(Exception e)
		{
			if (!SupressError)
			{
				Console.WriteLine("NAnt.Intellisense error: '{0}'", e.Message);
				ret = 1;
			}
		}

		private static void ShowBanner()
		{
			// Get version information directly from assembly.  This takes more work but keeps the version 
			// numbers being displayed in sync with what the assembly is marked with.
			string version = Assembly.GetExecutingAssembly().GetName().Version.ToString();

			string title = Assembly.GetExecutingAssembly().GetName().Name;
			object[] titleAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyTitleAttribute), true);
			if (titleAttribute.Length > 0)
				title = ((AssemblyTitleAttribute)titleAttribute[0]).Title;

			string copyright = "";
			object[] copyRightAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCopyrightAttribute), true);
			if (copyRightAttribute.Length > 0)
				copyright = ((AssemblyCopyrightAttribute)copyRightAttribute[0]).Copyright;

			string link = "";
			object[] companyAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCompanyAttribute), true);
			if (companyAttribute.Length > 0)
				link = ((AssemblyCompanyAttribute)companyAttribute[0]).Company;

			System.Console.WriteLine(String.Format("{0}-{1} {2}", title, version, copyright));
			System.Console.WriteLine(link);
			System.Console.WriteLine();
		}

		private static void PrintHelp()
		{
			Console.WriteLine("Usage: NAnt.Intellisense");
			Console.WriteLine();
			Console.WriteLine("  -v     Print Version Information");
			Console.WriteLine("  -h     Print Help Message");
			Console.WriteLine("  -s     Run in Silent Mode");

			Console.WriteLine();
			Console.WriteLine(" Installs Intellisense support for build files.");
			Console.WriteLine();
			Console.WriteLine(" Generates an intellisense schema and copies that to visual studio intellisense schema directory.");
			Console.WriteLine();
			Console.WriteLine(" To enable intellisense when editting make sure that the schema is added to the project node in your build file: " +
				"<project xmlns=\"schemas/ea/framework3.xsd\">.");
		}

	}
}
