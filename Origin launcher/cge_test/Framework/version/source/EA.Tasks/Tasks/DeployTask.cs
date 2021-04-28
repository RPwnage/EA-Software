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
using NAnt.Core.Util;
using NAnt.Core.Attributes;

using System.Collections.Generic;

namespace EA.Eaconfig
{
	[TaskName("DeployTask", NestedElementsCheck = true)]
	public class DeployTask : Task
	{
		protected static string DEFAULT_FILESET_NAME = "deploy-contents-fileset";
		protected static string DEFAULT_DEPLOY_TASK_NAME = "DeployPath";
		protected static string TOGGLE_PROPERTY_NAME = "deploy-contents";

		[TaskAttribute("groupname", Required = true)]
		public string GroupName { get; set; }

		[TaskAttribute("buildmodule", Required = true)]
		public string BuildModule { get; set; }

		[TaskAttribute("force", Required = false)]
		public bool Force { get; set; }

		public DeployTask()
		{

		}
		
		public DeployTask(string groupName, string buildModule)
		{
			GroupName = groupName;
			BuildModule = buildModule;
		}


		protected override void ExecuteTask()
		{
			// Check if deployment is enabled.
			bool deploymentEnabled = StringUtil.GetBooleanValue(PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + TOGGLE_PROPERTY_NAME, "true"));

			// If a local toggle property exists, then it overrides the eaconfig property.
			// Else, use the eaconfig property.
			deploymentEnabled = StringUtil.GetBooleanValue(PropertyUtil.GetPropertyOrDefault(Project, Project.ProjectName + "." + TOGGLE_PROPERTY_NAME, deploymentEnabled.ToString()));

			// Force always overrides everything
			if (Force)
				deploymentEnabled = true;
			
			if (deploymentEnabled)
			{
				CallDeployImplementation();
			}
		}


		protected void CallDeployImplementation()
		{
			string completeFilesetName = GroupName + "." + DEFAULT_FILESET_NAME;

			if (FileSetUtil.FileSetExists(Project, completeFilesetName))
			{
				FileSet deployContents = FileSetUtil.GetFileSet(Project, completeFilesetName);

				foreach (FileSetItem fileSetItem in deployContents.Includes)
				{
					string finalPattern = fileSetItem.Pattern.Value;

					if (!IsValidPattern(finalPattern))
					{
						throw new BuildException("The file pattern is invalid. If double wild cards exist, they must be at the end of the file path:\n" + finalPattern, Location);
					}

					bool isRecrusive = IsRecursive(finalPattern);

					finalPattern = FixRecursivePattern(finalPattern);

					var taskParameters = new Dictionary<string, string>() {
						{"BuildModule", BuildModule},
						{"GroupName", GroupName},
						{"Path", finalPattern},
						{"IsRecursive", isRecrusive.ToString()}};

					// The task is defined if this property exists
					string taskDeclaration = "Task." + DEFAULT_DEPLOY_TASK_NAME;
					if (PropertyUtil.PropertyExists(Project, taskDeclaration))
					{
						TaskUtil.ExecuteScriptTask(Project, DEFAULT_DEPLOY_TASK_NAME, taskParameters);
					}
				}


			}
		}


		protected bool IsRecursive(string pattern)
		{
			// We only support one special pattern, the "**" wild card. This simply means that
			// we should recursively go through the following directories.

			return pattern.EndsWith("**");
		}


		protected bool IsValidPattern(string pattern)
		{
			// The pattern is valid only if any "**" or "*" are the last parts of the string
			return (pattern.IndexOf("*") == pattern.LastIndexOf("*")) ||
				(pattern.IndexOf("**") == pattern.LastIndexOf("**"));
		}

		protected string FixRecursivePattern(string pattern)
		{
			/*
			 * If the path ends with **, then it is recursive.
			 * If the path ends with *, then it means just copy the files in the current directory.
			 * Either way, the directory  name must end with a directory separator
			 */
			if (pattern.EndsWith("**"))
				return pattern.Substring(0, pattern.Length - 2);
			else if (pattern.EndsWith("*"))
				return pattern.Substring(0, pattern.Length - 1);
			else
				return pattern;
		}
	}
}
