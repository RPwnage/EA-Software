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
using NAnt.Core.Util;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Build
{
	abstract public class ModuleProcessorBase
	{
		private readonly ProcessableModule _module;

		public readonly Project Project;

		protected readonly string LogPrefix;

		protected ModuleProcessorBase(ProcessableModule module, string logPrefix = "")
		{
			Project = module.Package.Project;
			_module = module;
			LogPrefix = logPrefix;

			BuildOptionSet = module.GetModuleBuildOptionSet();
		}

		private BuildGroups BuildGroup
		{
			get { return _module.BuildGroup; }
		}

		public BuildType BuildType
		{
			get { return _module.BuildType; }
		}

		public readonly OptionSet BuildOptionSet;

		protected Log Log
		{
			get { return Project.Log; }
		}

		protected string PropGroupName(string name)
		{
			return _module.PropGroupName(name);
		}

		protected string PropGroupValue(string name, string defVal = "")
		{
			return _module.PropGroupValue(name, defVal);
		}

		protected FileSet PropGroupFileSet(string name)
		{
			return _module.PropGroupFileSet(name);
		}

		protected string GetModulePropertyValue(string propertyname, params string[] defaultVals)
		{
			if (propertyname == null && (defaultVals == null || defaultVals.Length == 0))
			{
				return null;
			}

			string value = String.Concat(PropGroupValue(propertyname, null), Environment.NewLine, PropGroupValue(propertyname + "." + _module.Configuration.System, null)).TrimWhiteSpace();

			if (String.IsNullOrEmpty(value) && (defaultVals != null && defaultVals.Length > 0))
			{
				value = StringUtil.ArrayToString(defaultVals, Environment.NewLine);
			}

			return value.TrimWhiteSpace();
		}

		protected FileSet AddModuleFiles(Module module, string propertyname, params string[] defaultfilters)
		{
			FileSet dest = new FileSet();
			if (0 < AddModuleFiles(dest, module, propertyname, defaultfilters))
			{
				return dest;
			}
			return null;
		}

		internal int AddModuleFiles(FileSet dest, Module module, string propertyname, params string[] defaultfilters)
		{
			int appended = 0;
			if (dest != null)
			{
				appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname), copyNoFailOnMissing: true);
				appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname + "." + module.Configuration.System), copyNoFailOnMissing: true);
				if (appended == 0 && defaultfilters != null)
				{
					foreach (string filter in defaultfilters)
					{
						dest.Include(Project.ExpandProperties(filter));
						appended++;
					}
				}
			}
			return appended;
		}

		protected string GetOptionString(string name, OptionDictionary options, string extra_propertyname = null, bool useOptionProperty = true, params string[] defaultVals)
		{
			if (options.TryGetValue(name, out string val))
			{
				if (Log.Level >= Log.LogLevel.Diagnostic && !String.IsNullOrEmpty(val))
				{
					Log.Debug.WriteLine(LogPrefix + "{0}: set option '{1}' from optionset '{2}': {3}", _module.ModuleIdentityString(), name, BuildType.Name, StringUtil.EnsureSeparator(val, " "));
				}
			}
			else if (useOptionProperty)
			{
				val = Project.Properties[name];

				if (Log.Level >= Log.LogLevel.Diagnostic && !String.IsNullOrEmpty(val))
				{
					Log.Debug.WriteLine(LogPrefix + "{0} set option '{1}' from property '{2}': {3}", _module.ModuleIdentityString(), name, name, StringUtil.EnsureSeparator(val, " "));
				}
			}

			// Append extra properties:
			string extra = GetModulePropertyValue(extra_propertyname, defaultVals);
			if (!string.IsNullOrEmpty(extra))
			{
				val += Environment.NewLine + extra;
			}

			if (!String.IsNullOrEmpty(val))
			{
				val = Project.ExpandProperties(val);
			}
			return val;
		}

		protected OptionSet GetFileBuildOptionSet(FileItem fileitem)
		{
			return GetFileBuildOptionSet(ref fileitem.OptionSetName, fileitem.Path.Path);
		}

		protected OptionSet GetFileBuildOptionSet(FileSetItem fileSetItem)
		{
			return GetFileBuildOptionSet(ref fileSetItem.OptionSet, fileSetItem.Pattern.Value);
		}

		protected OptionSet GetFileBuildOptionSet(ref string optionSetName, string itemPath)
		{
			// remap from legacy names
			if (optionSetName == "Program")
			{
				optionSetName = "StdProgram";
			}
			else if (optionSetName == "Library")
			{
				optionSetName = "StdLibrary";
			}

			// error if optionset does not exist
			OptionSet fileOptions = null;
			if (!Project.NamedOptionSets.TryGetValue(optionSetName, out fileOptions))
			{
				throw new BuildException($"[CreateCcTool] File set entry '{itemPath} optionset '{optionSetName}' does not exist.");
			}

			// slight hack, only pch optionsets have a single entry so if the count of options is one do some special logic //TODO *slight* hacK? this is quite nasty
			if (fileOptions.Options.Count == 1)
			{
				// error just in case this wasn't a pch fileset
				KeyValuePair<string, string> entry = fileOptions.Options.First();
				if (entry.Key != "use-pch" && entry.Key != "create-pch")
				{
					throw new BuildException($"File set entry '{itemPath}' specified optionset '{optionSetName}' with a single option contains an invalid option '{entry.Key}={entry.Value}'. Valid options for a file set with one option are 'create-pch' or 'use-pch'.");
				}

				// ignore pch optionsets if enable-pch is false
				if (!Project.Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch", true))
				{
					optionSetName = null;
					return null;
				}

				// check if type of pch optionset, sets prefix only if option was enabled 
				string postfix = String.Empty;
				OptionDictionary opts = new OptionDictionary();
				if (fileOptions.GetBooleanOption("create-pch"))
				{
					postfix = "_CreatePch";
					opts.Add("create-pch", "on");
				}
				else if (fileOptions.GetBooleanOption("use-pch"))
				{
					postfix = "_UsePch";
					opts.Add("use-pch", "on");
				}
				if (String.IsNullOrEmpty(postfix))
				{
					// neither pch option was enabled, return null and use default optionset
					optionSetName = null;
					return null;
				}

				if (ModuleExtensions.ShouldForceDisableOptimizationForModule(Project,_module.Package.Name,_module.BuildGroup.ToString(),_module.Name))
				{
					postfix += "_DisOptm";
					opts.Add("optimization", "off");
				}

				// generate a new optionset based of module buildtype but with additional pch option (or reuse if it already exists)
				string fileOptionSetName = _module.BuildType.Name + postfix;
				OptionSet fileOptionSet = Project.NamedOptionSets.GetOrAdd(fileOptionSetName, oname =>
				{
					OptionSet os = new OptionSet();
					foreach (KeyValuePair<string,string> opt in opts)
					{
						os.Options[opt.Key] = opt.Value;
					}
					return EA.Eaconfig.Structured.BuildTypeTask.ExecuteTask(Project, oname, _module.BuildType.Name, os, saveFinalToproject: false);

				});
				optionSetName = fileOptionSetName;
				return fileOptionSet;

			}
			else if (fileOptions.Options.Count > 1)
			{
				return fileOptions;
			}

			return null; // optionset was empty, ignore and use default
		}
	}

	internal class DependencyDefinition
	{
		internal readonly string Key;
		internal readonly string PackageName;
		internal readonly string ConfigurationName;
		
		internal ISet<Tuple<BuildGroups, string>> ModuleDependencies
		{
			get
			{
				return _moduledependencies;
			}
		}
		internal HashSet<Tuple<BuildGroups, string>> _moduledependencies;

		internal DependencyDefinition(string packagename, string configuration)
		{
			PackageName = packagename;
			ConfigurationName = configuration;
			Key = CreateKey(packagename, configuration);
		}

		internal static string CreateKey(string packagename, string configuration)
		{
			return packagename + ":" + configuration;
		}
		
		internal bool AddModuleDependent(BuildGroups group, string moduleNmae)
		{
			if (_moduledependencies == null)
			{
				_moduledependencies = new HashSet<Tuple<BuildGroups, string>>(); ;
			}
			return _moduledependencies.Add(Tuple.Create<BuildGroups, string>(group, moduleNmae));
		}
	}
}
