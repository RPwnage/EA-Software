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

using System;
using System.IO;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.Tasks
{

	/// <summary>Deletes a file, file set or directory.</summary>
	/// <remarks>
	///   <para>Deletes either a single file, all files in a specified directory and its sub-directories, or a set of files specified by one or more file sets.</para>
	///   <note>If the file attribute is set then the file set contents will be ignored.  To delete the files in the file set, omit the file attribute in the delete element.</note>
	///   <note>All items specified including read-only files and directories are 
	///   deleted.  If an item cannot be deleted because of some other reason 
	///   the task will fail if "failonmissing" is true.</note>
	/// </remarks>
	/// <include file='Examples/Delete/File.example' path='example'/>
	/// <include file='Examples/Delete/Directory.example' path='example'/>
	/// <include file='Examples/Delete/FileSet.example' path='example'/>
	[TaskName("delete", NestedElementsCheck = true)]
	public class DeleteTask : Task
	{

		/// <summary>The file to delete. Applies only to the single file delete operation.</summary>
		[TaskAttribute("file")]
		public string FileName { get; set; } = null;

		/// <summary>The directory to delete.</summary>
		[TaskAttribute("dir")]
		public string DirectoryName { get; set; } = null;

		/// <summary>If true the task will fail if a file or directory specified is not present.  Default is false.</summary>
		[TaskAttribute("failonmissing")]
		public bool FailOnMissing { get; set; } = false;

		/// <summary>If true then be quiet - don't report on the deletes. Default is false</summary>
		[TaskAttribute("quiet")]
		public bool Quiet { get; set; } = false;

		/// <summary>All the files in the file set will be deleted.</summary>
		[FileSet("fileset")]
		public FileSet DeleteFileSet { get; } = new FileSet();

		protected override void ExecuteTask()
		{
			// limit task to deleting either a file or a directory or a file set
			if (FileName != null && DirectoryName != null)
			{
				throw new BuildException("Cannot specify 'file' and 'dir' in the same delete task.", Location);
			}

			if (FileName != null)
			{
				// try to delete specified file
				string path = null;
				try
				{
					path = Project.GetFullPath(FileName);
				}
				catch (Exception e)
				{
					string msg = String.Format("Could not determine path from '{0}'.", FileName);
					throw new BuildException(msg, Location, e);
				}

				DeleteFile(path, Verbose);

			}
			else if (DirectoryName != null)
			{
				// try to delete specified directory
				string path = null;
				try
				{
					path = Project.GetFullPath(DirectoryName);
				}
				catch (Exception e)
				{
					string msg = String.Format("Could not determine path from '{0}'.", DirectoryName);
					throw new BuildException(msg, Location, e);
				}
				DeleteDirectory(path);

			}
			else
			{
				// delete files in file set
				// propagate failonmissing to fileset
				DeleteFileSet.FailOnMissingFile = this.FailOnMissing;
				if (!Quiet)
					Log.Info.WriteLine(LogPrefix + "Deleting {0} files", DeleteFileSet.FileItems.Count);
				foreach (FileItem fileItem in DeleteFileSet.FileItems)
				{
					DeleteFile(fileItem.FileName, Verbose);
				}
			}
		}

		void DeleteDirectory(string path)
		{
			if (Directory.Exists(path))
			{
				if (!Quiet)
					Log.Info.WriteLine(LogPrefix + "Deleting directory {0}", path);
				try
				{
					PathUtil.DeleteDirectory(path);
				}
				catch (Exception e)
				{
					string msg = String.Format("Cannot delete directory '{0}'.", path);
					throw new BuildException(msg, Location, e);
				}
			}
			else
			{
				if (FailOnMissing)
				{
					string msg = String.Format("Cannot delete '{0}' because directory is missing.", path);
					throw new BuildException(msg, Location);
				}
			}
		}

		void DeleteFile(string path, bool verbose)
		{
			if (File.Exists(path))
			{
				if (!Quiet && verbose)
					Log.Status.WriteLine(LogPrefix + "Deleting file {0}", path);
			}

			PathUtil.DeleteFile(path, FailOnMissing, Location);
		}
	}
}
