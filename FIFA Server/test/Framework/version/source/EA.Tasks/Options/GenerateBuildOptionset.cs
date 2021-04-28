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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections.Generic;

namespace EA.Eaconfig
{
	/// <summary>
	/// (Deprecated) Creates <token>buildtype</token> optionset from a meta build optionset. 
	/// This task is deprecated please use the Structured XML &lt;BuildType&gt; task instead.
	/// </summary>
	/// <example>
	///  <para>This is an example of how to generate a build type using GenerateBuildOptionset:</para>
	///  <code><![CDATA[
	///  <optionset name="config-options-library-custom" fromoptionset="config-options-library">
	///    <option name="buildset.name" value="Library-Custom" />
	///    <option name="optimization" value="off"/>
	///  </optionset>
	///  <GenerateBuildOptionset configsetname="config-options-library-custom"/>
	///  ]]></code>
	///  <para>The same thing can be done much more simply now with Structured XML:</para>
	///  <code><![CDATA[
	///  <BuildType name="Library-Custom" from="Library">
	///    <option name="optimization" value="off"/>
	///  </BuildType>
	///  ]]></code>
	/// </example>
	[TaskName("GenerateBuildOptionset")]
	public class GenerateBuildOptionset : EAConfigTask
	{
		public static String[] DefaultControllingProperties = { "package.usemultithreadeddynamiclib:multithreadeddynamiclib", 
																"eaconfig.usemultithreadeddynamiclib:multithreadeddynamiclib",
																"eaconfig.debugflags",
																"eaconfig.debugsymbols",
																"eaconfig.optimization",
																"eaconfig.usedebuglibs",
																"eaconfig.warnings",
																"eaconfig.misc",
																"eaconfig.warningsaserrors",
																"eaconfig.rtti",
																"eaconfig.exceptions",
																"eaconfig.toolconfig",
																"eaconfig.incrementallinking",
																"eaconfig.generatemapfile",
																"eaconfig.generatedll",
																"eaconfig.multithreadeddynamiclib",
																"eaconfig.managedcpp",
																"eaconfig.banner",                          
																"eaconfig.editandcontinue",
																"eaconfig.c7debugsymbols",
																"eaconfig.standardsdklibs",
																"eaconfig.runtimeerrorchecking",
																"eaconfig.pcconsole",
																"eaconfig.profile",
																"eaconfig.uselibrarydependencyinputs",
																"eaconfig.optimization.ltcg",
																"eaconfig.stripunusedsymbols",
																"eaconfig.stripallsymbols",
																"eaconfig.enable.strict.aliasing",
																"eaconfig.disable_reference_optimization",
																"eaconfig.disable-crt-secure-warnings",
																"eaconfig.iteratorboundschecking",
																"eaconfig.trace",
																"eaconfig.fastpdbgeneration",
																"eaconfig.msvc.permissive",
																"eaconfig.msvc.char8_t",
																"eaconfig.shortchar",
																"eaconfig.deterministic"
															 };

		/// <summary>
		/// The name of the meta optionset. 
		/// The name of the target <token>buildtype</token> optionset is provided by 'buildset.name' option
		/// </summary>
		[TaskAttribute("configsetname", Required = false)]
		public string ConfigSetName { get; set; } = String.Empty;

		/// <summary>
		/// List of the controlling properties. Intended to be used in the configuration packages to override default list of controlling properties like 
		/// 'eaconfig.debugflags", eaconfig.optimization, etc.
		/// </summary>
		[TaskAttribute("controllingproperties", Required = false)]
		public string ControllingProperties
		{
			set
			{
				_controllingProperties = value.Split(new char[] { ' ', '\n', '\t' });
			}
		}

		public static void Execute(Project project, string configsetname)
		{
			GenerateBuildOptionset task = new GenerateBuildOptionset(project, configsetname);
			task.Execute();
		}

		public static OptionSet Execute(Project project, OptionSet configoptionset, string configsetname, bool saveFinalToproject = true)
		{
			GenerateBuildOptionset task = new GenerateBuildOptionset(project, configoptionset, saveFinalToproject);
			task.ConfigSetName = configsetname;
			task.ExecuteTask();
			return task._final;
		}

		public GenerateBuildOptionset(Project project, string configsetname)
			: base(project)
		{
			ConfigSetName = configsetname;
		}

		public GenerateBuildOptionset(Project project, OptionSet configoptionset, bool saveFinalToproject = true)
			: base(project)
		{
			_ConfigOptionSet = configoptionset;
			_saveFinalToproject = saveFinalToproject;
		}


		public GenerateBuildOptionset()
			: base()
		{
		}

		/// <summary>Execute the task.</summary>
		protected override void ExecuteTask()
		{
			String buildOptionSetName = GetConfigOptionSet().Options["buildset.name"];
			if (String.IsNullOrEmpty(buildOptionSetName))
			{
				throw new BuildException($"{LogPrefix}Proto OptionSet '{ConfigSetName}' must have option 'buildset.name'.");
			}

			if (buildOptionSetName == ConfigSetName)
			{
				Log.Warning.WriteLine($"{LogPrefix}{Location}Final and proto BuildOptionSets have the same name '{buildOptionSetName}'. Proto BuilOptionSet will be overwritten.");
			}
			else if (Project.NamedOptionSets.Contains(buildOptionSetName))
			{
				Log.Warning.WriteLine($"{LogPrefix}{Location}OptionSet '{buildOptionSetName}' already exists. Attempt to create new OptionSet '{buildOptionSetName}' from proto option set '{ConfigSetName}' is ignored.");
				return;
			}

			foreach (string propertyName in _controllingProperties)
			{
				ApplyProperty(propertyName);
			}

			OptionSet buildset = new OptionSet() { Project = Project };
			foreach (var entry in GetConfigOptionSet().Options)
			{
				string name = entry.Key;

				if (!String.IsNullOrEmpty(name))
				{
					string newName = name.Replace("buildset.", "");
					if (name == "buildset.tasks")
					{
						newName = "build.tasks";
					}
					string value = entry.Value as String;

					// There should be no duplicates;
					if (buildset.Options.Contains(newName))
					{
						if (Log.Level > Log.LogLevel.Diagnostic)
						{
							Log.Warning.WriteLine($"{LogPrefix}{Location}Proto optionset '{ConfigSetName}' contains conflicting options:\n  {name}={GetConfigOptionSet().Options[name]}\n  {newName}={GetConfigOptionSet().Options[newName]}\n\nOption {name} from {GetConfigOptionSet().Options[name]} will be used.");
						}

						value = GetConfigOptionSet().Options[name] as string;
						if (value == null) value = String.Empty;

						buildset.Options[newName] = value;
					}
					else
					{
						buildset.Options.Add(newName, value);
					}
				}

			}

			var clanguage = buildset.Options["clanguage"];

			if (clanguage != null && clanguage.Equals("on", StringComparison.OrdinalIgnoreCase))
			{
				SetOptionToPropertyValue(buildset, "cc.template.commandline", "cc-clanguage.template.commandline");
				SetOptionToPropertyValue(buildset, "as.template.commandline", "as-clanguage.template.commandline");
				SetOptionToPropertyValue(buildset, "lib.template.commandline", "lib-clanguage.template.commandline");
				SetOptionToPropertyValue(buildset, "link.template.commandline", "link-clanguage.template.commandline");

				SetOptionToPropertyValue(buildset, "cc.template.responsefile.commandline", "cc-clanguage.template.responsefile.commandline");
				SetOptionToPropertyValue(buildset, "as.template.responsefile.commandline", "as-clanguage.template.responsefile.commandline");
				SetOptionToPropertyValue(buildset, "lib.template.responsefile.commandline", "lib-clanguage.template.responsefile.commandline");
				SetOptionToPropertyValue(buildset, "link.template.responsefile.commandline", "link-clanguage.template.responsefile.commandline");
			}

			GeneratePlatformOptions.Execute(Project, buildset, buildOptionSetName);

			//Remove options:
			var changedValues = new Dictionary<string, string>();
			foreach (var entry in buildset.Options)
			{
				if(!String.IsNullOrEmpty(entry.Key) && entry.Key.StartsWith("remove.") && !String.IsNullOrWhiteSpace(entry.Value))
				{
					string newName = entry.Key.Replace("buildset.", "").Replace("remove.", "");

					string valueToEdit;
					if (buildset.Options.TryGetValue(newName, out valueToEdit))
					{
						foreach(var toRemove in entry.Value.LinesToArray())
						{
							valueToEdit = valueToEdit.Replace(toRemove + " ", " ").Replace(toRemove + "\n", "\n").Replace(toRemove + "\r", "\r");
						}

						changedValues[newName] = valueToEdit;
					}
				}
			}
			foreach(var newEntry in changedValues)
			{
				buildset.Options[newEntry.Key] = newEntry.Value;
			}



			buildset.Options.Remove("buildset.protobuildtype");
			buildset.Options.Add("buildset.finalbuildtype", "true");
			buildset.Options.Add("buildset.protobuildtype.name", ConfigSetName);

			if (_saveFinalToproject)
			{
				Project.NamedOptionSets[buildOptionSetName] = buildset;
				// For backwards compatibility. Some build scripts wrongly use "buildsettings" option set.           
				Project.NamedOptionSets["buildsettings"] = buildset;
			}
			_final = buildset;

			//Some of the packages hook into the following internal properties.
			// This should be removed 
			Project.Properties["config.optionset.name"] = ConfigSetName;
			Project.Properties["build.optionset.name"] = buildOptionSetName;
		}

		private void ApplyProperty(string propertyName)
		{
			int optionNameIndex = propertyName.LastIndexOf(':');
			string optionName = null;
			if (optionNameIndex > 0)
			{
				optionName = propertyName.Substring(optionNameIndex + 1);
				propertyName = propertyName.Substring(0, optionNameIndex);
			}
			string val = Properties[propertyName];
			if (val != null)
			{
				if (String.IsNullOrEmpty(optionName))
				{
					int dotIndex = propertyName.IndexOf('.');
					optionName = propertyName.Substring(dotIndex < 0 ? 0 : dotIndex + 1);
				}

				if (val.Equals("true", StringComparison.OrdinalIgnoreCase) ||
					val.Equals("on", StringComparison.OrdinalIgnoreCase))
				{
					GetConfigOptionSet().Options[optionName] = "on";
				}
				else if (val.Equals("false", StringComparison.OrdinalIgnoreCase) ||
					val.Equals("off", StringComparison.OrdinalIgnoreCase))
				{
					GetConfigOptionSet().Options[optionName] = "off";
				}
				else
				{
					throw new BuildException($"{LogPrefix}Property '{propertyName}' has invalid value '{val}'. Valid values are: 'on', 'true', 'off', 'false'.");
				}
			}
		}

		private OptionSet GetConfigOptionSet()
		{
			if (_ConfigOptionSet == null)
			{
				_ConfigOptionSet = Project.NamedOptionSets[ConfigSetName];
				if (_ConfigOptionSet == null)
				{
					throw new BuildException($"{LogPrefix}OptionSet '{ConfigSetName}' does not exist.");
				}
				else if (_ConfigOptionSet.Options.Contains("buildset.finalbuildtype"))
				{
					throw new BuildException
					(
						$"{LogPrefix}'{ConfigSetName}' is not a proto-buildtype optionset, and can not be used to generate build types.\n" +
						"\n" +
						$"Possible reasons: '{ConfigSetName}' might be derived from a non-proto-buildtype optionset.\n" +
						"\n" +
						"Examples of proto-buildtypes are 'config-options-program', 'config-options-library' etc.\n" +
						"and any other optionsets that are derived from proto-buildtypes."
					);
				}
			}
			return _ConfigOptionSet;
		}

		private void SetOptionToPropertyValue(OptionSet set, string optionName, string propName)
		{
			string propVal = Project.Properties[propName];
			if (propVal != null && !set.Options.Contains(optionName))
			{
				set.Options[optionName] = propVal;
			}
		}

		private string[] _controllingProperties = DefaultControllingProperties;
		private OptionSet _ConfigOptionSet = null;
		private OptionSet _final = null;
		private bool _saveFinalToproject = true; 
	}
}
