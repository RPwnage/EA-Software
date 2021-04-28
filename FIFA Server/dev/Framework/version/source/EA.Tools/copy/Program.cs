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

				uint upToDate = 0;
				uint copied = 0;
				uint linked = 0;

				foreach (var data in SrcDst)
				{
					try
					{
						uint errorCount = Copy.Execute(data.Src, data.Dst, ref upToDate, ref copied, ref linked);
						if (errorCount > 0)
						{
							if (!SupressError)
							{
								ret = 1;
							}
						}
					}
					catch (Exception er)
					{
						SetError(er);
					}
				}

				if (Copy.Summary)
				{
					Console.WriteLine("copy: files copied={0}, hard linked={1}, up-to-date={2}.", copied, linked, upToDate); 
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
			string src = null;
			foreach (var arg in args)
			{
				if (NAnt.Core.Util.PlatformUtil.HostSupportedCommandLineSwitchChars.Contains(arg[0]))
				{
					ProcessOptions(arg.Skip(1).GetEnumerator());
				}
				else if (arg[0] == '@')
				{
					var file = arg.Substring(1);
					using (var reader = new StreamReader(file))
					{
						string line = null;
						src = null;
						while ((line = reader.ReadLine()) != null)
						{
							if (String.IsNullOrWhiteSpace(line))
							{
								continue;
							}
							if (src == null)
							{
								src = line.Trim(TRIM_CHARS);
							}
							else
							{
								SrcDst.Add(new Data(src, line.Trim(TRIM_CHARS) ));
								src = null;
							}
						}
						if (src != null)
						{
							throw new ArgumentException(String.Format("Invalid response file '{0}': source '{1}' does not have corresponding destination", file, src));
						}
					}
				}
				else
				{
					if (src == null)
					{
						src = arg.Trim(TRIM_CHARS);
					}
					else
					{

						SrcDst.Add(new Data( src, arg.Trim(TRIM_CHARS)));
						src = null;
					}
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
				ProcessCurrent:
				switch (argCharEnumerator.Current)
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
					case 'r':
						Copy.Recursive = true;
						break;
					case 'o':
						Copy.OverwriteReadOnlyFiles = true;
						break;
					case 'n':
						Copy.OverwriteReadOnlyFiles = false;
						break;
					case 'c':
						Copy.ClearReadOnlyAttributeAfterCopy = true;
						break;
					case 't':
						Copy.SkipUnchangedFiles = true;
						break;
					case 'l':
						Copy.UseHardLinksIfPossible = true;
						break;
					case 'x':
						Copy.UseSymlinksIfPossible = true;
						break;
					case 'f':
						Copy.CopyEmptyDir = true;
						break;
					case 'i':
						Copy.Info = true;
						break;
					case 'w':
						Copy.Warnings = true;
						break;
					case 'm':
						Copy.Summary = true;
						break;
					case 'y':
						Copy.IgnoreMissingInput = true;
						break;
					case 'd':
						Copy.DeleteOriginalFile = true;
						break;
					case 'N':
						Copy.CopyOnlyIfNewerOrMissing = true;
						break;
					case 'a':
						string retriesString = String.Empty;
						bool readNext = argCharEnumerator.MoveNext();
						while (readNext && Char.IsDigit(argCharEnumerator.Current))
						{
							retriesString += argCharEnumerator.Current;
							readNext = argCharEnumerator.MoveNext();
						}
						int retriesCount = 0;
						if (!Int32.TryParse(retriesString, out retriesCount))
						{
							throw new ArgumentException("Option 'a' must be immediately followed by retry count!");
						}
						Copy.Retries = retriesCount;
						if (readNext)
						{
							goto ProcessCurrent;
						}
						break;
					default:
						if (!Silent)
						{
							Console.WriteLine("copy: unknown option '-{0}'", argCharEnumerator.Current);
						}
						break;
				}
			}
		}

		private static void SetError(Exception e)
		{
			if (!SupressError)
			{
				if (!Silent)
				{
					Console.WriteLine("copy error: '{0}'", e.Message);
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
			Console.WriteLine("Usage: copy [-esvh...] src dst [src1 dst1 ...] [@responsefile]");
			Console.WriteLine();
			Console.WriteLine("  -e     Do not return error code");
			Console.WriteLine("  -s     Silent");
			Console.WriteLine("  -v     Print version (logo)");
			Console.WriteLine("  -r     recursively copy directories");
			Console.WriteLine("  -o     overwrite readonly files (default)");
			Console.WriteLine("  -n     do not overwrite readonly files");
			Console.WriteLine("  -c     clear readonly attributes after copy (if links are not used)");
			Console.WriteLine("  -t     copy only newer files");
			Console.WriteLine("  -l     use hard links if possible");
			Console.WriteLine("  -x     use symbolic links if possible");
			Console.WriteLine("  -f     copy empty directories");
			Console.WriteLine("  -i     print list of copied files");
			Console.WriteLine("  -w     print warnings");
			Console.WriteLine("  -m     print summary");
			Console.WriteLine("  -y     ignore missing input (do not return error)");
			Console.WriteLine("  -N     copy only if src file is newer or dst is missing.");
			Console.WriteLine("  -aNN   retry copies NN times");


			Console.WriteLine("  @[filename] Responsefile can contain list of source and destination, each path on a separate line.");
			Console.WriteLine("  file name portion of the source  can contain patterns '*' and '?' .");
		}

		private static List<Data> SrcDst = new List<Data>();

		struct Data
		{
			internal string Src;
			internal string Dst;

			internal Data(string src, string dst)
			{
				Src = src;
				Dst = dst;
			}
		}

		private static readonly char[] TRIM_CHARS = new char[] { '"', ' ', '\n', '\r' };

	}
}
