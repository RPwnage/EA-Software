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

using System.Linq;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;

using EA.Eaconfig.Build;

namespace EA.Eaconfig
{
	/// <summary>
	/// Create a build graph for a specific package instead of current package.
	/// </summary>
	[TaskName("create-build-graph-for-package")]
	public class CreateBuildGraphForPackageTask : Task
	{
		public CreateBuildGraphForPackageTask() : base("create-build-graph-for-package") { }

		/// <summary>
		/// The package we want to create the build graph for.  If not specified, will create build graph for current package.
		/// </summary>
		[TaskAttribute("package", Required = false)]
		public string BuildPackage { get; set; }

		protected override void ExecuteTask()
		{
			string packageName = Project.Properties.GetPropertyOrFail("package.name");
			if (packageName != null && packageName != BuildPackage && !string.IsNullOrEmpty(BuildPackage))
			{
				Log.Minimal.WriteLine(LogPrefix + "Perform build graph generation for specified package: " + BuildPackage + " ...");
				TaskUtil.Dependent(Project, BuildPackage);
				packageName = BuildPackage;
			}
			else
			{
				Log.Minimal.WriteLine(LogPrefix + "Perform build graph generation for current package: " + packageName + " ...");
			}
			string packageBuildFile = NAnt.Core.Util.PathString.MakeCombinedAndNormalized(
				Project.Properties.GetPropertyOrFail("package." + packageName + ".dir"),
				packageName + ".build").Path;
			
			// This is basically the eaconfig-build-graph-all implementation with small tweak
			// to get build graph for another package rather than current package.
			string currConfig = Project.Properties.GetPropertyOrFail("config");
			string autoBuildTarget = 
					Project.Properties.GetPropertyOrDefault("eaconfig.build.autobuild-target",
					"load-package");
			string buildAllTarget = 
					Project.Properties.GetPropertyOrDefault("eaconfig.buildall.target",
					"load-package");
			string buildConfigs = 
					Project.Properties.GetPropertyOrDefault("eaconfig.build.configs",
					Project.Properties.GetPropertyOrFail("package.configs"));
			if (!buildConfigs.ToArray().Contains(currConfig))
			{
				buildConfigs = currConfig + " " + buildConfigs;
			}
			string buildgroup = Project.Properties.GetPropertyOrDefault("eaconfig.build.group", "runtime");
			string buildGroups = 
					Project.Properties.GetPropertyOrDefault("__eaconfig.build.groups",
					Project.Properties.GetPropertyOrDefault("eaconfig.build.group",
					"runtime tool example test"));
			bool buildProcessGenerationData = 
					Project.Properties.GetBooleanPropertyOrDefault("eaconfig.build.process-generation-data",
					false);

			Task_InitBuildGraph initBuildGraph = new Task_InitBuildGraph();
			initBuildGraph.BuildConfigurations = buildConfigs;
			initBuildGraph.BuildGroupNames = buildGroups;
			initBuildGraph.ProcessGenerationData = false;
			initBuildGraph.Project = Project;
			initBuildGraph.Parent = Parent;
			initBuildGraph.Execute();

			System.Threading.Tasks.Parallel.ForEach(buildConfigs.ToArray(), config =>
			{
				// The only way to get NAntTask to execute DoInitialize() is when it is parsing XmlDocument().
				// So, we're going to execute NAntTask by creating the XmlDocument();
				// Create XmlElement for the followings:
				//		<nant buildfile="..." 
				//			target="load-package" 
				//			optionset="nant.commandline.properties" 
				//			global-properties-action="initialize" 
				//			start-new-build="false">
				//			<property name="config" value="${build-config-name}"/>
				//			<property name="package.configs" value="${eaconfig.build.configs}"/>
				//			<property name="eaconfig.build.autobuild-target" value="${eaconfig.build.autobuild-target}"/>
				//			<property name="eaconfig.build.group" value="${eaconfig.build.group}"/>
				//			<property name="eaconfig.build.group.names" value="${eaconfig.build.group.names}"/>
				//		</nant>
				config = config.Trim();
				if (string.IsNullOrEmpty(config))
				{
					return;
				}

				XmlDocument doc = new XmlDocument();
				XmlElement nantTaskXml = doc.CreateElement("nant");
				nantTaskXml.SetAttribute("buildfile", packageBuildFile);
				nantTaskXml.SetAttribute("target", buildAllTarget);
				nantTaskXml.SetAttribute("optionset", "nant.commandline.properties");
				nantTaskXml.SetAttribute("global-properties-action", "initialize");
				nantTaskXml.SetAttribute("start-new-build", "false");
				XmlElement configPropNode = doc.CreateElement("property");
				configPropNode.SetAttribute("name", "config");
				configPropNode.SetAttribute("value", config);
				nantTaskXml.AppendChild(configPropNode);
				XmlElement packageConfigsPropNode = doc.CreateElement("property");
				packageConfigsPropNode.SetAttribute("name", "package.configs");
				packageConfigsPropNode.SetAttribute("value", buildConfigs);
				nantTaskXml.AppendChild(packageConfigsPropNode);
				XmlElement targetPropNode = doc.CreateElement("property");
				targetPropNode.SetAttribute("name", "eaconfig.build.autobuild-target");
				targetPropNode.SetAttribute("value", autoBuildTarget);
				nantTaskXml.AppendChild(targetPropNode);
				XmlElement buildGrpPropNode = doc.CreateElement("property");
				buildGrpPropNode.SetAttribute("name", "eaconfig.build.group");
				buildGrpPropNode.SetAttribute("value", buildgroup);
				nantTaskXml.AppendChild(buildGrpPropNode);
				XmlElement buildGrpListPropNode = doc.CreateElement("property");
				buildGrpListPropNode.SetAttribute("name", "eaconfig.build.group.names");
				buildGrpListPropNode.SetAttribute("value", buildGroups);
				nantTaskXml.AppendChild(buildGrpListPropNode);

				NAntTask nantTask = new NAntTask();
				nantTask.Project = Project;
				nantTask.Initialize(nantTaskXml);
				nantTask.Execute();
			}
			);

			string oldPackageName = Project.Properties["package.name"];
			string oldPackageVersion = Project.Properties["package.version"];
			Project.Properties.UpdateReadOnly("package.name", packageName);
			Project.Properties.UpdateReadOnly("package.version", Project.Properties.GetPropertyOrFail("package."+packageName+".version"));

			Task_CreateBuildGraph createBuildGraph = new Task_CreateBuildGraph();
			createBuildGraph.BuildConfigurations = buildConfigs;
			createBuildGraph.BuildGroupNames = buildGroups;
			createBuildGraph.ProcessGenerationData = buildProcessGenerationData;
			createBuildGraph.Project = Project;
			createBuildGraph.Execute();

			// Restore current package name and version
			Project.Properties.UpdateReadOnly("package.name", oldPackageName);
			Project.Properties.UpdateReadOnly("package.version", oldPackageVersion);
		}
	}
}