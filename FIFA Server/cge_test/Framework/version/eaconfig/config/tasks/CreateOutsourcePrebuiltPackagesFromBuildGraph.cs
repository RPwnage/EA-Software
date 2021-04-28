// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Perforce;
using NAnt.Perforce.ConnectionManagment;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig
{
	[TaskName("create-outsource-prebuilt-packages-from-buildgraph")]
	sealed class CreateOutsourcePrebuiltPackagesFromBuildGraph : Task
	{
		[TaskAttribute("prebuilt-output-dir", Required = true)]
		public string OutputRoot
		{
			get;
			set;
		}

		[TaskAttribute("outsource-packages", Required = true)]
		public string OutsourcePackages
		{
			get;
			set;
		}

		[TaskAttribute("test-for-boolean-properties", Required = false)]
		public string TestForBooleanProperties
		{
			get;
			set;
		}

		[TaskAttribute("redirect-includedirs-for-rootdirs", Required = false)]
		public string RedirectIncludeDirsForRootDirs
		{
			get { return mRedirectIncludeDirsForRootDirs; }
			set { mRedirectIncludeDirsForRootDirs = value; }
		}
		private string mRedirectIncludeDirsForRootDirs = String.Empty;

		[TaskAttribute("redirect-includedirs-for-packages", Required = false)]
		public string RedirectIncludeDirsForPackages
		{
			get { return mRedirectIncludeDirsForPackages; }
			set { mRedirectIncludeDirsForPackages = value; }
		}
		private string mRedirectIncludeDirsForPackages = String.Empty;

		[TaskAttribute("new-version-suffix", Required = false)]
		public string NewVersionSuffix
		{
			get { return String.IsNullOrEmpty(mNewVersionSuffix) ? "-prebuilt" : mNewVersionSuffix; }
			set { mNewVersionSuffix = value; }
		}
		private string mNewVersionSuffix = string.Empty;

		[TaskAttribute("output-flattened-package", Required = false)]
		public bool OutputFlattenedPackage { get; set; } = false;

		public List<string> RedirectIncludeDirsPackageList
		{
			get
			{
				if (mRedirectIncludeDirsPackageList == null)
				{
					mRedirectIncludeDirsPackageList = RedirectIncludeDirsForPackages.Split(new char[] { '\n', ';' }).Select(pkg => pkg.Trim().ToLower()).Where(pkg => !String.IsNullOrEmpty(pkg)).ToList();
				}
				return mRedirectIncludeDirsPackageList;
			}
		}
		private List<string> mRedirectIncludeDirsPackageList = null;

		[TaskAttribute("redirect-includedirs-exclude-packages", Required = false)]
		public string RedirectIncludeDirsExcludePackages
		{
			get { return mRedirectIncludeDirsExcludePackages; }
			set { mRedirectIncludeDirsExcludePackages = value; }
		}
		private string mRedirectIncludeDirsExcludePackages = String.Empty;

		public List<string> RedirectIncludeDirsExcludePackageList
		{
			get
			{
				if (mRedirectIncludeDirsExcludePackageList == null)
				{
					mRedirectIncludeDirsExcludePackageList = RedirectIncludeDirsExcludePackages.Split(new char[] { '\n', ';' }).Select(pkg => pkg.Trim().ToLower()).Where(pkg => !String.IsNullOrEmpty(pkg)).ToList();
				}
				return mRedirectIncludeDirsExcludePackageList;
			}
		}
		private List<string> mRedirectIncludeDirsExcludePackageList = null;

		[TaskAttribute("exclude-packages-in-paths", Required = false)]
		public string ExcludePackagesInPaths
		{
			get { return mExcludePackagesInPaths; }
			set
			{
				// Re-format input to make sure all entries have normalized paths.
				IEnumerable<string> pathList = value.Split(new char[] { ';'}).ToList().Select( p => NAnt.Core.Util.PathString.MakeNormalized(p.Trim()).Path );
				mExcludePackagesInPaths = String.Join(";", pathList);
			}
		}
		private string mExcludePackagesInPaths = "";

		[TaskAttribute("exclude-packages", Required = false)]
		public string ExcludePackages
		{
			get { return mExcludePackages; }
			set { mExcludePackages = value; }
		}
		private string mExcludePackages = "";

		[TaskAttribute("extra-token-mappings", Required = false)]
		public string ExtraTokenMappings
		{
			get { return mExtraTokenMappings; }
			set { mExtraTokenMappings = value; }
		}
		private string mExtraTokenMappings="";

		[TaskAttribute("preserve-properties", Required = false)]
		public string PreserveProperties
		{
			get { return mPreserveProperties; }
			set { mPreserveProperties = value; }
		}
		private string mPreserveProperties = String.Empty;

		[TaskAttribute("out-prebuilt-packages-optionsetname", Required = false)]
		public string OutPrebuiltPackagesOptionSetName
		{
			get;
			set;
		}

		[TaskAttribute("out-referenced-source-paths-property", Required = false)]
		public string OutReferenceSourcePathsProperty
		{
			get;
			set;
		}

		[TaskAttribute("out-skipped-packages-property", Required = false)]
		public string OutSkippedPackagesProperty
		{
			get;
			set;
		}

		public override string LogPrefix
		{
			get
			{
				string prefix = " [create-outsource-prebuilt-packages-from-buildgraph] ";
				return prefix.PadLeft(prefix.Length + Log.IndentSize);
			}
		}

		private class PackageData
		{
			public string mPackagePath;
			public string mPackageVersion;
			public List<IModule> mModules = new List<IModule>();
		}

		protected override void ExecuteTask()
		{
			// New version of Framework requires using a new Minimal Log member to show minimal logging.  So use
			// reflection to test if we are using new Framework with this new member or old Framework.
			ILog minLogger = Log.Status;
			System.Reflection.FieldInfo minimalLogField = Log.GetType().GetField("Minimal");
			if (minimalLogField != null)
			{
				object minLoggerObj = minimalLogField.GetValue(Log);
				if (minLoggerObj is ILog)
				{
					minLogger = minLoggerObj as ILog;
				}
			}
			minLogger.WriteLine(LogPrefix + "Creating prebuilt packages ...");

			if (!Project.BuildGraph().IsBuildGraphComplete)
			{
				throw new BuildException("If you want to use create-all-prebuilt-packages-from-buildgraph task directly, please create the build graph first before calling this task.");
			}

			// Group all the active modules from buildgraph by packages.
			Dictionary<string, PackageData> packages = new Dictionary<string, PackageData>();
			foreach (IModule module in Project.BuildGraph().SortedActiveModules)
			{
				if (!packages.ContainsKey(module.Package.Name))
				{
					packages.Add(module.Package.Name, new PackageData());
				}
				packages[module.Package.Name].mPackagePath = module.Package.Dir.Normalize().Path;
				packages[module.Package.Name].mPackageVersion = module.Package.Version;
				packages[module.Package.Name].mModules.Add(module);
			}

			// Packages to outsource
			HashSet<string> packagesExamined = new HashSet<string>();
			HashSet<string> cannotPrebuiltPackages = new HashSet<string>(OutsourcePackages.Split());
			int cannotPreBuiltCount;
			string [] outsourcePkgList = OutsourcePackages.Split();
			// Keep doing the following loop until the cannot prebuilt list is stabilized.
			do
			{
				packagesExamined = new HashSet<string>();
				cannotPreBuiltCount = cannotPrebuiltPackages.Count;
				foreach (string outsourcePkg in outsourcePkgList)
				{
					if (!packages.ContainsKey(outsourcePkg))
					{
						Log.Status.WriteLine("The specified outsource package {0} doesn't appear in build graph.  Skipping this package!", outsourcePkg);
						continue;
					}

					// Get dependent package list for all build modules
					RecursePackageTestDependentPackages(packages, outsourcePkg, packagesExamined, cannotPrebuiltPackages);
				}

			} while (cannotPreBuiltCount != cannotPrebuiltPackages.Count);

			HashSet<string> packagesToPrebuilt = new HashSet<string>(packagesExamined);
			packagesToPrebuilt.ExceptWith(cannotPrebuiltPackages);

			List<string> excludedPaths = new List<string>();
			if (!String.IsNullOrEmpty(ExcludePackagesInPaths))
			{
				excludedPaths = ExcludePackagesInPaths.Split(new char[] {';'}).ToList();
			}

			List<string> excludePackages = new List<string>();
			if (!String.IsNullOrEmpty(ExcludePackages))
			{
				excludePackages = ExcludePackages.Split().ToList();
			}

			OptionSet optSet = new OptionSet();
			if (!String.IsNullOrEmpty(OutPrebuiltPackagesOptionSetName))
			{
				Project.NamedOptionSets[OutPrebuiltPackagesOptionSetName] = optSet;
			}

			List<string> packageSkippedDueToNoProjectInfo = new List<string>();

			HashSet<string> referencedSourcePaths = new HashSet<string>();
			if (!string.IsNullOrEmpty(OutReferenceSourcePathsProperty))
			{
				if (!Project.Properties.Contains(OutReferenceSourcePathsProperty))
				{
					Project.Properties.Add(OutReferenceSourcePathsProperty, "");
				}

				// Using HashSet Union is just a quick way to make the input list is unique.
				referencedSourcePaths.UnionWith(
					Project.Properties[OutReferenceSourcePathsProperty].Split(new char[] { '\n' }).Where(p => !string.IsNullOrEmpty(p)));
			}

			if (!string.IsNullOrEmpty(OutSkippedPackagesProperty))
			{
				if (!Project.Properties.Contains(OutSkippedPackagesProperty))
				{
					Project.Properties.Add(OutSkippedPackagesProperty, "");
				}
			}

			foreach (string packageName in packagesToPrebuilt)
			{
				// Make sure that this is a package that actually exists in masterconfig and not some "fake" prebuild
				// package module added by some post build step.
				if (PackageMap.Instance.FindOrInstallCurrentRelease(Project, packageName) == null)
					continue;

				PackageData package = packages[packageName];

				if (excludePackages.Where(p => p.ToLowerInvariant() == packageName.ToLowerInvariant()).Any())
				{
					Log.Status.WriteLine(LogPrefix + "Skipping package {0}-{1} because it is in exclude package list.", packageName, package.mPackageVersion);
					continue;
				}

				if (excludedPaths.Where(p => package.mPackagePath.StartsWith(p)).Any())
				{
					Log.Status.WriteLine(LogPrefix + "Skipping package {0}-{1} because it is in exclude path list.", packageName, package.mPackageVersion);
					continue;
				}

				Log.Status.WriteLine(LogPrefix + "Creating prebuilt package for {0}-{1}", packageName, package.mPackageVersion);

				// Get all config list for this package.
				List<string> configs = package.mModules.Select( m => m.Configuration.Name ).Distinct().ToList();

				// The CreatePrebuiltPackage task should use the package's project.  Get this project and make sure that it is not null.
				// The project value can be null if it is a prebuilt (use dependency) package (ie build file is not loaded).
				Project packageProject = null;
				List<IModule> modulesWithNullProject = new List<IModule>();
				foreach (IModule module in package.mModules)
				{
					// We loop through all modules to find the first module with Project info.
					// The first module (package.mModules.First()) could be a Utility module which 
					// could contain a null Project.
					if (module.Package != null && module.Package.Project != null)
					{
						if (packageProject == null)
						{
							packageProject = new Project(module.Package.Project);
						}
					}
					else
					{
						modulesWithNullProject.Add(module);
					}
				}
				if (packageProject == null)
				{
					// If all modules has null project that means package is purely being used as "use dependent".  In that case, we skip the package.
					Log.Status.WriteLine(LogPrefix + "NOTE: Package {0}-{1} contains no project info.  This package may already be a prebuilt (use dependency) package. Skipping this package!", packageName, package.mPackageVersion);
					if (!packageSkippedDueToNoProjectInfo.Contains(package.mPackagePath))
					{
						packageSkippedDueToNoProjectInfo.Add(package.mPackagePath);
					}
					continue;
				}

				if (modulesWithNullProject.Any())
				{
					// In Frostbite workflow, we merged two build graphs into one and in one build graph, we have module
					// being used as "build dependent" (with one set of properties settings) while the same module
					// on another build graph is being used as "use dependent" (with different set of properties settings).
					// This cased some modules with null project info.  For the modules with null project assigned,
					// we need to find the parent module's project and use that project so that we can still setup the
					// module info for this use dependency use case with appropriate properties conditions.
					foreach (IModule nullProjMod in modulesWithNullProject)
					{
						IEnumerable<IModule> parentMods = Project.BuildGraph().SortedActiveModules.Where( curMod => curMod.Dependents.Contains(nullProjMod) );
						if (parentMods.Any())
						{
							nullProjMod.Package.Project = parentMods.First().Package.Project;
						}
					}
				}

				// Save and change the following property values that the CreatePrebuiltPackage task will use
				// and then restore after the task
				string oldPackageName = packageProject.Properties["package.name"];
				string oldPackageVersion = packageProject.Properties["package.version"];
				string oldPackageConfigs = packageProject.Properties["package.configs"];
				IEnumerable<IModule> oldTopModules = packageProject.BuildGraph().TopModules;

				packageProject.Properties.UpdateReadOnly("package.name", packageName);
				packageProject.Properties.UpdateReadOnly("package.version", package.mPackageVersion);
				packageProject.Properties["package.configs"] = string.Join(" ", configs);
				packageProject.BuildGraph().SetTopModules(package.mModules);

				bool force_redirect_includedirs = false;
				string redirectIncludeDirsForRootDirs = RedirectIncludeDirsForRootDirs;
				string packageNameLower = packageName.ToLower();
				if (RedirectIncludeDirsPackageList.Contains(packageNameLower))
				{
					force_redirect_includedirs = true;
				}
				if (RedirectIncludeDirsExcludePackageList.Contains(packageNameLower))
				{
					redirectIncludeDirsForRootDirs = "";
				}

				CreatePrebuiltPackage createPrebuiltPkg = new CreatePrebuiltPackage();
				createPrebuiltPkg.OutputRoot = OutputRoot;
				createPrebuiltPkg.Project = packageProject;
				createPrebuiltPkg.TestForBooleanProperties = TestForBooleanProperties;
				createPrebuiltPkg.ExtraTokenMappings = ExtraTokenMappings;
				createPrebuiltPkg.RedirectIncludeDirs = force_redirect_includedirs;
				createPrebuiltPkg.RedirectIncludeDirsForRootDirs = redirectIncludeDirsForRootDirs;
				createPrebuiltPkg.OutReferenceSourcePathsProperty = OutReferenceSourcePathsProperty;
				createPrebuiltPkg.PreserveProperties = PreserveProperties;
				createPrebuiltPkg.NewVersionSuffix = NewVersionSuffix;
				createPrebuiltPkg.OutputFlattenedPackage = OutputFlattenedPackage;
				createPrebuiltPkg.Execute();

				if (!string.IsNullOrEmpty(OutReferenceSourcePathsProperty))
				{
					List<string> extraReferencedSourcePaths = packageProject.Properties[OutReferenceSourcePathsProperty].Split(new char[] { '\n' }).Where(p => !string.IsNullOrEmpty(p)).ToList();
					referencedSourcePaths.UnionWith(extraReferencedSourcePaths);
				}

				// Restore the values before leaving the function.
				packageProject.Properties.UpdateReadOnly("package.name", oldPackageName);
				packageProject.Properties.UpdateReadOnly("package.version", oldPackageVersion);
				packageProject.Properties["package.configs"] = oldPackageConfigs;
				packageProject.BuildGraph().SetTopModules(oldTopModules.Any() ? oldTopModules : null);

				optSet.Options.Add(packageName, package.mPackageVersion + NewVersionSuffix);
			}

			if (!string.IsNullOrEmpty(OutReferenceSourcePathsProperty) && referencedSourcePaths.Any())
			{
				List<string> updatedReferencedSourcePaths = referencedSourcePaths.ToList();
				updatedReferencedSourcePaths.Sort();
				Project.Properties[OutReferenceSourcePathsProperty] = string.Join(Environment.NewLine, updatedReferencedSourcePaths) + Environment.NewLine;
			}

			if (packageSkippedDueToNoProjectInfo.Any() && !String.IsNullOrEmpty(OutSkippedPackagesProperty))
			{
				string currentProperty = "";
				if (Project.Properties.Contains(OutSkippedPackagesProperty))
				{
					currentProperty = Project.Properties[OutSkippedPackagesProperty];
				}
				else
				{
					Project.Properties.Add(OutSkippedPackagesProperty,"");
				}
				packageSkippedDueToNoProjectInfo.Sort();
				string newProperty = currentProperty + string.Join(Environment.NewLine, packageSkippedDueToNoProjectInfo) + Environment.NewLine;
				Project.Properties[OutSkippedPackagesProperty] = newProperty;
			}

			minLogger.WriteLine(LogPrefix + "Finished creating prebuilt packages");
		}

		private void RecursePackageTestDependentPackages(Dictionary<string, PackageData> packagesData, string packageToTest, HashSet<string> pkgExamined, HashSet<string> cannotPrebuiltPackages)
		{
			pkgExamined.Add(packageToTest);

			List<string> dependentPackages = new List<string>();

			foreach (var depList in packagesData[packageToTest].mModules.Select(m=>m.Dependents))
			{
				dependentPackages.AddRange(depList.Select(d => d.Dependent.Package.Name));
			}

			foreach (string testPkgName in dependentPackages.Distinct())
			{
				if (cannotPrebuiltPackages.Contains(testPkgName))
				{
					// if dependent package is one that cannot be prebuilt, this package should not be prebuilt either.
					// Add current test package to the cannotPrebuiltPackages list.
					if (!cannotPrebuiltPackages.Contains(packageToTest))
					{
						cannotPrebuiltPackages.Add(packageToTest);
					}
				}

				// Package has already been looked at, no need to check this package again.
				if (pkgExamined.Contains(testPkgName))
					continue;

				RecursePackageTestDependentPackages(packagesData, testPkgName, pkgExamined, cannotPrebuiltPackages);
			}
		}
	}
}
