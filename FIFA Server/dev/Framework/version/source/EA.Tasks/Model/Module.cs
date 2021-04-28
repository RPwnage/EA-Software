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
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;

using EA.Eaconfig;
using EA.Eaconfig.Build;

namespace EA.FrameworkTasks.Model
{
	public abstract class Module : BitMask, IModule
	{
		// DAVE-FUTURE-REFACTOR-TODO: seems like these types duplicate whats in BuildType already - why do we need two definitions?

		public const uint Native = 1 << 0;
		public const uint DotNet = 1 << 1;
		public const uint WinRT = 1 << 2;
		public const uint Utility = 1 << 3;
		public const uint MakeStyle = 1 << 4;
        public const uint ExternalVisualStudio = 1 << 5;

		public const uint Managed = 1 << 6;
		public const uint CSharp = 1 << 7;

		public const uint Program = 1 << 10;
		public const uint Library = 1 << 11;
		public const uint DynamicLibrary = 1 << 12;

		public const uint ExcludedFromBuild = 1 << 13;
		public const uint Python = 1 << 14;

		public const uint ForceMakeStyleForVcproj = 1 << 15; // This is bit is for internal use only and is intended to be used in conjunction with
															// other bits (usually Program, Library, etc ...)
		public const uint Apk = 1 << 16;
		public const uint Java = 1 << 17;
		public const uint Aar = 1 << 18;

		public string LogPrefix
		{
			get { return '[' + Package.Name + (Name != Package.Name ? " - " + Name : String.Empty) + "] "; }
		}

		public static readonly IModuleEqualityComparer EqualityComparer = new IModuleEqualityComparer();

		protected Module(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, IPackage package, bool isreal=true)
		{
			Name = name;
			GroupName = groupname;
			GroupSourceDir = groupsourcedir;
			BuildGroup = buildgroup;
			Configuration = configuration;
			Package = package;
			Key = MakeKey(name, buildgroup, package);

			CopyLocalFiles = new List<CopyLocalInfo>();
			RuntimeDependencyFiles = new HashSet<string>();

			Macros = new MacroMap(package.Macros)
			{
				{ "modulename", Name },
				{ "module.name", Name },
				{ "group", buildgroup.ToString() },
				{ "groupname", GroupName }
			};

			if (configuration.Name != package.ConfigurationName)
			{
				string msg = String.Format("Package '{0}-{1}' configuration name '{2}' does not match module '{3}' configuration name '{4}'.", package.Name, package.Version, package.ConfigurationName, name, configuration.Name);
				throw new BuildException(msg);
			}

			OutputName = Name;
			
			if (isreal)
			{
				_dependents = new DependentCollection(this);
				BuildSteps = new List<BuildStep>();
				_tools = new List<Tool>();

				Assets = new FileSet();
				SnDbsRewriteRulesFiles = new FileSet();
				ExcludedFromBuild_Files = new FileSet();
				ExcludedFromBuild_Sources = new FileSet();

				Package.AddModule(this);

				if (Package.Project != null)
				{
					OutputName = Package.Project.Properties[groupname + ".outputname"] ?? Name;
				}
			}
			else
			{
				_dependents = new DependentCollection(this);
			}
        }

		public static string MakeKey(string name, BuildGroups group, IPackage package)
		{
			return package.Key + ":" + group + "-" + name;
		}

		public static string MakeKey(string name, BuildGroups group, string packageName, string packageVersion, string config)
		{
			return Model.Package.MakeKey(packageName, packageVersion, config) + ":" + group + "-" + name;
		}

		public string Name { get; }

		public string GroupName { get; }

		public string GroupSourceDir { get; }

		public BuildGroups BuildGroup { get; }

		public string Key { get; protected set; }

		public IPackage Package { get; }

		public Configuration Configuration { get; }

		public DependentCollection Dependents { get { return _dependents; } }

		public PathString OutputDir { get; set; }

		public String OutputName { get; }

		public PathString IntermediateDir { get; set; }

		public PathString GenerationDir { get; set; }

		public PathString ScriptFile { get; set; }

		public bool DeployAssets { get; set; }

		public abstract CopyLocalType CopyLocal { get; }
		public abstract CopyLocalInfo.CopyLocalDelegate CopyLocalDelegate { get; }
		public abstract bool CopyLocalUseHardLink { get; }

        public List<CopyLocalInfo> CopyLocalFiles { get; set; }

		public HashSet<string> RuntimeDependencyFiles { get; set; }

		public FileSet Assets;
		public FileSet SnDbsRewriteRulesFiles;

		public readonly List<BuildStep> BuildSteps;

		public MacroMap Macros { get; private set; }

		public PathString ModuleConfigGenDir => PathString.MakeCombinedAndNormalized
		(
			Package.PackageConfigGenDir.Path,
			(BuildGroup == BuildGroups.runtime ? "" : BuildGroup.ToString() + Path.DirectorySeparatorChar) + Name
		);

		public PathString ModuleConfigBuildDir => PathString.MakeCombinedAndNormalized
		(
			Package.PackageConfigGenDir.Path,
			(BuildGroup == BuildGroups.runtime ? "" : BuildGroup.ToString() + Path.DirectorySeparatorChar)  + Name
		);

		public IPublicData Public(IModule parentModule)
		{
			IPublicData data = null;

			if (!Package.TryGetPublicData(out data, parentModule, this))
			{
				data = null;
			}
			return data;
		}

		public IEnumerable<Tool> Tools
		{
			get { return _tools; }
		}

		public void SetTool(Tool value)
		{
			if (value != null)
			{
				int ind = (_tools).FindIndex(tool => tool.ToolName == value.ToolName);
				if (ind >= 0)
				{
					_tools[ind] = value;
				}
				else
				{
					// This might look wasteful but we have parallel code asking for the most common tools (lib and link) while tools could be added
					var newTools = new List<Tool>(_tools);
					newTools.Add(value);
					_tools = newTools;
				}
			}
		}

		public void ReplaceTool(Tool oldTool, Tool value)
		{
			if (value != null)
			{
				var tname = (oldTool != null) ? oldTool.ToolName : value.ToolName;
				int ind = (_tools).FindIndex(tool => tool.ToolName == tname);
				if (ind >= 0)
				{
					_tools[ind] = value;
				}
				else
				{
					// This might look wasteful but we have parallel code asking for the most common tools (lib and link) while tools could be added
					var newTools = new List<Tool>(_tools);
					newTools.Add(value);
					_tools = newTools;
				}
			}
		}


		public void AddTool(Tool value)
		{
			if (value != null)
			{
				// This might look wasteful but we have parallel code asking for the most common tools (lib and link) while tools could be added
				var newTools = new List<Tool>(_tools);
				newTools.Add(value);
				_tools = newTools;
			}
		}

		public bool CollectAutoDependencies 
		{
			get { return IsKindOf(CSharp | DynamicLibrary | Program | Java); }
		}

		public ISet<IModule> ModuleSubtree(uint dependencytype, ISet<IModule> modules=null)
		{
			if (modules == null)
			{
				modules = new HashSet<IModule>(Module.EqualityComparer);
			}
			return modules;
		}

		public readonly IDictionary<string, string> MiscFlags = new Dictionary<string, string>();

		private List<Tool> _tools;

		// Data for Visual Studio and other IDEs not directly related to build, but reflected in IDE.
		public readonly FileSet ExcludedFromBuild_Files;
		public readonly FileSet ExcludedFromBuild_Sources;
		private readonly DependentCollection _dependents;

		public override string ToString()
		{
			return Key;
		}

		public IModule[] DependencyPathTo(IModule otherModule)
		{
			Stack<IModule> currentPath = new Stack<IModule>();
			HashSet<IModule> visited = new HashSet<IModule>();

			currentPath.Push(this); // don't add owner to visited, we might be trying to find a cyclic path back to ourself
			foreach (Dependency<IModule> dep in Dependents)
			{
				if (PathToRecursive(dep.Dependent, otherModule, ref currentPath, ref visited))
				{
					return currentPath.Reverse().ToArray();
				}
			}
			currentPath.Pop();

			return currentPath.Reverse().ToArray();
		}

		private static bool PathToRecursive(IModule visitModule, IModule otherModule, ref Stack<IModule> currentPath, ref HashSet<IModule> visited)
		{
			if (visited.Contains(visitModule))
			{
				return false;
			}
			visited.Add(visitModule);

			currentPath.Push(visitModule);
			if (visitModule == otherModule)
			{
				return true;
			}

			foreach (Dependency<IModule> dep in visitModule.Dependents)
			{
				if (PathToRecursive(dep.Dependent, otherModule, ref currentPath, ref visited))
				{
					return true;
				}
			}
			currentPath.Pop();

			return false;
		}
	}

	public class IModuleEqualityComparer : IEqualityComparer<IModule>
	{
		public bool Equals(IModule x, IModule y)
		{
			return x.Key == y.Key;
		}

		public int GetHashCode(IModule obj)
		{
			return obj.Key.GetHashCode();
		}
	}
}
