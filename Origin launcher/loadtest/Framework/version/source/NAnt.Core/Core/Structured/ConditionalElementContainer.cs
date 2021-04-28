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

using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Reflection;

namespace NAnt.Core.Structured
{
	public abstract class ConditionalElementContainer : Element
	{
		[XmlElement("echo", "NAnt.Core.Tasks.EchoTask", AllowMultiple = true)]
		[XmlElement("do", Container = XmlElementAttribute.ContainerType.ConditionalContainer | XmlElementAttribute.ContainerType.Recursive, AllowMultiple = true)]
		protected override void InitializeAttributes(XmlNode elementNode)
		{
			ElementInitHelper.ElementInitializer initializer = ElementInitHelper.GetElementInitializer(this);

			long foundRequiredAttributes = 0;
			long foundUnknownAttributes = 0;
			long foundRequiredElements = 0;
			long foundUnknownElements = 0;

			for (int i = 0; i < elementNode.Attributes.Count; i++)
			{
				XmlAttribute attr = elementNode.Attributes[i];

				initializer.InitAttribute(this, attr.Name, attr.Value, ref foundRequiredAttributes, ref foundUnknownAttributes);
			}

			initializer.CheckRequiredAttributes(elementNode, foundRequiredAttributes);
			initializer.CheckUnknownAttributes(elementNode, foundUnknownAttributes);

			if (initializer.HasNestedElements)
			{
				foreach (XmlNode childNode in elementNode.ChildNodes)
				{
					InitChildNode(initializer, childNode, ref foundRequiredElements, ref foundUnknownElements);
				}

				initializer.CheckRequiredNestedElements(elementNode, foundRequiredElements);

				CheckUnknownNestedElements(initializer, elementNode, foundUnknownElements);
			}

			else if (elementNode.HasChildNodes)
			{
				CheckUnknownNestedElements(initializer, elementNode, elementNode.ChildNodes.Count);
			}
		}

		protected void InitChildNode(ElementInitHelper.ElementInitializer initializer, XmlNode childNode, ref long foundRequiredElements, ref long foundUnknownElements)
		{
			if (childNode.NodeType == XmlNodeType.Element && childNode.LocalName == "do")
			{
				var dotask = new DoTask();
				dotask.Project = Project;
				dotask.Parent = null;
				dotask.Initialize(childNode);
				if (dotask.IfDefined && !dotask.UnlessDefined)
				{
					foreach (XmlNode childChildNode in childNode.ChildNodes)
					{
						InitChildNode(initializer, childChildNode, ref foundRequiredElements, ref foundUnknownElements);
					}
				}
			}
			else if (childNode.NodeType == XmlNodeType.Element && childNode.LocalName == "echo")
			{
				var echo = new EchoTask();
				echo.Project = Project;
				echo.Parent = null;
				echo.Initialize(childNode);
				echo.Execute();
			}
			else
			{
				initializer.InitNestedElement(this, childNode, true, ref foundRequiredElements, ref foundUnknownElements);
			}

		}
	}
}
