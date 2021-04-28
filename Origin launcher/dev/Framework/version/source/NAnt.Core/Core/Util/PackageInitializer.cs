using System;
using System.Collections.Concurrent;
using System.IO;
using System.Linq;

using NAnt.Core.PackageCore;

namespace NAnt.Core.Util
{
	internal static class PackageInitializer
	{
		private static readonly ConcurrentDictionary<string, string> s_scriptPath = new ConcurrentDictionary<string, string>();

		internal static void IncludePackageInitializeXml(Project project, Release info, Location location, bool warningOnMissing = true)
		{
			string initScriptPath = GetInitializeXmlScriptPath(info);
			if (initScriptPath == null)
			{
				if (
					!(info.Name?.Equals("eaconfig", StringComparison.OrdinalIgnoreCase) ?? false) &&
					!(info.Name?.Equals("Framework", StringComparison.OrdinalIgnoreCase) ?? false) &&
					warningOnMissing
				)
				{
					project.Log.Warning.WriteLine(" NOTE: Initialize.xml script not found in package '{0}' [{1}].", info.Name, Path.Combine(info.Path, Path.Combine("scripts", "Initialize.xml")));
				}
				return;
			}

			lock (project.ScriptInitLock)
			{
				bool initialized = project.Properties.Contains($"package.{info.FullName}.initialized");
				if (!initialized)
				{
					project.Properties.AddReadOnly($"package.{info.FullName}.initialized", "true");

					/* DAVE-FUTURE-REFACTOR-TODO - this would be a useful assumption to be able to make and would
					prevent a class of user error that can be very confusing, but it's potentially a nasty
					breaking change that I'd probably be too scared to make without a bunch integration testing
					against most games
					if (Project.Properties.Contains($"package.{info.FullName}.loading-initialize-script"))
					{
						throw new BuildException($"Cyclic dependency! File {initScriptPath} includes itself.");
					}*/
					project.Properties.Add(String.Format("package.{0}.loading-initialize-script", info.Name), "true");
				}

				// include the script
				if (project.Log.InfoEnabled)
				{
					project.Log.Info.WriteLine("include '{0}'", initScriptPath);
				}

				if (!initialized)
				{
					project.Properties["on-initializexml-load.package"] = info.Name;
					try
					{
						if (project.TargetExists("on-initializexml-load")) // TODO - having a single target for this is a bit limiting, be better to register a list of task names
						{
							project.Execute("on-initializexml-load", true);
						}
						project.IncludeBuildFileDocument(initScriptPath, Project.TargetStyleType.Use, info.Name, location);
					}
					finally
					{
						project.Properties.Remove("on-initializexml-load.package");
					}

					project.Properties.Remove(String.Format("package.{0}.loading-initialize-script", info.Name));
				}
			}
		}

		internal static void AddPackageProperties(Project project, Release info)
		{
			// add package.Framework-master.dir
			project.Properties.UpdateReadOnly(String.Format("package.{0}.dir", info.FullName), info.Path);

			// add package.Framework.dir 
			// TODO: make sure this package has the highest version (master and x.x override x.x.x)
			project.Properties.UpdateReadOnly(String.Format("package.{0}.dir", info.Name), info.Path);
			project.Properties.UpdateReadOnly(String.Format("package.{0}.version", info.Name), info.Version);

			// the version number of the SDK or third party files within the package
			// by default we will set this to the directory version up to the first dash
			// individual packages can override this if necessary
			if (info.Version.StartsWith("dev") == false)
			{
				string sdk_version_property_name = String.Format("package.{0}.sdk-version", info.Name);
				if (project.Properties.Contains(sdk_version_property_name) == false)
				{
					int index_of_first_dash = info.Version.IndexOf('-');
					index_of_first_dash = index_of_first_dash > 0 ? index_of_first_dash : info.Version.Length;
					string sdk_version = info.Version.Substring(0, index_of_first_dash);
					project.Properties.Add(sdk_version_property_name, sdk_version);
				}
			}

			// set builddir and gendir based on templates
			string packageRoot = info.IsFlattened ?
					PathNormalizer.Normalize(Path.Combine(info.Path, "..")) :
					PathNormalizer.Normalize(Path.Combine(info.Path, "..", ".."));

			// %version% in replacement templates is replaced with package version EXCEPT for when
			// package has the special version of "flattened" in which case it is replace by empty string
			string replacementVersion = info.Version;
			replacementVersion = replacementVersion == Release.Flattened ? String.Empty : replacementVersion;

			MacroMap rootTemplatesMap = new MacroMap()
			{
				{ "package.name", info.Name },
				{ "package.version", replacementVersion },
				{ "package.root", packageRoot },
				{ "buildroot", PackageMap.Instance.GetBuildGroupRoot(project, info.Name) }
			};

			string buildDirTemplate = project.Properties["package.builddir.template"] ?? "%buildroot%/%package.name%/%package.version%";
			string buildDir = PathNormalizer.Normalize(MacroMap.Replace(buildDirTemplate, rootTemplatesMap, additionalErrorContext: " from property package.builddir.template"));
			project.Properties.UpdateReadOnly(String.Format("package.{0}.builddir", info.Name), buildDir);

			string genDirTemplate = project.Properties["package.gendir.template"] ?? "%buildroot%/%package.name%/%package.version%";
			string genDir = PathNormalizer.Normalize(MacroMap.Replace(genDirTemplate, rootTemplatesMap, additionalErrorContext: " from property package.gendir.template"));
			project.Properties.UpdateReadOnly(String.Format("package.{0}.gendir", info.Name), genDir);

			// update package.all property
			project.Properties.SetOrAdd("package.all", (oldValue) => { return (oldValue + " " + info.Name).Split(new char[] { ' ' }).Distinct().ToString(" "); });
		}

		private static string GetInitializeXmlScriptPath(Release info)
		{
			string initScriptPath = s_scriptPath.GetOrAdd(info.Path, k =>
			{
				string path = Path.Combine(info.Path, Path.Combine("scripts", "Initialize.xml"));
				if (File.Exists(path))
					return path;

				// Some packages may use lower case, which is important on Linux
				if (PathUtil.IsCaseSensitive)
				{
					path = Path.Combine(info.Path, Path.Combine("scripts", "initialize.xml"));
					if (File.Exists(path))
						return path;
				}

				path = Path.Combine(info.Path, "Initialize.xml");
				if (File.Exists(path))
					return path;

				// Some packages may use lower case, which is important on Linux
				if (PathUtil.IsCaseSensitive)
				{
					path = Path.Combine(info.Path, "initialize.xml");
					if (File.Exists(path))
						return path;
				}

				return null;
			});
			return initScriptPath;
		}
	}
}
