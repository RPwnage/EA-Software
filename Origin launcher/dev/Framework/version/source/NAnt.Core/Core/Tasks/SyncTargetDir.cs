// NAnt - A .NET build tool
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
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)

using System;
using System.IO;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Linq;
using System.Threading;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Shared;

namespace NAnt.Core.Tasks
{

	/// <summary>Synchronizes target directory file set(s) to a target dir and removes files from the target dir that aren't present in the file set(s).</summary>
	/// <remarks>
	///   <para>
	///   By default, files are copied if the source file is newer than the destination file, or if the 
	///   destination file does not exist. However, you can explicitly overwrite (newer) files with the 
	///   <c>overwrite</c> attribute set to true.</para>
	///   <para>
	///   A file set(s), defining groups of files using patterns, is copied.
	///   All the files matched by the file set will be 
	///   copied to that directory, preserving their associated directory structure.
	///   </para>
	///   <para>Any directories are created as needed by the <c>synctargetdir</c> task.</para>
	/// </remarks>
	[TaskName("synctargetdir", NestedElementsCheck = true)]
	public class SyncTargetDirTask : Task
	{
		bool _overwrite = false;
		bool _clobber = false;
		bool _maintainAttributes = false;
		bool _flatten = false;
		int _retrycount = 5;
		int _retryDelay = 100;
		bool _hardlink = false;
		bool _deleteEmptyDirs = true;
		protected readonly Dictionary<string, string> FileCopyMap = new Dictionary<string, string>();

		public SyncTargetDirTask() : base("synctargetdir") 
		{
			ExcludeFileSet.FailOnMissingFile = false;
			CopyFileSet.FailOnMissingFile = false;
		}

		/// <summary>The directory to transfer to, when transferring a file set.</summary>
		[TaskAttribute("todir", Required = true)]
		public string ToDirectory { get; set; } = null;

		/// <summary>Overwrite existing files even if the destination files are newer. Defaults is false.</summary>
		[TaskAttribute("overwrite")]
		public bool Overwrite { get { return (_overwrite); } set { _overwrite = value; } }

		/// <summary>Allow hidden and read-only files to be overwritten if appropriate (i.e. if source is newer or overwrite is set to true). Default is false.</summary>
		[TaskAttribute("clobber")]
		public bool Clobber { get { return (_clobber); } set { _clobber = value; } }

		/// <summary>Maintain file attributes of overwritten files. By default and if destination file does not exist file attributes are carried over from source file. Default is false.</summary>
		[TaskAttribute("maintainattributes")]
		public bool MaintainAttributes { get { return (_maintainAttributes); } set { _maintainAttributes = value; } }

		/// <summary>Flatten directory structure when transferring a file set. All files are placed in the <i>todir</i>, without duplicating the directory structure.</summary>
		[TaskAttribute("flatten")]
		public bool Flatten { get { return (_flatten); } set { _flatten = value; } }

		/// <summary>Number of times to retry the copy operation if copy fails.</summary>
		[TaskAttribute("retrycount")]
		public int RetryCount { get { return (_retrycount); } set { _retrycount = value; } }

		/// <summary>Length of time in milliseconds between retry attempts.</summary>
		[TaskAttribute("retrydelay")]
		public int RetryDelay { get { return (_retryDelay); } set { _retryDelay = value; } }

		/// <summary>Tries to create a hard link to the source file rather than copying the entire contents of the file.
		/// If unable to create a hard link it simply copies the file.</summary>
		[TaskAttribute("hardlinkifpossible")]
		public bool HardLink { get { return (_hardlink); } set { _hardlink = value; } }

		/// <summary>Delete empty folders under the target directory. Default:true</summary>
		[TaskAttribute("deleteemptydirs")]
		public bool DeleteEmptyDirs { get { return (_deleteEmptyDirs); } set { _deleteEmptyDirs = value; } }

		/// <summary>Do not print any info. Default:false</summary>
		[TaskAttribute("quiet")]
		public bool Quiet { get; set; } = false;

		/// <summary>Just copy the files only but do not do directory/fileset synchronization. Default:false</summary>
		[Property("copyfilesonly")]
		public PropertyElement CopyFilesOnly { get; } = new PropertyElement();

		/// <summary>The (single) file set to be copied.  
		/// Note the todir attribute must be set, and that a fileset 
		/// may include another fileset.</summary>
		[FileSet("fileset")]
		public FileSet CopyFileSet { get; } = new FileSet();

		/// <summary>Names of the filesets to synchronize</summary>
		[Property("fileset-names")]
		public PropertyElement FilesetNames { get; } = new PropertyElement();

		/// <summary>Files to exclude from synchronization. Files in this fileset will 
		/// not be deleted even if these files aren't present in the input files to copy.
		/// Note: 'excludefiles' fileset does not affect copy phase.
		/// </summary>
		[FileSet("excludefiles")]
		public FileSet ExcludeFileSet { get; } = new FileSet();

		protected override void ExecuteTask()
		{
			ToDirectory = Project.GetFullPath(ToDirectory);

			bool copyonly = false;
			if (CopyFilesOnly.Value != null)
				copyonly = Project.ExpandProperties(CopyFilesOnly.Value).ToBoolean();

			ProcessUtil.WindowsNamedMutexWrapper syncTargetDirLock = null;

			try
			{
				if (copyonly)
				{
					Log.Status.WriteLine(LogPrefix + "Copying files to target directory '{0}'.", ToDirectory);
				}
				else
				{
					string syncTargetDirLockName = ToDirectory.Replace('\\', '_').Replace('/', '_').Replace(':', '_');
					syncTargetDirLock = new ProcessUtil.WindowsNamedMutexWrapper(syncTargetDirLockName);

					Log.Status.WriteLine(LogPrefix + "Synchronizing target directory '{0}' (lock: {1}).", ToDirectory, syncTargetDirLockName);

					if (!syncTargetDirLock.HasOwnership)
					{
						syncTargetDirLock.WaitOne();
					}
				}

				ProcessFileSet(CopyFileSet);

				if (!String.IsNullOrEmpty(FilesetNames.Value))
				{
					foreach (var fsname in Project.ExpandProperties(FilesetNames.Value).ToArray())
					{
						FileSet fs;
						if (Project.NamedFileSets.TryGetValue(fsname, out fs))
						{
							ProcessFileSet(fs);
						}
						else
						{
							Log.Warning.WriteLine(LogPrefix + "Fileset '{0}' does not exist.", fsname);
						}
					}
				}

				if (!Directory.Exists(ToDirectory))
				{
					Directory.CreateDirectory(ToDirectory);
				}
				else if (!copyonly)
				{
					DeleteFiles();
				}

				// do all the actual copy operations now...
				CopyFiles();

				if (DeleteEmptyDirs)
				{
					DeleteEmptyDirectories();
				}
			}
			finally
			{
				if (syncTargetDirLock != null)
				{
					syncTargetDirLock.Dispose();
				}
			}
		}


		protected virtual void ProcessFileSet(FileSet fs)
		{
			// NOTE: when working with file and directory names its useful to 
			// use the FileInfo an DirectoryInfo classes to normalize paths like:
			// c:\work\nant\extras\buildserver\..\..\..\bin

			if (fs != null && fs.FileItems.Count > 0)
			{
				if (Flatten)
				{
					StringCollection existingFiles = new StringCollection();
					foreach (FileItem fileItem in fs.FileItems)
					{
						FileInfo srcInfo = new FileInfo(fileItem.FileName);
						if (srcInfo.Exists)
						{
							if (existingFiles.Contains(srcInfo.Name) && !(Overwrite))
							{
								Log.Status.WriteLine(LogPrefix + "File '" + fileItem.FileName + "' is not going to be copied because another file of that name has already been copied into the target this iteration.  Perhaps you didn't mean to use the flatten option, or else you want to specify the overwrite option to force this behaviour.");
							}
							else
							{
								existingFiles.Add(srcInfo.Name);
								string dstPath = Path.Combine(Project.GetFullPath(ToDirectory), srcInfo.Name);

								if (FileCopyMap.ContainsKey(dstPath))
								{
									if (!String.Equals(FileCopyMap[dstPath], fileItem.FileName, PathUtil.IsCaseSensitive? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase))
									{
										Log.Warning.WriteLine(LogPrefix + "Destination file '{0}' has two different source files '{1}' and '{2}'. File '{2}' will be copied to destination.", dstPath, FileCopyMap[dstPath], fileItem.FileName);
										FileCopyMap[dstPath] = fileItem.FileName;
									}
								}
								else
								{
									FileCopyMap.Add(dstPath, fileItem.FileName);
								}
							}
						}
					}
				}
				else
				{
					// Copy file set contents.

					// get the complete path of the base directory of the file set, ie, c:\work\nant\src
					DirectoryInfo srcFileSetBaseInfo = new DirectoryInfo(fs.BaseDirectory);
					DirectoryInfo dstBaseInfo = new DirectoryInfo(Project.GetFullPath(ToDirectory));

					// if source file not specified use file set
					foreach (FileItem fileItem in fs.FileItems)
					{
						FileInfo srcInfo = new FileInfo(fileItem.FileName);
						if (srcInfo.Exists)
						{
							var srcBaseInfo = fileItem.BaseDirectory == null ? srcFileSetBaseInfo : new DirectoryInfo(fileItem.BaseDirectory);
							// replace the file set path with the destination path
							// NOTE: big problems could occur if the file set base dir is rooted on a different drive
							if (!fileItem.FileName.StartsWith(srcBaseInfo.FullName))
							{
								Log.Status.WriteLine(LogPrefix + "File '" + fileItem.FileName + "' is not going to be copied because it is not contained in the file set's base directory '" + fs.BaseDirectory + "'.  Use the flatten option to copy it.");
							}
							string dstPath = fileItem.FileName.Replace(srcBaseInfo.FullName, dstBaseInfo.FullName);

							if (FileCopyMap.ContainsKey(dstPath))
							{
								if (!String.Equals(FileCopyMap[dstPath], fileItem.FileName, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase))
								{
									Log.Warning.WriteLine(LogPrefix + "Destination file '{0}' has two different source files '{1}' and '{2}'. File '{2}' will be copied to destination.", dstPath, FileCopyMap[dstPath], fileItem.FileName);
									FileCopyMap[dstPath] = fileItem.FileName;
								}
							}
							else
							{
								FileCopyMap.Add(dstPath, fileItem.FileName);
							}
						}
						else
						{
							string msg = String.Format(LogPrefix + "Could not find file {0} to copy.", fileItem.FileName);
							throw new BuildException(msg, Location);
						}
					}
				}
			}
		}


		protected void DeleteFiles()
		{
			var excludedfiles = new HashSet<string>(ExcludeFileSet.FileItems.Select(fi => fi.Path.Path), PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

			int deleted = 0;
			PathUtil.ProcessDirectoriesRecurcively(ToDirectory,
				(dir) =>
				{
					foreach (var file in Directory.GetFiles(dir))
					{
						if (!FileCopyMap.ContainsKey(file) && !excludedfiles.Contains(file))
						{
							if (Verbose)
								Log.Status.WriteLine(LogPrefix + "Deleting file {0}", file);

							PathUtil.DeleteFile(file, location: Location);

							deleted++;
						}
					}
				});

			if (!Quiet && deleted > 0)
			{
				Log.Status.WriteLine(LogPrefix + "Removed {0} files", deleted);
			}
		}

		protected void DeleteEmptyDirectories()
		{
			int deleted = 0;

			PathUtil.ProcessDirectoriesRecurcively(ToDirectory,
				(dir) => { if (Directory.GetDirectories(dir).Length == 0 && Directory.GetFiles(dir).Length == 0) { PathUtil.DeleteDirectory(dir, failOnError: false); deleted++; } } 
				);

			if (!Quiet && deleted > 0)
			{
				Log.Status.WriteLine(LogPrefix + "Removed {0} empty directories", deleted);
			}

		}


		/// <summary>
		/// Actually does the file (and possibly empty directory)
		/// transfers.
		/// </summary>
		protected virtual void CopyFiles()
		{
			int fileCount = FileCopyMap.Keys.Count;
			if (fileCount > 0 || Verbose)
			{
				if (ToDirectory != null)
				{
					Log.Info.WriteLine(LogPrefix + "Copying {0} files to {1}...", fileCount, Project.GetFullPath(ToDirectory));
				}
				else
				{
					Log.Info.WriteLine(LogPrefix + "Copying {0} files...", fileCount);
				}

				// loop through our file list
				foreach (var entry in FileCopyMap)
				{
					var dstPath = entry.Key;
					var sourcePath = entry.Value;

					if (sourcePath == dstPath)
					{
						Log.Info.WriteLine(LogPrefix + "Skipping self-copy of {0}." + sourcePath);
						continue;
					}

					CopyFlags copyFlags = (MaintainAttributes ? CopyFlags.MaintainAttributes : CopyFlags.None) |
						(Overwrite ? CopyFlags.None : CopyFlags.SkipUnchanged);
					LinkOrCopyMethod[] methods = HardLink ?
						new LinkOrCopyMethod[] { LinkOrCopyMethod.Hardlink, LinkOrCopyMethod.Copy } :
						new LinkOrCopyMethod[] { LinkOrCopyMethod.Copy };

					Log.Info.WriteLine(LogPrefix + "Copying {0} to {1}...", sourcePath, dstPath);
					LinkOrCopyCommon.LinkOrCopy
					(
						sourcePath,
						dstPath,
						Clobber ? OverwriteMode.OverwriteReadOnly : OverwriteMode.Overwrite,
						copyFlags,
						(uint)RetryCount,
						(uint)RetryDelay,
						methods
					);
				}
			}
		}
	}
}
