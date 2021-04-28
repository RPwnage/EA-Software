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
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Chris Jenkin (oneinchhard@hotmail.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.IO;

using NAnt.Core.Attributes;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks
{

	/// <summary>Changes the file attributes of a file or set of files.</summary>
	/// <remarks>
	///   <para>The <c>attrib</c> task does not conserve prior file attributes.  Any specified file 
	///   attributes are set; all other attributes are switched off.</para>
	/// </remarks>
	/// <include file='Examples/Attrib/ReadOnly.example' path='example'/>
	/// <include file='Examples/Attrib/Normal.example' path='example'/>
	/// <include file='Examples/Attrib/FileSet.example' path='example'/>
	[TaskName("attrib", NestedElementsCheck = true)]
	public class AttribTask : Task {

		/// <summary>The name of the file which will have its attributes set.  This is provided as an alternate to using the task's fileset.</summary>
		[TaskAttribute("file")]
		public string FileName { get; set; } = null;

		/// <summary>All the files in this fileset will have their file attributes set.</summary>
		[FileSet("fileset")]
		public FileSet AttribFileSet { get; } = new FileSet();

		/// <summary>Set the archive attribute.  Default is "false".</summary>
		[TaskAttribute("archive")]
		public bool ArchiveAttrib { get; set; } = false;

		/// <summary>Set the hidden attribute.  Default is "false".</summary>
		[TaskAttribute("hidden")]
		public bool HiddenAttrib { get; set; } = false;

		/// <summary>Set the normal file attributes.  This attribute is valid only if used alone.  Default is "false".</summary>
		[TaskAttribute("normal")]
		public bool NormalAttrib { get; set; } = false;

		/// <summary>Set the read only attribute.  Default is "false".</summary>
		[TaskAttribute("readonly")]
		public bool ReadOnlyAttrib { get; set; } = false;

		/// <summary>Set the system attribute.  Default is "false".</summary>
		[TaskAttribute("system")]
		public bool SystemAttrib { get; set; } = false;

		protected override void ExecuteTask() {
			// add the shortcut filename to the file set
			if (FileName != null) {
				try {
					string path = Project.GetFullPath(FileName);
					AttribFileSet.Includes.Add(PatternFactory.Instance.CreatePattern(path));
				} catch (Exception e) {
					string msg = String.Format("Could not find file '{0}'", FileName);
					throw new BuildException(msg, Location, e);
				}
			}

			// gather the information needed to perform the operation
			FileAttributes fileAttributes = GetFileAttributes();

			// perform operation
			foreach (FileItem fileItem in AttribFileSet.FileItems) {
				SetFileAttributes(fileItem.FileName, fileAttributes);
			}
		}

		void SetFileAttributes(string path, FileAttributes fileAttributes) {
			try {
				if (File.Exists(path)) {
					FileAttributes oldAttrib = File.GetAttributes(path);
					if (oldAttrib != fileAttributes) {
						Log.Info.WriteLine(LogPrefix + "Setting file attributes for {0} to {1}", path, fileAttributes.ToString());
						File.SetAttributes(path, fileAttributes);
					}
				} else {
					throw new FileNotFoundException();
				}
			} catch (Exception e) {
				string msg = String.Format("Cannot set file attributes for '{0}'", path);
				throw new BuildException(msg, Location, e);
			}
		}

		FileAttributes GetFileAttributes() {
			FileAttributes fileAttributes = 0;
			if (NormalAttrib) {
				fileAttributes = FileAttributes.Normal;
			} else {
				if (ArchiveAttrib) {
					fileAttributes |= FileAttributes.Archive;
				}
				if (HiddenAttrib) {
					fileAttributes |= FileAttributes.Hidden;
				}
				if (ReadOnlyAttrib) {
					fileAttributes |= FileAttributes.ReadOnly;
				}
				if (SystemAttrib) {
					fileAttributes |= FileAttributes.System;
				}
			}
			if (fileAttributes == 0) {
				fileAttributes = FileAttributes.Normal;
			}
			return fileAttributes;
		}
	}
}
