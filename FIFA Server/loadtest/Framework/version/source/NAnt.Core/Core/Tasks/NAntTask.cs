// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
//
//
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)
// Scott Hernandez (ScottHernandez@hotmail.com)

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

using NAnt.Core.PackageCore;

namespace NAnt.Core.Tasks
{
	/// <summary>Runs NAnt on a supplied build file.</summary>
	/// <remarks>
	///     <para>This task can be used to build subprojects which have their own full build files.  See the
	///     <see cref="DependsTask"/> for a good example on how to build sub projects only once per build.</para>
	/// </remarks>
	[TaskName("nant", NestedElementsCheck = false)]
	public class NAntTask : Task, IDisposable
	{
		[ElementName("OutProperty", StrictAttributeCheck = true)]
		public class OutProperty : ConditionalElement
		{
			/// <summary>Property name in which to store property from child invocation.</summary>
			[TaskAttribute("out-property-name", Required = true)]
			public string OutPropertyName { get; set; }

			/// <summary>Name of property in child invocation to store in out-property-name.</summary>
			[TaskAttribute("child-property-name", Required = true)]
			public string ChildPropertyName { get; set; }
		}

		[ElementName("property", StrictAttributeCheck = true)]
		public class Property : ConditionalElement
		{
			[TaskAttribute("name", Required = true)]
			public string PropertyName { get; set; }

			[TaskAttribute("value", Required = false)]
			public string Value { get; set; }
		}

		//private const int DefaultIndentLevel = 1;
		private const int DefaultIndentLevel = 0;  // In framework 3 with parallel processing and non recursive model does not make sense to use indent on NAnt task invocation.
		private int? _indentLevel = null;
		private Release _packageInfo = null;

		public enum GlobalPropertiesActionTypes { propagate, initialize };

		/// <summary>The name of the *.build file to use.</summary>
		[TaskAttribute("buildfile", Required = false)]
		public string BuildFileName { get; set; }

		/// <summary>The name of the package from which to use .build file.</summary>
		[TaskAttribute("buildpackage", Required = false)]
		public string BuildPackage { get; set; }

		/// <summary>The target to execute.  To specify more than one target separate targets with a space.  Targets are executed in order if possible.  Default to use target specified in the project's default attribute.</summary>
		[TaskAttribute("target")]
		public string DefaultTarget { get; set; } = null;

		/// <summary>The name of an optionset containing a set of properties to be passed into the supplied build file.</summary>
		[TaskAttribute("optionset")]
		public string OptionSetName { get; set; } = null;

		/// <summary>The log IndentLevel. Default is the current log IndentLevel + 1.</summary>
		[TaskAttribute("indent")]
		public int IndentLevel {
			get
			{
				if (_indentLevel == null)
				{
					return Log.IndentLevel + DefaultIndentLevel;
				}
				return (int)_indentLevel;
			}
			set { _indentLevel = value; }
		}

		/// <summary>
		/// Defines how global properties are set up in the dependent project. 
		/// Valid values are <b>propagate</b> and <b>initialize</b>. Default is 'propagate'.
		/// </summary>
		/// <remarks>
		/// <para>
		/// 'propagate' means standard global properties propagation.
		/// </para>
		/// <para>
		/// when value is 'initialize' global properties in dependent project are set using values from masterconfig and NAnt command line
		/// the same way they are set at the start of NAnt process. Usual used in combination with 'start-new-build'
		/// </para>
		/// </remarks>
		[TaskAttribute("global-properties-action", Required = false)]
		public GlobalPropertiesActionTypes GlobalPropertiesAction { get; set; } = GlobalPropertiesActionTypes.propagate;

		/// <summary>
		/// Start new build graph in the dependent project. Default is 'false'
		/// </summary>
		/// <remarks>
		/// Normally dependent project is added to the current build graph. Setting 'start-new-build' 
		/// to true will create new build graph in dependent project. This is useful when NAnt task is used to invoke separate independent build.
		/// </remarks>
		[TaskAttribute("start-new-build", Required = false)]
		public bool StartNewBuild { get; set; } = true;

		public bool TopLevel { get; set; } = true;

		public string PackageName
		{
			get { return _packageInfo == null ? String.Empty : _packageInfo.Name; }
		}

		public Project NestedProject { get; private set; } = null;

		public Dictionary<string, string> TaskProperties = new Dictionary<string, string>();

		private readonly List<OutProperty> _outProperties = new List<OutProperty>();

		private static int s_dependentProjectCount = 0;

		[XmlElement("property", "NAnt.Core.Tasks.PropertyTask", AllowMultiple = true, Description = "Property to pass to the child nant invocation."),
		XmlElement("out-property", "NAnt.Core.Tasks.OutProperty", AllowMultiple = true, Description = "Property to retrieve from child nant invocation.")]
		protected override void InitializeTask(XmlNode taskNode)
		{
			if (taskNode != null)
			{
				foreach (XmlNode childNode in taskNode)
				{
					if (childNode.Name.Equals("property"))
					{
						PropertyTask taskProp = Project.TaskFactory.CreateTask(childNode, Project) as PropertyTask;
						taskProp.Initialize(childNode);
						taskProp.ReadOnly = true;
						taskProp.Project = NestedProject;


						// Add current command line properties.
						if (taskProp.IfDefined && !taskProp.UnlessDefined)
						{
							TaskProperties[taskProp.PropertyName] = taskProp.Value;
						}
					}
					else if (childNode.Name.Equals("out-property"))
					{
						OutProperty outProperty = new OutProperty
						{
							Project = Project
						};
						outProperty.Initialize(childNode);
						if (outProperty.IfDefined && !outProperty.UnlessDefined)
						{
							_outProperties.Add(outProperty);
						}
					}
				}
			}

			DoInitialize();
		}

		public void DoInitialize()
		{
			BuildFileName = GetBuildFileName();

			// creating new log for several reasons:
			// - new id so different projects can be differentiate in output
			// - new level in case Verbose as set for this <nant> task
			// - we don't want to propagate any local warning / deprecation settings to new project
			Log nestedLog = new Log
			(
				level: (Verbose && Log.Level < Log.LogLevel.Diagnostic) ? Log.LogLevel.Diagnostic: Log.Level,
				warningLevel : Log.WarningLevel,
				warningAsErrorLevel: Log.WarningAsErrorLevel,
				deprecationLevel: Log.DeprecationLevel,
				indentLevel: Log.IndentLevel,
				id: (Interlocked.Increment(ref s_dependentProjectCount).ToString() + ">").PadRight(Log.IndentLength),
				listeners: Log.Listeners,
				errorListeners: Log.ErrorListeners,
				Log.DeveloperWarningEnabled
			);

			// when a new project context is created vuia nanttask it's "command line" properties are
			// considered to be:
			//	propagate: parent context's command line
			//	initialize: intial command line passes to nant.xe
			//  + anything that explicitly passed to project via <property> or optionset
			StringComparer keyComparer = PropertyKey.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase;
			Dictionary<string, string> newCommandLineProperties = new Dictionary<string, string>
			(
				GlobalPropertiesAction == GlobalPropertiesActionTypes.propagate ? Project.InitialCommandLineProperties : Project.CommandLineProperties,
				keyComparer
			);
			{
				// add from optionset
				if (OptionSetName != null)
				{
					string optSetName = OptionSetName;
					if (String.Compare(optSetName, Project.NANT_OPTIONSET_PROJECT_CMDPROPERTIES_LEGACY) == 0)
					{
						Project.Log.ThrowWarning
						(
							Log.WarningId.LegacyCommandPropertiesOptions,
							Log.WarnLevel.Minimal,
							"Automatically remapping legacy optionset name '{0}' to '{1}' in {2} task at {3}. This automatic redirection will be removed in future so please update to the correct name.",
							Project.NANT_OPTIONSET_PROJECT_CMDPROPERTIES_LEGACY, Project.NANT_OPTIONSET_PROJECT_CMDPROPERTIES, Name, Location
						);
						optSetName = Project.NANT_OPTIONSET_PROJECT_CMDPROPERTIES;
					}

					if (Project.NamedOptionSets.TryGetValue(optSetName, out OptionSet optionSet))
					{
						foreach (KeyValuePair<string, string> entry in optionSet.Options)
						{
							newCommandLineProperties[entry.Key] = entry.Value;
						}
					}
					else
					{
						throw new BuildException(String.Format("Unknown option set '{0}'.", OptionSetName), Location);
					}
				}

				// add from properties, these trump anything from optionset
				foreach (KeyValuePair<string, string> taskProp in TaskProperties)
				{
					newCommandLineProperties[taskProp.Key] = taskProp.Value;
				}
			}

			Project.SanitizeConfigProperties(Log, ref newCommandLineProperties);

			NestedProject = new Project
			(
				log: nestedLog,
				buildFile: BuildFileName,
				buildTargets : StringUtil.ToArray(DefaultTarget),
				parent: Project, 
				topLevel: TopLevel,
				commandLineProperties: newCommandLineProperties
			);

			if (GlobalPropertiesAction == GlobalPropertiesActionTypes.propagate)
			{
				// in propagate mode pass down any global properties that weren't specifically given
				// a new value by optionset or properties
				// - a side note: it might seem a bit odd that we're propagating properties into the 
				// child based on how the evaluate in the parent (Project) NOT how they evaluate in the
				// child (NestedProject). That's because it is odd and doesn't make a whole lot of sense
				// but Framework has done it this way for a long time and it's too risky to change now.
				// - users don't actually tend to notice this wierdness however because they call 
				// <package> at the top of their build files which re-evalates global properties in the
				// context of *that* project again so the properties tend to fall the way you expect
				// by the time you actually use them
				foreach (Project.GlobalPropertyDef gprop in Project.GlobalProperties.EvaluateExceptions(Project))
				{
					if (!newCommandLineProperties.ContainsKey(gprop.Name))
					{
						// pass the intial value as a default for propagation in case parent doesnt' set a value -
						// this can happen if the property is conditionally global in the masterconfig and only
						// evaluates in the context of the nested project
						PropagateProperty(gprop.Name, Project, gprop.InitialValue);
					}
				}
			}

			if (!StartNewBuild)
			{
				// Passing down the Global Named Objects
				lock (Project.GlobalNamedObjects)
				{
					foreach (Guid id in Project.GlobalNamedObjects)
					{
						if (Project.NamedObjects.TryGetValue(id, out object obj))
						{
							NestedProject.NamedObjects[id] = obj;
						}
					}
				}
			}

			//IMTODO: do I need to copy all user tasks from parent?
			foreach (KeyValuePair<string,XmlNode> ent in Project.UserTasks)
			{
				NestedProject.UserTasks[ent.Key]= ent.Value;
			}
		}

		protected override void ExecuteTask()
		{
            if (Log.InfoEnabled)
            {
                Log.Info.WriteLine(LogPrefix + BuildFileName + " " + DefaultTarget);
            }

			if (!NestedProject.Run()) 
			{
				throw new BuildException("Recursive build failed.");
			}
			else
			{
				foreach (OutProperty outprop in _outProperties)
				{
					Project.Properties[outprop.OutPropertyName] = NestedProject.Properties[outprop.ChildPropertyName];
				}
			}
		}

		// This function is the same as ExecuteTask() except not disposing the project after execution.
		// Used in dependent task.
		//IMTODO - would be nice to get rid of it and insert dependency into parent package inside child <package> task.
		public void ExecuteTaskNoDispose()
		{
			if (!NestedProject.Run())
			{
				throw new BuildException("Recursive build failed.", stackTrace: BuildException.StackTraceType.None);
			}
		}

		protected bool PropagateProperty(string name, Project project, string initialValue = null)
		{
			bool propagated = false;
			string val = project.Properties[name] ?? initialValue;
			if (val != null)
			{
				NestedProject.Properties.Add(name, val, readOnly: !Project.IsControlProperty(name));
				propagated = true;
			}

			return (propagated);
		}

		protected OptionSet PropagateOptionset(string name)
		{
			OptionSet propagatedSet = null;
			if (Project.NamedOptionSets.TryGetValue(name, out OptionSet value))
			{
				OptionSet optionSet = new OptionSet(value)
				{
					Project = NestedProject
				};
				propagatedSet = optionSet;
			}
			else
			{
				propagatedSet = new OptionSet()
				{
					Project = NestedProject
				};
			}
			NestedProject.NamedOptionSets[name] = propagatedSet;
			return propagatedSet;
		}

		private string GetBuildFileName()
		{
			// parameter name mapped to value and action to perform on that string to resolve build file
			Dictionary<string, Tuple<string, Func<string, string>>> buildFileOptions = new Dictionary<string, Tuple<string, Func<string, string>>>()
			{
				{ "buildfile", new Tuple<string, Func<string, string>>(BuildFileName, fileName => fileName) },
				{ "buildpackage", new Tuple<string, Func<string, string>>(BuildPackage, fileName => PackageMap.Instance.GetPackageBuildFileName(Project, fileName)) }
			};

			// make sure at least one option is specified
			IEnumerable<KeyValuePair<string, Tuple<string, Func<string, string>>>> setOptions = buildFileOptions.Where(buildFileOption => !String.IsNullOrEmpty(buildFileOption.Value.Item1));
			if (!setOptions.Any())
			{
				throw new BuildException
				(
					String.Format
					(
						"One of the parameters {0} or {1} must be specified.",
						String.Join(", ", buildFileOptions.Keys.Take(buildFileOptions.Count() - 1).Select(s => "'" + s + "'")),
						buildFileOptions.Keys.Last()
					),
					Location
				);
			}

			// make sure only one option is specified
			if (setOptions.Count() > 1)
			{
				throw new BuildException
				(
					String.Format
					(
						"Parameters {0} and {1} are mutually exclusive. Only one of these parameters can have value.",
						String.Join(", ", setOptions.Take(setOptions.Count() - 1).Select(kvp => String.Format("'{0}={1}'", kvp.Key, kvp.Value.Item1))),
						String.Format("'{0}={1}'", setOptions.Last().Key, setOptions.Last().Value.Item1)
					),
					Location
				);
			}

			// resolve build file name using single set option
			Tuple<string, Func<string, string>> singleSetOption = setOptions.First().Value;
			string optionValue = singleSetOption.Item1;
			Func<string, string> optionTransform = singleSetOption.Item2;
			BuildFileName = optionTransform(optionValue);

			return Project.GetFullPath(BuildFileName);
		}

        [Obsolete("NAntTask does not need to be disposed.")]
		public void Dispose()
		{
			Project.Log.ThrowDeprecation(Log.DeprecationId.NAntTaskDisposal, Log.DeprecateLevel.Normal, DeprecationUtil.GetCallSiteKey(), "{0} NAntTask does not need to be disposed.", DeprecationUtil.GetCallSiteLocation());
		}
	}
}
