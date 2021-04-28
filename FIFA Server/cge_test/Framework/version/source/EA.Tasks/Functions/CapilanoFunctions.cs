// (c) Electronic Arts. All rights reserved.

using System;
using System.IO;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;
using NAnt.Core.Functions;
using EA.Eaconfig.Backends;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig
{
	/// <summary>A collection of functions used by capilano targets</summary>
	[FunctionClass("Capilano Functions")]
	public class CapilanoFunctions : FunctionClassBase
	{
		/// <summary>
		/// Get the Capilano Layout directory for a specific module.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="platform">The platform - expected to be one of 'capilano' or 'xbsx'</param>
		/// <param name="module">The name of the module to get the layour dir for.</param>
		/// <param name="groupname">The group that the module is part of.</param>
		/// <returns>Capilano Layout directory for this module.</returns>
		[Function()]
		public static string GetModuleLayoutDir(Project project, string platform, string module, string groupname = null)
		{
			module = module.TrimWhiteSpace();

			if (String.IsNullOrEmpty(module))
			{
				string msg = String.Format("GetModuleLayoutDir(module) - module parameter is empty string.");
				throw new BuildException(msg);
			}

			var path = GetPropGroupValue(project, module, "layoutdir", null);
			if (path == null)
			{
				string platformName = String.Empty;
				if (platform == "capilano")
				{
					platformName = "Durango";
					if (project.Properties.GetBooleanPropertyOrDefault("gdk.enabled", false))
					{
						if (project.Properties.GetPropertyOrDefault("package.GDK.gdkVersion", "0").StrCompareVersions("191100") >= 0)
						{
							platformName = "Gaming.Xbox.XboxOne.x64";
						}
						else
						{
							platformName = "Gaming.Xbox.x64";
						}
					}
				}
				else if (platform == "xbsx")
				{
					platformName = "Gaming.Xbox.Scarlett.x64";
				}
				else
				{
					throw new ArgumentException("Cannot get module layout dir for unknown platform " + platform);
				}

				string packageName = project.GetPropertyOrFail("package.name");
				groupname = groupname ?? project.Properties["eaconfig.build.group"];
				if (!BuildGroups.TryParse(groupname, out BuildGroups buildGroup))
				{
					buildGroup = BuildGroups.runtime;
				}

				// default directory may be different if there are duplicate module names, check which one exists
				string projectDir = VisualStudioFunctions.GetModuleProjectDir(project, module, buildGroup, packageName);
				string projectName = VisualStudioFunctions.GetModuleProjectName(project, module, buildGroup, packageName);

				path = Path.Combine(projectDir, projectName, platformName, "Layout");
				if (Directory.Exists(path) == false)
				{
					projectName = VisualStudioFunctions.GetModuleProjectName(project, module + "-" + groupname, buildGroup, packageName);
					path = Path.Combine(projectDir, projectName, platformName, "Layout");
					if (Directory.Exists(path) == false)
					{

						projectName = VisualStudioFunctions.GetModuleProjectName(project, packageName + "-" + module + "-" + groupname, buildGroup, packageName);
						path = Path.Combine(projectDir, projectName, platformName, "Layout");
					}
				}
				path = Path.Combine(path, "Image", "Loose");
			}
			else
			{
				path = Path.Combine(project.Properties["package.builddir"], path, "Image", "Loose");
			}

			return PathFunctions.PathToWindows(project, PathNormalizer.Normalize(path, getFullPath: false));
		}

		private static string GetGroupName(Project project, string module)
		{
			var groupname = project.Properties["groupname"];
			if (String.IsNullOrEmpty(groupname) && project.Properties["eaconfig.build.group"] != null)
			{
				groupname = String.Format("{0}.{1}", project.ExpandProperties("${eaconfig.${eaconfig.build.group}.groupname}"), module);
			}
			return groupname;
		}

		private static string GetPropGroupValue(Project project, string module, string propName, string defaultVal)
		{
			var groupname = GetGroupName(project, module);

			string val = null;

			if (!String.IsNullOrEmpty(groupname) && project.Properties[groupname + "." + propName] != null)
			{
				val = project.Properties[groupname + "." + propName].TrimWhiteSpace();
			}

			if (String.IsNullOrEmpty(val))
			{
				val = defaultVal;
			}
			return val;
		}
	}
}
