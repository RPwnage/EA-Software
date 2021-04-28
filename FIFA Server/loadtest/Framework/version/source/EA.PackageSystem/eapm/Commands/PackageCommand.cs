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
using System.Linq;
using System.IO;
using System.Xml;

using NAnt.Core.Util;
using NAnt.Core;
using EA.SharpZipLib;
using System.Collections.Specialized;
using NAnt.Core.Writers;
using System.Text;

namespace EA.PackageSystem.ConsolePackageManager
{
	[Command("package")]
	internal class PackageCommand : Command
	{
		internal override string Summary
		{
			get { return "Creates a package zip file from a package folder."; }
		}

		internal override string Usage
		{
			get { return "[packagename] [version] [-verify]"; } 
		}

		internal override string Remarks
		{
			get
			{
				return @"Description
	Creates a package zip file that can be posted to the package server.
	This command must be run from the root directory of the package, which contains the manifest and build files.
	By default this command works for non-flattened packages.  
	If you would like to use it on a flattened package please specify the Package name followed by the Package's version as arguments.

	The -verify flag can be used to verify a package's signature file.

Examples
	Package up the package in the current directory:
	> eapm package

	Package up a flattend package (Requires you to specify the package name and version):
	> eapm package EABase 3.0.0

	Verify a package's signature file:
	> eapm package -verify

	Verify a flattened package's signature file (Requires you to specify the package name and version):
	> eapm package EABase 3.0.0 -verify";
			} 
		}

		// This version should be updated if the signature file format changes significantly
		// Version 2 was the format created by nant's signature file generate whereas 3 is the one created by eapm
		private const double SignatureVersion = 3.0;

		private bool VerifySignature = false;

		private enum VerificationStatus { OK, NO_SIGNATURE, MISMATCH };

		internal override void Execute()
		{
			IEnumerable<string> positionalArgs = Arguments.Where(arg => !arg.StartsWith("-"));
			IEnumerable<string> optionalArgs = Arguments.Where(arg => arg.StartsWith("-"));

			VerifySignature = optionalArgs.Contains("-verify");
			
			string versionName = null;
			string packageName = null;
			bool isFlattened = false;
			if (positionalArgs.Count() > 0)
			{
				if(positionalArgs.Count() != 2)
					throw new ApplicationException("If trying to package a flattened package you must specify the package name followed by the package version, only received 1 parameters.");
				packageName = positionalArgs.ElementAt(0);
				versionName = positionalArgs.ElementAt(1);
				isFlattened = true;
			}

			try
			{
				DirectoryInfo currentFolder = new DirectoryInfo(".");
				if (versionName == null)
					versionName = Path.GetFileName(currentFolder.FullName);
				if (packageName == null)
					packageName = Path.GetFileName(Path.GetDirectoryName(currentFolder.FullName));

				string zipFileName = string.Format("{0}-{1}.zip", packageName, versionName);

				FileSet excludedFiles = LoadPackageIgnoreFile(zipFileName);

				if (VerifySignature)
				{
					VerifySignatureFile(currentFolder.FullName, packageName, versionName, excludedFiles);
					return;
				}

				// Validate that the directory the command is being run from contains the necessary files to be a package
				string[] files = Directory.GetFiles(".");
				string manifestFilePath = files.FirstOrDefault(x => x.ToLower().Equals("." + Path.DirectorySeparatorChar + "manifest.xml"));
				string buildFile = files.FirstOrDefault(x => x.ToLower().EndsWith(packageName.ToLower() + ".build"));

				if (manifestFilePath.IsNullOrEmpty())
				{
					throw new ApplicationException("Package must contain a Manifest.xml file.");
				}

				// load and verify the manifest and update the compatibility revision field
				UpdateManifestCompatibility(versionName, manifestFilePath);

				if (buildFile.IsNullOrEmpty())
				{
					if (isFlattened)
						throw new ApplicationException(currentFolder.FullName + "\\" + packageName + ".build not found!\nPackage must contain a .build file and its name must match the package name. ");
					else
						throw new ApplicationException(currentFolder.FullName + "\\" + packageName + ".build not found!\nPackage must contain a .build file and its name must match the package name.  This is derrived from the package folder name.");
				}

				// Create a signature file which is a hash of all the files in the package
				GenerateSignatureFile(currentFolder.FullName, packageName, versionName, excludedFiles);

				StringCollection filesToZip = GetAllPackageFiles(excludedFiles);

				try
				{
					Console.WriteLine("Zipping {0} files to {1}", filesToZip.Count, zipFileName);
					
					ZipLib zipLib = new ZipLib();
					zipLib.ZipFile(filesToZip,
						baseDirectory: currentFolder.FullName,
						zipFileName: zipFileName,
						zipEntryDir: Path.Combine(packageName, versionName),
						zipLevel: 6,
						useModTime: false);
				}
				catch (Exception e)
				{
					throw new ApplicationException(String.Format("Error creating zip file '{0}'", zipFileName), e);
				}
			}
			catch (Exception e) 
			{
				throw new ApplicationException("Failed to create package.", e);
			}
		}

		private StringCollection GetAllPackageFiles(FileSet excludedFiles)
		{
			StringCollection packageFiles = new StringCollection();
			foreach (string filepath in Directory.EnumerateFiles(Directory.GetCurrentDirectory(), "*", SearchOption.AllDirectories))
			{
				if (!excludedFiles.FileItems.Any(x => x.FullPath.Equals(filepath, PathUtil.PathStringComparison)))
				{
					packageFiles.Add(filepath);
				}
			}
			return packageFiles;
		}

		private string RemoveSuffixes(string version)
		{
			return version = version
				.Replace("-non-proxy", "")
				.Replace("-proxy", "")
				.Replace("-lite", "");
		}

		private FileSet LoadPackageIgnoreFile(string zipFileName)
		{
			FileSet excludedFiles = new FileSet();
			excludedFiles.Include(zipFileName, failonmissing: false);
			excludedFiles.Include("p4protocol.sync", failonmissing: false);
			excludedFiles.Include("p4protocol.manifest", failonmissing: false);

			string packageIgnorePath = Path.Combine(Directory.GetCurrentDirectory(), ".packageignore");
			if (File.Exists(packageIgnorePath))
			{
				string[] ignoreLines = File.ReadAllLines(packageIgnorePath);

				foreach (string line in ignoreLines)
				{
					if (line.StartsWith("#")) continue;

					string modifiedLine = line;
					if (modifiedLine.EndsWith("...")) modifiedLine = line.Substring(0, line.Length - 3) + "**";
					if (modifiedLine.StartsWith("!"))
					{
						excludedFiles.Exclude(modifiedLine.Substring(1));
					}
					else
					{
						excludedFiles.Include(modifiedLine, failonmissing: false);
					}
				}
			}
			return excludedFiles;
		}

		private void UpdateManifestCompatibility(string versionName, string manifestFilePath)
		{
			File.SetAttributes(manifestFilePath, FileAttributes.Normal);
			XmlDocument document = new XmlDocument();
			document.Load(manifestFilePath);
			XmlNode packageNode = document.GetChildElementByName("package");
			if (packageNode == null)
			{
				throw new ApplicationException("The root node of a Manifest.xml file must be <package>.");
			}
			XmlNode versionNameNode = packageNode.GetChildElementByName("versionName");
			if (versionNameNode == null)
			{
				throw new ApplicationException("The Manifest.xml file must contain a versionName field and that field must match the version folder of the package.");
			}
			else if (versionName != versionNameNode.InnerText)
			{
				Console.WriteLine("Desired package version ({0}) does not match the version number in the <versionName> field in the manifest ({1})! Updating... ", versionName, versionNameNode.InnerText);
				versionNameNode.InnerText = versionName;
				document.Save(manifestFilePath);
				Console.WriteLine();
				Console.WriteLine("  !!! Please submit updated Manifest file to source control !!!");
				Console.WriteLine();
			}
			XmlNode compatibilitynode = packageNode.GetChildElementByName("compatibility");
			if (compatibilitynode != null)
			{
				string revision = compatibilitynode.GetAttributeValue("revision");
				string plainVersionName = RemoveSuffixes(versionName);
				if (!revision.IsNullOrEmpty() && !revision.StartsWith(plainVersionName))
				{
					compatibilitynode.SetAttribute("revision", plainVersionName);
					document.Save(manifestFilePath);

					Console.WriteLine("Package revision number ({0}) does not match the version number({1})! Updating... ", revision, plainVersionName);
					Console.WriteLine();
					Console.WriteLine("  !!! Please submit updated Manifest file to source control !!!");
					Console.WriteLine();
				}
			}
		}

		private void GenerateSignatureFile(string directory, string packageName, string packageVersion, FileSet excludedFiles)
		{
			Console.WriteLine("Generating Signature file...");

			string filePath = Path.Combine(Directory.GetCurrentDirectory(), packageName + ".psf");
			string zipFilePath = Path.Combine(Directory.GetCurrentDirectory(), packageName + "-" + packageVersion + ".zip");

			if (File.Exists(filePath))
			{
				File.SetAttributes(filePath, FileAttributes.Normal);
			}

			using (var writer = new MakeWriter())
			{
				writer.FileName = filePath;

				writer.WriteLine("# Signature Version " + SignatureVersion.ToString());
				writer.WriteLine("# Signature file generated from {0}-{1}", packageName, packageVersion);

				StringBuilder combinedHash = new StringBuilder();
				string[] files = Directory.GetFiles(directory, "*.*", SearchOption.AllDirectories);
				foreach (string file in files)
				{
					if (excludedFiles.FileItems.Any(x => x.FullPath.Equals(file, PathUtil.PathStringComparison)))
					{
						continue;
					}

					if (file == filePath || file == zipFilePath)
					{
						// signature file has been added to package files fileset for use by packaging code, however
						// we don't want to include it in signature hash
						continue;
					}

					if (PathUtil.IsPathInDirectory(file, directory))
					{
						string hash = Hash.GetFileHashString(file);
						combinedHash.Append(hash);
						writer.WriteLine("{0} {1}", hash, PathUtil.RelativePath(file, directory, addDot: false).Quote());
					}
				}

				writer.WriteLine("{2}{0} \"{1} (Combined Hash)\"", Hash.GetHashString(combinedHash.ToString()), packageName, Environment.NewLine);
			}
			Console.WriteLine("Generated Signature file: " + filePath);
		}

		private void VerifySignatureFile(string directory, string packageName, string packageVersion, FileSet excludedFiles)
		{
			string filePath = Path.Combine(Directory.GetCurrentDirectory(), packageName + ".psf");
			string zipFilePath = Path.Combine(Directory.GetCurrentDirectory(), packageName + "-" + packageVersion + ".zip");

			VerificationStatus result = VerificationStatus.OK;

			List<string> nonMatchingFiles = new List<string>();
			List<string> newFiles = new List<string>();

			StringBuilder combinedHash = new StringBuilder();
			string extractedCombinedSignature = string.Empty;

			string[] files = Directory.GetFiles(directory, "*.*", SearchOption.AllDirectories);
			foreach (string file in files)
			{
				if (excludedFiles.FileItems.Any(x => x.FullPath.Equals(file, PathUtil.PathStringComparison)))
				{
					continue;
				}

				if (file == filePath || file == zipFilePath)
				{
					// signature file has been added to package files fileset for use by packaging code, however
					// we don't want to include it in signature hash
					continue;
				}
				if (PathUtil.IsPathInDirectory(file, directory))
				{
					newFiles.Add(PathUtil.RelativePath(file, directory));
				}
			}

			string fileSignatureVersionStr = null;

			using (StreamReader reader = new StreamReader(filePath))
			{
				string line;
				while ((line = reader.ReadLine()) != null)
				{
					if (line.StartsWith("# Signature Version"))
					{
						fileSignatureVersionStr = line.Substring(20);
						continue;
					}
					else if (line.StartsWith("#"))
					{
						// Ignore comment lines
						continue;
					}
					int ind = line.IndexOf(' ');
					if (ind != -1)
					{
						string signature = line.Substring(0, ind);
						string signaturePath = line.Substring(ind).TrimWhiteSpace();

						newFiles.Remove(signaturePath.TrimQuotes());

						PathString path = PathString.MakeCombinedAndNormalized(directory, signaturePath.TrimQuotes());

						if (File.Exists(path.Path))
						{
							var currentsignature = Hash.GetFileHashString(path.Path);
							combinedHash.Append(currentsignature);

							if (!String.Equals(signature, currentsignature, StringComparison.Ordinal))
							{
								result = VerificationStatus.MISMATCH;

								if (nonMatchingFiles == null)
								{
									nonMatchingFiles = new List<string>();
								}
								nonMatchingFiles.Add(signaturePath);
							}
						}
						else if (signaturePath.Contains("(Combined Hash)"))
						{
							extractedCombinedSignature = signature;
						}
						else
						{
							result = VerificationStatus.MISMATCH;
							if (nonMatchingFiles == null)
							{
								nonMatchingFiles = new List<string>();
							}
							nonMatchingFiles.Add(signaturePath);
						}
					}
				}
			}

			if (combinedHash.Length != 0 && extractedCombinedSignature != String.Empty)
			{
				string packageHash = Hash.GetHashString(combinedHash.ToString());
				if (!String.Equals(packageHash, extractedCombinedSignature, StringComparison.Ordinal))
				{
					result = VerificationStatus.MISMATCH;
					if (nonMatchingFiles == null)
					{
						nonMatchingFiles = new List<string>();
					}
					nonMatchingFiles.Add("(Combined Hash)");
				}
			}

			if (newFiles.Any())
			{
				result = VerificationStatus.MISMATCH;
			}

			if (result != VerificationStatus.OK)
			{
				foreach (string mismatchedFile in nonMatchingFiles)
				{
					Console.WriteLine("{0,-16}    diff:  {1}", String.Empty, mismatchedFile);
				}
				foreach (string newFile in newFiles)
				{
					Console.WriteLine("{0,-16}    new:   {1}", String.Empty, newFile);
				}
			}

			if (!double.TryParse(fileSignatureVersionStr, out double fileSignatureVersionValue) || fileSignatureVersionValue != SignatureVersion)
			{
				Console.WriteLine("Warning: The signature file is version {1} but is being verified against version {0}, " +
					"there may be differences due to changes in the format even if there are no changes to the files.", SignatureVersion, fileSignatureVersionValue);
			}

			if (result != VerificationStatus.OK)
			{
				switch (result)
				{
					case VerificationStatus.MISMATCH:
						Console.WriteLine("Failure: Package signature mismatch!");
						break;
					case VerificationStatus.NO_SIGNATURE:
						Console.WriteLine("Failure: Package missing signature file!");
						break;
					default:
						Console.WriteLine("Failure: Package signature verifcation failed due to internal error!");
						break;
				}
			}
			else
			{
				Console.WriteLine("Package Verification Successful!");
			}
		}

	}
}
