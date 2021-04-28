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

using System.Xml;
using System.IO;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;

namespace EA.Eaconfig
{
	/// <summary>
	/// Updates the compatibility element to match the version of the package
	/// </summary>
	[TaskName("update-manifest-compatibility")]
	public class UpdateManifestCompatibilityTask : Task
	{
		/// <summary>Allows a build script write to specify a different/modified version to use for compatibility in place of the package version</summary>
		[TaskAttribute("version")]
		public string Version { get; set; }

		protected override void ExecuteTask()
		{
			if (Version == null)
			{
				Version = Project.Properties.GetPropertyOrDefault("package.version", null);
			}
			string packagedir = Project.Properties.GetPropertyOrDefault("package.dir", null);

			if (Version != null && packagedir != null)
			{
				string path = Path.Combine(packagedir, "Manifest.xml");
				if (File.Exists(path))
				{
					File.SetAttributes(path, FileAttributes.Normal);
					XmlDocument document = new XmlDocument();
					document.Load(path);
					XmlNode compatibilitynode = document.GetChildElementByName("package").GetChildElementByName("compatibility");
					if (compatibilitynode != null)
					{
						string revision = compatibilitynode.GetAttributeValue("revision");
						Version = RemoveSuffixes(Version);

						if (revision.IsNullOrEmpty() == false && revision.StartsWith(Version) == false)
						{
							if (Project.Properties.GetBooleanPropertyOrDefault("nant.updatemanifestrevision", true))
							{
								compatibilitynode.SetAttribute("revision", Version);
								document.Save(path);

								Project.Log.Status.WriteLine("[UpdateManifestCompatibility] Package revision number ({0}) does not match the version number({1})! Updating... (note: this can be disabled with 'nant.updatemanifestrevision=false')", revision, Version);
								Project.Log.Status.WriteLine();
								Project.Log.Status.WriteLine("  !!! Please submit updated Manifest file to source control !!!");
								Project.Log.Status.WriteLine();
							}
							else
							{
								Project.Log.Warning.WriteLine("[UpdateManifestCompatibility] Package revision number ({0}) does not match the version number({1})!", revision, Version);
								Project.Log.Warning.WriteLine("Please ensure the revision number is up-to-date before submitting package!");
							}
						}
					}
				}
			}
		}

		/// <summary>
		/// Remove certain common suffixes so that packages with the same version number are treated the same.
		/// For example we don't want the proxy suffix to affect version comparisons.
		/// </summary>
		private string RemoveSuffixes(string version)
		{
			return version = version
				.Replace("-non-proxy", "")
				.Replace("-proxy", "")
				.Replace("-lite", "");
		}
	}
}
