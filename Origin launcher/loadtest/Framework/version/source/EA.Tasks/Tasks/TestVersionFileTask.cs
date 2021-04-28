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
using System.Collections.Specialized;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Shared.Properties;

using NAnt.Core.PackageCore;

namespace EA.FrameworkTasks
{

	/// <summary>Verifies package version info in version.h file.</summary>
	/// <remarks>
	/// <para>
	/// This task should be called by a package target.
	/// </para>
	/// <para>
	/// package task will verify that version information in the version.h file corresponds to the package version.
	/// This verification will apply to the packages that have 'include' directory to export header files. 
	/// Verification process will check that:
	/// * file 'version.h' or 'package_version.h" exists
	/// * version information inside 'version.h' file corresponds to the package version.
	///
	/// Task assumes  that version information in the version.h file complies with the following convention:
	/// #define <name/>_VERSION_MAJOR   1
	/// #define <name/>_VERSION_MINOR   2
	/// #define <name/>_VERSION_PATCH   3
	///
	/// Where <name/> is usually a package name but verification ignores content of <name/> 
	/// 
	/// NOTE: This task will also check the Manifest.xml to make sure that it has a &lt;versionName&gt; entry and that
	///       the version info matches as well.  If the entry is missing or incorrect, it will throw an error.
	/// </para>
	/// <para>
	/// The task declares the following properties:
	/// </para>
	/// <list type='table'>
	/// <listheader><term>Property</term><description>Description</description></listheader>
	/// <item><term>${test-version-file.packagename}</term><description>The name of the package.</description></item>
	/// <item><term>${test-version-file.targetversion}</term><description>The version number of the package</description></item>
	/// <item><term>${test-version-file.packagedir}</term><description>package directory: <b>path</b> to the package</description></item>
	/// </list>
	/// </remarks>
	[TaskName("test-version-file")]
	public class VerifyVersionFileTask : Task {
		class VersionInfo
		{
			public int count = 0;
			public int[] version = new int[4];
			public string[] line = new string[4];

			public override string ToString()
			{
				StringBuilder sb = new StringBuilder();
				for(int i = 0; i < count; i++)
				{
					sb.AppendFormat(i < count-1? "{0}." : "{0}", version[i]);
				}
				return sb.ToString();
			}
		}

		static string[] _versionFileNames = { "version.h", "package_version.h" };

		/// <summary>The name of a package comes from the directory name if the package is in the packages directory.  Use this attribute to name a package that lives outside the packages directory.</summary>
		/// <remarks>
		///   <para>The <c>name</c> attribute is used for packages that live outside of the packages directory.</para>
		/// </remarks>
		[TaskAttribute("packagename", Required = true)]
		public string PackageName { get; set; }

		/// <summary>The version of the package in development.</summary>
		[TaskAttribute("targetversion", Required = true)]
		public string TargetVersion { get; set; }

		/// <summary>The name of a package comes from the directory name if the package is in the packages directory.  Use this attribute to name a package that lives outside the packages directory.</summary>
		/// <remarks>
		///   <para>The <c>name</c> attribute is used for packages that live outside of the packages directory.</para>
		/// </remarks>
		[TaskAttribute("packagedir", Required = true)]
		public string PackageDir { get; set; }


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
					
					break;
				}
			}
		}

		protected override void ExecuteTask() {
			
			// Test Manifest.xml's version info as well.  It has been discussed previously to have Framework automatically modify the local
			// version of Manifest.xml before showing this message.  But if this task is done on a remote build machine in an automated process,
			// then we would ended up with a local modified writable copy of this file and the next time the remote build machine trying to do
			// the perforce sync will fail because there is a local modified file.  So it may not be safe to do too much and modify the file
			// for user.
			Manifest manifest = Manifest.Load(PackageDir);
			if (String.IsNullOrEmpty(manifest.Version))
			{
				throw new BuildException(String.Format("ERROR: Manifest.xml file is missing <versionName>{0}</versionName> entry under the <package> element!  Please add this entry and try again.\n", TargetVersion), Location);
			}
			else if (0 != String.Compare(TargetVersion,manifest.Version,true))
			{
				throw new BuildException(String.Format("ERROR: The Manifest.xml file has <versionName>{0}</versionName>.  But you are packaging for version {1}.  So it should have been <versionName>{1}</versionName>.  Please correct this entry and try again.\n", manifest.Version, TargetVersion), Location);
			}

			VersionInfo versionInfo = new VersionInfo();

			// Extract will return false for dev and other work and exotic versions. 
			// In this case we don't want to compare version to version.h file data
			if(ExtractVersionInfo(this.TargetVersion, versionInfo))
			{
				Check_CPP_Version_File(PackageDir, PackageName, versionInfo);
			}
			if (0 != String.Compare(TargetVersion, this.PackageVersion, true))
			{
				Log.Warning.WriteLine(LogPrefix + "package target version '{0}' does not correspond to the package version directory name {1}.\n", TargetVersion, PackageVersion);
			}
		}

		private void Check_CPP_Version_File(string packageDir, string packageName, VersionInfo packageVersion)
		{            
			string includeDir  = String.Empty;
			string versionFile = String.Empty;
			// Verify that include dir exists. Only then we look for version.h file
			foreach (string dir in Directory.GetDirectories(packageDir, "include"))
			{                
				if (0 == String.Compare(Path.GetFileName(dir), "include", true))
				{
					includeDir = dir;
					break;
				}
			}
			if(!String.IsNullOrEmpty(includeDir))
			{
				System.Collections.Specialized.StringCollection versionFileNames = new System.Collections.Specialized.StringCollection();
				versionFileNames.AddRange(_versionFileNames);
				versionFileNames.Add(String.Format("{0}_version.h", PackageName));

				foreach(string fileName in versionFileNames)
				{
					foreach(string foundFile in FindFiles(includeDir, fileName))
					{
						if (VerifyVersionFileData(foundFile, packageVersion))
						{
							versionFile = foundFile;
						}
					}
				}
				if(String.IsNullOrEmpty(versionFile))
				{
					Log.Warning.WriteLine(LogPrefix + "Package '{0}' does not contain 'version.h' file with version info in package 'include' directory. Add version.h with package version information.\n", PackageName);
				}
			}

		}

		private StringCollection FindFiles(string dir, string fileName)
		{
			StringCollection files = new StringCollection();            
			foreach (string file in Directory.GetFiles(dir, fileName, SearchOption.AllDirectories))
			{
				if (0 == String.Compare(Path.GetFileName(file), fileName, true))
				{
					files.Add(file);                                        
				}
			}
			return files;
		}

		private bool ExtractVersionInfo(string packageVersion, VersionInfo packageVersionInfo)
		{
			// using a regular expression look for a plausible version number and valid copyright date
			const string expression = @"(?<major>[0-9]+).(?<minor>[0-9]+).(?<build>[0-9]*)(?:.?)(?<revision>[0-9]*)";
			string[] groups = { "major", "minor", "build", "revision" };

			packageVersionInfo.count = 0;

			Match match = Regex.Match(packageVersion, expression);
			if (match.Success) 
			{
				try
				{
					foreach (string group in groups)
					{
						packageVersionInfo.version[packageVersionInfo.count] = Int32.Parse(match.Groups[group].Value);
						packageVersionInfo.count++;
					}
				}
				catch (Exception) { }
			}
			return packageVersionInfo.count > 2;
		}


		private bool VerifyVersionFileData(string versionFile, VersionInfo packageVersionInfo)
		{
			string expression = @"(\s*)#define(.*)((_VERSION_MAJOR(\s+)(?<major>[0-9]+))|(_VERSION_MINOR(\s+)(?<minor>[0-9]+))|(_VERSION_PATCH(\s+)(?<patch>[0-9]+)))";
			string expression1 = @"(\s*)#define(.*)((VERSION_MAJOR(\s+)(?<major>[0-9]+))|(VERSION_MINOR(\s+)(?<minor>[0-9]+))|(VERSION_PATCH(\s+)(?<patch>[0-9]+)))";
			string[] groups = { "major", "minor", "patch" };
			string[] labels = { "#define ..._VERSION_MAJOR", "#define ..._VERSION_MINOR", "#define ..._VERSION_PATCH" };
			VersionInfo fileVersionInfo = new VersionInfo();
			using (TextReader reader = File.OpenText(versionFile))
			{
				string line;                
				while ((line = reader.ReadLine()) != null)
				{
				   line = line.Trim();
				   if (line.StartsWith("//"))
					   continue;

				   Match match = Regex.Match(line, expression);
				   if (match.Success)
				   {
					   for (int i = 0; i < groups.Length; i++)
					   {
						   if (!String.IsNullOrEmpty(match.Groups[groups[i]].Value))
						   {
							   if (fileVersionInfo.line[i] != null && fileVersionInfo.line[i].Trim().Length > 0)
							   {
								   Log.Warning.WriteLine(LogPrefix + "\nWARNING:\nPackage: '{0}',\nfile: '{1}'\n\nhas multiple defines for the version information, unable to determine relevant define:\n{2}\n{3}\n", PackageName, versionFile, fileVersionInfo.line[i], line);                                   
								   return true;                                   
							   }

							   fileVersionInfo.version[i] = Int32.Parse(match.Groups[groups[i]].Value);
							   fileVersionInfo.count = Math.Max(i + 1, fileVersionInfo.count);
							   fileVersionInfo.line[i] = line;
						   }
					   }
				   }
				   else
				   {
					   match = Regex.Match(line, expression1);
					   if (match.Success)
					   {
						   for (int i = 0; i < groups.Length; i++)
						   {
							   if (!String.IsNullOrEmpty(match.Groups[groups[i]].Value))
							   {
								   Log.Warning.WriteLine(LogPrefix + "\nWARNING:\nPackage '{0}',\nfile '{1}'\nline: '{2}\n\nName of the version info define should be prepended by the package (or other unique) name to prevent name clash, for example '#define RWMATH_VERSION_MAJOR 1'\n", PackageName, versionFile, line);
								   return true;
							   }
						   }
					   }
				   }
			   }
		   }
		   if (fileVersionInfo.count > 0)
		   {
			   //Compare file has version info with package version info:

			   for (int i = 0; i < fileVersionInfo.count; i++)
			   {
				   if (fileVersionInfo.line[i] == null)
				   {
					   Log.Warning.WriteLine(LogPrefix + "\nWARNING:\nPackage: '{0}',\nfile: '{1}'\nis missing version definition: '{2} xxx'\n", PackageName, versionFile, labels[i]);
					   break;
				   }
				   if (fileVersionInfo.version[i] != packageVersionInfo.version[i])
				   {
					   Log.Warning.WriteLine(LogPrefix + "\nWARNING:\nPackage: '{0}',\nfile: '{1}'\n\nhas wrong version info. Package version='{2}', file version='{3}'.\n", PackageName, versionFile, packageVersionInfo.ToString(), fileVersionInfo.ToString());
					   break;
				   }
			   }


		   }
			// Return true if we found version info in the file.
		   return fileVersionInfo.count > 0;
		}
	}
}

