// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml.Linq;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using NAnt.NuGet.Deprecated;

namespace EA.PackageSystem
{
	public class NugetProtocol : ProtocolBase
	{
		// tracking information for nuget packages, needs to preserve nugetinstall.marker, for compatbility
		// with older FW versions
		internal class MarkerDetails
		{
			private const string LegacyInstallMarkerName = "nugetinstall.marker";
			private const string InstallMarkerName = "nugetinstall2.marker";

			internal bool ShouldRepackage { get => NugetVersion != CurrentPackageVersion // nuget package content is good*, but we need to update our generated files. *: Though we might preserve some old legacy stuff - see install logic
				|| AlwaysPackage; }
			internal bool ShouldReinstall
			{
				get
				{
					if (AlwaysDownload)
					{
#pragma warning disable CS0162 // Unreachable code detected
						return true;
					}
					else if (ValidMarker) // if we had a new marker with an old version - reinstall
					{
						return InstallVersion != CurrentInstallVersion;
					}
					else if (ValidLegacyMarker) // if we had an older marker with old version - reinstall
					{
						return LegacyInstallVersion != CurrentInstallVersion;
					}
					return false;
#pragma warning restore CS0162 // Unreachable code detected
				}
			}

			internal readonly uint NugetVersion = 0;
			internal readonly uint InstallVersion = 0;
			internal readonly uint LegacyNugetVersion = 0;
			internal readonly uint LegacyInstallVersion = 0;

			internal readonly bool ValidLegacyMarker = false;
			internal readonly bool ValidMarker = false;

			private MarkerDetails(string root)
			{
				string legacyMarkerPath = Path.Combine(root, LegacyInstallMarkerName);
				ValidLegacyMarker = TryReadMarkerFile(legacyMarkerPath, out LegacyNugetVersion, out LegacyInstallVersion);

				string markerPath = Path.Combine(root, InstallMarkerName);
				ValidMarker = TryReadMarkerFile(markerPath, out NugetVersion, out InstallVersion);
			}

			internal void Save(string root)
			{
				if (ValidLegacyMarker)
				{
					string legacyMarkerPath = Path.Combine(root, LegacyInstallMarkerName);
					WriteMarkerFile(legacyMarkerPath, LegacyNugetVersion, LegacyInstallVersion);
				}
				string markerPath = Path.Combine(root, InstallMarkerName);
				WriteMarkerFile(markerPath, CurrentPackageVersion, CurrentInstallVersion);
			}

			internal static void Delete(string root)
			{
				string legacyMarkerPath = Path.Combine(root, LegacyInstallMarkerName);
				File.Delete(legacyMarkerPath);
				string markerPath = Path.Combine(root, InstallMarkerName);
				File.Delete(markerPath);
			}

			internal static MarkerDetails Load(string root) => new MarkerDetails(root);

			private static void WriteMarkerFile(string markerFilePath, uint packageVersion, uint installVersion = 0)
			{
				string[] lines = installVersion == 0 ?
					new string[]
					{
						packageVersion.ToString()
					} :
					new string[]
					{
						packageVersion.ToString(),
						installVersion.ToString()
					};
				File.WriteAllLines(markerFilePath, lines);
			}

			private static bool TryReadMarkerFile(string markerFileLocation, out uint nugetPackageVersion, out uint nugetInstallVersion)
			{
				nugetPackageVersion = 0;
				nugetInstallVersion = 0;

				if (!File.Exists(markerFileLocation))
				{
					return false;
				}

				string[] markerFileLines = File.ReadAllLines(markerFileLocation);
				if (markerFileLines.Length != 1 && markerFileLines.Length != 2)
				{
					return false;
				}

				if (!UInt32.TryParse(markerFileLines[0], out nugetPackageVersion))
				{
					return false;
				}

				if (markerFileLines.Length == 2)
				{
					if (!UInt32.TryParse(markerFileLines[1], out nugetInstallVersion))
					{
						return false;
					}
				}

				return true;
			}
		}

		public const uint CurrentPackageVersion = 28; // if you make breaking changes that require nuget package to be repackaged (e.g, new initialize.xml needed) , increment this number to force re-package
		public const uint CurrentInstallVersion = 21; // if you make changes that requires package to be fully reinstalled from nuget (redownloaded), increment this number to force re-download and re-package
		public const bool AlwaysDownload = false; // set to true while debugging only, forces nuget packages to be redownloaded every time
		public const bool AlwaysPackage = false; // set to true while debugging only, forces framework wrapping of nuget packages to be regenerated everytime

		public const string LogPrefix = "Protocol package server (nuget): ";

		public NugetProtocol()
		{
		}

		public override Release Install(MasterConfig.IPackage package, Uri uri, string installRoot, ProtocolPackageServer protocolServer)
		{
			if (protocolServer.Project.UseNugetResolution)
			{
				throw new InvalidOperationException("INTERNAL ERROR! Legacy NuGet protocol invoked when using new NuGet resolution.");
			}

			return Install(package, uri, installRoot, protocolServer, null);
		}

		public override void Update(Release release, Uri uri, string defaultInstallDir, ProtocolPackageServer protocolServer)
		{
			if (protocolServer.Project.UseNugetResolution)
			{
				throw new InvalidOperationException("INTERNAL ERROR! Legacy NuGet protocol invoked when using new NuGet resolution.");
			}

			string rootPath = Path.Combine(defaultInstallDir, release.Name, release.Version);
			MarkerDetails markerDetails = MarkerDetails.Load(rootPath);
			if (markerDetails.ShouldReinstall)
			{
				// perform full reinstall of package
				protocolServer.Log.Minimal.WriteLine("Fully restoring NuGet package {0}...", release.Name);
				Install(release.Package, uri, defaultInstallDir, protocolServer, markerDetails);
			}
			else if (markerDetails.ShouldRepackage)
			{
				// in this case the nuget package is good, only framework trappings need updating
				protocolServer.Log.Minimal.WriteLine("Restoring NuGet package {0} Framework files...", release.Name);

				// delete marker file first, if repackaging fails for any reason we should trigger a full resintall next time
				// as we're breaking normal package server operation here and modifying an on demand package in place
				MarkerDetails.Delete(rootPath);

				NugetPackage nugetPackage = NugetPackageManager.GetLocalPackage(release.Name, release.Version, rootPath);
				if (nugetPackage == null)
				{
					throw new BuildException($"Nuget protocol internal error - tried to repackage package at '{rootPath}' but could not resolve local package there.");
				}

				// work out nuget package folder it will always be root path + name
				string nugetDir = Path.Combine(rootPath, nugetPackage.PackageRelativeDir);

				// delete all the files outside nuget package folder
				foreach (string filePath in Directory.EnumerateFiles(rootPath))
				{
					File.Delete(filePath);
				}
				string legacyDir = Path.Combine(rootPath, nugetPackage.Name + "." + nugetPackage.Version);
				foreach (string directory in Directory.EnumerateDirectories(rootPath))
				{
					// Make sure we do lower case compare in case packageName and real NuGet name has inconsistent case.
					if (directory.ToLower() != nugetDir.ToLower() && directory.ToLower() != legacyDir.ToLower())
					{
						PathUtil.DeleteDirectory(directory);
					}
				}

				PackageNugetPackage(protocolServer, release.Name, rootPath, nugetPackage, markerDetails);
				SetDependencyVersions(nugetPackage, protocolServer);
			}
			else
			{
				// populate PackageMap with autoversions for dependencies, this is normally done as part of
				// packaging in WriteInitalizeXml
				NugetPackage nugetPackage = NugetPackageManager.GetLocalPackage(release.Name, release.Version, rootPath);
				SetDependencyVersions(nugetPackage, protocolServer);
			}
		}

		private Release Install(MasterConfig.IPackage package, Uri uri, string rootDirectory, ProtocolPackageServer protocolServer, MarkerDetails markerDetails)
		{
			// create temp directory for installed
			string nugetTempRoot = Path.Combine(rootDirectory, ".nuget");
			string randomNameFolder = Path.Combine(nugetTempRoot, Path.GetRandomFileName());
			string tempInstallPath = Path.Combine(randomNameFolder, package.Name, package.Version);
			markerDetails = markerDetails ?? MarkerDetails.Load(tempInstallPath);

			if (Directory.Exists(tempInstallPath))
			{
				PathUtil.DeleteDirectory(tempInstallPath);
			}
			Directory.CreateDirectory(tempInstallPath);

			try
			{
				protocolServer.Log.Minimal.WriteLine("Downloading NuGet package {0}, {1} from {2}...", package.Name, package.Version, NugetUriToHttpString(uri));
				NugetPackage nugetPackage = NugetPackageManager.DownloadRemotePackage(package.Name, package.Version, NugetUriToHttpString(uri), tempInstallPath);
				SetDependencyVersions(nugetPackage, protocolServer);
				PackageNugetPackage(protocolServer, package.Name, tempInstallPath, nugetPackage, markerDetails);

				// if we have a valid marker, copy the old package contents to our temp folder to preserver old structure
				string finalDir = Path.Combine(rootDirectory, package.Name, package.Version);
				string legacyDir = Path.Combine(finalDir, nugetPackage.Name + "." + nugetPackage.Version);
				if (markerDetails.ValidLegacyMarker && Directory.Exists(legacyDir))
				{
					string legacyTempDir = Path.Combine(tempInstallPath, nugetPackage.Name + "." + nugetPackage.Version);
					CopyDirectory(legacyDir, legacyTempDir);
				}

				// copy synced package to destination
				if (Directory.Exists(finalDir))
				{
					PathUtil.DeleteDirectory(finalDir);
				}
				CopyDirectory(tempInstallPath, finalDir);

				return new Release(package, finalDir, isFlattened: false, Manifest.Load(finalDir));
			}
			finally
			{
				// clean up temp dir
				if (Directory.Exists(randomNameFolder))
				{
					PathUtil.DeleteDirectory(randomNameFolder);
				}

				// Making sure the temp root folder is removed after we have finished.
				// Don't remove unless it is empty, incase we are fetching multiple packages in parallel.
				if (Directory.Exists(nugetTempRoot) && !Directory.EnumerateFileSystemEntries(nugetTempRoot).Any())
				{
					try
					{
						Directory.Delete(nugetTempRoot);
					}
					catch
					{
					}
				}
			}
		}

		// add framework trappings around a nuget package that has already been installed, package argument should be the local
		// package that has been installed to rootPath
		private static void PackageNugetPackage(ProtocolPackageServer protocolServer, string packageName, string rootPath, NugetPackage package, MarkerDetails markerDetails)
		{
			// write license info to package
			string licenseFilePath = Path.Combine(rootPath, "3RDPARTYLICENSES.TXT");
			WriteLicenseInfoFile(licenseFilePath, package);

			// add build file
			string buildFilePath = Path.Combine(rootPath, packageName + ".build");
			WriteBuildFile(buildFilePath, packageName, rootPath);

			// add initialize xml
			string initializeFilePath = Path.Combine(rootPath, "scripts", "Initialize.xml");
			WriteInitalizeXml(protocolServer, initializeFilePath, packageName, package);

			// add manifest xml
			string manifestXmlPath = Path.Combine(rootPath, "Manifest.xml");
			WriteManifestXml(manifestXmlPath);

			// write marker file - allows us to reinstall if nuget protocol has breaking changes
			markerDetails.Save(rootPath);
		}

		private static void WriteInitalizeXml(ProtocolPackageServer protocolServer, string initializeFilePath, string packageName, NugetPackage package)
		{
			// add nuget dir property
			string nugetDir = $"${{package.{packageName}.dir}}/{package.PackageRelativeDir.Replace("\\", "/")}";

			// projectElements - accumulates initialize xml contents
			List<object> projectElements = new List<object>();

			string nugetDirPropertyName = $"package.{packageName}.nugetDir";
			projectElements.Add(new XElement("property", new XAttribute("name", nugetDirPropertyName), new XAttribute("value", nugetDir)));
			string nugetPackageBaseDir = $"${{{nugetDirPropertyName}}}"; // evaluated version of nugetdir property, used as base dir for most filesets

			// .net version definition
			projectElements.Add(new XElement
			(
				"do",
				new XAttribute("if", "@{PlatformIsWindows()}"),
				new XElement("dependent", new XAttribute("name", "DotNet")),
				new XElement("property", new XAttribute("name", "dot-net-version"), new XAttribute("value", "${package.DotNet.version}"), new XAttribute("local", "true"))
			));
			projectElements.Add(new XElement
			(
				"do",
				new XAttribute("if", "@{PlatformIsUnix()} or @{PlatformIsOSX()}"),
				new XElement("property", new XAttribute("name", "dot-net-version"), new XAttribute("value", "${package.mono.runtimeversion??4.70}"), new XAttribute("local", "true"))
			));

			// add a minimum version check if we have a minimum version
			List<NugetPackage.FrameworkItems> frameworkItems = package.GetFrameworkItems(out string highestUnsupported);
			if (frameworkItems.Any())
			{
				XElement chooseBlock = new XElement("choose");
				if (highestUnsupported != null)
				{
					chooseBlock.Add
					(
						new XElement
						(
							"do",
							new XAttribute("if", $"@{{StrCompareVersions('{RealDotNetVersionToPackageDotNetVersion(highestUnsupported)}', '${{dot-net-version}}')}} >= 0"),
							new XElement("fail", new XAttribute("message", $"This package cannnot support dotnet {highestUnsupported} and older. Current version is '${{dot-net-version}} '."))
						)
					);
				}

				// create do blocks for each framework set, with the last having open ended range
				for (int i = 0; i < frameworkItems.Count(); ++i)
				{
					NugetPackage.FrameworkItems currentItems = frameworkItems[i];
					if (i < frameworkItems.Count() - 1)
					{
						// write current range
						NugetPackage.FrameworkItems nextItems = frameworkItems[i + 1];
						string minRange = RealDotNetVersionToPackageDotNetVersion(currentItems.FrameworkVersion);
						string maxRange = RealDotNetVersionToPackageDotNetVersion(nextItems.FrameworkVersion);

						object[] doBlock = ConditionalIntializeXmlBlockContentsFromFrameworkItems(protocolServer, $"(@{{StrCompareVersions('${{dot-net-version}}', '{minRange}')}} >= 0) and (@{{StrCompareVersions('${{dot-net-version}}', '{maxRange}')}} < 0)",
							currentItems, packageName, nugetPackageBaseDir);
						chooseBlock.Add(doBlock);
					}
					else
					{
						object[] doBlock = ConditionalIntializeXmlBlockContentsFromFrameworkItems(protocolServer, $"@{{StrCompareVersions('${{dot-net-version}}', '{RealDotNetVersionToPackageDotNetVersion(currentItems.FrameworkVersion)}')}} >= 0",
							currentItems, packageName, nugetPackageBaseDir);
						chooseBlock.Add(doBlock);
					}
				}
				projectElements.Add(chooseBlock);
			}

			// add shared items if applicaable
			NugetPackage.FrameworkItems sharedItems = package.GetSharedItems();
			if (sharedItems != null)
			{
				projectElements.Add(ConditionalIntializeXmlBlockContentsFromFrameworkItems(protocolServer, null, sharedItems, packageName, nugetPackageBaseDir, append: true));
			}

			// create intialize.xml
			XDocument initializeDoc = new XDocument
			(
				new XElement
				(
					"project",
					projectElements
				)
			);
			string initializeFileDir = Path.GetDirectoryName(initializeFilePath);
			if (!Directory.Exists(initializeFileDir))
			{
				Directory.CreateDirectory(initializeFileDir);
			}
			initializeDoc.Save(initializeFilePath);
		}

		private static object[] ConditionalIntializeXmlBlockContentsFromFrameworkItems(ProtocolPackageServer protocolServer, string condition, NugetPackage.FrameworkItems items, string packageName, string nugetPackageBaseDir, bool append = false)
		{
			List<object> blockContents = new List<object>();

			// Case correct the dependencies if they're defined in the masterconfig
            IEnumerable<string> masterconfigDependencies = items.Dependencies.Select(dependency =>
            {
				if (PackageMap.Instance.TryGetMasterPackage(protocolServer.Project, dependency, out MasterConfig.IPackage dotNetSpec)) 
                    return dotNetSpec.Name;
                return dependency;
            }).ToList();

			// get dependents, add call dependent so we can reference them below
			if (masterconfigDependencies.Any())
			{
				// this do / parallel.do block is to handle the way we try to handle compatbility with older FW that 
				// pick up this package - older FW will hang if this is a parallel do
				blockContents.Add
				(
					new XElement
					(
						"parallel.do",
                        masterconfigDependencies
							.Select<string, object>(dependency => new XElement("dependent", new XAttribute("name", dependency)))
							.Prepend(new XAttribute("if", "@{StrCompareVersions(${nant.version}, '8.0.2')} gt 0"))
					)
				);
				blockContents.Add
				(
					new XElement
					(
						"do",
                        masterconfigDependencies
							.Select<string, object>(dependency => new XElement("dependent", new XAttribute("name", dependency)))
							.Prepend(new XAttribute("unless", "@{StrCompareVersions(${nant.version}, '8.0.2')} gt 0"))
					)
				);
			}

			// add assemblies, we also expose the assemblies of our dependencies
			blockContents.Add
			(
				FileSetXmlBlock
				(
					name: $"package.{packageName}.assemblies",
					baseDir: nugetPackageBaseDir,
					contents:
						// assemblies contained in the package
						items.Assemblies.Select(relativePath => new XElement("includes", new XAttribute("name", relativePath)))

						// framework assemblies that could come from installed .net
						.Concat
						(
							items.FrameworkAssemblies.Select(name => new XElement("includes", new XAttribute("name", name + ".dll"), new XAttribute("asis", "true")))
						)

						// assemblies from dependencies
						.Concat
						(
							masterconfigDependencies.Select(dependency => new XElement("includes", new XAttribute("fromfileset", $"package.{dependency}.assemblies")))
						),
					append: append
				)
			);

			// create a fileset for this packages nuget files, this is not a standard fileset
			blockContents.Add
			(
				FileSetXmlBlock
				(
					name: $"package.{packageName}.nuget-content-files",
					baseDir: nugetPackageBaseDir,
					contents:
						items.ContentFiles.Select
						(
							relativePath => new XElement
							(
								"includes",
								new XAttribute("name", relativePath.Substring($"{NugetPackageManager.ContentFolder}/".Length)),
								new XAttribute("basedir", $"{nugetPackageBaseDir}/{NugetPackageManager.ContentFolder}"), // nuget 5 api returns all files relative to root, but we don't want preserve the mandatory "content" folder for content files
								new XAttribute("optionset", $"${{package.{packageName}.contentfiles.action??do-not-copy}}")
							)
						),
					append: append
				)
			);

			// expose content files of this and dependencies through standard fileset
			blockContents.Add
			(
				FileSetXmlBlock
				(
					name: $"package.{packageName}.contentfiles",
					baseDir: nugetPackageBaseDir,
					contents:
						new object[] { new XElement("includes", new XAttribute("fromfileset", $"package.{packageName}.nuget-content-files")) }
						.Concat(FromAllDependenciesFilesSet(masterconfigDependencies, "contentfiles")),
					append: append
				)
			);

			// init files (powershell scripts run on whole solution context)
			blockContents.Add
			(
				FileSetXmlBlock
				(
					name: $"package.{packageName}.nuget-init-scripts",
					baseDir: nugetPackageBaseDir,
					contents: 
						items.InitFiles.Select(relativePath => new XElement("includes", new XAttribute("name", relativePath))),
					append: append
				)
			);

			// install files (powershell scripts run on project context)
			blockContents.Add
			(
				FileSetXmlBlock
				(
					name: $"package.{packageName}.nuget-install-scripts",
					baseDir: nugetPackageBaseDir,
					contents: 
						items.InstallFiles.Select(relativePath => new XElement("includes", new XAttribute("name", relativePath))),
					append: append
				)
			);

			// .props files (files to be included at the *start* of dependent .csprojs)
			blockContents.Add
			(
				FileSetXmlBlock
				(
					name: $"package.{packageName}.nuget-prop-files",
					baseDir: nugetPackageBaseDir,
					contents: 
						items.PropsFiles.Select(relativePath => new XElement("includes", new XAttribute("name", relativePath)))
						.Concat(FromAllDependenciesFilesSet(masterconfigDependencies, "nuget-prop-files")),
					append: append
				)
			);

			// .target files (files to be included at the *end* of dependent .csprojs)
			blockContents.Add
			(
				FileSetXmlBlock
				(
					name: $"package.{packageName}.nuget-target-files",
					baseDir: nugetPackageBaseDir,
					contents:
						items.TargetsFiles.Select(relativePath => new XElement("includes", new XAttribute("name", relativePath)))
						.Concat(FromAllDependenciesFilesSet(masterconfigDependencies, "nuget-target-files")),
					append: append
				)
			);

			// add dependencies
			string dependenciesString = append ? "${property.value}" : String.Empty;
			dependenciesString += String.Join("", masterconfigDependencies.Select(dependency => "\n" + dependency));
			blockContents.Add
			(
				new XElement
				(
					"property",
					new XAttribute("name", $"package.{packageName}.nuget-dependencies"),
					dependenciesString
				)
			);

			// if a conditions was specified we need a do block to wrap
			if (condition != null)
			{
				return new object[]
				{
					new XElement
					(
						"do",
						new XAttribute("if", condition),
						blockContents.ToArray()
					)
				};
			}

			return blockContents.ToArray();
		}

		private static void WriteBuildFile(string buildFilePath, string packageName, string rootPath)
		{
			string buildFileDir = Path.GetDirectoryName(buildFilePath);
			if (!Directory.Exists(buildFileDir))
			{
				Directory.CreateDirectory(buildFileDir);
			}
			File.WriteAllText(buildFilePath, String.Format("<project><package name=\"{0}\" initializeself=\"true\"/></project>", packageName));
			/*// TODO: can probably remove - keeping around temporarily in case I need to revive this approach - Dave V
			File.WriteAllText
			(
				buildFilePath,
				String.Join
				(
					Environment.NewLine,
					$"<project>",
					$"	<package name=\"{release.Name}\" initializeself=\"true\"/>",
					$"	<Utility name=\"{release.Name}\" transitivelink=\"true\">",
					$"		<dependencies>",
					$"			<auto>",
					$"				${{package.{release.Name}.nuget-dependencies}}",
					$"			<auto>",
					$"		</dependencies>",
					$"  </Utility>",
					$"</project>"
				)
			);*/
		}

		private static void WriteManifestXml(string manifestXmlPath)
		{
			string manifestFileDir = Path.GetDirectoryName(manifestXmlPath);
			if (!Directory.Exists(manifestFileDir))
			{
				Directory.CreateDirectory(manifestFileDir);
			}
			File.WriteAllText(manifestXmlPath, String.Format("<package>{0}    <frameworkVersion>2</frameworkVersion>{0}    <buildable>false</buildable>{0}</package>", Environment.NewLine));
		}

		private static void WriteLicenseInfoFile(string licenseFilePath, NugetPackage package)
		{
			if (package.LicenseUrl != null)
			{
				StringBuilder licenseFileBuilder = new StringBuilder();
				if (package.Copyright != null)
				{
					licenseFileBuilder.Append(package.Copyright);
					licenseFileBuilder.Append("\n\n");
				}
				licenseFileBuilder.Append("License Info: ");
				licenseFileBuilder.Append(package.LicenseUrl.ToString());
				string licenseFileDir = Path.GetDirectoryName(licenseFilePath);

				if (!Directory.Exists(licenseFileDir))
				{
					Directory.CreateDirectory(licenseFileDir);
				}
				File.WriteAllText(licenseFilePath, licenseFileBuilder.ToString());
			}
		}

		private static void SetDependencyVersions(NugetPackage package, ProtocolPackageServer protocolServer)
		{
			string currentDotNetVersion = Environment.Version.ToString(2);
			if (PlatformUtil.IsWindows)
			{
				// Get current DotNet version from masterconfig.
				if (PackageMap.Instance.TryGetMasterPackage(protocolServer.Project, "DotNet", out MasterConfig.IPackage dotNetSpec))
				{
					currentDotNetVersion = dotNetSpec.Version;
				}
			}
			else if (PlatformUtil.IsMonoRuntime)
			{
				// In the case of mono, since we can't check the registry and don't use the dotnet package, we just need to guess based on the version of mono installed
				// For now we will just assume that we have .Net 4.7 and allow eaconfig to change this based on the mono version in the future if needed
				currentDotNetVersion = protocolServer.Project.Properties.GetPropertyOrDefault("package.mono.runtimeversion", "4.70");
			}

			// ewww, translate from our broken dot net version number to actual dot net vesion numbers
			string[] parts = new string(currentDotNetVersion.TakeWhile(c => Char.IsDigit(c) || c == '.').ToArray()).Split('.');
			if (parts.Length == 2 && Int32.TryParse(parts[1].First().ToString(), out int partFirst) && partFirst >= 5 && parts[1].Length > 1)
			{
				currentDotNetVersion = parts[0] + "." + parts[1].First() + '.' + parts[1].Substring(1);
			}

			foreach (KeyValuePair<string, NugetPackage.NugetVersionRequirement> dependency in package.GetAutoDependencyVersions(currentDotNetVersion))
			{
				string dependentName = dependency.Key;
				string resolvedVersion = null;
				if (!PackageMap.Instance.TryGetMasterPackage(protocolServer.Project, dependentName, out MasterConfig.IPackage packageSpec))
				{
					// masterconfig has no entry for this package - determine a "best" version if possible 
					string bestVersion = dependency.Value.GetBestAutoVersion();
					if (bestVersion != null)
					{
						// we have a best version - create auto package version
						MasterConfig.IPackage dependencyPackage = PackageMap.Instance.GetMasterOrAutoPackage(protocolServer.Project, dependentName, bestVersion, package.Name);
						resolvedVersion = dependencyPackage.Version;
					}
					else
					{
						// throw warning that we couldn't come up with "best" auto version, this will give context to error later when version cannot be resolved
						string versionRequirements = dependency.Value.GetVersionRequirementDescription();
						protocolServer.Log.ThrowWarning
						(
							Log.WarningId.PackageServerErrors, Log.WarnLevel.Minimal, 
							"Package {0} does not specify concrete version restrictions for dependency {1}. Please set a version for this package in your masterconfig ({2}).", 
							package.Name, dependentName, versionRequirements
						);
						resolvedVersion = null;
					}
				}
				else
				{
					// People explicitly specified an entry in masterconfig.  Possibility:
					// 	1. The auto version resolve isn't satisfactory and people need specific version.
					//	2. Another NuGet package has the same dependency and already "auto" created this package.
					//	3. Multiple NuGet package has same dependency on this package but they used different case for the name.
					//	   This can affect our assemblies fileset or property name construction / usage.  So user specify one with correct case.
					if (packageSpec.IsAutoVerison && !dependentName.Equals(packageSpec.Name, StringComparison.Ordinal))
					{
						// if code reaches this point we have a matching auto version but of a different case, this means 
						// different parent nuget has given us same dependent package with differently cased names
						// and we have no way to determine which name is correct, for now, we put a warning message to let user
						// know of potential problem and see if we can still work around this by using the already created package name.  
						// If the work around isn't sufficient, we may require user to provide a correctly cased version in masterconfig
						// AND remove the existing package that is already downloaded (including the parent NuGet package that uses it).
						protocolServer.Log.Warning.WriteLine
						(
							String.Format("NuGet case-insensitve package '{0}' is depending on '{1}'. But this NuGet dependency has been previously auto-downloaded by a different NuGet with a different case '{2}'( which created filesets with different names). If you encounter issues like unknown fileset 'package.{1}.assemblies', please set a version for this package in your masterconfig with correctly cased name (and delete the downloaded version and all other NuGet packages that depends on it) so that new package with correct fileset name can be re-created).", package.Name, dependentName, packageSpec.Name)
						);
					}

					resolvedVersion = packageSpec.Version;

					// use pre-existing name in masterconfig
					dependentName = packageSpec.Name;
				}

				// if current Framework Profile is selected Framework and package version could be resolved, check it meets dependency spec requirements
				if (resolvedVersion != null)
				{
					if (!dependency.Value.Satisfies(resolvedVersion))
					{
						string versionRequirements = dependency.Value.GetVersionRequirementDescription();
						protocolServer.Log.ThrowWarning
						(
							Log.WarningId.PackageServerErrors, Log.WarnLevel.Minimal, 
							"Framework is using version '{0}' for package {1} - this does not meet the version requirements of Nuget package {2} ({3}). Specify a different version in your masterconfig.", 
							resolvedVersion, dependentName, package.Name, versionRequirements
						);
					}
				}
			}
		}

		private static string RealDotNetVersionToPackageDotNetVersion(string realDotNetVer)
		{
			// Beginning with .Net 4.5.1 and on, we build the DotNet package with versions 4.XX.  ie:
			// .Net 4.5      ->    4.5.XXXXX
			// .Net 4.5.1    ->    4.51
			// .Net 4.5.2    ->    4.52
			// .Net 4.6      ->    4.60
			// .Net 4.6.1    ->    4.61
			// ...
			// So this function basically convert the real .Net version string to our "package" version string.

			string packageDotNetVersion = realDotNetVer;

			string [] vers = realDotNetVer.Split('.');
			int partCounts = vers.Count();
			int minorVer;
			if (partCounts > 2 && int.TryParse(vers[1],out minorVer))
			{
				if ((minorVer > 5) || (minorVer == 5 && (vers[2] == "1" || vers[2] == "2")))
				{
					packageDotNetVersion = vers[0] + "." + vers[1] + vers[2];
				}
			}
			else if (partCounts == 2 && int.TryParse(vers[1],out minorVer))
			{
				if (minorVer > 5 && minorVer < 10)
				{
					packageDotNetVersion = vers[0] + "." + vers[1] + "0";
				}
			}

			return packageDotNetVersion;
		}

		private static string NugetUriToHttpString(Uri uri)
		{
			const string NugetPrefix = "nuget-";

			string nugetUriAsString = uri.ToString();
			if (!nugetUriAsString.StartsWith(NugetPrefix))
			{
				throw new InvalidOperationException(String.Format("Trying to parse invalid nuget uri '{0}'.", nugetUriAsString));
			}
			return nugetUriAsString.Substring(NugetPrefix.Length);
		}

		private static string GetProfileKeyFrameworkVersion(string profileKey)
		{
			/*
				example folder structure (where <folder> is /content, /lib, etc depending on file type),
				this folder structure drives the IPackageFile.TargetFramework property
					
					<folder>/myfile.ext							- sometimes we just get a raw file with no version info, we just take it as is
			 
					<folder>/monoandroid/_._					- this is special marker file to say that this profile is "supported" by this package, in that no file is required (usually because this 
																  profile automatically supports this in standard libs, or because a sibling portable assembly offers supports for this), monoandroid is
																  just being used as example here, this can be in any profile

					<folder>/net40/myfile.ext					- targets net40, profile will be blank, but can be interpreted as full

					<folder>/net40-client/myfile.ext			- explicitly client profile, we only used these if full is unavailable

					<folder>/net40-full/mmyfile.ext				- explicitly full profile

					<folder>/portable-net40+sl4+win8+wp71+wpa81	- this will have a framework identifer of ".NETPortable" but won't declare it framework support version, we have to manually parse 
																  the profile (profile string will be everything after the "portable-") to get the "net40" to figure out this can target 4.0

					<folder>/sl4/myfile.ext						- this will have a TargetFramework of Silverlight4, which we can ignore because we do not support silverlight, similar exists for wp8, etc
			*/

			if (String.IsNullOrEmpty(profileKey))
				return String.Empty; // distinct from null, empty string means no version constraints

			int idx1 = profileKey.IndexOf(":");
			if (idx1 < 0)
				return null;
			int idx2 = profileKey.IndexOf(":", idx1 + 1);
			if (idx2 < idx1)
				return null;

			string fwIdentifier = profileKey.Substring(0, idx1);
			string fwVersion = profileKey.Substring(idx1 + 1, idx2 - idx1 - 1);
			string fwProfile = profileKey.Substring(idx2 + 1);

			// regular .net file
			if (fwIdentifier == ".NETFramework")
			{
				return fwVersion;
			}

			// .net standard
			//if (fwIdentifier == ".NETStandard")
			//{
			//	if (DotNetStandardSupport.TryGetValue(fwVersion, out string supportedFwVersion))
			//	{
			//		return supportedFwVersion;
			//	}
			//	else if (fwVersion.StrCompareVersions("2.0") > 0)
			//	{
			//		return DotNetStandardSupport["2.0"];
			//	}
			//}

			// portable, might still contain regular .net framework support
			if (fwIdentifier == ".NETPortable")
			{
				// version string can be any number of digits i.e net4, net40, net451, etc
				// in theory Profile string will always start with portable but we match start of string as well as portable- just in case
				// also matching + before in case net isn't listed first though it always seems to be
				Match match = Regex.Match(fwProfile, @"(?:^|\+|(?:portable-))net(\d+)", RegexOptions.IgnoreCase);
				if (match.Success)
				{
					string version = match.Groups[1].Value;
					char majorVersion = version.First(); // guaranteed at least 1 digit, this will be major rversion
					string minorVersion = version.Length > 1 ? version.Substring(1) : "0"; // all following digits are minor
					return String.Format("{0}.{1}", majorVersion, minorVersion);
				}
				return null; // portable but doesn't support regular .net framework
			}

			// targets unsupported framework (silverlight, wp8, etc)
			return null;
		}

		private static XElement FileSetXmlBlock(string name, string baseDir, IEnumerable<object> contents, bool append = false)
		{
			return new XElement
			(
				"fileset",
				new object[]
				{
					new XAttribute("name", name),
					new XAttribute("basedir", baseDir)
				}
				.Concat(append ? new object[] { new XAttribute("append", "true") } : Enumerable.Empty<object>())
				.Concat(contents)
			);
		}

		private static IEnumerable<XElement> FromAllDependenciesFilesSet(IEnumerable<string> dependencies, string fileSetName)
		{
			return dependencies.Select(dependency => new XElement("includes", new XAttribute("fromfileset", $"package.{dependency}.{fileSetName}")));
		}
	}
}