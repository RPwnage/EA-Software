// Copyright (C) 2001-2003 Electronic Arts
//
// Kosta Arvanitis (karvanitis@ea.com) 

using System;
using System.IO;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;

using NAnt.Core.PackageCore;

namespace EA.FrameworkTasks.Functions
{
    /// <summary>
    /// A collection of package functions.
    /// </summary>
    [FunctionClass("Package Functions")]
    public class PackageFunctions : FunctionClassBase
    {
		/// <summary>
		/// Gets the directory of the default package root, which only exists for
		/// a Framework 1 package.
		/// </summary>
		/// <param name="project"></param>
		/// <returns>The default package root or empty string if none exists.</returns>
		/// <include file='Examples\Package\PackageGetDefaultPackageRoot.example' path='example'/>        
		[Function()]
		public static string PackageGetDefaultPackageRoot(Project project) 
		{
            if (PackageMap.Instance.TopLevelProjectManifest.FrameworkVersion == FrameworkVersions.Framework1 &&
                PackageMap.Instance.PackageRoots.Count > 0)
            {
                return PackageMap.Instance.PackageRoots[0].Dir.FullName.ToString();
            }
			return String.Empty;
		}
		
		/// <summary>
		/// Gets the directory of the Framework package root, which is where missing
		/// dependent packages may get auto downloaded and installed to.  
        /// </summary>
        /// <param name="project"></param>
		/// <returns>For a Framework 1 package, 
		/// or a Framework 2 package lacking &lt;packageroot> in
		/// its masterconfig.xml, this returns the package root
		/// containing the executing Framework.  For a Framework 2 package with 1 or more
		/// &lt;packageroot> in its masterconfig.xml, this returns the FIRST &lt;packageroot>
		/// listed.</returns>
        /// <include file='Examples\Package\PackageGetFrameworkPackageRoot.example' path='example'/>        
        [Function()]
        public static string PackageGetFrameworkPackageRoot(Project project) {
            if (PackageMap.Instance.PackageRoots.HasFrameworkRoot)
                return PackageMap.Instance.PackageRoots.FrameworkRoot.FullName.ToString();
            return String.Empty;
        }

        /// <summary>
        /// Gets a delimited list of all the package roots.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="delimiter">The delimiter to use when seperating the package roots.</param>
        /// <returns>The default package root or empty string if none exists.</returns>
        /// <include file='Examples\Package\PackageGetPackageRoots.example' path='example'/>        
        [Function()]
        public static string PackageGetPackageRoots(Project project, string delimiter) {
            StringBuilder sb = new StringBuilder();
            for(int i =0; i < PackageMap.Instance.PackageRoots.Count; i++) {
                var root = PackageMap.Instance.PackageRoots[i];
                sb.Append(root.Dir.FullName);
                if (i < PackageMap.Instance.PackageRoots.Count - 1) {
                    sb.Append(delimiter.Trim());
                }
            }
            return sb.ToString();
        }

        /// <summary>
        /// Returns a version associated with a package within masterconfig.
        /// </summary>
        /// <param name="packageName"></param>
        /// <returns>The version of package being used in masterconfig.</returns>
        /// <remarks>If package cannot be found, an empty string is returned.</remarks>
        [Function()]
        public static string GetPackageVersion(Project project, string packageName)
        {
            return PackageMap.Instance.GetMasterVersion(packageName, project) ?? String.Empty;
        }

        /// <summary>
        /// Returns true/false whether package is buildable as defined within its manifest.xml file.
        /// </summary>
        /// <param name="packageName"></param>
        /// <returns>true/false dependent on whether package is buildable based on manifest.xml file.</returns>
        /// <remarks>If package cannot be found, false is returned.</remarks>
        [Function()]
        public static string IsPackageBuildable(Project project, string packageName)
        {
            Release info = PackageMap.Instance.Releases.FindByName(packageName);
            if (info != null)
            {
                if (info.Buildable)
                    return "true";
                else
                    return "false";
            }
            return "false";
        }

        /// <summary>
        /// Returns true/false whether package is autobuildclean within masterconfig.
        /// </summary>
        /// <param name="packageName"></param>
        /// <returns>true/false dependent on whether package is autobuildclean or not in masterconfig.</returns>
        /// <remarks>If package cannot be found, false is returned.</remarks>
        [Function()]
        public static string IsPackageAutoBuildClean(Project project, string packageName)
        {
            Release info = PackageMap.Instance.Releases.FindByName(packageName);
            if (info != null)
            {
                bool val = info.AutoBuildClean(project);
                if (val == true)
                    return "true";
                else
                    return "false";
            }
            return "false";
        }

        /// <summary>
        /// Returns true/false whether package is present in masterconfig.
        /// </summary>
        /// <param name="packageName"></param>
        /// <returns>true/false dependent on whether package is in masterconfig or not.</returns>
        /// <remarks>If package cannot be found, false is returned.</remarks>
        [Function()]
        public static string IsPackageInMasterconfig(Project project, string packageName)
        {
            if (PackageMap.Instance.GetMasterVersion(packageName, project) != null)
                return "true";
            else
                return "false";
        }

        /// <summary>
        /// Returns masterconfig grouptype name.
        /// </summary>
        /// <param name="packageName">name of the package</param>
        /// <returns>grouptype name or empty string if package is not under a grouptype element in masterconfig.</returns>
        /// <remarks>If package cannot be found, empty string is returned.</remarks>
        [Function()]
        public static string GetPackageMasterconfigGrouptypeName(Project project, string packageName)
        {
            Release info = PackageMap.Instance.Releases.FindByName(packageName);
            if (info != null)
            {
                return info.BuildGroupName(project);
            }
            return String.Empty;
        }

    }
}