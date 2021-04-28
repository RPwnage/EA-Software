// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2002 Gerry Shaw
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

using System;
using System.IO;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.Tasks {

	/// <summary>Writes a message to the build log.</summary>
	/// <remarks>
	///   <para>A copy of the message will be sent to every defined
	/// build log and logger on the system.  Property references in the message will be expanded.</para>
	/// </remarks>
	/// <include file='Examples/Echo/HelloWorld.example' path='example'/>
	/// <include file='Examples/Echo/Macro.example' path='example'/>
	/// <include file='Examples/Echo/FileAppend.example' path='example'/>
	[TaskName("echo", Mixed = true)]
	public class EchoTask : Task
	{
		/// <summary>The message to display.  For longer messages use the inner text element of the task.</summary>
		[TaskAttribute("message", Required = false, ExpandProperties = false)]
		public string Message { get; set; } = null;

		/// <summary>The name of the file to write the message to.  If empty write message to log.</summary>
		[TaskAttribute("file", Required = false)]
		public string FileName { get; set; } = null;

		/// <summary>Indicates if message should be appended to file.  Default is "false".</summary>
		[TaskAttribute("append", Required = false)]
		public bool Append { get; set; } = false;

		/// <summary>The log level that the message should be printed at. (ie. quiet, minimal, normal, detailed, diagnostic) default is normal.</summary>
		[TaskAttribute("loglevel", Required = false)]
		public Log.LogLevel LogLevel { get; set; } = Log.LogLevel.Normal;

		public EchoTask() : base("echo") { }

		public EchoTask(Project project, string message = null, string fileName = null)
			: base("echo")
		{
			Project = project;
			Message = message;
			FileName = fileName;
		}

		public override string LogPrefix
		{
			get
			{
				return "[echo-" + Properties["package.name"] + "-" + Properties["package.version"] + "] ";
			}
		}

		/// <summary>Initializes the task.</summary>
		protected override void InitializeTask(XmlNode taskNode) 
		{
			if (Message != null && taskNode.InnerText.Length > 0) 
			{
				throw new BuildException("Cannot specify a message attribute and element value, use one or the other.", Location);
			}

			// use the element body if the message attribute was not used.
			if (Message == null) 
			{
				if (taskNode.InnerText.Length == 0) 
				{
					throw new BuildException("Need to specify a message attribute or have an element body.", Location);
				}
				Message = Project.ExpandProperties(taskNode.InnerText);
			}
		}

		protected override void ExecuteTask() 
		{
			if (Log.Level >= LogLevel || FileName != null)
			{
				Message = Project.ExpandProperties(Message);
				if (FileName == null)
				{
					if (Project.TraceEcho)
					{
						Message = string.Format(LogPrefix + "{0} {1}", this.Location, Message);
					}
					else
					{
						if (Log.Level == Log.LogLevel.Normal)
						{
							Log.Status.WriteLine(LogPrefix + Message);
						}
						else if (Log.Level == Log.LogLevel.Diagnostic)
						{
							Log.Debug.WriteLine(LogPrefix + Message);
						}
						else if (Log.Level == Log.LogLevel.Detailed)
						{
							Log.Info.WriteLine(LogPrefix + Message);
						}
						else if (Log.Level == Log.LogLevel.Minimal)
						{
							Log.Minimal.WriteLine(LogPrefix + Message);
						}
						else if (Log.Level == Log.LogLevel.Quiet)
						{
							Log.Error.WriteLine(LogPrefix + Message);
						}
					}
				}
				else
				{
					string path = Project.GetFullPath(FileName);
					StreamWriter streamWriter = null;
					try
					{
						streamWriter = new StreamWriter(new FileStream(path, Append ? FileMode.Append : FileMode.Create, FileAccess.Write));
						switch (PlatformUtil.Platform)
						{
							case PlatformUtil.Windows:
							default:
								{
									streamWriter.WriteLine(Message);
								}
								break;
							case PlatformUtil.Unix:
							case PlatformUtil.OSX:
								{
									streamWriter.WriteLine(Message.Replace("\r\n", "\n"));
								}
								break;
						}
					}
					catch (Exception e)
					{
						string msg = string.Format("Error writing to file '{0}'.", path);
						throw new BuildException(msg, Location, e);

					}
					finally
					{
						if (streamWriter != null)
						{
							streamWriter.Close();
						}
					}
				}
			}
		}
	}
}
