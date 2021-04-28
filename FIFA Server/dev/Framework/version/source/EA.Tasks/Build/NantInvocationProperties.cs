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

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;

using EA.Eaconfig.Core;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Build
{
	public static class NantInvocationProperties
	{
		public static string TargetToNantCommand(IModule module, TargetCommand targetCommand, bool addGlobal = false, PathString outputDir = null, Func<string, string> normalizePathString = null, bool isPortable = false)
		{
			// TODO: why this warning comment out? because of nant's copy-asset support actually creates a target as a postbuild step and thus
			// trips it's own warning - this can be re-enabled if the copy assets system is re-done
			// module.Package.Project.Log.ThrowWarning(Log.WarningId.NAntTargetAsBuildStep, Log.WarnLevel.Normal, "Module {0} is using a build step target without a command. The target to command auto-conversion will spawn additional nant processes which will impact build performance. Consider fixing the build step as this is being deprecated in the future. Refer to Framework Guide page on Pre/Post build steps.", module.Name);

			if (isPortable)
			{
				module.Package.Project.Log.ThrowWarning(Log.WarningId.NAntTargetAsBuildStepPortable, Log.WarnLevel.Normal, "Module {0} is generating a portable project with an automatic conversion from nant target to build step. This will only work if nant.exe is shipped at the correct relative to path to the generated project.", module.Name);
			}

			var userproperties = new Dictionary<string, string>();

			foreach(var item in targetCommand.TargetInput.Data)
			{
				if(item.Type == TargetInput.InputItemType.Property)
				{
					userproperties.Add(item.Name, module.Package.Project.Properties[item.SourceName]);
				}
			}

			var propertiesfilename = String.Format("target_{0}_properties_file.xml", targetCommand.Target);

			propertiesfilename = Path.Combine(outputDir.IsNullOrEmpty() ? module.IntermediateDir.Path : outputDir.Path, propertiesfilename);

			IDictionary<string, string> globalproperties = addGlobal ? new Dictionary<string, string>() : null;

			var properties = GetInvocationProperties(module, userproperties, globalproperties);

			if (normalizePathString != null)
			{
				foreach (var entry in properties.ToList())
				{
					properties[entry.Key] = normalizePathString(entry.Value);
				}
			}

			PropertiesFileLoader.SavePropertiesToFile(propertiesfilename, properties, globalproperties);

			Func<string, string> normalize = normalizePathString ?? (input => input);

			string monoPath = String.Empty;
			if (PlatformUtil.IsMonoRuntime)
			{
				monoPath = module.Package.Project.GetPropertyOrDefault("package.mono.mono-executable", "mono") + " ";
			}

			string masterConfigFragmentsArg = String.Empty;
			if (PackageMap.Instance.MasterConfig.CustomFragmentPaths.Any())
			{
				masterConfigFragmentsArg = $" -masterconfigfragments:{String.Join(";", PackageMap.Instance.MasterConfig.CustomFragmentPaths).Quote()}";
			}

			return String.Format("{0} -propertiesfile:{1} {2} -buildfile:{3} -masterconfigfile:{4}{5} -buildroot:{6} -loglevel:{7}",
				monoPath + normalize(String.Format("{0}/nant.exe",Project.NantLocation)),
				normalize(propertiesfilename).Quote(), 
				targetCommand.Target,
				normalize(module.Package.Project.BuildFileLocalName).Quote(),
				normalize(module.Package.Project.Properties[Project.NANT_PROPERTY_PROJECT_MASTERCONFIGFILE]).Quote(),
				masterConfigFragmentsArg,
				// Note that we need to trim the last backslash character before we put quote around it or windows command line will treat
				// the sequence \" as escape character
				normalize(module.Package.Project.Properties[Project.NANT_PROPERTY_PROJECT_BUILDROOT]).TrimEnd(new char[] { '\\' }).Quote(),
				module.Package.Project.Log.Level.ToString().ToLowerInvariant()
			);
		}


		public static IDictionary<string, string> GetInvocationProperties(IModule module, IDictionary<string, string> userproperties, IDictionary<string, string> globalproperties = null)
		{
			var properties = new SortedDictionary<string, string>();

			if (module != null && module.Package.Project != null)
			{
				PropagateProperties(properties, userproperties);

				PropagateProperties(properties, GetStandardNantInvokeProperties(module as ProcessableModule));

                OptionSet commandLinePropeties = module.Package.Project.NamedOptionSets[Project.NANT_OPTIONSET_PROJECT_GLOBAL_CMDPROPERTIES] ?? new OptionSet();
                OptionSet globalCommandLinePropeties = module.Package.Project.NamedOptionSets[Project.NANT_OPTIONSET_PROJECT_GLOBAL_CMDPROPERTIES] ?? new OptionSet();

                // filter cmd line properites that weren't specified global
                Dictionary<string, string> nonGlobalCmdLineProperties = commandLinePropeties.Options
                    .Where(prop => !globalCommandLinePropeties.Options.ContainsKey(prop.Key))
                    .ToDictionary(prop => prop.Key, prop => prop.Value);
				PropagateProperties(properties, nonGlobalCmdLineProperties);

                // filter cmd line properites that were specified global
                PropagateProperties(globalproperties, globalCommandLinePropeties.Options);

				if (globalproperties != null)
				{
					// Set global properties from Masterconfig:
					foreach (Project.GlobalPropertyDef propdef in Project.GlobalProperties.EvaluateExceptions(module.Package.Project))
					{
						if (propdef.Name == Project.NANT_PROPERTY_PROJECT_CMDTARGETS)
						{
							// This property is set at project initialization from the NAnt command line
							continue;
						}
						if (!globalproperties.ContainsKey(propdef.Name))
						{
							var val = module.Package.Project.Properties[propdef.Name];
							if (val != null)
							{
								globalproperties.Add(propdef.Name, val);
							}
						}
					}
				}
			}
			return properties;
		}

		private static void PropagateProperties(IDictionary<string, string> target, IDictionary<string, string> source)
		{
			foreach (var prop in source)
			{
				if (prop.Value != null && !target.ContainsKey(prop.Key))
				{
					target.Add(prop.Key, prop.Value);
				}
			}
		}

		private static IDictionary<string, string> GetStandardNantInvokeProperties(ProcessableModule module)
		{
			return new Dictionary<string, string>()
			{
				{ "config",                 module.Package.Project.Properties["config"] },
				{ "package.configs",        module.Package.Project.Properties["package.configs"] },
				{ "build.module",           module.Name },
				{ "build.buildtype",        module.BuildType.Name },
				{ "build.buildtype.base",   module.BuildType.BaseName },
				{ "groupname",              module.GroupName },
				{ "eaconfig.build.group",   Enum.GetName(typeof(BuildGroups), module.BuildGroup) },
				{ "build.outputname",       module.Package.Project.Properties[module.GroupName + ".outputname"] ?? module.Name },
			};
		}
	}
}
