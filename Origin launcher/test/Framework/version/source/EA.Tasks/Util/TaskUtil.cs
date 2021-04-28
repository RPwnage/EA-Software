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
using System.Xml;

using NAnt.Core;
using NAnt.Core.Tasks;

using EA.FrameworkTasks;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig
{
	public class TaskUtil
	{
		public static string Dependent(Project project, string name, Project.TargetStyleType? targetStyle = null, bool initialize = true, string target = null, bool dropCircular=false, List<ModuleDependencyConstraints> constraints = null, Location location = null, bool nonBlockingAutoBuildClean = false)
		{
			DependentTask dependentTask = new DependentTask
			{
				Project = project,
				Verbose = project.Verbose,
				TargetStyle = targetStyle ?? project.TargetStyle,
				Target = target,
				PackageName = name,
				InitializeScript = initialize,
				DropCircular = dropCircular,
				Constraints = constraints,
				NonBlockingAutoBuildclean = nonBlockingAutoBuildClean
			};
			// dependentTask.Location = DeprecationUtil.GetCallSiteLocation(location); // disabled - this is super slow // DAVE-FUTURE-REFACTOR-TODO: this is handy outside of deprecations should maybe move to a better named class, also be nice to make this automatic on Tasks?
			dependentTask.Execute();
			return dependentTask.PackageVersion;
		}

		// DAVE-FUTURE-REFACTOR-TODO - why does an overload that take a configuration exist? would this ever make sense?
		public static string Dependent(Project project, string name, string configuration, Project.TargetStyleType? targetStyle = null, bool initialize = true, string target = null, bool dropCircular = false, List<ModuleDependencyConstraints> constraints = null, bool propagatedLinkDependency = false, Location location = null, bool nonBlockingAutoBuildClean = false)
		{
			//IMTODO: dependent task with configuration.
			DependentTask dependentTask = new DependentTask
			{
				Project = project,
				Verbose = project.Verbose,
				TargetStyle = targetStyle ?? project.TargetStyle,
				Target = target,
				PackageName = name,
				InitializeScript = initialize,
				DropCircular = dropCircular,
				Constraints = constraints,
				PropagatedLinkDependency = propagatedLinkDependency,
				NonBlockingAutoBuildclean = nonBlockingAutoBuildClean
			};
			// dependentTask.Location = DeprecationUtil.GetCallSiteLocation(location); // disabled - this is super slow // DAVE-FUTURE-REFACTOR-TODO: this is handy outside of deprecations should maybe move to a better named class, also be nice to make this automatic on Tasks?
			dependentTask.Execute();
			return dependentTask.PackageVersion;
		}

		public static void ExecuteScriptTask(Project project, string name, string paramName, string paramValue)
		{
			IDictionary<string, string> parameters = new Dictionary<string, string>() { { paramName, paramValue } };

			ExecuteScriptTask(project, name, parameters);
		}

		public static void ExecuteScriptTask(Project project, string name, IDictionary<string, string> parameters)
		{
			EaconfigRunTask task = new EaconfigRunTask(project, name, parameters);

			task.Init();

			task.Execute();
		}

		// Helper class for RunScriptTask:
		internal class EaconfigRunTask : RunTask
		{
			internal EaconfigRunTask(Project project, string name, IDictionary<string, string> parameters)
			{
				Project = project;
				TaskName = name;
				_parameters = parameters;
			}

			internal void Init()
			{
				string declarationPropertyName = String.Format("Task.{0}", TaskName);
				string paramOptionSetName = String.Format("Task.{0}.Parameters", TaskName);

				string declaration = Project.Properties[declarationPropertyName];
				if (declaration == null)
				{
					string msg = String.Format("Missing property '{0}'. Task '{1}' has not been declared.", declarationPropertyName, TaskName);
					throw new BuildException(msg);
				}

				OptionSet parameterTypes = Project.NamedOptionSets[paramOptionSetName];
				if (parameterTypes == null)
				{
					string msg = String.Format("Missing option set '{0}'. Task '{1}' has not been declared.", paramOptionSetName, TaskName);
					throw new BuildException(msg);
				}

				// look up parameter option set and set parameters for this instance
				foreach (KeyValuePair<string, string> entry in parameterTypes.Options)
				{
					string name = (string)(entry.Key);
					string value = (string)(entry.Value);

					// check if parameter is passed in the input:
					string parValue;
					if (_parameters.TryGetValue(name, out parValue))
					{
						value = parValue;
					}
					else if (value == "Required")
					{
						string msg = String.Format("'{0}' is a required attribute/element for task '{1}'.", name, TaskName);
						throw new BuildException(msg);
					}

					ParameterValues.Options.Add(name, Project.ExpandProperties(value));
				}
			}

			protected override void ExecuteTask()
			{
				var _codeElement = Project.UserTasks[TaskName];

				Project.Log.Info.WriteLine(Project.LogPrefix + "Running task '{0}'", TaskName);

				// set properties for each parameter
				foreach (KeyValuePair<string, string> entry in ParameterValues.Options)
				{
					string name = String.Format("{0}.{1}", TaskName, entry.Key);
					string value = (string)(entry.Value);

					if (Project.Properties[name] != null)
					{
						string msg = String.Format("Property '{0}' exists but should not.", name);
						throw new BuildException(msg);
					}
					Project.Properties.Add(name, value);
				}

				try
				{
					try
					{
						// run tasks (taken from Target.cs in NAnt)
						foreach (XmlNode node in _codeElement)
						{
							if (node.NodeType == XmlNodeType.Element)
							{
								Task task = Project.CreateTask(node, this);
								task.Execute();
							}
						}
					}
					catch (BuildException e)
					{
						throw new BuildException("", Location.UnknownLocation, e);
					}
				}
				finally
				{
					// remove properties for each parameter
					foreach (KeyValuePair<string, string> entry in ParameterValues.Options)
					{
						string name = String.Format("{0}.{1}", TaskName, entry.Key);
						if (!Project.Properties.Contains(name))
						{
							string msg = String.Format("Property '{0}' does not exist but should.", name);
							throw new BuildException(msg);
						}
						Project.Properties.Remove(name);
					}
				}
			}

			private IDictionary<string, string> _parameters;
		}
	}

}


