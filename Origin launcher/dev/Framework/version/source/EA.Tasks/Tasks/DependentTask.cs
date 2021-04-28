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
using System.Linq;
using System.Text;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Shared.Properties;

using EA.FrameworkTasks.Model;

using ThreadPoolTask = System.Threading.Tasks.Task;
using EA.Eaconfig.Modules;
using LibGit2Sharp;
using EA.Tasks.Model;
using NAnt.NuGet;
using System.IO;

namespace EA.FrameworkTasks
{

	/// <summary>This task indicates that another package needs to be loaded as a dependency</summary>
	/// <remarks>
	///   <para>
	///   This task looks in the named package's <b>scripts</b> folder 
	///   and executes the <b>Initialize.xml</b> if the package has one.
	///   The file is loaded the same as using a NAnt <b>include</b> Task.
	///   </para><para>
	///   The <c>initialize.xml</c> file is a mechanism to expose information
	///   about a package to other packages that are using it's contents.
	///   It does this by defining properties that describe itself.
	///   </para><para>
	///   Any code required for initializing a dependent package should also appear in the <b>Initialize.xml</b> file.
	///   </para><para>
	///   When NAnt executes the dependent task it will create these properties by default:
	///   <list type="table">
	///     <item>
	///       <term>package.all</term>
	///       <description>List of descriptive names of all the packages this build file depends on.</description>
	///     </item>
	///     <item>
	///       <term>package.<i>name</i>.dir</term>
	///       <description>Base directory of package <i>name</i>.</description>
	///     </item>
	///     <item>
	///       <term>package.<i>name</i>.version</term>
	///       <description>Specific version number of package <i>name</i>.</description>
	///     </item>
	///     <item>
	///       <term>package.<i>name</i>.frameworkversion</term>
	///       <description>The version number of the Framework the given package is designed for, 
	///       determined from the <b>&lt;frameworkVersion&gt;</b> of the package's Manifest.xml. 
	///       Default value is 1 if Manifest.xml doesn't exist.</description>
	///     </item>
	///     <item>
	///       <term>package.<i>name</i>.sdk-version</term>
	///       <description>Version of the SDK or third party files contained within the package. 
	///       By default this is just the package version number up to the first dash character, 
	///       unless it has been overridden in the package's initialize.xml file. 
	///       Also, if the package's version starts with "dev" this property will be undefined. 
	///       </description>
	///     </item>
	///   </list>
	///   </para><para>
	///   If you only want these default properties and don't want to load the rest of the package's initialize.xml file 
	///   you can set the attribute <b>initialize</b> to <b>false</b> when you call the dependent task.
	///   </para><para>
	///   By convention, packages should only define properties using
	///   names of the form <c>package.<i>name</i>.<i>property</i></c>
	///   Following this convention avoids namespace problems and makes
	///   it much easier for other people to use the properties your package sets.
	///   </para><para>
	///   One common use for the <b>Initialize.xml</b> file is for compiler packages to indicate where 
	///   the compiler is installed on the local machine by providing a property with the path.
	///   It is important to use these properties to compilers rather <c>package.<i>name</i>.dir</c> 
	///   because proxy SDK packages may point to a compiler installed elsewhere on the system.
	///   SDK packages typically define the property <c>package.<i>name</i>.appdir</c> 
	///   which is supposed to be the directory where the executable software resides.
	///   </para><para>
	///   This task must be embedded within a &lt;target> with style 'build'
	///   or 'clean' if this dependent, <b>Framework 2</b> package is to autobuildclean.
	///   </para>
	/// </remarks>
	/// <include file='Examples/Dependent/Dependent.example' path='example'/>
	[TaskName("dependent", NestedElementsCheck = true)]
	public class DependentTask : Task
	{
		private XmlNode _taskNode;
		private IPackage m_dependentPackage;
		private Project.TargetStyleType? _targetStyle = null;
		private IList<string> _dependencystack = null;
		internal List<ModuleDependencyConstraints> Constraints = null;

		public enum WarningLevel
		{
			NOTHING, WARNING, HALT
		}
		static WarningLevel _warningLevel = WarningLevel.NOTHING;

		public DependentTask() : base("dependent") { }

		/// <summary>The name of the package to depend on.</summary>
		[TaskAttribute("name", Required = true)]
		[StringValidator(Trim = true)]
		public string PackageName { get; set; }

		// The version of the package (eg. 1.02.03).
		/// <summary>
		/// Deprecated (Used in Framework 1)
		/// </summary>
		[TaskAttribute("version", Required = false)]
		[StringValidator(Trim = true)]
		public string PackageVersion { get; set; }

		/// <summary>If true the package will be automatically downloaded from the package server. Default is true.</summary>
		[TaskAttribute("ondemand", Required = false)]
		public bool OnDemand { get; set; } = true;

		/// <summary>If false the execution of the Initialize.xml script will be suppressed. Default is true.</summary>
		[TaskAttribute("initialize", Required = false)]
		public bool InitializeScript { get; set; } = true;

		/// <summary>Warning level for missing or mismatching version. Default is NOTHING (0).</summary>
		[TaskAttribute("warninglevel", Required = false)]
		public WarningLevel Level
		{
			get { return _warningLevel; }
			set { _warningLevel = value; }
		}

		/// <summary>Drop circular build dependency. If false throw on circular build dependencies</summary>
		[TaskAttribute("dropcircular", Required = false)]
		public bool DropCircular { get; set; } = false;

		public Project.TargetStyleType TargetStyle
		{
			get { return _targetStyle ?? Project.TargetStyle; }

			set { _targetStyle = value; }
		}

		public string Target { get; set; } = null;

		private string ConfigName
		{
			get
			{
				return Project.Properties[PackageProperties.ConfigNameProperty] ?? PackageMap.Instance.DefaultConfig;
			}
		}

		// HACK: Provide a new LogPrefix property to allow the <requires> task to change
		// the name of this task so we don't duplicate code.
		new public string LogPrefix
		{
			get
			{
				if (_logPrefix == null)
				{
					_logPrefix = base.LogPrefix;
				}
				return _logPrefix;
			}
			set { _logPrefix = value; }
		}	
		private string _logPrefix = null;

		public bool NonBlockingAutoBuildclean = false; // if true, spawn auto clean or build in a thread pool task and don't wait for it
		public bool PropagatedLinkDependency = false; // if true this dependent task is being called as part of a transitive dependency hierarchy, and we want to promote use dependencies to build dependencies
													  // for the purposes of loading package definitions to make sure we can can recursive walk all dependency and pass link dependencies upward transitively

		/// <summary>Initializes the task and checks for correctness.</summary>
		protected override void InitializeTask(XmlNode taskNode)
		{
			_taskNode = taskNode;
		}

		// For metrics processing
		public bool IsAlreadyCLeaned = false;

		protected override void ExecuteTask()
		{
			if (PackageVersion != null)
			{
				Log.ThrowDeprecation
				(
					Log.DeprecationId.DependentVersion, Log.DeprecateLevel.Normal,
					new object[] { Location },
					"{0} The 'version' attribute is no longer supported for <dependent>. Version will be determined by masterconfig.",
					Location
				);
			}

			// if this package shared a name with a nuget reference
			if (Project.UseNugetResolution && PackageMap.Instance.MasterConfigHasNugetPackage(PackageName))
			{
				if (XmlNode is null)
				{
					throw new InvalidOperationException($"Implicit DependentTask on NuGet package '{PackageName}' was called. DependentTask cannot be run for NuGet packages. This may be an internal Framework error or an issue with <taskdef> or <script> code.");
				}
#if NETFRAMEWORK
				// we can only allow this backwards compatbility if we know nant is .NET Framework and all
				// modules target .NET Framework - without this we don't know which .NET family to target
				// for this <dependent> on a nuget package. This code can be removed when dotnet core support
				// is not optional in Framework and only exists to allow <dependent name="SomeNugetPackage"/>
				// to continue working 
				if (!Project.NetCoreSupport) 
				{
					if (XmlNode is null)
					{
						throw new InvalidOperationException($"Implicit DependentTask on NuGet package '{PackageName}' was called. DependentTask cannot be run for NuGet packages. This may be an internal Framework error or an issue with <taskdef> or <script> code.");
					}

					string nantNetFamily = "net472"; // NUGET-TODO
					IEnumerable<NugetPackage> contentsAndDependenciesContents = PackageMap.Instance.GetNugetPackages(nantNetFamily, new string[] { PackageName });

					// NUGET-TODO: try to get similarish output to old nugetprotocol
					FileSet assemblies = new FileSet
					{
						Project = Project
					};
					NugetPackage contents = contentsAndDependenciesContents.First(package => package.Id.Equals(PackageName, StringComparison.OrdinalIgnoreCase));
					foreach (NugetPackage packageContents in contentsAndDependenciesContents)
					{
						foreach (FileInfo compileItem in packageContents.CompileItems)
						{
							assemblies.Include(compileItem.FullName);
						}
					}
					Project.NamedFileSets[$"package.{PackageName}.assemblies"] = assemblies;
					Project.Properties.UpdateReadOnly($"package.{PackageName}.nugetDir", contents.RootDirectory);
					return;
				}
#endif
				throw new BuildException($"A <dependent> task was called for NuGet package '{PackageName}'. This is not supported. Please convert usage of this package to a <nugetreference> (supported on both <taskdef> and modules).");
			}

			MasterConfig.IPackage masterPkg = PackageMap.Instance.GetMasterPackage(Project, PackageName);

			if (masterPkg.Name != PackageName && String.Compare(masterPkg.Name, PackageName, true) == 0)
			{
				throw new BuildException(
					string.Format("{0}{1} A <dependent> task was called for package '{2}'. " +
						"But this package name is specified in masterconfig using different case: '{3}'. Please correct your build script. " +
						"The case sensitive package name is being used as 'keys' in too many places including user tasks that is outside Framework's control.",
						LogPrefix, Location, PackageName, masterPkg.Name)
				);
			}

			Release release = PackageMap.Instance.FindOrInstallCurrentRelease(Project, PackageName);

			PackageVersion = release.Version; // PackageVersion as an attribute is deprecated, but the resolved version is used as an out value of sorts (see TaskUtil)

			bool buildableAndAutoBuildClean = PackageMap.Instance.IsPackageAutoBuildClean(Project, PackageName) && release.Manifest.Buildable;

			ValidatePackage(release, buildableAndAutoBuildClean);

			BuildGraph buildGraph = Project.BuildGraph();
			bool isReleaseSatisfied = false;
			lock (Project.PackageInitLock)
			{
				string releaseVersion = Project.Properties[$"package.{release.Name}.version"];
				if (releaseVersion != release.Version)
				{
					if (releaseVersion != null)
					{
						Log.Warning.WriteLine(LogPrefix + "{0} ({1}) {2} (different version already included)", release.FullName, release.Path, ConfigName);
					}
					PackageInitializer.AddPackageProperties(Project, release);
				}
				else
				{
					isReleaseSatisfied = true;
				}

				m_dependentPackage = buildGraph.GetOrAddPackage(release, ConfigName);
				if (Project.TryGetPackage(out IPackage parent))
				{
					uint depType = buildableAndAutoBuildClean && (TargetStyle == Project.TargetStyleType.Build || TargetStyle == Project.TargetStyleType.Clean) ?
						PackageDependencyTypes.Build :
						PackageDependencyTypes.Use;
					((Package)parent).TryAddDependentPackage(m_dependentPackage, depType);
				}
				else
				{
					// This is happening when dependent task is called before package task. WE can safely ignore this warning.
					//Log.Warning.WriteLine(LogPrefix + "[{0}]: Dependent task can't retrieve IPackage object from parent project for package '{1}-{2}'{3}{3}\t**** Most likely <package> task is not declared in the package build script! ****{3}\t**** Check {0}{3}", 
					//      Location, _dependentPackage.Name, _dependentPackage.Version, Environment.NewLine);
				}

				if (release.Manifest.Buildable)
				{
					m_dependentPackage.SetType(Package.Buildable);
					if (PackageMap.Instance.IsPackageAutoBuildClean(Project, PackageName))
					{
						m_dependentPackage.SetType(Package.AutoBuildClean);
					}
				}
			}

			if (InitializeScript && TargetStyle != Project.TargetStyleType.Clean)
			{
				PackageInitializer.IncludePackageInitializeXml(Project, release, Location);
			}

			PackageAutoBuildCleanMap packageAutoBuildCleanMap = buildGraph.PackageAutoBuildCleanMap;
			switch (TargetStyle)
			{
				case Project.TargetStyleType.Use:

					if (!packageAutoBuildCleanMap.IsCleanedNonBlocking(release.FullName, ConfigName ?? "no-config"))
					{
						if (isReleaseSatisfied)
						{
							Log.Debug.WriteLine(LogPrefix + "{0} {1} (already satisfied)", release.FullName, ConfigName);
						}
						else
						{
							if (Log.InfoEnabled)
								Log.Info.WriteLine(LogPrefix + "{0} ({1}) {2}", release.FullName, release.Path, ConfigName);
						}
					}
					break;

				case Project.TargetStyleType.Clean:

					using (PackageAutoBuildCleanMap.PackageAutoBuildCleanState autoCleanState = packageAutoBuildCleanMap.StartClean(release.FullName, ConfigName))
					{
						Action<string> reportCircular = null;
						if (DropCircular && Log.InfoEnabled)
						{
							reportCircular = (msg) => Log.Info.WriteLine("Dropped " + msg, Location);
						}
						else if (!DropCircular)
						{
							reportCircular = (msg) => Log.Warning.WriteLine("Dropped " + msg, Location);
						}

						if (false == FindCircularDependency(buildableAndAutoBuildClean, reportCircular))
						{
							if (autoCleanState.IsDone())
							{
								Log.Info.WriteLine(LogPrefix + "Package {0}-{1} ({2})already auto-cleaned", PackageName, PackageVersion, ConfigName);
								IsAlreadyCLeaned = true;
							}
							else
							{
								AutoBuildClean(release, buildableAndAutoBuildClean, TargetStyle, !Project.NoParallelNant && NonBlockingAutoBuildclean);
							}
						}
					}
					break;

				case Project.TargetStyleType.Build:

					using (PackageAutoBuildCleanMap.PackageAutoBuildCleanState autoBuildState = packageAutoBuildCleanMap.StartBuild(release.FullName, ConfigName))
					{
						Action<string> reportCircular = null;
						if (DropCircular && Log.InfoEnabled)
							reportCircular = (msg) => Log.Info.WriteLine("Dropped " + msg, Location);
						else if (!DropCircular)
							reportCircular = (msg) => throw new BuildException("Detected " + msg, Location);

						if (false == FindCircularDependency(buildableAndAutoBuildClean, reportCircular))
						{
							if (autoBuildState.IsDone())
							{
								Log.Info.WriteLine(LogPrefix + "Package {0}-{1} ({2}) already auto-built", PackageName, PackageVersion, ConfigName);
							}
							else
							{
								AutoBuildClean(release, buildableAndAutoBuildClean, TargetStyle, !Project.NoParallelNant && NonBlockingAutoBuildclean);
							}
						}
					}
					break;

				default:
					throw new BuildException(String.Format("Unknown NAnt Project TargetStype = '{0}'", TargetStyle));
			}
		}

		private void AutoBuildClean(Release release, bool buildableAndAutoBuildClean, Project.TargetStyleType targetStyle, bool nonBlocking)
		{
			if (nonBlocking)
			{
				PackageLoadWait loadCompletionTracker = Project.BuildGraph().PackageLoadCompletion;
				ThreadPoolTask autoBuildCleanTask = new ThreadPoolTask
				(
					() => 
					{
						try
						{
							AutoBuildClean(release, buildableAndAutoBuildClean, targetStyle);
						}
						finally
						{
							loadCompletionTracker.FinishedLoadTask();
						}
					}
				);
				bool wait = loadCompletionTracker.StartLoadTask(autoBuildCleanTask);
				autoBuildCleanTask.Start();
				if (wait)
				{
					loadCompletionTracker.Wait();
				}
			}
			else
			{
				AutoBuildClean(release, buildableAndAutoBuildClean, targetStyle);
			}
		}

		private void AutoBuildClean(Release release, bool buildableAndAutoBuildClean, Project.TargetStyleType targetStyle)
		{
			if (buildableAndAutoBuildClean)
			{
				MasterConfig.IPackage packageSpec = PackageMap.Instance.GetMasterPackage(Project, PackageName);

				NAntTask nantTask = new NAntTask
				{
					StartNewBuild = false,
					TopLevel = false, // this is a dependent - we can skip loading things that will only be needed at top level
					Project = Project,

					// This code used to use the build file path (ie: set nantTask.BuildFileName) but
					// the casing of this build file sometimes does not match the package itself
					// which can cause issues. It is more fool-proof to set the build package element
					// and let the package mapping system sort out the paths.
					BuildPackage = PackageName
				};

				// this block sets up dependenttask.dot-net-family property by trying to find a consistent family in the current
				// package (if we didn't inherit a value from dependning package) - this allows packages that did not set
				// a family to try and do the right thing based on what depends on them
				{
					string dependentTaskDotNetFamilyPropertyName = "dependenttask.dot-net-family";
					string dependentTaskDotNetFamilyPropertyValue = Project.Properties[dependentTaskDotNetFamilyPropertyName];
					DotNetFrameworkFamilies? dotNetCoreFamily = null;
					if (dependentTaskDotNetFamilyPropertyValue != null)
					{
						if (!Enum.TryParse(dependentTaskDotNetFamilyPropertyValue, ignoreCase: true, out DotNetFrameworkFamilies inheritedFamily))
						{
							throw new BuildException($"Property '{dependentTaskDotNetFamilyPropertyName}' has unrecognised value: {dependentTaskDotNetFamilyPropertyValue}. Expected values are: {String.Join(", ", Enum.GetValues(typeof(DotNetFrameworkFamilies)).Cast<DotNetFrameworkFamilies>().Select(e => e.ToString())).ToLowerInvariant()}.");
						}
						dotNetCoreFamily = inheritedFamily;
					}
					else
					{
						// get group names for this depedents based on target groups
						IEnumerable<string> groups = Project.Properties["eaconfig.build.group.names"]
							?.ToArray() ?? new string[] { "runtime" };

						// get module names, or implict package-as-module-name for legacy syntax
						IEnumerable<(string, string)> moduleGroupNames = groups.SelectMany
						(
							group => Project.Properties[group + ".buildmodules"]
								?.ToArray()
								.Select(moduleName => (moduleName, group + "." + moduleName))
								??
								new (string, string)[] { (PackageName, group) }
						);

						// make sure all modules are consistent in dot net family
						foreach ((string module, string groupName) in moduleGroupNames)
						{
							string moduleFamilyPropertyName = groupName + ".targetframeworkfamily";
							string moduleFamilyPropertyValue = Project.Properties[moduleFamilyPropertyName];
							if (moduleFamilyPropertyValue != null)
							{
								if (!Enum.TryParse(moduleFamilyPropertyValue, ignoreCase: true, out DotNetFrameworkFamilies moduleFamily))
								{
									throw new BuildException($"Module '{module}' has unrecognised value for '{moduleFamilyPropertyName}': {moduleFamilyPropertyValue}. Expected values are: {String.Join(", ", Enum.GetValues(typeof(DotNetFrameworkFamilies)).Cast<DotNetFrameworkFamilies>().Select(e => e.ToString())).ToLowerInvariant()}.");
								}

								if (!dotNetCoreFamily.HasValue)
								{
									dotNetCoreFamily = moduleFamily;
									continue;
								}

								// validate module is compatible with previous settings
								if (moduleFamily == DotNetFrameworkFamilies.Standard)
								{
									continue; // standard is always acceptable
								}
								else if (dotNetCoreFamily == DotNetFrameworkFamilies.Standard)
								{
									// upgrade from standard to a more specific .net family
									dotNetCoreFamily = moduleFamily;
								}
								else if (dotNetCoreFamily != moduleFamily)
								{
									// incompatibility - we can't set this, ignore and move on - incompatbility will fail layer at buildgraph creation
									// where we can give better errors
									dotNetCoreFamily = null;
									break;
								}
							}
						}
					}

					if (dotNetCoreFamily != null)
					{
						string familyAsString = dotNetCoreFamily.ToString().ToLowerInvariant();
						nantTask.TaskProperties["dependenttask.dot-net-family"] = familyAsString;
						Project.Properties["dependenttask.dot-net-family"] = familyAsString; // also store in current project so we don't recalculate
					}
				}

				nantTask.TaskProperties[PackageProperties.ConfigNameProperty] = Properties[PackageProperties.ConfigNameProperty];

				if (Target == null)
				{
					string target = "build";

					if (targetStyle == Project.TargetStyleType.Build)
					{
						// Reverted correct fix for autobuild target
						//if(!String.IsNullOrEmpty(info.AutoBuildTarget))
						if (packageSpec.AutoBuildTarget != null)
						{
							target = packageSpec.AutoBuildTarget;
						}
						Log.Info.WriteLine(LogPrefix + "Auto-building " + PackageName + "-" + PackageVersion + "\n", target);
					}
					else if (targetStyle == Project.TargetStyleType.Clean)
					{
						target = packageSpec.AutoCleanTarget;
						if (null == target)
						{
							target = "clean";
						}
						Log.Info.WriteLine(LogPrefix + "Auto-cleaning " + PackageName + "-" + PackageVersion + " target '{0}'", target);
					}
					else
					{
						string msg = String.Format("Auto-building '" + PackageName + "-" + PackageVersion + "': invalid target style '{0}'", Enum.GetName(typeof(Project.TargetStyleType), targetStyle));
						throw new BuildException(msg, Location);
					}

                    if (target != null)
                    {
                        nantTask.DefaultTarget = target;
                    }
				}
				else
				{
					nantTask.DefaultTarget = Target;
				}

				nantTask.DoInitialize();
				nantTask.NestedProject.SetConstraints(Constraints);

				if (Project.Properties.Contains(Project.NANT_PROPERTY_PROJECT_MASTERTARGET))
				{
					nantTask.NestedProject.Properties.UpdateReadOnly(Project.NANT_PROPERTY_PROJECT_MASTERTARGET, Project.Properties[Project.NANT_PROPERTY_PROJECT_MASTERTARGET]);
				}

				if (Project.Properties.Contains(Project.NANT_PROPERTY_PROJECT_CMDTARGETS))
				{
					nantTask.NestedProject.Properties.UpdateReadOnly(Project.NANT_PROPERTY_PROJECT_CMDTARGETS, Project.Properties[Project.NANT_PROPERTY_PROJECT_CMDTARGETS]);
				}

				if (Project.Properties.GetBooleanProperty(Project.NANT_PROPERTY_TRANSITIVE))
				{
					nantTask.NestedProject.Properties.UpdateReadOnly(Project.NANT_PROPERTY_LINK_DEPENDENCY_IN_CHAIN, PropagatedLinkDependency.ToString().ToLowerInvariant());
				}

				// Set parent package for dependent
				nantTask.NestedProject.Properties.AddReadOnly("package." + PackageName + ".parent", Project.Properties["package.name"]);

				// Set dependency stack property for detecting circular dependencies:

				nantTask.NestedProject.Properties.AddReadOnly("package.dependency.stack", _dependencystack.ToNewLineString());

				nantTask.ExecuteTaskNoDispose();

                if (!nantTask.NestedProject.TryGetPackage(out IPackage depPackage))
                {
					throw new BuildException($"Internal error: Dependent task can't retrieve IPackage object from nested project. Make sure package {PackageName}-{PackageVersion} build file contains <package> task.");
                }

                if (m_dependentPackage.Project == null)
				{
					if (depPackage != null && !String.Equals(depPackage.Name, m_dependentPackage.Name))
					{
						throw new BuildException(String.Format(
							LogPrefix + "Package name set in the task <package name='{0}'> does not match package name '{1}' in masterconfig. This may lead to invalid build. Fix build script in the package  {1}-{2}.", depPackage.Name, PackageName, PackageVersion));
					}
					else
					{
						throw new BuildException($"Internal error: Project object was not set to the package {PackageName}-{PackageVersion}.");
					}
				}

				//write log entry in format ASSUMED by "nant -quickrun:list_dependents"
				Log.Info.WriteLine(LogPrefix + "{0} ({1})", release.FullName, release.Path);
			}
			else
			{
				Log.Info.WriteLine("Package {0}-{1} not marked for autobuild", PackageName, PackageVersion);
			}
		}

		private bool FindCircularRecursive(IPackage package, Stack<IPackage> stack, HashSet<IPackage> visitedPackages, IPackage startPackage, Action<string> onCircularAction)
		{
			stack.Push(package);

			foreach (var dep in package.DependentPackages)
			{
				// Skip entering packages that depend on themselves
				if (dep.Dependent == package)
					continue;

				// Skip packages we've already visited (we know they are not circular)
				if (!visitedPackages.Add(dep.Dependent))
					continue;

				// Oops, we have a circular dependency
				if (startPackage == dep.Dependent)
				{
					if (onCircularAction != null)
					{
						var builder = new StringBuilder();
						builder.AppendFormat("Circular build dependency between packages: ");// {0} ({1}) and {2} ({3})", startPackage.Name, startPackage.ConfigurationName, b.Value.Name, b.Value.ConfigurationName).AppendLine().AppendLine();
						builder.AppendLine();
						foreach (var n in stack.Reverse())
						{
							builder.Append(n.Name);
							builder.AppendFormat(" -> ");
							builder.AppendLine();
						}
						builder.Append(startPackage.Name);
						onCircularAction(builder.ToString());
					}
					return true;
				}

				// Recurse in to dependent package
				if (FindCircularRecursive(dep.Dependent, stack, visitedPackages, startPackage, onCircularAction))
					return true;
			}

			stack.Pop();

			return false;
		}

		private bool FindCircularDependency(bool buildableAndAutoBuildClean, Action<string> onCircularAction)
		{
			_dependencystack = Project.Properties["package.dependency.stack"].LinesToArray();

			if (Project.TryGetPackage(out IPackage parent))
			{
				((Package)parent).TryAddDependentPackage(m_dependentPackage, (buildableAndAutoBuildClean && (TargetStyle == Project.TargetStyleType.Build || TargetStyle == Project.TargetStyleType.Clean)) ? PackageDependencyTypes.Build : PackageDependencyTypes.Use);

				// Looks like this is first dependency and we need to add top level package to avoid circular dep
				if (_dependencystack.IsNullOrEmpty())
				{
					_dependencystack.Add(parent.Key);
				}
			}

			// This is an optimization, see if there is a direct dependency
			if (_dependencystack.Contains(m_dependentPackage.Key))
				return true;

			_dependencystack.Add(m_dependentPackage.Key);

			var stack = new Stack<IPackage>();
			if (parent == null)
				parent = m_dependentPackage;
			else
				stack.Push(parent);

			return FindCircularRecursive(m_dependentPackage, stack, new HashSet<IPackage>(), parent, onCircularAction);
		}

		/// <summary>
		/// Perform miscellaneous validity checks on the package we're depending on.
		/// </summary>
		private void ValidatePackage(Release release, bool buildableAndAutoBuildClean)
		{
			PackageMap.Instance.CheckCompatibility(Log, release, (pkg) =>
			{
				if (PackageMap.Instance.TryGetMasterPackage(Project, pkg, out MasterConfig.IPackage pkgSpec))
				{
					string key = Package.MakeKey(pkg, (pkg == "Framework") ? PackageMap.Instance.GetFrameworkRelease().Version : pkgSpec.Version, Project.Properties[PackageProperties.ConfigNameProperty]);
					if (Project.BuildGraph().Packages.TryGetValue(key, out IPackage package))
					{
						return package.Release;
					}
				}
				return null;
			});

			//if dependent is AutoBuildClean and call context is fw 2
			//and <dependent> is not nested under <target>
			if (buildableAndAutoBuildClean)
			{
				if (!IsNestedUnderTarget())
				{
					//then warn because we can't autobuildclean w/o knowing if style is build or clean;
					//we avoid LogPrefix here because Dependency Analyzer currently expects 
					//a package name-version after LogPrefix (see WriteLineIf below)
					Log.Debug.WriteLine("Can't AutoBuildClean {0} since <dependent> is not nested under <target>.", release.FullName);
				}
			}
		}

		/// <summary>
		/// denote whether dependent is nested under a target task
		/// </summary>
		private bool IsNestedUnderTarget()
		{
			if (_taskNode == null)
			{
				// We are calling from C# 
				return true;
			}
			System.Xml.XmlNode currNode = _taskNode.ParentNode;
			while (currNode != null)
			{
				if (currNode.Name == "target")
				{
					//found target task as parent
					return true;
				}

				currNode = currNode.ParentNode;
			}
			//failed to find target task as parent
			return false;
		}
	}
}
