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
using System.Collections.Generic;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
	/// <summary>Specifies dependencies at both the package and module level. For more details see dependency section of docs, filed under "Reference/Packages/build scripts/Dependencies".</summary>
	[ElementName("dependencies", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public class DependenciesPropertyElement : ConditionalElementContainer
	{
		/// <summary>(Note: New in Framework 3+) Using autodependencies is always a safe option. They simplify build scripts when module declaring dependencies can be built as a static or dynamic library depending on configuration settings.</summary>
		[Property("auto", Required = false)]
		public DependencyDefinitionPropertyElement Dependencies { get; set; } = new DependencyDefinitionPropertyElement(isinterface: true, islink: true);

		/// <summary>This type of dependency is often used with static libraries. Acts as a combined interface and link dependency.</summary>
		[Property("use", Required = false)]
		public DependencyDefinitionPropertyElement UseDependencies { get; set; } = new DependencyDefinitionPropertyElement(isinterface: true, islink: true);

		/// <summary>Sets the dependencies to be built by the package. Ignored when dependent package is non buildable, or has autobuildclean=false.</summary>
		[Property("build", Required = false)]
		public DependencyDefinitionPropertyElement BuildDependencies { get; set; } = new DependencyDefinitionPropertyElement(isinterface: true, islink: true);

		/// <summary>These dependencies are used when only header files from dependent package are needed. Adds include directories and defines set in the Initialize.xml file of dependent package.</summary>
		[Property("interface", Required = false)]
		public DependencyDefinitionPropertyElement InterfaceDependencies { get; set; } = new DependencyDefinitionPropertyElement(isinterface: true, islink: false);

		/// <summary>Use these dependencies to make sure no header files or defines from dependent package are used. Library directories, using directories, assemblies and DLLs are also taken from the dependent package.</summary>
		[Property("link", Required = false)]
		public DependencyDefinitionPropertyElement LinkDependencies { get; set; } = new DependencyDefinitionPropertyElement(isinterface: false, islink: true);

		/// <summary>Allows you to disable the automatic build dependency on runtime modules that is used for test and example builds.</summary>
		[TaskAttribute("skip-runtime-dependency")]
		public bool SkipRuntimeDependency { get; set; } = false;

		///<summary>Initializes task and ensures the supplied attributes are valid.</summary>
		///<param name="taskNode">XML node used to define this task instance.</param>
		protected override void InitializeElement(XmlNode taskNode)
		{
			foreach (XmlNode child in taskNode.ChildNodes)
			{
				if (child is XmlText)
				{
					throw new BuildException("The 'dependencies' element cannot contain text. For auto dependencies, please place the names of the dependencies in a nested <auto> element.", Location);
				}
			}
		}
	}

	/// <summary></summary>
	[ElementName("conditionalproperty", StrictAttributeCheck = true, Mixed = true)]
	public class DependencyDefinitionPropertyElement : ConditionalPropertyElement
	{
		internal DependencyDefinitionPropertyElement(bool isinterface=true, bool islink=true)
		{
			IsInterface = _isInterfaceInit = isinterface;
			IsLink = _isLinkInit = islink;
		}

		/// <summary>Public include directories from dependent packages are added.</summary>
		[TaskAttribute("interface", Required = false)]
		public bool IsInterface { get; set; }

		/// <summary>Public libraries from dependent packages are added if this attribute is true.</summary>
		[TaskAttribute("link", Required = false)]
		public bool IsLink { get; set; }

		/// <summary>Set copy local flag for this dependency output.</summary>
		[TaskAttribute("copylocal", Required = false)]
		public bool IsCopyLocal { get; set; }

		/// <summary>Deprecated, this will be ignored.</summary>
		[TaskAttribute("internal", Required = false)]
		public bool? IsInternal { get; set; }

		public override string Value
		{
			set { SelectedValue = value; }
			get { return SelectedValue; }
		}

		public string CopyLocalValue
		{
			get { return _copyLocalDependencies.ToNewLineString(); }
		}

		public override void Initialize(XmlNode elementNode)
		{
			IsCopyLocal = false;
			IsInterface = _isInterfaceInit;
			IsLink = _isLinkInit;

			base.Initialize(elementNode);

			if (IsInternal.HasValue)
			{
				Log.ThrowDeprecation(NAnt.Core.Logging.Log.DeprecationId.InternalDependencies, NAnt.Core.Logging.Log.DeprecateLevel.Minimal, $"{Location} Internal dependencies are no longer supported.");
			}
		}

		// override normal initialize, each time this is called we want to know the value being added and then based on internal / copy local
		// settings add the added value to the appropriate member variables
		protected override void InitializeElement(XmlNode node)
		{
			if (IfDefined && !UnlessDefined)
			{	
				string newValue = null; // build up the total string value to be added by this element
				
				// if <element value="text"/> is set take this value
				if (AttrValue != null)
				{
					newValue = AttrValue;
				}

				// handle inner elements
				foreach (XmlNode child in node)
				{
					string newFragment = null; // fragment of new value added by text, cdata or <do> element
					switch (child.NodeType)
					{				
						case XmlNodeType.Comment:
							continue; // skip comments
						case XmlNodeType.Text:
						case XmlNodeType.CDATA:
							if (AttrValue != null)
							{
								throw new BuildException($"Element value can not be defined in both '{AttrValue}' attribute and as element text.", Location);
							}

							newFragment = child.InnerText;
							break;
						default:
							if (child.Name == "do")
							{
								// gross hack, ConditionalElement gets direct access to internal value so cache current value then clear it....
								string currentValue = Value;
								Value = null;

								// ...initialize child and store what it set our value to...
								ConditionElement condition = new ConditionElement(Project, this);
								condition.Initialize(child);
								newFragment = Value;

								// ...then set it back to old value
								Value = currentValue;
							}
							else
							{
								// do is the only allow nested element
								throw new BuildException($"XML tag <{child.Name}> is not allowed inside property values. Use <do> element for conditions.", Location);
							}
							break;
					}

					// append new fragment to new value 
					if (newFragment != null)
					{
						newValue = (newValue == null) ? newFragment : newValue + Environment.NewLine + newFragment;
					}
				}

				// append new value to existing value, copy local value and internal value as appropriate
				if (newValue != null)
				{
					Value = (Value == null) ? newValue : Value + Environment.NewLine + newValue;

					string trimmedValue = newValue.TrimWhiteSpace();
					if (!String.IsNullOrWhiteSpace(trimmedValue))
					{
						if (IsCopyLocal)
						{
							_copyLocalDependencies.Add(trimmedValue);
						}
					}
				}

				// perform expansion to get final value
				if (!String.IsNullOrEmpty(Value))
				{
					Value = Project.ExpandProperties(Value);
				}
			}
		}

		private string SelectedValue
		{
			get
			{
				if (IsLink && IsInterface)
				{
					return Interface_1_Link_1;
				}
				else if (IsLink && !IsInterface)
				{
					return Interface_0_Link_1;
				}
				else if (!IsLink && IsInterface)
				{
					return Interface_1_Link_0;
				}
				return _Value;
			}

			set
			{
				if (IsLink && IsInterface)
				{
					Interface_1_Link_1 = value;
				}
				else if (IsLink && !IsInterface)
				{
					Interface_0_Link_1 = value;
				}
				else if (!IsLink && IsInterface)
				{
					Interface_1_Link_0 = value;
				}
				_Value = value;
			}
		}

		internal string Interface_0_Link_0 { get { return _Value; } }
		internal string Interface_1_Link_1 { get; private set; }
		internal string Interface_1_Link_0 { get; private set; }
		internal string Interface_0_Link_1 { get; private set; }

		private HashSet<string> _copyLocalDependencies = new HashSet<string>();

		private readonly bool _isInterfaceInit;
		private readonly bool _isLinkInit;
	}
}