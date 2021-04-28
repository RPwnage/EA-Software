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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using NAnt.Core.Util;

namespace EA.FrameworkTasks 
{
	/// <summary>
	/// [Deprecated] Verify if package content matches signature
	/// </summary>
	/// <remarks>
	/// <para>This task is used to verify that the package has an up to date Signature File.
	/// This can also be done using the target "verify-signature", which is the recommended way to preform this action.
	/// The task version is mainly for internal use and should only be used if you are writing a custom target that needs to preform additional steps.</para>
	/// </remarks>
	[TaskName("verify-package-signature")]
	public class VerifyPackageSignatureTask : Task
	{
		public enum VerificationStatus { OK, NO_SIGNATURE, MISMATCH };

		/// <summary>Name of the fileset containing all package files</summary>
		[TaskAttribute("packagefiles-filesetname", Required = true)]
		public string PackageFilesFilesetName { get; set; }

        protected override void ExecuteTask()
        {
			Log.Minimal.WriteLine("Warning: The verify-package-signature task is deprecated and will soon be removed in a coming Framework 8 release. " +
				"We are considering removing the signature file feature entirely because it has been around for a long time and we have not encountered many situations where it has actually been useful." +
				"You may see this message if you are using an old version of eaconfig that still uses these tasks in the packaging target, in which case you can ignore it or can use the new \"eapm package\" command as an alternative for basic packaging." +
				"If you have overridden the packge target to do more complicated packaging and are using this task please remove it.");

			string packageDir = Project.Properties["package.dir"];
			string packageName = Project.Properties["package.name"];

			if (!Project.NamedFileSets.TryGetValue(PackageFilesFilesetName, out FileSet packageFiles))
			{
				throw new BuildException(String.Format("verify-package-signature task: fileset '{0}' does not exist.", PackageFilesFilesetName), Location);
			}

			Log.Status.WriteLine();
            Log.Status.WriteLine("Signature verification:");
            Log.Status.WriteLine();

			VerificationStatus match = VerifyPackageSignature(PathString.MakeNormalized(packageDir), packageName, packageFiles, out List<string> nonMatchingFiles, out List<string> newFiles);

			string pkgVersion = Project.Properties.GetPropertyOrFail($"package.{packageName}.version");

            Log.Status.WriteLine("{0,-16} {1,-40} {2}", match, (packageName + "-" + pkgVersion), packageDir);
            if (match != VerificationStatus.OK)
            {
                foreach (string mismatchedFile in nonMatchingFiles)
                {
                    Log.Status.WriteLine("{0,-16}    diff:  {1}", String.Empty, mismatchedFile);
                }
				foreach (string newFile in newFiles)
				{
					Log.Status.WriteLine("{0,-16}    new:   {1}", String.Empty, newFile);
				}
            }
            Log.Status.WriteLine();

            if (match != VerificationStatus.OK)
            {
                FailTask fail = new FailTask();
                fail.Project = Project;
                switch (match)
                {
                    case VerificationStatus.MISMATCH:
                        fail.Message = "Package signature mismatch!";
                        break;
                    case VerificationStatus.NO_SIGNATURE:
                        fail.Message = "Package missing signature file!";
                        break;
                    default:
                        fail.Message = "Package signature verifcation failed due to internal error!";
                        break;
                }
                fail.Execute();
            }
		}

		public static VerificationStatus VerifyPackageSignature(PathString packageDir, string packagename, FileSet packageFiles, out List<string> nonMatchingFiles, out List<string> newFiles)
		{
			PathString signatureFile = SignPackageTask.GetSignatureFilePath(packageDir, packagename);

			packageFiles.Include(signatureFile.Path); // packageFiles is a reference to the fileset that will be used during package, add the signature file so it also gets added to the .zip
			packageFiles.Exclude(signatureFile.Path + ".bak"); // signature file is written using cached writer which will create .bak on rewrite we don't want to package this or include it in signing

			nonMatchingFiles = new List<string>();
			newFiles = new List<string>();

			if (!File.Exists(signatureFile.Path))
			{
				return VerificationStatus.NO_SIGNATURE;
			}

			VerificationStatus res = VerificationStatus.OK;
	
			System.Text.StringBuilder combinedHash = new System.Text.StringBuilder();
			string extractedCombinedSignature = String.Empty;

			foreach (FileItem fi in packageFiles.FileItems)
			{
				if (PathUtil.IsPathInDirectory(fi.Path.Path, packageDir.Path))
				{
					newFiles.Add(PathUtil.RelativePath(fi.Path.Path, packageDir.Path));
				}
			}

			using (StreamReader reader = new StreamReader(signatureFile.Path))
			{
				string line;
				while ((line = reader.ReadLine()) != null)
				{
					if (line.StartsWith("#"))
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

						PathString path = PathString.MakeCombinedAndNormalized(packageDir.Path, signaturePath.TrimQuotes());

						if (File.Exists(path.Path))
						{
							var currentsignature = Hash.GetFileHashString(path.Path);
							combinedHash.Append(currentsignature);

							if (!String.Equals(signature, currentsignature, StringComparison.Ordinal))
							{
								res = VerificationStatus.MISMATCH;

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
							res = VerificationStatus.MISMATCH;
							if (nonMatchingFiles == null)
							{
								nonMatchingFiles = new List<string>();
							}
							nonMatchingFiles.Add(signaturePath);
						}
					}
				}

				if (combinedHash.Length != 0 && extractedCombinedSignature != String.Empty)
				{
					string packageHash = Hash.GetHashString(combinedHash.ToString());
					if (!String.Equals(packageHash, extractedCombinedSignature, StringComparison.Ordinal))
					{
						res = VerificationStatus.MISMATCH;
						if (nonMatchingFiles == null)
						{
							nonMatchingFiles = new List<string>();
						}
						nonMatchingFiles.Add("(Combined Hash)");
					}
				}

				if (newFiles.Any())
				{
					res = VerificationStatus.MISMATCH;
				}
			}
			return res;
		}
	}
}
