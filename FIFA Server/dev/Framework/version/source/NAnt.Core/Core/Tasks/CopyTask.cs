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
using System.Collections;
using System.Collections.Specialized;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Shared;
using System.Collections.Generic;

namespace NAnt.Core.Tasks
{

	/// <summary>Copies a file or file set to a new location.</summary>
	/// <remarks>
	///   <para>
	///   By default, files are copied if the source file is newer than the destination file, or if the 
	///   destination file does not exist. However, you can explicitly overwrite (newer) files with the 
	///   <c>overwrite</c> attribute set to true.</para>
	///   <para>
	///   A file set, defining groups of files using patterns, can be copied if 
	///   the <c>todir</c> attribute is set.  All the files matched by the file set will be 
	///   copied to that directory, preserving their associated directory structure.
	///   Beware that <c>copy</c> handles only a single fileset, but note that
	///   a fileset may include another fileset.
	///   </para>
	///   <para>Any directories are created as needed by the <c>copy</c> task.</para>
	///   <para>There is another task available that is similar to the copy task, the &lt;synctargetdir&gt; task.
	///   The difference is that it tries to ensure two directories are in sync by only copying 
	///   files with changes and deleting files that are no longer existent in the source fileset.
	///   </para>
	/// </remarks>
	/// <include file='Examples/Copy/SingleFile.example' path='example'/>
	/// <include file='Examples/Copy/FileSet.example' path='example'/>
	/// <include file='Examples/Copy/BaseDirectory.example' path='example'/>
	/// <include file='Examples/Copy/DuplicateFile.example' path='example'/>
	/// <include file='Examples/Copy/CopyFlatten.example' path='example'/>
	[TaskName("copy", NestedElementsCheck = true)]
	public class CopyTask : Task
	{
		/// <summary>The file to transfer in a single file transfer operation.</summary>
		[TaskAttribute("file")]
		public string SourceFile { get; set; } = null;

		/// <summary>The file to transfer to in a single file transfer operation.</summary>
		[TaskAttribute("tofile")]
		public string ToFile { get; set; } = null;

		/// <summary>The directory to transfer to, when transferring a file set.</summary>
		[TaskAttribute("todir")]
		public string ToDirectory { get; set; } = null;

		/// <summary>Overwrite existing files even if the destination files are newer. Defaults is false.</summary>
		[TaskAttribute("overwrite")]
		public bool Overwrite { get; set; } = false;

		/// <summary>Allow hidden and read-only files to be overwritten if appropriate (i.e. if source is newer or overwrite is set to true). Default is false.</summary>
		[TaskAttribute("clobber")]
		public bool Clobber { get; set; } = false;

		/// <summary>Maintain file attributes of overwritten files. By default and if destination file does not exist file attributes are carried over from source file. Default is false.</summary>
		[TaskAttribute("maintainattributes")]
		public bool MaintainAttributes { get; set; } = false;

		/// <summary>Flatten directory structure when transferring a file set. All files are placed in the <i>todir</i>, without duplicating the directory structure.</summary>
		[TaskAttribute("flatten")]
		public bool Flatten { get; set; } = false;

		/// <summary>Number of times to retry the copy operation if copy fails.</summary>
		[TaskAttribute("retrycount")]
		public int RetryCount { get; set; } = 5;

		/// <summary>Length of time in milliseconds between retry attempts.</summary>
		[TaskAttribute("retrydelay")]
		public int RetryDelay { get; set; } = 100;

		/// <summary>Tries to create a hard link to the source file rather than copying the entire contents of the file.
		/// If unable to create a hard link it simply copies the file.</summary>
		[TaskAttribute("hardlinkifpossible")]
		public bool HardLink { get; set; } = false;

		/// <summary>Tries to create a hard link to the source file rather than copying the entire contents of the file.
		/// If unable to create a hard link it simply copies the file.</summary>
		[TaskAttribute("symlinkifpossible")]
		public bool SymLink { get; set; } = false;

		/// <summary>The (single) file set to be copied.  
		/// Note the todir attribute must be set, and that a fileset 
		/// may include another fileset.</summary>
		[FileSet("fileset")]
		public FileSet CopyFileSet { get; } = new FileSet();

		protected Hashtable FileCopyMap { get; } = new Hashtable();

		public CopyTask() : base("copy") { }

		public CopyTask(Project project, string file = null, string todir = null, string tofile = null, bool overwrite = false, bool clobber = false) 
			: base("copy")
		{
			Project = project;
			SourceFile = file;
			ToDirectory = todir;
			ToFile = tofile;
			Overwrite = overwrite;
			Clobber = clobber;
		}

		/// <summary>
		/// Actually does the file (and possibly empty directory)
		/// transfers.
		/// </summary>
		protected virtual void DoFileOperations()
		{
			int fileCount = FileCopyMap.Keys.Count;
			if (fileCount > 0 || Verbose)
			{
				if (ToDirectory != null)
				{
					Log.Info.WriteLine(LogPrefix + "Copying {0} files to {1}", fileCount, Project.GetFullPath(ToDirectory));
				}
				else
				{
					Log.Info.WriteLine(LogPrefix + "Copying {0} files", fileCount);
				}

				// loop through our file list
				foreach (string sourcePath in FileCopyMap.Keys)
				{
					string dstPath = (string)FileCopyMap[sourcePath];
					if (sourcePath == dstPath)
					{
						Log.Info.WriteLine(LogPrefix + "Skipping self-copy of {0}" + sourcePath);
						continue;
					}

					Log.Info.WriteLine(LogPrefix + "Copying {0} to {1}", sourcePath, dstPath);

					CopyFlags copyFlags = CopyFlags.PreserveTimestamp |
						(MaintainAttributes ? CopyFlags.MaintainAttributes : CopyFlags.None) |
						(Overwrite ? CopyFlags.None : CopyFlags.SkipUnchanged);

					List<LinkOrCopyMethod> methods = new List<LinkOrCopyMethod>();
					if (SymLink)
					{
						methods.Add(LinkOrCopyMethod.Symlink);
					}
					if (HardLink)
					{
						methods.Add(LinkOrCopyMethod.Hardlink);
					}
					methods.Add(LinkOrCopyMethod.Copy);

					LinkOrCopyStatus result = LinkOrCopyCommon.TryLinkOrCopy
					(
						sourcePath,
						dstPath,
						Clobber ? OverwriteMode.OverwriteReadOnly : OverwriteMode.Overwrite,
						copyFlags,
						(uint)RetryCount,
						(uint)RetryDelay,
						methods.ToArray()
					);

					bool succeeded = result != LinkOrCopyStatus.Failed;
					if (!succeeded)
					{
						string msg = String.Format("Cannot copy {0} to {1}", sourcePath, dstPath);
						throw new BuildException(msg, Location);
					}
				}
			}
		}

		protected override void ExecuteTask()
		{
			// NOTE: when working with file and directory names its useful to 
			// use the FileInfo an DirectoryInfo classes to normalize paths like:
			// c:\work\nant\extras\buildserver\..\..\..\bin

			if (SourceFile != null)
			{
				// Copy single file.
				FileInfo srcInfo = new FileInfo(Project.GetFullPath(SourceFile));
				if (srcInfo.Exists)
				{
					FileInfo dstInfo = null;
					if (ToFile != null)
					{
						dstInfo = new FileInfo(Project.GetFullPath(ToFile));
					}
					else
					{
						string dstDirectoryPath = Project.GetFullPath(ToDirectory);
						string dstFilePath = Path.Combine(dstDirectoryPath, srcInfo.Name);
						dstInfo = new FileInfo(dstFilePath);
					}

					// do the outdated check
					bool outdated = (!dstInfo.Exists) || (srcInfo.LastWriteTime > dstInfo.LastWriteTime);

					if (Overwrite || outdated)
					{
						// add to a copy map of absolute verified paths
						FileCopyMap.Add(srcInfo.FullName, dstInfo.FullName);
					}
				}
				else
				{
					string msg = String.Format(LogPrefix + "Could not find file {0} to copy.", SourceFile);
					throw new BuildException(msg, Location);
				}
			}

			if (CopyFileSet.FileItems.Count > 0)
			{
				if (Flatten)
				{
					StringCollection existingFiles = new StringCollection();
					foreach (FileItem fileItem in CopyFileSet.FileItems)
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
								FileInfo dstInfo = new FileInfo(dstPath);
								bool outdated = (!dstInfo.Exists) || (srcInfo.LastWriteTime > dstInfo.LastWriteTime);
								if (Overwrite || outdated)
								{
									FileCopyMap.Add(fileItem.FileName, dstPath);
								}
							}
						}
					}
				}
				else
				{
					// Copy file set contents.

					// get the complete path of the base directory of the file set, ie, c:\work\nant\src
					DirectoryInfo srcFileSetBaseInfo = new DirectoryInfo(CopyFileSet.BaseDirectory.TrimRightSlash());
					DirectoryInfo dstBaseInfo = new DirectoryInfo(Project.GetFullPath(ToDirectory).TrimRightSlash());

					// if source file not specified use file set
					foreach (FileItem fileItem in CopyFileSet.FileItems)
					{
						FileInfo srcInfo = new FileInfo(fileItem.FileName);
						if (srcInfo.Exists)
						{
							var srcBaseInfo = fileItem.BaseDirectory == null ? srcFileSetBaseInfo : new DirectoryInfo(fileItem.BaseDirectory.TrimRightSlash());
							// replace the file set path with the destination path
							// NOTE: big problems could occur if the file set base dir is rooted on a different drive
							if (!fileItem.FileName.StartsWith(srcBaseInfo.FullName))
								Log.Status.WriteLine(LogPrefix + "File '" + fileItem.FileName + "' is not going to be copied because it is not contained in the file set's base directory '" + CopyFileSet.BaseDirectory + "'.  Use the flatten option to copy it.");
							string dstPath = fileItem.FileName.Replace(srcBaseInfo.FullName, dstBaseInfo.FullName);

							// do the outdated check
							FileInfo dstInfo = new FileInfo(dstPath);
							bool outdated = (!dstInfo.Exists) || (srcInfo.LastWriteTime > dstInfo.LastWriteTime);

							if (Overwrite || outdated)
							{
								FileCopyMap.Add(fileItem.FileName, dstPath);
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

			// do all the actual copy operations now...
			DoFileOperations();
		}
	}
}
