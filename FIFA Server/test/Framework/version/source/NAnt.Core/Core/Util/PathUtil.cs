// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.IO;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Threading;

using NAnt.Core.PackageCore;

namespace NAnt.Core.Util
{
	public static partial class PathUtil
	{
		private const string LongPathMagic = @"\\?\";

		static PathUtil()
		{
			var location = Assembly.GetExecutingAssembly().Location;
			IsCaseSensitive = !(File.Exists(location.ToUpperInvariant()) && File.Exists(location.ToLowerInvariant()));

			PathStringComparison = PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase;
		}

		public static readonly bool IsCaseSensitive;

		public static StringComparison PathStringComparison;

		public static string PathToWsl(string path)
		{
			string drive_letter = path.Substring(0, 1);
			// WSL requires drive letter to be small letter at all times.  But don't arbitrary make the entire path lower case because
			// Linux by default is a case sensitive file system and we should preserve original path's casing.
			path = path.Replace(drive_letter + ":", "/mnt/" + drive_letter.ToLowerInvariant());
			path = path.Replace(@"\", @"/"); // PathToUnix
			return path;
		}

		// HACK: This is a hacky fix added for Dylan to convert all paths to WSL paths without spending a lot of time fixing all our code to work with WSL
		public static string AllPathsToWsl(string input)
		{
			string converted = Regex.Replace(input, "([A-Z])[:][\\\\/]", m => string.Format("/mnt/{0}/", m.Groups[1].Value.ToLowerInvariant()));
			converted = converted.Replace("lib\\", "lib/");
			return converted;
		}

		/// <summary>Takes two paths and returns a relative path from the source path to the destination path.</summary>
		/// <param name="srcPath">The source path.</param>
		/// <param name="dstPath">The destination path.</param>
		/// <param name="failOnError">Whether this function should throw an exception if an error occurs.</param>
		/// <param name="addDot">Whether to add a dot, representing the current directory, to the beginning of the relative path.</param>
		/// <param name="separatorOptions">Indicates whether to preserve the separator or change it to match the current system.</param>
		public static string RelativePath(PathString srcPath, PathString dstPath, bool failOnError = false, bool addDot = false, DirectorySeparatorOptions separatorOptions = DirectorySeparatorOptions.CurrentSystem)
		{
			if (srcPath == null)
			{
				return String.Empty;
			}
			if (dstPath == null)
			{
				return srcPath.Path;
			}
			return RelativePath(srcPath.Path, dstPath.Path, failOnError, addDot, separatorOptions: separatorOptions);
		}

		/// <summary>Takes two paths and computes relative path and returns it if (dstPath + relativePath) is less than 260. Otherwise returns full path.</summary>
		/// <param name="srcPath">The source path.</param>
		/// <param name="dstPath">The destination path.</param>
		/// <param name="failOnError">Whether this function should throw an exception if an error occurs.</param>
		/// <param name="addDot">Whether to add a dot, representing the current directory, to the beginning of the relative path.</param>
		/// <param name="log">If log is not null - prints warning message when full path is returned.</param>

		public static string SafeRelativePath_OrFullPath(PathString srcPath, PathString dstPath, bool failOnError = false, bool addDot = false, Logging.Log log = null)
		{
			if (srcPath == null)
			{
				return String.Empty;
			}
			if (dstPath == null)
			{
				return srcPath.Path;
			}

			var relativePath = RelativePath(srcPath.Path, dstPath.Path, failOnError, addDot);

			// Need to check the combined length which the relative path is going to have
			// before choosing whether to relativize or not.  Some tools
			var combinedFilePath = Path.Combine(dstPath.Path, relativePath);

			if (combinedFilePath.Length >= 260 && !String.IsNullOrEmpty(srcPath.Path)) 
			{

				if (srcPath.Path.Length < 260)
				{
					if (log != null)
					{
						log.Warning.WriteLine("Relative path {0} combined with base path {1} is longer than 260 characters. Use full path {2}.", relativePath, combinedFilePath, srcPath.Path);
					}
					return srcPath.Path;
				}
				else 
				{
					log.Warning.WriteLine("Relative path {0} combined with base path {1} is longer than 260 characters and a full path {2} is longer that 260 characters.", relativePath, combinedFilePath, srcPath.Path);
				}
			}

			return relativePath;
		}

		/// <summary>Takes two paths and returns a relative path from the source path to the destination path.</summary>
		/// <param name="srcPath">The source path.</param>
		/// <param name="dstPath">The destination path.</param>
		/// <param name="failOnError">Whether this function should throw an exception if an error occurs.</param>
		/// <param name="addDot">Whether to add a dot, representing the current directory, to the beginning of the relative path.</param>
		/// <param name="separatorOptions">Indicates whether to preserve the directory separators or change to match current system.</param>
		public static string RelativePath(string srcPath, string dstPath, bool failOnError = false, bool addDot = false, DirectorySeparatorOptions separatorOptions = DirectorySeparatorOptions.CurrentSystem)
		{
			try
			{
				if (srcPath == dstPath)
				{
					return String.Empty;
				}

				srcPath = srcPath.TrimQuotes();
				dstPath = dstPath.TrimQuotes().EnsureTrailingSlash();

				if (srcPath != null && srcPath.StartsWith(dstPath, StringComparison.Ordinal))
				{
					if (addDot)
					{
						return "." + Path.DirectorySeparatorChar + srcPath.Substring(dstPath.Length).TrimLeftSlash();
					}
					return srcPath.Substring(dstPath.Length).TrimLeftSlash();
				}
				Uri u1 = new Uri(srcPath, UriKind.RelativeOrAbsolute);
				if (u1.IsAbsoluteUri)
				{
					// On Linux need to add dot to get correct relative path.On Windows we need to ensure there is no trailing
					Uri u2 = UriFactory.CreateUri(dstPath + '.');
					PathString relative = PathString.MakeNormalized(Uri.UnescapeDataString(u2.MakeRelativeUri(u1).ToString()), 
						PathString.PathParam.NormalizeOnly, separatorOptions: separatorOptions);
					return relative.Path;
				}
			}
			catch (Exception)
			{
				if (failOnError)
				{
					throw;
				}
			}
			return srcPath;
		}

		/// <summary>Return the drive letter as string</summary>
		/// <param name="path">The path whose drive letter we are interested in.</param>
		/// <returns>The drive letter as a string, or null if no drive letter can be determined.</returns>
		public static string GetDriveLetter(string path)
		{
			if (String.IsNullOrEmpty(path))
			{
				return null;
			}

			// Some versions of Runtime do not have MacOSX definition. Use integer value 6
			if (Environment.OSVersion.Platform == PlatformID.Unix ||
			   (int)Environment.OSVersion.Platform == 6)
			{
				if (path[0] == Path.DirectorySeparatorChar)
				{
					int i = path.IndexOf(Path.DirectorySeparatorChar, 1);
					if (i != -1)
					{
						return path.Substring(0, i);
					}
				}
				return String.Empty;
			}

			int ind = path.IndexOf(':');

			if (ind < 0) // It must be a network path. return host name as a drive letter
			{
				path = path.TrimStart(PATH_SEPARATOR_CHARS_ALT);
				ind = path.IndexOfAny(PATH_SEPARATOR_CHARS_ALT);
			}
			if (ind < 0)
			{
				return null;
			}
			return path.Substring(0, ind);
		}

		public static string GetExecutingAssemblyLocation()
		{
			var location = Assembly.GetExecutingAssembly().Location;
			return Path.GetDirectoryName(IsCaseSensitive ? location : location.ToLower());
		}

		/// <summary>Returns the path to the drive letter directory on windows (or /mnt/[drive] under Windows Subsystem for Linux) and to the home directory on Unix host.  Mainly used to setup 'nant.drive' system property.</summary>
		/// <param name="assemblyLocation">The path to the assembly whose root directory the function should return, if null it will get the location of the executing assembly</param>
		public static string GetAssemblyRootDir(string assemblyLocation = null)
		{
			if (assemblyLocation == null)
			{
				assemblyLocation = GetExecutingAssemblyLocation();
			}
			
			string assemblyDrive = Path.GetPathRoot(assemblyLocation);
			if (PlatformUtil.IsUnix && PlatformUtil.IsOnWindowsSubsystemForLinux && assemblyLocation.StartsWith("/mnt/") && assemblyLocation[6] == '/')
			{
				assemblyDrive = assemblyLocation.Substring(0, 6);
			}
			else if (PlatformUtil.IsUnix || PlatformUtil.IsOSX)
			{
				string homedir = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
				if (assemblyLocation.StartsWith(homedir) ||
					assemblyDrive == "/")       // We can't really use root folder for osx or unix environment.
				{
					assemblyDrive = homedir;
				}
				if (assemblyDrive.EndsWith("/"))
				{
					assemblyDrive = assemblyDrive.TrimEnd(new char[] { '/' });      // Try to make output format consistent
				}
			}
			else if (assemblyDrive.EndsWith("\\"))
			{
				assemblyDrive = assemblyDrive.TrimEnd(new char[] { '\\' });         // Try to make output format consistent
			}

			return assemblyDrive;
		}
		
		/// <summary>Checks whether a path is a subdirectory of a projects build root directory.</summary>
		public static bool IsPathInBuildRoot(Project project, string path)
		{
			string buildroot = project.Properties[Project.NANT_PROPERTY_PROJECT_BUILDROOT];
			if (buildroot != null)
			{
				path = project.GetFullPath(path);
				buildroot = project.GetFullPath(project.ExpandProperties(buildroot));
				buildroot = buildroot.TrimEnd(new char[] { '\\', '/' }) + Path.DirectorySeparatorChar;

				return IsPathInDirectory(path, buildroot);
			}
			return false;
		}

		/// <summary>
		/// Test the input path is in the package roots that is listed in your masterconfig.
		/// Note that if PackageMap is not initialized (for tools like eapm that doesn't
		/// load build file and masterconfig), this function will always return false.
		/// </summary>
		public static bool IsPathInPackageRoots(string path)
		{
			if (!PackageMap.IsInitialized)
			{
				// This function is probably being used by a tool like eapm that doesn't load a build
				// file and masterconfig.
				return false;
			}
			foreach (var root in PackageMap.Instance.PackageRoots)
			{
				if (IsPathInDirectory(path, root.Dir.FullName))
				{
					return true;
				}
			}
			return false;
		}

		/// <summary>
		///  Test that the given source to destination copy allow copy with hard link usage. We currently
		///  disallow hard link usage if source file is in source tree. File attributes are part of the file
		///  not the link so changing one link modifies them all. Files inside the source tree have file
		///  attributes that are usually tied to source control (such as Perforce) so we don't want to modify
		///  them if we can avoid it.
		/// </summary>
		public static bool TestAllowCopyWithHardlinkUsage(Project project, string source, string destination)
		{
			bool allowHardLinkUsage = true;
			if (!IsPathInBuildRoot(project, source) && IsPathInPackageRoots(source))
			{
				allowHardLinkUsage = false;
			}
			return allowHardLinkUsage;
		}

		/// <summary>Checks whether the given path is equal to or a sub directory of the given directory.</summary>
		/// <param name="path">The path that is tested to see if it is within the directory.</param>
		/// <param name="dir">The directory that is tested to see if it contains the path.</param>
		/// <returns>Return true if the path is within the directory and false otherwise.</returns>
		public static bool IsPathInDirectory(string path, string dir)
		{
			if (path == dir) return true;

			return path.StartsWith(dir.EnsureTrailingSlash(), PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase);
		}

		/// <summary>Checks if a string is a valid path string, ie. contains no invalid symbols.</summary>
		public static bool IsValidPathString(string path)
		{
			return (-1 == path.IndexOfAny(Path.GetInvalidPathChars())) && !path.ContainsMSBuildMacro();
		}

		private static readonly char[] PATH_SEPARATOR_CHARS_ALT = new char[] { Path.DirectorySeparatorChar, PathNormalizer.AltDirectorySeparatorChar };


		public static void DeleteFile(string path, bool failonmissing=false, Location location = null, bool failonerror=true)
		{
			if (File.Exists(path))
			{
				try
				{
					
					int count = 0;
					const int maxIters = 10;
					while (File.Exists(path))
					{
						++count;
						try
						{
							File.SetAttributes(path, FileAttributes.Normal);
							File.Delete(path);
						}
						catch (Exception)
						{
							if (count >= maxIters) throw;
						}

						if (count >= maxIters) break;
						System.Threading.Thread.Sleep(100);
					}
				}
				catch (Exception e)
				{
					if (failonerror && File.Exists(path))
					{
						string msg = String.Format("Cannot delete file '{0}'.", path);
						if (location != null)
						{
							throw new BuildException(msg, location, e);
						}
						else
						{
							throw new BuildException(msg, e);
						}
					}
				}

				// verify that the file has been deleted
				if (failonerror && File.Exists(path))
				{
					string msg = String.Format("Cannot delete file '{0}' because of an unknown reason.", path);
					if (location != null)
					{
						throw new BuildException(msg, location);
					}
					else
					{
						throw new BuildException(msg);
					}
				}

			}
			else
			{
				if (failonmissing)
				{
					string msg = String.Format("Cannot delete '{0}' because file is missing.", path);
					if (location != null)
					{
						throw new BuildException(msg, location);
					}
					else
					{
						throw new BuildException(msg);
					}
				}
			}
		}

		/// <summary>
		/// Deletes a directory with all content. File attributes are reset to normal before delete.
		/// </summary>
		/// <param name="path">The path to the directory</param>
		/// <param name="verify">Verify that directory does not exist after delete and throw exception otherwise.</param>
		/// <param name="failOnError">Fail in the case of error if true, otherwise ignore errors</param>
		public static void DeleteDirectory(string path, bool verify = true, bool failOnError = true)
		{

			// JB - If deletion issues persist then we can try a DFS deletion from the bottom up. Have to account for hard links though
			if (Directory.Exists(path))
			{
				try
				{
					int count = 0;
					const int maxIters = 2000;
					SetAllFileAttributesToNormal(path);					
					try
					{
						Directory.Delete(Longify(path), true);
					}
					catch(Exception)
					{// try to delete once more due to a handle being open on the directory
						Directory.Delete(Longify(path), true);
					}

					while (Directory.Exists(path) && count < maxIters)
					{
						Thread.Sleep(1);
						count++;
					}
				}
				catch (Exception e)
				{
					if (failOnError)
					{
						string msg = String.Format("Cannot delete directory '{0}'.", path);
						throw new BuildException(msg, e);
					}
				}

				// It's still here, try to rename first, then delete (this will also likely fail if the above attempt is stuck, worth a try)
				if (Directory.Exists(path))
				{
					string toDeletePath = path + ".todelete";
					bool moved = false;
					try
					{
						Directory.Move(path, toDeletePath);
						moved = true;
					}
					catch(Exception ex)
					{
						if (failOnError)
						{
							string msg = String.Format("Unable to move directory while preparing to delete: '{0}'.", path);
							throw new BuildException(msg, ex);
						}
					}
					if (moved)
					{
						Directory.Delete(toDeletePath, true);
					}
				}

				// verify that the directory has been deleted
				if (verify && failOnError && Directory.Exists(path))
				{
					string msg = String.Format("Cannot delete directory '{0}' because of an unknown reason.", path);
					throw new BuildException(msg);
				}
			}
		}

		/// <summary>
		/// Move a directory from one destination to another. Note that operation will fail if you're moving one directory to a different volume or
		/// if source and destination is the same. If the operation fails it will retry several times and throw an exception if the retries fail.
		/// </summary>
		/// <param name="sourceDir">The source directory to move to</param>
		/// <param name="destDir">Verify that directory does not exist after delete and throw exception otherwise.</param>
		/// <param name="failOnError">Fail in the case of error if true, otherwise ignore errors</param>
		/// <param name="iterations"></param>
		/// <param name="waitMs"></param>
		public static void MoveDirectory(string sourceDir, string destDir, bool failOnError = true, int iterations = 20, int waitMs = 100)
		{
			string parent = Directory.GetParent(destDir).FullName;
			if (!Directory.Exists(parent))
			{
				Directory.CreateDirectory(parent);
			}

			try
			{
				int count = 0;
				while (true)
				{
					++count;
					try
					{
						Directory.Move(sourceDir, destDir);
						break;
					}
					catch (Exception)
					{
						if (count >= iterations) throw;
					}

					Thread.Sleep(waitMs);
				}
			}
			catch (Exception e)
			{
				if (failOnError)
				{
					string msg = String.Format("Cannot move directory '{0}' to '{1}'.", sourceDir, destDir);
					throw new BuildException(msg, e);
				}
			}

		}

		public static void ProcessDirectoriesRecurcively(string path, Action<string> action)
		{
			if (action != null && !String.IsNullOrEmpty(path) && Directory.Exists(path))
			{
				foreach (var dir in Directory.GetDirectories(path))
				{
					ProcessDirectoriesRecurcively(dir, action);
				}

				action(path);
			}
		}

		public static bool TryGetFilePathFileNameInsensitive(string path, out string existingPath)
		{
			// check if path exactly as given exists first
			if (File.Exists(path))
			{
				existingPath = path;
				return true;
			}
			else if (!IsCaseSensitive)
			{
				// on case insensitive filesystem we can fail immediately
				existingPath = null;
				return false;
			}

			// iterator through files in directory and accept the first tile that matches
			// request name insensitively
			string directory = Path.GetDirectoryName(path);
			string fileName = Path.GetFileName(path);
			if (Directory.Exists(directory))
			{
				foreach (string file in Directory.EnumerateFiles(directory))
				{
					if (file.Equals(fileName, StringComparison.OrdinalIgnoreCase))
					{
						existingPath = Path.Combine(directory, fileName);
						return true;
					}
				}
			}

			existingPath = null;
			return false;
		}

		/// <summary>For each file and directory in the given path set the file attributes to Normal. Will NOT enter symlinked directories</summary>
		private static void SetAllFileAttributesToNormal(string path)
		{
			var directoryInfo = new DirectoryInfo(Longify(path));

			// clear any read-only flags on directories
			directoryInfo.Attributes &= ~FileAttributes.ReadOnly;

			// clear any read-only flags on files
			foreach (var fileInfo in directoryInfo.GetFiles())
			{
				try
				{
					fileInfo.Attributes &= ~FileAttributes.ReadOnly;
				}
				catch (Exception ex)
				{
					if (File.Exists(path))
					{
						string msg = String.Format("Could not set file attribute to normal '{0}'.", path);
						throw new BuildException(msg, ex);
					}
				}
			}

			// recurse into sub directories to do the same
			foreach (var subDir in directoryInfo.GetDirectories())
			{
				if ((subDir.Attributes & FileAttributes.ReparsePoint) != 0)
					continue;
				SetAllFileAttributesToNormal(subDir.FullName);
			}
		}

		/// <summary>
		/// Check if a file is inaccessible by doing a simple file open test.  If it failed, it can be either
		/// user has no access permission to the file or the file has exclusive lock by another process.
		/// NOTE:  This function is really only useful on PC.  On OSX and Unix, we can't detect a lock by
		/// doing a simple file open.
		/// </summary>
		/// <param name="filepath">The path to the file</param>
		public static bool IsFileNotAccessible(string filepath)
		{
			try
			{
				using (FileStream strm = File.Open(filepath, FileMode.Open, FileAccess.Read))
				{
					return false;
				}
			}
			catch
			{
				return true;
			}
		}

		/// <summary>
		/// Find the first file that cannot be accessed from the given directory.  This function will search through all subdirectories as well.
		/// </summary>
		/// <param name="path">The path to the file</param>
		/// <param name="firstNotAccessibleFile">This parameter returns the first file being found that has access issue (or null if nothing is found)</param>
		public static bool FindFirstNotAccessibleFileInDirectory(string path, out string firstNotAccessibleFile)
		{
			bool foundFile = false;

			firstNotAccessibleFile = null;

			foreach (string file in Directory.EnumerateFiles(path, "*.*", SearchOption.AllDirectories))
			{
				if (IsFileNotAccessible(file))
				{
					firstNotAccessibleFile = file;
					foundFile = true;
					break;
				}
			}

			return foundFile;
		}

		private static string Longify(string path)
		{
			if (PlatformUtil.IsWindows && !path.StartsWith(LongPathMagic))
			{
				return LongPathMagic + path;
			}
			return path;
		}
	}
}
