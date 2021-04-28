// (c) 2002-2003 Electronic Arts Inc.

using System;
using System.Data;
using System.IO;
using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using NAnt.Core.PackageCore;

namespace EA.FrameworkTasks {
    
    /// <summary>For a Framework 1 package, make this package require another package.
    /// <b>This task is DEPRECATED for a Framework 2 package.</b></summary>
    /// <remarks>
    ///   <para>
    ///   Provide an expression inside the tag elements using the keyword <c>version</c> 
    ///   to represent the package version.  The task will start with the highest version and
    ///   work down stopping when it finds the first match.  The sort order uses a method where
    ///   it has a basic understanding of numbers.  As long as your version system makes reasonable
    ///   sense it should work.  The same algorithm is used on the Package Server to sort releases.
    ///   </para><para>
    ///   This task reaches into the named package's <b>scripts</b>
    ///   folder and executes the <b>Initialize.xml</b> (as a NAnt <b>include</b>).
    ///   This file is optional and does not need to exist.
    ///   </para><para>
    ///   <c>Intialize.xml</c> is the mechanism to make any property
    ///   from your package available to users of your package's
    ///   contents.
    ///   </para><para>
    ///   By default, NAnt will create the following properties:
    ///   </para><para>
    ///   <list type="table">
    ///   <item><term>package.all</term><description>List of descriptive names of all the packages this build file depends on.</description></item>
    ///   <item><term>package.<i>name</i>.dir</term><description>Base directory f package <i>name</i>.</description></item>
    ///   <item><term>package.<i>name</i>.version</term><description>Specific version number of package <i>name</i>.</description></item>
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
    /// <include file='Examples/Requires/Requires.example' path='example'/>
    /// <include file='Examples/Requires/Impossible.example' path='example'/>
    [TaskName("requires")]
    public class RequiresTask : Task, IDependentTask {

        string  _package;
        string  _expression;
		string  _version = null;
        bool    _onDemand = true;
        string  _packageConfig = null;
        bool    _initializeScript = true;
		bool    _GetDependents= true;

        /// <summary>The name of the required package without a version.</summary>
        [TaskAttribute("package", Required=true)]
        [StringValidator(Trim=true)]
        public string PackageName {
            get { return _package; }
            set { _package = value; }
        }

		/// <summary>The version for which the specified expression evaluates too.</summary>
		/// <returns>The matching version number, or null if none found.</returns>
		public string PackageVersion {
			get { 
				if (_version == null) {
					_version = FindMatchingVersion();
				}
				return _version; 
			}
		}

		/// <summary>If true the package will be automatically downloaded from the package server. Default is true.</summary>
        [TaskAttribute("ondemand", Required=false)]
        public bool OnDemand {
            get { return _onDemand; }
            set { _onDemand = value; }	
        }

        /// <summary>If false the execution of the Initialize.xml script will be suppressed. Default is true.</summary>
        [TaskAttribute("initialize", Required=false)]
        public bool InitializeScript {
            get { return _initializeScript; }
            set { _initializeScript = value; }
        }

        /// <summary>The value of the config property to use when initializing this package.</summary>
        [TaskAttribute("config", Required=false)]
        public string PackageConfig {
            get { return _packageConfig; }
            set { _packageConfig = value; }
        }

		/// <summary>Setting this to false prevents the installation of the package release's "Required Packages", only as specified on the release's web page on packages.ea.com.  Default is true.</summary>
		[TaskAttribute("getdependents", Required=false)]
		public bool GetDependents
		{
			get { return _GetDependents; }
			set { _GetDependents = value; }
		}


#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return PackageName; }
		}
#endif

        protected override void InitializeTask(System.Xml.XmlNode taskNode) {
            _expression = taskNode.InnerText;
            _expression = Project.Properties.ExpandProperties(_expression);
        }

        protected override void ExecuteTask() 
		{
			if (Project.Properties["package.frameworkversion"] == "2")
			{
				// fail since task is deprecated for fw 2 pkg
				string msg = String.Format("<requires> has been deprecated for Framework 2 packages (like {0}).", PackageName);
				throw new BuildException(msg, Location);
			}

            if (PackageVersion != null) 
			{
                // use the <dependent> task to do the actual work
                DependentTask dependentTask = new DependentTask();
                dependentTask.PackageName = PackageName;
                dependentTask.PackageVersion = PackageVersion;
                dependentTask.OnDemand = OnDemand;
                dependentTask.Project = Project;
                dependentTask.Verbose = Verbose;
                dependentTask.LogPrefix = LogPrefix;
                dependentTask.PackageConfig = PackageConfig;
                dependentTask.InitializeScript = InitializeScript;
				dependentTask.GetDependentDependents= GetDependents;

                dependentTask.Execute();

            } else {
                // fail if we can't find a match
                string msg = String.Format("No versions of package '{0}' matches expression '{1}'.",
                    PackageName, _expression);
                throw new BuildException(msg, Location);
            }
        }

        string FindMatchingVersion()
        {
            // Step 1: look in the package map for installed pacakges
            Package package = PackageMap.Instance.Packages.FindByName(PackageName);
            if (package != null)
            {
                // try each version until we find a match for the version expression
                foreach (Release release in package.Releases)
                {
                    if (IsValidVersion(release.Version))
                    {
                        return release.Version;
                    }
                }
            }

            // Step 2: look on the package server

            using (IPackageServer packageServer = PackageServerFactory.CreatePackageServer())
            {
                IList<Release> packageReleases;
                if (packageServer.TryGetPackageReleases(PackageName, out packageReleases))
                {
                    foreach (Release release in packageReleases)
                    {
                        if (IsValidVersion(release.Version))
                        {
                            return release.Version;
                        }
                    }
                }
            }
            // no matching version found anywhere
            return null;
        }

        bool IsValidVersion(string version) {
            // if the user doesn't give a version expression then use the first (higest) release
            if (_expression == null || _expression == "") {
                return true;
            }

            string expr = _expression.Replace("version", version);
            return Expression.Evaluate(expr);
        }
    }
}
