// Copyright (C) Electronic Arts Canada Inc. 2002.  All rights reserved.

using System;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Shared.Properties;

using NAnt.Core.PackageCore;
using System.Xml;
using System.IO;
using System.Collections.Specialized;

namespace EA.FrameworkTasks 
{
	/// <summary>
	/// Checks compatibility among all dependent packages (listed in &lt;masterversions> section
	/// of masterconfig.xml). <b>This task is a Framework 2 feature.</b>
	/// </summary>
	/// <remarks>
	///  <para>
	///  It uses package versions listed in master config file to check package compatibility. A Framework 2.0
	///  package may list its package compatibility, in its Manifest.xml file, using &lt;compatibility&gt;.
	///  </para>
	/// </remarks>
    [TaskName("checkdependents")]
	public class CheckDependentsTask : Task 
	{
		protected override void ExecuteTask() 
		{
			if (Project.Properties["package.frameworkversion"] == "1")
			{
				throw new BuildException("Calling <checkdependents> task within a Framework 1.x package.  This is a Framework 2.x feature.", Location);
			}
//IMTODO
            /*
                        if (PackageMap.Instance.HasMasterPackageVersions())
                        {
                            foreach (PackageXml px in PackageMap.Instance.MasterVersions)
                            {
                                string name = px.Name;
                                string version = px.Version;
                                Release info = PackageMap.Instance.Releases.FindByNameAndVersion(name, version);
                                if (info == null)
                                {
                                    string errorMsg = String.Format("Cannot find package '{0}-{1}' in package roots:\n'{2}'.", 
                                        name, version, PackageMap.Instance.PackageRoots.ToString());

                                    info = PackageMap.Instance.Releases.FindByName(name);
                                    if (info != null)
                                    {
                                        int count = 0;
                                        foreach (Release r in info.Package.Releases)
                                        {
                                            if (r.Path != String.Empty)
                                            {
                                                if (count == 0) errorMsg += String.Format("\n\nFollowing versions of the package {0} are found in package roots:", name);
                                                errorMsg += String.Format("\n  {0}-{1}: {2}", name, r.Version, r.Path);
                                                count++;
                                            }
                                        }
                                    }

                                    throw new BuildException(errorMsg, Location);
                                }
                                Compatibility comp = info.Compatibility;
                                if (comp != null)
                                {
                                    // 2. Check if each package has conflict
                                    StringCollection warnings = comp.CheckCompatibility(version);
                                    if (warnings.Count > 0)
                                    {
                                        foreach (string warning in warnings)
                                            Log.WriteLine(LogPrefix + warning);
                                    }
                                }
                            }
                        }
             **/
        }
 

	}
}