// For ProfileStartTask and ProfileEndTask
// 1. Profile tasks are (build) file-based, not project-based
// 2. It needs a stack for profile stack tasks
// 3. It needs a log file shared by profile tasks
// 4. The log file must be of eacore profiler format
// 5. TaskName of profile start task is unique in a file
// 6. In the log file, profile data are identified by full path of the file and name of profile start task

// Copyright (C) Electronic Arts Canada Inc. 2002.  All rights reserved.

using System;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Shared.Properties;

using System.Collections;
using System.Collections.Specialized;
using NAnt.Core.Events;
using System.Xml;

namespace EA.FrameworkTasks 
{
	/// <summary>	
	/// Mark the start of a tasks profiling. <b>This task is a Framework 2 feature.</b>
	/// </summary>
	/// <remarks>
	/// <para>
	/// Framework 2 supports profiling, both at target level and task level, and introduces a new property,
	/// <i>profile</i>. It outputs profiling data in the eacore profiler format.
	/// </para>
	/// <para>
	/// The usage of the new property is: "nant -D:profile=value". Valid values are:
	/// <list type="bullet">
	///  <item>on, for task-level profiling</item>
	///  <item>targets, for target-level profiling</item>
	///  <item>all, for both levels</item>
	///  <item>off, for no profiling. This is the default value.</item>
	/// </list>
	/// </para>
	/// <para>
	/// To do task-level profiling, you use a pair of new tags, &lt;profilestart&gt;, and &lt;profileend&gt;,
	/// and put them around the tasks you want to profile. The former takes one required option, name, to name
	/// the tag uniquely in the build file. It must be matched with &lt;profileend&gt;. You don't need to use 
	/// any tag to do target-level profiling. Targets run by NAnt will be profiled.
	/// </para>
	/// <para>
	/// Framework 2 includes a file called profiler.mdb in its distribution. It's just an MS Access file, with
	/// a couple of data tables, and among them, MarkerTable. When NAnt does profiling for the 1st time, it copies
	/// this file to the folder of the root package. When profiling is done, you can see data for a given target or
	/// &lt;profilestart&gt; in MarkerTable. Take target "build" for example, the marker name will be: /build/target/<i>Location</i>,
	/// where "build" is the name of the target, "target" the name of the tag, and <i>Location</i> a string
	/// like "C:\packages\MyGame\0.0.0\source\AI\AI.build(2,6)".
	/// </para>
	/// <para><b>WARNING:</b> Keep in mind that profiler.mdb must be closed before profiling.</para>
	/// </remarks>
	/// <include file='Examples/Profiling/Profiling.example' path='example'/>
	/// <include file='Examples/Profiling/Profiling.example' path='example/code[@file="masterconfig.xml"]/*'/>
	[TaskName("profilestart")]
	public class ProfileStartTask : Task 
	{
		string	_name;
		DateTime _startTime;

		/// <summary>The name to identify a &lt;profilestart&gt; tag. Must be unique in a build file.</summary>
		[TaskAttribute("name", Required=true)]
		[StringValidator(Trim=true)]
		public string TaskName 
		{
			get { return _name; }
			set { _name = value; }
		}

		public DateTime StartTime
		{
			get { return _startTime; }
		}

		protected override void ExecuteTask() 
		{
			if (Project.Properties["package.frameworkversion"] == "1")
			{
				throw new BuildException("Calling <profilestart> task within a Framework 1.x package.  This is a Framework 2.x feature.", Location);
			}

			if (ProfileLogger.ShouldProfile(false))
			{
				_startTime = DateTime.Now;
				ProfilerStack.Instance.Push(Location.FileName, this);
			}
		}
	}

	/// <summary>
	/// Marks the end of a task profiling. Must match a &lt;profilestart&gt;. <b>This task is a Framework 2 feature.</b>
	/// </summary>
	[TaskName("profileend")]
	public class ProfileEndTask : Task 
	{
		protected override void ExecuteTask() 
		{
			if (Project.Properties["package.frameworkversion"] == "1")
			{
				throw new BuildException("Calling <profileend> task within a Framework 1.x package.  This is a Framework 2.x feature.", Location);
			}

			if (ProfileLogger.ShouldProfile(false))
			{
				// 1. Pop ProfileStartTask off stack
				ProfileStartTask task = ProfilerStack.Instance.Pop(Location.FileName, Location);
				if (task != null)
				{
					// 2. Calc execute time and write it to log file 
					ProfileLogger.Log("profilestart", task.TaskName, task.Location, task.StartTime, DateTime.Now);
				}
				else
					throw new BuildException("Error: Unmatched <profileend>", Location);
			}
		}
	}

	/// <summary>
	/// Singleton class to hold stack of instances of ProfileStartTask
	/// </summary>
	class ProfilerStack
	{
		class NameStack
		{
			internal string FileName = null;
			internal Stack Stack = null;
		}

		static ProfilerStack _instance = new ProfilerStack();
		ArrayList _stacks = new ArrayList();

		protected ProfilerStack()
		{
		}

		// Check if the build file has matching/duplicate profile tags
		bool CheckProfileTags(string fileName, Project project)
		{
			XmlDocument xmlDoc = new XmlDocument();
			xmlDoc.Load(fileName);
			XmlNodeList nodes = xmlDoc.SelectNodes("//profilestart | //profileend");

			Stack tmpStack = new Stack();
			StringCollection names = new StringCollection();
			foreach (XmlNode node in nodes)
			{
				if (node.Name == "profilestart")
				{
					// Check for duplicate
					string name = node.Attributes["name"].Value;
					if (!names.Contains(name))
						names.Add(name);
					else
					{
						Location location = Location.GetLocationFromNode(node);
						throw new BuildException("Duplicate <profilestart name='" + name + "'>", location);
					}
					tmpStack.Push(node);
				}
				else
				{
					// Check for empty stack
					if (tmpStack.Count == 0)
					{
						Location location = Location.GetLocationFromNode(node);
						throw new BuildException("Error: Unmatched <profileend>", location);
					}
					tmpStack.Pop();
				}
			}
			if (tmpStack.Count == 0)
				return true;
			while (tmpStack.Count > 0)
			{
				XmlNode node = (XmlNode)tmpStack.Pop();
				Location location = Location.GetLocationFromNode(node);
				throw new BuildException("Unmatched <profilestart>", location);
			}
			return false;
		}

		public void Push(string fileName, ProfileStartTask task)
		{
			NameStack stack = FindStack(fileName);
			if (stack == null)
			{
				// If it's the 1st time to handle fileName, check stacks
				if (!CheckProfileTags(fileName, task.Project))
					return;
				stack = new NameStack();
				stack.FileName = fileName;
				stack.Stack = new Stack();
				_stacks.Add(stack);
			}
			stack.Stack.Push(task);
		}

		public ProfileStartTask Pop(string fileName, Location location)
		{
			NameStack item = FindStack(fileName);
			if (item == null || item.Stack.Count == 0)
				return null;

			ProfileStartTask task = (ProfileStartTask)item.Stack.Pop();
			if (item.Stack.Count == 0)
				_stacks.Remove(item);
			return task;
		}

		NameStack FindStack(string fileName)
		{
			foreach (NameStack ns in _stacks)
			{
				if (ns.FileName == fileName)
					return ns;
			}
			return null;
		}

		static public ProfilerStack Instance
		{
			get { return _instance; }
		}
	}
}