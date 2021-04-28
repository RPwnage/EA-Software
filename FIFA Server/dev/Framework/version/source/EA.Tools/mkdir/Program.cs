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
using System.IO;
using System.Linq;
using System.Reflection;

namespace EA.Tools
{
	class Program
	{
		static private bool SupressError = false;
		static private bool Silent = false;
		static private bool PrintLogo = false;
		static private bool Help = false;

		static private int ret = 0;

		static int Main(string[] args)
		{
			try
			{
				ParseCommandLine(args);

				if (!Silent && PrintLogo)
				{
					ShowBanner();
				}

				if (Help)
				{
					PrintHelp();
				}

				foreach (var dir in Directories)
				{
					try
					{
						Directory.CreateDirectory(dir);
					}
					catch (Exception er)
					{
						SetError(er);
					}
				}
			}
			catch (Exception ex)
			{
				SetError(ex);
			}
			return ret;
		}

		private static void ParseCommandLine(string[] args)
		{
			foreach (var arg in args)
			{
				if (NAnt.Core.Util.PlatformUtil.HostSupportedCommandLineSwitchChars.Contains(arg[0]))
				{
					for (int i = 1; i < arg.Length; i++)
					{
						SetOption(arg[i]);
					}
				}
				else if (arg[0] == '@')
				{
					var file = arg.Substring(1);
					using (var reader = new StreamReader(file))
					{
						string line = null;
						
						while ((line = reader.ReadLine()) != null)
						{
							Directories.Add(line.Trim(TRIM_CHARS));
						}
					}
				}
				else
				{
					Directories.Add(arg.Trim(TRIM_CHARS));
				}
			}
		}

		private static void SetOption(char o)
		{
			switch (o)
			{
				case 'e':
					SupressError = true;
					break;
				case 's':
					Silent = true;
					break;
				case 'v':
					PrintLogo = true;
					break;
				case 'h':
					Help = true;
					break;
				default:
					if (!Silent)
					{
						Console.WriteLine("mkdir: unknown option '-{0}'", o);
					}
					break;
			}
		}

		private static void SetError(Exception e)
		{
			if (!SupressError)
			{
				if (!Silent)
				{
					Console.WriteLine("fw_mkdir error: '{0}'", e.Message);
				}
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
			Console.WriteLine("Usage: mkdir [-esvh] dir1 [dir2, dir3 ...] [@responsefile]");
			Console.WriteLine();
			Console.WriteLine("  -e  Do not return error code");
			Console.WriteLine("  -s  Silent");
			Console.WriteLine("  -v  Print version (logo)");
			Console.WriteLine("  @[filename] Responsefile can contain list of directory names, each directory on a separate line.");
		}

		private static List<string> Directories = new List<string>();

		private static readonly char[] TRIM_CHARS = new char[] { '"', ' ', '\n', '\r' };

	}
}
