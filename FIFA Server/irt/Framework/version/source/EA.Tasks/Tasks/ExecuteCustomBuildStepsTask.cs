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
using System.Linq;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Model;

namespace EA.FrameworkTasks
{
	/// <summary>Executes a named custom build step.</summary>
	/// <remarks>
	/// <para>
	/// Note: This task is intended for internal use within eaconfig to allow 
	/// certain build steps to be called at the beginning or end of specific
	/// eaconfig targets.
	/// </para>
	/// <para>
	/// Executes a custom build step which may either consist of NAnt tasks or
	/// batch/shell script commands.
	/// </para>
	/// <para>
	/// The custom build step is defined using the property 
	/// runtime.[module].vcproj.custom-build-tool.
	/// </para>
	/// </remarks>
	[TaskName("ExecuteCustomBuildSteps")]
	public class ExecuteCustomBuildSteps : Task
	{

		/// <summary>The name of the module whose build steps we want to execute.</summary>
		[TaskAttribute("groupname", Required = true)]
		public string GroupName { get; set; } = null;

		protected override void InitializeTask(XmlNode taskNode)
		{
			
		}

		protected string PropGroupName(string name)
		{
			return String.Format("{0}.{1}", GroupName, name);
		}

		protected string PropGroupValue(string name, string defVal = "")
		{
			string val = null;
			if(!String.IsNullOrEmpty(name))
			{
				val = Project.Properties[PropGroupName(name)];
			}
			return val ?? defVal;
		}

		private FileSet PropGroupFileSet(string name)
		{
			if (String.IsNullOrEmpty(name))
			{
				return null;
			}
			return Project.NamedFileSets[PropGroupName(name)];
		}

		protected override void ExecuteTask()
		{
			string customcmdline = PropGroupValue("vcproj.custom-build-tool");
			if (!String.IsNullOrWhiteSpace(customcmdline))
			{
				BuildStep step = new BuildStep("custombuild", BuildStep.None);
				step.Commands.Add(new Command(customcmdline));
				step.OutputDependencies = PropGroupValue("vcproj.custom-build-outputs").LinesToArray().Select(file => PathString.MakeNormalized(file)).OrderedDistinct().ToList();
				step.InputDependencies = PropGroupValue("vcproj.custom-build-dependencies").LinesToArray().Select(file => PathString.MakeNormalized(file)).OrderedDistinct().ToList();
				var inputs = PropGroupFileSet("vcproj.custom-build-dependencies");
				if (inputs != null)
				{
					step.InputDependencies.AddRange(inputs.FileItems.Select(fi => fi.Path));
				}

				Project.ExecuteBuildStep(step, moduleName: GroupName);
			}
		}
	}
}

