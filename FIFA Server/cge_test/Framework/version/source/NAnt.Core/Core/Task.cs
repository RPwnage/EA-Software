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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Concurrent;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Metrics;
using NAnt.Core.Reflection;
using NAnt.Core.Util;

namespace NAnt.Core
{
	public abstract class Task : Element
	{
		// using reflection to get the correct type name, is slow so have a map to cache automatic type names
		private static ConcurrentDictionary<Type, string> s_typesToTaskNames = new ConcurrentDictionary<Type, string>();

		public Task()
		{
			// if task is constructed without name use reflection to get a name (mainly for metrics reporting), cache result
			// due to expense of reflection
			_elementName = s_typesToTaskNames.GetOrAdd(GetType(), (key) =>
			{
				TaskNameAttribute taskNameAttribute = Attribute.GetCustomAttribute(GetType(), typeof(TaskNameAttribute)) as TaskNameAttribute;
				if (taskNameAttribute != null)
				{
					return taskNameAttribute.Name;
				}
				else
				{
					return "(Anonymous) " + GetType().Name;
				}
			});
		}

		public Task(string name) : base(name) { }

		/// <summary>Determines if task failure stops the build, or is just reported. Default is "true".</summary>
		[TaskAttribute("failonerror")]
		public virtual bool FailOnError { get; set; } = true;

		/// <summary>Task reports detailed build log messages.  Default is "false".</summary>
		[TaskAttribute("verbose")]
		public virtual bool Verbose
		{
			get { return (_verbose || Project.Verbose); }
			set { _verbose = value; }
		} bool _verbose = false;

		/// <summary>If true then the task will be executed; otherwise skipped. Default is "true".</summary>
		[TaskAttribute("if")]
		public virtual bool IfDefined { get; set; } = true;

		/// <summary>Opposite of if.  If false then the task will be executed; otherwise skipped. Default is "false".</summary>
		[TaskAttribute("unless")]
		public virtual bool UnlessDefined { get; set; } = false;

		/// <summary>Returns true if the task succeeded.</summary>
		public bool TaskSuccess { get; set; } = true;

		protected override bool InitializeChildElements { get { return IfDefined && !UnlessDefined; } }

		private string _logPrefix;
		/// <summary>The prefix used when sending messages to the log.</summary>
		public virtual string LogPrefix
		{
			get
			{
				if (_logPrefix != null)
				{
					return _logPrefix;
				}
				_logPrefix = " [" + Name + "] ";
				return _logPrefix;
			}
		}

		/// <summary>Executes the task unless it is skipped.</summary>
		public void Execute()
		{
			if (IfDefined && !UnlessDefined)
			{
				try
				{
					// initialize error conditions
					TaskSuccess = true;

					ExecuteTask();
				}
				catch (BuildException e)
				{
					// if the exception was anything lower than a BuildException than its an internal error
					bool continueOnFail = ConvertUtil.ToBoolean(Project.Properties.ExpandProperties(Project.Properties[Project.NANT_PROPERTY_CONTINUEONFAIL]));

					// update error conditions
					TaskSuccess = false;
					Project.LastError = e;

					if (FailOnError && !continueOnFail)
					{
						// If continue on fail is false we throw, otherwise we have to go on
						throw new ContextCarryingException(e, Name, Location);
					}
					else
					{
						// If local FailOnError is false, we just print the message.
						// If continueOnFail is true and we are here and need to also set the global success flag.
						if (continueOnFail)
						{
							Project.GlobalSuccess = false;
						}
						Log.Error.WriteLine(e.Message);
						if (e.InnerException != null)
						{
							Log.Error.WriteLine(e.InnerException.Message);
						}
					}
				}
				catch (Exception e)
				{
					bool continueOnFail = ConvertUtil.ToBoolean(Project.Properties.ExpandProperties(Project.Properties[Project.NANT_PROPERTY_CONTINUEONFAIL]));

					if (FailOnError && !continueOnFail)
					{
						throw new ContextCarryingException(e, Name, Location);
					}
				}
				finally
				{
					// Updating this property has huge performance hit on Mono. Do not update it.
					//Project.Properties.UpdateReadOnly(TaskSuccessProperty, TaskSuccess.ToString());
				}
			}
		}

		protected override void InitializeElement(XmlNode elementNode)
		{
			if (IfDefined && !UnlessDefined)
			{
				// Just defer for now so that everything just works
				InitializeTask(elementNode);
			}
		}

		/// <summary>Initializes the task and checks for correctness.</summary>
		protected virtual void InitializeTask(XmlNode taskNode)
		{
		}

		protected bool OptimizedTaskElementInit(string name, string value)
		{
			bool ret = true;
			switch (name)
			{
				case "failonerror": // REq
					FailOnError = ElementInitHelper.InitBoolAttribute(this, name, value);
					break;
				case "verbose":  // ExpandProperties = false
					_verbose = ElementInitHelper.InitBoolAttribute(this, name, value);
					break;
				case "if":
					IfDefined = ElementInitHelper.InitBoolAttribute(this, name, value);
					break;
				case "unless":
					UnlessDefined = ElementInitHelper.InitBoolAttribute(this, name, value);
					break;
				default:
					ret = false;
					break;
			}
			return ret;
		}

		// used to walk up the task chain to find a specific type
		// useful for cases where the internal type might be wrapped in conditionals
		// e.g in
		// 
		// <Parent>
		//		<Child/>
		//		<do>
		//			<Child/>
		//		</do>
		// <Parent>
		// 
		// FubdParentTask<Parent>() will find the enclosing parent from both child tasks
		protected T FindParentTask<T>() where T : Task
		{
			Task test = Parent as Task;

			while (test != null)
			{
				if (test is T result)
				{
					return result;
				}
				test = test.Parent as Task;
			}
			return null;
		}

		/// <summary>Executes the task.</summary>
		protected abstract void ExecuteTask();
	}
}
