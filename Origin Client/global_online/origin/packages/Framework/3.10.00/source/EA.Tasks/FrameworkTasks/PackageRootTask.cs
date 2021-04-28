// Copyright (C) 2001-2005 Electronic Arts
//

using System;
using System.Collections;
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

using NAnt.Core.PackageCore;

namespace EA.FrameworkTasks {
    
    /// <summary>
    /// Declares a directory as a new package root.  This function is DEPRECATED for
    /// the entire build tree if the top level project is marked as Framework 2 
    /// in its manifest.xml.
    /// </summary>
    /// <remarks>
    /// <para>
    /// Packages are searched for in the following order of sources: 
    /// <list type="bullet">
    ///     <item>Default package root (initially set by searching from the current directory; will coincide with framework root below if you have only one package root)</item>
    ///     <item>Other package roots (excluding the default and framework roots) specified via &lt;packageroot> AS WELL AS optional "package directories" specified by &lt;packageDirs> contained in known packageroot.xml files (i.e. any packageroot.xml belonging to one of the following package root types:  default, framework, or &lt;packageroot>). </item>
    ///     <item>Framework package root (package root which contains the currently executing framework package and defines where packages get downloaded/installed).</item>
    /// </list>
    /// If the same package release is found in two package roots, the first one found will be used.
    /// </para>
    /// <para>
    /// To define a folder as a package root it must contain a valid 
    /// <see href="../../../Resources/PackageRoot.xml.txt">PackageRoot.xml</see> file.
    /// </para>
    /// </remarks>
	// <include file='Examples/Package/Package.example' path='example'/>
    [TaskName("packageroot")]
    public class PackageRootTask : Task 
    {
        string _packageRoot = null;
        bool _default = false;

        /// <summary>
        /// The directory to set as the new package root.
        /// </summary>
        [TaskAttribute("dir", Required=true)]
        public string PackageRoot {
            get { return _packageRoot; }
            set { _packageRoot = value; }
        }

        /// <summary>
        /// If true the directory specified by the 'dir' attribute will be set
        /// as the default package root. Default is "false".
        /// </summary>
        [TaskAttribute("default", Required=false)]
        public bool Default {
            get { return _default; }
            set { _default = value; }
        }


#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return PackageRoot; }
		}
#endif

        protected override void ExecuteTask() 
        {
			if (PackageMap.Instance.TopLevelProjectManifest.FrameworkVersion == FrameworkVersions.Framework2 ||
				Project.Properties["package.frameworkversion"] == "2")
			{
				//deprecated
				throw new BuildException("<PackageRoot> tasks being deprecated/ignored since current or top level project is marked as Framework 2.", Location);
			}
			else
			{
				try 
				{
					// Check for strings like "c:"
					if (_packageRoot.Length == 2 && _packageRoot[1] == ':')
					{
						// Path.GetFullPath() returns current directory instead of "c:\" for "c:", so
						// append the missing separator.
						_packageRoot += Path.DirectorySeparatorChar;
					}

					PackageRoot = Project.GetFullPath(PackageRoot);
					DirectoryInfo packageRoot = new DirectoryInfo(PackageRoot);

					if (!packageRoot.Exists) 
					{
						string msg = String.Format("Directory does not exist.");
						throw new BuildException(msg, this.Location, new Exception(packageRoot.FullName));
					}

					if (!PackageMap.Instance.IsPackageRoot(packageRoot, Project.Log)) 
					{
						string msg = String.Format("Directory is not a valid package root.");
						throw new BuildException(msg, this.Location, new Exception(packageRoot.FullName));
					}

					if (0 == PackageMap.Instance.PackageRoots.Add(PathString.MakeNormalized(packageRoot.FullName), PackageRootList.DefaultMinLevel, PackageRootList.DefaultMaxLevel, _default ? PackageRootList.RootType.DefaultRoot : PackageRootList.RootType.Undefined)) 
					{
						throw new Exception(packageRoot.FullName); // should not happen
					}
				}
				catch(BuildException) 
				{
					throw;
				}
				catch(System.Exception e) 
				{
					throw new BuildException("Failed to add package root.", this.Location, e);
				}
			}
        }
    }
}
