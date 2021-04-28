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

using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal abstract class VSCommonProject : VSCppMsbuildProject // DAVE-FUTURE-REFACTOR-TODO: can we just merge this down into VSCppMsbuildProject?
	{
		public sealed override string ProjectFileName { get => ProjectFileNameWithoutExtension + ".vcxproj"; }

		internal VSCommonProject(VSSolutionBase solution, IEnumerable<IModule> modules)
			: base(solution, modules)
		{
		}

		protected override void AddProjectGlobals(IDictionary<string, string> projectGlobals)
		{
			foreach (var module in Modules)
			{
				string targetPlatformVer = module.Package.Project.GetPropertyOrDefault("package.WindowsSDK.TargetPlatformVersion", null);
				if (!string.IsNullOrEmpty(targetPlatformVer))
				{
					if ((module.Configuration.System == "pc" || module.Configuration.System == "pc64") && !projectGlobals.ContainsKey("WindowsTargetPlatformVersion"))
					{
						// This property in WindowsSDK is only created starting with version 10.x.
						// Setting this value allows Visual Studio to properly setup the default enviornment (like default include
						// paths, tools, etc).
						projectGlobals.Add("WindowsTargetPlatformVersion", targetPlatformVer);
					}
				}
			}
		}

		protected override void WriteProjectGlobals(IXmlWriter writer)
		{
			base.WriteProjectGlobals(writer);

			// Bug in msbuild task results in wrong warnings when files with same name but in different directories and built in different configs are present.
			writer.WriteStartElement("PropertyGroup");
			writer.WriteNonEmptyElementString("IgnoreWarnCompileDuplicatedFilename", "true");
			writer.WriteEndElement();
		}

		internal override bool SupportsDeploy(Configuration config)
		{
			var module = Modules.FirstOrDefault(m => m.Configuration == config);

			if (IsWinRTProgram(module as Module_Native) || IsGDKEnabledProgram(module as Module_Native))
			{
				return module.Package.Project.Properties.GetBooleanPropertyOrDefault("package.consoledeployment", true);
			}
			return base.SupportsDeploy(config);
		}

		protected override void WriteLinkerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			base.WriteLinkerTool(writer, name, tool, config, module, nameToDefaultXMLValue, file);

			var linker = tool as Linker;
			if (linker != null)
			{
				if (linker.UseLibraryDependencyInputs)
				{
					// Need an additional setting in to get UseLibraryDependencyInputs to work
					// <ItemDefinitionGroup Condition=" '$(Configuration)|$(Platform)' == 'pc64-vc-debug|x64' ">
					// ...
					//  <ProjectReference>
					//      <UseLibraryDependencyInputs>true</UseLibraryDependencyInputs>
					//  </ProjectReference>
					// </ItemDefinitionGroup>
					writer.WriteStartElement("ProjectReference");
					writer.WriteElementString("UseLibraryDependencyInputs", "TRUE");
					writer.WriteEndElement(); //Tool
				}
			}
		}

		protected override void InitCompilerToolProperties(Module module, Configuration configuration, string includedirs, string usingdirs, string defines, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue)
		{
			base.InitCompilerToolProperties(module, configuration, includedirs, usingdirs, defines, out nameToXMLValue, nameToDefaultXMLValue);

			// Remove some 2010 settings:
			nameToDefaultXMLValue.Remove("BufferSecurityCheck");

			// Add 2012 specific defaults:
			nameToDefaultXMLValue.Add("SDLCheck", "FALSE");
		}

		protected override void WriteExtensions(IXmlWriter writer)
		{
			base.WriteExtensions(writer);

			WriteSdkReferences(writer);
		}

		protected virtual void WriteSdkReferences(IXmlWriter writer)
		{
			var sdkreferences = new Dictionary<string, List<Configuration>>();
			foreach (Module module in Modules)
			{
				var sdkrefprop = (module.Package.Project.Properties[module.GroupName + ".sdkreferences." + module.Configuration.System] ?? module.Package.Project.Properties[module.GroupName + ".sdkreferences"]).TrimWhiteSpace();
				var globalsdkrefprop = (module.Package.Project.Properties["package.sdkreferences." + module.Configuration.System] ?? module.Package.Project.Properties["package.sdkreferences"]).TrimWhiteSpace();

				if (!(String.IsNullOrEmpty(sdkrefprop) && String.IsNullOrEmpty(globalsdkrefprop)))
				{

					var references = new List<string>(sdkrefprop.LinesToArray());
					references.AddRange(globalsdkrefprop.LinesToArray());

					foreach (var sdkref in references.OrderedDistinct())
					{
						List<Configuration> configs;
						if (!sdkreferences.TryGetValue(sdkref, out configs))
						{
							configs = new List<Configuration>();
							configs.Add(module.Configuration);
							sdkreferences.Add(sdkref, configs);
						}
						else
						{
							configs.Add(module.Configuration);
						}
					}
				}
			}

			foreach (var group in sdkreferences.GroupBy(f => GetConfigCondition(f.Value), f => f))
			{
				if (group.Count() > 0)
				{
					StartElement(writer, "ItemGroup", condition: group.Key);

					foreach (var sdkref in group)
					{
						writer.WriteStartElement("SDKReference");
						writer.WriteAttributeString("Include", sdkref.Key);
						writer.WriteEndElement();
					}
					writer.WriteEndElement();
				}
			}
		}

		protected override string GetConfigurationType(IModule module)
		{
			// ANDROID TODO: this is a bit confusing, this condition indicates whether there is a separate packaging module
			// which there is for Visual Studio android but not for Tegra which uses gradle packaging.
			if (module.Configuration.System == "android" && module.IsKindOf(Module.Program)
				&& module.Package.Project.Properties.GetPropertyOrDefault("package.android_config.build-system", "") != "tegra-android")
			{
				return "DynamicLibrary";
			}
			return base.GetConfigurationType(module);
		}
	}
}
