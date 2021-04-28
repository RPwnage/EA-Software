// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2002 Gerry Shaw
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
// Ian MacLean (ian_maclean@another.com)
// Scott Hernandez (ScottHernandez@hotmail.com)
// William E. Caputo (wecaputo@thoughtworks.com | logosity@yahoo.com)

using System;
using System.Xml;
using System.Collections.Specialized;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core
{
	public class Target : Element, ICloneable, IComparable
	{
		internal Target()
		{
		}

		/// <summary>Copy constructor.</summary>
		/// <param name="t">The target to copy values from.</param>
		protected Target(Target t) : base(t)
		{
			Name = t.Name;
			Description = t.Description;
			Style = t.Style;
			Override = t.Override;
			AllowOverride = t.AllowOverride;
			BaseDirectory = t.BaseDirectory;
			Dependencies = t.Dependencies;
			IfDefined = t.IfDefined;
			UnlessDefined = t.UnlessDefined;
			if (t.BaseTarget != null)
			{
				BaseTarget = t.BaseTarget.Copy();
			}
			HasExecuted = false;
		}

		/// <summary>The name of the target.</summary>
		/// <remarks>
		///   <para>Hides Element.Name to have <c>Target</c> return the name of target, not the name of XML element - which would always be <c>target</c>.</para>
		///   <para>Note: Properties are not allowed in the name.</para>
		/// </remarks>
		[TaskAttribute("name", Required = true, ExpandProperties = false)]
		public new string Name { get; set; } = null;

		/// <summary>If true then the target will be executed; otherwise skipped. Default is "true".</summary>
		[TaskAttribute("if")]
		public bool IfDefined { get; set; } = true;

		/// <summary>Opposite of if.  If false then the target will be executed; otherwise skipped. Default is "false".</summary>
		[TaskAttribute("unless")]
		public bool UnlessDefined { get; set; } = false;

		/// <summary>The Target description.</summary>
		[TaskAttribute("description")]
		public string Description { set; get; } = null;

		/// <summary>Framework 2 packages only: Style can be 'use', 'build', or 'clean'.  Default value is either 'use' or inherited from parent target. See <see cref="AutoBuildClean"/> for details.</summary>
		[TaskAttribute("style")]
		public string Style { set; get; } = null;

		/// <summary>A space separated list of target names that this target depends on.</summary>
		[TaskAttribute("depends")]
		public string DependencyList
		{
			set
			{
				foreach (string str in value.Split(new char[] { ' ', ',', '\t', '\n' }))
				{ // TODO: change this to just ' ' before 1.0
					string dependency = str.Trim();
					if (dependency.Length > 0)
					{
						Dependencies.Add(dependency);
					}
				}
			}
		}

		/// <summary>Prevents the target from being listed in the project help. Default is false.</summary>
		[TaskAttribute("hidden")]
		public bool Hidden { get; set; } = false;

		/// <summary>Override target with the same name if it already exists.</summary>
		[TaskAttribute("override")]
		public bool Override { get; set; } = false;

		/// <summary>Override target with the same name if it already exists.</summary>
		[TaskAttribute("allowoverride")]
		public bool AllowOverride { get; set; } = false;

		/// <summary> Overridden base target </summary>
		public Target BaseTarget { get; set; } = null;

		/// <summary>The base directory to use when executing tasks in this target.</summary>
		public string BaseDirectory { get; set; } = null;

		/// <summary>Indicates if the target has been executed.</summary>
		/// <remarks>
		///   <para>Targets that have been executed will not execute a second time.</para>
		/// </remarks>
		public bool HasExecuted { get; private set; } = false;

		/// <summary>A collection of target names that must be executed before this target.</summary>
		public StringCollection Dependencies { get; } = new StringCollection();

		/// <summary>The XML used to initialize this Target.</summary>
		protected XmlNode TargetNode
		{
			get { return _xmlNode; }
		}

		public virtual int CompareTo(object obj)
		{
			return CompareTo((Target)obj);
		}

		public virtual int CompareTo(Target target)
		{
			return String.Compare(this.Name, target.Name);
		}

		public bool Equals(Target other)
		{
			if (this == other)
				return true;

			if (IfDefined != other.IfDefined || UnlessDefined != other.UnlessDefined || Hidden != other.Hidden || Override != other.Override || AllowOverride != other.AllowOverride)
				return false;

			return _xmlNode.ToString().Equals(other._xmlNode.ToString());

		}

		/// <summary>The prefix used when sending messages to the log.</summary>
		public virtual string LogPrefix
		{
			get
			{
				string prefix = "[" + Name + "] ";
				return prefix;
			}
		}

		public bool TargetSuccess = true;

		/// <summary>Executes dependent targets first, then the target.</summary>
		public virtual void Execute(Project project)
		{
			if (!HasExecuted && IfDefined && !UnlessDefined)
			{
				// set right at the start or a <call> task could start an infinite loop
				HasExecuted = true;

				Project.TargetStyleType styleType = Project.TargetStyleType.Use;
				if (Name == "build" || Name == "buildall")
				{
					styleType = Project.TargetStyleType.Build;
				}
				else if (Name == "clean" || Name == "cleanall")
				{
					styleType = Project.TargetStyleType.Clean;
				}
				// This else is to prevent Style overriding build/buildall/clean/cleanall // TODO this seems hacky to check the target name, why is this necessary?
				else if (Style != null)
				{
					if (Style == "build")
					{
						styleType = Project.TargetStyleType.Build;
					}
					else if (Style == "clean")
					{
						styleType = Project.TargetStyleType.Clean;
					}
					else if (Style == "use")
					{
						styleType = Project.TargetStyleType.Use;
					}
				}

                Project targetContextProject = new Project(project, this, styleType);
				{
					targetContextProject.Properties.UpdateReadOnly("target.name", Name, local: true, inheritable: true);
					foreach (string targetName in Dependencies)
					{
						if (targetContextProject.Targets.TryGetValue(targetName, out Target target) && target != null)
						{
							target.Execute(targetContextProject);
						}
						else
						{
							throw new BuildException(String.Format("Unknown dependent target '{0}' of target '{1}'", targetName, Name), Location);
						}
					}

					try
					{
						TargetSuccess = true;

						Log.Info.WriteLine("   [target] {0}", Name);

						// select all the task elements and execute them
						foreach (XmlNode taskNode in _xmlNode)
						{
							if (taskNode.NodeType == XmlNodeType.Element)
							{
								Task task = targetContextProject.CreateTask(taskNode, this);
								task.Execute();
							}
						}
					}
					catch (BuildException e)
					{
						// if the exception was anything lower than a BuildException than its an internal error
						TargetSuccess = false;
						e.SetLocationIfMissing(Location);
						throw;
					}
					catch (ContextCarryingException e)
					{
						TargetSuccess = false;

						e.PushNAntStackFrame(Name, NAntStackScopeType.Target, Location);

						//throw e instead of throw to reset the stacktrace
						throw e;
					}
					catch (Exception e)
					{
						throw new ContextCarryingException(e, Name, Location);
					}
				}
			}
		}

		/// <summary>
		/// Creates a deep copy by calling Copy().
		/// </summary>
		/// <returns></returns>
		object ICloneable.Clone()
		{
			return (object)Copy();
		}

		/// <summary>
		/// Creates a new (deep) copy.
		/// </summary>
		/// <returns>A copy with the _hasExecuted set to false. This allows the new Target to be Executed.</returns>
		public Target Copy()
		{
			return new Target(this);
		}

		protected override void InitializeElement(XmlNode elementNode)
		{
			_xmlNode = elementNode;
		}
	}
}
