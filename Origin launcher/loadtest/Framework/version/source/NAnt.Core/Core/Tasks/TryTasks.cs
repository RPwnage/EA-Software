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
using System.Xml;
using System.Text.RegularExpressions;

using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks
{
	/// <summary>
	/// Allows wrapping of a group of tasks to be executed in try/catch/finally clauses. <b>This task is a Framework 2 
	/// feature.</b>
	/// </summary>
	/// <remarks>
	/// It uses format similar to Ant's: <a href="http://ant-contrib.sourceforge.net/tasks/trycatch.html">http://ant-contrib.sourceforge.net/tasks/trycatch.html</a>. It's a simple implementation of try/catch/finally/throw
	/// common in languages like C++ and C#.
	/// <para>
	/// A build exception will be thrown if:
	/// <list type="number">
	/// <item>&lt;trycatch&gt; doesn't have one &lt;try&gt;</item>
	/// <item>&lt;trycatch&gt; has more than one &lt;finally&gt;</item>
	/// <item>&lt;trycatch&gt; has tags other than &lt;try&gt;, &lt;catch&gt;, and &lt;finally&gt;</item>
	/// <item>&lt;catch&gt; and &lt;finally&gt; are both missing</item>
	/// <item>&lt;catch&gt; is ahead of &lt;try&gt;</item>
	/// <item>&lt;finally&gt; is ahead of &lt;try&gt; or &lt;catch&gt;</item>
	/// <item>&lt;throw&gt; appears in &lt;try&gt; or &lt;finally&gt;</item>
	/// </list>
	/// </para>
	/// <para>trycatch won't handle the above exception. Instead, it'll rethrow it, halting the build process,
	/// and allow you to correct the error. But for other exceptions, it'll catch them if it has a &lt;catch&gt;.
	/// </para>
	/// <para>
	/// When an exception is caught, &lt;trycatch&gt; will store the exception message in property 
	/// <i>trycatch.error</i> of the project, and remove the message when it goes out of the scope. It'll 
	/// rethrow the exception at the end of execution if &lt;catch&gt; has &lt;throw&gt;. As usual, tasks
	/// remained in &lt;catch&gt; will be ignored.
	/// </para>
	/// <para>
	/// &lt;trycatch&gt; can be nested within &lt;try&gt;, &lt;catch&gt;, and &lt;finally&gt;.
	/// </para>
	/// </remarks>
	/// <include file='Examples/Try/Simple.example' path='example'/>
	[TaskName("trycatch")]
	public class TryCatchTask : TaskContainer
	{
		protected ThrowTask _throwTask = null;

		public ThrowTask ThrowTask
		{
			set
			{
				_throwTask = value;
			}
		}

		[XmlElement("try", "NAnt.Core.Tasks.TryTask", AllowMultiple = true)]
		[XmlElement("catch", "NAnt.Core.Tasks.CatchTask", AllowMultiple = true)]
		[XmlElement("finally", "NAnt.Core.Tasks.FinallyTask", AllowMultiple = true)]
		protected override void ExecuteChildTasks()
		{
			Exception tryException = null;
			Location locTry = Location.UnknownLocation;
			Location locCatch = Location.UnknownLocation;
			Location locFinally = Location.UnknownLocation;

			foreach (XmlNode childNode in _xmlNode)
			{
				if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(_xmlNode.NamespaceURI))
				{

					if (IsPrivateXMLElement(childNode)) continue;

					Task task = CreateChildTask(childNode);
					// we should assume null tasks are because of incomplete metadata about the XML.
					if (task != null)
					{
						task.Initialize();
						Location loc = Location.GetLocationFromNode(childNode);

						if (task is TryTask)
						{
							if (locTry != Location.UnknownLocation)
							{
								throw new BuildException("<trycatch> can't have more than one <try>", loc);
							}
							locTry = loc;
							try
							{
								task.Execute();
							}
							catch (BuildException be)
							{
								if (be.Message.IndexOf("can't have <throw>") != -1)
								{
									throw;
								}
								else
								{
									tryException = be;
								}
							}
							catch (Exception ex)
							{
								tryException = ex;
							}
							if (tryException != null)
							{
								string tryCatchMessage = tryException.Message;
								// put the inner messages together
								Exception current = tryException.InnerException;
								while (current != null)
								{
									// We have container / pass through exceptions that doesn't have
									// messages.  But the real message is deep in the InnerExceptions.
									// So we need to go through all InnerExceptions.
									if (!String.IsNullOrEmpty(current.Message))
									{
										tryCatchMessage += Environment.NewLine;
										tryCatchMessage += current.Message;
									}
									current = current.InnerException;
								}
								Project.Properties["trycatch.error"] = tryCatchMessage;
								Project.Properties["trycatch.exceptiontype"] = (tryException is BuildException) ? ((BuildException)tryException).Type : tryException.GetType().ToString();
							}
						}
						else if (task is CatchTask)
						{
							if (locTry == Location.UnknownLocation)
							{
								throw new BuildException("A <try> is expected before this <catch>", loc);
							}
							else if (locFinally != Location.UnknownLocation)
							{
								if (locFinally.LineNumber < loc.LineNumber)
								{
									throw new BuildException("A <finally> is declared ahead of this <catch>", loc);
								}
							}
							locCatch = loc;
							// Run catch's body only if there is actually an exception whose type matched with the type specified in the catch task
							// and the exception has not been already rethrown.
							string exTypeName = String.Empty;
							if (tryException != null)
							{
								exTypeName = tryException is BuildException ? ((BuildException)tryException).Type : tryException.GetType().ToString();
							}

							if (tryException != null && Regex.IsMatch(exTypeName, ((CatchTask)task).Exception)
							  && (_throwTask == null || !_throwTask.IfDefined || _throwTask.UnlessDefined))
							{
								task.Execute();
								// Null out the tryException unless it's been rethrown
								if (_throwTask == null || !_throwTask.IfDefined || _throwTask.UnlessDefined)
								{
									// It's not quite logical, but some build scripts access trycatch.error in the finally statement.
									// for backwards compatibility do Remove after <finally>
									//Project.Properties.Remove("trycatch.error");
									//Project.Properties.Remove("trycatch.exceptiontype");

									tryException = null;
								}
							}
						}
						else if (task is FinallyTask)
						{
							if (locFinally != Location.UnknownLocation)
								throw new BuildException("<trycatch> can't have more than one <finally>", loc);
							else if (locTry == Location.UnknownLocation)
								throw new BuildException("A <try> is expected before this <finally>", loc);
							locFinally = loc;
							task.Execute();
						}
						else
						{
							throw new BuildException("<trycatch> can have only <try>, <catch>, or <finally>, but not <" + task.Name + "> at (" + loc.LineNumber + "," + loc.ColumnNumber + ")",
								Location);
						}
					}
				}
			}

			// Remove exception properties unless rethrown
			if (_throwTask == null || !_throwTask.IfDefined || _throwTask.UnlessDefined)
			{
				if (Project.Properties.Contains("trycatch.error"))
					Project.Properties.Remove("trycatch.error");
				if (Project.Properties.Contains("trycatch.exceptiontype"))
					Project.Properties.Remove("trycatch.exceptiontype");
			}

			if (locTry == Location.UnknownLocation &&
				(locCatch == Location.UnknownLocation || locFinally == Location.UnknownLocation))
			{
				throw new BuildException("<trycatch> must contain one <try> element and at least one of <catch> and <finally>", Location);
			}
			else if (locTry != Location.UnknownLocation &&
				locCatch == Location.UnknownLocation && locFinally == Location.UnknownLocation)
			{
				throw new BuildException("Missing <catch> and <finally> for <try>", Location);
			}
			if (tryException != null)
			{
				if (tryException is BuildException)
				{
					throw tryException;
				}
				else
				{
					throw new BuildException(tryException.Message, Location, tryException);
				}
			}
		}

		//Override with a single purpose to reset XmlTaskContainer attribute. 
		// Need this for schema and docs generation
		[XmlTaskContainer("-")]
		protected override Task CreateChildTask(XmlNode node)
		{
			return base.CreateChildTask(node);
		}
	}

	/// <summary>
	/// Allows wrapping of a group of tasks to be executed in a try clause. <b>This task can appear only inside trycatch task.</b>
	/// </summary>
	/// <include file='Examples/Try/Simple.example' path='example'/>
	[TaskName("try", XmlSchema=false)]
	public class TryTask : TaskContainer { }

	/// <summary>
	/// Allows wrapping of a group of tasks to be executed in a catch clause. <b>This task can appear only inside trycatch task.</b>
	/// </summary>
	/// <remarks>Also allows rethrowing a caught exception.</remarks>
	/// <include file='Examples/Try/Simple.example' path='example'/>
	[TaskName("catch", XmlSchema = false)]
	public class CatchTask : TaskContainer
	{
		/// <summary>The type of the exception to be caught.</summary>
		[TaskAttribute("exception", Required = false)]
		public string Exception { get; set; } = ".*";

		protected override void ExecuteChildTasks()
		{
			foreach (XmlNode childNode in _xmlNode)
			{
				if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(_xmlNode.NamespaceURI))
				{

					if (IsPrivateXMLElement(childNode)) continue;

					Task task = CreateChildTask(childNode);
					task.Execute();
					if (task is ThrowTask)
					{
						if (Parent is TryCatchTask parentAsTryCatch)
						{
							parentAsTryCatch.ThrowTask = (ThrowTask)task;
						}
						return;
					}
				}
			}
		}
	}

	/// <summary>
	/// Allows wrapping of a group of tasks to be executed in a finally clause. <b>This task can appear only inside trycatch task.</b>
	/// </summary>
	/// <include file='Examples/Try/Simple.example' path='example'/>
	[TaskName("finally", XmlSchema = false)]
	public class FinallyTask : TaskContainer { }

	/// <summary>
	/// Rethrows a caught exception. <b>This task can appear only inside catch task.</b>
	/// </summary>
	/// <include file='Examples/Try/Simple.example' path='example'/>
	[TaskName("throw")]
	public class ThrowTask : Task
	{
		protected override void ExecuteTask()
		{
			if (!(Parent is CatchTask))
				throw new BuildException("<" + (Parent as Task).Name + "> can't have <throw>, which must be used within <catch>.", Location);
		}
	}
}
