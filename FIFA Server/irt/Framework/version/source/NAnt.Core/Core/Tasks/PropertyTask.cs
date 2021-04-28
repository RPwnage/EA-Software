// NAnt - A .NET build tool
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
// File Maintainers:
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System.IO;
using System;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using System.Collections.Generic;

namespace NAnt.Core.Tasks
{
	/// <summary>Sets a property in the current project.</summary>
	/// <remarks>
	///   <para>NAnt uses a number of predefined properties that start with nant.* or [taskname].*.  In general you should place properties into a namespace such as global.* or ProjectName.*.</para>
	///   <para>If the property name is invalid a build error will occur.</para>
	///   <para>The following regular expression is used to test for valid properties: <c>^([\w-\.]+)$</c>.  In English this means only A-Z, a-z, 0-9, '_', '-', and '.' characters are allowed.  The leading character should be a letter for readability.</para>
	///   <para>The task declares the <c>${property.value}</c> property within the task itself.  The <c>${property.value}</c> property is equal to the previous value of the property if already defined, otherwise it is equal to an empty string.  By using the <c>${property.value}</c> property, user can easily insert/append to an existing property.</para>
	///   <para><b>NOTE:</b> If you are using this property task inside a &lt;parallel.do&gt; task or &lt;parallel.foreach&gt; task, please be aware that this property task is not thread safe.  So if you are using this task inside either of the parallel blocks, you will need to make sure that they have unique names inside each block to avoid conflict.</para>
	/// </remarks>
	/// <include file='Examples/Property/Simple.example' path='example'/>
	/// <include file='Examples/Property/Nested.example' path='example'/>
	/// <include file='Examples/Property/PropertyValue.example' path='example'/>
	/// <include file='Examples/Property/PropertyFromFile.example' path='example'/>
	[TaskName("property", Mixed = true)]
	public class PropertyTask : Task
	{
		private string localValue;

		/// <summary>The name of the property to set.</summary>
		[TaskAttribute("name", Required = true)]
		public string PropertyName { get; set; }

		/// <summary>The value of the property. If not specified, the default will be no value.</summary>
		[TaskAttribute("value", ExpandProperties = false)]
		public string Value { get; set; }

		/// <summary>The path to a file from which content is read into the property value.</summary>
		[TaskAttribute("fromfile", Required = false)]
		public string FileName { get; set; }

		/// <summary>Indicates if the property should be read-only.  Read only properties can never be changed.  Default is false.</summary>
		[TaskAttribute("readonly")]
		public bool ReadOnly { get; set; }

		/// <summary>Indicates if the property's value will expand encapsulated properties' value at definition time or at use time. Default is false.</summary>
		[TaskAttribute("deferred")]
		public bool Deferred { get; set; }

		/// <summary>Indicates if the property is going to be defined in a local context and thus, it will be restricted to a local scope. Default is false.</summary>
		[TaskAttribute("local")]
		public bool Local { get; set; }

		public PropertyTask() : base() { }

		// constructor for programmatically initializing the task
		public PropertyTask(Project project, string name, string value = null, string fromFile = null, bool local = false) 
			: base()
		{
			Project = project;
			PropertyName = name;
			Value = value;
			FileName = fromFile;
			Local = local;
		}

		/// <summary>Optimization. Directly initialize</summary>
		public override void Initialize(XmlNode elementNode)
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

			string _readOnlyVal = null;
			string _deferredVal = null;
			string _localVal = null;

			try
			{
				foreach (XmlAttribute attr in elementNode.Attributes)
				{
					switch (attr.Name)
					{
						case "name": // REq
							PropertyName = attr.Value; 
							break;
						case "value":  
							Value = attr.Value;
							break;
						case "fromfile":
							FileName = attr.Value;
							break;
						case "readonly":
							_readOnlyVal = attr.Value;
							break;
						case "deferred":
							_deferredVal = attr.Value;
							break;
						case "local":
							_localVal = attr.Value;
							break;
						default:
							if (!OptimizedTaskElementInit(attr.Name, attr.Value))
							{
								string msg = String.Format("Unknown attribute '{0}'='{1}' in Property task", attr.Name, attr.Value);
								throw new BuildException(msg, Location);
							}
							break;
					}
				}

				if (PropertyName == null)
				{
					throw new BuildException("Missing required <property> attribute 'name'", Location.GetLocationFromNode(elementNode));
				}

				if (IfDefined && !UnlessDefined)
				{
					PropertyName = Project.ExpandProperties(PropertyName);
					if (FileName != null) FileName = Project.ExpandProperties(FileName);
					if (_readOnlyVal != null) ReadOnly = ElementInitHelper.InitBoolAttribute(this, "readonly", _readOnlyVal);
					if (_deferredVal != null) Deferred = ElementInitHelper.InitBoolAttribute(this, "deferred", _deferredVal);
					if (_localVal != null) Local = ElementInitHelper.InitBoolAttribute(this, "local", _localVal);

					// Just defer for now so that everything just works
					InitializeTask(elementNode);
				}

			}
			catch (Exception e)
			{
				throw new ContextCarryingException(e, Name, Location);
			}
		}
 
		///<summary>Initializes task and ensures the supplied attributes are valid.</summary>
		///<param name="taskNode">XML node used to define this task instance.</param>
		protected override void InitializeTask(XmlNode taskNode)
		{
			if (Value != null && taskNode.InnerText.Length != 0)
			{
				throw new BuildException("The property value can only appear in the value attribute or the element body.", Location);
			}
			if ((Value != null || taskNode.InnerText.Length != 0) && FileName != null)
			{
				throw new BuildException("The property value can not be defined together with the 'fromfile' attribute.", Location);
			}

			if (Value == null)
			{
				if (FileName == null)
				{
					// get property value from element data (ie, <property name="foo">${bar}</property>
					Value = taskNode.InnerText;
				}
				else if (FileName != null)
				{
					// get property value from a file (ie, <property name="foo" fromfile="filename.txt" /> )
					Deferred = true;
				}
			}

			if (!Deferred)
			{
				Value = StringParser.ExpandString(Value, new StringParser.PropertyEvaluator(EvaluateParameter), new StringParser.FunctionEvaluator(Properties.EvaluateFunction), new StringParser.FindClosePropertyMatches(Properties.FindClosestMatches));
			}

			if (taskNode.HasChildNodes)
			{
				CheckNestedElements(taskNode);
			}
		}

		protected override void ExecuteTask()
		{
			if (FileName != null)
			{
				Value = ReadValueFromFile(FileName);
			}
			Properties.Add(PropertyName, Value, ReadOnly, Deferred, Local);
		}

		private string EvaluateParameter(string name, Stack<string> evulationStack)
		{
			if (name == "property.value")
			{
				if (localValue == null)
				{
					localValue = Properties.EvaluateParameter(PropertyName, evulationStack) ?? String.Empty;
				}
				return localValue;
			}
			return Properties.EvaluateParameter(name, evulationStack);
		}

		private void CheckNestedElements(XmlNode taskNode)
		{
			foreach (XmlNode node in taskNode.ChildNodes)
			{
				if (node.NodeType == XmlNodeType.Element)
				{
					throw new BuildException(string.Format("{0} property '{1}' contains unknown nested element: '{2}'.",
						Location.GetLocationFromNode(node), PropertyName, node.LocalName));
				}
			}
		}

		private string ReadValueFromFile(string filename)
		{
			// If file exists but empty, property value will be left null
			string value = String.Empty;

			try
			{
				string path = Project.GetFullPath(filename);
				StreamReader sr = File.OpenText(path);
				sr.BaseStream.Seek(0, SeekOrigin.Begin);
				while (sr.Peek() > -1)
				{
					value += sr.ReadLine();
					if (sr.Peek() > -1)
						value += Environment.NewLine;
				}
				sr.Close();
			}
			catch (Exception e)
			{
				throw new BuildException(e.Message, Location);
			}

			return value;
		}
	}
}
