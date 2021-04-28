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

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;


namespace EA.Eaconfig.Build
{
	[TaskName("generated-make-file-path")]
	public class GeneratedMakeFilePath : Task
	{
		[TaskAttribute("split-by-group-names", Required = false)]
		public bool SplitByGroupNames { get; set; } = true;

		[TaskAttribute("makefile-dir-property", Required = true)]
		public string DirProperty { get; set; }

		[TaskAttribute("makefile-name-property", Required = true)]
		public string NameProperty { get; set; }

		protected override void ExecuteTask()
		{
			Project.Properties[DirProperty] = GetMakeFileDir(Project).Path;
			Project.Properties[NameProperty] = GetMakeFileName(Project, SplitByGroupNames);
		}

		public static PathString GetMakeFileDir(Project project)
		{
			return PathString.MakeNormalized(project.GetPropertyValue("package.configbuilddir"));
		}

		public static string GetMakeFileName(Project project, bool splitByGroupNames)
		{
			var group = project.GetPropertyValue("eaconfig.build.group");
			if (splitByGroupNames && group != "runtime")
			{
				return String.Format("{0}-{1}-Makefile", project.GetPropertyValue("eaconfig.build.group"), project.GetPropertyValue("package.name"));
			}
			return String.Format("{0}-Makefile", project.GetPropertyValue("package.name"));
		}
	}
}

