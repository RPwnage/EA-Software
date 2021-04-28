// Originally based on NAnt - A .NET build tool
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

using System;
using System.Collections.Generic;
using System.Xml;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks
{

	/// <summary>
	/// Executes a task created by &lt;createtask&gt;. 
	/// </summary>
	/// <remarks>
	/// <para>Use &lt;createtask&gt; to create a new 'task' that is made of NAnt 
	/// script.  Then use the &lt;task&gt; task to call that task with the required parameters.</para>
	/// 
	/// <para>Each parameter may be either an attribute (eg. Message in example below) or a nested text element
	/// (eg. DefList in example below) that allows multiple lines of text.  Multiple lines may be needed for certain task elements like
	/// &lt;includedirs>, &lt;usingdirs>, and &lt;defines> in &lt;cc>.	</para>
	/// 
	/// <para>If &lt;createtask&gt; was not used to create a task with that name and there is a regular C# task with the same name it will execute that.
	/// This mechanism can be used to override a default task with custom behavior or convert a xml task to a C# task without breaking users build scripts.</para>
	/// </remarks>
	/// <include file='Examples/CreateTask/simple.example' path='example'/>
	[TaskName("task", StrictAttributeCheck = false, NestedElementsCheck = false)]
	public class RunTask : Task
	{
		XmlElement _codeElement;

		/// <summary>The name of the task being declared.</summary>
		[TaskAttribute("name", Required = true)]
		public string TaskName { get; set; }

		/// <summary>
		/// The parameters to be passed to &lt;createtask&gt;
		/// </summary>
		public OptionSet ParameterValues { get; } = new OptionSet();

		private bool IsUserTask = true;

		protected override void InitializeTask(XmlNode taskNode)
		{
			base.InitializeTask(taskNode);

			string declarationPropertyName = String.Format("Task.{0}", TaskName);
			string paramOptionSetName = String.Format("Task.{0}.Parameters", TaskName);

			string declaration = Project.Properties[declarationPropertyName];
			if (declaration == null)
			{
				// check if the taskname matches any regular task names
				if (Project.TaskFactory.Contains(TaskName))
				{
					// Just remove the name attribute and change the name of the task
					// CreateTask will handle any invalid attributes like it would for any other task
					XmlDocument doc = taskNode.OwnerDocument;
					_codeElement = doc.CreateElement(TaskName);

					// need to make a copy of the attribute list so that we don't modify the original
					// initialize.xml files are processed multiple times but store in the line info cache so if we modify the original we modify the cache
					XmlAttribute[] attributes = new XmlAttribute[taskNode.Attributes.Count];
					taskNode.Attributes.CopyTo(attributes, 0);

					foreach (XmlAttribute attribute in attributes)
					{
						if (attribute.Name == "name") continue;
						_codeElement.Attributes.Append(attribute);
					}
					foreach (XmlNode child in taskNode.ChildNodes)
					{
						XmlAttribute attribute = doc.CreateAttribute(child.Name);
						attribute.Value = child.InnerText.Trim();
						_codeElement.Attributes.Append(attribute);
					}

					IsUserTask = false;
					return;
				}

				string msg = String.Format("Missing property '{0}'.  Task '{1}' has not been declared.", declarationPropertyName, TaskName);
				throw new BuildException(msg);
			}

			OptionSet parameterTypes;
			if (!Project.NamedOptionSets.TryGetValue(paramOptionSetName, out parameterTypes))
			{
				string msg = String.Format("Missing option set '{0}'.  Task '{1}' has not been declared.", paramOptionSetName, TaskName);
				throw new BuildException(msg);
			}

			// look up parameter option set and set parameters for this instance
			foreach (KeyValuePair<string,string> entry in parameterTypes.Options)
			{
				string name = entry.Key;
				string value = entry.Value;

				// check if parameter is passed in the XmlAttributes
				XmlAttribute parameterAttribute = taskNode.Attributes[name];
				if (parameterAttribute != null)
				{
					value = parameterAttribute.Value;
				}
				else
				{
					// check if parameter is in child element
					bool found = false;
					foreach (XmlNode child in taskNode.ChildNodes)
					{
						if (child.Name == name)
						{
							value = child.InnerText.Trim();
							found = true;
						}
					}
					if (!found && value == "Required")
					{
						string msg = String.Format("'{0}' is a required attribute/element for task '{1}'.", name, TaskName);
						throw new BuildException(msg);
					}
				}

				ParameterValues.Options.Add(name, Project.ExpandProperties(value));
			}
			_codeElement = (XmlElement)Project.UserTasks[TaskName]; 
		}

		protected override void ExecuteTask()
		{
			Log.Debug.WriteLine(LogPrefix + "Running task '{0}'", TaskName);

			if (!IsUserTask)
			{
				Task task = Project.CreateTask(_codeElement, this);
				task.Execute();
			}
			else
			{
                Project taskContextProject = new Project(Project, Project.ScriptInitLock);
				{
					// set properties for each parameter
					foreach (KeyValuePair<string, string> entry in ParameterValues.Options)
					{
						string name = String.Format("{0}.{1}", TaskName, entry.Key);
						string value = entry.Value;

						if (taskContextProject.Properties[name] != null)
						{
							string msg = String.Format("Property '{0}' exists but should not.", name);
							throw new BuildException(msg);
						}
						taskContextProject.Properties.Add(name, value, local: true, inheritable: true);
					}

					// run tasks (taken from Target.cs in NAnt)
					try
					{
						foreach (XmlNode node in _codeElement)
						{
							if (node.NodeType == XmlNodeType.Element)
							{
								Task task = taskContextProject.CreateTask(node, this);
								task.Execute();
							}
						}
					}
					catch (ContextCarryingException e)
					{
						e.PushNAntStackFrame(TaskName, NAntStackScopeType.Task, Location);
						throw e; //throw e instead of throw to reset the stacktrace
					}
					catch (BuildException e)
					{
						throw new ContextCarryingException(e, Name, Location);
					}
				}
			}
		}
	}

	/// <summary>Create a task that is made of NAnt script and can be used by &lt;task&gt;.
	/// </summary>
	/// <remarks>
	/// 
	/// <para>Use the &lt;createtask&gt; to create a new 'task' that is made of NAnt 
	/// script.  Then use the &lt;task&gt; task to call that task with the required parameters.</para>
	/// 
	/// <para>The following unique properties will be created, for use by &lt;task>:</para>
	/// <list type="bullet">
	/// <item><b>Task.{name}</b></item>
	/// <item><b>Task.{name}.Code</b></item>
	/// </list>
	/// The following named optionset will be created, for use by &lt;task>:
	/// <list type="bullet">
	/// <item><b>Task.{name}.Parameters</b></item>
	/// </list>
	/// </remarks>
	/// <example>
	///     <code>
	/// &lt;createtask name="BasicTask">
	///    &lt;parameters>
	///        &lt;option name="DummyParam" value="Required"/>
	///        &lt;option name="Indentation"/>
	///    &lt;/parameters>
	///    &lt;code>
	///        &lt;echo message="${BasicTask.Indentation}Start BasicTask."/>
	///        &lt;echo message="${BasicTask.Indentation}    DummyParam value = ${BasicTask.DummyParam}"/>
	///        &lt;echo message="${BasicTask.Indentation}Finished BasicTask."/>
	///    &lt;/code>
	/// &lt;/createtask>
	///     </code>
	/// </example>
	/// <include file='Examples/CreateTask/simple.example' path='example'/>
	[TaskName("createtask", NestedElementsCheck = true)]
	public class CreateTaskTask : Task
	{

		/// <summary>The name of the task being declared.</summary>
		[TaskAttribute("name", Required = true)]
		public string TaskName { get; set; }

		/// <summary>Overload existing definition.</summary>
		[TaskAttribute("overload", Required = false)]
		public bool Overload { get; set; } = false;

		/// <summary>The set of task parameters.</summary>
		[OptionSet("parameters")]
		public OptionSet Parameters { get; } = new OptionSet();

		/// <summary>
		/// The NAnt script this task is made out from
		/// </summary>
		[Property("code")]
		public XmlPropertyElement Code { get; set; } = new XmlPropertyElement();

		protected override void InitializeTask(XmlNode taskNode)
		{
			// find the <Code> element and get all the XML without expanding any text
			var codeElement = taskNode.GetChildElementByName("code");
			if (codeElement == null)
			{
				throw new BuildException("Missing required <code> element.");
			}
			Project.UserTasks[TaskName] = codeElement;
		}


		protected override void ExecuteTask()
		{
			if (Log.DebugEnabled)
				Log.Debug.WriteLine(LogPrefix + "Adding task '{0}'", TaskName);

			string declarationPropertyName = String.Format("Task.{0}", TaskName);
			string paramOptionSetName = String.Format("Task.{0}.Parameters", TaskName);

			// ensure properties and option set don't exist
			var declarationLocation = Project.Properties[declarationPropertyName];
			if (declarationLocation != null)
			{
				if (!Overload)
				{
					string msg = String.Format("Task '{0}' already defined in {1}.", TaskName, declarationLocation);
					throw new BuildException(msg, Location);
				}
				else
				{
					Log.Debug.WriteLine("Task '{0}' defined at {1} is overloaded from {2}.", TaskName, declarationLocation, Location.ToString());
				}
			}


			if (Project.NamedOptionSets.Remove(paramOptionSetName))
			{
				if (!Overload)
				{
					string msg = String.Format("Option Set '{0}' exists.", paramOptionSetName);
					throw new BuildException(msg);
				}
			}

			// ensure Code XML is valid

			// create optionset for task parameter list
			Project.NamedOptionSets.Add(paramOptionSetName, Parameters);

			if (Overload && Project.Properties.Contains(declarationPropertyName))
			{
				Project.Properties.UpdateReadOnly(declarationPropertyName, Location.ToString() + "Task Overloaded");
				return;
			}

			// create property for task name
			Project.Properties.AddReadOnly(declarationPropertyName, Location.ToString() + "Task Declared");
		}
	}
}
