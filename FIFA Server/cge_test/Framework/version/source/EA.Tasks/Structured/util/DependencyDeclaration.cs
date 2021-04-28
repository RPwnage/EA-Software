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
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using NAnt.Core.PackageCore;

namespace EA.Eaconfig.Structured
{
	// structured xml dependency syntax:
	// Package						- depends on package, later this should be converted to package use dependency or dependency on all runtime modules but that is outside scope of this class
	// Package/Module				- depends on specfic module of a package, assumed that module is in runtime group
	// Package/group/Module			- depends on specific module in specific group of specific package
	//
	// above style strings can be parsed using this helper class:
	//
	//	DependencyDeclaration dependencies = new DependencyDeclaration(dependencyString, ...);
	//	foreach (DependencyDeclaration.Package package in dependencies.Packages)
	//	{
	//		if (package.FullDependency)
	//		{
	//			... // assume all runtime modules, true when just package is specified
	//		}
	//		
	// 		foreach (DependencyDeclaration.Group group in package.Groups)
	// 		{
	// 			foreach (string module in group.Modules)
	// 			{
	// 				...
	// 			}
	//		}
	//
	//		... OR ...
	//
	//		Dictionary<BuildGroups, HashSet<string>> modulesNames = dependencies.GetPublicModuleNamesForPackage(package, Project);
	//	}
	//
	public class DependencyDeclaration
	{
		// a package that was declared as a dependency
		public class Package
		{	
			public IEnumerable<Group> Groups { get { return m_groups.Values; } }
			
			public readonly string Name;

			public bool FullDependency { get; internal set; } // if true it means this package was depended on without explicit modules dependency, up to context what this implies

			private Dictionary<BuildGroups, Group> m_groups = new Dictionary<BuildGroups, Group>();

			internal Package(string packageName)
			{
				Name = packageName;
				FullDependency = false;
			}

			internal Package(Package otherPackage)
			{
				Name = otherPackage.Name;
				FullDependency = otherPackage.FullDependency;
				foreach (Group otherGroup in otherPackage.Groups)
				{
					m_groups.Add(otherGroup.Type, new Group(otherGroup));
				}
			}

			internal Group GetOrAddGroup(BuildGroups groupType)
			{
				if (!m_groups.TryGetValue(groupType, out Group group))
				{
					group = new Group(groupType);
					m_groups.Add(groupType, group);
				}
				return group;
			}

			internal Package Combine(Package otherPackage)
			{
				Package combined = new Package(this);
				foreach (Group otherGroup in otherPackage.Groups)
				{
					Group group;
					if (combined.m_groups.TryGetValue(otherGroup.Type, out group))
					{
						combined.m_groups[otherGroup.Type] = group.Combine(otherGroup);
					}
					else
					{
						combined.m_groups.Add(otherGroup.Type, new Group(otherGroup));
					}
				}
				return combined;
			}
		}

		// a group which contains specific module dependencies
		public class Group
		{			
			public IEnumerable<string> Modules { get { return m_modules; } }

			public readonly BuildGroups Type;

			private HashSet<string> m_modules = new HashSet<string>();

			internal Group(BuildGroups groupTpe)
			{
				Type = groupTpe;
			}

			internal Group(Group otherGroup)
			{
				Type = otherGroup.Type;
				m_modules = new HashSet<string>(otherGroup.Modules);
			}

			internal void AddModule(string moduleName)
			{
				m_modules.Add(moduleName);
			}

			internal Group Combine(Group otherGroup)
			{
				Group combined = new Group(this);
				combined.m_modules = new HashSet<string>(combined.m_modules.Concat(otherGroup.m_modules));
				return combined;
			}
		}

		public IEnumerable<Package> Packages { get { return m_packages.Values; } }

		private readonly Dictionary<string, Package> m_packages = new Dictionary<string, Package>();

		private readonly Location m_errorContextLocation = null;
		private readonly string m_errorContextElementName = null;

		[Obsolete("Dependency declaration no longer take a Project parameter.")]
		public DependencyDeclaration(string dependencyString, Project project, string module) : this(dependencyString, (Location)null, null) {}

		// copy constructor
		public DependencyDeclaration(DependencyDeclaration other)
		{
			foreach (Package otherPackage in other.Packages)
			{
				m_packages.Add(otherPackage.Name, new Package(otherPackage));
			}
		}

		// parsing implementation constructor
		public DependencyDeclaration(string dependencyString, Location location = null, string elementName = null)
		{
			try
			{
				m_errorContextLocation = location;
				m_errorContextElementName = elementName;

				foreach (string dep in StringUtil.ToArray(dependencyString))
				{
					IList<string> depDetails = StringUtil.ToArray(dep, "/");
					if (depDetails.Count == 0)
					{
						throw new BuildException("Invalid dependency string: " + dep);
					}

					// get-or-add package to dictionary
					if (!m_packages.TryGetValue(depDetails[0], out Package package))
					{
						package = new Package(depDetails[0]);
						m_packages.Add(depDetails[0], package);
					}

					// add module and gorup information if specified
					BuildGroups depGroup = BuildGroups.runtime;
					string depModule = null;
					switch (depDetails.Count)
					{
						case 1: // package name only, continue to next loop as we have no modules to add
							package.FullDependency = true;
							continue;
						case 2: // package name + module name, group defaults to runtime
							depModule = depDetails[1];
							break;
						case 3: // package name + group name + module name, everything explicit
							if (!Enum.TryParse(depDetails[1], out depGroup))
							{
								ThrowContextError("Invalid dependency group '{0}' in dependency string '{1}'!", depDetails[1], dep);
								// unreachable, above function throws exception
							}
							depModule = depDetails[2];
							break;
						default:
							ThrowContextError("Invalid dependency description: {0}, valid values are 'package_name' or 'package_name/module_name' or 'package_name/group/module_name', where group is one of 'runtime, test, example, tool'.", dep);
							break; // unreachable, above function throws exception
					}
					package.GetOrAddGroup(depGroup).AddModule(depModule);
				}
			}
			catch (Exception e)
			{
				throw new BuildException("Failed to parse dependency string: " + dependencyString, e);
			}
		}

		public DependencyDeclaration Combine(DependencyDeclaration other)
		{
			DependencyDeclaration combined = new DependencyDeclaration(this);
			foreach (Package otherPackage in other.Packages)
			{
				if (combined.m_packages.TryGetValue(otherPackage.Name, out Package package))
				{
					combined.m_packages[otherPackage.Name] = package.Combine(otherPackage);
				}
				else
				{
					combined.m_packages.Add(otherPackage.Name, new Package(otherPackage));
				}
			}
			return combined;
		}

		// gets the public module names that this declaration refers to by referencing listed modules against
		// publically declared modules - errors if dependency is declare on modules that was not declared in
		// initialize.xml
		public Dictionary<BuildGroups, HashSet<string>> GetPublicModuleNamesForPackage(Package packageDependency, Project project)
		{
			if (PackageMap.Instance.MasterConfigHasNugetPackage(packageDependency.Name))
			{
				// NUGET-TODO hack to short circuit this for public(build)dependencies nuget auto converted to packages, we can't depend on this
				// we may need to forward these to downstream packages as nuget references
				return new Dictionary<BuildGroups, HashSet<string>>();
			}

			if (packageDependency.Name != project.Properties.GetPropertyOrFail("package.name"))
			{
				TaskUtil.Dependent(project, packageDependency.Name, Project.TargetStyleType.Use);
			}

			Dictionary<BuildGroups, HashSet<string>> packageModuleNames = Enum.GetValues(typeof(BuildGroups)).Cast<BuildGroups>().ToDictionary
			(
				grp => grp,
				grp => new HashSet<string>()
			);

			// runtime is special, doesn't necesarily need "package." or ".runtime." for property name
			IEnumerable<string> allRuntimeModules = (
				project.GetPropertyValue(PropertyKey.Create("package.", packageDependency.Name, ".runtime.buildmodules")) ??
				project.GetPropertyValue(PropertyKey.Create("package.", packageDependency.Name, ".buildmodules")) ??
				project.GetPropertyValue(PropertyKey.Create(packageDependency.Name, ".buildmodules"))
				).ToArray();

			if (packageDependency.FullDependency)
			{
				if (allRuntimeModules.Any())
				{
					foreach (string moduleName in allRuntimeModules)
					{
						packageModuleNames[BuildGroups.runtime].Add(moduleName);
					}
				}
				else
				{
					packageModuleNames[BuildGroups.runtime].Add(packageDependency.Name);
				}
			}

			foreach (Group group in packageDependency.Groups)
			{
				IEnumerable<string> allGroupModules = group.Type == BuildGroups.runtime ?
					allRuntimeModules :
					project.GetPropertyValue("package." + packageDependency.Name + "." + group.Type.ToString() + ".buildmodules").ToArray();

				foreach (string moduleName in group.Modules)
				{
					if (!allGroupModules.Contains(moduleName))
					{
						ThrowContextError("Declaring dependency on module '{0}' from package '{1}' but this package does not declare this module publically in it's Initialize.xml!", moduleName, packageDependency.Name);
					}
					packageModuleNames[group.Type].Add(moduleName);
				}
			}

			return packageModuleNames;
		}

		// helper class can be used in multiple contexts where different levels of target error reporting can be done, single function to throw
		// appropriate context error based on context information available
		private void ThrowContextError(string error, params object[] args)
		{				
			string errorMessage = String.Format(error, args);
			if (m_errorContextElementName != null)
			{
				throw new BuildException($"[{m_errorContextElementName}] {errorMessage}", m_errorContextLocation);
			}
			else
			{
				throw new BuildException(errorMessage, m_errorContextLocation);
			}
		}
	}
}
