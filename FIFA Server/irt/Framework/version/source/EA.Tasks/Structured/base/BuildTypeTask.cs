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
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using NAnt.Core.Threading;

using System;
using System.Collections.Generic;
using System.Xml;
using System.Linq;


namespace EA.Eaconfig.Structured
{
	/// <summary>The build type task can be used to create custom build types, derived from existing build types but with custom settings.</summary>
	/// <example>
	///  <para>This is an example of how to generate and use a build type using the Build Type task:</para>
	///  <code><![CDATA[
	///  <BuildType name="Library-Custom" from="Library">
	///    <option name="optimization" value="off"/>
	///  </BuildType>
	///  
	///  <Library name="MyLibrary" buildtype="Library-Custom">
	///    . . .
	///  </Library>
	///  ]]></code>
	/// </example>
	[TaskName("BuildType", NestedElementsCheck = false)]
	public class BuildTypeTask : Task
	{
	   protected BuildTypeElement _buildTypeElement;

		/// <summary>The name of a buildtype ('Library', 'Program', etc.) to derive new build type from.</summary>
		[TaskAttribute("from", Required = true)]
		public string FromBuildType { get; set; }

		/// <summary>Sets the name for the new buildtype</summary>
		[TaskAttribute("name", Required = true)]
		public string BuildTypeName { get; set; }

		/// <summary>Define options to remove from the final optionset</summary>
		[Property("remove", Required = false)]
		public RemoveBuildOptions RemoveOptions { get; } = new RemoveBuildOptions();

		[XmlElement("option", "NAnt.Core.OptionSet+Option", AllowMultiple = true)]
		protected override void InitializeTask(XmlNode elementNode)
		{
			if (OptionSetUtil.OptionSetExists(Project, BuildTypeName))
			{
				Log.Warning.WriteLine($"{0}{1}BuildOptionSet '{2}' exists and will be overwritten.", LogPrefix, Location, BuildTypeName);
			}

			_buildTypeElement = new BuildTypeElement(FromBuildType);
			_buildTypeElement.Project = Project;
			_buildTypeElement.LogPrefix = LogPrefix;
			_buildTypeElement.FromOptionSetName = FromBuildType;

			_buildTypeElement.Initialize(elementNode);

			 SetOption("remove.defines", RemoveOptions.Defines, splitlines: true);
			 SetOption("remove.cc.options", RemoveOptions.CcOptions);
			 SetOption("remove.as.options", RemoveOptions.AsmOptions);
			 SetOption("remove.link.options", RemoveOptions.LinkOptions);
			 SetOption("remove.lib.options", RemoveOptions.LibOptions);
		}

		private void SetOption(string name, PropertyElement value, bool splitlines = false)
		{
			if(value != null && value.Value != null)
			{
				var text = value.Value.TrimWhiteSpace();
				if(!String.IsNullOrEmpty(text))
				{
					_buildTypeElement.Options[name] = (splitlines ? text.LinesToArray() : text.ToArray()).ToString(Environment.NewLine);
				}
			}
		}

		protected override void ExecuteTask()
		{
			OptionSet os = _buildTypeElement as OptionSet;
			os.Options["buildset.name"] = BuildTypeName;
			Project.NamedOptionSets[BuildTypeName + "-temp"] = _buildTypeElement as OptionSet;

			GenerateBuildOptionset.Execute(Project, os, BuildTypeName + "-temp");
		}

		public static OptionSet ExecuteTask(Project project, string buildTypeName, string fromBuildType, OptionSet flags, bool saveFinalToproject = true, Location location = null, string name = null)
		{
			string finalFromName;
			var fromoptionset = BuildTypeElement.GetFromOptionSetName(project, fromBuildType, out finalFromName, location, name);

			OptionSet os = new OptionSet(fromoptionset);
			if (flags != null)
			{
				os.Append(flags);
			}
			os.Options["buildset.name"] = buildTypeName;
			project.NamedOptionSets[buildTypeName + "-temp"] = os;

			return GenerateBuildOptionset.Execute(project, os, buildTypeName + "-temp", saveFinalToproject: false);
		}
	}

	/// <summary></summary>
	[ElementName("buildtype", StrictAttributeCheck = true, NestedElementsCheck = false)]
	public class BuildTypeElement : OptionSet
	{
		public delegate void OnVerifyOptionEx(string optionName, XmlNode node);

		public OnVerifyOptionEx VerifyOptionEx;
		OptionSet BuildOptionSet;
		ModuleBaseTask _module; 
		private XmlNode _elementNode;

		public string LogPrefix = "[buildoptions] ";

		public BuildTypeElement(ModuleBaseTask module, OptionSet buildOptionSet)
			: base() 
		{
			_module = module;
			BuildOptionSet = buildOptionSet;
		}

		public BuildTypeElement(string fromBuildType)
			: base()
		{
			FromOptionSetName = fromBuildType;
			BuildOptionSet = null;
		}

		/// <summary>If true then the option will be included; otherwise skipped. Default is "true".</summary>
		[TaskAttribute("if")]
		public bool IfDefined { get; set; } = true;

		/// <summary>Opposite of if.  If false then the option will be included; otherwise skipped. Default is "false".</summary>
		[TaskAttribute("unless")]
		public bool UnlessDefined { get; set; } = false;

		public override void Initialize(XmlNode elementNode)
		{
			if (Project == null)
			{
				throw new InvalidOperationException("Element has invalid (null) Project property.");
			}

			// Save position in buildfile for reporting useful error messages.
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
				IfDefined = true;
				UnlessDefined = false;

				foreach (XmlAttribute attr in elementNode.Attributes)
				{
					switch (attr.Name)
					{
						case "if":
							IfDefined = ElementInitHelper.InitBoolAttribute(this, attr.Name, attr.Value);
							break;
						case "unless":
							UnlessDefined = ElementInitHelper.InitBoolAttribute(this, attr.Name, attr.Value);
							break;
						default:
							break;
					}
				}

				InitializeElement(elementNode);
			}
			catch (Exception ex)
			{
				throw new ContextCarryingException(ex, Name, Location);
			}
		}

		/// <summary>Add all the child option elements.</summary>
		/// <param name="elementNode">XML node to initialize from.</param>
		protected override void InitializeElement(XmlNode elementNode)
		{
			if (IfDefined && !UnlessDefined)
			{
				if (_elementNode == null)
				{
					_elementNode = elementNode;
				}
				else
				{
					foreach (XmlNode child in elementNode.ChildNodes)
					{
						_elementNode.AppendChild(child);
					}
				}

				if (_module == null)
				{
					InternalInitializeElement(String.Empty);
				}
			}
		}

		internal static OptionSet GetFromOptionSetName(Project project, string fromOptionSetName, out string finalFromName, Location location = null, string name = null)
		{
			OptionSet optionSet = null;

			if (fromOptionSetName == "Library")
			{
				fromOptionSetName = "StdLibrary";
			}
			else if (fromOptionSetName == "Program")
			{
				fromOptionSetName = "StdProgram";
			}

			optionSet = OptionSetUtil.GetOptionSet(project, fromOptionSetName);

			string prefix = $"[{name}] ";
			if (optionSet == null)
			{
				throw new BuildException($"OptionSet '{fromOptionSetName}' does not exist.", location);
			}

			if (OptionSetUtil.GetBooleanOption(optionSet, "buildset.finalbuildtype"))
			{
				string protoSetName = OptionSetUtil.GetOptionOrDefault(optionSet, "buildset.protobuildtype.name", String.Empty);

				if (String.IsNullOrEmpty(protoSetName))
				{
					throw new BuildException($"{prefix}Build OptionSet '{fromOptionSetName}' does not specify proto-OptionSet name.", location);
				}
				optionSet = OptionSetUtil.GetOptionSet(project, protoSetName);

				if (optionSet == null)
				{
					throw new BuildException($"{prefix}Proto-OptionSet '{protoSetName}' specified by Build OptionSet '{fromOptionSetName}' does not exist.", location);
				}
				else if (OptionSetUtil.GetBooleanOption(optionSet, "buildset.finalbuildtype"))
				{
					throw new BuildException
					(
						$"Proto OptionSet '{protoSetName}' specified in Build OptionSet '{fromOptionSetName} is not a proto-buildtype optionset, and can not be used to generate build types.\n" +
						"\n" +
						$"Possible reasons: '{protoSetName}' might be derived from a non-proto-buildtype optionset.\n" +
						"\n" +
						"Examples of proto-buildtypes are 'config-options-program', 'config-options-library' etc.\n" +
						"and any other optionsets that are derived from proto-buildtypes.",
						location
					);
				}

				fromOptionSetName = protoSetName;
			}
			finalFromName = fromOptionSetName;

			return optionSet;
		}

		internal void InternalInitializeElement(string basebuildtype)
		{
			if (_elementNode != null || Options.Count > 0)
			{
				if (_module != null)
				{
					FromOptionSetName = basebuildtype != null ? basebuildtype : _module.BuildType;

					Project = Project ?? _module.Project;
				}

				FromOptionSetName = Project.ExpandProperties(FromOptionSetName);

				string finalFromOptionSetName;
				var fromOptionSet = GetFromOptionSetName(Project, FromOptionSetName, out finalFromOptionSetName, Location, Name);

				FromOptionSetName = null;

				OptionSet inputOptions = null; // These options can be set by derived Module task
				if (Options.Count > 0)
				{
					inputOptions = new OptionSet(this);
				}

				Append(fromOptionSet);

				if (inputOptions != null)
				{
					Append(inputOptions);
				}

				if (_elementNode != null)
				{
					VerifyOptionNames(_elementNode);

					SetBuildOptions(_elementNode, FromOptionSetName);
				}

				FromOptionSetName = finalFromOptionSetName;

				if (BuildOptionSet != null)
				{
					BuildOptionSet.Append(this);
				}
			}
		}

		private void SetBuildOptions(XmlNode elementNode, string fromOptionSetName)
		{
			if (fromOptionSetName != null)
			{
				if (!Project.NamedOptionSets.TryGetValue(fromOptionSetName, out OptionSet fromSet))
				{
					throw new BuildException($"Unknown named option set '{fromOptionSetName}'.", Location.GetLocationFromNode(elementNode));
				}
				Append(fromSet);
			}

			foreach (XmlNode node in elementNode)
			{
				if (node.NodeType == XmlNodeType.Element)
				{
					if (node.Name == "option")
					{
						ParseOption(node);
					}
					else if (node.Name != "remove")
					{
						throw new BuildException($"Illegal element '{node.Name}' inside buildtype element.", Location.GetLocationFromNode(node));
					}
				}
			}

		}

		private void VerifyOptionNames(XmlNode elementNode)
		{
			foreach (XmlNode node in elementNode)
			{
				if (node.NodeType == XmlNodeType.Element)
				{
					if (node.Name == "option")
					{
						var attr = node.Attributes["name"];
						if (attr != null && attr.Value != null) 
						{
							var name = Project.ExpandProperties(attr.Value);
							if(INVALID_OPTIONS.Contains(name,StringComparer.InvariantCultureIgnoreCase))
							{
								Log.Warning.WriteLine($"{Location.GetLocationFromNode(node)} invalid option name='{name}' in BuildType optionset, did you mean 'buildset.{name.ToLowerInvariant()}'?");
							} 
							else if(name.StartsWith("buildset.") && INVALID_OPTIONS.Any(val => 0 == String.Compare(val, 0, name, 9, name.Length, StringComparison.InvariantCultureIgnoreCase)))
							{
								attr = node.Attributes["value"];
								var value = (attr != null && attr.Value != null) ? attr.Value : node.InnerText;
								if (value != null && value.Contains("@{StrReplace("))
								{
									Log.Warning.WriteLine($"{Location.GetLocationFromNode(node)}Use 'remove' options in buildtype definition, 'StrReplace()' in option '{name}' will likely not work as you expect.");
								}
							}
							VerifyOptionEx?.Invoke(name, node);
						}
					}
				}
			}
		}

		private static readonly List<string> INVALID_OPTIONS = new List<string>() { "as.options", "cc.options", "lib.options", "link.options" };

	}
}
