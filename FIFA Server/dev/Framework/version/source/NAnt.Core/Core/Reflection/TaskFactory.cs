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
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Threading;
using NAnt.Core.Util;

namespace NAnt.Core.Reflection
{
	public sealed class TaskFactory
	{
		private readonly ConcurrentDictionary<string, TaskBuilder> m_taskBuilders;

		private static readonly Lazy<TaskFactory> s_globalTaskFactory = new Lazy<TaskFactory>(() => new TaskFactory(AssemblyLoader.WellKnownAssemblies));

		internal TaskFactory()
		{
			m_taskBuilders = new ConcurrentDictionary<string, TaskBuilder>(s_globalTaskFactory.Value.m_taskBuilders);
		}

		internal TaskFactory(TaskFactory other)
		{
			m_taskBuilders = new ConcurrentDictionary<string, TaskBuilder>(other.m_taskBuilders);
		}

		// constructor for the global task factory
		private TaskFactory(IEnumerable<Assembly> assemblies)
		{
			m_taskBuilders = new ConcurrentDictionary<string, TaskBuilder>();
			foreach (Assembly wellKnownAssembly in assemblies)
			{
				AddTasks(wellKnownAssembly);
			}
		}

		/// <summary> Creates a new Task instance for the given XML and project.</summary>
		/// <param name="taskNode">The XML to initialize the task with.</param>
		/// <param name="project">The Project that the Task belongs to.</param>
		/// <returns>The Task instance.</returns>
		public Task CreateTask(XmlNode taskNode, Project project)
		{
			if (m_taskBuilders.TryGetValue(taskNode.Name, out TaskBuilder builder))
			{
				Task task = builder.CreateTask();
				task.Project = project;
				return task;
			}
			throw new BuildException(String.Format("Unknown task <{0}>", taskNode.Name), Location.GetLocationFromNode(taskNode));
		}

		/// <summary> Creates a new Task instance for the given task name and project.</summary>
		/// <param name="taskName">The task name to initialize the task with.</param>
		/// <param name="project">The Project that the Task belongs to.</param>
		/// <returns>The Task instance, or null if not found.</returns>
		public Task CreateTask(string taskName, Project project)
		{
			if (m_taskBuilders.TryGetValue(taskName, out TaskBuilder builder))
			{
				Task task = builder.CreateTask();
				task.Project = project;
				return task;
			}
			return null;
		}

		public bool Contains(string key)
		{
			return m_taskBuilders.ContainsKey(key);
		}

		internal int AddTasks(Assembly assembly)
		{
			return AddTasks(assembly, false, false, "unknown assembly location");
		}

		internal int AddTasks(Assembly assembly, bool overrideExistingTask, bool failOnError, string assemblyName)
		{
			try
			{
				int taskCount = 0;
				NAntUtil.ParallelForEach
				(
					assembly.GetTypes(),
					type =>
					{
						if (type.IsSubclassOf(typeof(Task)) && !type.IsAbstract && (Attribute.GetCustomAttribute(type, typeof(TaskNameAttribute)) is TaskNameAttribute attribute))
						{
							TaskBuilder taskBuilder = new TaskBuilder(type, attribute.Name);
							if (AddBuilder(taskBuilder, (overrideExistingTask || attribute.Override), failOnError, out TaskBuilder oldBuilder))
							{
								taskCount++;
							}
						}
					},
					noParallelOnMono: true // slower to run parallel last time this was profiled on mono
				);			return taskCount;
			}
			catch (Exception e)
			{
				Exception ex = ThreadUtil.ProcessAggregateException(e);

				string assemblyLocation = String.IsNullOrEmpty(assembly.Location) ? assemblyName : assembly.Location;
				string msg = String.Format("Could not load tasks from '{0}'. {1}", assemblyLocation, ex.Message);
				if (ex is ReflectionTypeLoadException)
				{
					msg = String.Format("Could not load tasks from '{0}'. Reasons:", assemblyLocation);
					foreach (string reason in (ex as ReflectionTypeLoadException).LoaderExceptions.Select(le => le.Message).OrderedDistinct())
					{
						msg += Environment.NewLine + "\t" + reason;
					}

					ex = null; // We retrieved messages from inner exception, no need to add it.
				}
				throw new BuildException(msg, ex);
			}
		}

		private bool AddBuilder(TaskBuilder builder, bool overrideExistingBuilder, bool failOnError, out TaskBuilder oldbuilder)
		{
			bool ret = false;
			oldbuilder = null;
			if (overrideExistingBuilder)
			{
				TaskBuilder tmp = null;
				m_taskBuilders.AddOrUpdate(builder.TaskName, builder, (key, ob) => { tmp = ob; return builder; });
				oldbuilder = tmp;
				ret = true;
			}
			else
			{
				if (m_taskBuilders.TryAdd(builder.TaskName, builder))
				{
					ret = true;
				}
				else if (failOnError)
				{
					throw new BuildException("Task '" + builder.TaskName + "' already exists!");
				}
			}
			return ret;
		}
	}
}