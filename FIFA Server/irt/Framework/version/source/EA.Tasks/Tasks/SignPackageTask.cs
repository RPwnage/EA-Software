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
using System.IO;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Events;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Writers;

namespace EA.FrameworkTasks
{
	/// <summary>
	/// Creates package signature file
	/// </summary>
	[TaskName("signpackage")]
	public class SignPackageTask : Task
	{
		/// <summary>Name of the fileset containing all package files</summary>
		[TaskAttribute("packagefiles-filesetname", Required = true)]
		public string PackageFilesFilesetName { get; set; }

		protected override void ExecuteTask()
		{
			Log.Minimal.WriteLine("Warning: The signpackage task is deprecated and will soon be removed in a coming Framework 8 release. " +
				"We are considering removing the signature file feature entirely because it has been around for a long time and we have not encountered many situations where it has actually been useful." +
				"You may see this message if you are using an old version of eaconfig that still uses these tasks in the packaging target, in which case you can ignore it or can use the new \"eapm package\" command as an alternative for basic packaging." +
				"If you have overridden the packge target to do more complicated packaging and are using this task please remove it.");

			FileSet packageFiles;

			if (Project.NamedFileSets.TryGetValue(PackageFilesFilesetName, out packageFiles))
			{
				var signaturefile = CreateSignatureFile(packageFiles, PathString.MakeNormalized(Project.GetPropertyValue("package.dir")), Project.GetPropertyValue("package.name"));
			}
			else
			{
				var msg = String.Format("signpackage task: fileset '{0}' does not exist.", PackageFilesFilesetName);
				throw new BuildException(msg, Location);
			}
		}

		private  PathString CreateSignatureFile(FileSet packagefiles, PathString packagedir, string packageName)
		{
			var timer = new Chrono();

			var signaturefile = GetSignatureFilePath(packagedir, packageName);

			packagefiles.Include(signaturefile.Path, failonmissing: false); // packageFiles is a reference to the fileset that will be used during package, add the signature file so it also gets added to the .zip
			packagefiles.Exclude(signaturefile.Path + ".bak"); // signature file is written using cached writer which will create .bak on rewrite we don't want to package this or include it in signing

			int filecount = 0;

			using(var writer = new MakeWriter())
			{
				writer.FileName = signaturefile.Path;

				writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnSignatureFileUpdate);

				string pkgVersion = Project.Properties.GetPropertyOrFail($"package.{packageName}.version");

				writer.WriteLine("# Signature Version 2.0");
				writer.WriteLine("# Signature file generated from {0}-{1}", packageName, pkgVersion);

				StringBuilder combinedHash = new StringBuilder();
				foreach (var fi in packagefiles.FileItems)
				{
					if (fi.Path == signaturefile)
					{
						// signature file has been added to package files fileset for use by packaging code, however
						// we don't want to include it in signature hash
						continue;
					}

					if (PathUtil.IsPathInDirectory(fi.Path.Path, packagedir.Path))
					{
						string hash = Hash.GetFileHashString(fi.Path.Path);
						combinedHash.Append(hash);
						writer.WriteLine("{0} {1}", hash, PathUtil.RelativePath(fi.Path.Path, packagedir.Path, addDot: false).Quote());
						filecount++;
					}
				}

				writer.WriteLine("\n{0} \"{1} (Combined Hash)\"", Hash.GetHashString(combinedHash.ToString()), packageName);
			}

			Log.Status.WriteLine();
			Log.Status.WriteLine(LogPrefix + "Signed package '{0}', hashed {1} files {2}", packageName, filecount, timer.ToString());
			return signaturefile;
		}

		protected void OnSignatureFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				if (!File.Exists(e.FileName))
				{
					Log.Status.WriteLine("{0}{1} package signature file '{2}'", LogPrefix, "    Creating", Path.GetFileName(e.FileName));
					Log.Status.WriteLine();
					Log.Status.WriteLine("  !!! Update source control with new signature file !!!");
					Log.Status.WriteLine();
				}
				else
				{
					if (e.IsUpdatingFile && (File.GetAttributes(e.FileName) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
					{
						Log.Warning.WriteLine("{0} package signature file '{1}' is readonly. Removing readonly flag", LogPrefix, Path.GetFileName(e.FileName));
						File.SetAttributes(e.FileName, File.GetAttributes(e.FileName) & ~FileAttributes.ReadOnly & ~FileAttributes.Hidden);
					}
					
					string message = string.Format("{0}{1} package signature file '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
					if (e.IsUpdatingFile)
						Log.Minimal.WriteLine(message);
					else
						Log.Status.WriteLine(message);
				}
			}
		}

		public static PathString GetSignatureFilePath(PathString packagedir, string packageName)
		{
			return PathString.MakeCombinedAndNormalized(packagedir.Path, packageName + ".psf");
		}
	}
}
