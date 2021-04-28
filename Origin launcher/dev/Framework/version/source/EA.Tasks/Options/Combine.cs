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

using NAnt.Core;
using NAnt.Core.Attributes;
using EA.Eaconfig.Build;
using NAnt.Core.Util;

namespace EA.Eaconfig
{
	/// <summary>
	/// An internal task used by eaconfig to setup buildtypes. Not intended to be used by end users.
	/// </summary>
	[TaskName("Combine")]
	public class Combine : EAConfigTask
	{
		public static readonly string[] CONFIG_OPTION_SETS = { 
				  "config-options-program", 
				  "config-options-library", 
				  "config-options-dynamiclibrary",
				  "config-options-windowsprogram", 
				  "config-options-managedcppprogram", 
				  "config-options-managedcppwindowsprogram", 
				  "config-options-managedcppassembly",
				  "config-options-pythonprogram",
				  "config-options-csharplibrary",
				  "config-options-csharpprogram",
				  "config-options-csharpwindowsprogram",
				  "config-options-rsoprogram",
				  "config-options-winrtcppprogram",
				  "config-options-winrtcpplibrary",
				  "config-options-winrtcppdynamiclibrary"
		};


		public static void Execute(Project project)
		{
			Combine task = new Combine(project);
			task.Execute();
		}

		public Combine(Project project)
			: base(project)
		{
		}

		public Combine()
			: base()
		{
		}

		/// <summary>Execute the task.</summary>
		protected override void ExecuteTask()
		{
			VerifyConfigOptionsetsExist();

			List<string> buildSets = new List<string>();
			buildSets.AddRange(CONFIG_OPTION_SETS);

			SetRemappedLibBinDirectories();

			//NOTICE(RASHIN):
			//The below allows SDK packages to provide their own optionsets
			//but delay in actually generating them until other optionsets are defined (fromoptionset).
			string osName = ConfigSystem + ".buildoptionsets";
			OptionSet platformOS;
			if (Project.NamedOptionSets.TryGetValue(osName, out platformOS))           
			{
				foreach (KeyValuePair<string,string> entry in platformOS.Options)
				{
					string name = (string)entry.Key;
					buildSets.Add(name);
				}
			}

			foreach (string setName in buildSets)
			{
				if (Project.NamedOptionSets.Contains(setName))
				{
					MergeOptionset.Execute(Project, setName, "config-options-common");
				}
			}

			SetDefaultOptionsIfMissing();

			var optionsetsToGenerate = new List<string>();

			foreach (string setName in buildSets)
			{
				if (Project.NamedOptionSets.Contains(setName))
					optionsetsToGenerate.Add(setName);
			}

			GenerateAdditionalBuildTypes(optionsetsToGenerate);

			NAntUtil.ParallelForEach
			(
				optionsetsToGenerate,
				(name) =>
				{
					GenerateBuildOptionset.Execute(Project, name);
				},
				noParallelOnMono: true
			);
		}

		// Rather than randomly crashing somewhere inside the combine task we will first verify that the config package has defined the required base optionsets
		private void VerifyConfigOptionsetsExist()
		{
			string[] requiredOptionsets = new string[] { "config-options-common", "config-options-program", "config-options-library", "config-options-dynamiclibrary" };
			foreach (string optionset in requiredOptionsets)
			{
				if (!Project.NamedOptionSets.Contains(optionset))
				{
					throw new BuildException(string.Format("The base optionset '{0}' has not been defined by any of the config packages, this most likely means that your top level config package (eaconfig) is not a valid config package.", optionset));
				}
			}
		}

		private void GenerateAdditionalBuildTypes(List<string> optionsetsToGenerate)
		{
			// --- Generate C language Program and Library BuildOptionSets ---:

			OptionSet libC = CreateBuildOptionSetFrom("CLibrary", "config-options-library-c", "config-options-library");
			libC.Options["clanguage"] = "on";
			SetOptionToPropertyValue(libC, "cc", "cc-clanguage");
			SetOptionToPropertyValue(libC, "as", "as-clanguage");
			SetOptionToPropertyValue(libC, "lib", "lib-clanguage");
			SetOptionToPropertyValue(libC, "link", "link-clanguage");

			optionsetsToGenerate.Add("config-options-library-c");

			OptionSet progC = CreateBuildOptionSetFrom("CProgram", "config-options-program-c", "config-options-program");
			progC.Options["clanguage"] = "on";
			SetOptionToPropertyValue(progC, "cc", "cc-clanguage");
			SetOptionToPropertyValue(progC, "as", "as-clanguage");
			SetOptionToPropertyValue(progC, "lib", "lib-clanguage");
			SetOptionToPropertyValue(progC, "link", "link-clanguage");


			optionsetsToGenerate.Add("config-options-program-c");

			// --- Generate DynamicLibrary-Static-Link BuildOptionSet ---:
			if (Project.NamedOptionSets.Contains("config-options-dynamiclibrary"))
			{
				// Provide a DLL optionset which doesn't define EA_DLL, so has static linkage against EAThread or
				// other packages which provide DLL configurations.  This should probably be the default behaviour
				// for DynamicLibrary actually, but that would break existing clients. -->
				OptionSet dynLibStat = new OptionSet();
				dynLibStat.FromOptionSetName = "config-options-dynamiclibrary";
				dynLibStat.Project = Project;
				dynLibStat.Append(OptionSetUtil.GetOptionSet(Project, dynLibStat.FromOptionSetName));
				dynLibStat.Options["buildset.name"] = "DynamicLibraryStaticLinkage";
				if (!Properties.GetBooleanPropertyOrDefault("Dll",false))
				{
					OptionSetUtil.ReplaceOption(dynLibStat, "buildset.cc.defines", "EA_DLL", String.Empty);
				}
				Project.NamedOptionSets["options-dynamiclibrary-static-linkage"] = dynLibStat;

				optionsetsToGenerate.Add("options-dynamiclibrary-static-linkage");

				//CDynamicLibrary
				OptionSet dynlibC = CreateBuildOptionSetFrom("CDynamicLibrary", "config-options-dynamiclibrary-c", "config-options-dynamiclibrary");
				dynlibC.Options["clanguage"] = "on";
				SetOptionToPropertyValue(dynlibC, "cc", "cc-clanguage");
				SetOptionToPropertyValue(dynlibC, "as", "as-clanguage");
				SetOptionToPropertyValue(dynlibC, "lib", "lib-clanguage");
				SetOptionToPropertyValue(dynlibC, "link", "link-clanguage");

				optionsetsToGenerate.Add("config-options-dynamiclibrary-c");
			}
		}

		protected bool SetOptionIfMissing(OptionSet optionset, string optionName, string optionValue)
		{
			bool ret = false;
			if (!optionset.Options.Contains(optionName))
			{
				optionset.Options[optionName] = optionValue;
				ret = true;
			}
			return ret;
		}


		protected void SetRemappedLibBinDirectories()
		{
			string packageName = Properties["package.name"]; // package.name is sometimes unset in test cases
			if (packageName != null)
			{
				OptionSet mapping = ModulePath.Private.PackageOutputMapping(Project, packageName);
				if (mapping != null)
				{
					string configlibdir;
					if (mapping.Options.TryGetValue("configlibdir", out configlibdir))
					{
						Project.Properties["package.configlibdir"] = configlibdir;
					}

					string configbindir;
					if (mapping.Options.TryGetValue("configbindir", out configbindir))
					{
						Project.Properties["package.configbindir"] = configbindir;
					}
				}
			}
		}

		//IMTODO: remove this. All should be set in the platform-specific sections:
		private void SetDefaultOptionsIfMissing()
		{
			//Defaults for DLL
			OptionSet dynamiclibrary;
			if (Project.NamedOptionSets.TryGetValue("config-options-dynamiclibrary", out dynamiclibrary))
			{
				SetOptionIfMissing(dynamiclibrary, "linkoutputmapname", @"%outputdir%\%outputname%.map");
				if (ConfigCompiler == "vc")
				{
					SetOptionIfMissing(dynamiclibrary, "linkoutputpdbname", @"%outputdir%\%outputname%.pdb");
					SetOptionIfMissing(dynamiclibrary, "linkoutputname", @"%outputdir%\%outputname%.dll");
				}
				if (Properties["dll-suffix"] != null)
				{
					SetOptionIfMissing(dynamiclibrary, "linkoutputname", @"%outputdir%/%outputname%" + Properties["dll-suffix"]);
				}
			}

			//Defaults for Program.
			OptionSet program = Project.NamedOptionSets["config-options-program"];
			if (program != null)
			{
				SetOptionIfMissing(program, "linkoutputname", @"%outputdir%/%outputname%" + Properties["exe-suffix"]);
				SetOptionIfMissing(program, "linkoutputpdbname", @"%outputdir%\%outputname%.pdb");
				SetOptionIfMissing(program, "linkoutputmapname", @"%outputdir%\%outputname%.map");
			}

			//Defaults for the Managed buildtypes for PC.
			string[] managedExeNames = { "windowsprogram", "managedcppprogram", "managedcppwindowsprogram", "csharpprogram", "csharpwindowsprogram", "winrtcppprogram" };
			string[] managedLibraryNames = { "managedcppassembly", "csharplibrary", "winrtcpplibrary", "winrtcppdynamiclibrary" };
			foreach (string name in managedExeNames.Concat(managedLibraryNames))
			{
				OptionSet os;

				if (Project.NamedOptionSets.TryGetValue("config-options-" + name, out os))
				{
					if (managedLibraryNames.Contains(name))
					{
						SetOptionIfMissing(os, "linkoutputname", @"%outputdir%\%outputname%.dll");
					}
					else
					{
						SetOptionIfMissing(os, "linkoutputname", @"%outputdir%\%outputname%.exe");
					}
					SetOptionIfMissing(os, "linkoutputpdbname", @"%outputdir%\%outputname%.pdb");
					SetOptionIfMissing(os, "linkoutputmapname", @"%outputdir%\%outputname%.map");
				}
			}
		}

		private void SetOptionToPropertyValue(OptionSet set, string optionName, string propName)
		{
			string propVal = Project.Properties[propName];
			if (propVal != null)
			{
				set.Options[optionName] = propVal;
			}
		}
	}
}
