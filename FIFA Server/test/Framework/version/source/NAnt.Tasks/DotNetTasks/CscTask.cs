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
// Mike Krueger (mike@icsharpcode.net)

using System.Collections.Generic;
using System.IO;
using System.Linq;

using NAnt.Core.Attributes;
using NAnt.Core;
using NAnt.Core.Util;

namespace NAnt.DotNetTasks
{

	/// <summary>Compiles C# programs using csc, Microsoft's C# compiler.
	/// <para>
	/// Command line switches are passed using this syntax: <b>&lt;arg value=""/></b>.
	/// See examples below to see how it can be used outside OR inside the <b>&lt;args></b> tags.
	/// </para>
	/// </summary>
	/// <include file="Examples/Csc/HelloWorld.example" path="example"/>
	/// <include file="Examples/Csc/HelloArgs.example" path="example"/>
	/// <include file="Examples/Csc/Resources.example" path="example"/>
	/// <include file="Examples/Csc/WindowsForms.xml" path="example"/>
	[TaskName("csc")]
	public class CscTask : CompilerBase
	{
		public CscTask() : base("csc") { }

		/// <summary>The name of the XML documentation file to generate.  This attribute 
		/// corresponds to the <c>/doc:</c> flag.  By default, documentation is not 
		/// generated.</summary>
		[TaskAttribute("doc")]
		public string Doc { get; set; }

		/// <summary>A set of command line arguments.</summary>
		/// note this method exists solely for auto doc generation (nant.documenter)
		[ArgumentSet("args", Initialize = true)]
		public ArgumentSet ArgSet
		{
			get { return _argSet; }
		}

		/// <summary>The name of the XML documentation file to generate.  This attribute 
		/// corresponds to the <c>/doc:</c> flag.  By default, documentation is not 
		/// generated.</summary>
		[TaskAttribute("LanguageVersion")]
		public string LanguageVersion { get; set; }

		public bool useDebugProperty = true;

		public override string ProgramFileName
		{
			get
			{
				if (Compiler == null)
				{
					if (TargetFrameworkFamily != null && (TargetFrameworkFamily.ToLower() == "core" || TargetFrameworkFamily.ToLower() == "standard"))
					{
						return PathString.MakeNormalized(Project.Properties["package.DotNetCoreSdk.dotnet.exe"]).Path;
					}

					// Instead of relying on the .NET compilers to be in the user's path point
					// to the compiler directly since it lives in the .NET Framework's runtime directory
					if (PlatformUtil.IsWindows)
					{
						return Path.Combine(Path.GetDirectoryName(System.Runtime.InteropServices.RuntimeEnvironment.GetRuntimeDirectory()), Name);
					}

					string comp = null;
					string defaultComp = "csc";
					List<string> testPaths = new List<string>();
					string[] testComps = new string[] { "csc", "mcs" };  // Latest mono has csc, but not on older one.
					if (PlatformUtil.IsUnix)
					{
						testPaths.Add("/usr/local/bin/");
						testPaths.Add("/usr/bin/");
						testPaths.Add("/opt/novell/");
					}
					else if (PlatformUtil.IsOSX)
					{
						// On an OSX machine it is no longer being in /usr/bin for a long time.  It is now installed through an installer and it is typically
						// located in /Library/Frameworks/Mono.framework/Versions/Current/Commands.  This path is normally added to PATH environment by installer
						// But in some use cases, the PATH environment seems to got messed up (sometimes through Xcode launching another app), so we explicitly 
						// test for this as well.
						testPaths = System.Environment.GetEnvironmentVariable("PATH").Split(new char[] { ':', ';' }).ToList();
						testPaths.Insert(0, "/usr/bin");
						testPaths.Insert(0, "/usr/local/bin");
						testPaths.Insert(0, "/Library/Frameworks/Mono.framework/Versions/Current/Commands");
					}
					foreach (string testpath in testPaths)
					{
						foreach (string testcomp in testComps)
						{
							string cscPath = Path.Combine(testpath, testcomp);
							if (File.Exists(cscPath))
							{
								comp = cscPath;
								break;
							}
						}
						if (!string.IsNullOrEmpty(comp))
						{
							break;
						}
					}
					return (string.IsNullOrEmpty(comp) ? defaultComp : comp);
				}
				return Compiler;
			}
		}

		protected override void WriteOptions(TextWriter writer)
		{
			WriteOption(writer, "fullpaths"); // print full paths to file when reporting errors
			if (Doc != null)
			{
				WriteOption(writer, "doc", Doc);
			}

			if (useDebugProperty)
			{
				if (Debug)
				{
					WriteOption(writer, "debug");
					WriteOption(writer, "define", "DEBUG");
					WriteOption(writer, "define", "TRACE");
				}
				else
				{
					WriteOption(writer, "optimize");
				}
			}

			if (!LanguageVersion.IsNullOrEmpty())
			{
				WriteOption(writer, "langversion", LanguageVersion);
			}
		}

		protected override string GetExtension()
		{
			return "cs";
		}
	}
}
