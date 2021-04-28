// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Util;

using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Structured;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig
{
    public static partial class AndroidBuildUtil
	{
		internal enum OutputDirType 
		{ 
			Classes,
			Bin, 
			Lib 
		};

		internal static IEnumerable<Module_Native> GetTopAndroidModules(Project project)
		{
			return project.BuildGraph().TopModules.Where(m => m.Configuration.System == "android").Where(mod => mod is Module_Native).Cast<Module_Native>();
		}

		internal static IEnumerable<Module_Native> GetAllAndroidModules(Project project)
		{
			return project.BuildGraph().SortedActiveModules.Where(m => m.Configuration.System == "android").Where(mod => mod is Module_Native).Cast<Module_Native>();
		}

		internal static string GetAndroidSdkDir(IModule module)
		{
			TaskUtil.Dependent(module.Package.Project, "AndroidSDK");
			return PathNormalizer.Normalize(module.Package.Project.GetPropertyValue("package.AndroidSDK.appdir"), true);
		}

		internal static string GetAndroidNdkDir(IModule module)
		{
			TaskUtil.Dependent(module.Package.Project, "AndroidNDK");
			return PathNormalizer.Normalize(module.Package.Project.GetPropertyValue("package.AndroidNDK.appdir"), true);
		}

		internal static string GetAndroidNdkBinDir(IModule module)
		{
			TaskUtil.Dependent(module.Package.Project, "AndroidNDK");
			return PathNormalizer.Normalize(module.Package.Project.GetPropertyValue("package.AndroidNDK.bindir"), true);
		}

		internal static string GetAndroidBuildToolsDir(IModule module)
		{
			TaskUtil.Dependent(module.Package.Project, "AndroidSDK");
			return PathNormalizer.Normalize(module.Package.Project.GetPropertyValue("package.AndroidSDK.buildtooldir"), true);
		}

        internal static string GetJavaHome(IModule module)
		{
			TaskUtil.Dependent(module.Package.Project, "jdk");
			return PathNormalizer.Normalize(module.Package.Project.GetPropertyValue("package.jdk.home"), true);
		}

		internal static string GetJavaAppdir(IModule module)
		{
			TaskUtil.Dependent(module.Package.Project, "jdk");
			return PathNormalizer.Normalize(module.Package.Project.Properties["package.jdk.appdir"], true);
		}

		private static string s_gradleRoot;

		internal static string GetGradleRoot(IModule module)
		{
			if (s_gradleRoot == null)
			{
				string gradleRoot = module.Package.Project.Properties["package.GradleWrapper.appdir"];
				if (gradleRoot == null)
				{
					TaskUtil.Dependent(module.Package.Project, "GradleWrapper");
					gradleRoot = module.Package.Project.Properties.GetPropertyOrFail("package.GradleWrapper.appdir");
				}
				s_gradleRoot = PathNormalizer.Normalize(gradleRoot, true);
			}
			return s_gradleRoot;
		}

		internal static bool WrapPrint(IModule module)
		{
			return module.Package.Project.Properties.GetBooleanPropertyOrDefault("android.wrap-print", true);
		}

		internal static bool IsVS2015OrNewer(IModule module)
		{
			return SetConfigVisualStudioVersion.Execute(module.Package.Project).StrCompareVersions("14.0") >= 0;
		}

		internal static string GetSafeApplicationPackageName(IModule module)
		{
			return module.PropGroupValue("packagename", module.Name).Replace('-', 'X').Replace('.', '_');
		}

		internal static string GetApplicationPackageName(IModule module)
		{
			return module.PropGroupValue("packagename", module.Name).Replace('.', '_');
		}

		internal static string GetApkName(IModule module)
		{
			string packageName = module.PropGroupValue("packagename", module.Name);
			string androidApkSuffix = Path.GetFileNameWithoutExtension(AndroidBuildUtil.GetApkSuffix(module));
			return packageName + androidApkSuffix;
		}

		internal static string GetApkSuffix(IModule module)
		{
			return module.Package.Project.GetPropertyOrDefault("android-apk-suffix", "-" + module.Configuration.Name + ".apk");
		}

		internal static string GetApkOutputDir(IModule module)
		{
			string outputDir = PathNormalizer.Normalize(module.PropGroupValue("outputdir"));
			string binPath = Path.Combine(module.Package.Project.Properties["package.configbindir"], module.PropGroupValue("packagename", module.Name));
			outputDir = String.IsNullOrWhiteSpace(outputDir) ? binPath : outputDir;
			return outputDir;
		}

		internal static PathString GetAndroidIntermediateDir(IModule module)
		{
			return PathString.MakeNormalized(Path.Combine
			(
				module.Package.Project.Properties["package.configbuilddir"],
				module.Package.Project.Properties["eaconfig." + module.BuildGroup + ".outputfolder"].TrimLeftSlash(),
				module.Name
			));
		}

		internal static void OnFileUpdate(Log log, string fileName, string logPrefix, string fileType, bool updating)
		{
			string updatePrefix = updating ? "    Updating" : "NOT Updating";
			fileType = !String.IsNullOrWhiteSpace(fileType) ? " " + fileType : String.Empty;
			log.Status.WriteLine("{0}{1}{2} file '{3}'", logPrefix, updatePrefix, fileType, fileName);
		}

		public static string NormalizePath(string path)
		{
			return path.Replace('\\', '/');
		}

		// android has lots of features that require a single file, so this helper function finds filesets
		// and verifys they only contains a single file (and then returns normalized abs path)
		internal static string GetUniqueAndroidFilesetFile(IModule module, string fileSetName, bool failOnMissing = false)
		{
			// search for a matching file set
			FileSet fileSet = module.GetPlatformModuleFileSet(fileSetName);
			if (fileSet != null)
			{
				FileItemList fileSetItems = fileSet.FileItems;
				if (!fileSetItems.Any())
				{
					throw new BuildException(String.Format("Android fileset '{0}' is empty. If you were expecting this file to be auto-generated do not declare this fileset.", fileSetName));
				}
				else if (fileSetItems.Count() > 1)
				{
					throw new BuildException(String.Format("Android fileset '{0}' contains more than one file: {1}{2}{1}", fileSetName, Environment.NewLine, FileSetFunctions.FileSetToString(module.Package.Project, fileSet.Name, Environment.NewLine)));
				}
				return PathNormalizer.Normalize(fileSetItems.First().Path.Path, getFullPath: true);
			}
			else if (failOnMissing)
			{
				throw new BuildException(String.Format("Critical Android file missing! Define fileset {0} or {1}.", module.PropGroupName(fileSetName), module.GetPlatformModulePropGroupName(fileSetName)));
			}
			else
			{
				return null;
			}
		}

		internal static void CreateUniqueAndroidFilesetFile(IModule module, string fileSetNamePropertyName, string baseDir, string fileInclude)
		{
			FileSet fileset = new FileSet(module.Package.Project);
			fileset.BaseDirectory = baseDir;
			fileset.Include(fileInclude);
			module.Package.Project.NamedFileSets[module.PropGroupName(fileSetNamePropertyName + ".android")] = fileset;
		}

		internal static PathString MapDependentPackageOutputDir(PathString dir, Module module, string packageName, OutputDirType type)
		{
			var outputDir = dir;

			if (outputDir.IsNullOrEmpty())
			{
				return outputDir;
			}

			var typeStr = "bin";
			PathString rootdir = null;
			switch (type)
			{
				case OutputDirType.Classes:
					typeStr = "bin";
					rootdir = PathString.MakeCombinedAndNormalized(module.Package.Project.GetPropertyValue("package."+packageName + ".builddir"), module.Package.Project.GetPropertyValue("config") +"/bin");
					break;
				case OutputDirType.Bin:
					typeStr = "bin";
					rootdir = PathString.MakeCombinedAndNormalized(module.Package.Project.GetPropertyValue("package." + packageName + ".builddir"), module.Package.Project.GetPropertyValue("config") + "/bin");
					break;
				case OutputDirType.Lib:
					typeStr = "lib";
					break;
				default:
					typeStr = "bin";
					rootdir = PathString.MakeCombinedAndNormalized(module.Package.Project.GetPropertyValue("package." + packageName + ".builddir"), module.Package.Project.GetPropertyValue("config") + "/bin");
					break;
			}

			if (dir.Path.StartsWith(rootdir.Path))
			{
				var mapoptionset = ModulePath.Private.ModuleOutputMapping(module.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString());
				var outputRootDir = MapDependentBuildOutputs.MapDependentOutputDir(module.Package.Project, rootdir, mapoptionset, packageName, typeStr);
				outputDir = PathString.MakeNormalized(dir.Path.Replace(rootdir.Path, outputRootDir.Path));
			}
			return outputDir;
		}

        internal static List<PathString> GetAdditonalResourceDirs(Module module)
        {
            List<PathString> additonalResourceDirs = new List<PathString>();
            DependencyDeclaration dependencyDeclaration = new DependencyDeclaration(module.GetPlatformModuleProperty("assetdependencies"));
            foreach (DependencyDeclaration.Package packageDependency in dependencyDeclaration.Packages)
            {
                // load dependencey pulbic data
                if (packageDependency.Name != module.Package.Name)
                {
                    TaskUtil.Dependent(module.Package.Project, packageDependency.Name, Project.TargetStyleType.Use);
                }

                // compute all modules that are covered by dependency declaration
                HashSet<string> moduleNames = new HashSet<string>();
                if (packageDependency.FullDependency)
                {
                    moduleNames.UnionWith
                    (
                        (module.Package.Project.GetPropertyValue("package." + packageDependency.Name + ".runtime.buildmodules") ??
                        module.Package.Project.GetPropertyValue("package." + packageDependency.Name + ".buildmodules") ??
                        module.Package.Project.GetPropertyValue(packageDependency.Name + ".buildmodules")).ToArray()
                    );
                }
                foreach (DependencyDeclaration.Group groupDependency in packageDependency.Groups)
                {
                    moduleNames.UnionWith(groupDependency.Modules);
                }

                // coolect resource dirs from modules, falling back to package level resource dirs if not defined at module level
                bool addedPackageLevel = false;
                foreach (string moduleName in moduleNames)
                {
                    string moduleSytemResourceDirsProperty = module.Package.Project.Properties["package." + packageDependency.Name + "." + moduleName + ".resourcedir." + module.Configuration.System];
                    string moduleResourceDirsProperty = module.Package.Project.Properties["package." + packageDependency.Name + "." + moduleName + ".resourcedir"];
                    if (moduleSytemResourceDirsProperty == null && moduleResourceDirsProperty == null)
                    {
                        // if module doesn't define module level resource dirs fall back to package level
                        if (!addedPackageLevel)
                        {
                            string packageSytemResourceDirsProperty = module.Package.Project.Properties["package." + packageDependency.Name + ".resourcedir." + module.Configuration.System];
                            string packageResourceDirsProperty = module.Package.Project.Properties["package." + packageDependency.Name + ".resourcedir"];
                            foreach (string dir in packageSytemResourceDirsProperty.LinesToArray())
                            {
                                additonalResourceDirs.Add(PathString.MakeNormalized(dir, PathString.PathParam.NormalizeOnly));
                            }
                            foreach (string dir in packageResourceDirsProperty.LinesToArray())
                            {
                                additonalResourceDirs.Add(PathString.MakeNormalized(dir, PathString.PathParam.NormalizeOnly));
                            }
                            addedPackageLevel = true;
                        }
                    }
                    else
                    {
                        // add module level data
                        foreach (string dir in moduleSytemResourceDirsProperty.LinesToArray())
                        {
                            additonalResourceDirs.Add(PathString.MakeNormalized(dir, PathString.PathParam.NormalizeOnly));
                        }
                        foreach (string dir in moduleResourceDirsProperty.LinesToArray())
                        {
                            additonalResourceDirs.Add(PathString.MakeNormalized(dir, PathString.PathParam.NormalizeOnly));
                        }
                    }
                }
            }
            return additonalResourceDirs;
        }
    }
}