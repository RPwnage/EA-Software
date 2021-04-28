// (c) Electronic Arts. All rights reserved.

using System.Xml;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;


namespace EA.Eaconfig
{
	// This task is a wrapper that calls <create-prebuilt-package>.  The main reason for this wrapper is that
	// the <create-prebuilt-package> task assume you are operating on a top level package while this task
	// allows you to have a different top level package and find the package you want to operate on from
	// current build graph and then call <create-prebuilt-package> using the "Project" information associated for that
	// package in the build graph.

	[TaskName("create-prebuilt-for-package")]
	sealed class CreatePrebuiltForPackage : Task
	{
		[TaskAttribute("package-name", Required = true)]
		public string PackageName
		{
			get;
			set;
		}

		[TaskAttribute("package-version", Required = true)]
		public string PackageVersion
		{
			get;
			set;
		}

		[TaskAttribute("prebuilt-output-dir", Required = true)]
		public string OutputRoot
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

		[TaskAttribute("redirect-includedirs", Required = false)]
		public bool RedirectIncludeDirs
		{
			get { return mRedirectIncludeDirs; }
			set { mRedirectIncludeDirs = value; }
		}
		private bool mRedirectIncludeDirs = false;

		[TaskAttribute("redirect-includedirs-for-rootdirs", Required = false)]
		public string RedirectIncludeDirsForRootDirs
		{
			get { return mRedirectIncludeDirsForRootDirs; }
			set { mRedirectIncludeDirsForRootDirs = value; }
		}
		private string mRedirectIncludeDirsForRootDirs = string.Empty;

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
		private string mPreserveProperties = string.Empty;

		[TaskAttribute("new-version-suffix", Required = false)]
		public string NewVersionSuffix
		{
			get { return string.IsNullOrEmpty(mNewVersionSuffix) ? "-prebuilt" : mNewVersionSuffix; }
			set { mNewVersionSuffix = value; }
		}
		private string mNewVersionSuffix = string.Empty;

		[TaskAttribute("output-flattened-package", Required = false)]
		public bool OutputFlattenedPackage { get; set; } = false;

		[TaskAttribute("out-referenced-source-paths-property", Required = false)]
		public string OutReferenceSourcePathsProperty
		{
			get;
			set;
		}

		public override string LogPrefix
		{
			get
			{
				return " [create-prebuilt-for-package] ";
			}
		}

		protected override void InitializeTask(XmlNode taskNode)
		{
			mCurrentTaskXmlNode = taskNode;
			base.InitializeTask(mCurrentTaskXmlNode);
		}
		private XmlNode mCurrentTaskXmlNode;

		protected override void ExecuteTask()
		{
			if (!Project.BuildGraph().IsBuildGraphComplete)
			{
				throw new BuildException("Build graph needs to be setup before you can execute the <create-prebuilt-for-package> task.");
			}

			// Gather all the active modules for current package
			List<IModule> packageModules = Project.BuildGraph().SortedActiveModules.Where(
				m => m.Package.Name.Equals(PackageName,System.StringComparison.InvariantCulture)).ToList();

			Log.Status.WriteLine(LogPrefix + "Creating prebuilt package for {0}-{1}", PackageName, PackageVersion);

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

			// The CreatePrebuiltPackage task should use the package's project.  Get this project and make sure that it is not null.
			// The project value can be null if it is a prebuilt (use dependency) package (ie build file is not loaded).
			Project packageProject = null;
			List<IModule> modulesWithNullProject = new List<IModule>();
			foreach (IModule module in packageModules)
			{
				// We loop through all modules to find the first module with Project info.
				// The first module (packageModules.First()) could be a Utility module which 
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

			if (modulesWithNullProject.Any())
			{
				// The entire package is being dependent on as "use" or "interface" dependent.  In this case, use the
				// parent package's Project.
				foreach (IModule nullProjMod in modulesWithNullProject)
				{
					IEnumerable<IModule> parentMods = Project.BuildGraph().SortedActiveModules.Where( curMod => curMod.Dependents.Contains(nullProjMod) );
					if (parentMods.Any())
					{
						nullProjMod.Package.Project = parentMods.First().Package.Project;
						if (packageProject == null)
						{
							packageProject = new Project(nullProjMod.Package.Project);
						}
					}
				}
			}

			if (packageProject == null)
			{
				// If this is still null, this package isn't really part of active module's build dependency list.
				throw new BuildException(string.Format("ERROR: Package {0}-{1} contains no project info and unable to retrieve parent package's Project either!  This package isn't part of any module build dependency list.", PackageName, PackageVersion));
			}

			List<string> configs = packageModules.Select( m => m.Configuration.Name ).Distinct().ToList();

			// Save and change the following property values that the CreatePrebuiltPackage task will use
			// and then restore after the task
			string oldPackageName = packageProject.Properties["package.name"];
			string oldPackageVersion = packageProject.Properties["package.version"];
			string oldPackageConfigs = packageProject.Properties["package.configs"];
			IEnumerable<IModule> oldTopModules = packageProject.BuildGraph().TopModules;

			packageProject.Properties.UpdateReadOnly("package.name", PackageName);
			packageProject.Properties.UpdateReadOnly("package.version", PackageVersion);
			packageProject.Properties["package.configs"] = string.Join(" ", configs);
			packageProject.BuildGraph().SetTopModules(packageModules);

			EA.Eaconfig.CreatePrebuiltPackage createPrebuiltPkg = new EA.Eaconfig.CreatePrebuiltPackage();
			createPrebuiltPkg.OutputRoot = OutputRoot;
			createPrebuiltPkg.Project = packageProject;
			createPrebuiltPkg.TestForBooleanProperties = TestForBooleanProperties;
			createPrebuiltPkg.ExtraTokenMappings = ExtraTokenMappings;
			createPrebuiltPkg.RedirectIncludeDirs = RedirectIncludeDirs;
			if (!string.IsNullOrEmpty(RedirectIncludeDirsForRootDirs))
			{
				createPrebuiltPkg.RedirectIncludeDirsForRootDirs = RedirectIncludeDirsForRootDirs;
			}
			createPrebuiltPkg.PreserveProperties = PreserveProperties;
			createPrebuiltPkg.NewVersionSuffix = NewVersionSuffix;
			createPrebuiltPkg.OutputFlattenedPackage = OutputFlattenedPackage;
			createPrebuiltPkg.OutReferenceSourcePathsProperty = OutReferenceSourcePathsProperty;
			createPrebuiltPkg.Execute();

			if (!string.IsNullOrEmpty(OutReferenceSourcePathsProperty))
			{
				List<string> extraReferencedSourcePaths = packageProject.Properties[OutReferenceSourcePathsProperty].Split(new char[] { '\n' }).Where(p => !string.IsNullOrEmpty(p)).ToList();
				List<string> newReferencedSourcePaths = referencedSourcePaths.Union(extraReferencedSourcePaths).ToList();
				newReferencedSourcePaths.Sort();
				Project.Properties[OutReferenceSourcePathsProperty] = string.Join(System.Environment.NewLine, newReferencedSourcePaths) + System.Environment.NewLine;
			}

			// Restore the values before leaving the function.
			packageProject.Properties.UpdateReadOnly("package.name", oldPackageName);
			packageProject.Properties.UpdateReadOnly("package.version", oldPackageVersion);
			packageProject.Properties["package.configs"] = oldPackageConfigs;
			packageProject.BuildGraph().SetTopModules(oldTopModules.Any() ? oldTopModules : null);
		}
	}
}
