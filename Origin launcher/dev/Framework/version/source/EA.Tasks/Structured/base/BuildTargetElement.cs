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
using NAnt.Core.Structured;
using NAnt.Core.Tasks;

using System;
using System.Xml;

namespace EA.Eaconfig.Structured
{
	/// <summary></summary>
	[ElementName("BuildTarget", StrictAttributeCheck = true, NestedElementsCheck=true, Mixed = true)]
	public class BuildTargetElement : ConditionalElementContainer
	{
		public BuildTargetElement(string defaultTargetName)
			: base()
		{
			Target = new TargetElement(defaultTargetName);
		}

		/// <summary>Sets the target name</summary>
		[TaskAttribute("targetname")]
		public string TargetName { get; set; }

		/// <summary>Autoconvert target to command when needed in case command is not defined. Default is 'true'</summary>
		[TaskAttribute("nant-build-only")]
		public bool NantBuildOnly { get; set; } = false;

		/// <summary>Sets the target</summary>
		[Property("target", Required = false)]
		public TargetElement Target { get; }

		/// <summary>Sets the comand</summary>
		[Property("command", Required = false)]
		public ConditionalPropertyElement Command { get; } = new ConditionalPropertyElement();

		protected override void InitializeElement(XmlNode node)
		{
			base.InitializeElement(node);

			if (Target.HasTargetElement && !String.IsNullOrEmpty(TargetName))
			{
				throw new BuildException($"Can not define both targetname[{TargetName}] and <target> element.");
			}
		}
	}

	[ElementName("PostBuildTarget", StrictAttributeCheck = true, NestedElementsCheck=true, Mixed = true)]
	public class PostBuildTargetElement : BuildTargetElement
	{
		public PostBuildTargetElement(string defaultTargetName)
			: base(defaultTargetName)
		{
		}

		[TaskAttribute("skip-if-up-to-date")]
		public bool SkipIfUpToDate { get; set; } = false;
	}

	/// <summary>Create a dynamic target. This task is a Framework 2 feature</summary>
	[ElementName("target", StrictAttributeCheck=true, NestedElementsCheck=false)]
	public class TargetElement : ElementWithCondition {
		XmlNode _targetNode;

		public TargetElement(string targetName) : base()
		{
			TargetName = targetName;
		}
		public string TargetName { get; private set; }

		/// <summary>A space separated list of target names that this target depends on.</summary>
		[TaskAttribute("depends")]
		public string DependencyList { get; set; }

		/// <summary>The Target description.</summary>
		[TaskAttribute("description")]
		public string Description { get; set; }

		/// <summary>Prevents the target from being listed in the project help. Default is true.</summary>
		[TaskAttribute("hidden")]
		public bool Hidden { get; set; } = true;

		/// <summary>Style can be 'use', 'build', or 'clean'.   See 'Auto Build Clean' 
		/// page in the Reference/NAnt/Fundamentals section of the help doc for details.</summary>
		[TaskAttribute("style")]
		public string Style { get; set; }

		/// <summary>This attribute is not used. Adding it here only to prevent build breaks after turning on strict attribute check.</summary>
		[TaskAttribute("name")]
		public string DummyName { get; set; }

		public bool HasTargetElement
		{
			get { return _targetNode != null; }
		}

		public override void Initialize(XmlNode elementNode)
		{
			base.Initialize(elementNode);
		}

		protected override void InitializeElement(XmlNode elementNode) 
		{
			_targetNode= elementNode;

			if (!String.IsNullOrEmpty(DummyName))
			{
				Log.ThrowWarning
				(
					Log.WarningId.SyntaxError, Log.WarnLevel.Normal,
					$"{Location} Structured element 'target' does not have attribute 'name', this attribute is ignored."
				);
			}
		}

		[XmlTaskContainer()]
		public void Execute(string targetName) 
		{
			if (!string.IsNullOrEmpty(targetName))
			{
				TargetName = targetName;
			}
	
			DynamicTarget target = new DynamicTarget();
			target.BaseDirectory = Project.BaseDirectory;
			target.Name = TargetName;
			target.Hidden = Hidden;
			target.UnlessDefined = UnlessDefined; 
			target.IfDefined = IfDefined;
			target.Project = Project;
			if (DependencyList != null) 
			{
				target.DependencyList = DependencyList;
			}
			if (Description != null) 
			{
				target.Description = Description;
			}
			if (Style != null) 
			{
				target.Style = Style;
			}

			target.SetXmlNode(_targetNode);
			Project.Targets.Add(target, ignoreduplicate: true);
		}
	}
}
