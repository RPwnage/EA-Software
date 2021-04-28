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

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks {

	/// <summary>Creates a directory path.</summary>
	/// <remarks>
	/// <para>
	/// A directory specified with an absolute pathname will be
	/// created, and any parent directories in the directory path which
	/// do not already exist will also be created.
	/// </para> <para>
	/// A directory specified with a relative pathname will be created 
	/// relative to the
	/// location of the build file.  Any directories in the relative
	/// directory path which do not already exist will be created.
	/// </para>
	/// </remarks>
	/// <include file='Examples/MkDir/Directory.example' path='example'/>
	/// <include file='Examples/MkDir/Tree.example' path='example'/>
	[TaskName("mkdir", NestedElementsCheck = true)]
	public class MkDirTask : Task {

		/// <summary>The directory to create.</summary>
		[TaskAttribute("dir", Required = true)]
		public string Dir { get; set; } = null;

		public static void CreateDir(Project project, string dir)
		{
			MkDirTask mkdir = new MkDirTask();
			mkdir.Project = project;
			mkdir.Dir = dir;
			mkdir.Execute();
		}

		protected override void ExecuteTask() {
			try {
				string directory = Project.GetFullPath(Dir);
				if (!Directory.Exists(directory)) {
					Log.Info.WriteLine(LogPrefix + "Creating directory {0}", directory);
					DirectoryInfo result = Directory.CreateDirectory(directory);
					if (result == null) {
						string msg = String.Format("Unknown error creating directory '{0}'", directory);
						throw new BuildException(msg, Location);
					}
				}
			} catch (Exception e) {
				throw new BuildException(e.Message, Location, e);
			}
		}
	}
}
