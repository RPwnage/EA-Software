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

using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;

namespace EA.Eaconfig
{
	/// <summary>Exclude a directory from an existing fileset</summary>
	[TaskName("ExcludeDir")]
	public class ExcludeDirTask : Task
	{
		/// <summary>The name of the fileset that we want to exclude from</summary>
		[TaskAttribute("Fileset", Required = true)]
		public string FilesetName { get; set; }

		/// <summary>The name of the directory that we want to exclude</summary>
		[TaskAttribute("Directory", Required = true)]
		public string DirectoryName { get; set; }

		public ExcludeDirTask() : base() { }

		public ExcludeDirTask(Project project, string fileset, string directory)
			: base()
		{
			Project = project;
			FilesetName = fileset;
			DirectoryName = directory;
		}

/*
<do if="@{DirectoryExists('${ExcludeDir.Directory}')}">
	<!-- handle full path buildroot -->
	<do if="@{PathIsPathRooted('${ExcludeDir.Directory}}')}">
		<!-- only add the exclusion if the directory is within the package directory -->
		<do if="@{StrStartsWith('${ExcludeDir.Directory}', '${package.dir}')}">
			<fileset name="${ExcludeDir.Fileset}" append="true">
				<excludes name="${ExcludeDir.Directory}/**" />
			</fileset>
		</do>
	</do>
	<!-- handle relative path buildroot -->
	<do unless="@{PathIsPathRooted('${ExcludeDir.Directory}}')}">
		<fileset name="${ExcludeDir.Fileset}" append="true" basedir="${package.dir}">
			<excludes name="${ExcludeDir.Directory}/**" />
		</fileset>
	</do>
</do>
*/

		protected override void ExecuteTask()
		{
			if (Log.Level == Log.LogLevel.Diagnostic)
			{
				Log.Status.WriteLine("ExcludeDir('{0}', '{1}')", FilesetName, DirectoryName);
			}

			//Append exclusions to fileset for the specified directory
			if (Directory.Exists(DirectoryName))
			{
				if (Path.IsPathRooted(DirectoryName))
				{
					if (DirectoryName.StartsWith(Project.Properties["package.dir"]))
					{
						FileSet fileset = Project.NamedFileSets[FilesetName];
						fileset.Exclude(DirectoryName + "/**");
					}
				}
				else
				{
					FileSet fileset = Project.NamedFileSets[FilesetName];
					fileset.BaseDirectory = Project.Properties["package.dir"];
					fileset.Exclude(DirectoryName + "/**");
				}
			}
		}
	}
}
