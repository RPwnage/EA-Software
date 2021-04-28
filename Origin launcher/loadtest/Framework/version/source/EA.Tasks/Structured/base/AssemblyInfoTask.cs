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
using System.IO;
using System.Text.RegularExpressions;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
	/// <summary>If this XML element is present on a module definition Framework will generate an AssemblyInfo file and add it to the project</summary>
	[ElementName("AssemblyInfo", StrictAttributeCheck = true)]
	public class AssemblyInfoTask : ConditionalElementContainer
	{
		public bool Generate = false;

		/// <summary>Overrides the company field in the generated AssemblyInfo file, default is "Electronic Arts"</summary>
		[TaskAttribute("company", Required = false)]
		public string Company { get; set; }

		/// <summary>Overrides the copyright field in the generated AssemblyInfo file, default is "(c) Electronic Arts. All Rights Reserved."</summary>
		[TaskAttribute("copyright", Required = false)]
		public string Copyright { get; set; }

		/// <summary>Overrides the product field in the generated AssemblyInfo file, default is the package's name</summary>
		[TaskAttribute("product", Required = false)]
		public string Product { get; set; }

		/// <summary>Overrides the title field in the generated AssemblyInfo file, default is "(ModuleName).dll"</summary>
		[TaskAttribute("title", Required = false)]
		public string Title { get; set; }

		/// <summary>Overrides the description field in the generated AssemblyInfo file, default is the an empty string.</summary>
		[TaskAttribute("description", Required = false)]
		public string Description { get; set; }

		/// <summary>Overrides the version field in the generated AssemblyInfo file, default is the package version</summary>
		[TaskAttribute("version", Required = false)]
		public string VersionNumber { get; set; }

        [TaskAttribute("deterministic", Required = false)]
        public bool Deterministic { get; set; } = false;

        public string AssemblyExtension = ".dll";

		private const string DefaultCSharpText = 
@"using System.Reflection;

[assembly: AssemblyCompany(""{0}"")]
[assembly: AssemblyCopyright(""{1}"")]
[assembly: AssemblyProduct(""{2}"")]
[assembly: AssemblyTitle(""{3}"")]
[assembly: AssemblyDescription(""{4}"")]
[assembly: AssemblyVersion(""{5}"")]";

		private const string DefaultFSharpText =
@"module GeneratedAssemblyInfo

open System.Reflection

[<assembly: AssemblyCompany(""{0}"")>]
[<assembly: AssemblyCopyright(""{1}"")>]
[<assembly: AssemblyProduct(""{2}"")>]
[<assembly: AssemblyTitle(""{3}"")>]
[<assembly: AssemblyDescription(""{4}"")>]
[<assembly: AssemblyVersion(""{5}"")>]
()";

		private const string DefaultCppText =
@"using namespace System::Reflection;

[assembly: AssemblyCompanyAttribute(""{0}"")];
[assembly: AssemblyCopyrightAttribute(""{1}"")];
[assembly: AssemblyProductAttribute(""{2}"")];
[assembly: AssemblyTitleAttribute(""{3}"")];
[assembly: AssemblyDescriptionAttribute(""{4}"")];
[assembly: AssemblyVersionAttribute(""{5}"")];";

		public AssemblyInfoTask() : base()
		{
			Company = "Electronic Arts";
			Copyright = "(c) Electronic Arts. All Rights Reserved.";
			Product = "";
			Title = "";
			Description = "";
			VersionNumber = "";
		}

		protected override void InitializeElement(XmlNode elementNode)
		{
			// We want to only generate a default assemblyinfo file if the structured xml task is present on the module.
			// This Generate flag only gets set if the task has been used, if it is not used the flag will remain false.
			Generate = true;

			base.InitializeElement(elementNode);
		}

		public string WriteFile(string groupName, string moduleName, string configuration, BuildType type)
		{
			string packageName = Project.Properties["package.name"];
			string packageVersion = Project.Properties["package.version"];
			string groupModuleName = groupName + "." + moduleName;

			string generatedSourceDir = Path.Combine(Project.Properties["package.configbuilddir"], groupModuleName, "Properties", configuration);
			string generatedSourceFile = null;
			string template = null;
			if (type.IsFSharp) 
			{
				generatedSourceFile = Path.Combine(generatedSourceDir, "GeneratedAssemblyInfo.fs");
				template = DefaultFSharpText;
			}
			else if (type.IsManaged) 
			{
				generatedSourceFile = Path.Combine(generatedSourceDir, "GeneratedAssemblyInfo.cpp");
				template = DefaultCppText;
			}
			else if (type.IsCSharp) 
			{
				generatedSourceFile = Path.Combine(generatedSourceDir, "GeneratedAssemblyInfo.cs");
				template = DefaultCSharpText;
			}
			else
			{
				throw new ArgumentException(string.Format("Unexpected AssemblyInfoType '{0}'", type.ToString()));
			}

			string product = GetProduct(packageName);
			string title = GetTitle(moduleName);
			string description = Description ?? String.Empty;
			string version = GetAssemblyVersion(packageVersion).ToString();

			string fileContents = String.Format(template, Company, Copyright, product, title, description, version);

			if (!File.Exists(generatedSourceFile) || File.ReadAllText(generatedSourceFile) != fileContents)
			{
				Log.Info.WriteLine("Generating assembly info file '{0}'", generatedSourceFile);
				if (!Directory.Exists(generatedSourceDir))
				{
					Directory.CreateDirectory(generatedSourceDir);
				}
				File.WriteAllText(generatedSourceFile, fileContents);
			}
			return generatedSourceFile;
		}

		public string GetProduct(string packageName)
		{
			if (!Product.IsNullOrEmpty())
			{
				return Product;
			}
			return packageName;
		}

		public string GetTitle(string moduleName)
		{
			if (!Title.IsNullOrEmpty())
			{
				return Title;
			}
			return moduleName + AssemblyExtension;
		}

		public Version GetAssemblyVersion(string packageVersion)
		{
			if (!VersionNumber.IsNullOrEmpty())
			{
				return new Version(VersionNumber);
			}

			int suffixIndex = packageVersion.IndexOf("-");
			if (suffixIndex >= 0)
			{
				packageVersion = packageVersion.Substring(0, suffixIndex);
			}

			Version version;
			if (!Version.TryParse(packageVersion, out version))
			{
				version = new Version(0, 0, 0);
			}

			if (version.Revision == -1)
			{
                int major = version.Major < 0 ? 0 : version.Major;
                int minor = version.Minor < 0 ? 0 : version.Minor;
                int build = version.Build < 0 ? 0 : version.Build;
                int rev = 0;
                if (Deterministic == false)
                {
                    DateTime centuryBegin = new DateTime(2001, 1, 1);
                    DateTime currentDate = DateTime.Now;
                    long elapsedTicks = currentDate.Ticks - centuryBegin.Ticks;
                    TimeSpan elapsedSpan = new TimeSpan(elapsedTicks);
                    rev = (int)elapsedSpan.TotalDays;
                }
                version = new Version(major, minor, build, rev);
			}

			return version;
		}

		private string NormalizeString(string input)
		{
			// Nicolas' Regex failed to remove newline characters on Julien's pc, for reasons unknown.
			// However, some googling around leads me to believe that RegexOptions.Multiline is unreliable.
			//return Regex.Replace(input, @"\s*(.*)" + Environment.NewLine, "$1", RegexOptions.Multiline);
			return Regex.Replace(input, @"[\n\r]", "");
		}
	}
}
