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
using System.Xml;
using System.Globalization;

using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using NAnt.Core;

namespace EA.PackageSystem.PackageCore
{
	public class PackageCleanup
	{
        private static HashSet<string> s_updatedMetaData = new HashSet<string>();

        public static void CleanupOndemandPackages(Project project, string ondemandpath, int threshold, bool warn)
		{
			string metadataFolder = Path.Combine(ondemandpath, "ondemand_metadata");

			// get the path to each of the existing metadata files
			List<string> metadataFiles = new List<string>();
			if (Directory.Exists(metadataFolder))
			{
				metadataFiles.AddRange(Directory.GetFiles(metadataFolder, "*.metadata.xml"));
			}

			if (PackageMap.Instance.MasterConfig == MasterConfig.UndefinedMasterconfig)
			{
				// when calling this from eapm we will need to add the ondemand path to the package roots and rescan the for releases
				// if one isn't automatically set.
				PackageRootList packageRootList = PackageMap.Instance.PackageRoots;
				PathString ondemandRootPathString = new PathString(ondemandpath);

				if (packageRootList.HasOnDemandRoot)
				{
					// May be set if the user has FRAMEWORK_ONDEMAND_ROOT set. Not explicitly overriding will cause eapm to throw an
					// error about it having already been set.
					packageRootList.OverrideOnDemandRoot(ondemandRootPathString);
				}
				else
				{
					packageRootList.Add(ondemandRootPathString, PackageRootList.RootType.OnDemandRoot);
				}
			}

			// check to see if any of the packages in the ondemand root are missing an ondemand metadata file and if so generate one.
			// This should be done after collecting the existing metadata files so that we don't bother reading these files we just created.
			CreateDefaultOnDemandMetadataFile(project.Log, ondemandpath);

			int packagesDeleted = 0;
			int packagesWarned = 0;
			int packagesKept = 0;

			project.Log.Info.WriteLine("Checking for releases that haven't been referenced in over {0} days...", threshold);

			foreach (string metadataFilepath in metadataFiles)
			{
				string packageName = null;
				string packageVersion = null;
				string packageDirectory = null;
				DateTime packageReferencedTime= DateTime.Now;

				try
				{
					ReadOndemandMetadataFile(metadataFilepath, out packageName, out packageVersion, out packageDirectory, out packageReferencedTime);
				}
				catch (Exception e)
				{
					project.Log.Warning.WriteLine("Unable to load package metadata file ({0}): {1}", metadataFilepath, e.Message);
					continue;
				}

				string packageFullName = packageName + "-" + packageVersion;
				int days = (int)(DateTime.Now - packageReferencedTime).TotalDays;

				if (!Directory.Exists(packageDirectory))
				{
					try
					{
						project.Log.Info.WriteLine("Directory has already been deleted '{0}', deleting ondemand metadata file", packageDirectory);
						File.SetAttributes(metadataFilepath, FileAttributes.Normal);
						File.Delete(metadataFilepath);
					}
					catch (Exception e)
					{
						project.Log.Error.WriteLine("Failed to delete ondemand metadata file '{0}': {1}", packageFullName, e.Message);
					}
				}
				else if (days > threshold)
				{
					if (warn)
					{
						project.Log.Warning.WriteLine("Ondemand release '{0}' hasn't been referenced in {1} days", packageFullName, days);
						packagesWarned++;
					}
					else
					{
						try
						{
							project.Log.Status.WriteLine("Deleting ondemand release '{0}', it was last referenced {1} days ago...", packageFullName, days);
							PathUtil.DeleteDirectory(packageDirectory);
							File.SetAttributes(metadataFilepath, FileAttributes.Normal);
							File.Delete(metadataFilepath);
							packagesDeleted++;
						}
						catch (Exception e)
						{
							project.Log.Error.WriteLine("Failed to delete ondemand release '{0}': {1}", packageFullName, e.Message);
						}
					}
				}
				else
				{
					packagesKept++;
				}
			}

			project.Log.Info.WriteLine("Package Cleanup Complete: {0} deleted, {1} warning, {2} kept", packagesDeleted, packagesWarned, packagesKept);
		}

		// Set the time of the release's metadata file to keep track of the last time this package was referenced
		public static void UpdateMetadataTime(Project project, Release release, string ondemandPath)
		{
			if (release.Path.StartsWith(ondemandPath, StringComparison.Ordinal) && project.Properties.GetBooleanPropertyOrDefault("ondemand.referencetracking", true))
			{
				string metadataFolder = Path.Combine(ondemandPath, "ondemand_metadata");
				string metadataFilePath = Path.Combine(metadataFolder, release.FullName + ".metadata.xml");

                lock (s_updatedMetaData)
                {
                    // only update each metadata file one per nant invocation
                    if (s_updatedMetaData.Contains(metadataFilePath))
                    {
                        return;
                    }
                    s_updatedMetaData.Add(metadataFilePath);
                }

				try
				{
					if (!Directory.Exists(metadataFolder))
					{
						Directory.CreateDirectory(metadataFolder);
					}

					UpdateOndemandMetadataFile(metadataFilePath, release.Name, release.Version, release.Path);
				}
				catch (Exception e)
				{
					project.Log.Info.WriteLine("Failed to write to Ondemand Metadata file '{0}': {1}", metadataFilePath, e.Message);
				}
			}
		}

		// Check to see if any of the packages in the ondemand root are missing an ondemand metadata file and if not generate one.
		// We don't want to delete packages without one because we don't know how old they are, but we want to eventually be able to delete them.
		private static void CreateDefaultOnDemandMetadataFile(Log log, string onDemandPath)
		{
			string metadataFolder = Path.Combine(onDemandPath, "ondemand_metadata");
			List<Release> releaseVersions = PackageMap.Instance.GetReleasesInPath(onDemandPath);
			foreach (Release release in releaseVersions)
			{
				string releaseMetadataPath = Path.Combine(metadataFolder, release.FullName + ".metadata.xml");
				if (!File.Exists(releaseMetadataPath))
				{
					log.Warning.WriteLine("The ondemand release '{0}' has no ondemand metadata file, cannot determine the last time this release was referenced. Generating a new metadata file for this package.", release.FullName);

					if (!Directory.Exists(metadataFolder))
					{
						Directory.CreateDirectory(metadataFolder);
					}
					UpdateOndemandMetadataFile(releaseMetadataPath, release.Name, release.Version, release.Path);
				}
			}
		}

		private static void ReadOndemandMetadataFile(string path, out string packageName, out string packageVersion, out string packageDirectory, out DateTime referencedTime)
		{
			XmlDocument doc = new XmlDocument();
			doc.Load(path);

			XmlNode packageElement = doc.GetChildElementByName("package");

			XmlNode nameElement = packageElement.GetChildElementByName("name");
			packageName = nameElement.InnerText;

			XmlNode versionElement = packageElement.GetChildElementByName("version");
			packageVersion = versionElement.InnerText;

			XmlNode directoryElement = packageElement.GetChildElementByName("directory");
			packageDirectory = directoryElement.InnerText;

			XmlNode referencedElement = packageElement.GetChildElementByName("referenced");
			string culture = referencedElement.GetAttributeValue("culture", CultureInfo.CurrentCulture.Name);
			referencedTime = DateTime.Parse(referencedElement.InnerText, CultureInfo.CreateSpecificCulture(culture));
		}

		private static void UpdateOndemandMetadataFile(string path, string packageName, string packageVersion, string packageDirectory)
		{
			XmlDocument doc = new XmlDocument();
			if (File.Exists(path))
			{
				doc.Load(path);

				XmlNode packageElement = doc.GetChildElementByName("package");
				XmlNode referencedElement = packageElement.GetChildElementByName("referenced");
				string cultureName = referencedElement.GetAttributeValue("culture", CultureInfo.CurrentCulture.Name);
				CultureInfo culture = CultureInfo.CreateSpecificCulture(cultureName);
				DateTime referencedTime = DateTime.Parse(referencedElement.InnerText, culture);

				// only update the time if it hasn't been updated in the last day to reduce the number of writes for common dependencies
				if (referencedTime < DateTime.Now.AddDays(-1))
				{
					referencedElement.InnerText = DateTime.Now.ToString(culture);
					doc.Save(path);
				}
			}
			else
			{
				XmlElement topElement = doc.CreateElement("package");

				XmlElement nameElement = doc.CreateElement("name");
				nameElement.InnerText = packageName;
				topElement.AppendChild(nameElement);

				XmlElement versionElement = doc.CreateElement("version");
				versionElement.InnerText = packageVersion;
				topElement.AppendChild(versionElement);

				XmlElement directoryElement = doc.CreateElement("directory");
				directoryElement.InnerText = packageDirectory;
				topElement.AppendChild(directoryElement);

				XmlElement referenceDateElement = doc.CreateElement("referenced");
				referenceDateElement.SetAttribute("culture", CultureInfo.CurrentCulture.Name);
				referenceDateElement.InnerText = DateTime.Now.ToString();
				topElement.AppendChild(referenceDateElement);

				doc.AppendChild(topElement);

				doc.Save(path);
			}
		}
	}
}