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

using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;


namespace EA.FrameworkTasks.Model
{
	public class CommandWithDependencies : Command
	{
		public CommandWithDependencies(string script, string description = "", PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
			: base(script, description, workingDir, env, createdirectories)
		{
			InputDependencies = new FileSet();
			OutputDependencies = new FileSet();
		}

		public CommandWithDependencies(PathString executable, IEnumerable<string> options, string description = "", PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
			: base(executable, options, description, workingDir, env, createdirectories)
		{
			InputDependencies = new FileSet();
			OutputDependencies = new FileSet();
// Why is the executable path an input dependency?
// For example, if we add this, then what do we do with the WriteX360DeploymentTool function?
// Because as that function is currently written, it expects InputDependencies to be the DeploymentFiles
//            InputDependencies.Include(Executable.Path);
		}

		public readonly FileSet InputDependencies;
		public readonly FileSet OutputDependencies;

		internal virtual bool NeedsRunning(Project project)
		{
			return DependsTask.IsTaskNeedsRunning(project, InputDependencies, OutputDependencies);
		}

	}
}
