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
using System.Linq;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio.sln_taskdef
{

	internal class TaskDefProject : VSDotNetProject
	{
		internal TaskDefProject(TaskDefSolution solution, IEnumerable<IModule> modules)
			: base(solution, modules, CSPROJ_GUID)
		{
			ScriptFile = StartupModule.ExcludedFromBuild_Files.FileItems.First().Path;
		}

		internal readonly PathString ScriptFile;


		public override string ProjectFileName
		{
			get
			{
				return Name + ".csproj";
			}
		}

		public override PathString OutputDir
		{
			get { return BuildGenerator.OutputDir; }
		}

		protected override PathString GetDefaultOutputDir()
		{
			return BuildGenerator.OutputDir;
		}

		protected override string TargetFrameworkVersion
		{
			get
			{
				return "v4.5"; // no easy way in code to detect 4 vs 4.5 (some idea here though: http://stackoverflow.com/questions/8517159/how-to-detect-at-runtime-that-net-version-4-5-currently-running-your-code)
			}
		}

		protected override string ToolsVersion
		{
			get
			{
				return "12.0";
			}
		}

		protected override string DefaultTargetFrameworkVersion
		{
			get { throw new BuildException("Internal error. This method should not be called"); }
		}

		protected override string ProductVersion
		{
			get { return "12.0.20827"; }
		}

		protected override string GetLinkPath(FileEntry fe)
		{
			string link_path = String.Empty;

			if (!String.IsNullOrEmpty(PackageDir))
			{
				link_path = PathUtil.RelativePath(fe.Path.Path, PackageDir).TrimStart(new char[] { '/', '\\', '.' });

				if (!String.IsNullOrEmpty(link_path))
				{
					if (Path.IsPathRooted(link_path) || (link_path.Length > 1 && link_path[1] == ':'))
					{
						link_path = link_path[0] + "_Drive" + link_path.Substring(2);
					}
				}

				return link_path;

			}
			return base.GetLinkPath(fe);
		}
	}
}
