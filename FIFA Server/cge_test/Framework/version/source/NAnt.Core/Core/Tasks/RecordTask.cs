// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2002-2003 Gerry Shaw
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
// File Maintainer
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks
{

	/// <summary>A task that records the build's output to a property or file.</summary>
	/// <remarks>
	/// This task allows you to record the build's output, or parts of it to 
	/// a named property or file.
	/// </remarks>
	/// <include file='Examples/Record/Simple.example' path='example'/>
	[TaskName("record")]
	public class RecordTask : TaskContainer
	{
		/// <summary>Name of the property to record output.</summary>
		[TaskAttribute("property", Required = false)]
		public string PropertyName { get; set; }

		/// <summary>If set to true, no other output except of property is produced. Console or other logs are suppressed</summary>
		[TaskAttribute("silent", Required = false)]
		public bool Silent { get; set; } = false;

		/// <summary>The name of a file to write the output to</summary>
		[TaskAttribute("file", Required = false)]
		public string FileName { get; set; }

		public RecordTask() : base() { }

		public RecordTask(Project project, string propertyName = null, bool silent = false, string fileName = null) : base()
		{
			Project = project;
			PropertyName = propertyName;
			Silent = silent;
			FileName = fileName;
		}

		protected override void InitializeTask(XmlNode taskNode)
		{
			base.InitializeTask(taskNode);
		}

		protected override void ExecuteTask()
		{
			List<ILogListener> listeners = new List<ILogListener>();
			List<ILogListener> errorListeners = new List<ILogListener>();

			// add parent project log listners to continue standard logging unless Silent is set
			if (!Silent)
			{
				listeners.AddRange(Log.Listeners);
				errorListeners.AddRange(Log.ErrorListeners);
			}

			FileListener fileListener = null;
			if (FileName != null)
			{
				fileListener = new FileListener(FileName);
				listeners.Add(fileListener);
				errorListeners.Add(fileListener);
			}

			StringBuilder propertyLogAccumulator = null;
			if (PropertyName != null)
			{
				propertyLogAccumulator = new StringBuilder();
				BufferedListener bufferedListner = new BufferedListener(propertyLogAccumulator);
				listeners.Add(bufferedListner);
				errorListeners.Add(bufferedListner);
			}

			Log recordingLog = new Log
			(
				Project.Log,
				listeners: listeners,
				errorListeners: errorListeners
			);

			try
			{
                Project recordContextProject = new Project(Project, recordingLog);
				{
					foreach (XmlNode node in _xmlNode)
					{
						if (node.NodeType == XmlNodeType.Element)
						{
							Task task = recordContextProject.CreateTask(node, this);
							task.Execute();
						}
					}
				}
			}
			finally
			{
				if (fileListener != null)
				{
					fileListener.Close();
				}
				if (propertyLogAccumulator != null)
				{
					Project.Properties.Add(PropertyName, propertyLogAccumulator.ToString().Trim());
				}
			}
		}
	}
}
