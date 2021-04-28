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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Reflection;

using System;
using System.Reflection;
using System.Collections.Specialized;
using System.Xml;

namespace EA.Eaconfig.Structured
{
	public abstract class ConditionalTaskContainer : TaskContainer
	{
		protected StringCollection _parentSubXMLElements = null;
		protected Element _parentElement = null;

		public ConditionalTaskContainer()
		{
		}

		public ConditionalTaskContainer(string name)
			: base(name)
		{
		}

		protected virtual void InitializeParentTask(XmlNode taskNode, Element parent)
		{
			_parentSubXMLElements = new StringCollection();

			 _parentElement = parent;

			if (_parentElement != null)
			{
				foreach (MemberInfo memInfo in _parentElement.GetType().GetMembers(BindingFlags.Instance | BindingFlags.Public))
				{
					if (memInfo.DeclaringType.Equals(typeof(object)))
					{
						continue;
					}

					BuildElementAttribute buildElemAttr = (BuildElementAttribute)Attribute.GetCustomAttribute(memInfo, typeof(BuildElementAttribute), true);
					if (buildElemAttr != null)
					{
						_parentSubXMLElements.Add(buildElemAttr.Name);
					}
				}
			}
		}

		protected override void ExecuteChildTasks()
		{
			long foundRequiredElements = 0;
			long foundUnknownElements = 0;

			foreach (XmlNode childNode in _xmlNode)
			{
				if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(_xmlNode.NamespaceURI))
				{
					if (IsPrivateXMLElement(childNode)) continue;

					if (IsParentXMLElement(childNode))
					{
						ElementInitHelper.GetElementInitializer(_parentElement).InitNestedElement(_parentElement, childNode, false, ref foundRequiredElements, ref foundUnknownElements);
					}
					else
					{
						Task task = CreateChildTask(childNode);
						task.Execute();
					}
				}
			}
			try
			{
				CheckUnknownNestedElements(ElementInitHelper.GetElementInitializer(this), _xmlNode, foundUnknownElements);
			}
			catch (BuildException bex)
			{
				throw new BuildException("Error in task  <" + this.Name + ">: " + bex.BaseMessage, bex.Location);
			}
		}

		[XmlTaskContainer("-")]
		[XmlElement("echo", "NAnt.Core.Tasks.EchoTask", AllowMultiple = true)]
		[XmlElement("do", Container = XmlElementAttribute.ContainerType.ConditionalContainer | XmlElementAttribute.ContainerType.Recursive, AllowMultiple = true)]
		protected override Task CreateChildTask(XmlNode node)
		{
			Task task = null;

			if (node.NodeType != XmlNodeType.Comment)
			{
				if (node.Name == "do")
				{
					task = new SectionTask();

					task.Project = Project;
					task.Parent = null;
					task.Initialize(node);
				}
				else
				{
					task = base.CreateChildTask(node);
				}

				ConditionalTaskContainer taskContainer = task as ConditionalTaskContainer;

				Element parent = _parentElement ?? this;

				if (taskContainer != null)
				{
					taskContainer.InitializeParentTask(node, parent);
				}
				else if (task != null && !IsValidElement(task))
				{
					Location loc = (task != null) ? task.Location : Location.GetLocationFromNode(node);
					throw new BuildException($"XML element <{node.Name}> is not allowed inside <{parent.Name}> element.", loc);
				}
			}

			return task;
		}

		protected virtual bool IsParentXMLElement(XmlNode node)
		{
			return (_parentSubXMLElements != null && _parentSubXMLElements.Contains(node.Name));
		}

		protected virtual bool IsValidElement(Element task)
		{
			bool valid = task != null && task is EchoTask;
			return valid;
		}

	}
}
