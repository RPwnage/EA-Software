// (c) 2002-2003 Electronic Arts Inc.

using System;
using System.Data;
using System.IO;
using System.Xml;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Text.RegularExpressions;
using System.Threading;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Shared.Properties;
using NAnt.Core.PackageCore;

using EA.FrameworkTasks.Model;

namespace EA.FrameworkTasks
{

    /// <summary>Makes this package dependent on another.</summary>
    /// <remarks>
    ///   <para>
    ///   This task must be embedded within a &lt;target> with style 'build'
    ///   or 'clean' if this dependent, <b>Framework 2</b> package is to autobuildclean.
    ///   </para>
    ///   <para>
    ///   This task reaches into the named package's <b>scripts</b>
    ///   folder and executes the <b>Initialize.xml</b> (as a NAnt <b>include</b>).
    ///   This file is optional and does not need to exist.
    ///   </para><para>
    ///   <c>Intialize.xml</c> is the mechanism to make any property
    ///   from your package available to users of your package's
    ///   contents.
    ///   </para><para>
    ///   By default, NAnt will create the properties
    ///   <list type="table">
    ///     <item>
    ///       <term>package.all</term>
    ///       <description>List of descriptive names of all the packages this build file depends on.</description>
    ///     </item>
    ///     <item>
    ///       <term>package.<i>name</i>.dir</term>
    ///       <description>Base directory f package <i>name</i>.</description>
    ///     </item>
    ///     <item>
    ///       <term>package.<i>name</i>.version</term>
    ///       <description>Specific version number of package <i>name</i>.</description>
    ///     </item>
    ///		<item><term>package.<i>name</i>.frameworkversion</term><description>The version number of the Framework the given package is designed for, determined from the <b>&lt;frameworkVersion&gt;</b> of the package's Manifest.xml.  Default value is 1 if Manifest.xml doesn't exist.</description></item>
    ///   </list>
    ///   </para><para>
    ///   By convention, packages should only define properties using
    ///   names of the form <c>package.<i>name</i>.<i>property</i></c>
    ///   Following the convention avoids namespace problems and makes
    ///   it much easier for other people to use the properties your
    ///   package sets.
    ///   </para><para>
    ///   The value of <c>package.<i>name</i>.dir</c> isn't
    ///   interesting in the case of a proxy package; proxy packages
    ///   should use <c>Initialize.xml</c> to set <c>package.<i>name</i>.appdir</c> to the directory where the executable for the software proxied by the package resides.
    ///   </para><para>
    ///   Any required initialization code to use the dependent package should appear
    ///   in the <b>Initialize.xml</b> file.
    ///   </para><para>
    ///   One use for the <b>Initialize.xml</b> file is for compiler packages to find where 
    ///   the compiler is installed on the local machine and set a property.
    ///   </para>
    /// </remarks>
    /// <include file='Examples/Dependent/Dependent.example' path='example'/>
    [TaskName("dependent")]
    public class DependentTask : Task, IDependentTask
    {
        private string _PackageName;
        private string _PackageVersion;
        private string _PackageConfig;
        private bool _OnDemand = true;
        private bool _InitializeScript = true;
        private bool _GetDependentDependents = true;
        private XmlNode _taskNode;
        private Model.IPackage _dependentPackage;
        private Project.TargetStyleType? _targetStyle = null;
        private string _target = null;
        private IList<string> _dependencystack = null;
        private bool _dropCircular = false;

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
        public string PackageName
        {
            get { return _PackageName; }
            set { _PackageName = value; }
        }

        /// <summary>The version of the package (eg. 1.02.03). 
        /// Not applicable to a Framework 2 package</summary>
        [TaskAttribute("version", Required = false)]
        [StringValidator(Trim = true)]
        public string PackageVersion
        {
            get { return _PackageVersion; }
            set { _PackageVersion = value; }
        }

        private string FullPackageName
        {
            get { return String.Format("{0}-{1}", PackageName, PackageVersion); }
        }

        /// <summary>The value of the config property to use when initializing this package.
        /// Not applicable to a (buildable) Framework 2 package using &lt;config> in 
        /// masterconfig.xml.</summary>
        [TaskAttribute("config", Required = false)]
        public string PackageConfig
        {
            get { return _PackageConfig; }
            set
            {
                //if <config> was used in masterconfig.xml and package is not Framework 1
                Release release = PackageMap.Instance.Releases.FindByName(_PackageName);
                if (null != PackageMap.Instance.ConfigDir && null != release &&
                    release.FrameworkVersion == FrameworkVersions.Framework2)
                {
                    //avoid using LogPrefix [dependent] since -quickrun:list_dependents and Dependency
                    //Analyzer use it with an ASSUMED format
                    Log.Info.WriteLine("WARNING: 'config' attribute will be ignored for package {0}-{1} due to <config> in masterconfig.", _PackageName, _PackageVersion);
                }
                else
                {
                    _PackageConfig = value;

                }
            }
        }

        /// <summary>If true the package will be automatically downloaded from the package server. Default is true.</summary>
        [TaskAttribute("ondemand", Required = false)]
        public bool OnDemand
        {
            get { return _OnDemand; }
            set { _OnDemand = value; }
        }

        /// <summary>If false the execution of the Initialize.xml script will be suppressed. Default is true.</summary>
        [TaskAttribute("initialize", Required = false)]
        public bool InitializeScript
        {
            get { return _InitializeScript; }
            set { _InitializeScript = value; }
        }

        /// <summary>When package context is Framework 1, setting this to false 
        /// prevents the installation of the package release's "Required Packages", 
        /// only as specified on the release's web page on packages.ea.com.  
        /// Default is true.  When package context is Framework 2, the value is
        /// ALWAYS false and its setting by user is deprecated.</summary>
        [TaskAttribute("getdependents", Required = false)]
        public bool GetDependentDependents
        {
            get
            {
                if (Project.Properties["package.frameworkversion"] == "1")
                {
                    return _GetDependentDependents;
                }
                else
                {
                    //DEPRECATED attribute for framework 2 pkg; never GetDependentDependents
                    return false;
                }
            }
            set
            {
                if (Project.Properties["package.frameworkversion"] == "1")
                {
                    _GetDependentDependents = value;
                }
                else
                {
                    //DEPRECATED attribute for framework 2 pkg; never GetDependentDependents

                    //avoid using LogPrefix [dependent] since -quickrun:list_dependents and Dependency
                    //Analyzer use it with an ASSUMED format
                    Log.Info.WriteLine("WARNING: Deprecated 'getdependents' attribute will be ignored for package {0}-{1}.", _PackageName, _PackageVersion);
                }
            }
        }

        /// <summary>Warning level for missing or mismatching version. Default is NOTHING (0).</summary>
        [TaskAttribute("warninglevel", Required = false)]
        public WarningLevel Level
        {
            get { return _warningLevel; }
            set { _warningLevel = value; }
        }

        /// <summary>Drop circular build dependencie. If false throw on circulatr build dependencies</summary>
        [TaskAttribute("dropcircular", Required = false)]
        public bool DropCircular
        {
            get { return _dropCircular; }
            set { _dropCircular = value; }
        }



        public Project.TargetStyleType TargetStyle
        {
            get { return _targetStyle ?? Project.TargetStyle; }

            set { _targetStyle = value; }
        }

        public string Target
        {
            get { return _target; }

            set { _target = value; }
        }


        // HACK: Provide a new LogPrefix property to allow the <requires> task to change
        // the name of this task so we don't duplicate code.
        string _logPrefix = null;

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


        /// <summary>Initializes the task and checks for correctness.</summary>
        protected override void InitializeTask(XmlNode taskNode)
        {
            _taskNode = taskNode;
        }


#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return PackageName; }
		}
#endif
        // For metrics processing
        public bool IsAlreadyBuilt = false;
        public bool IsAlreadyCLeaned = false;

        protected override void ExecuteTask()
        {
            IsAlreadyBuilt = false;
            bool isProjectFramework2;

            bool isReleaseSatisfied = false;

            DependentPackageInfo dependentInfo = InitDependentPackageInfo(out isProjectFramework2);

            lock (Project.SyncRoot)
            {
                isReleaseSatisfied = IsReleaseSatisfied(dependentInfo.ReleaseInfo);

                if (!isReleaseSatisfied)
                {
                    if (IsPackageSatisfied(dependentInfo.ReleaseInfo))
                    {
                        Log.Warning.WriteLine(LogPrefix + "{0} ({1}) {2} (different version already included)", dependentInfo.ReleaseInfo.FullName, dependentInfo.ReleaseInfo.Path, ConfigName);
                    }

                    AddPackageProperties(Project, dependentInfo.ReleaseInfo);
                }
                else
                {
                    if (!Project.BuildGraph().TryGetPackage(dependentInfo.ReleaseInfo.Name, _PackageVersion, ConfigName, out _dependentPackage))
                    {
                        // When builg graph was reset there is a chance that package was cleared from Build graph, but dependency was already executed. 
                        //In this case let the code fall through, otherwise throw an exception
                        if (!BuildGraph.WasReset)
                        {
                            throw new BuildException(String.Format("Expected an already satisfied package dependency in the buildgraph for {0} ({1}), but didn't find it.", dependentInfo.PackageFullName, ConfigName));
                        }
                    }
                }

                _dependentPackage = Project.BuildGraph().GetOrAddPackage(dependentInfo.ReleaseInfo.Name, _PackageVersion, PathString.MakeNormalized(dependentInfo.ReleaseInfo.Path), ConfigName, dependentInfo.ReleaseInfo.FrameworkVersion);
                _dependentPackage.SetType(dependentInfo.AutoBuildClean? Model.Package.AutoBuildClean : BitMask.None);
                _dependentPackage.SetType(dependentInfo.ReleaseInfo.Buildable ? Model.Package.Buildable : BitMask.None);
            }

            PackageAutoBuilCleanMap packageAutoBuilCleanMap = Project.BuildGraph().PackageAutoBuilCleanMap;

            switch (TargetStyle)
            {
                case Project.TargetStyleType.Use:

                    if (!packageAutoBuilCleanMap.IsCleanedNonBlocking(dependentInfo.PackageFullName, ConfigName))
                    {
                        if (isReleaseSatisfied)
                        {
                            Log.Debug.WriteLine(LogPrefix + "{0} {1} (already satisified)", dependentInfo.ReleaseInfo.FullName, ConfigName);
                        }
                        else
                        {
                            Log.Info.WriteLine(LogPrefix + "{0} ({1}) {2}", dependentInfo.ReleaseInfo.FullName, dependentInfo.ReleaseInfo.Path, ConfigName);
                        }
                    }
                    break;

                case Project.TargetStyleType.Clean:

                    using (PackageAutoBuilCleanMap.PackageAutoBuilCleanState autoCleanState = packageAutoBuilCleanMap.StartClean(dependentInfo.PackageFullName, Project.Properties[PackageProperties.ConfigNameProperty]))
                    {
                        if (false == FindCircularDependency(dependentInfo, (msg)=>
                            {
                                if (DropCircular)
                                {
                                    Log.Info.WriteLine("Dropped " + msg, Location);
                                }
                                else
                                {
                                    Log.Warning.WriteLine("Dropped "+ msg, Location);
                                }
                            }))
                        {
                            if (autoCleanState.IsDone())
                            {
                                Log.Info.WriteLine(LogPrefix + "Package {0}-{1} ({2})already auto-cleaned", PackageName, PackageVersion, ConfigName);
                                IsAlreadyCLeaned = true;
                            }
                            else
                            {
                                AutoBuildClean(dependentInfo, TargetStyle);
                            }
                        }
                    }
                    break;

                case Project.TargetStyleType.Build:

                    using (PackageAutoBuilCleanMap.PackageAutoBuilCleanState autoBuildState = packageAutoBuilCleanMap.StartBuild(dependentInfo.PackageFullName, ConfigName))
                    {
                        if (false == FindCircularDependency(dependentInfo, (msg)=>
                            {
                                if (DropCircular)
                                {
                                    Log.Info.WriteLine("Dropped " + msg, Location);
                                }
                                else
                                {
                                    throw new BuildException("Detected " + msg, Location);
                                }
                            }))                            
                        {
                            if (autoBuildState.IsDone())
                            {
                                Log.Info.WriteLine(LogPrefix + "Package {0}-{1} ({2}) already auto-built", PackageName, PackageVersion, ConfigName);
                                IsAlreadyBuilt = true;
                            }
                            else
                            {
                                AutoBuildClean(dependentInfo, TargetStyle);
                            }
                        }
                    }
                    break;

                default:
                    throw new BuildException(String.Format("Unknown NAnt Project TargetStype = '{0}'", TargetStyle));
            }

            if (InitializeScript && TargetStyle != Project.TargetStyleType.Clean)
            {
                InternalInitializeScript(dependentInfo.ReleaseInfo);
            }


            if (_dependentPackage == null)
            {
                // Something is wrong here
                 return;
            }

            Model.IPackage parent;
            if (Project.TryGetPackage(out parent))
            {
                ((Model.Package)parent).TryAddDependentPackage(_dependentPackage, (dependentInfo.AutoBuildClean && (TargetStyle == Project.TargetStyleType.Build || TargetStyle == Project.TargetStyleType.Clean)) ? PackageDependencyTypes.Build : PackageDependencyTypes.Use);
            }
            else
            {
                // This is happening when dependent task is called before package task. WE can safely ignore this warning.
                //Log.Warning.WriteLine(LogPrefix + "[{0}]: Dependent task can't retrieve IPackage object from parent project for package '{1}-{2}'{3}{3}\t**** Most likely <package> task is not declared in the package build script! ****{3}\t**** Check {0}{3}", Location, _dependentPackage.Name, _dependentPackage.Version, Environment.NewLine);
            }
        }

        private void AutoBuildClean(DependentPackageInfo dependentInfo, Project.TargetStyleType targetStyle)
        {
            // Auto build the package?
            if (dependentInfo.AutoBuildClean)
            {
                using (NAntTask nantTask = new NAntTask())
                {
                    nantTask.StartNewBuild = false;
                    nantTask.Project = Project;
                    // string buildFile = Path.Combine(Project.Properties["package." + PackageName + ".dir"], PackageName + ".build");
                    // nantTask.BuildFileName = buildFile;
                    string dir = Project.Properties["package." + PackageName + ".dir"];
                    string buildFileName = BuildFile.GetPackageBuildFileName(dir, PackageName, false);

                    nantTask.BuildFileName = buildFileName;

                    if (Target == null)
                    {
                        string target = "build";

                        if (targetStyle == Project.TargetStyleType.Build)
                        {
                            // Reverted correct fix for autobuild target
                            //if(!String.IsNullOrEmpty(info.AutoBuildTarget))
                            if (dependentInfo.ReleaseInfo.AutoBuildTarget != null)
                            {
                                target = dependentInfo.ReleaseInfo.AutoBuildTarget;
                            }
                            Log.Info.WriteLine(LogPrefix + "Auto-building " + PackageName + "-" + PackageVersion + "\n", target);
                        }
                        else if (targetStyle == Project.TargetStyleType.Clean)
                        {
                            target = dependentInfo.ReleaseInfo.AutoCleanTarget;
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
                            nantTask.DefaultTarget = target;
                    }
                    else
                    {
                        nantTask.DefaultTarget = Target;
                    }

                    nantTask.DoInitialize();

                    if (nantTask.NestedProject.DefaultTargetName != "build")
                    {
                        Log.Info.WriteLine(Log.Padding + "Auto-building '" + PackageName + "-" + PackageVersion + "': build script default target differs from the standard autobuild target, this is not a bug but make sure this is intended");
                    }

                    //autobuildclean using project config
                    nantTask.NestedProject.Properties.AddReadOnly(PackageProperties.ConfigNameProperty, Project.Properties[PackageProperties.ConfigNameProperty]);

                    nantTask.NestedProject.SetConstraints(Constraints);

                    if (Project.Properties.Contains(Project.NANT_PROPERTY_PROJECT_MASTERTARGET))
                    {
                        nantTask.NestedProject.Properties.UpdateReadOnly(Project.NANT_PROPERTY_PROJECT_MASTERTARGET, Project.Properties[Project.NANT_PROPERTY_PROJECT_MASTERTARGET]);
                    }

                    if (Project.Properties.Contains(Project.NANT_PROPERTY_PROJECT_CMDTARGETS))
                    {
                        nantTask.NestedProject.Properties.UpdateReadOnly(Project.NANT_PROPERTY_PROJECT_CMDTARGETS, Project.Properties[Project.NANT_PROPERTY_PROJECT_CMDTARGETS]);
                    }

                    // Set parent package for dependent
                    nantTask.NestedProject.Properties.AddReadOnly("package." + PackageName + ".parent", Project.Properties["package.name"]);

                    // Set dependency stack property for detecting circular dependencies:

                    nantTask.NestedProject.Properties.AddReadOnly("package.dependency.stack", _dependencystack.ToNewLineString());

                    nantTask.ExecuteTaskNoDispose();

                    Model.IPackage depPackage;
                    if (!nantTask.NestedProject.TryGetPackage(out depPackage))
                    {
                        Log.Warning.WriteLine(LogPrefix + "Internal error: Dependent task can't retrieve IPackage object from nested project. Make sure package {0}-{1} build file conins <package> task.", PackageName, PackageVersion);
                    }

                    if (_dependentPackage.Project == null)
                    {
                        Log.Warning.WriteLine(LogPrefix + "Internal error: Project object was not set to the package {0}-{1}.", PackageName, PackageVersion);
                        _dependentPackage.Project = nantTask.NestedProject;
                    }

                    //write log entry in format ASSUMED by "nant -quickrun:list_dependents"
                    Log.Info.WriteLine(LogPrefix + "{0} ({1})", dependentInfo.ReleaseInfo.FullName, dependentInfo.ReleaseInfo.Path);
                }
            }
            else
            {
                Log.Info.WriteLine("Package {0}-{1} not marked for autobuild", PackageName, PackageVersion);
            }
        }

        private void InternalInitializeScript(Release info)
        {
            lock (Project.SyncRoot)
            {
                var originalTargetStyle = Project.TargetStyle;
                using (new OnExit(() => Project.TargetStyle = originalTargetStyle))
                {
                    // Set target style to br 'Use' as we don't want to trigger any build when Initialize.xml invokes <dependent> task
                    // As long as we are inside lock() there shouldn't be any concurrency problems;

                    Project.TargetStyle = Project.TargetStyleType.Use;  

                    //package might have been included with initialize=false before
                    //and we not want that to not be the case..
                    if (!IsReleaseInitialized(info))
                    {
                        Project.Properties.AddReadOnly(String.Format("package.{0}.initialized", info.FullName), info.FullName);

                        if (IsAlreadyBuilt)
                        {
                            Log.Info.WriteLine("But we haven't run the Initialize.xml script yet within the " + Project.Properties["package.name"] + " package.");
                        }

                        using (PackageMap.Framework1OverridesHelper doOver = new PackageMap.Framework1OverridesHelper(Project, info))
                        {
                            IncludeInitializeScript(info);
                        }
                    }
                    else
                    {
                        Log.Debug.WriteLine(LogPrefix + "{0} {1} (already initialized)", info.FullName, ConfigName);
                    }
                }
            }
        }

        private void IncludeInitializeScript(Release info)
        {

            // update the config property as needed
            string originalConfig = null;
            try
            {
                if (PackageConfig != null)
                {
                    originalConfig = Project.Properties.UpdateReadOnly(PackageProperties.PackageConfigPropertyName, PackageConfig);
                    Project.Properties.UpdateReadOnly(PackageProperties.ConfigNameProperty, PackageConfig);
                }

                // include the script
                if (!IncludeInitializeScript(Project, info))
                {
                    // Skip NOTE for FW1 packages, to avoid warnings on configuration packages
                    if (info.FrameworkVersion == FrameworkVersions.Framework2)
                    {
                        if (!String.IsNullOrEmpty(info.Name) &&
                            !(info.Name.Equals("eaconfig", StringComparison.OrdinalIgnoreCase)
                            || info.Name.Equals("rwconfig", StringComparison.OrdinalIgnoreCase)
                            || info.Name.Equals("Framework", StringComparison.OrdinalIgnoreCase)))
                        {
                            Log.Warning.WriteLine(LogPrefix + " NOTE: Initialize.xml script not found in package '{0}' [{1}].", info.Name, Path.Combine(info.Path, Path.Combine("scripts", "Initialize.xml")));
                        }
                    }
                }

            }
            finally
            {
                // reset the config property to its original value
                if (originalConfig != null)
                {
                    Project.Properties.UpdateReadOnly(PackageProperties.PackageConfigPropertyName, originalConfig);
                    Project.Properties.UpdateReadOnly(PackageProperties.ConfigNameProperty, originalConfig);
                }
            }
        }

        static internal void AddPackageProperties(Project project, Release info)
        {
            // add package.Framework-master.dir
            project.Properties.AddReadOnly(String.Format("package.{0}.dir", info.FullName), info.Path);

            // add package.Framework.dir 
            // TODO: make sure this package has the highest version (master and x.x override x.x.x)
            project.Properties.AddReadOnly(String.Format("package.{0}.dir", info.Name), info.Path);
            project.Properties.AddReadOnly(String.Format("package.{0}.version", info.Name), info.Version);

            string buildroot = PackageMap.Instance.GetBuildGroupRoot(info.Name, project);
            if (buildroot != null && info.FrameworkVersion == FrameworkVersions.Framework2)
            {
                //set package.{0}.builddir using buildroot, eg. ${buildroot}/eacore/1.00.00
                project.Properties.AddReadOnly(String.Format("package.{0}.builddir", info.Name),
                    Path.Combine(buildroot, String.Format("{0}/{1}", info.Name, info.Version)));
            }
            else
            {
                //set package.{0}.builddir w/o buildroot, eg. /eacore/1.00.00
                project.Properties.AddReadOnly(String.Format("package.{0}.builddir", info.Name), info.Path);
            }
            project.Properties.AddReadOnly("package." + info.Name + ".frameworkversion", info.FrameworkVersion.ToString("d"));

            // update package.all property
            const string AllProperty = "package.all";
            string all = project.Properties[AllProperty];
            if (all == null)
            {
                project.Properties.Add(AllProperty, info.Name);
            }
            else
            {
                all = all + " " + info.Name;
                project.Properties[AllProperty] = all;
            }
        }

        private bool IncludeInitializeScript(Project project, Release info)
        {
            // use the <include> task to include the "Initialize" script if it exists
            bool found = false;
            string initScriptPath = Path.Combine(info.Path, Path.Combine("scripts", "Initialize.xml"));
            if (!File.Exists(initScriptPath))
            {
                // Some packages may use lower case, which is important on Linux
                initScriptPath = Path.Combine(info.Path, Path.Combine("scripts", "initialize.xml"));
            }
            if (File.Exists(initScriptPath))
            {
                Log.Info.WriteLine(LogPrefix + "include '{0}'", initScriptPath);

                IncludeTask includeTask = new IncludeTask();
                includeTask.Project = project;
                includeTask.FileName = initScriptPath;
                includeTask.Execute();
                found = true;
            }
            return found;
        }

        private DependentPackageInfo InitDependentPackageInfo(out bool isProjectFramework2)
        {
            isProjectFramework2 = Project.Properties["package.frameworkversion"] == "2";

            SetPackageVersion(isProjectFramework2);

            Release info = DownloadFromPackageServerIfNecessary();

            DependentPackageInfo dependentPackageInfo =  new DependentPackageInfo(PackageName + "-" + PackageVersion, info, Project);

            ValidatePackage(dependentPackageInfo, isProjectFramework2);

            return dependentPackageInfo;
        }

        private void SetPackageVersion(bool isProjectFramework2)
        {
            if (!isProjectFramework2)
            {
                if (String.IsNullOrEmpty(_PackageVersion))
                {
                    Log.Warning.WriteLine("Calling <dependent> within a Framework 1.x package and 'version' doesn't exist.  You must specify a 'version' attribute.");
                }
            }
            else
            {
                //fw 2 package uses master version

                if (_PackageVersion != null)
                {
                    if (Log.WarningLevel >= Log.WarnLevel.Deprecation)
                    {
                        //we avoid LogPrefix here cuz Dependency Analyzer currently expects 
                        //a package name-version after LogPrefix (see WriteLineIf below)
                        Log.Warning.WriteLine("package {1}: 'version' attribute is no longer supported for <dependent> {0} in Framework 2.x package {2} or the config package used by it.", _PackageName, _PackageVersion, Project.Properties[Project.NANT_PROPERTY_PROJECT_BUILDFILE]);
                    }
                }

                string masterVersion = PackageMap.Instance.GetMasterVersion(PackageName, Project);
                if (masterVersion == null)
                {
                    string currPkgMsg = "";
                    if (Project.Properties.Contains("package.name"))
                    {
                        currPkgMsg = string.Format("Package {0} is depending on {1}.  ", Project.Properties["package.name"], PackageName);
                    }
                    string msg = string.Format("ERROR:  {0}You must specify a version for package '{1}' in your masterconfig.xml file.", currPkgMsg, PackageName);
                    throw new BuildException(msg, Location);
                }

                _PackageVersion = masterVersion;
            }
        }

        /// <summary>
        /// If the package we're depending on isn't found and download-on-demand is enabled
        /// then attempt to download the missing package from the package server.
        /// Here are the rules of using which OnDemand:
        /// Top-Level	2nd-Level	Which OnDemand
        /// 1.x			1.x			DependenTask
        /// 1.x			2.x			DependenTask
        /// 2.x			1.x			PackageMap
        /// 2.x			2.x			PackageMap
        /// 
        /// It doesn't download 3rd level packages. The top level package doesn't know if a 2nd level package
        /// is Framework 1.x or 2.x if it's missing.
        /// </summary>
        private Release DownloadFromPackageServerIfNecessary()
        {
            bool onDemand = PackageMap.Instance.IsTopLevelProjectFramework2 ? PackageMap.Instance.OnDemand : OnDemand;
            return PackageMap.Instance.InstallRelease(Project, PackageName, PackageVersion, Location, onDemand);
        }


        private bool FindCircularDependency(DependentPackageInfo dependentInfo, Action<string> onCircularAction)
        {
            _dependencystack = Project.Properties["package.dependency.stack"].LinesToArray();

            // This is an optimization, we can avoid building DAG slice
            var has_circular = Project.Properties["package.dependency.stack"].LinesToArray().Contains(_dependentPackage.Key);

            _dependencystack.Add(_dependentPackage.Key);

            Model.IPackage parent;
            if (Project.TryGetPackage(out parent))
            {
                ((Model.Package)parent).TryAddDependentPackage(_dependentPackage, (dependentInfo.AutoBuildClean && (TargetStyle == Project.TargetStyleType.Build || TargetStyle == Project.TargetStyleType.Clean)) ? PackageDependencyTypes.Build : PackageDependencyTypes.Use);
            }

            if (has_circular)
            {
                return true;
            }

            // More thorrow analysis using DAG 
            // Construct portion of the DAG with packages in the dependencystack
            var unsorted = new Dictionary<string,EA.Eaconfig.DAGNode<IPackage>>();

            foreach (var key in _dependencystack.Distinct().OrderBy(k=>k))
            {
                IPackage package;
                if(Project.BuildGraph().Packages.TryGetValue(key, out package))
                {
                    var newNode = new EA.Eaconfig.DAGNode<IPackage>(package);
                    unsorted.Add(key, newNode);
                }
            }

            // Populate dependencies
            var stack = new Stack<EA.Eaconfig.DAGNode<IPackage>>(unsorted.Values);
            while (stack.Count > 0)
            {
                var dagNode = stack.Pop();
                // Sort dependents so that we always have the same order after sorting the whole graph
                // Note, looks like under mono OrderBy is crashing when collection is changing, convert to list first
                foreach (var dep in dagNode.Value.DependentPackages.Where(d => d.IsKindOf(PackageDependencyTypes.Build)).ToList().OrderBy(d => d.Dependent.Key))
                {
                    EA.Eaconfig.DAGNode<IPackage> childDagNode;
                    if (unsorted.TryGetValue(dep.Dependent.Key, out childDagNode))
                    {
                        dagNode.Dependencies.Add(childDagNode);
                    }
                    else
                    {
                        var newNode = new EA.Eaconfig.DAGNode<IPackage>(dep.Dependent);
                        unsorted.Add(dep.Dependent.Key, newNode);
                        dagNode.Dependencies.Add(newNode);
                        stack.Push(newNode);
                    }
                }
            }
            

            var sorted = EA.Eaconfig.DAGNode<IPackage>.Sort(unsorted.Values, (a, b) =>
            {


                if (_dependentPackage.Key == a.Value.Key || _dependentPackage.Key == b.Value.Key)
                {
                    var msg = String.Format("circular build dependency between packages '{0}' ({1}) and '{2}' ({3})", a.Value.Name, a.Value.ConfigurationName, b.Value.Name, b.Value.ConfigurationName);

                    onCircularAction(msg);

                    has_circular = true;
                }
            });

            return has_circular;
        }

        /// <summary>
        /// Perform miscellaneous validity checks on the package we're depending on.
        /// </summary>
        private void ValidatePackage(DependentPackageInfo dependentPackageInfo, bool isFramework2Project)
        {
            //Framework3 compatibility checks:
            PackageMap.Instance.CheckCompatibility(Log, dependentPackageInfo.ReleaseInfo, (pkg) =>
                {
                    IPackage package;
                    if (Project.BuildGraph().Packages.TryGetValue(pkg, out package))
                    {
                        if (package != null)
                        {
                            return PackageMap.Instance.Releases.FindByNameAndVersion(package.Name, package.Version);
                        }
                    }
                    return null;
                });


            //if dependent is AutoBuildClean and call context is fw 2
            //and <dependent> is not nested under <target>
            if (dependentPackageInfo.AutoBuildClean && isFramework2Project)
            {
                if (!IsNestedUnderTarget())
                {
                    //then warn cuz we can't autobuildclean w/o knowing if style is build or clean;
                    //we avoid LogPrefix here cuz Dependency Analyzer currently expects 
                    //a package name-version after LogPrefix (see WriteLineIf below)
                    Log.Debug.WriteLine("Can't AutoBuildClean {0} since <dependent> is not nested under <target>.", dependentPackageInfo.ReleaseInfo.FullName);
                }
                CheckCompatibility(dependentPackageInfo.ReleaseInfo);
            }

            if (IsDependingOnSelf(dependentPackageInfo.ReleaseInfo))
            {
                throw new BuildException("Cannot depend on the current package.", Location);
            }
        }

        private string ConfigName
        {
            get
            {
                return PackageConfig ?? (Project.Properties[PackageProperties.ConfigNameProperty] ?? PackageMap.Instance.DefaultConfig);
            }
        }

        /// <summary>
        /// denote whether dependent is nested under a target task
        /// </summary>
        bool IsNestedUnderTarget()
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

        private bool IsDependingOnSelf(Release info)
        {
            //IMTODO: I don't need to normalize. Use PathString consistently to make sure everything is normalized.             
            string packagePath = PathNormalizer.Normalize(Project.BaseDirectory);
            string dependentPath = PathNormalizer.Normalize(info.Path);

            return (packagePath == dependentPath);
        }

        void CheckCompatibility(Release info)
        {
            Compatibility comp = info.Compatibility;
            if (comp != null)
            {
                StringCollection warnings = comp.CheckCompatibility(_PackageVersion);
                if (warnings.Count > 0)
                {
                    foreach (string warning in warnings)
                        //we avoid LogPrefix here cuz Dependency Analyzer currently expects 
                        //a package name-version after LogPrefix (see WriteLineIf below)
                        Log.Warning.WriteLine(warning);
                }
            }
        }

        /// <summary>Checks if dependent has already been executed with exact package name and version.</summary>
        /// <param name="info">Release info of the package depending on.</param>
        /// <returns><c>true</c> if already executed, otherwise <c>false</c>.</returns>
        bool IsReleaseSatisfied(Release info)
        {
            return Project.Properties.Contains(String.Format("package.{0}.dir", info.FullName));
        }

        bool IsReleaseInitialized(Release info)
        {
            return Project.Properties.Contains(String.Format("package.{0}.initialized", info.FullName));
        }

        /// <summary>Checks if dependent has already been executed with exact package name.</summary>
        /// <param name="info">Release info of the package depending on.</param>
        /// <returns><c>true</c> if already executed, otherwise <c>false</c>.</returns>
        bool IsPackageSatisfied(Release info)
        {
            bool res = Project.Properties.Contains(String.Format("package.{0}.dir", info.Name));

            if (res)
            {
                // In case of self dependency package.{0}.dir property is set by a package task, and it can't be used to detect if 
                // dependency on the package is already satisfied.
                if (Project.Properties["package.name"] == info.Name && Project.Properties["package.version"] == info.Version)
                {
                    res = false;
                }
            }

            return res;
        }

        private class DependentPackageInfo
        {
            internal DependentPackageInfo(string packageFullName, Release info, Project project)
            {
                PackageFullName = packageFullName;
                ReleaseInfo = info;
                AutoBuildClean = ReleaseInfo.AutoBuildClean(project);

            }
            internal readonly Release ReleaseInfo;
            internal readonly string PackageFullName;
            internal readonly bool AutoBuildClean;

        }
    }
}
