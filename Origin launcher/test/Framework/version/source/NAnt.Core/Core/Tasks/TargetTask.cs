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
// Bob Summerwill (bobsummerwill@ea.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

//-----------------------------------------------------------------------------
// TargetTask.cs
//
// A NAnt custom task which allows you to nest <target> definitions within
// tasks, so you can add targets to a project dynamically.  This task was
// originally written by Gerry Shaw.  Bob Summerwill then added support for
// all <target> attributes and moved it into the NAnt core.
//
//-----------------------------------------------------------------------------

using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks
{
	public class DynamicTarget : Target {
		public void SetXmlNode(XmlNode xmlNode) {
			_xmlNode = xmlNode;
			// useful for error messages
			IXmlLineInfo iLineInfo = xmlNode as IXmlLineInfo;
			if( null != iLineInfo && iLineInfo.HasLineInfo() )
			{
				Location = new Location(xmlNode.BaseURI, ((IXmlLineInfo)xmlNode).LineNumber, ((IXmlLineInfo)xmlNode).LinePosition);
			}
			else
			{
				Log.Warning.WriteLine(LogPrefix + "Missing line info in XmlNode. There is likely a bug in the task code. Please use a class which implements IXmlLineInfo.");
			}
		}
	}

	/// <summary>
	/// Create a dynamic target. 
	/// </summary>
	/// <remarks>
	/// <para>With Framework 1.x, you can declare &lt;target&gt; within a &lt;project&gt;, but not within any
	/// task. Moreover, the target name can't be variable. But with TargetTask, you can declare a target with variable
	/// name, or within any task that supports probing. You declare a dynamic target, and call it using &lt;call&gt;.
	/// </para>
	/// </remarks>
	/// <include file='Examples/CreateTarget/simple.example' path='example'/>
	[TaskName("target")]
	public class TargetTask : Task {
		bool _ifDefined = true;
		bool _unlessDefined = false;
		XmlNode _targetNode;

		/// <summary>The name of the target.</summary>
		[TaskAttribute("name", Required = true)]
		public string TargetName { get; set; }

		/// <summary>A space separated list of target names that this target depends on.</summary>
		[TaskAttribute("depends")]
		public string DependencyList { get; set; }

		/// <summary>The Target description.</summary>
		[TaskAttribute("description")]
		public string Description { get; set; }

		/// <summary>If true then the target will be executed; otherwise skipped. Default is "true".</summary>
		[TaskAttribute("if")]
		public override bool IfDefined
		{
			get { return _ifDefined; }
			set { _ifDefined = value; }
		}

		/// <summary>Prevents the target from being listed in the project help. Default is false.</summary>
		[TaskAttribute("hidden")]
		public bool Hidden { get; set; } = false;

		/// <summary>
		/// Override target with the same name if it already exists. Default is 'false'. 
		/// Depends on the 'allowoverride' setting in target it tries to override.
		/// </summary>
		[TaskAttribute("override")]
		public bool Override { get; set; } = false;

		/// <summary>Defines whether target can be overridden by other target with same name. Default is 'false'</summary>
		[TaskAttribute("allowoverride")]
		public bool AllowOverride { get; set; } = false;

		/// <summary>Style can be 'use', 'build', or 'clean'.   See <see cref="AutoBuildClean"/> 
		/// page for details.</summary>
		[TaskAttribute("style")]
		public string Style { get; set; }

		/// <summary>Opposite of if.  If false then the target will be executed; otherwise skipped. Default is "false".</summary>
		[TaskAttribute("unless")]
		public override bool UnlessDefined
		{
			get { return _unlessDefined; }
			set { _unlessDefined = value; }
		}

		[XmlTaskContainer("*")]
		protected override void InitializeTask(XmlNode taskNode) 
		{
			_targetNode= taskNode;
		}

		protected override void ExecuteTask() 
		{
			DynamicTarget target = new DynamicTarget();
			target.BaseDirectory = Project.BaseDirectory;
			target.Name = TargetName;
			target.Hidden = Hidden;
			target.UnlessDefined = UnlessDefined;
			target.IfDefined = IfDefined;
			target.Project = Project;
			target.Override = Override;
			target.AllowOverride = AllowOverride;
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
			Project.Targets.Add(target);
		}
	}
}
