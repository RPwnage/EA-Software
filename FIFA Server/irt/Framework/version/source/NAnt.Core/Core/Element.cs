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
using System.Collections.Generic;
using System.Xml;

using NAnt.Core.Reflection;
using NAnt.Core.Logging;

namespace NAnt.Core
{
	/// <summary>Models a NAnt XML element in the build file.</summary>
	/// <remarks>
	///   <para>Automatically validates attributes in the element based on Attribute settings in the derived class.</para>
	/// </remarks>
	abstract public class Element
	{
		public string Name
		{
			get { return _elementName; }
		} internal string _elementName;

		protected bool __element_initialized = false;

		protected XmlNode _xmlNode = null;

		public Element()
		{
		}

		public Element(string name)
		{
			_elementName = name;
		}

		public Element(Element e) : this(e.Name)
		{
			Location = e.Location;
			Project = e.Project;
			Parent = e.Parent;
			_xmlNode = e._xmlNode;
		}

		public XmlNode XmlNode
		{
			get { return _xmlNode; }
			set
			{
				if (__element_initialized) throw new BuildException("Cannot modify XmlNode definition after the element has been initialized");
				_xmlNode = value;
			}
		}

		/// <summary>Whether to print a warning if the same element is initialized multiple times.</summary>
		public virtual bool WarnOnMultiple { get; } = true;

		/// <summary><see cref="Location"/> in the build file where the element is defined.</summary>
		public virtual Location Location { get; set; } = Location.UnknownLocation;

		/// <summary>The <see cref="Project"/> this element belongs to.</summary>
		public Project Project { get; set; }

		/// <summary>The Parent object. This will be your parent Task, Target, or Project depending on where the element is defined.</summary>
		public object Parent { get; set; } = null;

		/// <summary>The properties local to this Element and the project.</summary>
		public PropertyDictionary Properties
		{
			get { return Project.Properties; }
		}

		/// <summary>The Log instance associated with the project.</summary>
		public Log Log
		{
			get { return Project.Log; }
		}

		protected virtual bool InitializeChildElements { get { return true; } }

		/// <summary>Calls initialize on an elements store XmlNode value, used when creating a task programmatically</summary>
		public virtual void Initialize()
		{
			if (XmlNode != null)
			{
				Initialize(XmlNode);
			}
		}

		/// <summary>Performs default initialization.</summary>
		/// <remarks>
		///   <para>Derived classes that wish to add custom initialization should override <see cref="InitializeElement"/>.</para>
		/// </remarks>
		public virtual void Initialize(XmlNode elementNode)
		{
			if (Project == null)
			{
				throw new InvalidOperationException("Element has invalid (null) Project property.");
			}

			// Save position in build file for reporting useful error messages.
			if (!String.IsNullOrEmpty(elementNode.BaseURI))
			{
				try
				{
					Location = Location.GetLocationFromNode(elementNode);
				}
				catch (ArgumentException ae)
				{
					Log.Warning.WriteLine("Can't find node '{0}' location, file: '{1}'{2}{3}", elementNode.Name, elementNode.BaseURI, Environment.NewLine, ae.ToString());
				}
			}

			try
			{
				InitializeAttributes(elementNode);

				// Allow inherited classes a chance to do some custom initialization.
				InitializeElement(elementNode);

			}
			catch (Exception ex)
			{
				throw new ContextCarryingException(ex, Name, Location);
			}
		}

		/// <summary>Helper task for manual initialization of a build element.(nested into this element)</summary>
		protected void InitializeBuildElement(XmlNode elementNode, string xmlName, bool isRequired=false)
		{
			// get value from XML node
			XmlNode nestedElementNode = elementNode[xmlName, elementNode.OwnerDocument.DocumentElement.NamespaceURI];

			if (nestedElementNode != null)
			{
				long foundRequiredElements = 0;
				long foundUnknownElements = 0;
				ElementInitHelper.GetElementInitializer(this).InitNestedElement(this, nestedElementNode, false, ref foundRequiredElements, ref foundUnknownElements);
			}
			else if (isRequired) {
				string msg = String.Format("'{0}' is a required element.", xmlName);
				throw new BuildException(msg, Location);
			}
		}

		protected virtual void InitializeAttributes(XmlNode elementNode)
		{
			ElementInitHelper.ElementInitializer initializer = ElementInitHelper.GetElementInitializer(this);

			long foundRequiredAttributes = 0;
			long foundUnknownAttributes = 0;

			// init attributes
			foreach (XmlAttribute attr in elementNode.Attributes)
			{
				initializer.InitAttribute(this, attr.Name, attr.Value, ref foundRequiredAttributes, ref foundUnknownAttributes);
			}

			initializer.CheckRequiredAttributes(elementNode, foundRequiredAttributes);
			initializer.CheckUnknownAttributes(elementNode, foundUnknownAttributes);

			if (initializer.HasNestedElements && InitializeChildElements)
			{
				long foundRequiredElements = 0;
				long foundUnknownElements = 0;

				foreach (XmlNode childNode in elementNode.ChildNodes)
				{
					initializer.InitNestedElement(this, childNode, true, ref foundRequiredElements, ref foundUnknownElements);
				}

				initializer.CheckRequiredNestedElements(elementNode, foundRequiredElements);

				CheckUnknownNestedElements(initializer, elementNode, foundRequiredElements);
			}
			
			else if (elementNode.HasChildNodes)
			{
				CheckUnknownNestedElements(initializer, elementNode, elementNode.ChildNodes.Count);
			}
		}

		protected void CheckUnknownNestedElements(ElementInitHelper.ElementInitializer initializer, XmlNode eltNode, long foundUnknownElements)
		{
			List<string> unknownNestedElements;

			var unknownCount = initializer.CheckUnknownNestedElements(eltNode, foundUnknownElements, out unknownNestedElements);

			if (unknownCount > 0 && unknownNestedElements != null)
			{
				Log.ThrowWarning
				(
					Log.WarningId.SyntaxError, Log.WarnLevel.Normal,
					"Element '{0}' contains unknown nested element{1}:{2}{3}.", 
					eltNode.Name, 
					unknownCount > 1 ? "s" : String.Empty, 
					unknownCount > 1 ? Environment.NewLine : String.Empty, 
					unknownNestedElements.ToString(Environment.NewLine, s => (unknownCount > 1 ? String.Empty.PadLeft(Log.Padding.Length + 14) : " ") + s));
			}
		}

		/// <summary>Prints a warning if the same element is initialized multiple times.</summary>
		protected virtual void WarnOnMultipleInitialization(XmlNode elementNode)
		{
			if (__element_initialized == false)
			{
				__element_initialized = true;
			}
			else if (WarnOnMultiple)
			{
				Log.ThrowWarning(Log.WarningId.SyntaxError, Log.WarnLevel.Normal,
					@"Element '{0}' was initialized more than once! 
Multiple initializations may occur when there is more than one of a child element with conditions satisfied and only one is expected, the unexpected elements will in most cases be ignored.
({1})", elementNode.Name, Location.ToString());
			}
		}

		/// <summary>Allows derived classes to provide extra initialization and validation not covered by the base class.</summary>
		/// <param name="elementNode">The XML node of the element to use for initialization.</param>
		protected virtual void InitializeElement(XmlNode elementNode) 
		{
			WarnOnMultipleInitialization(elementNode);
		}
	}
}
