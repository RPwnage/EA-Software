using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Reflection;
using System.Threading;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.Collections.Concurrent;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Threading;

namespace NAnt.Core.PackageCore
{
    public class PackageMap
    {
        public const string MasterConfigFile = "masterconfig.xml";

        public static PackageMap Instance
        {
            get
            {
                if (PackageMap._instance == null)
                {
                    throw new BuildException("PackageMap is not initialized, call Init(Project project) method");
                }
                return _instance;
            }

        }

        /// <summary>Returns a collection of packages.</summary>
        public readonly PackageCollection Packages;

        /// <summary>Returns a collection of packages.</summary>
        public readonly List<string> MasterPackages;

        /// <summary>Returns a collection of releases.</summary>
        public readonly ReleaseCollection Releases;

        /// <summary>
        /// The collection of package roots.
        /// </summary>
        public readonly PackageRootList PackageRoots;

        /// <summary>
        /// returns the build root directory
        /// </summary>
        public string BuildRoot
        {
            get { return _buildRoot.Path; }
        }
        private PathString _buildRoot;

        /// <summary>
        /// returns the configdir default build configuration
        /// </summary>

        public string DefaultConfig
        {
            get
            {
                return MasterConfig.Config.Default;
            }
        }

        /// <summary>
        /// Where is the directory containing configuration package?
        /// </summary>        
        public readonly string ConfigPackageDir;

        /// <summary>
        /// Where is the directory containing the build configurations?
        /// </summary>        
        public readonly string ConfigDir;

        /// <summary>
        /// Config package's name (for FW2 only)
        /// </summary>
        public string ConfigPackageName
        {
            get { return MasterConfig.Config.Package; }
        }


        /// <summary>
        /// Config package's version (for FW2 only)
        /// </summary>
        public readonly string ConfigPackageVersion;


        public IList<string> ConfigExcludes
        {
            get { return MasterConfig.Config.Excludes; }
        }

        public IList<string> ConfigIncludes
        {
            get { return MasterConfig.Config.Includes; }
        }

        /// <summary>
        /// Should we download missing packages from the package server on demand?
        /// This functionality make it harder than necessary to have a complete
        /// archive in source control, due to silent downloads of packages which
        /// haven't been checked into source control, so is turned off by default.
        /// </summary>
        public bool OnDemand
        {
            get { return _ondemand; }
        }
        private bool _ondemand;

        public List<Fw1Override> Framework1Overrides
        {
            get { return MasterConfig.Framework1Overrides; }
        }


        /// <summary>
        /// Manifest file of the top level project
        /// </summary>
        public readonly Manifest TopLevelProjectManifest;

        /// <summary>
        /// Returns the release directory containing the executing framework.
        /// This method does not add the release.
        /// </summary>
        /// <returns></returns>
        public DirectoryInfo GetFrameworkReleaseDirectory()
        {
            DirectoryInfo releaseDirectory = null;
            if (_executingFrameworkRelease != null)
            {
                releaseDirectory = new DirectoryInfo(_executingFrameworkRelease.Path);
                return releaseDirectory;
            }
            try
            {
                // start from where the executing assembly is located
                // navigate upward until we hit the release folder

                // TODO: might need to wrap this in PathNormalizer.Normalize???
                releaseDirectory = new DirectoryInfo(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location));
                while (releaseDirectory != null)
                {
                    if (Release.IsReleaseDirectory(releaseDirectory) && releaseDirectory.Parent.Name.Equals("Framework", StringComparison.OrdinalIgnoreCase))
                    {
                        // found it
                        return releaseDirectory;
                    }
                    releaseDirectory = releaseDirectory.Parent;
                }
            }
            catch { }
            // Can't find one. Returns null.
            return releaseDirectory;
        }

        /// <summary>Returns the Framework package release we are currently running or null.
        /// Assumes the framework package has already been added.
        /// </summary>
        public Release GetFrameworkRelease()
        {

            //if we already have _executingFrameworkRelease
            if (_executingFrameworkRelease != null)
            {
                //then return it
                return _executingFrameworkRelease;
            }
            try
            {
                DirectoryInfo info = GetFrameworkReleaseDirectory();

                if (info != null)
                {
                    _executingFrameworkRelease = Releases.FindByNameAndDirectory("Framework", info.FullName);
                    //if we found a matching release
                    if (_executingFrameworkRelease != null)
                    {
                        //then return it
                        return _executingFrameworkRelease;
                    }
                }
            }
            catch
            {
            }
            return null;
        }

        /// <summary>
        /// get master version of the given package; return null if none was set
        /// Called by Release
        /// </summary>
        public string GetMasterVersion(string packageName, Project project)
        {
            MasterConfig.Package masterPackage;

            if (!String.IsNullOrEmpty(packageName) && MasterConfig.Packages.TryGetValue(packageName, out masterPackage))
            {
                return masterPackage.EvaluateMasterVersion(project);
            }
            return null;
        }

        /// <summary>
        /// get group of the given package;        
        /// </summary>
        public string GetMasterGroup(string packageName, Project project)
        {
            MasterConfig.Package masterPackage;

            if (MasterConfig.Packages.TryGetValue(packageName, out masterPackage))
            {
                return masterPackage.EvaluateGrouptype(project).Name;
            }
            return null;
        }

        public Release GetMasterRelease(string packageName, Project project, bool? onDemand = null)
        {
            Release r = null;

            string version = GetMasterVersion(packageName, project);

            if (!String.IsNullOrEmpty(version))
            {
                r = InstallRelease(project, packageName, version, onDemand: onDemand);
            }

            return r;
        }

        public string GetBuildGroupRoot(string packageName, Project project)
        {
            if (!String.IsNullOrEmpty(BuildRoot))
            {
                Release r = GetMasterRelease(packageName, project);
                if (r != null)
                {
                    string buildgroup = r.BuildGroupName(project);
                    if (!String.IsNullOrEmpty(buildgroup))
                    {
                        //append groupType to buildroot
                        return Path.Combine(_buildRoot.NormalizedPath, buildgroup);
                    }
                }
            }
            return BuildRoot;
        }



        #region Implementation

        public readonly MasterConfig MasterConfig;

        public string MasterConfigFilePath
        {
            get
            {
                return MasterConfig == null ? string.Empty : MasterConfig.MasterconfigFile;
            }
        }

        public bool IsTopLevelProjectFramework2
        {
            get
            {
                return TopLevelProjectManifest.FrameworkVersion == FrameworkVersions.Framework2;
            }
        }

        public static void Init(Project project)
        {
            _instance = new PackageMap(project);
        }

        public static void Init(string buildFileDirectory, string masterconfigfile, string buildroot, bool autobuildAll, string autobuildPackages, string autobuildGroups, Project project)
        {
              _instance = CreateInstance(buildFileDirectory, masterconfigfile, buildroot, autobuildAll, autobuildPackages, autobuildGroups, project);
        }

        public static PackageMap CreateInstance(string buildFileDirectory, string masterconfigfile, string buildroot, bool autobuildAll, string autobuildPackages, string autobuildGroups, Project project)
        {
            return new PackageMap(buildFileDirectory, masterconfigfile, buildroot, autobuildAll, autobuildPackages, autobuildGroups, project);
        }

        public Release InstallRelease(Project project, string packageName, string packageVersion, Location location = null, bool? onDemand = null, bool withDependents = false)
        {
            Release releaseInfo = Releases.FindByNameAndVersion(packageName, packageVersion);
            if (releaseInfo != null)
            {
                //already sartisfied
                return releaseInfo;
            }

            bool ondemand = onDemand ?? OnDemand;

            if (ondemand)
            {
                try
                {
                    using (var installState = _packageInstallMap.StartBuild(packageName, packageVersion))
                    {
                        if (!installState.IsDone())
                        {

                            using (IPackageServer packageServer = PackageServerFactory.CreatePackageServer())
                            {
                                packageServer.Log = project.Log;
                                packageServer.UpdateProgress += new InstallProgressEventHandler(UpdateInstallProgress);

                                project.Log.Status.WriteLine("Searching for package '{0}-{1}' on the package server.", packageName, packageVersion);

                                Release packageRelease;
                                if (!packageServer.TryGetRelease(packageName, packageVersion, out packageRelease))
                                {
                                    string msg = String.Format("Package '{0}-{1}' not found on package server.", packageName, packageVersion);
                                    throw new ApplicationException(msg);
                                }

                                IList<Release> releasesToinstall = null;

                                if (withDependents)
                                {
                                    if (packageServer.TryGetReleaseDependents(packageName, packageVersion, out releasesToinstall) && releasesToinstall.Count > 0)
                                    {
                                        project.Log.Status.WriteLine("   Found {0} dependents for package '{1}-{2}' on the package server:", releasesToinstall.Count, packageName, packageVersion);
                                        foreach (Release r in releasesToinstall)
                                        {
                                            project.Log.Status.WriteLine("          '{0}-{1}'", r.Name, r.Version);
                                        }
                                    }

                                }
                                if (releasesToinstall == null)
                                {
                                    releasesToinstall = new List<Release>();
                                }

                                releasesToinstall.Add(packageRelease);

                                foreach (Release release in releasesToinstall)
                                {
                                    packageServer.Install(release, PackageRoots.OnDemandRoot.FullName);
                                }

                                // Rescan installed packages;
                                foreach (Release release in releasesToinstall)
                                {
                                    int maxdepth = PackageRootList.DefaultMaxLevel - PackageRootList.DefaultMinLevel;

                                    var packageDir = new DirectoryInfo(Path.Combine(PackageRoots.OnDemandRoot.FullName, release.Name));
                                   
                                    // Clear cache for this package so that package map will rescan
                                    Release.ClearPackageDirectoryCache(packageDir, maxdepth);

                                    AddPackage(packageDir, maxdepth, project.Log);
                                }
                            }
                        }
                    }

                }
                catch (Exception e)
                {
                    string msg = GetMissingReleaseErrorMessage(project, packageName, packageVersion, ondemand, String.Format("Failed to download package '{0}-{1}' or one of its dependents from the Package Server:\n    {2}", packageName, packageVersion, e.Message));

                    throw new BuildException(msg, location, BuildException.StackTraceType.None);
                }

                releaseInfo = Releases.FindByNameAndVersion(packageName, packageVersion);
            }

            // package still not there
            if (releaseInfo == null)
            {
                string currPkgMsg = "";
                if (project.Properties.Contains("package.name"))
                {
                    currPkgMsg = string.Format("Package {0} is depending on {1}.\n", project.Properties["package.name"], packageName);
                }
                throw new BuildException(GetMissingReleaseErrorMessage(project, packageName, packageVersion, ondemand, String.Format("{0}{0}{1}Cannot find package '{2}-{3}' in package roots:{0}{4}.", Environment.NewLine, currPkgMsg, packageName, packageVersion, PackageRoots.ToString(sep:Environment.NewLine, prefix:"\t"))), location);
            }
            return releaseInfo;
        }

        private void UpdateInstallProgress(object sender, InstallProgressEventArgs e)
        {
            if (e != null)
            {
                IPackageServer packageServer = sender as IPackageServer;
                if (packageServer != null && packageServer.Log != null)
                {
                    if (e.LocalProgress < 0)
                    {
                        packageServer.Log.Status.WriteLine(e.Message);
                    }
                    else
                    {
                        packageServer.Log.Debug.WriteLine("{0}. Progress - {1}%", e.Message, e.LocalProgress);
                    }
                }
            }
        }

        private string GetMissingReleaseErrorMessage(Project project, string packageName, string packageVersion, bool ondemand, string messageHeader)
        {

            StringBuilder message = new StringBuilder(messageHeader);

            if (null != project && null != project.BuildFileLocalName)
            {
                message.AppendFormat("{0}{0}'{1}-{2}' package is referenced in: '{3}'.", Environment.NewLine, packageName, packageVersion, project.BuildFileLocalName);
            }

            message.AppendFormat("{0}{0}If package '{1}-{2}' already exists in your package roots, make sure it is either under its version folder, or it has a <version> tag inside its manifest.", Environment.NewLine, packageName, packageVersion);
            Release info = Releases.FindByName(packageName);
            if (info != null)
            {
                int count = 0;
                foreach (Release r in info.Package.Releases)
                {
                    if (r.Path != String.Empty)
                    {
                        if (count == 0) message.AppendFormat("{0}{0}Following versions of the package {1} are found in package roots:{0}", Environment.NewLine, packageName);
                        message.AppendFormat("{0}   {1}-{2}:   {3}", Environment.NewLine, packageName, r.Version.PadRight(25), r.Path);
                        count++;
                    }
                }
            }
            if (ondemand)
            {
                try
                {
                    using (IPackageServer packageServer = PackageServerFactory.CreatePackageServer())
                    {
                        Release release;
                        if (packageServer.TryGetRelease(packageName, packageVersion, out release))
                        {
                            string url = release.Manifest.HomePageUrl;
                            if (String.IsNullOrEmpty(url))
                                message.AppendFormat("{0}The package can be installed from:{0}{1}", Environment.NewLine, url);
                        }
                        else
                        {
                            IList<Release> releases;
                            if (packageServer.TryGetPackageReleases(packageName, out releases))
                            {
                                List<Release> sorted = new List<Release>(releases);
                                sorted.Sort((x, y) => x.Status.CompareTo(y.Status));

                                int count = 0;
                                foreach (Release r in sorted)
                                {
                                    string url = r.HomePageUrl;
                                    if (!String.IsNullOrEmpty(r.HomePageUrl))
                                    {
                                        if (count == 0) message.AppendFormat("{0}{0}Following versions of the {1} package were found on the package server:{0}", Environment.NewLine, packageName);
                                        message.AppendFormat("{0}   {1}-{2} ({3}):   {4}", Environment.NewLine, r.Name, r.Version.PadRight(25), r.Status.ToString().PadRight(10), url);
                                        count++;
                                    }
                                }
                            }
                        }
                    }
                }
                catch (Exception) { }
            }
            return message.ToString();
        }

        private PackageMap(Project project)
        {
            _packageInstallMap = new PackageAutoBuilCleanMap();

            Packages = new PackageCollection();
            Releases = new ReleaseCollection(Packages);
            PackageRoots = new PackageRootList();

            Log log = null;

            if (project != null)
            {
                log = project.Log;
            }
            else
            {
                log = new Log(Log.LogLevel.Normal, id: String.Empty);
                log.Listeners.AddRange(LoggerFactory.Instance.CreateLogger(Log.DefaultLoggerTypes));
            }

            TopLevelProjectManifest = Manifest.DefaultManifest;

            MasterConfig = MasterConfig.UndefinedMasterconfig;

            MasterPackages = new List<string>();


            // Set Framework location as a root:

            PackageRoots.Add(PathString.MakeNormalized(GetFrameworkPackageRoot().FullName), null, null, PackageRootList.RootType.FrameworkRoot);

            if (project != null)
            {
                this._ondemand = project.Properties.GetBooleanPropertyOrDefault(Project.NANT_PROPERTY_ONDEMAND, MasterConfig.OnDemand);
            }
            else
            {
                this._ondemand = MasterConfig.OnDemand;
            }


            ScanAllPackagesInPackageRoots(log);

        }


        private PackageMap(string buildFileDirectory, string masterconfigfile, string buildroot, bool autobuildAll, string autobuildPackages, string autobuildGroups, Project project)
        {
            _packageInstallMap = new PackageAutoBuilCleanMap();

            Packages = new PackageCollection();
            Releases = new ReleaseCollection(Packages);
            PackageRoots = new PackageRootList();

            Log log = null;

            if (project != null)
            {
                log = project.Log;

            }
            else
            {
                log = new Log(Log.LogLevel.Normal, id:String.Empty);
                log.Listeners.AddRange(LoggerFactory.Instance.CreateLogger(Log.DefaultLoggerTypes));
            }
            
            TopLevelProjectManifest = Manifest.Load(buildFileDirectory, log);

            MasterConfig = MasterConfig.Load(GetMasterconfigFilePath(buildFileDirectory, masterconfigfile), autobuildAll, autobuildPackages, autobuildGroups);

            MasterConfig.MergeFragments(project);

            MasterPackages = new List<string>(MasterConfig.Packages.Keys);

            // SetProjectProperties needs to be executed before SetConfigPackage in case
            // there are exceptions specified in the masterconfig file for the the
            // configuration package version.

            if (TopLevelProjectManifest.FrameworkVersion == FrameworkVersions.Framework2)
            {
                SetGlobalAndGroupPropertiesFromMasterconfig(log);
            }

            if (project != null)
            {
                SetProjectProperties(project);
            }

            SetBuildRoot(project, buildroot);

            if (project!= null && !String.IsNullOrEmpty(BuildRoot))
            {
                project.Properties.AddReadOnly(Project.NANT_PROPERTY_PROJECT_BUILDROOT, BuildRoot);
                project.Properties.AddReadOnly(Project.NANT_PROPERTY_PROJECT_TEMPROOT, Path.Combine(BuildRoot, "framework_tmp"));
            }

            SetPackageRoots(project, log);

            if (project != null)
            {
                // set NANT_PROPERTY_PROJECT_PACKAGEROOTS, a multi-line property expected by eaconfig's package-external 
                // target to exclude package root folders. Example: tlapt_rw4 has a package root "contrib"
                // in its folder, and we don't want to package contrib.
                StringBuilder sb1 = new StringBuilder();
                foreach (var root in PackageRoots)
                {
                    sb1.AppendLine(root.Dir.FullName);
                }
                project.Properties.AddReadOnly(Project.NANT_PROPERTY_PROJECT_PACKAGEROOTS, sb1.ToString());

                this._ondemand = project.Properties.GetBooleanPropertyOrDefault(Project.NANT_PROPERTY_ONDEMAND, MasterConfig.OnDemand);
            }
            else
            {
                this._ondemand = MasterConfig.OnDemand;
            }

            ScanAllPackagesInPackageRoots(log);

            if (null == GetFrameworkRelease())
            {
                var info = GetFrameworkReleaseDirectory();

                throw new BuildException(String.Format("Unable to find executing Framework release '{0}' in the package roots", info != null ? info.FullName : "..."));
            }

            CheckCompatibility(project.Log, this.GetFrameworkRelease(), null);

            if (TopLevelProjectManifest.FrameworkVersion == FrameworkVersions.Framework2)
            {
                SetGlobalAndGroupPropertiesFromMasterconfig(log);

                // SetProjectProperties needs to be executed before SetConfigPackage in case
                // there are exceptions specified in the masterconfig file for the the
                // configuration package version.
                if (project != null)
                {
                    SetProjectProperties(project);
                }

                if (!String.IsNullOrEmpty(ConfigPackageName))
                {
                    SetConfigPackage(project, out ConfigPackageVersion, out ConfigPackageDir, out ConfigDir);
                }
                else if (TopLevelProjectManifest.Buildable)
                {
                    throw new BuildException("No masterconfig file for buildable frameworkVersion 2 package.");
                }
            }
            else if (project != null)
            {
                SetProjectProperties(project);
            }
        }

        private void SetConfigPackage(Project project, out string configPackageVersion, out string configpackagedir, out string configDir)
        {
            configPackageVersion = GetMasterVersion(ConfigPackageName, project);

            if (String.IsNullOrEmpty(configPackageVersion))
            {
                throw new BuildException("Cannot find master config version for config package '" + ConfigPackageName + "'");
            }

            // We need to search in the package map for the information in the package's manifest.
            Release releaseInfo = InstallRelease(project, ConfigPackageName, configPackageVersion);

            //Framework3 compatibility checks:
            CheckCompatibility(project.Log, releaseInfo, null);

            configpackagedir = releaseInfo.Path;

            configDir = Path.Combine(releaseInfo.Path, "config");

            if (String.IsNullOrEmpty(DefaultConfig))
            {
                if (!project.Properties.Contains(NAnt.Shared.Properties.PackageProperties.ConfigNameProperty))
                {
                    //if top level project is frameworkVersion 2 and buildable
                    if (TopLevelProjectManifest.Buildable && TopLevelProjectManifest.FrameworkVersion == FrameworkVersions.Framework2)
                    {
                        //fail cuz config is needed
                        throw new BuildException("Top level project is frameworkVersion 2 and buildable, so ${config} must be given if masterconfig file lacks <config default>.");
                    }
                }
            }
        }

        public void SetGlobalAndGroupPropertiesFromMasterconfig(Log log)
        {
            if (MasterConfig.GlobalProperties == null)
                return;

            // ---- Set Global properties:
            foreach (var prop in MasterConfig.GlobalProperties)
            {
                Project.GlobalProperties.Add(prop.Name, true, prop.Value, prop.Condition);
            }
        }

        private void SetProjectProperties(Project project)
        {
            // Set properties defined in the package map:

            if (!String.IsNullOrEmpty(MasterConfigFilePath))
            {
                project.Properties.AddReadOnly(Project.NANT_PROPERTY_PROJECT_MASTERCONFIGFILE, MasterConfigFilePath);
            }
            // Set global properties from Masterconfig:
            foreach (Project.GlobalPropertyDef propdef in Project.GlobalProperties.EvaluateExceptions(project))
            {
                if (propdef.InitialValue != null && !project.Properties.Contains(propdef.Name))
                {
                    // Top level project has all global properties except properties passed from the command line as writable.
                    // deferred set to false to allow even more delayed evaluation. NOTE. Scripts / tasks need to evaluate these properties explicitly. 
                    project.Properties.Add(propdef.Name, propdef.InitialValue, readOnly: false, deferred: false, local: false);
                }
            }
        }

        private PathString GetMasterconfigFilePath(string buildFileDirectory, string masterconfigfile)
        {
            string masterConfigFilePath = String.Empty;

            if (String.IsNullOrEmpty(masterconfigfile))
            {
                masterConfigFilePath = Path.Combine(buildFileDirectory, MasterConfigFile);
            }
            else
            {
                masterConfigFilePath = masterconfigfile;

                if (TopLevelProjectManifest.FrameworkVersion == FrameworkVersions.Framework1)
                {
                    string msg = "\nCan not use master configuration file: " + masterconfigfile +
                                " on this Framework 1.x package.\nMaster configuration file is a Framework 2.x feature.\n" +
                                "Please add Manifest.xml in the same location as your build file to make this package Framework 2.x compatible.\n";
                    throw new BuildException(msg);
                }
            }

            if (File.Exists(masterConfigFilePath))
            {
                if (TopLevelProjectManifest.FrameworkVersion == FrameworkVersions.Framework1)
                {
                    string msg = "\nMaster configuration file: " + masterConfigFilePath + " is found, but Manifest.xml is missing." +
                    "\nMaster configuration file is a Framework 2.x feature. Please add Manifest.xml in the same location as your build file to make this package Framework 2.x compatible\n" +
                    "or delete " + masterConfigFilePath + " to make it as Framework 1.x package.\n";
                    throw new BuildException(msg);
                }

                //masterconfigfile exists; if it's a relative path,
                //use Environment.CurrentDirectory as reference dir cuz it's coming from cmd line switch;
                //if givenMasterConfigFilePath is absolute, Path.Combine will return it
                //exactly (and ignore Environment.CurrentDirectory).  Then
                //store path to MasterConfigFile, for <nant> use
                masterConfigFilePath = Path.Combine(Environment.CurrentDirectory, masterConfigFilePath);

            }
            else
            {
                if (TopLevelProjectManifest.FrameworkVersion == FrameworkVersions.Framework2 && TopLevelProjectManifest.Buildable)
                {
                    string msg = String.Format("{0}  masterconfig file is missing: '{1}'.  Buildable Framework 2.x packages require a master configuration file.", String.IsNullOrEmpty(masterconfigfile) ? "Default" : "Given", masterconfigfile);
                    throw new BuildException(msg);
                }
                else
                {
                    // Caveat:  We allow missing masterconfig.xml files if we were just searching
                    // for them automatically in the same directory as the build file.
                    masterConfigFilePath = String.Empty;
                }
            }

            return PathString.MakeNormalized(masterConfigFilePath);
        }

        private void SetBuildRoot(Project project, string buildroot)
        {
            // Evaluate masterconfig build root:
            if (project != null)
            {
                // project is null in the case of eapm.
                if ((MasterConfig != null) && (MasterConfig.BuildRoot != null) && MasterConfig.BuildRoot.Exception != null)
                {
                    string result = null;
                    MasterConfig.BuildRoot.Exception.EvaluateException(project, out result);

                    if (result != null)
                    {
                        MasterConfig.BuildRoot.Path = result;
                    }
                }

                // Set Build root:
                var baselocation = String.IsNullOrWhiteSpace(MasterConfig.MasterconfigFile) ? Path.GetFullPath(project.BaseDirectory) : Path.GetDirectoryName(Path.GetFullPath(MasterConfig.MasterconfigFile));

                _buildRoot = PathString.MakeNormalized(
                    String.IsNullOrEmpty(buildroot) ?
                    Path.Combine(baselocation, MasterConfig.BuildRoot.Path ?? "build")
                    : buildroot);
            }
        }

        private void SetPackageRoots(Project project, Log log)
        {
            // We will first add all the packageroots to _packageroots, and then we will 
            // add the packages underneath them. This ensures we know all the roots when 
            // we add the packages, and we won't add a root as a package by mistake.
            if (TopLevelProjectManifest.FrameworkVersion == FrameworkVersions.Framework2)
            {
                // Framework 2: 1) add all roots in masterconfig; 2) Add on demand root 3) Add Framework as root

                if (MasterConfig != null)
                {
                    foreach (var root in MasterConfig.PackageRoots)
                    {
                        PathString path = MakePathStringFromProperty(root.EvaluatePackageRoot(project), project);

                        if (Directory.Exists(path.Path) == false)
                        {
                            if (Log.WarningLevel > Log.WarnLevel.Minimal)
                            {
                                string msg = String.Format("Package root given in masterconfig file does not exist: {0}", path.Path);
                                log.Warning.WriteLine(msg);
                            }
                            continue;
                        }

                        PackageRoots.Add(path, root.MinLevel, root.MaxLevel);
                    }

                    if (!String.IsNullOrEmpty(MasterConfig.OnDemandRoot))
                    {
                        PathString onDemandRoot = MakePathStringFromProperty(MasterConfig.OnDemandRoot, project);

                        try //Directory.CreateDirectory throws exceptions
                        {
                            if (Directory.Exists(onDemandRoot.Path) == false)
                            {
                                Directory.CreateDirectory(onDemandRoot.Path);
                            }

                            PackageRoots.Add(onDemandRoot, null, null, PackageRootList.RootType.OnDemandRoot);

                        }
                        catch (Exception e)
                        {
                            log.Warning.WriteLine("On-demand package download package root in masterconfig file could not be created: {0}", e.Message);
                        }
                    }
                }

                //IMTODO: See if instead of throwing an error we can just sort roots.
                // We need to handle higher level package roots first, such that release folders won't be 
                // identified as flattened packages, if the package folder happens to be a package root. 
                // Check the list of roots to see if there are any nested roots listed before their parents.
                // If so, throw an error.
                string nestedMsg = PackageRoots.HasNestedRootListedFirst();
                if (nestedMsg != null)
                {
                    throw new BuildException(nestedMsg);
                }

                // Now add Framework location as a root:

                PackageRoots.Add(PathString.MakeNormalized(GetFrameworkPackageRoot().FullName), null, null, PackageRootList.RootType.FrameworkRoot);
            }
            else
            {
                // Framework 1 logic:
                // Step 1: try searching from the assembly location to find the Framework package root
                PathString s1 = PathString.MakeNormalized(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), PathString.PathParam.NormalizeOnly);

                PackageRootFile packageRootFile = FindPackageRoot(s1, log);
                //if package root isn't found searching from executing framework location
                if (packageRootFile == null)
                {
                    var frameworkPackageRoot = PathString.MakeNormalized(GetFrameworkPackageRoot().FullName);

                    PackageRoots.Add(frameworkPackageRoot, null, null, PackageRootList.RootType.FrameworkRoot | PackageRootList.RootType.DefaultRoot);

                    // Framework is not installed under a proper package root.
                    // We throw an exception now because the build will fail anyway, 
                    // if the Framework package cannot be found and added.					
                    log.Warning.WriteLine("The top level package is a Framework 1.x package because of the missing manifest.xml file in the package directory. However the required " + PackageRootFile.PackageRootFileName + " file for Framework 1.x packages is missing." + Environment.NewLine + Environment.NewLine +
                        "1) If the top level package is actually a Framework 2 package, Please add manifest.xml in its package directory." + Environment.NewLine +
                        "2) Or add " + PackageRootFile.PackageRootFileName + " to any of the parent directories of the current executing nant.exe" + Environment.NewLine + Environment.NewLine +
                        "If problem still exists, please email Framework2 Users for help." + Environment.NewLine + Environment.NewLine +
                        "Executing Framework location {0} will be used as package root." + Environment.NewLine, frameworkPackageRoot.Path.Quote());
                }
                else
                {
                    //add packageDirectory1 to PackageRootList
                    PackageRoots.Add(packageRootFile.MainPackageRoot, null, null, PackageRootList.RootType.FrameworkRoot | PackageRootList.RootType.DefaultRoot);

                    // add any <packageDirs> in the framework package root packageroot.xml
                    foreach (PathString path in packageRootFile.AdditionalPackageRoots)
                    {
                        PackageRoots.Add(path, minlevel: null, maxlevel: null);
                    }

                    // Step 2: begin searching for the default package root from the current directory                
                    PathString s2 = PathString.MakeNormalized(Environment.CurrentDirectory, PathString.PathParam.NormalizeOnly);

                    PackageRootFile packageRootFile2 = FindPackageRoot(s2, log);

                    //if we found a package root different from frameworkRoot
                    if (packageRootFile2 != null && !packageRootFile2.MainPackageRoot.Equals(packageRootFile.MainPackageRoot))
                    {
                        //add it to PackageRootList as Default root
                        PackageRoots.Add(packageRootFile2.MainPackageRoot, null, null, PackageRootList.RootType.DefaultRoot);

                        //IMTODO: Move Framework root to be the last 
                        PackageRoots.Add(packageRootFile.MainPackageRoot, null, null, PackageRootList.RootType.FrameworkRoot);

                        // add any <packageDirs> in the packageroot.xml
                        foreach (PathString path in packageRootFile2.AdditionalPackageRoots)
                        {
                            PackageRoots.Add(path, minlevel: null, maxlevel: null);
                        }
                    }
                }
            }

            if (log.Level >= Log.LogLevel.Detailed)
            {
                log.Info.WriteLine("Package roots:");
                foreach (var root in PackageRoots)
                {
                    log.Info.WriteLine("    [minlevel={0} maxlevel={1}] '{2}'", root.MinLevel, root.MaxLevel, root.Dir.FullName);
                }
                log.Info.WriteLine("");
            }

        }

        private void CollectRoots(ICollection<PackageRootList.Root> roots, DirectoryInfo dir, int minlevel, int maxlevel, int depth)
        {
            if (depth == minlevel)
            {
                roots.Add(new PackageRootList.Root(dir, 0, maxlevel - depth));
                return;
            }

            foreach (var dirInfo in dir.GetDirectories())
            {
                CollectRoots(roots, dirInfo, minlevel, maxlevel, depth + 1);
            }
        }

        private DirectoryInfo GetFrameworkPackageRoot()
        {
            // Framework 2 logic:
            // 1. Look for the release directory of the excuting framework.
            // 2. Navigate upward until we hit a folder that contains a folder called "Framework".
            //    Assume that is the fw root.
            // 3. Add the root. Set it as fw and default root.
            DirectoryInfo curr = GetFrameworkReleaseDirectory();
            while (curr != null && curr.GetDirectories("Framework").Length == 0)
            {
                curr = curr.Parent;
            }
            if (curr == null)
            {
                // We cannot find a folder with the name Framework
                throw new ApplicationException("Error: can not find the container directory 'Framework' for the current executing framework.\n" +
                    " One possible reason is that the executing framework is not installed properly. Installing it again might fix the problem.");
            }
            return curr;
        }

        private void ScanAllPackagesInPackageRoots(Log log)
        {
#if PROFILE
            System.Diagnostics.Stopwatch sw = new System.Diagnostics.Stopwatch();
            sw.Start();
#endif
            // Do not run this loop in parallel, need to process roots in order.
            foreach (var root in PackageRoots)
            {
                if (root.MinLevel == 0)
                {
                    AddPackages(root, log);
                }
                else
                {
                    var roots = new List<PackageRootList.Root>();
                    CollectRoots(roots, root.Dir, root.MinLevel, root.MaxLevel, 0);
                    foreach (var subroot in roots)
                    {
                        AddPackages(root, log);
                    }
                }
            }
#if PROFILE
            sw.Stop();
            Console.WriteLine("PackageMap.ScanAllPackagesInPackageRoots: {0} millisec ({1} Packages, {2} Releases)", sw.ElapsedMilliseconds, Packages.Count, Releases.Count);
#endif

            if (log.Level >= Log.LogLevel.Detailed)
            {
                log.Info.WriteLine("Found {0} packages[{1} releases] in package roots.", Packages.Count, Releases.Count);

                if (log.Level >= Log.LogLevel.Diagnostic)
                {
                    log.Debug.WriteLine("");
                    foreach (Package p in Packages.Values)
                    {
                        log.Debug.WriteLine("    {0}", p.Name);
                        foreach (Release r in p.Releases)
                        {
                            log.Debug.WriteLine("        {0} - '{1}'", r.Version.PadRight(20), r.Path);
                        }
                    }
                    log.Info.WriteLine("");
                }
            }


        }

        /// <summary>
        /// Adds any package releases located in the specified package root. If a specific release 
        /// already exists in the map it will not be added.
        /// </summary>
        public void AddPackages(PackageRootList.Root packageRoot, Log log)
        {
            try
            {
#if NANT_CONCURRENT
            Parallel.ForEach(packageRoot.Dir.GetDirectories(), dirInfo =>
#else
                foreach (var dirInfo in packageRoot.Dir.GetDirectories())
#endif
                {
                    AddPackage(dirInfo, packageRoot.MaxLevel - packageRoot.MinLevel, log);
                }
#if NANT_CONCURRENT
            );
#endif
            }
            catch (Exception ex)
            {
                ThreadUtil.RethrowThreadingException(String.Format("Error scanning package root '{0}'", packageRoot.Dir.FullName), ex);
            }
        }

        /// <summary>
        /// Adds any package releases located in the specified package folder. If a specific release 
        /// already exists in the map it will not be added.
        /// </summary>
        private Package AddPackage(DirectoryInfo packageDirectory, int maxdepth, Log log)
        {
            MasterConfig.Package masterpackage;
            if (!MasterConfig.Packages.TryGetValue(packageDirectory.Name, out masterpackage))
            {
                masterpackage = null;
            }
            Package package = new Package(this, packageDirectory, maxdepth, masterpackage);

            if (package.Releases.Count > 0)
            {
                Package existing = Packages.AddOrUpdate(package.Name, package, (name, existingPackage) =>
                    {
                        foreach (Release r in package.Releases)
                        {
                            Release oldRelease = existingPackage.Releases.FindByNameAndVersion(r.Name, r.Version);

                            if (oldRelease == null)
                            {
                                existingPackage.Releases.Add(new Release(r, existingPackage));
                            }
                            else
                            {
                                bool skipwarning = false;

                                if(0 == String.Compare(r.Path, oldRelease.Path, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase))
                                {
                                    skipwarning = true;
                                }

                                // If it is Framework, the self reference should take precedence over other locations of Framework with the same version.
                                if (String.Compare(oldRelease.Name, "Framework", true) == 0)
                                {
                                    DirectoryInfo info = GetFrameworkReleaseDirectory();

                                    if (info != null)
                                    {
                                        if (String.Compare(r.Path, PathNormalizer.Normalize(info.FullName), true) == 0)
                                        {
                                            existingPackage.Releases.Remove(oldRelease);
                                            existingPackage.Releases.Add(new Release(r, existingPackage));
                                            skipwarning = true;
                                        }
                                    }
                                }


                                if (!skipwarning && Log.WarningLevel > Log.WarnLevel.Minimal && masterpackage != null && masterpackage.ContainsMasterVersion(oldRelease.Version))
                                {
                                    log.Warning.WriteLine("Duplicate package release found: '{0}' and '{1}', second release ignored.", oldRelease.Path, r.Path);
                                }
                            }
                        }
                        return existingPackage;
                    });

                return existing;
            }
            return null;
        }

        /// <summary>
        /// Search upwards from the specified path, until a package root, or null, is found.
        /// </summary>
        private PackageRootFile FindPackageRoot(PathString path, Log log)
        {
            PackageRootFile packageRootFile = null;

            DirectoryInfo dirInfo = new DirectoryInfo(path.Path);

            while (dirInfo != null && null == (packageRootFile = PackageRootFile.Load(dirInfo.FullName, log)))
            {
                dirInfo = dirInfo.Parent;
            }
            return packageRootFile;
        }

        /// <summary>Determines if the given directory is a package root.</summary>
        /// <param name="directory">The complete path to the directory to test.</param>
        /// <param name="log">Instance of Log</param>
        /// <returns>True if the directory is a package root; otherwise false.</returns>
        public bool IsPackageRoot(DirectoryInfo directory, Log log)
        {
            return null != PackageRootFile.Load(directory.FullName, log);
        }

        // returns normalized full path.
        // Relative paths are computed against masterconfig location.
        private PathString MakePathStringFromProperty(string dir, Project project)
        {
            string path = project == null ? dir : project.ExpandProperties(dir);
            return PathString.MakeCombinedAndNormalized(Path.GetFullPath(Path.GetDirectoryName(MasterConfigFilePath)), path);
        }

        private Release _executingFrameworkRelease = null;

        private enum PackageConfigBuildStatus { Built = 1, Cleaned = 2 }


        private readonly PackageAutoBuilCleanMap _packageInstallMap;

        private static PackageMap _instance = null;

        #endregion

        #region Framework1Overrides helper

        public class Framework1OverridesHelper : IDisposable
        {
            public Framework1OverridesHelper(Project project, Release info)
            {
                _project = project;

                if (_project.Properties["package.frameworkversion"] == "2" //I'm Fw2
                    && info.FrameworkVersion != FrameworkVersions.Framework2)	// depending on Fw1
                {
                    _map = new Dictionary<string, string>();

                    foreach (Fw1Override over in PackageMap.Instance.Framework1Overrides)
                    {
                        string propVal = project.Properties[over.Name];
                        if (propVal != null)
                        {
                            foreach (Fw1OverrideItem item in over.Items)
                            {
                                if (string.Equals(item.From, propVal, StringComparison.Ordinal))
                                {
                                    _map[over.Name] = propVal;
                                    _project.Properties.UpdateReadOnly(over.Name, item.To);
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            public void Dispose()
            {
                Dispose(true);
                GC.SuppressFinalize(this);
            }

            void Dispose(bool disposing)
            {
                if (!this._disposed)
                {
                    if (disposing && _map != null)
                    {
                        foreach (KeyValuePair<string, string> item in _map)
                        {
                            _project.Properties.UpdateReadOnly(item.Key, item.Value);
                        }

                    }
                }
                _disposed = true;
            }
            private bool _disposed = false;
            private readonly Project _project;
            private readonly Dictionary<string,string> _map;

        }

        private void VerifyCompatibilityData(Log log, CompatibilityData compatibilityData, Release release)
        {
            if (compatibilityData != null && (1 == Interlocked.Increment(ref compatibilityData.Verified)))
            {

                if (compatibilityData.MinCondition != null && compatibilityData.MinCondition.revision != null)
                {
                    if (release.Manifest.Compatibility != null && release.Manifest.Compatibility.Revision != null)
                    {
                        if (0 < compatibilityData.MinCondition.revision.StrCompareVersions(release.Manifest.Compatibility.Revision))
                        {
                            var msg = String.Format("Package '{0}' requires minimal revision '{1}' of package '{2}', but package '{2}-{3}' ({4}) has revision '{5}' {6}",
                                compatibilityData.MinCondition.ConditionSource.Package.Name, compatibilityData.MinCondition.revision, release.Package.Name, release.Version, release.Path, release.Manifest.Compatibility.Revision,
                                String.IsNullOrWhiteSpace(compatibilityData.MinCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MinCondition.Message);

                            if (compatibilityData.MinCondition.fail)
                            {
                                throw new BuildException(msg);
                            }
                            else
                            {
                                log.Warning.WriteLine(msg);
                            }
                        }
                    }
                    else
                    {
                        var msg = String.Format("Package '{0}' requires minimal revision '{1}' of package '{2}', but Manifest.xml file of package '{2}-{3}' ({4}) does not declare revision number. Can not check compatibility. {5}",
                                      compatibilityData.MinCondition.ConditionSource.Package.Name, compatibilityData.MinCondition.revision, release.Package.Name, release.Version, release.Path, String.IsNullOrWhiteSpace(compatibilityData.MinCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MinCondition.Message);

                        if (compatibilityData.MinCondition.fail)
                        {
                            throw new BuildException(msg);
                        }
                        else
                        {
                            log.Warning.WriteLine(msg);
                        }
                    }
                }

                if (compatibilityData.MinVersionCondition != null && compatibilityData.MinVersionCondition.revision != null)
                {
                    if (0 < compatibilityData.MinVersionCondition.revision.StrCompareVersions(release.Version))
                    {
                        var msg = String.Format("Package '{0}' requires minimal version '{1}' of package '{2}', but package '{2}' ({3}) has version '{4}' {5}",
                            compatibilityData.MinVersionCondition.ConditionSource.Package.Name, compatibilityData.MinVersionCondition.revision, release.Package.Name, release.Path, release.Version,
                            String.IsNullOrWhiteSpace(compatibilityData.MinVersionCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MinVersionCondition.Message);

                        if (compatibilityData.MinVersionCondition.fail)
                        {
                            throw new BuildException(msg);
                        }
                        else
                        {
                            log.Warning.WriteLine(msg);
                        }
                    }
                }

                if (compatibilityData.MaxCondition != null && compatibilityData.MaxCondition.revision != null)
                {
                    if (release.Manifest.Compatibility != null && release.Manifest.Compatibility.Revision != null)
                    {
                        if (0 > compatibilityData.MaxCondition.revision.StrCompareVersions(release.Manifest.Compatibility.Revision))
                        {
                            var msg = String.Format("Package '{0}' requires maximal revision '{1}' of package '{2}', but package '{2}-{3}' ({4}) has revision '{5}' {6}",
                                compatibilityData.MaxCondition.ConditionSource.Package.Name, compatibilityData.MaxCondition.revision, release.Package.Name, release.Version, release.Path, release.Manifest.Compatibility.Revision,
                                String.IsNullOrWhiteSpace(compatibilityData.MaxCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MaxCondition.Message);

                            if (compatibilityData.MaxCondition.fail)
                            {
                                throw new BuildException(msg);
                            }
                            else
                            {
                                log.Warning.WriteLine(msg);
                            }
                        }
                    }
                    else
                    {
                        var msg = String.Format("Package '{0}' requires maximal revision '{1}' of package '{2}', but Manifest.xml file of package '{2}-{3}' ({4}) does not declare revision number. Can not check compatibility. {5}",
                                       compatibilityData.MaxCondition.ConditionSource.Package.Name, compatibilityData.MaxCondition.revision, release.Package.Name, release.Version, release.Path, String.IsNullOrWhiteSpace(compatibilityData.MaxCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MaxCondition.Message);

                        if (compatibilityData.MaxCondition.fail)
                        {
                            throw new BuildException(msg);
                        }
                        else
                        {
                            log.Warning.WriteLine(msg);
                        }
                    }
                }

                if (compatibilityData.MaxVersionCondition != null && compatibilityData.MaxVersionCondition.revision != null)
                {
                    if (0 > compatibilityData.MaxVersionCondition.revision.StrCompareVersions(release.Version))
                    {
                        var msg = String.Format("Package '{0}' requires maximal version '{1}' of package '{2}', but package '{2}' ({3}) has version '{4}' {5}",
                            compatibilityData.MaxVersionCondition.ConditionSource.Package.Name, compatibilityData.MaxVersionCondition.revision, release.Package.Name, release.Path, release.Version,
                            String.IsNullOrWhiteSpace(compatibilityData.MaxVersionCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MaxVersionCondition.Message);

                        if (compatibilityData.MaxVersionCondition.fail)
                        {
                            throw new BuildException(msg);
                        }
                        else
                        {
                            log.Warning.WriteLine(msg);
                        }
                    }
                }
            }
        }

        public void CheckCompatibility(Log log, Release release, Func<string, Release> packages)
        {
            // Check this release against existing conditions:
            CompatibilityData compatibilityData;

            if(CompatibilityConditions.TryGetValue(release.Package.Name, out compatibilityData))
            {
                VerifyCompatibilityData(log, compatibilityData, release);
            }


            var compatibility = release.Manifest.Compatibility;
            if (compatibility != null)
            {
                foreach (var dependent in compatibility.Dependents)
                {
                    CompatibilityData updated = null;

                    CompatibilityConditions.AddOrUpdate(dependent.PackageName,
                        (key) =>
                        {
                            updated = new CompatibilityData(dependent, release);
                            return updated;
                        },
                        (key, data) =>
                        {
                            CompatibilityData.Condition minCondition = null;
                            CompatibilityData.Condition minVersionCondition = null;
                            CompatibilityData.Condition maxCondition = null;
                            CompatibilityData.Condition maxVersionCondition = null;

                            if (dependent.MinRevision != null)
                            {
                                    if (data.MinCondition.revision == null || 0 < dependent.MinRevision.StrCompareVersions(data.MinCondition.revision))
                                    {
                                        minCondition = new CompatibilityData.Condition(dependent.MinRevision, data.MinCondition.fail ? true : dependent.Fail, dependent.Message, release);
                                    }
                            }
                            if (dependent.MinVersion != null)
                            {
                                if (data.MinVersionCondition.revision == null || 0 < dependent.MinVersion.StrCompareVersions(data.MinVersionCondition.revision))
                                {
                                    minVersionCondition = new CompatibilityData.Condition(dependent.MinVersion, data.MinVersionCondition.fail ? true : dependent.Fail, dependent.Message, release);
                                }
                            }

                            if (dependent.MaxRevision != null)
                            {
                                    if (data.MaxCondition.revision == null || 0 > dependent.MaxRevision.StrCompareVersions(data.MaxCondition.revision))
                                    {
                                        maxCondition = new CompatibilityData.Condition(dependent.MaxRevision, data.MaxCondition.fail ? true : dependent.Fail, dependent.Message, release);
                                    }
                            }
                            if (dependent.MaxVersion != null)
                            {
                                if (data.MaxVersionCondition.revision == null || 0 > dependent.MaxVersion.StrCompareVersions(data.MaxVersionCondition.revision))
                                {
                                    maxVersionCondition = new CompatibilityData.Condition(dependent.MaxRevision, data.MaxVersionCondition.fail ? true : dependent.Fail, dependent.Message, release);
                                }
                            }

                            updated = new CompatibilityData(minCondition ?? new CompatibilityData.Condition(data.MinCondition.revision, data.MinCondition.fail, data.MinCondition.Message, data.MinCondition.ConditionSource),
                                                            minVersionCondition ?? new CompatibilityData.Condition(data.MinVersionCondition.revision, data.MinVersionCondition.fail, data.MinVersionCondition.Message, data.MinVersionCondition.ConditionSource),
                                                            maxCondition ?? new CompatibilityData.Condition(data.MaxCondition.revision, data.MaxCondition.fail, data.MaxCondition.Message, data.MaxCondition.ConditionSource),
                                                            maxVersionCondition ?? new CompatibilityData.Condition(data.MaxVersionCondition.revision, data.MaxVersionCondition.fail, data.MaxVersionCondition.Message, data.MaxVersionCondition.ConditionSource));
                            return updated;
                        }
                  );

                    if (updated!= null && packages != null)
                    {
                        //Verify existing packages against new/updated compatibility condition:
                        var packageRelease = packages(dependent.PackageName);
                        if (packageRelease != null)
                        {
                            VerifyCompatibilityData(log, updated, packageRelease);
                        }
                    }
                }
            }
        }

        private class CompatibilityData
        {
            internal readonly Condition MinCondition;
            internal readonly Condition MaxCondition;
            internal readonly Condition MinVersionCondition;
            internal readonly Condition MaxVersionCondition;

            internal int Verified = 0;

            internal CompatibilityData(Compatibility.Dependent dependent, Release source)
            {
                MinCondition = new Condition(dependent.MinRevision, dependent.Fail, dependent.Message, source);
                MinVersionCondition = new Condition(dependent.MinVersion, dependent.Fail, dependent.Message, source);
                MaxCondition = new Condition(dependent.MaxRevision, dependent.Fail, dependent.Message, source);
                MaxVersionCondition = new Condition(dependent.MaxVersion, dependent.Fail, dependent.Message, source);
            }

            internal CompatibilityData(Condition minCondition, Condition minVersionCondition, Condition maxCondition, Condition maxVersionCondition)
            {
                MinCondition = minCondition;
                MinVersionCondition = minVersionCondition;
                MaxCondition = maxCondition;
                MaxVersionCondition = maxVersionCondition;
            }

            internal class Condition
            {
                internal string revision;
                internal bool fail;
                internal string Message;
                internal Release ConditionSource;

                internal Condition(string rev, bool isFail, string message, Release source)
                {
                    revision = rev;
                    fail = isFail;
                    Message = message;
                    ConditionSource = source;
                }
            }
        }
        /// <summary>Returns a collection of releases.</summary>
        private readonly ConcurrentDictionary<string, CompatibilityData> CompatibilityConditions = new ConcurrentDictionary<string,CompatibilityData>();

        #endregion
    }
}
