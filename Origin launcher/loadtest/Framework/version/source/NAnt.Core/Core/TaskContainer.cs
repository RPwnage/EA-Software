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
// Module Maintainers:
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Scott Hernandez (ScottHernandez@hotmail.com)

using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Reflection;

namespace NAnt.Core
{
	public abstract class TaskContainer : Task
	{
		private ElementInitHelper.ElementInitializer _initializer;

		public TaskContainer()
		{
		}

		public TaskContainer(string name)
			: base(name)
		{
		}
		
		protected override void InitializeTask(XmlNode taskNode)
		{
			base.InitializeTask(taskNode);

			_initializer = ElementInitHelper.GetElementInitializer(this);
			_xmlNode = taskNode;
		}

		protected override void ExecuteTask()
		{
			try
			{
				if (IfDefined && !UnlessDefined)
				{
					ExecuteChildTasks();
				}
			}
			catch (BuildException ex)
			{
				throw new ContextCarryingException(ex, Name, Location);
			}
		}

		/// <summary>
		/// Creates and Executes the embedded (child XML nodes) elements.
		/// </summary>
		/// <remarks> Skips any element defined by the host task that has an BuildElementAttribute (included filesets and special XML) defined.</remarks>
		protected virtual void ExecuteChildTasks()
		{
			foreach (XmlNode childNode in _xmlNode)
			{
				if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(_xmlNode.NamespaceURI))
				{
					if (!IsPrivateXMLElement(childNode))
					{
						Task task = CreateChildTask(childNode);
						task.Execute();
					}
				}
			}
		}

		[XmlTaskContainer()]
		protected virtual Task CreateChildTask(XmlNode node)
		{
			return Project.CreateTask(node, this);
		}

		protected virtual bool IsPrivateXMLElement(XmlNode node)
		{
			return (_initializer.HasNestedElements && _initializer.IsNestedElement(node.Name));
		}

	}
}
