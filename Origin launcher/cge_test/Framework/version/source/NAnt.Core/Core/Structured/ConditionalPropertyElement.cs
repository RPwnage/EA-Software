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

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Structured
{
	// DAVE-FUTURE-REFACTOR-TODO: PropertyElement is kind of redundant comapred to ConditionalPropertyElement ...
	// DAVE-FUTURE-REFACTOR-TODO: ... and then this could also inherit from ConditionalElement ...

	/// <summary></summary>
	[ElementName("conditionalproperty", StrictAttributeCheck = true, Mixed=true)]
	public class ConditionalPropertyElement : PropertyElement
	{
		public ConditionalPropertyElement(bool append = true)
			: base()
		{
			Append = append;
		}

		/// <summary>If true then the task will be executed; otherwise skipped. Default is "true".</summary>
		[TaskAttribute("if")]
		public virtual bool IfDefined { get; set; } = true;

		/// <summary>Opposite of if.  If false then the task will be executed; otherwise skipped. Default is "false".</summary>
		[TaskAttribute("unless")]
		public virtual bool UnlessDefined { get; set; } = false;
		
		/// <summary>Argument. Default is null.</summary>
		[TaskAttribute("value", Required = false, ExpandProperties = false)]
		public virtual string AttrValue { get; set; }

		/// <summary>Append new data to the current value. The current value may come from partial modules. Default: 'true'.</summary>
		[TaskAttribute("append")]
		public virtual bool Append { get; set; }

		public override void Initialize(XmlNode elementNode)
		{
			// reset if / unless in case this is a multiply defined element
			IfDefined = true;
			UnlessDefined = false;

			base.Initialize(elementNode);
		}

		[XmlElement("do", Container = XmlElementAttribute.ContainerType.ConditionalContainer | XmlElementAttribute.ContainerType.Recursive, AllowMultiple = true, Mixed=true)]
		protected override void InitializeElement(XmlNode node)
		{
			if (IfDefined && !UnlessDefined)
			{
				if (AttrValue != null)
				{
					if (Value != null)
					{
						Value += Environment.NewLine + AttrValue;
					}
					else
					{
						Value = AttrValue;
					}
				}

				foreach (XmlNode child in node)
				{
					if (child.NodeType == XmlNodeType.Comment)
					{
						continue;
					}

					if (AttrValue != null)
					{
						throw new BuildException("Element value can not be defined in both 'value' attribute and as element text", Location);
					}

					if (child.NodeType == XmlNodeType.Text || child.NodeType == XmlNodeType.CDATA)
					{
						if (Value != null)
						{
							Value += Environment.NewLine + child.InnerText;
						}
						else
						{
							Value = child.InnerText;
						}
					}
					else if (child.Name == "do")
					{
						ConditionElement condition = new ConditionElement(Project, this);
						condition.Initialize(child);
					}
					else
					{
						throw new BuildException($"XML tag <{child.Name}> is not allowed inside property values. Use <do> element for conditions.", Location);
					}
				}
				if (!String.IsNullOrEmpty(Value))
				{
					Value = Project.ExpandProperties(Value);
				}
			}
		}
	}

	[ElementName("deprecatedproperty")]
	public class DeprecatedPropertyElement : Element
	{
		private readonly Log.DeprecationId m_deprecationId;
		private readonly Log.DeprecateLevel m_deprecateLevel;
		private readonly string m_additionalError;
		private readonly DeprecationCondition m_condition;

		public delegate bool DeprecationCondition(Project project);

		public DeprecatedPropertyElement(Log.DeprecationId deprecatedId, Log.DeprecateLevel deprecateLevel, string addtionalError = null, DeprecationCondition condition = null)
		{
			m_deprecationId = deprecatedId;
			m_deprecateLevel = deprecateLevel;
			m_additionalError = addtionalError;
			m_condition = condition;
		}

		protected override void InitializeElement(XmlNode node)
		{
			if (m_condition == null || m_condition(Project))
			{
				Log.ThrowDeprecation(m_deprecationId, m_deprecateLevel, new object[] { Location, node.Name }, "{0} Element '<{1}>' is no longer supported and has no effect.{2}",
					Location, node.Name, String.IsNullOrWhiteSpace(m_additionalError) ? String.Empty : " " + m_additionalError.TrimStart());
			}
		}
	}

	/// <summary></summary>
	[ElementName("if", StrictAttributeCheck = true)]
	class ConditionElement : PropertyElement
	{
		private bool _ifDefined = true;
		private bool _unlessDefined = false;

		private ConditionalPropertyElement _property;

		internal ConditionElement(Project project, ConditionalPropertyElement property)
			: base()
		{
			Project = project;
			_property = property;
		}

		/// <summary>If true then the task will be executed; otherwise skipped. Default is "true".</summary>
		[TaskAttribute("if")]
		public virtual bool IfDefined
		{
			get { return _ifDefined; }
			set { _ifDefined = value; }
		}

		/// <summary>Opposite of if.  If false then the task will be executed; otherwise skipped. Default is "false".</summary>
		[TaskAttribute("unless")]
		public virtual bool UnlessDefined
		{
			get { return _unlessDefined; }
			set { _unlessDefined = value; }
		}

		public override void Initialize(XmlNode elementNode)
		{
			_ifDefined = true;
			_unlessDefined = false;
			base.Initialize(elementNode);
		}

		protected override void InitializeElement(XmlNode node)
		{
			if (IfDefined && !UnlessDefined)
			{
				foreach (XmlNode child in node)
				{
					if (child.NodeType == XmlNodeType.Comment)
					{
						continue;
					}

					if (child.NodeType == XmlNodeType.Text || child.NodeType == XmlNodeType.CDATA)
					{

						if(_property.Value != null)
						{
							_property.Value += Environment.NewLine + child.InnerText;
						}
						else
						{
							_property.Value = child.InnerText;
						}
					}
					else if (child.Name == "do")
					{
						ConditionElement condition = new ConditionElement(Project, _property);
						condition.Initialize(child);
					}
					else
					{
						throw new BuildException($"XML tag <{child.Name}> is not allowed inside property values. Use <do> element for conditions.", Location);
					}
				}
			}
		}
	}
}
