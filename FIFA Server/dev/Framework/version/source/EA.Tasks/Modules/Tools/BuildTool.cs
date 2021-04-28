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

using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using EA.FrameworkTasks.Model;


namespace EA.Eaconfig.Modules.Tools
{
	public class BuildTool : Tool
	{
		public static readonly string SCRIPT_ERROR_CHECK;

		static BuildTool()
		{
			if (PlatformUtil.IsWindows)
			{
				SCRIPT_ERROR_CHECK = @"IF %ERRORLEVEL% NEQ 0 exit %ERRORLEVEL%";
			}
		}

		public BuildTool(Project project, string toolname, PathString toolexe, uint type, string description = "", PathString outputdir = null, PathString intermediatedir = null, IEnumerable<PathString> createdirectories = null, bool treatoutputascontent = false)
			: base(toolname, toolexe, type, description, workingDir:intermediatedir,
			env: Environment(project),
			createdirectories:createdirectories)
		{
			Files = new FileSet();
			OutputDir = outputdir;
			IntermediateDir = intermediatedir;
			TreatOutputAsContent = treatoutputascontent;
		}

		public BuildTool(Project project, string toolname, string script, uint type, string description = "", PathString outputdir = null, PathString intermediatedir = null, IEnumerable<PathString> createdirectories = null, bool treatoutputascontent = false)
			: base(toolname, script, type, description, workingDir: intermediatedir,
			env: Environment(project),
			createdirectories: createdirectories)
		{
			Files = new FileSet();
			OutputDir = outputdir;
			IntermediateDir = intermediatedir;
			TreatOutputAsContent = treatoutputascontent;
		}
		
		// Generation properties - Used in VS solution generation
		public readonly FileSet Files;  
		public readonly PathString OutputDir;
		public readonly PathString IntermediateDir;
		public readonly bool TreatOutputAsContent;


		private static IDictionary<string, string> Environment(Project project)
		{
			var optionset = project.NamedOptionSets["build.env"];
			return optionset == null ? null : optionset.Options;
		}

		internal override bool NeedsRunning(Project project)
		{
			FileSet indep = new FileSet();
			indep.IncludeWithBaseDir(InputDependencies);
			indep.Include(Files);
			return DependsTask.IsTaskNeedsRunning(project, indep, OutputDependencies);
		}
	}
}