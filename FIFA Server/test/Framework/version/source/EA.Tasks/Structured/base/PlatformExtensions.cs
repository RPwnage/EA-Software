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
using System.Xml;

using NAnt.Core;

namespace EA.Eaconfig.Structured
{
	public class PlatformExtensions : Task
	{
		protected override void InitializeTask(XmlNode taskNode)
		{
			base.InitializeTask(taskNode);
			_xmlNode = taskNode;
		}

		protected override void ExecuteTask()
		{
		}

		public void ExecutePlatformTasks(ModuleBaseTask module)
		{
			if (_xmlNode != null)
			{
				string platformNodeName = Properties.GetPropertyOrDefault("structured-xml.platform-extension.name", Project.Properties["config-system"]);

				foreach (XmlNode childNode in _xmlNode)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(_xmlNode.NamespaceURI))
					{
						if ((platformNodeName ?? childNode.Name).Equals(childNode.Name, StringComparison.OrdinalIgnoreCase))
						{
							var task = CreatePlatformExtensionTask(childNode);
							task.Parent = this;
							task.Project = Project;
							task.Init(module);
							task.Initialize(childNode);
							task.Execute();
						}
					}
				}
			}
		}

		private PlatformExtensionBase CreatePlatformExtensionTask(XmlNode node)
		{
			string taskName = "structured-extension-" + node.Name.ToLowerInvariant();

			var task = Project.TaskFactory.CreateTask(taskName, Project);

			if (task == null)
			{
				throw new BuildException(String.Format("Can't find platform extension task: '{0}' reflected from XML node '{1}'", taskName, node.Name), Location.GetLocationFromNode(node));
			}

			var extensiontask = task as PlatformExtensionBase;
			if (extensiontask == null)
			{
				throw new BuildException(String.Format("Platform extension task: '{0}' reflected from XML node '{1}' is not derived from PlatformExtensionBase class", taskName, node.Name), Location.GetLocationFromNode(node));
			}

			return extensiontask;
		}

	}

	abstract public class PlatformExtensionBase : Task
	{
		public ModuleBaseTask Module { get; private set; }

		internal void Init(ModuleBaseTask module)
		{
			Module = module;
		}
	}

}
