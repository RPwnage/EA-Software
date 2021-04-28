// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2020 Electronic Arts Inc.
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
using NAnt.Core.Util;

namespace EA.Eaconfig.Structured
{
	/// <summary>
	/// A special task to define a shared precompiled header module name in build file.  Note that to 
	/// use this task, people must first declare this module in Initialize.xml using 'sharedpchmodule' public data task.
	/// </summary>
	[TaskName("SharedPch", NestedElementsCheck = true)]
	public class SharedPchTask : ConditionalTaskContainer
	{
		public SharedPchTask() : base()
		{
		}

		/// <summary>The name of this shared precompiled header module.</summary>
		[TaskAttribute("name", Required = true)]
		public string ModuleName { get; set; }

		protected string PackageName { get { return Project.Properties["package.name"]; } }

		protected override void ExecuteTask()
		{
			base.ExecuteTask();

			// Error check to make sure people setup their Initialize.xml property
			if (!Project.Properties.GetBooleanPropertyOrDefault("package." + PackageName + "." + ModuleName + ".issharedpchmodule", false))
			{
				throw new BuildException(string.Format("BUILD SCRIPT ERROR: To use <SharedPch> module task for module {0} (in package {1}), you must properly declare this module in Initialize.xml using <sharedpchmodule> public data task!", ModuleName, PackageName));
			}

			// Get buildtype info specified in Initialize.xml
			string initializeXmlBuildType = Project.Properties["package." + PackageName + "." + ModuleName + ".buildtype"];
			if (string.IsNullOrEmpty(initializeXmlBuildType))
			{
				throw new BuildException(string.Format("The <SharedPch> module task for module {0} (in package {1}) seems to be unable to retrieve buildtype info setup in Initialize.xml", ModuleName, PackageName));
			}

			if (XmlNode.ChildNodes.Count != 0)
			{
				throw new BuildException(string.Format("BUILD SCRIPT ERROR: The <SharedPch> module task for module {0} (in package {1}) should not specify any child XML elements.", ModuleName, PackageName));
			}

			// NOTE: This task is derived from a regular task instead of being derived from LibraryTask so that the generated help
			// page won't list extra attributes that we don't want user to modify.  We implement this task by internally creating a new
			// Library task based on settings in Initialize.xml so that we don't have to worry about inconsistency between settings
			// in .build file and Initialize.xml (especially when it comes to builtype settings where we need to dynamically convert
			// it to Utility for platforms that doesn't support this.  We should do that logic inside Framework and make sure it is 
			// applied in a consistent mannor.

			LibraryTask realModuleTask = new LibraryTask();
			realModuleTask.BuildType = initializeXmlBuildType;
			realModuleTask.Project = Project;
			realModuleTask.Parent = Parent;
			realModuleTask.ModuleName = ModuleName;
			realModuleTask.IfDefined = IfDefined;
			realModuleTask.UnlessDefined = UnlessDefined;
			realModuleTask.Location = Location;
			realModuleTask.Verbose = Verbose;
			realModuleTask.Group = BuildGroupBaseTask.GetGroupName(Project);

			// The current module's XML node should be empty already! But this object cannot be null or the Execute()
			// function would crash with null object!  So just assign it as current task's XmlNode.
			realModuleTask.XmlNode = XmlNode;

			realModuleTask.Config.PrecompiledHeader.Enable = Project.Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch",true);

			realModuleTask.Config.PrecompiledHeader.PchHeaderFile = Project.Properties["package." + PackageName + "." + ModuleName + ".pchheader"];
			realModuleTask.Config.PrecompiledHeader.PchHeaderFileDir = Project.Properties["package." + PackageName + "." + ModuleName + ".pchheaderdir"];
			string configSystem = Project.Properties["config-system"];
			if (Project.Properties.GetBooleanPropertyOrDefault(configSystem + ".support-shared-pch", false) &&
				Project.Properties.GetBooleanPropertyOrDefault("eaconfig.use-shared-pch-binary", true) &&
				realModuleTask.Config.PrecompiledHeader.Enable)
			{
				realModuleTask.Config.PrecompiledHeader.UseForcedInclude = true;
				realModuleTask.Config.PrecompiledHeader.PchFile = Project.Properties["package." + PackageName + "." + ModuleName + ".pchfile"];
			}

			string publicdependencies = Project.Properties.GetPropertyOrDefault("package."+PackageName+"."+ModuleName+".publicdependencies", null);
			if (!string.IsNullOrEmpty(publicdependencies))
			{
				realModuleTask.Dependencies.InterfaceDependencies.Value = publicdependencies;
			}

			string exportDefines = Project.Properties.GetPropertyOrDefault("package." + PackageName + "." + ModuleName + ".defines", null);
			if (!string.IsNullOrEmpty(exportDefines))
			{
				realModuleTask.Config.Defines.Value = exportDefines;
			}

			string includedirs = Project.Properties.GetPropertyOrDefault("package." + PackageName + "." + ModuleName + ".includedirs", null);
			if (!string.IsNullOrEmpty(includedirs))
			{
				realModuleTask.IncludeDirs.Value = includedirs;
			}

			realModuleTask.HeaderFiles.Include(realModuleTask.Config.PrecompiledHeader.PchHeaderFile, baseDir: realModuleTask.Config.PrecompiledHeader.PchHeaderFileDir);
			realModuleTask.SourceFiles.Include(Project.Properties["package." + PackageName + "." + ModuleName + ".pchsource"], optionSetName: "create-precompiled-header");

			realModuleTask.Execute();
		}
	}
}
