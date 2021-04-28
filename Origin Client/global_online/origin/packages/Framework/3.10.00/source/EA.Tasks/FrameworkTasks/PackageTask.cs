// Copyright (C) Electronic Arts Canada Inc. 2002.  All rights reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Shared.Properties;
using PackageCore = NAnt.Core.PackageCore;

using EA.FrameworkTasks.Model;

namespace EA.FrameworkTasks {
    
    /// <summary>Declares a package.</summary>
    /// <remarks>
    /// <para>
    /// This task should be called only once per package build file.
    /// </para>
    /// <para>
    /// The package specifies a version number evolution scheme.
    /// In the <b>dev</b> branch this should be of the form <b>major.minor</b>, indicating
    /// the number of the next major/minor release expected (the one you are working on).  
    /// In the <b>version</b> branches
    /// the number should be in the form <b>major.minor</b> reflecting the major/minor release 
    /// number of this release candidate.  The <b>targetversion</b> attribute is used (and required) by the system when
    /// the package is zipped up.
    /// </para>
    /// <para>The package specifies a default configuration. 
    /// You may override the default configuration on the command line by specifying
    /// <b>-D:config=configname</b> where configname is the name of the configuration to use.
    /// </para>
    /// <para>
    /// The task declares the following properties:
    /// </para>
    /// <list type='table'>
    /// <listheader><term>Property</term><description>Description</description></listheader>
    /// <item><term>${package.name}</term><description>The name of the package.</description></item>
    /// <item><term>${package.targetversion}</term><description>The version number of the package, determined from the <b>targetversion</b> attribute</description></item>
    /// <item><term>${package.version}</term><description>The version number of the package, determined from the <b>path</b> to the package</description></item>
    /// <item><term>${package.${package.name}.version}</term><description>Same as <b>${package.version}</b> but the property name includes the package name.</description></item>
    /// <item><term>${package.config}</term><description>The configuration to build.</description></item>
    /// <item><term>${package.configs}</term><description>For a Framework 1 package, it's a space delimited list 
    /// of all the configs found in the config folder.  For a Framework 2 package, this property excludes any configs
    /// specified by &lt;config excludes> in masterconfig.xml</description></item>
    /// <item><term>${package.dir}</term><description>The directory of the package build file (depends on packageroot(s) but path should end in <b>/${package.name}/${package.version}</b>.</description></item>
    /// <item><term>${package.${package.name}.dir}</term><description>The directory of the package build file (depends on packageroot(s) but path should end in <b>/${package.name}/${package.version}</b>.</description></item>
    /// <item><term>${build.dir}</term><description>The build directory of the package (depends on <b>buildroot</b> but path should end in <b>/${package.name}/${package.version}/build</b>.</description></item>
    /// <item><term>${package.builddir}</term><description>The parent directory of ${build.dir} (depends on <b>buildroot</b> but path should end in <b>/${package.name}/${package.version}</b>.</description></item>
    /// <item><term>${package.${package.name}.builddir}</term><description>Same as <b>${package.builddir}</b> but the property name includes the package name.</description></item>
    /// <item><term>${package.${package.name}.buildroot}</term><description>The directory for building binaries (can be set in masterconfig.xml via <b>buildroot</b> and <b>grouptype</b>) .</description></item>
    /// <item><term>${package.frameworkversion}</term><description>The version number of the Framework the package is designed for, determined from the <b>&lt;frameworkVersion&gt;</b> of the package's manifest.</description></item>
    /// <item><term>${package.${package.name}.parent}</term><description>Name of this package's parent package (applies only when this package is set to autobuildclean).</description></item>
    /// </list>
    /// <para>
    /// For Framework 1.x packages, the task declares the following standard targets, unless the <b>nostandardtargets="true"</b> attribute is used:
    /// </para>
    /// <list type="table">
    /// <listheader><term>Target</term><description>Description</description></listheader>
    /// <item><term>cleanall</term><description>Remove the <b>${build.dir}</b> directory.</description></item>
    /// <item><term>clean</term><description>Remove the <b>${build.dir}/${package.config}</b> directory.</description></item>
    /// <item><term>publish</term><description>Copy exported files from the export filesets.</description></item>
    /// <item><term>package</term><description>Create a zip file under the build folder for distribution.</description></item>
    /// <item><term>configure</term><description>Generate a visual studio (VS) makefile project using <b>generateproject</b> task. VS version depends on <b>generateproject</b> task's <b>version</b> attribute.</description></item>
    /// <item><term>configure70</term><description>Generate a visual studio 7.0 makefile project using <b>generateproject</b> task.</description></item>
    /// <item><term>configure71</term><description>Generate a visual studio 7.1 makefile project using <b>generateproject</b> task</description></item>
    /// 
    /// </list>
    /// <para>
    /// For Framework 2.x, standard targets are declared by standard packages such as eaconfig. For details of standard
    /// targets, please refer to documentation of the standard package.
    /// </para>
    /// <para>
    /// Package names cannot have a '-' character.  This is to prevent ambiguity when extracting 
    /// the name of the package and version from a <c>&lt;PackageName&gt;-&lt;Version&gt;</c> string.
    /// </para>
    /// <para>
    /// If the package you are building is outside of your packages folder, then the package version
    /// will be <b>private</b>.  No other packages will be able to depend on this package but
    /// it can depend on other packages in your packages folder.
    /// </para>
    /// </remarks>
    /// <include file='Examples/Package/Package.example' path='example'/>
    /// <include file='Examples/Package/Configure.example' path='example'/>
    [TaskName("package")]
    public class PackageTask : Task {
        string	_packageName;
        string	_targetVersion;
        string	_excludeTargets = string.Empty;
        string	_defaultConfiguration = "";
        string	_fromFileSet = null;
        FileSet	_fileset = new FileSet();
        bool	_noStandardTargets = false;
        bool	_useDefaultFileSet = true;
        bool    _initializeSelf = false;
        string _configDirectoryName = "config";
        string _frameworkVersion;

        private static readonly ConcurrentDictionary<string, string[]> _configFilesCache = new ConcurrentDictionary<string, string[]>();

        /// <summary>The name of a package comes from the directory name if the package is in the packages directory.  Use this attribute to name a package that lives outside the packages directory.</summary>
        /// <remarks>
        ///   <para>The <c>name</c> attribute is used for packages that live outside of the packages directory.</para>
        /// </remarks>
        [TaskAttribute("name", Required=true)]
        public string PackageName {
            get { return _packageName; }
            set { _packageName = value; }
        }

        /// <summary>The version of the package in development.</summary>
        [TaskAttribute("targetversion", Required=false)]
        public string TargetVersion {
            get { return _targetVersion; }
            set { _targetVersion = value; }
        }

        /// <summary> If <c>excludetargets</c> contains target names (delimited by spaces), we will exclude all 
        /// of those targets from the list of standard targets. Ignored for Framework 2.x packages.</summary>
        [TaskAttribute("excludetargets", Required=false)]
        public string ExcludeTargets {
            get 
            { 
                if (IsFramework2)
                    Log.Info.WriteLine(LogPrefix + 
                        "WARNING: 'excludetargets' attribute will be ignored for Framework 2.x packages");
                return _excludeTargets; 
            }
            set 
            { 
                if (IsFramework2)
                    Log.Info.WriteLine(LogPrefix + 
                        "WARNING: 'excludetargets' attribute will be ignored for Framework 2.x packages");
                else
                    _excludeTargets = value; 
            }
        }

        /// <summary> If true then exclude all standard targets.  Default is "false". Ignored for Framework 2.x 
        /// packages.</summary>        
        [TaskAttribute("nostandardtargets", Required=false)]
        public bool NoStandardTargets {
            get 
            { 
                if (IsFramework2)
                    Log.Info.WriteLine(LogPrefix + 
                        "WARNING: 'nostandardtargets' attribute will be ignored for Framework 2.x packages");
                return _noStandardTargets; 
            }
            set 
            { 
                if (IsFramework2)
                    Log.Info.WriteLine(LogPrefix + 
                        "WARNING: 'nostandardtargets' attribute will be ignored for Framework 2.x packages");
                else
                    _noStandardTargets = value; 
            }
        }

        /// <summary>The name of the default configuration. No longer supported for Framework 2.x packages.</summary>
        [TaskAttribute("defaultconfig", Required=false)]
        public string DefaultConfiguration {
            get 
            {
                // For Framework 1.x, <package> has defaultconfig; but for Framework 2.x, it's been
                // replaced by one defined in master config file, and you can't set it here
                if (IsFramework2)
                    throw new BuildException("'defaultconfig' attribute cannot be used for Framework 2.x packages.");
                else
                    return _defaultConfiguration; 
            }
            set 
            {
                // Ditto
                if (IsFramework2)
                    throw new BuildException("'defaultconfig' attribute cannot be used for Framework 2.x packages.");
                else
                    _defaultConfiguration = value; 
            }
        }


        /// <summary>The name of fileset to package.</summary>
        [TaskAttribute("fromfileset", Required=false)]
        public string FromFileSet {
            get { return _fromFileSet; }
            set { _fromFileSet = value; }
        }

        /// <summary>A fileset to package. Ignored for Framework 2.x packages.</summary>
        [FileSet("fileset", Required=false)]
        public FileSet FileSet {
            get { return _fileset; }
        }

        /// <summary>The directory containing the configuration files. Default is 'config'. Ignored for Framework 2.x 
        /// packages.</summary>
        [TaskAttribute("configdir", Required=false)]
        public string ConfigDirectoryName {
            get { return _configDirectoryName; }
            set 
            { 
                // Ditto
                if (IsFramework2)
                {
                    Log.Info.WriteLine(LogPrefix + "WARNING: 'configdir' attribute will be ignored for package {0}-{1}.", _packageName, _targetVersion);
                }
                else
                    _configDirectoryName = value; 
            }
        }

        private bool UseDefaultFileSet {
            get { return _useDefaultFileSet; }
            set { _useDefaultFileSet = value; }
        }

        /// <summary>If true the packages own Initialize.xml script will be loaded. Default is false.</summary>
        [TaskAttribute("initializeself", Required=false)]
        public bool InitializeSelf {
            get { return _initializeSelf; }
            set { _initializeSelf = value; }
        }

        public string PackageVersion {
            get { return Project.Properties[PackageProperties.PackageVersionPropertyName]; }
        }

        public string FullPackageName {
            get { return PackageName + "-" + PackageVersion; }
        }

        protected override void InitializeTask(XmlNode taskNode) {
            foreach(XmlNode node in taskNode.ChildNodes) {
                if(node.Name == "fileset") {
                    // file set has been specified
                    UseDefaultFileSet = false;
                    break;
                }
            }
        }


#if BPROFILE
        /// <summary>Return additional info identifying the task to the BProfiler.</summary>
        public override string BProfileAdditionalInfo 
        {
            get { return FullPackageName; }
        }
#endif

        public bool IsFramework2
        {
            get
            {
                // Don't get Framework version if already done so
                if (_frameworkVersion != null)
                    return _frameworkVersion == "2";
                else
                {
                    // IsFramework2 may be called by property setters prior to Execute()
                    // Project checks Framework version. That may be sufficient. We keep this function around
                    // just in case we need it.
                    //PackageFrameworkVersion();
                    _frameworkVersion = Project.Properties["package.frameworkversion"];
                    return _frameworkVersion == "2";
                }
            }
        }

        protected override void ExecuteTask() {
            
            string msg = null;

            if (_targetVersion == null)
            {
                _targetVersion = Path.GetFileName(Project.BaseDirectory);
            }

            // check for multiple packages
            if(Project.Properties.Contains(PackageProperties.PackageNamePropertyName) && 
                Project.Properties.Contains(PackageProperties.PackageTargetVersionPropertyName)) {
                msg = String.Format("Duplicate package {0}-{1} detected.  The <package> task can only be executed once per package build.", PackageName, TargetVersion);
                throw new BuildException(msg);
            }

            // Error checking for the name and targetversion attributes in <package>
            // trim off whitespace at both ends
            PackageName = PackageName.Trim();
            TargetVersion = TargetVersion.Trim();

            if ( PackageName == String.Empty ) 
            {
                msg = "[build file] The name attribute in <package> cannot be empty.";
                throw new BuildException(msg, Location);
            }
            if ( TargetVersion == String.Empty ) 
            {
                msg = "[build file] The targetversion attribute in <package> cannot be empty.";
                throw new BuildException(msg, Location);
            }

            if (!NAntUtil.IsPackageNameValid(PackageName))
            {
                // Make sure we have a valid package name
                if (PackageName.IndexOf("-") != -1)
                {
                    throw new BuildException("A package name cannot have a '-' character present.", Location);
                }
                msg = String.Format("[build file] Package name {0} is invalid.", PackageName);
                throw new BuildException(msg);

            }
            if (!NAntUtil.IsPackageVersionStringValid(TargetVersion))
            {
                msg = String.Format("[build file] Package targetversion {0} is invalid.", TargetVersion);
                throw new BuildException(msg);
            }


            // Get the directory where this package is being built
            DirectoryInfo packageDirectory = new DirectoryInfo(Project.BaseDirectory);

            // Create a PackageMap and try to find the the directory containing the buildfile in it.
            // If we cannot find it make the version "private".
            // If we can find it make sure:
            // 1. The package is not sealed
            // 2. The package name specified in the buildfile matches the directory name.
            PackageCore.PackageMap packageMap = PackageCore.PackageMap.Instance;
            PackageCore.Release packageInfo = PackageCore.PackageMap.Instance.Releases.FindByNameAndDirectory(PackageName, packageDirectory.FullName);
            
            string packageVersion = null;
            string masterVer = PackageCore.PackageMap.Instance.GetMasterVersion(PackageName, Project);

            if (packageInfo == null) 
            {
                // can't find package in any of the package roots.
                packageVersion = TargetVersion;
                PackageCore.Package selfPackage = null;

                selfPackage = new PackageCore.Package(PackageName);

                if ( selfPackage != null )
                {
                    PackageCore.Release selfRelease = new PackageCore.Release(packageVersion, packageDirectory.FullName, selfPackage, flattened:false); 
                    selfPackage.Releases.Add(selfRelease);
                    PackageCore.PackageMap.Instance.Packages.TryAdd(PackageName, selfPackage);
                    packageInfo = packageMap.Releases.FindByName(selfPackage.Name);
                } 
            }
            else
            {
                if (PackageName != packageInfo.Package.Name)
                {
                    msg = String.Format("Package name defined in the package task <package name='{0}' ...  does not match package folder name: '{1}'.", PackageName, packageInfo.Package.Name);
                    if (PackageName.Equals(packageInfo.Package.Name, StringComparison.OrdinalIgnoreCase))
                    {
                        msg += " Difference is in the letters case only. Package names are case sensitive in Framework and case of the package name should match case of the directory name";
                    }

                    throw new BuildException(msg, Location);
                }

                if (packageInfo.IsFlattened && masterVer != null && masterVer == PackageCore.Release.Flattened)
                {
                    packageVersion = PackageCore.Release.Flattened;
                }
                else
                {
                    packageVersion = packageInfo.Version;
                }
            }
           
            // Determine my Framework version if not done so (by property setters)
            if (_frameworkVersion == null)
            {
                // Keep it around just in case				
                _frameworkVersion = Project.Properties["package.frameworkversion"];
            }
            if (_frameworkVersion == "2" && packageInfo != null)
            {
                // Check <buildable> because:
                // 1. When creating its singleton, PackageMap just gets a package's releases
                // 2. Release doesn't read Manifest.xml until its properties like Buildable are accssed
                // So force an access here.
                //
                // We don't want PackageMap to do unnecessary checking
            }
           
            // add package.* properties
            Project.Properties.AddReadOnly(PackageProperties.PackageNamePropertyName,          PackageName);
            Project.Properties.AddReadOnly(PackageProperties.PackageVersionPropertyName,       packageVersion);
            Project.Properties.AddReadOnly(PackageProperties.PackageTargetVersionPropertyName, TargetVersion);
            Project.Properties.AddReadOnly(PackageProperties.PackageDirectoryPropertyName,     Project.BaseDirectory);

            //setup build folder properties
            string buildroot = PackageCore.PackageMap.Instance.GetBuildGroupRoot(PackageName, Project);
            if (buildroot != null && _frameworkVersion == "2")
            {
                //set package.builddir to: <buildroot>/<package name>/<package version>
                Project.Properties.AddReadOnly(PackageProperties.PackageBuildDirectoryPropertyName, 
                    PathNormalizer.Normalize(Path.Combine(buildroot, String.Format("{0}/{1}", 
                        PackageName, packageVersion))));
                
                //set build.dir to: <package.builddir>/build
                // This property, ${build.dir} is legacy from FW1 and should not be used -
                // use PackageBuildDirectoryPropertyName ${package.builddir} instead
                Project.Properties.AddReadOnly(PackageProperties.BuildDirectoryPropertyName,       
                    PathNormalizer.Normalize(Path.Combine(Project.Properties[PackageProperties.PackageBuildDirectoryPropertyName], "build")));

                //set package.<name>.buildroot to: <buildroot>
                Project.Properties.AddReadOnly("package." + PackageName + ".buildroot",
                    PathNormalizer.Normalize(buildroot));
            }
            else
            {
                //set package.builddir, eg. /eacore/1.00.00
                Project.Properties.AddReadOnly(PackageProperties.PackageBuildDirectoryPropertyName, 
                    Project.BaseDirectory);

                //set build.dir, eg. /eacore/1.00.00/build
                Project.Properties.AddReadOnly(PackageProperties.BuildDirectoryPropertyName,       
                    Path.Combine(Project.BaseDirectory, "build"));
            }

            // add package.name.* properties
            Project.Properties.AddReadOnly(String.Format("package.{0}.dir", PackageName), Project.BaseDirectory);
            Project.Properties.AddReadOnly(String.Format("package.{0}.version", PackageName), packageVersion);
            //set package.builddir
            Project.Properties.AddReadOnly(String.Format("package.{0}.builddir", PackageName), 
                Project.Properties[PackageProperties.PackageBuildDirectoryPropertyName]);

            // Add config package's version
            if (packageMap.ConfigPackageName != null && packageMap.ConfigPackageVersion != null && packageMap.ConfigPackageName != PackageName)
            {
                Project.Properties.AddReadOnly(String.Format("package.{0}.version", packageMap.ConfigPackageName),
                    packageMap.ConfigPackageVersion);
            }


            AddImplicitFrameworkDependency();

            if (_frameworkVersion == "1")
            {
                string[] excludedTargetNames = _excludeTargets.Split(' ');

                ArrayList standardTargetsXml = new ArrayList();
                if (!_noStandardTargets && Array.IndexOf(excludedTargetNames, "cleanall") == -1) 
                {
                    const string target = @"
                    <project>
                        <target name='cleanall' description='Clean all configurations'>
                            <property name='builddir' value='${build.dir}'/>
                            <delete dir='${builddir}' failonerror='false'/>
                        </target>
                    </project>";

                    standardTargetsXml.Add(target);
                }

                if (!_noStandardTargets && Array.IndexOf(excludedTargetNames, "clean") == -1) 
                {
                    const string target = @"
                    <project>
                        <target name='clean' description='Clean current configuration'>
                            <delete dir='${build.dir}/${package.config}' failonerror='false'/>
                        </target>
                    </project>";

                    standardTargetsXml.Add(target);
                }

                if (!_noStandardTargets && Array.IndexOf(excludedTargetNames, "publish") == -1) 
                {
                    const string target = @"
                    <project>
                        <target name='publish' description='Copy files from build/stable dir to package dir'>
                            <!-- push all the files from stable folder to package folder clearing any readonly flags -->
                            <attrib readonly='false'>
                                <fileset basedir='${package.builddir}/build/stable'>
                                    <includes name='**'/>
                                </fileset>
                            </attrib>

                            <copy todir='${package.dir}'>
                                <fileset basedir='${package.builddir}/build/stable'>
                                    <includes name='**'/>
                                </fileset>
                            </copy>
                        </target>
                    </project>";

                    standardTargetsXml.Add(target);
                }

                if (!_noStandardTargets && Array.IndexOf(excludedTargetNames, "configure") == -1) 
                {        
                    const string target = @"
                    <project>
                        <target name='configure' description='Generate a Visual Studio.NET 2002 project for the package'>
                            <!-- generate a visual studio project -->
                            <generateproject>
                                <fileset sort='true'>
                                    <!-- includes -->
                                    <includes name='**'/>

                                    <!-- excludes -->
                                    <excludes name='build\**'/>
                                    <excludes name='bin\**'/>
                                    <excludes name='obj\**'/>
                                    <excludes name='*.csproj*'/>
                                    <excludes name='*.vcproj*'/>
                                    <excludes name='*.sln'/>
                                    <excludes name='*.suo'/>
                                    <excludes name='*.ncb'/>
                                </fileset>
                            </generateproject>
                        </target>
                    </project>";

                    standardTargetsXml.Add(target);
                }

                if (!_noStandardTargets && Array.IndexOf(excludedTargetNames, "configure70") == -1) 
                {        
                    const string target = @"
                    <project>
                        <target name='configure70' description='Generate a Visual Studio.NET 2002 project for the package'>
                            <!-- generate a visual studio project -->
                            <generateproject version='7.0'>
                                <fileset sort='true'>
                                    <!-- includes -->
                                    <includes name='**'/>

                                    <!-- excludes -->
                                    <excludes name='build\**'/>
                                    <excludes name='bin\**'/>
                                    <excludes name='obj\**'/>
                                    <excludes name='*.csproj*'/>
                                    <excludes name='*.vcproj*'/>
                                    <excludes name='*.sln'/>
                                    <excludes name='*.suo'/>
                                    <excludes name='*.ncb'/>
                                </fileset>
                            </generateproject>
                        </target>
                    </project>";

                    standardTargetsXml.Add(target);
                }

                if (!_noStandardTargets && Array.IndexOf(excludedTargetNames, "configure71") == -1) 
                {        
                    const string target = @"
                    <project>
                        <target name='configure71' description='Generate a Visual Studio.NET 2002 project for the package'>
                            <!-- generate a visual studio project -->
                            <generateproject version='7.1' >
                                <fileset sort='true'>
                                    <!-- includes -->
                                    <includes name='**'/>

                                    <!-- excludes -->
                                    <excludes name='build\**'/>
                                    <excludes name='bin\**'/>
                                    <excludes name='obj\**'/>
                                    <excludes name='*.csproj*'/>
                                    <excludes name='*.vcproj*'/>
                                    <excludes name='*.sln'/>
                                    <excludes name='*.suo'/>
                                    <excludes name='*.ncb'/>
                                </fileset>
                            </generateproject>
                        </target>
                    </project>";

                    standardTargetsXml.Add(target);
                }

                if (!_noStandardTargets && Array.IndexOf(excludedTargetNames, "package") == -1) 
                {        
                
                    if (UseDefaultFileSet == false && FromFileSet != null) 
                    {
                        throw new BuildException("The 'fileset' can not be defined together with the 'fromfileset' attribute.", Location);
                    }
                
                    string target;
                    if (UseDefaultFileSet == true)
                    {
                        if (FromFileSet != null)
                        {
                            // use the fromfileset attribute
                            target = @"
                            <project>
                                <target name='package' description='Create a package archive for distribution'>
                                    <zip zipfile='${package.builddir}/build/${package.name}-${package.targetversion}.zip' zipentrydir='${package.name}\${package.targetversion}'>
                                        <fileset fromfileset='{0}' />
                                    </zip>
                                </target>
                            </project>";

                            target = target.Replace("{0}", FromFileSet);
                        }
                        else
                        {
                            // use the default
                            target = @"
                            <project>
                                <target name='package' description='Create a package archive for distribution'>
                                    <zip zipfile='${package.builddir}/build/${package.name}-${package.targetversion}.zip' zipentrydir='${package.name}\${package.targetversion}'>
                                        <fileset basedir='${package.dir}'>
                                            <!-- includes -->
                                            <includes name='**'/>

                                            <!-- excludes -->
                                            <excludes name='build\**'/>
                                            <excludes name='*.csproj*'/>
                                            <excludes name='*.vcproj*'/>
                                            <excludes name='*.sln'/>
                                            <excludes name='*.suo'/>
                                            <excludes name='*.ncb'/>
                                        </fileset>
                                    </zip>
                                </target>
                            </project>";
                        }
                    }
                    else
                    {
                        // use the fileset
                        target = @"
                        <project>
                            <target name='package' description='Create a package archive for distribution'>
                                <zip zipfile='${package.builddir}/build/${package.name}-${package.targetversion}.zip' zipentrydir='${package.name}\${package.targetversion}'>
                                    <fileset>
                                        {0}
                                    </fileset>
                                </zip>
                            </target>
                        </project>";

                        StringBuilder sb = new StringBuilder();
                        foreach(FileItem fileItem in FileSet.FileItems) 
                        {
                            sb.AppendFormat("<includes name='{0}' />", fileItem.FileName);
                        }
                        target = target.Replace("{0}", sb.ToString());
                    }

                    standardTargetsXml.Add(target);
                }

                foreach (string targetXml in standardTargetsXml) 
                {
                    try 
                    {
                        XmlTextReader reader = new XmlTextReader(new StringReader(targetXml));
                        XmlDocument doc = new XmlDocument();
                        doc.Load(reader);
                        Project.IncludeBuildFileDocument(doc, Location);
                    } 
                    catch (Exception e) 
                    {
                        throw new BuildException("Error adding default tasks.", Location, e);
                    }
                } 
            }

            var configDir = SetPackageConfigsProperty();

            IncludeConfiguration(configDir, _frameworkVersion, packageInfo);
            IncludeInitializeScript();

            // Verify that if masterconfig version doesn't match the current version.
            // WE do this check after configuration package is loaded because in case masterconfig file has version exceptions,
            // some properties in the config package might affect the version
            if (masterVer != null && masterVer != PackageVersion)
            {
                // Re-evaluate masterversion, and if there is still no match - write a warning
                if (PackageCore.PackageMap.Instance.GetMasterVersion(PackageName, Project) != PackageVersion)
                {
                    Log.Warning.WriteLine(LogPrefix +
                        "WARNING: Package {0}'s current version ({1}) doesn't match masterconfig's version ({2}).",
                        PackageName, PackageVersion, masterVer);
                }
            }
        }


        /// <summary>Make an implicit dependency on the Framework package we 
        /// are using to build with.</summary>
        void AddImplicitFrameworkDependency() {
            PackageCore.Release frameworkRelease = PackageCore.PackageMap.Instance.GetFrameworkRelease();

            if (frameworkRelease == null) 
            {
                throw new Exception("Cannot determine Framework release.");
            }

            string requiredVersion = PackageCore.PackageMap.Instance.GetMasterVersion("Framework", Project);

            if (requiredVersion != null && frameworkRelease.Version != requiredVersion)
            {
                string msg = String.Format("Master config file specified Framework-{0}, but we're using Framework-{1}.", requiredVersion, frameworkRelease.Version);
                throw new BuildException(msg, Location);
            }

            DependentTask.AddPackageProperties(Project, frameworkRelease);
            Project.BuildGraph().GetOrAddPackage(frameworkRelease.Name, frameworkRelease.Version, PathString.MakeNormalized(frameworkRelease.Path), Project.Properties[PackageProperties.ConfigNameProperty] ?? PackageCore.PackageMap.Instance.DefaultConfig, frameworkRelease.FrameworkVersion);
        }

        void AddPackageConfigs(string configDirPath, List<string> configs, IList<string> items, bool exclude)
        {
            foreach (string fileName in _configFilesCache.GetOrAdd(configDirPath, cdp => Directory.GetFiles(cdp))) 
            {
                if (Path.GetExtension(fileName).ToLower() != PackageProperties.ConfigFileExtension)
                    continue;
                        
                bool found = false;
                string config = Path.GetFileNameWithoutExtension(fileName);
                foreach (string item in items)
                {
                    int index = item.IndexOf('*');
                    if (index > 0)
                    {
                        if (config.StartsWith(item.Substring(0, index), StringComparison.OrdinalIgnoreCase))
                        {
                            found = true;
                            break;
                        }
                    }
                    else
                    {
                        if (String.Equals(config, item, StringComparison.OrdinalIgnoreCase))
                        {
                            found = true;
                            break;
                        }
                    }
                }
                if (exclude)
                {
                    if (!found)
                    {
                        if (!configs.Contains(config))
                        {
                            configs.Add(config);
                        }
                    }
                }
                else
                {
                    if (found)
                    {
                        if (!configs.Contains(config))
                        {
                            configs.Add(config);
                        }
                    }
                }
            }
        }

        string SetPackageConfigsProperty()
        {
            // get a list of all the configuration names
            List<string> configs = new List<string>();
            string configDirectoryPath = null;
            if (IsFramework2)
            {
                if (PackageCore.PackageMap.Instance.ConfigDir != null)
                {
                    configDirectoryPath = Project.GetFullPath(PackageCore.PackageMap.Instance.ConfigDir);
                    Log.Info.WriteLine(LogPrefix + " '{0}' is Framework 2, using configdir {1}.", _packageName, configDirectoryPath);
                }
                else
                {
                    configDirectoryPath = Project.GetFullPath(ConfigDirectoryName);

                    if (String.IsNullOrEmpty(PackageCore.PackageMap.Instance.MasterConfigFilePath))
                    {
                        Log.Warning.WriteLine(LogPrefix + "Masterconfig file is not defined for Framework 2 package '{0}'.", _packageName);
                    }
                    else if (String.IsNullOrEmpty(PackageCore.PackageMap.Instance.ConfigPackageName))
                    {
                        Log.Warning.WriteLine(LogPrefix + "Configuration package is not defined for Framework 2 package '{0}'.", _packageName);
                    }
                    else
                    {
                        Log.Warning.WriteLine(LogPrefix + "Config directory is not defined for Framework 2 package '{0}'.", _packageName);
                    }
                    Log.Info.WriteLine(LogPrefix + " Framework 2 package {0} is using Framework 1 style configdir {0}.", _packageName, configDirectoryPath);
                }
            }
            else
            {
                configDirectoryPath = Project.GetFullPath(ConfigDirectoryName);
                Log.Info.WriteLine(LogPrefix + " '{0}' is not Framework 2 buildable, so using configdir {1}.", _packageName, configDirectoryPath);
            }

            if (Directory.Exists(configDirectoryPath))
            {
                IList<string> excludes = PackageCore.PackageMap.Instance.ConfigExcludes;
                IList<string> includes = PackageCore.PackageMap.Instance.ConfigIncludes;

                if (_frameworkVersion == "2" && (excludes.Count > 0 || includes.Count > 0))
                {
                    if (!String.IsNullOrEmpty(GetConfigLoaderFile(configDirectoryPath)) && includes.Count > 0)
                    {
                        // Get all platform configs
                        foreach (string platform in GetPlatformsFromIncludes(includes))
                        {
                            // For each platform scan platform config directory:
                            string platformConfigDirectory = GetPlatformConfigDirectory(platform);
                            if (!String.IsNullOrEmpty(platformConfigDirectory) && Directory.Exists(platformConfigDirectory))
                            {
                                AddPackageConfigs(platformConfigDirectory, configs, includes, false);
                            }
                        }
                    }
                    // Add Standard FW2 configs
                    if (excludes.Count> 0)
                        AddPackageConfigs(configDirectoryPath, configs, excludes, true);
                    else
                        AddPackageConfigs(configDirectoryPath, configs, includes, false);
                }
                else
                {
                    // Get configurations for the current platform at least if platform_config is present.
                    if (_frameworkVersion == "2" && !String.IsNullOrEmpty(GetConfigLoaderFile(configDirectoryPath)))
                    {
                        foreach (string platform in GetAllPlatforms())
                        {
                            string platformConfigDirectory = GetPlatformConfigDirectory(platform);
                            if (!String.IsNullOrEmpty(platformConfigDirectory) && Directory.Exists(platformConfigDirectory))
                            {
                                foreach (string fileName in Directory.GetFiles(platformConfigDirectory))
                                {
                                    if (!fileName.ToLower().EndsWith(PackageProperties.ConfigFileExtension.ToLower()))
                                        continue;
                                    configs.Add(Path.GetFileNameWithoutExtension(fileName));
                                }
                            }
                        }
                    }
                    // Add local directory:
                    foreach (string fileName in Directory.GetFiles(configDirectoryPath))
                    {
                        if (!fileName.ToLower().EndsWith(PackageProperties.ConfigFileExtension.ToLower()))
                            continue;
                        string config = Path.GetFileNameWithoutExtension(fileName);
                        if(!configs.Contains(config))
                            configs.Add(config);
                    }
                }
            }

            var activeconfig = Project.Properties[PackageProperties.PackageConfigPropertyName];

            

            if (!String.IsNullOrEmpty(activeconfig) && !configs.Contains(activeconfig))
            {
                Log.Warning.WriteLine(LogPrefix +"List of all included configurations does not contain active configuration defined by property {0}='{1}'. Adding active configuration to the list of included configurations.",PackageProperties.PackageConfigPropertyName, activeconfig);
                configs.Insert(0,activeconfig);
            }

            // create a package.configs property with a list of all the config names delimited by a space
            Project.Properties[PackageProperties.PackageConfigsPropertyName] = configs.ToString(" ");
            Project.Properties.UpdateReadOnly(PackageProperties.PackageAllConfigsPropertyName, configs.ToString(" "));
            //Console.WriteLine("- configs " + configsPropertyValue);

            return configDirectoryPath;
        }

        string GetPlatformConfigDirectory(string platform)
        {
            string configPath = String.Empty;

            string packageName = platform + PackageProperties.PlatformConfigPackagePostfix;

            if (PackageCore.PackageMap.Instance.ConfigPackageName != packageName)
            {
                PackageCore.Release platformConfigInfo = PackageCore.PackageMap.Instance.Releases.FindByName(packageName, true);
                if (platformConfigInfo != null && !String.IsNullOrEmpty(platformConfigInfo.Path))
                {

                    configPath = PathNormalizer.Normalize(Path.Combine(platformConfigInfo.Path, "config"), true);
                }
            }
            return configPath;
        }

        List<string> GetAllPlatforms()
        {
            List<string> allPlatformNames = new List<string>();

            foreach (string packageName in PackageCore.PackageMap.Instance.MasterPackages)
            {
                if (packageName != PackageCore.PackageMap.Instance.ConfigPackageName)
                {
                    if (packageName.EndsWith(PackageProperties.PlatformConfigPackagePostfix, StringComparison.OrdinalIgnoreCase))
                    {
                        string platform = packageName.Substring(0, packageName.Length - PackageProperties.PlatformConfigPackagePostfix.Length);
                        if (!allPlatformNames.Contains(platform))
                        {
                            allPlatformNames.Add(platform);
                        }
                    }
                }
            }
            return allPlatformNames;
        }

        List<string> GetPlatformsFromIncludes(IList<string> includes)
        {
            List<string> platformNames = new List<string>();
            List<string> allPlatforms = GetAllPlatforms();
            foreach (string item in includes)
            {
                List<string> found = GetPlatformNameFromPattern(item, allPlatforms);
                foreach (string platform in found)
                {
                    if (!String.IsNullOrEmpty(platform) && !platformNames.Contains(platform))
                    {
                        platformNames.Add(platform);
                    }
                }
            }
            return platformNames;
        }

        List<string> GetPlatformNameFromPattern(string pattern, List<string> allPlatforms)
        {
            List<string> platforms = new List<string>();
            if (pattern != null)
            {
                int index = pattern.IndexOf('-');
                if (index > 0)
                {
                    platforms.Add(pattern.Substring(0, index));
                }
                else if (allPlatforms != null && pattern.EndsWith("*"))
                {
                    pattern = pattern.Substring(0, pattern.Length - 1);
                    foreach (string pl in allPlatforms)
                    {
                        if (pl.StartsWith(pattern))
                        {
                            platforms.Add(pl);
                        }
                    }
                }
            }
            return platforms;
        }


        private string GetConfigLoaderFile(string configDirectory)
        {
            PathString platformLoader = PathString.MakeNormalized(Path.Combine(configDirectory, "platformloader/load.xml"));
            string configMode = Project.Properties["config-platform-mode"];

            if (!(configMode != null && configMode.Equals("legacy", StringComparison.OrdinalIgnoreCase)))
            {
                if (File.Exists(platformLoader.Path))
                {
                    return platformLoader.Path;
                }
            }
            return string.Empty;
        }

        private string GetConfigFilePath(string configDir, string configName, out string filetype)
        {
            filetype = "platform loader";
            string configFilePath = GetConfigLoaderFile(configDir);

            if(String.IsNullOrEmpty(configFilePath))
            {
                configFilePath = Path.Combine(PackageCore.PackageMap.Instance.ConfigDir,
                                 Path.ChangeExtension(configName, PackageProperties.ConfigFileExtension));

                filetype = "configuration";
            }
            return configFilePath;
        }

        void IncludeConfiguration(string configDir, string frameworkVersion, PackageCore.Release packageInfo) 
        {
            string configName = Project.Properties[PackageProperties.ConfigNameProperty];
            if (configName == null) {
                if (IsFramework2)
                    configName = PackageCore.PackageMap.Instance.DefaultConfig;
                else
                    configName = DefaultConfiguration;
                Project.Properties.AddReadOnly(PackageProperties.ConfigNameProperty, configName);
            }

            StringBuilder targetString = new StringBuilder();
            
            if (Project.BuildTargetNames.Count > 0 )
            {
                foreach (string name in Project.BuildTargetNames) 
                {
                    targetString.AppendFormat(" {0}", name);
                }
            }
            else
            {
                targetString.AppendFormat(" {0}", Project.DefaultTargetName);
            }

            if (!String.IsNullOrEmpty(configName))
            {

                // Verify input path values:
                NAnt.Core.Functions.PathFunctions.PathVerifyValid(PackageCore.PackageMap.Instance.ConfigDir, "PackageMap.Instance.ConfigDir");
                NAnt.Core.Functions.PathFunctions.PathVerifyValid(configName, "configName");
                NAnt.Core.Functions.PathFunctions.PathVerifyValid(PackageProperties.ConfigFileExtension, "PackageProperties.ConfigFileExtension");

                string configFilePath = null;
                string confifFileType = "configuration";
                // get path to config file
                if (frameworkVersion == "2" && PackageCore.PackageMap.Instance.ConfigDir != null)
                {
                    configFilePath = GetConfigFilePath(configDir, configName, out confifFileType);

                    configFilePath = configFilePath.Replace('\\', Path.DirectorySeparatorChar);
                    configFilePath = configFilePath.Replace('/', Path.DirectorySeparatorChar);
                }
                else
                {
                    configFilePath = Project.GetFullPath(Path.Combine(ConfigDirectoryName,
                        Path.ChangeExtension(configName, PackageProperties.ConfigFileExtension)));

                    confifFileType = "configuration";
                }
                if (!File.Exists(configFilePath))
                {
                    string msg = String.Format("Cannot find {0} file '{1}' for configuration '{2}'.", confifFileType, configFilePath, configName);

                    if (IsFramework2)
                    {
                        var platformLoader = PathString.MakeNormalized(Path.Combine(configDir, "platformloader/load.xml"));
                        if (!File.Exists(platformLoader.Path))
                        {
                            msg += Environment.NewLine + 
                            String.Format("NOTE. platform loader file: {0} does not exist in config package, which means configurations specified in platform config packages (i.e. playbook_config) will not be found or loaded.", platformLoader.Path);
                        }
                    }

                    throw new BuildException(msg, Location);
                }

                Log.Status.WriteLine(LogPrefix + "{0} ({1}) {2}", FullPackageName, configName, targetString.ToString());
                Log.Info.WriteLine(LogPrefix + "{0} file '{1}'", confifFileType, configFilePath);

                // set the package.config property

                Project.Properties.AddReadOnly(PackageProperties.PackageConfigPropertyName, configName);

                IPackage package;

                if (!Project.TrySetPackage(PackageName, PackageVersion, PathString.MakeNormalized(Project.Properties[PackageProperties.PackageDirectoryPropertyName]), Project.Properties[PackageProperties.PackageConfigPropertyName], packageInfo.FrameworkVersion, out package))
                {
                    Project.Log.Warning.WriteLine("<package> task is called twice for {0}-{1} [{2}] in the build file, or Framework internal error", PackageName, PackageVersion, Project.Properties[PackageProperties.PackageConfigPropertyName]);
                }
                else
                {
                    package.SetType(packageInfo.Buildable ? Package.Buildable : BitMask.None);
                    package.Project = Project;
                }

                // use the <include> task to include the "config" script
                IncludeTask includeTask = new IncludeTask();
                includeTask.Project = Project;
                includeTask.Verbose = false;
                includeTask.FileName = configFilePath;
                includeTask.Execute();

                // Set package properties that are possibly defined in the configuration package.
                {
                    if (package.PackageConfigBuildDir == null)
                    {
                        if (Properties.Contains("package.configbuilddir"))
                        {
                            package.PackageConfigBuildDir = PathString.MakeNormalized(Properties["package.configbuilddir"]);
                        }
                        else
                        {
                            package.PackageConfigBuildDir = package.PackageBuildDir;
                        }
                    }
                }

            } 
            else {
                // no configuration used by package in this build
                Log.Status.WriteLine(LogPrefix + "{0} {1}", FullPackageName, targetString.ToString());
                Project.Properties.AddReadOnly(PackageProperties.PackageConfigPropertyName, "");
            }

            // add config dir property
            Project.Properties.AddReadOnly(PackageProperties.PackageConfigDirPropertyName, ConfigDirectoryName);
            
        }

        void IncludeInitializeScript() 
        {
            if (InitializeSelf) {

                // use the <include> task to include the "Initialize" script if it exists
                string initScriptPath = Path.Combine(Project.BaseDirectory, Path.Combine("scripts", "Initialize.xml"));
                if (!File.Exists(initScriptPath))
                {
                    // Some packages may use lower case, which is important on Linux
                    initScriptPath = Path.Combine(Project.BaseDirectory, Path.Combine("scripts", "initialize.xml"));
                }

                if (File.Exists(initScriptPath))
                {
                    Log.Info.WriteLine(LogPrefix + "initializeself='true': include '{0}'", initScriptPath);

                    IncludeTask includeTask = new IncludeTask();                    
                    includeTask.Project = Project;
                    includeTask.FileName = initScriptPath;
                    includeTask.Execute();
                }
                else
                {
                    if (!String.IsNullOrEmpty(FullPackageName) &&
                        !( FullPackageName.Equals("eaconfig", StringComparison.OrdinalIgnoreCase)
                        || FullPackageName.Equals("rwconfig", StringComparison.OrdinalIgnoreCase)
                        || FullPackageName.Equals("Framework", StringComparison.OrdinalIgnoreCase)))
                    {
                        Log.Status.WriteLine(LogPrefix + " NOTE: Initialize.xml script not found in package '{0}' [{1}].", FullPackageName, initScriptPath);
                    }
                }
            }
        }
    }
}

