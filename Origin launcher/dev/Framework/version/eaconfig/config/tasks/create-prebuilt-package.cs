// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Structured;
using EA.Eaconfig.Build;


namespace EA.Eaconfig
{
	[TaskName("create-prebuilt-package")]
	sealed class CreatePrebuiltPackage : Task
	{
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

		[TaskAttribute("extra-token-mappings", Required = false)]
		public string ExtraTokenMappings
		{
			get { return mExtraTokenMappings; }
			set { mExtraTokenMappings = value; }
		}
		private string mExtraTokenMappings="";

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
		private string mRedirectIncludeDirsForRootDirs = String.Empty;

		[TaskAttribute("preserve-properties", Required = false)]
		public string PreserveProperties
		{
			get { return mPreserveProperties; }
			set { mPreserveProperties = value; }
		}
		private string mPreserveProperties = String.Empty;

		[TaskAttribute("new-version-suffix", Required = false)]
		public string NewVersionSuffix
		{
			get { return String.IsNullOrEmpty(mNewVersionSuffix) ? "-prebuilt" : mNewVersionSuffix; }
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

		public List<string> RedirectIncludesRootDirList
		{
			get
			{
				if (mRedirectIncludesRootDirList == null)
				{
					mRedirectIncludesRootDirList = RedirectIncludeDirsForRootDirs.Split(new char[] { '\n', ';' }).Select(p => p.Trim()).Where(p => !String.IsNullOrEmpty(p)).ToList();
				}
				return mRedirectIncludesRootDirList;
			}
		}
		private List<string> mRedirectIncludesRootDirList = null;

		public override string LogPrefix
		{
			get
			{
				return " [create-prebuilt-package] ";
			}
		}

		// Local helper properties to be used throughout all functions
		private string PackageName
		{
			get
			{
				if (mPackageName == null)
				{
					if (!Project.Properties.Contains("package.name"))
					{
						throw new BuildException("Project property is missing 'package.name'!  This shouldn't happen.");
					}
					mPackageName = Project.Properties["package.name"];
				}
				return mPackageName;
			}
		}
		private string mPackageName = null;

		private string PackageVersion
		{
			get
			{
				if (mPackageVersion == null)
				{
					if (!Project.Properties.Contains("package.version"))
					{
						throw new BuildException("Project property is missing 'package.version'!  This shouldn't happen.");
					}
					mPackageVersion = Project.Properties["package.version"];
				}
				return mPackageVersion;
			}
		}
		private string mPackageVersion = null;

		private string PrebuiltPackageVersion
		{
			get
			{
				if (mPrebuiltPackageVersion==null)
				{
					mPrebuiltPackageVersion = PackageVersion + NewVersionSuffix;
				}
				return mPrebuiltPackageVersion;
			}
		}
		private string mPrebuiltPackageVersion=null;

		private string NantBuildRoot
		{
			get
			{
				if (mNantBuildRoot==null)
				{
					mNantBuildRoot = Project.Properties[Project.NANT_PROPERTY_PROJECT_BUILDROOT];
				}
				return mNantBuildRoot;
			}
		}
		private string mNantBuildRoot=null;

		private string NantBuildRootToken
		{
			get
			{
				return "${" + Project.NANT_PROPERTY_PROJECT_BUILDROOT + "}";
			}
		}

		private string PackageDir
		{
			get
			{
				if (mPackageDir==null)
				{
					string propName = String.Format("package.{0}.dir",PackageName);
					if (!Project.Properties.Contains(propName))
					{
						throw new BuildException(String.Format("Project properties is missing define for '{0}'", propName));
					}

					// We use direct string comparison like String.StartsWith, etc.  So it is important that we run through
					// standard code to normalized all path strings.  Or we could end up having accidental mismatch between d:\\ vs D:\\, etc
					mPackageDir = PathString.MakeNormalized(Project.Properties[propName]).Path;
				}
				return mPackageDir;
			}
		}
		private string mPackageDir=null;

		private string PackageDirToken
		{
			get
			{
				if (mPackageDirToken==null)
				{
					mPackageDirToken = String.Format("${{package.{0}.dir}}", PackageName);
				}
				return mPackageDirToken;
			}
		}
		private string mPackageDirToken=null;

		private string PackageBuildDir
		{
			get
			{
				if (mPackageBuildDir==null)
				{
					string propName = String.Format("package.{0}.builddir",PackageName);
					if (!Project.Properties.Contains(propName))
					{
						throw new BuildException(String.Format("Project properties is missing define for '{0}'", propName));
					}
					mPackageBuildDir = PathString.MakeNormalized(Project.Properties[propName]).Path;
				}
				return mPackageBuildDir;
			}
		}
		private string mPackageBuildDir=null;

		private string PackageBuildDirToken
		{
			get
			{
				if (mPackageBuildDirToken == null)
				{
					mPackageBuildDirToken = String.Format("${{package.{0}.builddir}}", PackageName);
				}
				return mPackageBuildDirToken;
			}
		}
		private string mPackageBuildDirToken = null;

		private string PackageGenDir
		{
			get
			{
				if (mPackageGenDir == null)
				{
					string propName = String.Format("package.{0}.gendir", PackageName);
					if (!Project.Properties.Contains(propName))
					{
						throw new BuildException(String.Format("Project properties is missing define for '{0}'", propName));
					}
					mPackageGenDir = PathString.MakeNormalized(Project.Properties[propName]).Path;
				}
				return mPackageGenDir;
			}
		}
		private string mPackageGenDir = null;

		private string PackageGenDirToken
		{
			get
			{
				if (mPackageGenDirToken == null)
				{
					mPackageGenDirToken = String.Format("${{package.{0}.gendir}}", PackageName);
				}
				return mPackageGenDirToken;
			}
		}
		private string mPackageGenDirToken = null;

		private string ExternalIncludeSourceBaseDir
		{
			get
			{
				if (mExternalIncludeSourceBaseDir == null)
				{
					// We allow this property mainly to serve as a hack to deal with packages like "AudoFb" which really
					// just re-direct everything to another folder (deal to another hack that they did to change name of
					// the package from Audio to AudioFb)
					string propName = String.Format("package.{0}.PrebuiltExternalIncludeSourceBaseDir", PackageName);
					if (Project.Properties.Contains(propName))
					{
						mExternalIncludeSourceBaseDir = PathString.MakeNormalized(Project.Properties[propName]).Path;
					}
				}
				return mExternalIncludeSourceBaseDir;
			}
		}
		private string mExternalIncludeSourceBaseDir = null;

		private string PrebuiltPackageDir
		{
			get 
			{
				if (mPrebuiltPackageDir == null)
				{
					if (OutputFlattenedPackage)
						mPrebuiltPackageDir = Path.Combine(OutputRoot, PackageName);
					else
						mPrebuiltPackageDir = Path.Combine(OutputRoot, PackageName, PrebuiltPackageVersion);
				}
				return mPrebuiltPackageDir;
			}
		}
		private string mPrebuiltPackageDir = null;

		private string PrebuiltPackageDirToken
		{
			get
			{
				if (mPrebuiltPackageDirToken==null)
				{
					mPrebuiltPackageDirToken = PrebuiltPackageDir;
					string tokenLower = PrebuiltPackageDir.ToLowerInvariant();
					foreach (string propName in ExtraTokenMappings.Split(new char [] {';'}))
					{
						string propVal = Project.Properties.GetPropertyOrDefault(propName, null);
						if (!string.IsNullOrEmpty(propVal))
						{
							if (tokenLower.StartsWith(propVal.ToLowerInvariant()))
							{
								mPrebuiltPackageDirToken = "${" + propName + "}" + mPrebuiltPackageDirToken.Substring(propVal.Length);
								break;
							}
						}
					}
				}
				return mPrebuiltPackageDirToken;
			}
		}
		private string mPrebuiltPackageDirToken=null;

		private string PrebuiltPackageGeneratedFolderName
		{
			get
			{
				return "";
			}
		}

		private string PrebuiltGeneratedFilesDir
		{
			get
			{
				return Path.Combine(PrebuiltPackageDir, PrebuiltPackageGeneratedFolderName);
			}
		}

		private string PrebuiltGeneratedFilesDirToken
		{
			get
			{
				return Path.Combine(PackageDirToken, PrebuiltPackageGeneratedFolderName);
			}
		}

		private string PrebuiltConfigBinDir
		{
			get
			{
				return Path.Combine(PrebuiltPackageDir, "bin", "${config}");
			}
		}

		private string PrebuiltConfigBinDirToken
		{
			get
			{
				return Path.Combine(PackageDirToken, "bin", "${config}");
			}
		}

		private string PrebuiltConfigLibDir
		{
			get
			{
				return Path.Combine(PrebuiltPackageDir, "lib", "${config}");
			}
		}

		private string PrebuiltConfigLibDirToken
		{
			get
			{
				return Path.Combine(PackageDirToken, "lib", "${config}");
			}
		}

		private bool PrebuiltSkipPdb
		{
			get
			{
				return Project.Properties.GetBooleanPropertyOrDefault("eaconfig.create-prebuilt." + PackageName + ".skip-pdb",
					Project.Properties.GetBooleanPropertyOrDefault("eaconfig.create-prebuilt.skip-pdb", true));
			}
		}

		// This variable get an initialization from current content of property 'OutReferenceSourcePathsProperty'.
		private HashSet<string> mPathsReferencingSourceTree = new HashSet<string>();

		bool RedirectSourceIncludePath(string sourcePath)
		{
			if ((RedirectIncludeDirs && sourcePath.StartsWith(PackageDir)) ||
			     RedirectIncludesRootDirList.Any(inc => sourcePath.StartsWith(inc)))
			{
				if (!mPathsReferencingSourceTree.Contains(sourcePath))
				{
					mPathsReferencingSourceTree.Add(sourcePath);
				}
				return true;
			}
			else
			{
				return false;
			}
		}

		// The public data that is expected to be in initialize.xml.
		public class SharedPchExportInfo
		{
			public SharedPchExportInfo()
			{
				PchHeader = null;
				PchHeaderDir = null;
				PchSourceFile = null;
				PchFile = null;
			}
			public string PchHeader;
			public string PchHeaderDir;
			public string PchSourceFile;
			public string PchFile;
		};

		public class PrebuiltPublicData
		{
			// We don't need to override this when we export custom initialize.xml
			public IEnumerable<string> Defines { get { return null; } }

			// We need to override includedirs when we export custom initialize.xml as some items may point to
			// package.builddir.  If there are items that point to external package, we may have a problem.  If user 
			// do need to have includes to external package:
			//    * Non-SDK packages: they should use the "publicdependencies" SXML element to add include paths of dependent package.
			//    * SDK packages: We need to examine the SDK package include list and maybe we need to update that?
			//    * None of the above: We need to examine the error case by case and determine what to do.
			// Aside from overriding the initialize.xml, we also need to copy header files of these directories 
			// to destination package.
			public IEnumerable<PathString> IncludeDirectories
			{
				get
				{
					foreach (string path in mIncludeDirs)
					{
						yield return new PathString(path);
					}
				}
			}

			// We won't be overriding this or do anything with this.  Most usage I've seen is adding
			// extra SDK package's directory.  But don't need this as export of the current package's binaries.
			public IEnumerable<PathString> UsingDirectories { get { return null; } }

			// This usually only get used for adding extra SDK lib directories.  Nothing in it usually points to
			// the current package.  No need to override this or do anything with it.
			public IEnumerable<PathString> LibraryDirectories { get { return null; } }

			// Frostbite created package.[package_name].internalIncludeDirs for headers that is included by 
			// "team" internal use but public headers are still referencing them.  So we need to treat these
			// like the regular includedirs property but under different property name.
			public IEnumerable<PathString> InternalIncludeDirectories 
			{
				get 
				{
					foreach (string path in mInternalIncludeDirs)
					{
						yield return new PathString(path);
					}
				}
			}

			// The following filesets are build artifacts.  Need to be copied over from
			// build output folder to new out source package folder.  The ContentFiles fileset (normally) won't be overridden as
			// it usually doesn't contain build artifacts.  But we may need to copy over the files to destination
			// prebuilt package.  We're not overriding ContentFiles fileset in initialize.xml because a lot of the times
			// it actually list files from another package (which would be difficult to resolve back from fullpath 
			// to ${package.[how_to_know_what_is_this_other_package].dir})  HOWEVER, we'll monitor contentfiles' fileset content
			// If it contains files in buildroot or builddir, we'll copy those and create override for the fileset.
			public FileSet Assemblies
			{
				get { return mAssembliesFs; }
				set { mAssembliesFs = value; }
			} private FileSet mAssembliesFs;

			public FileSet ContentFiles
			{
				get { return mContentFilesFs; }
				set { mContentFilesFs = value; }
			} private FileSet mContentFilesFs;

			public FileSet DynamicLibraries
			{
				get { return mDynamicLibrariesFs; }
				set { mDynamicLibrariesFs = value; }
			} private FileSet mDynamicLibrariesFs;

			public FileSet Libraries
			{
				get { return mLibrariesFs; }
				set { mLibrariesFs = value; }
			} private FileSet mLibrariesFs;

			public FileSet Programs
			{
				get { return mProgramsFs; }
				set { mProgramsFs = value; }
			} private FileSet mProgramsFs;

			public FileSet NatvisFiles
			{
				get { return mNatvisFs; }
				set { mNatvisFs = value; }
			}
			private FileSet mNatvisFs;

			public FileSet DllsExternal
			{
				get { return mDllsExternalFs; }
				set { mDllsExternalFs = value; }
			}
			private FileSet mDllsExternalFs;

			public FileSet LibsExternal
			{
				get { return mLibsExternalFs; }
				set { mLibsExternalFs = value; }
			}
			private FileSet mLibsExternalFs;

			// The data that we actually use are stored in List<string> so that it would be easier
			// to construct HastSet<string> for config common set and config specific set later on.
			public List<string> mIncludeDirs;
			public List<string> mInternalIncludeDirs;
			public List<string> mContentFiles;
			public List<string> mLibs;
			public List<string> mDlls;
			public List<string> mAssemblies;
			public List<string> mPrograms;
			public List<string> mNatvisFiles;

			// The "dlls-external" and "libs-external" fileset that points to files that is cached local to current package
			// rather than external package.
			public List<string> mDllsExternal;
			public List<string> mLibsExternal;

			// This one is in <publicdata> format, but actually not being stored in IPublicData because content
			// is automatically expanded and rolled into package.[packagename].[module].includedirs.  However,
			// we need to keep track of it so that we can re-construct the new Initialize.xml properly and
			// tokenize those paths back to ${package.<packageName>.dir}
			public Dictionary<string, string> mPublicDependencies;

			// This one is more frostbite specific and not in the general <publicdata> format.
			public List<string> mDdfIncludeDirs;

			// Shared PCH module specific data
			public SharedPchExportInfo mSharedPchData;

			// The project used when creating this public data cache.  This local
			// project store different property set for different config / buildgraph route
			public Project mPublicDataProject;

			public PrebuiltPublicData(Project project, string PackageName, string moduleName)
			{
				mPublicDataProject = project;

				mPublicDependencies = new Dictionary<string, string>();
				GetPublicDataPublicDependencies(project, PackageName, moduleName, mPublicDependencies, new List<string>());

				GetPublicDataIncludeDirs(project, PackageName, moduleName, "includedirs", out mIncludeDirs);
				GetPublicDataIncludeDirs(project, PackageName, moduleName, "internalIncludeDirs", out mInternalIncludeDirs);
				GetPublicDataIncludeDirs(project, PackageName, moduleName, "ddfincludedirs", out mDdfIncludeDirs);

				GetPublicDataFileSet(project, PackageName, moduleName, "libs", out mLibrariesFs, out mLibs);
				GetPublicDataFileSet(project, PackageName, moduleName, "dlls", out mDynamicLibrariesFs, out mDlls);
				GetPublicDataFileSet(project, PackageName, moduleName, "assemblies", out mAssembliesFs, out mAssemblies);
				GetPublicDataFileSet(project, PackageName, moduleName, "programs", out mProgramsFs, out mPrograms);
				GetPublicDataFileSet(project, PackageName, moduleName, "contentfiles", out mContentFilesFs, out mContentFiles);
				GetPublicDataFileSet(project, PackageName, moduleName, "natvisfiles", out mNatvisFs, out mNatvisFiles);

				GetPublicDataFileSet(project, PackageName, moduleName, "dlls.external", out mDllsExternalFs, out mDllsExternal);
				GetPublicDataFileSet(project, PackageName, moduleName, "libs.external", out mLibsExternalFs, out mLibsExternal);

				mSharedPchData = null;
				if (project.Properties.GetBooleanPropertyOrDefault("package."+PackageName+"."+moduleName+".issharedpchmodule",false))
				{
					mSharedPchData = new SharedPchExportInfo();
					mSharedPchData.PchHeader = PathString.MakeNormalized(
						project.Properties.GetPropertyOrFail("package."+PackageName+"."+moduleName+".pchheader"),
						PathString.PathParam.NormalizeOnly).Path;
					mSharedPchData.PchHeaderDir = PathString.MakeNormalized(
						project.Properties.GetPropertyOrFail("package." + PackageName + "." + moduleName + ".pchheaderdir"),
						PathString.PathParam.NormalizeOnly).Path;
					mSharedPchData.PchSourceFile = PathString.MakeNormalized(
						project.Properties.GetPropertyOrFail("package." + PackageName + "." + moduleName + ".pchsource"),
						PathString.PathParam.NormalizeOnly).Path;
					mSharedPchData.PchFile = PathString.MakeNormalized(
						project.Properties.GetPropertyOrFail("package." + PackageName + "." + moduleName + ".pchfile"),
						PathString.PathParam.NormalizeOnly).Path;
				}
			}

			private void GetPublicDataPublicDependencies(Project project, string packageName, string moduleName, Dictionary<string, string> dependencies, List<string> parentPackageNames)
			{
				string publicDependenciesPropName = null;
				string publicBuildDependenciesPropName = null;
				if (String.IsNullOrEmpty(moduleName))
				{
					publicDependenciesPropName = String.Format("package.{0}.publicdependencies", packageName);
					publicBuildDependenciesPropName = String.Format("package.{0}.publicbuilddependencies", packageName);
				}
				else
				{
					publicDependenciesPropName = String.Format("package.{0}.{1}.publicdependencies", packageName, moduleName);
					publicBuildDependenciesPropName = String.Format("package.{0}.{1}.publicbuilddependencies", packageName, moduleName);
				}

				// Cache public dependency package's package.dir and package.builddir info so that later we can 
				// use use those info to re-tokenize the includedirs, etc during new Initialize.xml construction.
				string publicExportPublicDependencies = project.Properties.GetPropertyOrDefault(publicDependenciesPropName, null);
				string publicBuildDependencies = project.Properties.GetPropertyOrDefault(publicBuildDependenciesPropName, null);
				if (!String.IsNullOrEmpty(publicBuildDependencies))
				{
					if (String.IsNullOrEmpty(publicExportPublicDependencies))
					{
						publicExportPublicDependencies = publicBuildDependencies;
					}
					else
					{
						publicExportPublicDependencies += "\n" + publicBuildDependencies;
					}
				}
				if (!String.IsNullOrWhiteSpace(publicExportPublicDependencies))
				{
					IEnumerable<string> dependencyList = publicExportPublicDependencies.Split(new char [] {'\n' }).Select(dep => dep.Trim()).Where(dep => !String.IsNullOrEmpty(dep));

					foreach (string dependencyString in dependencyList)
					{
						string fullDependencyString = dependencyString;
						DependencyDeclaration depInfo = new DependencyDeclaration(dependencyString);
						string depPkgName = depInfo.Packages.First().Name;
						string depModuleGroup = null;
						string depModuleName = null;
						if (depInfo.Packages.First().Groups.Count()==1)
						{
							depModuleGroup = depInfo.Packages.First().Groups.First().Type.ToString();
							depModuleName = depInfo.Packages.First().Groups.First().Modules.First();
							
							// If there are module specification in this dependency line, we just wanted to make sure that the group
							// info is in the dependency line.
							fullDependencyString = String.Format("{0}/{1}/{2}", depPkgName, depModuleGroup, depModuleName);
						}
						
						// If we already processed this dependency, just go to the next one
						if (dependencies.ContainsKey(fullDependencyString))
						{
							continue;
						}

						string propDirName = String.Format("package.{0}.dir", depPkgName);
						string propDirValue = project.Properties.GetPropertyOrDefault(propDirName, null);
						if (propDirValue != null)
						{
							propDirValue = PathString.MakeNormalized(propDirValue, PathString.PathParam.NormalizeOnly).Path;
							dependencies.Add(fullDependencyString, propDirValue);
						}

						// If the dependent package is itself (just exporting includes of another module of it's own package, 
						// we can skip the following because the current module's includedirs properly would have already
						// expanded and we already have package dir info to resolve the token later.
						if (depPkgName == packageName)
						{
							continue;
						}

						string depIncludeDirProp = String.Format("package.{0}.includedirs", depPkgName);
						if (depModuleName != null)
						{
							string depModuleIncludeDirProp = String.Format("package.{0}.{1}.includedirs", depPkgName, depModuleName);
							if (project.Properties.Contains(depModuleIncludeDirProp))
							{
								depIncludeDirProp = depModuleIncludeDirProp;
							}
						}

						// We maybe making a full dependency of the entire package, but the package didn't specify package level includedirs
						// property.  The package may only have module level includedirs defined.  We need to check for that.
						if (!project.Properties.Contains(depIncludeDirProp))
						{
							List<string> moduleListIncludeDirProps = new List<string>();
							// Get the full list of that package's modules and get all module level includedirs property.
							string moduleList = project.Properties.GetPropertyOrDefault(String.Format("package.{0}.buildmodules", depPkgName),"");
							foreach (BuildGroups grp in Enum.GetValues(typeof(BuildGroups)))
							{
								moduleList += " " + project.Properties.GetPropertyOrDefault(String.Format("package.{0}.{1}.buildmodules", depPkgName, grp.ToString()),"");
							}
							foreach (string module in moduleList.Split().Select(m=>m.Trim()).Where(m=>!String.IsNullOrEmpty(m)))
							{
								string depModuleIncludeDirProp = String.Format("package.{0}.{1}.includedirs", depPkgName, module);
								if (project.Properties.Contains(depModuleIncludeDirProp))
								{
									moduleListIncludeDirProps.Add(depModuleIncludeDirProp);
								}
							}
							if (!moduleListIncludeDirProps.Any())
							{
								// There could be an error in build file.  Making publicdependencies to this package but
								// there are no includedirs property defined.  But it could also be a publicbuilddependencies with
								// no includedirs export.  So for now, just give a warning message without actually throwing an 
								// exception.
								project.Log.Status.WriteLine("WARNING: Package '{0}' is having publicdependencies or publicbuilddependencies to '{1}', but no includedirs property is defined for that dependent.", packageName, fullDependencyString);
								depIncludeDirProp = null;
							}
							else
							{
								depIncludeDirProp = String.Join(";", moduleListIncludeDirProps);
							}
						}
						else if(String.IsNullOrEmpty(project.Properties[depIncludeDirProp].Trim()))
						{
							project.Log.Status.WriteLine("WARNING: Package '{0}' is having publicdependencies or publicbuilddependencies to '{1}', but includedirs property defined for that dependent contain empty info.  Ignoring this includedirs.  If {1} is a C# module, you should not be using publicdependencies and publicbuilddependencies, just setup dependencies in .build file!", packageName, fullDependencyString);
							depIncludeDirProp = null;
						}

						if (depIncludeDirProp != null)
						{
							dependencies.Add(fullDependencyString + "+includedirsprops", depIncludeDirProp);
						}

						string propBuildDirName = String.Format("package.{0}.builddir", depPkgName);
						string propBuildDirValue = project.Properties.GetPropertyOrDefault(propBuildDirName, null);
						if (propBuildDirValue != null)
						{
							propBuildDirValue = PathString.MakeNormalized(propBuildDirValue, PathString.PathParam.NormalizeOnly).Path;
							// The '+' character is not a valid module name character, so it should be save to be used to make a new unique key.
							string builddirKey = depPkgName + "+builddir";
							if (!dependencies.ContainsKey(builddirKey))
							{
								// Note that we're not using the full dependency string for the dependent package's builddir key.  Regardless of
								// which module we dependent on, there is only one builddir for that package.
								dependencies.Add(builddirKey, propBuildDirValue);
							}
						}
					}

					// Do a recursive pass to check each dependent to see if those dependent has public dependencies as well.  If they do,
					// we need to cache the package name's package.dir and package.builddir token info as well.

					// Track a list of all the packages we've seen so we don't loop
					if (!parentPackageNames.Contains(packageName))
					{
						parentPackageNames.Add(packageName);
					}

					foreach (string dependencyString in dependencyList)
					{
						DependencyDeclaration depInfo = new DependencyDeclaration(dependencyString);
						string depPkgName = depInfo.Packages.First().Name;

						if (depPkgName == packageName || parentPackageNames.Contains(depPkgName))
						{
							// If the public dependency is listing itself, we can just skip it, we're only interested in the package name itself
							// and not really interested in the full module dependency info.  So there is no need to recurse to itself or we'll
							// ended up in infinite loop and blow up the stack.
							continue;
						}

						GetPublicDataPublicDependencies(project, depPkgName, null, dependencies, parentPackageNames);

						// Note that we technically support the syntax "package.buildmodules" property.  But that property only exists for the top
						// package's "project" space and we won't be able to see that property from another package's space.  This property don't get
						// used often and we'll see if we can get away with it.
						
						string moduleList = project.Properties.GetPropertyOrDefault(String.Format("package.{0}.buildmodules", depPkgName),"");
						foreach (BuildGroups grp in Enum.GetValues(typeof(BuildGroups)))
						{
							moduleList += " " + project.Properties.GetPropertyOrDefault(String.Format("package.{0}.{1}.buildmodules", depPkgName, grp.ToString()),"");
						}

						foreach (string module in moduleList.Split().Select(m=>m.Trim()).Where(m=>!String.IsNullOrEmpty(m)))
						{
							GetPublicDataPublicDependencies(project, depPkgName, module, dependencies, parentPackageNames);
						}
					}
				}
			}

			private void GetPublicDataIncludeDirs(Project project, string packageName, string moduleName, string propertyName, out List<string> list)
			{
				list = new List<string>();
				string includedirsPropName = null;
				if (String.IsNullOrEmpty(moduleName))
				{
					includedirsPropName = String.Format("package.{0}.{1}", packageName, propertyName);
				}
				else
				{
					includedirsPropName = String.Format("package.{0}.{1}.{2}", packageName, moduleName, propertyName);
				}

				string publicExportIncludeDirs = project.Properties.GetPropertyOrDefault(includedirsPropName, null);
				if (!String.IsNullOrWhiteSpace(publicExportIncludeDirs))
				{
					foreach (string include in publicExportIncludeDirs.Split(new char[] { '\n' }))
					{
						string trimmedInclude = include.Trim();
						if (String.IsNullOrWhiteSpace(trimmedInclude))
							continue;

						// We normalize the path string for easier string manipulation later on.
						trimmedInclude = PathString.MakeNormalized(trimmedInclude, PathString.PathParam.NormalizeOnly).Path;

						list.Add(trimmedInclude);
					}
				}
			}

			private void GetPublicDataFileSet(Project project, string packageName, string moduleName, string filesetname, out FileSet fileset, out List<string> list)
			{
				string fullFilesetName = null;
				if (String.IsNullOrEmpty(moduleName))
				{
					fullFilesetName = String.Format("package.{0}.{1}", packageName, filesetname);
				}
				else
				{
					fullFilesetName = String.Format("package.{0}.{1}.{2}", packageName, moduleName, filesetname);
				}

				// Check if this package has a prebuilt overrides and point the files to different location.
				List<string> oldPrebuiltOverrideLocations = new List<string>();
			
				if (PackageMap.Instance.TryGetMasterPackage(project, packageName, out MasterConfig.IPackage masterPackage))
				{
					if (PackageMap.Instance.IsPackageAutoBuildClean(project, packageName) == false)
					{
						string optionsetName = masterPackage.EvaluateGroupType(project).OutputMapOptionSet;
						if (!String.IsNullOrEmpty(optionsetName) && OptionSetUtil.OptionSetExists(project, optionsetName))
						{
							// This package is currently using prebuilt data.  Need to test for file exists and if not exists, test for new location.
							OptionSet optSet = OptionSetUtil.GetOptionSet(project, optionsetName);
							if (filesetname == "libs")
							{
								string oldPrebuiltLibLoc = OptionSetUtil.GetOptionOrDefault(optSet, "configlibdir", null);
								if (!String.IsNullOrEmpty(oldPrebuiltLibLoc))
									oldPrebuiltOverrideLocations.Add(oldPrebuiltLibLoc);
								// Some platform requires dll stub libs to be in bin folder instead.  So add that as a second option
								string oldPrebuiltBinLoc = OptionSetUtil.GetOptionOrDefault(optSet, "configbindir", null);
								if (!String.IsNullOrEmpty(oldPrebuiltBinLoc))
									oldPrebuiltOverrideLocations.Add(oldPrebuiltBinLoc);
							}
							else if (filesetname == "dlls" || filesetname == "assemblies")
							{
								string oldPrebuiltBinLoc = OptionSetUtil.GetOptionOrDefault(optSet, "configbindir", null);
								if (!String.IsNullOrEmpty(oldPrebuiltBinLoc))
									oldPrebuiltOverrideLocations.Add(oldPrebuiltBinLoc);
							}
						}
					}
				}

				list = new List<string>();
				if (project.NamedFileSets.TryGetValue(fullFilesetName, out fileset))
				{
					fileset.FailOnMissingFile = false;
					if (fileset.FileItems.Any())
					{
						foreach (FileItem file in fileset.FileItems)
						{
							string filepath = file.FullPath;
							if (!File.Exists(filepath) && oldPrebuiltOverrideLocations.Any())
							{
								string filename = Path.GetFileName(filepath);
								foreach (string possibleRoot in oldPrebuiltOverrideLocations)
								{
									string newFilePath = Path.Combine(possibleRoot, filename);
									if (File.Exists(newFilePath))
									{
										filepath = newFilePath;
										break;
									}
								}
							}
							list.Add(filepath);
						}
					}
				}
			}
		}

		class ModuleInfo
		{
			public string mModuleName;

			// At present, the build system don't really allow same module name being belong to different build group in different config.
			// So we don't have to worry about caching build group info by configs.
			public string mModuleBuildGroup;

			public bool mIsSharedPchModule;

			public Dictionary<string, BuildType> mModuleBuildTypeByConfig;

			// Some test modules requires to setup this attribute in dependency block.  For now, we're not going to support different value by config.
			public bool mModuleSkipRuntimeDependency;

			// Dictionary key is config, value is dependent module list in a HashSet.
			public Dictionary<string, HashSet<string>> mDependentsByConfig;

			// Module type can be different depending on configs (dll configs can have DynamicLibrary build type while regular config have Library build type).
			public Dictionary<string, uint> mModuleTypeByConfig;

			// This is frostbite specific.  In the buildfile, if the module has ddffiles, we need to create a module level property 'has-ddf-files'
			// so that the ddfincludedirs dependency can be setup properly.
			public Dictionary<string, bool> mModuleHasDdfByConfig;

			// These are public data info for exporting out includedirs property and libs / dlls / assemblies / etc filesets to initialize.xml
			public Dictionary<string, PrebuiltPublicData> mModulePublicDataByConfig;

			public ModuleInfo()
			{
				mModuleName = String.Empty;
				mIsSharedPchModule = false;
				mModuleBuildTypeByConfig = new Dictionary<string, BuildType>();
				mDependentsByConfig = new Dictionary<string, HashSet<string>>();
				mModuleTypeByConfig = new Dictionary<string, uint>();
				mModuleHasDdfByConfig = new Dictionary<string, bool>();
				mModulePublicDataByConfig = new Dictionary<string, PrebuiltPublicData>();
			}

			public void SetupModuleConfigInfo(ProcessableModule module)
			{
				mModuleName = module.Name;

				mIsSharedPchModule = false;
				if (module.Package.Project != null)
				{
					mIsSharedPchModule = module.Package.Project.Properties.GetBooleanPropertyOrDefault("package." + module.Package.Name + "." + module.Name + ".issharedpchmodule", false);
				}

				// Don't expect identical module name being used in different build group!
				mModuleBuildGroup = module.BuildGroup.ToString();

				if (!mModuleBuildTypeByConfig.ContainsKey(module.Configuration.Name))
				{
					mModuleBuildTypeByConfig.Add(module.Configuration.Name, module.BuildType);
				}

				// Get this module's dependeny list for this config.
				if (!mDependentsByConfig.ContainsKey(module.Configuration.Name))
				{
					mDependentsByConfig.Add(module.Configuration.Name, new HashSet<string>());
				}

				mModuleSkipRuntimeDependency = module.Package.Project.Properties.GetBooleanPropertyOrDefault(mModuleBuildGroup + "." + module.Name + ".skip-runtime-dependency", false);

				if (module.Dependents.Any())
				{
					foreach (Dependency<IModule> dependent in module.Dependents.Select(dep => dep))
					{
						string depPkgVer = PackageMap.Instance.GetMasterPackage(module.Package.Project, dependent.Dependent.Package.Name).Version;
						if (String.IsNullOrEmpty(depPkgVer))
						{
							// FBConfig seems to add a dynamically created Forstbite.Prebuild module to all ddf module's dependency list.  But this 
							// module / package actually doesn't exist.  The Prebuild module just collect all the prebuild generation of generated header and source
							// files.  But for prebuilt package, these are already generated (listed in includedirs property in initialize.xml) and will be copied
							// over to generated prebuilt package.  So, for now, we'll just skip adding this dependency and put a warning message in case
							// there are other special cases we need to look ingo.
							module.Package.Project.Log.Debug.WriteLine("WARNING: module '{0}' is depending on package '{1}'.  But this package is not specified in masterconfig.  We're skipping this dependency in the generated prebuilt package.", module.Name, dependent.Dependent.Package.Name);
							continue;
						}

						// If the dependent package didn't provide module specific export in the initialize.xml, we do a dependent on the entire 
						// package instead of specifying the module.  Otherwise, when people use it, they will get warning message.
						string dependentExportedModuleList = module.Package.Project.Properties.GetPropertyOrDefault(string.Format("package.{0}.{1}.buildmodules", dependent.Dependent.Package.Name, dependent.Dependent.BuildGroup.ToString()), "");
						if (string.IsNullOrEmpty(dependentExportedModuleList) && dependent.Dependent.Package.Project != null)
						{
							// Normally, parent module should be able to see dependent module's export module list.  If for some reason it isn't available, try using dependent module's project!
							dependentExportedModuleList = dependent.Dependent.Package.Project.Properties.GetPropertyOrDefault(string.Format("package.{0}.{1}.buildmodules", dependent.Dependent.Package.Name, dependent.Dependent.BuildGroup.ToString()), "");
						}
						string dependencyType = "build";  // As default, use "build" dependent.
						if (dependent.IsKindOf(DependencyTypes.Interface) && !(dependent.IsKindOf(DependencyTypes.Build) || dependent.IsKindOf(DependencyTypes.Link) || dependent.IsKindOf(DependencyTypes.AutoBuildUse)))
						{
							dependencyType = "interface";
						}
						else if (dependent.IsKindOf(DependencyTypes.Interface) && dependent.IsKindOf(DependencyTypes.Link) && !dependent.IsKindOf(DependencyTypes.AutoBuildUse) && !dependent.IsKindOf(DependencyTypes.Build))
						{
							dependencyType = "use";
						}
						else if (dependent.IsKindOf(DependencyTypes.Interface) && dependent.IsKindOf(DependencyTypes.Link) && dependent.IsKindOf(DependencyTypes.AutoBuildUse) && !dependent.IsKindOf(DependencyTypes.Build))
						{
							dependencyType = "auto";
						}
						else if (!dependent.IsKindOf(DependencyTypes.Link))
						{
							dependencyType = "build link=\"false\"";
						}
						string dependentString = "<" + dependencyType + ">" + dependent.Dependent.Package.Name;
						if (!String.IsNullOrWhiteSpace(dependentExportedModuleList))
						{
							if (dependentExportedModuleList.Split().Contains(dependent.Dependent.Name))
							{
								dependentString = string.Format("<{0}>{1}/{2}/{3}", dependencyType, dependent.Dependent.Package.Name, dependent.Dependent.BuildGroup.ToString(), dependent.Dependent.Name);
							}
						}
						if (!mDependentsByConfig[module.Configuration.Name].Contains(dependentString))
						{
							mDependentsByConfig[module.Configuration.Name].Add(dependentString);
						}
					}
				}

				// We need to save original module build type info so that later on when we create the build file using Utility module type, we can 
				// decide whether we need to add the transitivelink attribute.
				if (!mModuleTypeByConfig.ContainsKey(module.Configuration.Name))
				{
					mModuleTypeByConfig.Add(module.Configuration.Name, module.Type);
				}

				// Save the info whether this module has ddffiles
				if (!mModuleHasDdfByConfig.ContainsKey(module.Configuration.Name))
				{
					mModuleHasDdfByConfig.Add(module.Configuration.Name, !string.IsNullOrEmpty(module.PropGroupValue("ddf-optionset-names")));
				}

				// Now extract this module's public data info.
				if (!mModulePublicDataByConfig.ContainsKey(module.Configuration.Name))
				{
					// We are collecting config specific data, so make sure we use the module.Package.Project instead of Project.  The
					// current "Project" is started by a different config and may not be what you wanted.
					mModulePublicDataByConfig.Add(module.Configuration.Name, new PrebuiltPublicData(module.Package.Project, module.Package.Name, module.Name));
				}
			}
		}

		class PrebuiltPackageInfo
		{
			// List of modules for this package.
			// mModulesDataByProperty[propertyGrpKey][moduleName]
			public Dictionary<string, Dictionary<string, ModuleInfo>> mModulesDataByProperty;

			// Caching the full config deltails by config name.  The config name itself can potentially be anything 
			// and may not contain the config-system info.  If there are usage where we need to test for config-system
			// specific info, we should do mConfigDetailsByConfig[configName] to get the details info.
			// Also, this config list is cumulative for all modules for this package.  So the list could be bigger than
			// individual module's supported config list and contains duplicate.
			public Dictionary<string, EA.FrameworkTasks.Model.Configuration> mConfigDetailsByConfig;

			// If this package has package level public data, this is where it is stored.
			// mPackageLevelPublicDataByPropertyAndConfig[propertyGrpKey][ConfigKey]
			public Dictionary<string, Dictionary<string, PrebuiltPublicData>> mPackageLevelPublicDataByPropertyAndConfig;

			// Optional fileset that user can define in their initialize.xml to tell us explicitly what files not to package for prebuilt.
			// Expect to be config independent.  The optional fileset name is 'package.<PackageName>.PrebuiltExcludeSourceFiles'
			public HashSet<string> mPrebuiltPackageExcludeFiles;

			// Optional fileset that user can define in their initialize.xml to tell us explicitly what files have to be packaged for prebuilt.
			// Expect to be config independent.  The optional fileset name is 'package.<PackageName>.PrebuiltIncludeSourceFiles'
			public HashSet<string> mPrebuiltPackageIncludeFiles;

			// Optional fileset that user can define in their initialize.xml to tell us explicitly package up extra *.build/*.xml files
			// AND add the following at end of the newly constructed [PackageName].build file:
			// <foreach item="FileSet" in="package.[PackageName].PrebuiltPackageExtraBuildFiles" property="filename"> 
			//     <include file="${filename}"/>
			// </foreach>
			// NOTE that this 'package.[PackageName].PrebuiltPackageExtraBuildFiles' fileset MUST be common for ALL configs.  Any config
			// specific instructions must be inside those build files.
			// IMPORTANT NOTE: These extra build files CANNOT contain module definitions like `<Library>`, etc.
			public HashSet<string> mPrebuiltPackageExtraBuildFiles;

			// Store extra properties that are normally created in package's .build file and not created in Initialize.xml.
			// But due to the fact that original .build file is no longer preserved in generated prebuilt package, we need
			// to preserve these properties to the Initialize.xml.
			public Dictionary<string, string> mPreserveProperties;

			public PrebuiltPackageInfo()
			{
				mModulesDataByProperty = new Dictionary<string, Dictionary<string, ModuleInfo>>();
				mConfigDetailsByConfig = new Dictionary<string,Configuration>();
				mPackageLevelPublicDataByPropertyAndConfig = new Dictionary<string,Dictionary<string, PrebuiltPublicData>>();
				mPrebuiltPackageExcludeFiles = new HashSet<string>();
				mPrebuiltPackageIncludeFiles = new HashSet<string>();
				mPrebuiltPackageExtraBuildFiles = new HashSet<string>();
				mPreserveProperties = new Dictionary<string, string>();
			}
		}

		// This package's data initialized by BuildPackageModuleInfo().  Once initialized, it is being used by the rest of the functions.
		private PrebuiltPackageInfo  mPackageData = null;

		protected override void ExecuteTask()
		{
			if (!Project.BuildGraph().IsBuildGraphComplete)
			{
				throw new BuildException("If you want to use create-prebuilt-package task directly instead of using create-prebuilt-package target, please setup the build graph first before calling this task.");
			}

			// If output directory exists, wipe it clean and start from scratch!
			if (Directory.Exists(PrebuiltPackageDir))
			{
				PathUtil.DeleteDirectory(PrebuiltPackageDir);
			}

			mPathsReferencingSourceTree.Clear();
			if (!String.IsNullOrEmpty(OutReferenceSourcePathsProperty))
			{
				if (!Project.Properties.Contains(OutReferenceSourcePathsProperty))
				{
					Project.Properties.Add(OutReferenceSourcePathsProperty, "");
				}
				mPathsReferencingSourceTree.UnionWith(Project.Properties[OutReferenceSourcePathsProperty].Split(new char[] { '\n' }).Where(p => !string.IsNullOrEmpty(p)));
			}

			// Analyse build graph info and extract the necessary info to internal data that is needed for the next few functions that does the work.
			BuildPackageModuleInfo();

			Project.Log.Status.WriteLine(LogPrefix + "Creating new prebuilt package at: {0}", PrebuiltPackageDir);

			// Copy and create build files.
			CreateNewBuildFile();

			CreateAndCopyOldInitializeXml();

			CopyAndUpdateManifestXml();

			// Now copy over build artifacts and other package files like include files and content files that is in this package.
			CopyBuildBinaries();

			CopyInPackageExternalBinaries();

			CopyIncludes();

			CopyNatvisFiles();

			CopyContentFiles();

			CopyExplicitIncludeFiles();

			if (!String.IsNullOrEmpty(OutReferenceSourcePathsProperty) && mPathsReferencingSourceTree.Any())
			{
				Project.Properties[OutReferenceSourcePathsProperty] = string.Join(Environment.NewLine, mPathsReferencingSourceTree) + Environment.NewLine;
			}

			Project.Log.Status.WriteLine(LogPrefix + "Prebuilt package successfully created at: {0}", PrebuiltPackageDir);
		}

		private string BuildPropertyConditionString(Project project)
		{
			if (String.IsNullOrWhiteSpace(TestForBooleanProperties))
			{
				return "";
			}

			SortedList<string,string> requiredProperties = new SortedList<string, string>();
			foreach (string propertyName in TestForBooleanProperties.Split())
			{
				if (project.Properties.Contains(propertyName))
				{
					if (project.Properties.GetBooleanProperty(propertyName))
					{
						requiredProperties.Add(propertyName, "${" + propertyName + "??false}");
					}
				}
			}

			string propertyCondition = "";
			if (requiredProperties.Any())
			{
				propertyCondition = String.Join(" and ", requiredProperties.Select( p => p.Value ));
			}

			return propertyCondition;
		}

		private void BuildPackageModuleInfo()
		{
			Project.Log.Status.WriteLine(LogPrefix + "Extracting all module info in package {0} for package.configs list: {1}", PackageName, Project.Properties["package.configs"]);

			mPackageData = new PrebuiltPackageInfo();

			string PrebuiltExcludeFileSetName = String.Format("package.{0}.PrebuiltExcludeSourceFiles",PackageName);
			if (Project.NamedFileSets.Contains(PrebuiltExcludeFileSetName))
			{
				FileSet excludeSet = Project.NamedFileSets[PrebuiltExcludeFileSetName];
				excludeSet.FailOnMissingFile = false;
				foreach (FileItem item in excludeSet.FileItems)
				{
					mPackageData.mPrebuiltPackageExcludeFiles.Add(PathString.MakeNormalized(item.FullPath, PathString.PathParam.NormalizeOnly).Path);
				}
			}

			string PrebuiltIncludeFileSetName = String.Format("package.{0}.PrebuiltIncludeSourceFiles",PackageName);
			if (Project.NamedFileSets.Contains(PrebuiltIncludeFileSetName))
			{
				FileSet includeSet = Project.NamedFileSets[PrebuiltIncludeFileSetName];
				includeSet.FailOnMissingFile = false;
				foreach (FileItem item in includeSet.FileItems)
				{
					mPackageData.mPrebuiltPackageIncludeFiles.Add(PathString.MakeNormalized(item.FullPath, PathString.PathParam.NormalizeOnly).Path);
				}
			}

			string PrebuiltPackageExtraBuildFileSetName = String.Format("package.{0}.PrebuiltPackageExtraBuildFiles", PackageName);
			if (Project.NamedFileSets.Contains(PrebuiltPackageExtraBuildFileSetName))
			{
				FileSet includeSet = Project.NamedFileSets[PrebuiltPackageExtraBuildFileSetName];
				includeSet.FailOnMissingFile = false;
				foreach (FileItem item in includeSet.FileItems)
				{
					mPackageData.mPrebuiltPackageExtraBuildFiles.Add(PathString.MakeNormalized(item.FullPath, PathString.PathParam.NormalizeOnly).Path);
					mPackageData.mPrebuiltPackageIncludeFiles.Add(PathString.MakeNormalized(item.FullPath, PathString.PathParam.NormalizeOnly).Path);
				}
			}

			List<string> moduleSpecificPreservePropertyNames = new List<string>();
			if (!String.IsNullOrEmpty(PreserveProperties))
			{
				foreach (string prop in PreserveProperties.Split(new char[] { '\n', ';' }))
				{
					string expandedPropertyName = prop.Trim().Replace("%packagename%", PackageName).Replace("%modulename%","[modulename]");
					if (expandedPropertyName.Contains("%"))
					{
						Project.Log.Warning.WriteLine("create-prebuilt-package task's preserve-properties input currently only support %packagename% and %modulename% tokens!  This property will be ignored: {0}", expandedPropertyName);
						continue;
					}
					if (expandedPropertyName.Contains("[modulename]"))
					{
						// Will have to evaluate module specific item later when we iterate through the modules.  
						// For now, just cache this name in a list,
						if (!moduleSpecificPreservePropertyNames.Contains(expandedPropertyName))
							moduleSpecificPreservePropertyNames.Add(expandedPropertyName);
						continue;
					}
					if (Project.Properties.Contains(expandedPropertyName) &&
						!mPackageData.mPreserveProperties.ContainsKey(expandedPropertyName))
					{
						string propertyValue = Project.Properties[expandedPropertyName].Replace("\r","").Replace("\n",Environment.NewLine);
						mPackageData.mPreserveProperties.Add(expandedPropertyName, propertyValue);
					}
				}
			}

			// The TopModules list can have same module from multiple configs.  We want to group all those info into the same group.
			foreach (IModule mod in Project.BuildGraph().TopModules)
			{
				ProcessableModule module = mod as ProcessableModule;

				// Because of the Frostbite usage (where we merge build graphs from different build,
				// we need to skip over modules that has null Project info - used dependent in one graph 
				// and build dependent on another graph).
				if (module == null || module.Package.Project == null)
					continue;

				// Get module specific properties.
				foreach (string modulePropertyName in moduleSpecificPreservePropertyNames)
				{
					string realPropName = modulePropertyName.Replace("[modulename]", module.Name);
					if (module.Package.Project.Properties.Contains(realPropName))
					{
						if (!mPackageData.mPreserveProperties.ContainsKey(realPropName))
						{
							mPackageData.mPreserveProperties.Add(realPropName, module.Package.Project.Properties[realPropName]);
						}
						else
						{
							if (mPackageData.mPreserveProperties[realPropName] != module.Package.Project.Properties[realPropName])
							{
								Project.Log.Warning.WriteLine("create-prebuilt-package task's preserve-properties for '{0}' have previously stored value '{1}', but encounter another instance which has value '{2}'.  This use case is currently not supported.  Please contact Frostbite EW Team if you need to update this tool.",
									realPropName, mPackageData.mPreserveProperties[realPropName], module.Package.Project.Properties[realPropName]);
								continue;
							}
						}
					}
				}

				// We test the PrebuiltIncludeSourceFiles again in case the fileset is different for different config.
				if (module.Package.Project.NamedFileSets.Contains(PrebuiltIncludeFileSetName))
				{
					FileSet includeSet = module.Package.Project.NamedFileSets[PrebuiltIncludeFileSetName];
					includeSet.FailOnMissingFile = false;
					foreach (FileItem item in includeSet.FileItems)
					{
						string newItem = PathString.MakeNormalized(item.FullPath, PathString.PathParam.NormalizeOnly).Path;
						if (!mPackageData.mPrebuiltPackageIncludeFiles.Contains(newItem))
						{
							mPackageData.mPrebuiltPackageIncludeFiles.Add(newItem);
						}
					}
				}

				// fail if any of the modules were built with fastlink enabled, fastlink pdb files are not portable and should not be distributed in prebuilt packages
				// Note that the current IntermediateDir setting could be null for the module if the module doesn't do build!
				if (module.IntermediateDir != null && !PrebuiltSkipPdb)
				{
					string tlog_directory = mod.GetVSTLogLocation().Path;
					if (Directory.Exists(tlog_directory))
					{
						string linker_tlog_file = Path.Combine(tlog_directory, "link.command.1.tlog");
						if (File.Exists(linker_tlog_file))
						{
							string linker_command_text = File.ReadAllText(linker_tlog_file);
							if (linker_command_text.IndexOf("/debug:fastlink", StringComparison.InvariantCultureIgnoreCase) >= 0)
							{
								throw new BuildException(String.Format("module '{0}' was built with /debug:fastlink and has non-portable PDB files. "
									+ "Please rebuild without fastlink by using the global property '-D:package.eaconfig.usedebugfastlink=false'.", mod.Name));
							}
						}
					}
				}

				string propertyCondition = BuildPropertyConditionString(module.Package.Project);

				if (!mPackageData.mModulesDataByProperty.ContainsKey(propertyCondition))
				{
					mPackageData.mModulesDataByProperty.Add(propertyCondition, new Dictionary<string, ModuleInfo>());
				}

				if (!mPackageData.mModulesDataByProperty[propertyCondition].ContainsKey(module.Name))
				{
					mPackageData.mModulesDataByProperty[propertyCondition].Add(module.Name, new ModuleInfo());
				}

				mPackageData.mModulesDataByProperty[propertyCondition][module.Name].SetupModuleConfigInfo(module);

				if (!mPackageData.mPackageLevelPublicDataByPropertyAndConfig.ContainsKey(propertyCondition))
				{
					mPackageData.mPackageLevelPublicDataByPropertyAndConfig.Add(propertyCondition, new Dictionary<string, PrebuiltPublicData>());
				}

				if (!mPackageData.mPackageLevelPublicDataByPropertyAndConfig[propertyCondition].ContainsKey(module.Configuration.Name))
				{
					// We are collecting config specific data, so make sure we use the module.Package.Project instead of Project.  The
					// current "Project" is started by a different config and may not be what you wanted.
					mPackageData.mPackageLevelPublicDataByPropertyAndConfig[propertyCondition].Add(module.Configuration.Name, new PrebuiltPublicData(module.Package.Project, PackageName, null));

					// Do some backward compatibility addition.  Some packages added <publicdata> with module specific <publicdependencies> setup but also
					// provided package level includedirs property which reference external package.  In this case we gather all module's publicdependencies
					// setup and dump it to package level.
					if (!mPackageData.mPackageLevelPublicDataByPropertyAndConfig[propertyCondition][module.Configuration.Name].mPublicDependencies.Any() &&
						mPackageData.mPackageLevelPublicDataByPropertyAndConfig[propertyCondition][module.Configuration.Name].IncludeDirectories.Any() &&
						mPackageData.mModulesDataByProperty.ContainsKey(propertyCondition))
					{
						Dictionary<string,string> allModulesPublicDependencies = new Dictionary<string, string>();
						foreach (var mc in mPackageData.mModulesDataByProperty[propertyCondition].Select(c=>c.Value.mModulePublicDataByConfig))
						{
							foreach (var pdList in mc.Select(m=>m.Value.mPublicDependencies))
							{
								foreach (var pd in pdList)
								{
									if (!allModulesPublicDependencies.ContainsKey(pd.Key))
										allModulesPublicDependencies.Add(pd.Key, pd.Value);
								}
							}
						}
						if (allModulesPublicDependencies.Any())
						{
							mPackageData.mPackageLevelPublicDataByPropertyAndConfig[propertyCondition][module.Configuration.Name].mPublicDependencies = allModulesPublicDependencies;
						}
					}
				}

				if (!mPackageData.mConfigDetailsByConfig.ContainsKey(module.Configuration.Name))
				{
					mPackageData.mConfigDetailsByConfig.Add(module.Configuration.Name, module.Configuration);
				}
			}
		}

		private string GetBuildGroupXmlElementName(string group)
		{
			if (group == "runtime")
			{
				return group;
			}
			else
			{
				// the sxml group names other than runtime need to be plural, for example <tests> instead of <test>
				return group + "s";
			}
		}

		// Extract all modules for this build group and re-package as dictionary Tuple(module_name,property_condition) to [module info]
		private Dictionary< Tuple<string, string>, ModuleInfo > ExtractModulesForBuildGroup(string buildgroup)
		{
			Dictionary< Tuple<string, string>, ModuleInfo > buildGroupModules = new Dictionary< Tuple<string, string>, ModuleInfo >();
			foreach (var p in mPackageData.mModulesDataByProperty)
			{
				foreach (var module in p.Value)
				{
					if (module.Value.mModuleBuildGroup == buildgroup)
					{
						buildGroupModules.Add(new Tuple<string, string>(module.Key, p.Key), module.Value);
					}
				}
			}

			return buildGroupModules;
		}

		private void CreateNewBuildFile()
		{
			Project.Log.Status.WriteLine(LogPrefix + "Constructing a new build file ...");

			string indentationDelta = "    ";
			StringBuilder currIndentation = new StringBuilder();

			StringBuilder newBuildFileContent = new StringBuilder();
			newBuildFileContent.AppendLine("<project xmlns=\"schemas/ea/framework3.xsd\">");
			newBuildFileContent.AppendLine();
			currIndentation.Append(indentationDelta);
			newBuildFileContent.AppendLine(String.Format("{0}<package name=\"{1}\"/>", currIndentation.ToString(), PackageName));
			newBuildFileContent.AppendLine();

			bool inFrostbiteConfig = !String.IsNullOrEmpty(Project.Properties.GetPropertyOrDefault("frostbite.target",null));

			// Frostbite Engine packages has setup the following properties to re-define the csproj/vcxproj/solution file names
			// through the use of set-fb-vs-solution-name task.  We need to re-export those properties as well if they are defined
			// by calling the same task in the build file!
			if (
				   inFrostbiteConfig
				&& Project.Properties.Contains("runtime.gen.slndir.template")
				&& Project.Properties.Contains("runtime.gen.slnname.template")
				&& Project.Properties.Contains("runtime.gen.projectname.template")
				&& Project.Properties.Contains("tool.gen.slndir.template")
				&& Project.Properties.Contains("tool.gen.slnname.template")
				&& Project.Properties.Contains("tool.gen.projectname.template")
				&& Project.Properties.Contains("test.gen.slndir.template")
				&& Project.Properties.Contains("test.gen.slnname.template")
				&& Project.Properties.Contains("test.gen.projectname.template")
				)
			{
				// Do one more final quick sanity check to make sure that these are indeed Frostbite's package override
				if (Project.Properties["runtime.gen.slnname.template"].StartsWith(PackageName + "_") &&
				    Project.Properties["runtime.gen.projectname.template"].Contains("_" + PackageName + "_"))
				{
					newBuildFileContent.AppendLine(String.Format("{0}<set-fb-vs-solution-name baseName=\"${{package.name}}\"/>", currIndentation.ToString()));
					newBuildFileContent.AppendLine();
				}
			}

			foreach (string buildgroup in new string[] {"runtime", "tool", "example", "test"})
			{
				// Extract all modules for this build group and re-package as dictionary [module_name] to Tuple(property condition, module info)
				Dictionary<Tuple<string, string>, ModuleInfo> buildGroupModules = ExtractModulesForBuildGroup(buildgroup);

				if (!buildGroupModules.Any())
					continue;

				if (buildgroup != "runtime")
				{
					newBuildFileContent.AppendLine(String.Format("{0}<{1}>", currIndentation.ToString(), GetBuildGroupXmlElementName(buildgroup)));
					newBuildFileContent.AppendLine();
					currIndentation.Append(indentationDelta);
				}

				foreach (KeyValuePair<Tuple<string,string>, ModuleInfo> module in buildGroupModules.OrderBy(m=>m.Key.Item1))
				{
					string currModuleName = module.Key.Item1;
					string propertiesCondition = module.Key.Item2;
					ModuleInfo moduleData = module.Value;

					if (currModuleName == EA.Eaconfig.Modules.Module_UseDependency.PACKAGE_DEPENDENCY_NAME)
					{
						// This package is old style package being loaded as used dependency and doesn't
						// really have module info.  Reset the dummy module name to package name.
						currModuleName = PackageName;
					}

					if (!moduleData.mModuleTypeByConfig.Any())
					{
						continue;
					}

					string transitivelinkAttrib = "";
					List<string> configsRequireTransitiveLink = new List<string>();
					foreach (KeyValuePair<string, uint> moduleType in moduleData.mModuleTypeByConfig)
					{
						string config = moduleType.Key;
						BitMask type = new BitMask(moduleType.Value);
						if (type.IsKindOf(Module.Library))
						{
							configsRequireTransitiveLink.Add("${config}==" + config);
						}
					}
					if (configsRequireTransitiveLink.Any())
					{
						if (configsRequireTransitiveLink.Count == moduleData.mModuleTypeByConfig.Count)
						{
							transitivelinkAttrib = " transitivelink=\"true\"";
						}
						else
						{
							transitivelinkAttrib = String.Format(" transitivelink=\"{0}\"", String.Join(" or ", configsRequireTransitiveLink));
						}
					}

					string configSpecificCondition = "";
					if (moduleData.mModuleTypeByConfig.Count != mPackageData.mConfigDetailsByConfig.Count)
					{
						// This module is only intended for a specific set of configs from the full config set that the package support.
						configSpecificCondition = "${config}==" + String.Join(" or ${config}==", moduleData.mModuleTypeByConfig.Keys.ToArray());
					}

					List<string> finalConditionList = new List<string>();
					if (!string.IsNullOrEmpty(configSpecificCondition))
					{
						finalConditionList.Add("(" + configSpecificCondition + ")");
					}
					if (!string.IsNullOrEmpty(propertiesCondition))
					{
						finalConditionList.Add("(" + propertiesCondition + ")");
					}
					string finalCondition = "";
					if (finalConditionList.Any())
					{
						finalCondition = " if=\"" + String.Join(" and ", finalConditionList) + "\"";
					}

					if (moduleData.mModuleHasDdfByConfig.Any())
					{
						if (moduleData.mModuleHasDdfByConfig.Where(m => m.Value==true).Count() == moduleData.mModuleHasDdfByConfig.Count)
						{
							newBuildFileContent.AppendLine(String.Format("{0}<property name=\"{1}.{2}.has-ddf-files\" value=\"true\"{3}/>", currIndentation.ToString(), buildgroup, currModuleName, finalCondition));
						}
						else
						{
							foreach (KeyValuePair<string,bool> item in moduleData.mModuleHasDdfByConfig)
							{
								List<string> currConfigConditionList = new List<string>();
								currConfigConditionList.Add("${config}==" + item.Key);
								if (!string.IsNullOrEmpty(propertiesCondition))
								{
									currConfigConditionList.Add("(" + propertiesCondition + ")");
								}
								if (item.Value)
								{
									newBuildFileContent.AppendLine(String.Format("{0}<property name=\"{1}.{2}.has-ddf-files\" value=\"true\" if=\"{3}\"/>", currIndentation.ToString(), buildgroup, currModuleName, String.Join(" and ", currConfigConditionList)));
								}
							}
						}
					}

					if (moduleData.mIsSharedPchModule)
					{
						// A Shared PCH module cannot be turned into Utility module as we don't cache it's build output.
						// We duplicate the SharedPch module definition.  All information necessary to define a 
						// SharedPch module is actally defined in Initialize.xml properties.  So this new SharedPch module
						// definition just need to reference what is already setup in Initialize.xml.
						newBuildFileContent.AppendLine(String.Format("{0}<SharedPch name=\"{1}\"{2} />", currIndentation.ToString(), currModuleName, finalCondition));
					}
					else // Regular module
					{
						// All other modules are turned into Utility modules.
						newBuildFileContent.AppendLine(String.Format("{0}<Utility name=\"{1}\"{2}{3}>", currIndentation.ToString(), currModuleName, transitivelinkAttrib, finalCondition));

						currIndentation.Append(indentationDelta);

						if (moduleData.mDependentsByConfig.Any())
						{
							bool moduleHasDependent = false;
							HashSet<string> configCommonList = null;
							foreach (KeyValuePair<string, HashSet<string>> configSpecific in moduleData.mDependentsByConfig)
							{
								moduleHasDependent = moduleHasDependent || configSpecific.Value.Any();
								if (configCommonList != null)
								{
									configCommonList.IntersectWith(configSpecific.Value);
								}
								else
								{
									configCommonList = new HashSet<string>(configSpecific.Value);
								}
							}

							if (moduleHasDependent)
							{
								if (moduleData.mModuleSkipRuntimeDependency)
								{
									newBuildFileContent.AppendLine(String.Format("{0}<dependencies skip-runtime-dependency=\"true\">", currIndentation.ToString()));
								}
								else
								{
									newBuildFileContent.AppendLine(String.Format("{0}<dependencies>", currIndentation.ToString()));
								}
								currIndentation.Append(indentationDelta);

								foreach (string depType in new string[] { "interface", "use", "build", "build link=\"false\"", "auto", "auto link=\"false\"" })
								{
									StringBuilder dependentBlockContent = new StringBuilder();

									currIndentation.Append(indentationDelta);
									string currDepTypeMatch = "<" + depType + ">";

									// Write out dependents that is common to all config.
									foreach (string dep in configCommonList)
									{
										if (dep.StartsWith(currDepTypeMatch))
										{
											dependentBlockContent.AppendLine(String.Format("{0}{1}", currIndentation.ToString(), dep.Substring(currDepTypeMatch.Length)));
										}
									}

									// Write out config specific dependents.
									foreach (KeyValuePair<string, HashSet<string>> configSpecific in moduleData.mDependentsByConfig)
									{
										HashSet<string> configDep = new HashSet<string>(configSpecific.Value);
										configDep.ExceptWith(configCommonList);
										if (configDep.Any())
										{
											dependentBlockContent.AppendLine(String.Format("{0}<do if=\"${{config}}=={1}\">", currIndentation.ToString(), configSpecific.Key));
											currIndentation.Append(indentationDelta);

											foreach (string dep in configDep)
											{
												if (dep.StartsWith(currDepTypeMatch))
												{
													dependentBlockContent.AppendLine(String.Format("{0}{1}", currIndentation.ToString(), dep.Substring(currDepTypeMatch.Length)));
												}
											}

											currIndentation.Remove(0, indentationDelta.Length);

											dependentBlockContent.AppendLine(String.Format("{0}</do>", currIndentation.ToString()));
										}
									}

									currIndentation.Remove(0, indentationDelta.Length);

									string finalDependentBlockContent = dependentBlockContent.ToString();
									if (!String.IsNullOrEmpty(finalDependentBlockContent))
									{
										newBuildFileContent.AppendLine(String.Format("{0}<{1}>", currIndentation.ToString(), depType));
										newBuildFileContent.Append(finalDependentBlockContent);
										string closeTag = depType.Split(new char[] { ' ' }).First();
										newBuildFileContent.AppendLine(String.Format("{0}</{1}>", currIndentation.ToString(), closeTag));
									}
								}

								currIndentation.Remove(0, indentationDelta.Length);
								newBuildFileContent.AppendLine(String.Format("{0}</dependencies>", currIndentation.ToString()));
							}
						}

						// Test if module is a program type module containing package.[pkgName].[module].programs fileset.
						// If it is, we need to add <filecopystep> to copy the exe to expected destination location.
						if (moduleData.mModulePublicDataByConfig.Where(p => p.Value.mPrograms.Any()).Any())
						{
							Dictionary<string, string> configDestDir = new Dictionary<string, string>();
							foreach (var configSpecificPubdata in moduleData.mModulePublicDataByConfig)
							{
								// For module specific programs fileset, we really expect only one file being listed.
								string currConfig = configSpecificPubdata.Key;
								PrebuiltPublicData currPubData = configSpecificPubdata.Value;
								string destDir = currPubData.mPrograms.Any() ? Path.GetDirectoryName(ConvertFullPathToTokenPath(currPubData.mPublicDataProject, currPubData.mPrograms.First(), currConfig, new Dictionary<string, string>())) : null;
								configDestDir[currConfig] = destDir;
							}
							if (configDestDir.Select(kv => kv.Value).ToHashSet().Count == 1)
							{
								// Output path is common to all config, just create one instruction.
								string destDir = configDestDir.First().Value;
								if (!String.IsNullOrEmpty(destDir))
								{
									newBuildFileContent.AppendLine();
									newBuildFileContent.AppendLine(String.Format("{0}<filecopystep todir=\"{1}\"{2}>", currIndentation.ToString(), destDir, finalCondition));
									currIndentation.Append(indentationDelta);
									newBuildFileContent.AppendLine(String.Format("{0}<fileset>", currIndentation.ToString()));
									currIndentation.Append(indentationDelta);
									newBuildFileContent.AppendLine(String.Format("{0}<includes fromfileset=\"package.{1}.{2}.programs\"/>", currIndentation.ToString(), PackageName, currModuleName));
									currIndentation.Remove(0, indentationDelta.Length);
									newBuildFileContent.AppendLine(String.Format("{0}</fileset>", currIndentation.ToString()));
									currIndentation.Remove(0, indentationDelta.Length);
									newBuildFileContent.AppendLine(String.Format("{0}</filecopystep>", currIndentation.ToString()));
								}
							}
							else
							{
								// Unable to resolve common output path for all config.  Write out instruction for each config.
								foreach (var destDirKv in configDestDir)
								{
									if (!String.IsNullOrEmpty(destDirKv.Value))
									{
										string conditionStr = String.Format(" if=\"${{config}}=={0}{1}\"", destDirKv.Key, String.IsNullOrEmpty(propertiesCondition) ? "" : " and (" + propertiesCondition + ")");
										newBuildFileContent.AppendLine();
										newBuildFileContent.AppendLine(String.Format("{0}<filecopystep todir=\"{1}\"{2}>", currIndentation.ToString(), destDirKv.Value, conditionStr));
										currIndentation.Append(indentationDelta);
										newBuildFileContent.AppendLine(String.Format("{0}<fileset>", currIndentation.ToString()));
										currIndentation.Append(indentationDelta);
										newBuildFileContent.AppendLine(String.Format("{0}<includes fromfileset=\"package.{1}.{2}.programs\">", currIndentation.ToString(), PackageName, currModuleName));
										currIndentation.Remove(0, indentationDelta.Length);
										newBuildFileContent.AppendLine(String.Format("{0}</fileset>", currIndentation.ToString()));
										currIndentation.Remove(0, indentationDelta.Length);
										newBuildFileContent.AppendLine(String.Format("{0}</filecopystep>", currIndentation.ToString()));
									}
								}
							}
						}

						currIndentation.Remove(0, indentationDelta.Length);
						newBuildFileContent.AppendLine(String.Format("{0}</Utility>", currIndentation.ToString()));
					}
					newBuildFileContent.AppendLine();
				}
				if (buildgroup != "runtime")
				{
					currIndentation.Remove(0, indentationDelta.Length);
					newBuildFileContent.AppendLine(String.Format("{0}</{1}>", currIndentation.ToString(), GetBuildGroupXmlElementName(buildgroup)));
				}
			}

			if (mPackageData.mPrebuiltPackageExtraBuildFiles.Any())
			{
				// Add this at end of build file
				// <foreach item="FileSet" in="package.[PackageName].PrebuiltPackageExtraBuildFiles" property="filename"> 
				//     <include file="${filename}"/>
				// </foreach>

				newBuildFileContent.AppendLine();
				newBuildFileContent.AppendLine(String.Format("{0}<foreach item=\"FileSet\" in=\"package.{1}.PrebuiltPackageExtraBuildFiles\" property=\"buildfile\">", currIndentation.ToString(), PackageName));
				currIndentation.Append(indentationDelta);
				newBuildFileContent.AppendLine(String.Format("{0}<include file=\"${{buildfile}}\"/>", currIndentation.ToString()));
				currIndentation.Remove(0, indentationDelta.Length);
				newBuildFileContent.AppendLine(String.Format("{0}</foreach>",currIndentation.ToString()));
				newBuildFileContent.AppendLine();
			}

			newBuildFileContent.AppendLine();
			newBuildFileContent.AppendLine("</project>");

			string fileContent = newBuildFileContent.ToString();

			if (!Directory.Exists(PrebuiltPackageDir))
			{
				Directory.CreateDirectory(PrebuiltPackageDir);
			}

			string buildFileName = string.Format("{0}.build", Project.Properties["package.name"]);
			string newBuildFile = Path.Combine(PrebuiltPackageDir, buildFileName);
			string oldBuildFile = Path.Combine(PackageDir, buildFileName);

			File.WriteAllText(newBuildFile, fileContent);

			// Add the build file to the list of files to exclude to make sure it won't get overwritten.
			if (!mPackageData.mPrebuiltPackageExcludeFiles.Contains(oldBuildFile))
			{
				mPackageData.mPrebuiltPackageExcludeFiles.Add(oldBuildFile);
			}
		}

		private string ConvertFullPathToTokenPath(Project currProject, string path, string currConfig, Dictionary<string, string> dependencyList, bool skipPackageDirCheck = false)
		{
			string pathOut = path;

			int startTokenLen = 0;
			if (pathOut.StartsWith(PackageBuildDir))
			{
				// We are not shipping build directory.  Change non-binaries generated files to different folder.
				pathOut = String.Format("{0}{1}", PrebuiltGeneratedFilesDirToken, pathOut.Substring(PackageBuildDir.Length));
				startTokenLen = PrebuiltGeneratedFilesDirToken.Length;
			}
			else if (pathOut.StartsWith(PackageGenDir))
			{
				// We are not shipping build directory.  Change non-binaries generated files to different folder.
				pathOut = String.Format("{0}{1}", PrebuiltGeneratedFilesDirToken, pathOut.Substring(PackageGenDir.Length));
				startTokenLen = PrebuiltGeneratedFilesDirToken.Length;
			}
			else if (pathOut.StartsWith(PackageDir))
			{
				if (!skipPackageDirCheck)
				{
					pathOut = String.Format("{0}{1}", PackageDirToken, pathOut.Substring(PackageDir.Length));
					startTokenLen = PackageDirToken.Length;
				}
			}
			else if (dependencyList != null)
			{
				foreach (KeyValuePair<string, string> item in dependencyList)
				{
					// If the include path contains paths to external packages, we'll simply strip them out.
					// We'll explicitly add dependency list's includedirs property later on.
					if (pathOut.StartsWith(item.Value))
					{
						return "";
					}
				}
			}

			if (pathOut.StartsWith(NantBuildRoot) && startTokenLen==0)
			{
				pathOut = String.Format("{0}{1}",NantBuildRootToken, pathOut.Substring(NantBuildRoot.Length));
				startTokenLen = NantBuildRootToken.Length;
			}

			if (!String.IsNullOrWhiteSpace(currConfig))
			{
				pathOut = StringReplaceNotInToken(pathOut, currConfig, "${config}", startTokenLen);
			}

			foreach (string propName in ExtraTokenMappings.Split(new char[] {';'}))
			{
				string propVal = currProject.Properties.GetPropertyOrDefault(propName,null);
				if (!String.IsNullOrEmpty(propVal))
				{
					string newPathOut = StringReplaceNotInToken(pathOut, propVal, "${" + propName + "}", startTokenLen);
					if (newPathOut == pathOut && (propVal.Contains("../") || propVal.Contains("..\\")))
					{
						propVal = PathString.MakeNormalized(propVal).Path;
						pathOut = StringReplaceNotInToken(pathOut, propVal, "${" + propName + "}", startTokenLen);
					}
					else
					{
						pathOut = newPathOut;
					}
				}
			}

			return pathOut;
		}

		private string StringReplaceNotInToken(string srcString, string oldVal, string newVal, int startIdx=0)
		{
			// Similar to a regular StringReplace except that we want to make sure that oldVal is not a sub-string
			// of an existing property token name.
			string outString = srcString;
			int startSearchIdx = startIdx;
			if (startSearchIdx < outString.Length)
			{
				int foundIdx = outString.IndexOf(oldVal, startSearchIdx);
				while (foundIdx >= startSearchIdx)
				{
					int startBracketIdx = outString.IndexOf("${", foundIdx);
					int closeBracketIdx = outString.IndexOf('}', foundIdx);
					if (closeBracketIdx > foundIdx && (startBracketIdx<0 || startBracketIdx > closeBracketIdx))
					{
						// We must be inside a token.  Skip this one.
						startSearchIdx = closeBracketIdx+1;
						if (startSearchIdx < outString.Length)
						{
							foundIdx = outString.IndexOf(oldVal, startSearchIdx);
						}
						else
						{
							foundIdx = -1;
						}
					}
					else
					{
						outString = outString.Substring(0,foundIdx) + newVal + outString.Substring(foundIdx+oldVal.Length);
						startSearchIdx = foundIdx + newVal.Length;
						if (startSearchIdx < outString.Length)
						{
							foundIdx = outString.IndexOf(oldVal, startSearchIdx);
						}
						else
						{
							foundIdx = -1;
						}
					}
				}
			}
			return outString;
		}

		private bool FilesetContainFilesInFolderRoots(IEnumerable<string> fileset, IEnumerable<string> folderRoots)
		{
			return fileset.Where( file => folderRoots.Where(root => file.StartsWith(root)).Any() ).Any();
		}

		enum ExportType
		{
			IncludeDir,
			InternalIncludeDir,
			DDFIncludeDir,
			Libs,
			Dlls,
			Assemblies,
			Programs,
			ContentFiles,
			NatvisFiles
		}

		// This function is mainly used by CreateAndCopyOldInitializeXml() below.
		private string BuildInitializeXmlExportContent(
				ExportType exportType, 
				string buildgroup,
				string currModuleName, 
				string condition,
				Dictionary<string, PrebuiltPublicData> exportPublicData, 
				string currentIndentation, 
				string indentationDelta)
		{
			StringBuilder output = new StringBuilder();
			StringBuilder currIndent = new StringBuilder(currentIndentation);

			if (!exportPublicData.Any())
				return "";

			string xmlElementName = "";
			bool elementIsProperty = false;
			string xmlFormatString = "";
			string exportName = "";
			switch (exportType)
			{
				case ExportType.IncludeDir:
					xmlElementName = "property";
					elementIsProperty = true;
					exportName = String.Format("package.{0}{1}.includedirs", PackageName, (currModuleName==null ? "" : "." + currModuleName));
					xmlFormatString = "{0}{1}{2}";
					break;
				case ExportType.InternalIncludeDir:
					xmlElementName = "property";
					elementIsProperty = true;
					exportName = String.Format("package.{0}{1}.internalIncludeDirs", PackageName, (currModuleName == null ? "" : "." + currModuleName));
					xmlFormatString = "{0}{1}{2}";
					break;
				case ExportType.DDFIncludeDir:
					xmlElementName = "property";
					elementIsProperty = true;
					exportName = String.Format("package.{0}{1}.ddfincludedirs", PackageName, (currModuleName == null ? "" : "." + currModuleName));
					xmlFormatString = "{0}{1}{2}";
					break;
				case ExportType.Libs:
					xmlElementName = "fileset";
					elementIsProperty = false;
					exportName = String.Format("package.{0}{1}.libs", PackageName, (currModuleName == null ? "" : "." + currModuleName));
					xmlFormatString = "{0}<includes name=\"{1}\" {2}/>";
					break;
				case ExportType.Dlls:
					xmlElementName = "fileset";
					elementIsProperty = false;
					exportName = String.Format("package.{0}{1}.dlls", PackageName, (currModuleName == null ? "" : "." + currModuleName));
					xmlFormatString = "{0}<includes name=\"{1}\" {2}/>";
					break;
				case ExportType.Assemblies:
					xmlElementName = "fileset";
					elementIsProperty = false;
					exportName = String.Format("package.{0}{1}.assemblies", PackageName, (currModuleName == null ? "" : "." + currModuleName));
					xmlFormatString = "{0}<includes name=\"{1}\" {2}/>";
					break;
				case ExportType.Programs:
					xmlElementName = "fileset";
					elementIsProperty = false;
					exportName = String.Format("package.{0}{1}.programs", PackageName, (currModuleName == null ? "" : "." + currModuleName));
					xmlFormatString = "{0}<includes name=\"{1}\" {2}/>";
					break;
				case ExportType.NatvisFiles:
					xmlElementName = "fileset";
					elementIsProperty = false;
					exportName = String.Format("package.{0}{1}.natvisfiles", PackageName, (currModuleName == null ? "" : "." + currModuleName));
					xmlFormatString = "{0}<includes name=\"{1}\" {2}/>";
					break;
				case ExportType.ContentFiles:
					xmlElementName = "fileset";
					elementIsProperty = false;
					exportName = String.Format("package.{0}{1}.contentfiles", PackageName, (currModuleName == null ? "" : "." + currModuleName));
					xmlFormatString = "{0}<includes name=\"{1}\" {2}/>";
					break;
				default:
					throw new BuildException("Programming error: create-prebuilt-package task is processing an unknown export type!");
			}

			HashSet<string> commonTokenizedSet = null;
			Dictionary<string, HashSet<string>> configTokenizedSet = new Dictionary<string, HashSet<string>>();
			Dictionary<string, string> tokenizedFileOptionset = new Dictionary<string, string>();
			string extraElementAttribute = "";

			// Storing the external package names where we will need to do an explicit dependent task because we are going to reference
			// that external package's includedirs property that is defined in that package's Initialize.xml
			HashSet<string> commonExternalDepPkgSet = new HashSet<string>();
			Dictionary<string, HashSet<string>> configExternalDepPkgSet = new Dictionary<string, HashSet<string>>();

			// Build the set that is common to all configs
			foreach (KeyValuePair<string, PrebuiltPublicData> configData in exportPublicData)
			{
				List<string> exportData = null;
				FileSet exportFileSet = null;
				switch (exportType)
				{
					case ExportType.IncludeDir:
						exportData = configData.Value.mIncludeDirs;
						break;
					case ExportType.InternalIncludeDir:
						exportData = configData.Value.mInternalIncludeDirs;
						break;
					case ExportType.DDFIncludeDir:
						exportData = configData.Value.mDdfIncludeDirs;
						break;
					case ExportType.Libs:
						exportData = configData.Value.mLibs;
						break;
					case ExportType.Dlls:
						exportData = configData.Value.mDlls;
						break;
					case ExportType.Assemblies:
						exportData = configData.Value.mAssemblies;
						break;
					case ExportType.Programs:
						exportData = configData.Value.mPrograms;
						break;
					case ExportType.NatvisFiles:
						exportData = configData.Value.mNatvisFiles;
						break;
					case ExportType.ContentFiles:
						exportData = configData.Value.mContentFiles;
						exportFileSet = configData.Value.ContentFiles;
						if (!exportFileSet.FailOnMissingFile)
						{
							extraElementAttribute = " failonmissing=\"false\"";
						}
						break;
					default:
						throw new BuildException("Programming error: create-prebuilt-package task is processing an unknown export type!");
				}

				configTokenizedSet.Add(configData.Key, new HashSet<string>());
				configExternalDepPkgSet.Add(configData.Key, new HashSet<string>());
				Project currProject = configData.Value.mPublicDataProject;
				if (exportData != null && exportData.Any())
				{
					foreach (string path in exportData)
					{
						string tokenizedPath = "";
						switch (exportType)
						{
							case ExportType.IncludeDir:
								tokenizedPath = ConvertFullPathToTokenPath(currProject, path, configData.Key, configData.Value.mPublicDependencies,
													RedirectSourceIncludePath(path));
								break;
							case ExportType.InternalIncludeDir:
								tokenizedPath = ConvertFullPathToTokenPath(currProject, path, configData.Key, configData.Value.mPublicDependencies,
													RedirectSourceIncludePath(path));
								break;
							case ExportType.DDFIncludeDir:
								tokenizedPath = ConvertFullPathToTokenPath(currProject, path, configData.Key, configData.Value.mPublicDependencies,
													RedirectSourceIncludePath(path));
								break;
							case ExportType.NatvisFiles:
								// Need to preserve original folder structure.
								tokenizedPath = ConvertFullPathToTokenPath(currProject, path, configData.Key, configData.Value.mPublicDependencies,
													false);
								break;
							case ExportType.Libs:
								if (configData.Value.mDlls.Contains(path))
								{
									// This happens with unix config where we are using the output dll as input lib because unix build
									// don't create stub libs for dll.
									tokenizedPath = PrebuiltConfigBinDirToken;
								}
								else
								{
									// Happens with platform like NX where the dlls stub lib (.nrs) need to be placed side by side the dll (.nro) file in bin folder.
									// So we need to test original file was actually located in bin folder and we should preserve that when constructing the new Initialize.xml
									string binDir = configData.Value.mPublicDataProject.Properties.GetPropertyOrDefault("package.configbindir", "");
									if (currModuleName != null)
									{
										binDir = BuildFunctions.GetModuleOutputDirEx(configData.Value.mPublicDataProject, "bin", PackageName, currModuleName, buildgroup);
									}
									if (!String.IsNullOrEmpty(binDir) && path.StartsWith(binDir))
									{
										tokenizedPath = PrebuiltConfigBinDirToken;
									}
									else
									{
										tokenizedPath = PrebuiltConfigLibDirToken;
									}
								}
								if (buildgroup != null && buildgroup != "runtime")
									tokenizedPath = Path.Combine(tokenizedPath,buildgroup);
								tokenizedPath = Path.Combine(tokenizedPath, Path.GetFileName(path));
								break;
							case ExportType.Dlls:
							case ExportType.Assemblies:
							case ExportType.Programs:
								tokenizedPath = PrebuiltConfigBinDirToken;
								if (buildgroup != null && buildgroup != "runtime")
									tokenizedPath = Path.Combine(tokenizedPath,buildgroup);
								tokenizedPath = Path.Combine(tokenizedPath, Path.GetFileName(path));
								break;
							case ExportType.ContentFiles:
								string rebasedPath = path;
								if (path.StartsWith(PackageBuildDir) || path.StartsWith(PackageGenDir) || path.StartsWith(NantBuildRoot))
								{
									// By default when content files are being copied, it is designed to copy with relative path to basedir (which default setup to package dir)
									// If actual content file doesn't start with package dir, it will be copied flattened.  The following basically 
									// simulate that without actually bother to setup basedir.
									rebasedPath = Path.Combine(PackageDir, Path.GetFileName(path));
								}
								tokenizedPath = ConvertFullPathToTokenPath(currProject, rebasedPath, configData.Key, configData.Value.mPublicDependencies);
								// If the file lives in builddir or buildroot, we need to move it to new location.
								if (tokenizedPath.StartsWith(NantBuildRootToken))
									tokenizedPath = PrebuiltGeneratedFilesDirToken + tokenizedPath.Substring(NantBuildRootToken.Length);
								break;
							default:
								throw new BuildException("Programming error: create-prebuilt-package task is processing an unknown export type!");
						}

						if (String.IsNullOrEmpty(tokenizedPath))
						{
							continue;
						}

						if (!tokenizedPath.StartsWith("${"))
						{
							// We must have missed something.  We cannot override the export data leaving absolute path for a specific computer.
							// Throw an error notify user of this problem.  They might have setup explicit include path for another package like
							// <dependent name="ABC"/>
							// ... <includes name="${package.ABC.dir}\include"/>
							// We currently couldn't handle this at the moment.  However, people can change their build script to use
							// <publicdata><module><publicdependencies> to deal with dependencies' includes.  
							// Fow now, we just throw an error and see if we can let user change their build script with this use case.
							throw new BuildException(
								String.Format("We encountered an export path for '{0}' in initialize.xml that we cannot properly tokenize.  Are you explicitly including paths from another package?  The path having problem tokenizing is: {1}",
												exportName, path));
						}

						configTokenizedSet[configData.Key].Add(tokenizedPath);
						if (exportFileSet != null)
						{
							IEnumerable<FileItem> founditems = exportFileSet.FileItems.Where(item => String.Compare(item.FullPath,path,true)==0);
							if (founditems.Any())
							{
								string optionsetname = founditems.First().OptionSetName;
								if (!string.IsNullOrEmpty(optionsetname))
								{
									if (tokenizedFileOptionset.ContainsKey(tokenizedPath))
									{
										tokenizedFileOptionset[tokenizedPath] = optionsetname;
									}
									else
									{
										tokenizedFileOptionset.Add(tokenizedPath, optionsetname);
									}
								}
							}
						}
					}
				}

				// Add publicdependencies' includedirs
				if (exportType == ExportType.IncludeDir)
				{
					foreach (KeyValuePair<string, string> dep in configData.Value.mPublicDependencies)
					{
						if (dep.Key.EndsWith("includedirsprops"))
						{
							foreach (string includedirsPropName in dep.Value.Split(new char[] { ';' }))
							{
								string includedirsToken = "${" + includedirsPropName + "}";
								if (!configTokenizedSet[configData.Key].Contains(includedirsToken))
								{
									configTokenizedSet[configData.Key].Add(includedirsToken);
									configExternalDepPkgSet[configData.Key].Add(dep.Key.Substring(0,dep.Key.Length - "+includedirsprops".Length).Split('/').First());
								}
							}
						}
					}
				}

				if (commonTokenizedSet == null)
				{
					commonTokenizedSet = new HashSet<string>(configTokenizedSet[configData.Key]);
					commonExternalDepPkgSet = new HashSet<string>(configExternalDepPkgSet[configData.Key]);
				}
				else
				{
					commonTokenizedSet.IntersectWith(configTokenizedSet[configData.Key]);
					commonExternalDepPkgSet.IntersectWith(configExternalDepPkgSet[configData.Key]);
				}
			}

			// Now write out the common set.  Note that we're still writing out the condition for all configs even though this is
			// the common set because of the way we merge different build graphs and we ended up having same module listed twice
			// in build graph with different config list.  So in this function when we extract the info, it does not necessary
			// contains "ALL" configs in the build graph for the same module.
			string conditionAttrib = "";
			string allConfigsCondition = String.Join(" or ", exportPublicData.Select( data => String.Format("${{config}}=={0}",data.Key) ).Distinct());
			if (!string.IsNullOrEmpty(condition))
			{
				conditionAttrib = " if=\"(" + allConfigsCondition + ") and (" + condition + ")\"";
			}
			else
			{
				conditionAttrib = " if=\"" + allConfigsCondition + "\"";
			}

			if (commonExternalDepPkgSet != null && commonExternalDepPkgSet.Any())
			{
				// Need to add dependent task to external packages that we are using it's includedirs property.
				foreach (string pkg in commonExternalDepPkgSet)
				{
					output.AppendLine(string.Format("{0}<dependent name=\"{1}\"{2}/>", currIndent.ToString(), pkg, conditionAttrib));
				}
			}

			bool commonSetDefined = false;
			if (commonTokenizedSet != null && commonTokenizedSet.Any())
			{
				commonSetDefined = true;
				output.AppendLine(String.Format("{0}<{1} name=\"{2}\"{3}{4}>", currIndent.ToString(), xmlElementName, exportName, extraElementAttribute, conditionAttrib));
				currIndent.Append(indentationDelta);

				foreach (string path in commonTokenizedSet)
				{
					string optionset = "";
					if (tokenizedFileOptionset.ContainsKey(path))
					{
						optionset = "optionset=\"" + tokenizedFileOptionset[path] + "\"";
					}
					output.AppendLine(String.Format(xmlFormatString, currIndent.ToString(), path, optionset));
				}

				currIndent.Remove(0, indentationDelta.Length);

				output.AppendLine(String.Format("{0}</{1}>", currIndent.ToString(),xmlElementName));
			}

			// Now write out config specific set that 'appends' to previous set.
			foreach (KeyValuePair<string, HashSet<string>> configData in configTokenizedSet)
			{
				HashSet<string> configSpecificSet = new HashSet<string>(configData.Value);
				configSpecificSet.ExceptWith(commonTokenizedSet);
				if (configSpecificSet.Any())
				{
					string ifclause = "${config}=="+configData.Key;
					if (!String.IsNullOrEmpty(condition))
					{
						ifclause = ifclause + " and (" + condition + ")";
					}

					if (configExternalDepPkgSet.ContainsKey(configData.Key))
					{
						HashSet<string> configSpecificDependentSet = new HashSet<string>(configExternalDepPkgSet[configData.Key]);
						configSpecificDependentSet.ExceptWith(commonExternalDepPkgSet);
						if (configSpecificDependentSet.Any())
						{
							// Need to add dependent task to external packages that we are using it's includedirs property.
							foreach (string pkg in configSpecificDependentSet)
							{
								output.AppendLine(string.Format("{0}<dependent name=\"{1}\" if=\"{2}\"/>", currIndent.ToString(), pkg, ifclause));
							}
						}
					}

					// For fileset, we add the append xml element attribute, for property, we prepand ${property.value} before adding extra stuff.
					string line = String.Format("{0}<{1} name=\"{2}\" if=\"{3}\"{4}>",
						currIndent.ToString(), xmlElementName, exportName, ifclause, !commonSetDefined || elementIsProperty ? extraElementAttribute : extraElementAttribute + " append=\"true\"");

					output.AppendLine(line);
					currIndent.Append(indentationDelta);

					if (commonSetDefined && elementIsProperty)
					{
						output.AppendLine(String.Format("{0}${{property.value}}", currIndent.ToString()));
					}

					foreach (string path in configSpecificSet)
					{
						string optionset = "";
						if (tokenizedFileOptionset.ContainsKey(path))
						{
							optionset = "optionset=\"" + tokenizedFileOptionset[path] + "\"";
						}
						output.AppendLine(String.Format(xmlFormatString, currIndent.ToString(), path, optionset));
					}

					currIndent.Remove(0, indentationDelta.Length);
					output.AppendLine(String.Format("{0}</{1}>", currIndent.ToString(), xmlElementName));
				}
			}

			return output.ToString();
		}

		private string BuildInitializeXmlSharedPchExportContent(
			string sharedPchPropertyName,
			string buildgroup,
			string currModuleName,
			string condition,
			Dictionary<string, PrebuiltPublicData> exportPublicData,
			string currentIndentation,
			string indentationDelta)
		{
			StringBuilder output = new StringBuilder();
			StringBuilder currIndent = new StringBuilder(currentIndentation);

			if (!exportPublicData.Any())
				return "";

			// We are not changing this field.
			if (sharedPchPropertyName == "pchfile")
				return "";

			HashSet<string> commonTokenizedSet = null;
			Dictionary<string, HashSet<string>> configTokenizedSet = new Dictionary<string, HashSet<string>>();

			string propertyExportName = String.Format("package.{0}.{1}.{2}", PackageName, currModuleName, sharedPchPropertyName);

			// Build the set that is common to all configs
			foreach (KeyValuePair<string, PrebuiltPublicData> configData in exportPublicData)
			{
				string exportData = null;
				switch (sharedPchPropertyName)
				{
					case "pchheader":
						exportData = configData.Value.mSharedPchData == null ? null : configData.Value.mSharedPchData.PchHeader;
						break;

					case "pchheaderdir":
						exportData = configData.Value.mSharedPchData == null ? null : configData.Value.mSharedPchData.PchHeaderDir;
						break;

					case "pchsource":
						exportData = configData.Value.mSharedPchData == null ? null : configData.Value.mSharedPchData.PchSourceFile;
						break;

					default:
						throw new BuildException(string.Format("INTERNAL ERROR: Not expecting to override '{0}' property in Initialize.xml", sharedPchPropertyName));
				}

				configTokenizedSet.Add(configData.Key, new HashSet<string>());
				Project currProject = configData.Value.mPublicDataProject;
				if (!String.IsNullOrEmpty(exportData))
				{
					string tokenizedPath = null;
					if (sharedPchPropertyName == "pchsource")
					{
						// We want to copy the .cpp file to prebuilt folder.  This file should be empty or just a single #include.
						tokenizedPath = ConvertFullPathToTokenPath(currProject, exportData, configData.Key, configData.Value.mPublicDependencies);
					}
					else
					{
						tokenizedPath = ConvertFullPathToTokenPath(currProject, exportData, configData.Key, configData.Value.mPublicDependencies,
										RedirectSourceIncludePath(exportData));
					}
					configTokenizedSet[configData.Key].Add(tokenizedPath);
				}

				if (commonTokenizedSet == null)
				{
					commonTokenizedSet = new HashSet<string>(configTokenizedSet[configData.Key]);
				}
				else
				{
					commonTokenizedSet.IntersectWith(configTokenizedSet[configData.Key]);
				}
			}

			// Now write out the common set.  Note that we're still writing out the condition for all configs even though this is
			// the common set because of the way we merge different build graphs and we ended up having same module listed twice
			// in build graph with different config list.  So in this function when we extract the info, it does not necessary
			// contains "ALL" configs in the build graph for the same module.
			string conditionAttrib = "";
			string allConfigsCondition = String.Join(" or ", exportPublicData.Select(data => String.Format("${{config}}=={0}", data.Key)).Distinct());
			if (!string.IsNullOrEmpty(condition))
			{
				conditionAttrib = " if=\"(" + allConfigsCondition + ") and (" + condition + ")\"";
			}
			else
			{
				conditionAttrib = " if=\"" + allConfigsCondition + "\"";
			}

			if (commonTokenizedSet != null && commonTokenizedSet.Any())
			{
				// There should be only one value.
				string propertyExportValue = string.Join(Environment.NewLine, commonTokenizedSet).Trim();

				output.AppendLine(String.Format("{0}<property name=\"{1}\" value=\"{2}\"{3} />", currIndent.ToString(), propertyExportName, propertyExportValue, conditionAttrib));
			}

			// Now write out config specific set.
			foreach (KeyValuePair<string, HashSet<string>> configData in configTokenizedSet)
			{
				HashSet<string> configSpecificSet = new HashSet<string>(configData.Value);
				configSpecificSet.ExceptWith(commonTokenizedSet);
				if (configSpecificSet.Any())
				{
					string ifclause = "${config}==" + configData.Key;
					if (!String.IsNullOrEmpty(condition))
					{
						ifclause = ifclause + " and (" + condition + ")";
					}

					// There should be only one value.
					string propertyExportValue = String.Join(Environment.NewLine, configSpecificSet);

					// For fileset, we add the append xml element attribute, for property, we prepand ${property.value} before adding extra stuff.
					output.AppendLine(String.Format("{0}<property name=\"{1}\" value=\"{2}\" if=\"{3}\" />",
						currIndent.ToString(), propertyExportName, propertyExportValue, ifclause));
				}
			}
			return output.ToString();
		}

		private void CreateAndCopyOldInitializeXml()
		{
			Project.Log.Status.WriteLine(LogPrefix + "Copying old Initialize.xml as Initialize-Original.xml and constructing a new initialize.xml file ...");

			string destPkgDir = PrebuiltPackageDir;

			string sourceDir = Path.Combine(PackageDir, "scripts");
			string destDir = Path.Combine(destPkgDir, "scripts");
			string srcInitializeXml = Path.Combine(PackageDir, "scripts", "Initialize.xml");
			string destInitializeXml = Path.Combine(destPkgDir, "scripts", "Initialize.xml");
			if (!File.Exists(srcInitializeXml) && File.Exists(Path.Combine(PackageDir,"Initialize.xml")))
			{
				// Note that if people uses non-standard location for Initialzie.xml and that xml contains include to source another file, 
				// those extra files will not get copied over.
				sourceDir = PackageDir;
				destDir = destPkgDir;
				srcInitializeXml = Path.Combine(PackageDir,"Initialize.xml");
				destInitializeXml = Path.Combine(destPkgDir,"Initialize.xml");
			}

			if (Directory.Exists(sourceDir))
			{
				if (sourceDir != PackageDir)
				{
					CopyDirectory(sourceDir, destDir, excludeSrcFiles: mPackageData.mPrebuiltPackageExcludeFiles);
				}

				if (File.Exists(srcInitializeXml) && !File.Exists(destInitializeXml))
				{
					File.Copy(srcInitializeXml,destInitializeXml);
				}

				// Now that Initialize.xml is copied over, add this to the list of files to exclude to make sure it won't get overwritten by original again
				// beacause we will be writing a new one.
				if (File.Exists(srcInitializeXml) && !mPackageData.mPrebuiltPackageExcludeFiles.Contains(srcInitializeXml))
				{
					mPackageData.mPrebuiltPackageExcludeFiles.Add(srcInitializeXml);
				}
			}
			if (!Directory.Exists(destDir))
			{
				Directory.CreateDirectory(destDir);
			}

			string indentationDelta = "    ";
			StringBuilder currIndentation = new StringBuilder();

			// Create new Initialize.xml content
			StringBuilder newXmlContent = new StringBuilder();
			newXmlContent.AppendLine("<project xmlns=\"schemas/ea/framework3.xsd\">");
			newXmlContent.AppendLine();
			currIndentation.Append(indentationDelta);

			if (File.Exists(destInitializeXml))
			{
				string renamedInitializeXml = Path.Combine(destDir,"Initialize-Original.xml");

				// Rename this file and then re-add with new name.  A new file will be generated in place to original name.
				if (File.Exists(renamedInitializeXml))
				{
					File.Delete(renamedInitializeXml);
				}
				File.Move(destInitializeXml, renamedInitializeXml);

				newXmlContent.AppendLine(String.Format("{0}<include file=\"Initialize-Original.xml\"/>", currIndentation.ToString()));
			}

			// Write out property and fileset that is at package level.  Note that old package level export properties / filesets doesn't
			// support exporting out non-runtime group libraries.

			if (mPackageData.mPreserveProperties.Any())
			{
				foreach (KeyValuePair<string,string> propertyData in mPackageData.mPreserveProperties)
				{
					if (!String.IsNullOrEmpty(propertyData.Value))
					{
						newXmlContent.AppendLine(String.Format("{0}<property name=\"{1}\">{2}</property>", currIndentation.ToString(), propertyData.Key, propertyData.Value));
					}
				}
			}

			foreach (var propertyGrpPublicData in mPackageData.mPackageLevelPublicDataByPropertyAndConfig)
			{
				//
				// Exporting includedirs property
				//
				newXmlContent.Append(
					BuildInitializeXmlExportContent(
						ExportType.IncludeDir,
						"runtime",
						null,
						propertyGrpPublicData.Key,
						propertyGrpPublicData.Value,
						currIndentation.ToString(),
						indentationDelta
					)
				);

				newXmlContent.Append(
					BuildInitializeXmlExportContent(
						ExportType.InternalIncludeDir,
						"runtime",
						null,
						propertyGrpPublicData.Key,
						propertyGrpPublicData.Value,
						currIndentation.ToString(),
						indentationDelta
					)
				);

				//
				// Exporting ddfincludedirs property
				//
				newXmlContent.Append(
					BuildInitializeXmlExportContent(
						ExportType.DDFIncludeDir,
						"runtime",
						null,
						propertyGrpPublicData.Key,
						propertyGrpPublicData.Value,
						currIndentation.ToString(),
						indentationDelta
					)
				);

				//
				// Exporting libs fileset
				//
				newXmlContent.Append(
					BuildInitializeXmlExportContent(
						ExportType.Libs,
						"runtime",
						null,
						propertyGrpPublicData.Key,
						propertyGrpPublicData.Value,
						currIndentation.ToString(),
						indentationDelta
					)
				);

				//
				// Exporting dlls fileset
				//
				newXmlContent.Append(
					BuildInitializeXmlExportContent(
						ExportType.Dlls,
						"runtime",
						null,
						propertyGrpPublicData.Key,
						propertyGrpPublicData.Value,
						currIndentation.ToString(),
						indentationDelta
					)
				);

				//
				// Exporting assemblies fileset
				//
				newXmlContent.Append(
					BuildInitializeXmlExportContent(
						ExportType.Assemblies,
						"runtime",
						null,
						propertyGrpPublicData.Key,
						propertyGrpPublicData.Value,
						currIndentation.ToString(),
						indentationDelta
					)
				);

				//
				// Exporting programs fileset
				//
				newXmlContent.Append(
					BuildInitializeXmlExportContent(
						ExportType.Programs,
						"runtime",
						null,
						propertyGrpPublicData.Key,
						propertyGrpPublicData.Value,
						currIndentation.ToString(),
						indentationDelta
					)
				);

				//
				// Exporting natvis fileset
				//
				if (propertyGrpPublicData.Value.Where(
						configData => 
							FilesetContainFilesInFolderRoots(configData.Value.mNatvisFiles, new string[] { PackageBuildDir, NantBuildRoot })
						).Any()
					)
				{
					newXmlContent.Append(
						BuildInitializeXmlExportContent(
							ExportType.NatvisFiles,
							"runtime",
							null,
							propertyGrpPublicData.Key,
							propertyGrpPublicData.Value,
							currIndentation.ToString(),
							indentationDelta
						)
					);
				}

				//
				// Exporting contentfiles fileset if necessary
				//
				bool needExport = false;
				List<string> outputRoots = new List<string> {PackageBuildDir, NantBuildRoot};
				foreach (var configData in propertyGrpPublicData.Value)
				{
					if (FilesetContainFilesInFolderRoots(configData.Value.mContentFiles, outputRoots))
					{
						needExport = true;
						break;
					}
				}
				if (needExport)
				{
					newXmlContent.Append(
						BuildInitializeXmlExportContent(
							ExportType.ContentFiles,
							"runtime",
							null,
							propertyGrpPublicData.Key,
							propertyGrpPublicData.Value,
							currIndentation.ToString(),
							indentationDelta
						)
					);
				}
			}

			// Export module level override data.
			foreach (string buildgroup in new string[] {"runtime", "tool", "example", "test"})
			{
				Dictionary<Tuple<string, string>, ModuleInfo> buildGroupModules = ExtractModulesForBuildGroup(buildgroup);

				if (!buildGroupModules.Any())
					continue;

				string exportLibPath = PrebuiltConfigLibDirToken;
				string exportBinPath = PrebuiltConfigBinDirToken;

				if (buildgroup != "runtime")
				{
					exportLibPath = Path.Combine(exportLibPath, buildgroup);
					exportBinPath = Path.Combine(exportBinPath, buildgroup);

					// This technically not necessary.  But visually, makes it easier for people to see if we're exporting properties / fileset
					// for anything other than runtime groups.
					newXmlContent.AppendLine();
					newXmlContent.AppendLine(String.Format("{0}<{1}>", currIndentation.ToString(), GetBuildGroupXmlElementName(buildgroup)));
					currIndentation.Append(indentationDelta);
				}

				foreach (KeyValuePair<Tuple<string,string>,ModuleInfo> module in buildGroupModules.OrderBy(m=>m.Key.Item1))
				{
					string currModuleName = module.Key.Item1;
					string propertiesCondition = module.Key.Item2;
					ModuleInfo moduleData = module.Value;

					if (currModuleName == EA.Eaconfig.Modules.Module_UseDependency.PACKAGE_DEPENDENCY_NAME)
					{
						// This package is old style package being loaded as used dependency and doesn't
						// really have module info.  Reset the dummy module name to package name.
						currModuleName = PackageName;
					}

					newXmlContent.AppendLine();
					newXmlContent.AppendLine(String.Format("{0}<!--   Export override for module {1}   -->", currIndentation.ToString(), currModuleName));

					//
					// Exporting includedirs property
					//
					newXmlContent.Append(
						BuildInitializeXmlExportContent(
							ExportType.IncludeDir,
							buildgroup,
							currModuleName,
							propertiesCondition,
							moduleData.mModulePublicDataByConfig,
							currIndentation.ToString(),
							indentationDelta
						)
					);

					newXmlContent.Append(
						BuildInitializeXmlExportContent(
							ExportType.InternalIncludeDir,
							buildgroup,
							currModuleName,
							propertiesCondition,
							moduleData.mModulePublicDataByConfig,
							currIndentation.ToString(),
							indentationDelta
						)
					);

					//
					// Exporting ddfincludedirs property
					//
					newXmlContent.Append(
						BuildInitializeXmlExportContent(
							ExportType.DDFIncludeDir,
							buildgroup,
							currModuleName,
							propertiesCondition,
							moduleData.mModulePublicDataByConfig,
							currIndentation.ToString(),
							indentationDelta
						)
					);

					//
					// Export Shared PCH module data
					//
					if (moduleData.mIsSharedPchModule)
					{
						newXmlContent.Append(
							BuildInitializeXmlSharedPchExportContent(
								"pchheader",
								buildgroup,
								currModuleName,
								propertiesCondition,
								moduleData.mModulePublicDataByConfig,
								currIndentation.ToString(),
								indentationDelta
							)
						);
						newXmlContent.Append(
							BuildInitializeXmlSharedPchExportContent(
								"pchheaderdir",
								buildgroup,
								currModuleName,
								propertiesCondition,
								moduleData.mModulePublicDataByConfig,
								currIndentation.ToString(),
								indentationDelta
							)
						);
						newXmlContent.Append(
							BuildInitializeXmlSharedPchExportContent(
								"pchsource",
								buildgroup,
								currModuleName,
								propertiesCondition,
								moduleData.mModulePublicDataByConfig,
								currIndentation.ToString(),
								indentationDelta
							)
						);
					}

					//
					// Exporting libs fileset
					//
					if (!moduleData.mIsSharedPchModule)
					{
						newXmlContent.Append(
							BuildInitializeXmlExportContent(
								ExportType.Libs,
								buildgroup,
								currModuleName,
								propertiesCondition,
								moduleData.mModulePublicDataByConfig,
								currIndentation.ToString(),
								indentationDelta
							)
						);
					}

					//
					// Exporting dlls fileset
					//
					newXmlContent.Append(
						BuildInitializeXmlExportContent(
							ExportType.Dlls,
							buildgroup,
							currModuleName,
							propertiesCondition,
							moduleData.mModulePublicDataByConfig,
							currIndentation.ToString(),
							indentationDelta
						)
					);

					//
					// Exporting assemblies fileset
					//
					newXmlContent.Append(
						BuildInitializeXmlExportContent(
							ExportType.Assemblies,
							buildgroup,
							currModuleName,
							propertiesCondition,
							moduleData.mModulePublicDataByConfig,
							currIndentation.ToString(),
							indentationDelta
						)
					);

					//
					// Exporting programs fileset
					//
					newXmlContent.Append(
						BuildInitializeXmlExportContent(
							ExportType.Programs,
							buildgroup,
							currModuleName,
							propertiesCondition,
							moduleData.mModulePublicDataByConfig,
							currIndentation.ToString(),
							indentationDelta
						)
					);

					//
					// Exporting natvis files fileset
					//
					if (moduleData.mModulePublicDataByConfig.Where(
							configData =>
								FilesetContainFilesInFolderRoots(configData.Value.mNatvisFiles, new string[] { PackageBuildDir, NantBuildRoot })
							).Any()
						)
					{
						newXmlContent.Append(
							BuildInitializeXmlExportContent(
								ExportType.NatvisFiles,
								buildgroup,
								currModuleName,
								propertiesCondition,
								moduleData.mModulePublicDataByConfig,
								currIndentation.ToString(),
								indentationDelta
							)
						);
					}

					//
					// Exporting content fileset
					//
					bool needExport = false;
					List<string> outputRoots = new List<string> {PackageBuildDir, NantBuildRoot};
					foreach (var configData in moduleData.mModulePublicDataByConfig)
					{
						if (FilesetContainFilesInFolderRoots(configData.Value.mContentFiles, outputRoots))
						{
							needExport = true;
							break;
						}
					}
					if (needExport)
					{
						newXmlContent.Append(
							BuildInitializeXmlExportContent(
								ExportType.ContentFiles,
								buildgroup,
								currModuleName,
								propertiesCondition,
								moduleData.mModulePublicDataByConfig,
								currIndentation.ToString(),
								indentationDelta
							)
						);
					}
				}

				if (buildgroup != "runtime")
				{
					currIndentation.Remove(0, indentationDelta.Length);
					newXmlContent.AppendLine();
					newXmlContent.AppendLine(String.Format("{0}</{1}>", currIndentation.ToString(), GetBuildGroupXmlElementName(buildgroup)));
				}
			}


			newXmlContent.AppendLine();
			newXmlContent.AppendLine("</project>");

			string fileContent = newXmlContent.ToString();

			File.WriteAllText(destInitializeXml, fileContent);
		}

		private void CopyAndUpdateManifestXml()
		{
			Project.Log.Status.WriteLine(LogPrefix + "Copying Manifest.xml ...");

			string sourceManifest = Path.Combine(PackageDir, "Manifest.xml");
			string destManifest = Path.Combine(PrebuiltPackageDir, "Manifest.xml");
			if (!File.Exists(sourceManifest))
			{
				Project.Log.Status.WriteLine(LogPrefix + "    Source package has no Manifest.xml.  Generating a basic one.  We need one to be able to assign a new package version.");

				FileInfo dstInfo = new FileInfo(destManifest);
				if (dstInfo.Exists)
				{
					dstInfo.IsReadOnly = false;
					dstInfo.Delete();
				}
				StringBuilder manifestXmlContent = new StringBuilder();
				manifestXmlContent.AppendLine("<?xml version=\"1.0\"?>");
				manifestXmlContent.AppendLine("<package>");
				manifestXmlContent.AppendLine("    <frameworkVersion>2</frameworkVersion>");
				manifestXmlContent.AppendLine("    <buildable>true</buildable>");
				manifestXmlContent.AppendFormat("    <packageName>{0}</packageName>{1}", PackageName, Environment.NewLine);
				manifestXmlContent.AppendFormat("    <versionName>{0}</versionName>{1}", PackageVersion, Environment.NewLine);
				manifestXmlContent.AppendLine("</package>");
				File.WriteAllText(destManifest, manifestXmlContent.ToString());
			}
			else
			{
				File.Copy(sourceManifest, destManifest);
			}

			Project.Log.Status.WriteLine(LogPrefix + "Update new Manifest.xml's versionName field ...");

			// Need to make sure we remove any read only file attributes.
			FileAttributes attr = File.GetAttributes(destManifest);
			if ((attr & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
			{
				attr = attr & ~FileAttributes.ReadOnly;
				File.SetAttributes(destManifest, attr);
			}

			// Only purpose of doing this instead of using XmlDocument class is so that we can preserve whitespace and when we do
			// file diff comparison of original package, we don't see unnecessary changes.  However, note that doing 
			// regular expression matching isn't full proof, but probably covered our common use cases (ie we don't split
			// versionName in multiple lines and we modify all occurance of <versionName> (unless we can easily detect it is 
			// inside a comment block).
			List<string> fileContent = File.ReadAllLines(destManifest).ToList();
			List<string> linesUpdated = new List<string>();
			int closePackageIndex = -1;
			// use "for" instead of "foreach" because we want to find the line index 
			// for </package>.  We will need it if we can't find versionName element.
			for (int idx = 0; idx < fileContent.Count; ++idx)
			{
				System.Text.RegularExpressions.Match mt = System.Text.RegularExpressions.Regex.Match(fileContent[idx], "<versionName>.+</versionName>");
				if (mt.Success)
				{
					string trimmedLine = fileContent[idx].Trim();
					if (!trimmedLine.StartsWith("<!--") && !trimmedLine.EndsWith("-->"))
					{
						linesUpdated.Add(fileContent[idx]);
						fileContent[idx] = fileContent[idx].Replace(mt.Value, "<versionName>" + PrebuiltPackageVersion + "</versionName>");
					}
				}
				if (fileContent[idx].Contains("</package>"))
				{
					closePackageIndex = idx;
				}
			}
			if (!linesUpdated.Any())
			{
				if (closePackageIndex == -1)
				{
					throw new BuildException(String.Format("The original Manifest.xml for package '{0}' might have been malformed.  Unable to find the </package> close tag!", PackageName));
				}
				else
				{
					// If versionName is not in original manifest.xml, we need to add it.
					// We insert it right before the "package" close element.
					fileContent.Insert(closePackageIndex, "    <versionName>" + PrebuiltPackageVersion + "</versionName>");
				}
			}
			else if (linesUpdated.Count > 1)
			{
				// More than one lines are updated.  Show user these lines are updated.
				StringBuilder msg = new StringBuilder();
				msg.AppendFormat("These lines from original Manifest.xml has been updated to use <versionName>{0}</versionName>", PrebuiltPackageVersion);
				msg.AppendLine();
				msg.AppendLine(string.Join(Environment.NewLine, linesUpdated));
				Project.Log.Status.WriteLine(LogPrefix + msg.ToString());
			}
			// Write out this file.
			File.WriteAllLines(destManifest, fileContent);
		}

		private void CopyBuildBinaries()
		{
			Project.Log.Status.WriteLine(LogPrefix + "Copying built binaries (if any) ...");

			foreach (var prop in mPackageData.mModulesDataByProperty)
			{
				foreach (var mod in prop.Value)
				{
					ModuleInfo module = mod.Value;
					foreach (KeyValuePair<string, PrebuiltPublicData> exportPublicDataByConfig in module.mModulePublicDataByConfig)
					{
						CopyFilesToLibDir(module, exportPublicDataByConfig);
						CopyFilesToBinDir(module, exportPublicDataByConfig);
					}
				}
			}

			// Some packages are still using old style build script and specified package level export properties and fileset only.
			// In those cases, we need to check package level fileset export as well.
			foreach (KeyValuePair<string, Dictionary<string,PrebuiltPublicData>> packageLevelExportByConfig in mPackageData.mPackageLevelPublicDataByPropertyAndConfig)
			{
				foreach (var packageLevelExport in packageLevelExportByConfig.Value)
				{
					CopyFilesToLibDir(null, packageLevelExport);
					CopyFilesToBinDir(null, packageLevelExport);
				}
			}
		}

		private void CopyFilesToLibDir(ModuleInfo module, KeyValuePair<string, PrebuiltPublicData> exportPublicDataByConfig)
		{
			// We don't cache the shared pch module's build output.  The pch binary must be created locally because
			// the pch binary itself contains full path information to the original header file location and we cannot
			// remove those path information from the pch file itself.  It is part of the pch design (both pc and clang).
			if (module != null && module.mIsSharedPchModule)
			{
				return;
			}

			List<string> totalLibList = exportPublicDataByConfig.Value.mLibs;

			if (totalLibList.Count() == 0)
			{
				return;
			}

			// On platforms like NX, the dll stub lib (.nrs) were placed side by side the dll (.nro) file itself.  So when we do the file copy, we need to check and
			// see if this lib file actually came from the 'bin' folder instead.  If it is, we need to preserve that and copy it to the bin folder instead.
			// First, let's retrieve the original "bin" folder path for this module so that we can test for it later in the file loop below.
			string realOriginalBinDir = exportPublicDataByConfig.Value.mPublicDataProject.Properties.GetPropertyOrDefault("package.configbindir", "");
			if (module != null && !String.IsNullOrEmpty(module.mModuleName))
			{
				realOriginalBinDir = BuildFunctions.GetModuleOutputDirEx(exportPublicDataByConfig.Value.mPublicDataProject, "bin", PackageName, module.mModuleName, module.mModuleBuildGroup);
			}

			string outputLibDir = PrebuiltConfigLibDir.Replace("${config}", exportPublicDataByConfig.Key);
			string outputBinDir = PrebuiltConfigBinDir.Replace("${config}", exportPublicDataByConfig.Key);
			if (module != null && module.mModuleBuildGroup != "runtime")
			{
				outputLibDir = Path.Combine(outputLibDir, module.mModuleBuildGroup);
				outputBinDir = Path.Combine(outputBinDir, module.mModuleBuildGroup);
			}

			foreach (string file in totalLibList)
			{
				if (!File.Exists(file))
				{
					// Note that Visual Studio no longer generate the import lib for managed c++ module.
					if (module != null && !module.mModuleBuildTypeByConfig[exportPublicDataByConfig.Key].IsManaged)
					{
						Project.Log.Warning.WriteLine("WARNING: You may not have built the binaries for required config.  Unable to find built file: {0}.  For now not triggering an error in case the package is exporting modules that actually didn't get used in your build.", file);
					}
					continue;
				}

				if (exportPublicDataByConfig.Value.mDlls.Contains(file))
				{
					// We will copy this file when we copy the dll fileset.  No need to copy this to the wrong directory (Dll should stay in bin folder).
					// We will run into this situation when we are dealing with unix config where we don't have dll stub libs.  We just link directly
					// against the build dll (.so file).
					continue;
				}

				// We have platforms that need to have the dll stub libs to be placed side by side the dll.  So we need to test if the source file is
				// actually located in the "bin" folder and we need to copy this lib to the bin folder instead!
				string destRootDir = outputLibDir;
				if (!String.IsNullOrEmpty(realOriginalBinDir) && file.StartsWith(realOriginalBinDir))
				{
					destRootDir = outputBinDir;
				}

				string fileName = Path.GetFileName(file);
				string destFileName = Path.Combine(destRootDir, fileName);
				FileInfo destFileInfo = new FileInfo(destFileName);
				if (!destFileInfo.Exists)
				{
					destFileInfo.Directory.Create();  // If directory already exists, it will do nothing.
					// The above create directory is needed because File.Copy() will fail if destination directory doesn't exists.
					File.Copy(file, destFileInfo.FullName);
				}

				if (!PrebuiltSkipPdb)
				{
					string pdbFileName = Path.GetFileNameWithoutExtension(fileName) + ".pdb";
					string destPdbFileName = Path.Combine(destRootDir, pdbFileName);
					string srcPdbFile = Path.Combine(Path.GetDirectoryName(file), pdbFileName);
					if (File.Exists(srcPdbFile) && !File.Exists(destPdbFileName))
					{
						File.Copy(srcPdbFile, destPdbFileName);
					}
				}
			}
		}

		private void CopyFilesToBinDir(ModuleInfo module, KeyValuePair<string, PrebuiltPublicData> exportPublicDataByConfig)
		{
			List<string> totalBinList = new List<string>();
			totalBinList.AddRange(exportPublicDataByConfig.Value.mDlls);
			totalBinList.AddRange(exportPublicDataByConfig.Value.mAssemblies);
			totalBinList.AddRange(exportPublicDataByConfig.Value.mPrograms);

			if (totalBinList.Count() == 0)
			{
				return;
			}

			string outputBinDir = PrebuiltConfigBinDir.Replace("${config}", exportPublicDataByConfig.Key);
			if (module != null && module.mModuleBuildGroup != "runtime")
			{
				outputBinDir = Path.Combine(outputBinDir, module.mModuleBuildGroup);
			}

			if (!Directory.Exists(outputBinDir))
			{
				Directory.CreateDirectory(outputBinDir);
			}

			foreach (string file in totalBinList)
			{
				if (!File.Exists(file))
				{
					Project.Log.Status.WriteLine("WARNING: You may not have built the binaries for required config.  Unable to find built file: {0}.  For now not triggering an error in case this package is exporting modules that your build isn't linking against.", file);
					continue;
				}

				string fileName = Path.GetFileName(file);
				string destFileName = Path.Combine(outputBinDir, fileName);
				if (!File.Exists(destFileName))
				{
					File.Copy(file, destFileName);
				}

				if (!PrebuiltSkipPdb)
				{
					string pdbFileName = Path.GetFileNameWithoutExtension(fileName) + ".pdb";
					string destPdbFileName = Path.Combine(outputBinDir, pdbFileName);
					string srcPdbFile = Path.Combine(Path.GetDirectoryName(file), pdbFileName);
					if (File.Exists(srcPdbFile) && !File.Exists(destPdbFileName))
					{
						File.Copy(srcPdbFile, destPdbFileName);
					}
				}

				// DotNet assemblies / programs can have important *.config file that goes with it.  We may need to copy that
				// as well.
				string configFileName = Path.GetFileName(fileName) + ".config";
				string destConfigFileName = Path.Combine(outputBinDir, configFileName);
				string srcConfigFile = Path.Combine(Path.GetDirectoryName(file), configFileName);
				if (File.Exists(srcConfigFile) && !File.Exists(destConfigFileName))
				{
					File.Copy(srcConfigFile, destConfigFileName);
				}
			}
		}

		private void CopyInPackageExternalBinaries()
		{
			Project.Log.Status.WriteLine(LogPrefix + "Copying any dlls-external and libs-external files that is actually inside the package ...");
			FileSet combinedFileset = new FileSet();
			foreach (var prop in mPackageData.mModulesDataByProperty)
			{
				foreach (var mod in prop.Value)
				{
					ModuleInfo module = mod.Value;
					foreach (KeyValuePair<string, PrebuiltPublicData> exportPublicDataByConfig in module.mModulePublicDataByConfig)
					{
						combinedFileset.Append(exportPublicDataByConfig.Value.DllsExternal);
						combinedFileset.Append(exportPublicDataByConfig.Value.LibsExternal);
					}
				}
			}
			if (combinedFileset.FileItems.Any())
			{
				
				foreach (FileItem itm in combinedFileset.FileItems)
				{
					if (itm.FullPath.StartsWith(PackageDir, ignoreCase: true, culture: System.Globalization.CultureInfo.InvariantCulture))
					{
						string relativePath = PathUtil.RelativePath(itm.FullPath, PackageDir);
						string dstFile = Path.Combine(PrebuiltPackageDir, relativePath);
						string dstFileDir = Path.GetDirectoryName(dstFile);
						if (!Directory.Exists(dstFileDir))
						{
							Directory.CreateDirectory(dstFileDir);
						}
						if (File.Exists(dstFile))
						{
							File.SetAttributes(dstFile, FileAttributes.Normal);
						}
						File.Copy(itm.FullPath, dstFile, true);
					}
				}
			}
		}

		private void CopyIncludes()
		{
			Project.Log.Status.WriteLine(LogPrefix + "Copying include directories - .h, .hpp, .inl, and .ddf only (if any) ...");

			HashSet<string> dirCopied = new HashSet<string>();

			// Collect all module specific public data export
			List< Dictionary<string, PrebuiltPublicData> > allExportByConfigGroups = new List<Dictionary<string, PrebuiltPublicData>>();
			foreach (var prop in mPackageData.mModulesDataByProperty.Select(p => p.Value))
			{
				allExportByConfigGroups.AddRange(prop.Select(m => m.Value.mModulePublicDataByConfig));
			}

			// Add package level public data export.
			allExportByConfigGroups.AddRange(mPackageData.mPackageLevelPublicDataByPropertyAndConfig.Select( p => p.Value ));

			bool inFrostbiteConfig = !String.IsNullOrEmpty(Project.Properties.GetPropertyOrDefault("frostbite.target",null));

			// Include files with only the following extensions (typically source files generated by prebuild tasks.  We should only need to export the .h files.)
			HashSet<string> incList = new HashSet<string>();
			incList.Add(".h");
			incList.Add(".H");
			incList.Add(".hpp");	// Shouldn't really use this, but possible
			incList.Add(".HPP");
			incList.Add(".inl");
			incList.Add(".INL");
			if (inFrostbiteConfig)
			{
				// Frostbite need to have ddf files distributed as well.
				incList.Add(".ddf");
			}

			// ANT's code gen related file extensions.
			HashSet<string> antCodeGenIncludes = new HashSet<string>();
			antCodeGenIncludes.Add(".edf");
			antCodeGenIncludes.Add(".bdd");
			antCodeGenIncludes.Add(".adf");

			// The shared pch dummy cpp files.
			HashSet<string> sharedPchCppFiles = new HashSet<string>();

			foreach (Dictionary<string, PrebuiltPublicData> exportByConfigGroup in allExportByConfigGroups)
			{
				foreach (KeyValuePair<string, PrebuiltPublicData> exportPublicDataByConfig in exportByConfigGroup)
				{
					string currConfig = exportPublicDataByConfig.Key;
					List<string> allIncludeDirs = new List<string>();

					if (inFrostbiteConfig
					 && (exportPublicDataByConfig.Value.mIncludeDirs.Where(incdir => incdir.Contains("Legacy")).Any()
					 ||  exportPublicDataByConfig.Value.mDdfIncludeDirs.Where(incdir => incdir.Contains("Legacy")).Any())
					   )
					{
						// This is Frostbite specific.  If the frostbite pacakge has a Legacy folder to support
						// backward compatibility, then the real header files can be all over the entire package and we
						// need to add the package's root folder as include dirs to get all header files in the package.
						allIncludeDirs.Add(PackageDir);
					}
					allIncludeDirs.AddRange(exportPublicDataByConfig.Value.mIncludeDirs);
					allIncludeDirs.AddRange(exportPublicDataByConfig.Value.mInternalIncludeDirs);
					allIncludeDirs.AddRange(exportPublicDataByConfig.Value.mDdfIncludeDirs);
					if (allIncludeDirs.Any(p=>p.Contains(Path.Combine(PackageDir,".."))))
					{
						Log.Warning.WriteLine("WARNING: Package {0} contains export includedir for config {1} that is relative path out of the package itself.  Any include files that is relative out of it's own package will not be copied to standalone package and this package may not work!", PackageName, currConfig);
						List<string> badIncludes = allIncludeDirs.Where(p => p.Contains(Path.Combine(PackageDir, ".."))).ToList();
						foreach (string badInc in badIncludes)
						{
							allIncludeDirs.Remove(badInc);
						}
					}

					if (exportPublicDataByConfig.Value.mSharedPchData != null)
					{
						if (!String.IsNullOrEmpty(exportPublicDataByConfig.Value.mSharedPchData.PchHeaderDir) && !allIncludeDirs.Contains(exportPublicDataByConfig.Value.mSharedPchData.PchHeaderDir))
							allIncludeDirs.Add(exportPublicDataByConfig.Value.mSharedPchData.PchHeaderDir);
						if (!String.IsNullOrEmpty(exportPublicDataByConfig.Value.mSharedPchData.PchSourceFile))
						{
							sharedPchCppFiles.Add(exportPublicDataByConfig.Value.mSharedPchData.PchSourceFile);
						}
					}
					foreach (string incDir in allIncludeDirs)
					{
						if (dirCopied.Contains(incDir))
							continue;

						if (incDir.StartsWith(PackageBuildDir))
						{
							string destDir = PrebuiltGeneratedFilesDir + incDir.Substring(PackageBuildDir.Length);
							CopyDirectory(incDir, destDir, includeOnlyExtensions: incList, excludeSrcFiles: mPackageData.mPrebuiltPackageExcludeFiles);
						}
						else if(incDir.StartsWith(PackageGenDir))
						{
							string destDir = PrebuiltGeneratedFilesDir + incDir.Substring(PackageGenDir.Length);
							CopyDirectory(incDir, destDir, includeOnlyExtensions: incList, excludeSrcFiles: mPackageData.mPrebuiltPackageExcludeFiles);
						}
						else if (incDir.StartsWith(PackageDir))
						{
							string destDir = PrebuiltPackageDir + incDir.Substring(PackageDir.Length);
							if (!RedirectSourceIncludePath(incDir))
							{
								CopyDirectory(incDir, destDir, includeOnlyExtensions: incList, excludeSrcFiles: mPackageData.mPrebuiltPackageExcludeFiles);
							}

							// ANT's adf code gen files don't need to be re-directed.  ANT's packages used to be standalone packages and doesn't 
							// depend on include path TnT\Code.  So they are safe to just copy over to prebuilt package folder.
							// Also, ANT's custom task also create a package.[PackageName].adfexportdir property to point to ${package.[PackageName].dir}\include
							// So we need those adf copied.
							CopyDirectory(incDir, destDir, includeOnlyExtensions: antCodeGenIncludes, excludeSrcFiles: mPackageData.mPrebuiltPackageExcludeFiles);
						}

						if (Directory.Exists(incDir))
						{
							// For performance, cache all sub-directories full path so that we don't see to process them if we see them again.
							foreach (string dir in Directory.GetDirectories(incDir, "*", SearchOption.AllDirectories))
							{
								dirCopied.Add(dir);
							}
						}

						dirCopied.Add(incDir);
					}
				}
			}

			if (sharedPchCppFiles.Any())
			{
				foreach (string sharedPchCpp in sharedPchCppFiles)
				{
					string outputDirRoot = PrebuiltPackageDir;
					string relativePath = null;
					if (sharedPchCpp.StartsWith(PackageDir))
						relativePath = sharedPchCpp.Substring(PackageDir.Length);
					else
						relativePath = Path.GetFileName(sharedPchCpp);
					if (relativePath.StartsWith("\\") || relativePath.StartsWith("/"))
					{
						relativePath = relativePath.Substring(1);
					}
					string destFile = Path.Combine(outputDirRoot, relativePath);

					if (File.Exists(destFile))
					{
						File.SetAttributes(destFile, FileAttributes.Normal);
					}
					string destFileDir = Path.GetDirectoryName(destFile);
					if (!Directory.Exists(destFileDir))
					{
						Directory.CreateDirectory(destFileDir);
					}
					File.Copy(sharedPchCpp, destFile, true);
				}
			}
		}

		private void CopyNatvisFiles()
		{
			Project.Log.Status.WriteLine(LogPrefix + "Copying natvis files (if any) ...");

			HashSet<string> filesHandled = new HashSet<string>();

			foreach (var prop in mPackageData.mModulesDataByProperty)
			{
				foreach (var module in prop.Value)
				{
					foreach (KeyValuePair<string, PrebuiltPublicData> exportPublicDataByConfig in module.Value.mModulePublicDataByConfig)
					{
						string currConfig = exportPublicDataByConfig.Key;
						foreach (string natvisFile in exportPublicDataByConfig.Value.mNatvisFiles)
						{
							if (filesHandled.Contains(natvisFile))
								continue;

							string destFile = null;
							if (natvisFile.StartsWith(PackageBuildDir))
							{
								string outputDirRoot = PrebuiltGeneratedFilesDir;
								string relativePath = natvisFile.Substring(PackageBuildDir.Length);
								if (relativePath.StartsWith("\\") || relativePath.StartsWith("/"))
								{
									relativePath = relativePath.Substring(1);
								}
								destFile = Path.Combine(outputDirRoot, relativePath);
							}
							else if (natvisFile.StartsWith(NantBuildRoot))
							{
								string outputDirRoot = PrebuiltGeneratedFilesDir;
								string relativePath = natvisFile.Substring(NantBuildRoot.Length);
								if (relativePath.StartsWith("\\") || relativePath.StartsWith("/"))
								{
									relativePath = relativePath.Substring(1);
								}
								destFile = Path.Combine(outputDirRoot, relativePath);
							}
							else
							{
								string outputDirRoot = PrebuiltPackageDir;
								string relativePath = null;
								if (natvisFile.StartsWith(PackageDir))
									relativePath = natvisFile.Substring(PackageDir.Length);
								else
									relativePath = Path.GetFileName(natvisFile);
								if (relativePath.StartsWith("\\") || relativePath.StartsWith("/"))
								{
									relativePath = relativePath.Substring(1);
								}
								destFile = Path.Combine(outputDirRoot, relativePath);
							}

							if (File.Exists(destFile))
							{
								File.SetAttributes(destFile, FileAttributes.Normal);
							}
							string destFileDir = Path.GetDirectoryName(destFile);
							if (!Directory.Exists(destFileDir))
							{
								Directory.CreateDirectory(destFileDir);
							}
							File.Copy(natvisFile, destFile, true);

							filesHandled.Add(natvisFile);
						}
					}
				}
			}
		}

		private void CopyContentFiles()
		{
			Project.Log.Status.WriteLine(LogPrefix + "Copying content files (if any) ...");

			HashSet<string> filesHandled = new HashSet<string>();

			foreach (var prop in mPackageData.mModulesDataByProperty)
			{
				foreach (var module in prop.Value)
				{ 
					foreach (KeyValuePair<string, PrebuiltPublicData> exportPublicDataByConfig in module.Value.mModulePublicDataByConfig)
					{
						string currConfig = exportPublicDataByConfig.Key;
						foreach (string contentFile in exportPublicDataByConfig.Value.mContentFiles)
						{
							if (filesHandled.Contains(contentFile))
								continue;

							string destFile = null;

							if (contentFile.StartsWith(PackageBuildDir) || contentFile.StartsWith(PackageGenDir) || contentFile.StartsWith(NantBuildRoot))
							{
								// Copy these files directly to package's base folder so that we don't need to setup basedir in the fileset.
								string outputDirRoot = PrebuiltPackageDir;
								string flattenedPath = Path.GetFileName(contentFile);
								destFile = Path.Combine(outputDirRoot, flattenedPath);
							}
							else if (contentFile.StartsWith(PackageDir))
							{
								destFile = PrebuiltPackageDir + contentFile.Substring(PackageDir.Length);
							}

							if (destFile != null)
							{
								if (File.Exists(destFile))
								{
									File.SetAttributes(destFile, FileAttributes.Normal);
								}
								string destFileDir = Path.GetDirectoryName(destFile);
								if (!Directory.Exists(destFileDir))
								{
									Directory.CreateDirectory(destFileDir);
								}
								File.Copy(contentFile, destFile, true);
							}

							filesHandled.Add(contentFile);
						}
					}
				}
			}
		}

		private void CopyExplicitIncludeFiles()
		{
			Project.Log.Status.WriteLine(LogPrefix + "Copying user specified files (if any) ...");

			HashSet<string> filesHandled = new HashSet<string>();

			foreach (string file in mPackageData.mPrebuiltPackageIncludeFiles)
			{
				if (filesHandled.Contains(file))
					continue;

				if (!file.StartsWith(PackageDir) && !file.StartsWith(PackageBuildDir) && (String.IsNullOrEmpty(ExternalIncludeSourceBaseDir) || !file.StartsWith(ExternalIncludeSourceBaseDir)))
				{
					string message = string.Format("ERROR: The package.{0}.PrebuiltIncludeSourceFiles fileset can only be used to specify files in the package tree or build artifacts, not outside the package.  This file:{1}{2}{1}appears to be outside package tree or build directory:{1}    {3}{1}    {4}{1}",
						PackageName, Environment.NewLine, file, PackageDir, PackageBuildDir);
					message = string.Format("{0}If it is really correct to include source files from location external to the package, please provide property \"package.{1}.PrebuiltExternalIncludeSourceBaseDir\"!{2}", message, PackageName, Environment.NewLine);
					throw new BuildException(message);
				}
				else
				{
					string destFile = PrebuiltPackageDir + file.Substring(PackageDir.Length);
					if (file.StartsWith(PackageBuildDir))
					{
						destFile = PrebuiltGeneratedFilesDir + file.Substring(PackageBuildDir.Length);
					}
					if (file.StartsWith(PackageGenDir))
					{
						destFile = PrebuiltGeneratedFilesDir + file.Substring(PackageGenDir.Length);
					}
					if (!String.IsNullOrEmpty(ExternalIncludeSourceBaseDir))
					{
						if (file.StartsWith(ExternalIncludeSourceBaseDir))
						{
							// We put all external files under package root.  It's up to package owner to deal with folder structure and
							// specifying the correct basedir info.
							destFile = PrebuiltPackageDir + file.Substring(ExternalIncludeSourceBaseDir.Length);
						}
					}
					if (File.Exists(destFile))
					{
						File.SetAttributes(destFile, FileAttributes.Normal);
					}
					string destFileDir = Path.GetDirectoryName(destFile);
					if (!Directory.Exists(destFileDir))
					{
						Directory.CreateDirectory(destFileDir);
					}
					File.Copy(file, destFile, true);
				}

				filesHandled.Add(file);
			}
		}

		private void CopyDirectory(string sourceDir, string destDir, HashSet<string> includeOnlyExtensions = null, HashSet<string> excludeSrcFiles = null)
		{
			if (!Directory.Exists(sourceDir))
				return;

			string normalizedSourceDir = PathString.MakeNormalized(sourceDir, PathString.PathParam.NormalizeOnly).Path;
			if (!normalizedSourceDir.EndsWith(new string(new char[] { Path.DirectorySeparatorChar, '\n' })))
			{
				normalizedSourceDir += Path.DirectorySeparatorChar;
			}

			foreach (string file in Directory.GetFiles(normalizedSourceDir, "*", SearchOption.AllDirectories))
			{
				string relativePath = file.Substring(normalizedSourceDir.Length);
				string baseDir = Path.GetDirectoryName(relativePath);
				string filename = Path.GetFileName(relativePath);
				string fileext = Path.GetExtension(relativePath);
				string destFileDir = Path.Combine(destDir, baseDir);
				string destFilePath = Path.Combine(destFileDir, filename);

				if (includeOnlyExtensions != null && !includeOnlyExtensions.Contains(fileext))
				{
					// We're including only file with indicated extensions.
					continue;
				}

				if (excludeSrcFiles != null && excludeSrcFiles.Contains(file))
				{
					// We're excluding copy of this source file.
					continue;
				}

				if (!Directory.Exists(destFileDir))
				{
					Directory.CreateDirectory(destFileDir);
				}

				if (File.Exists(destFilePath))
				{
					File.SetAttributes(destFilePath, FileAttributes.Normal);
				}
				File.Copy(file, destFilePath, true);
			}
		}
	}
}
