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

using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks
{
	// this task is a very specific hack for use with the remotebuild target
	// and the RemoteBuild package. The package parses the command line
	// and then triggers a build on a remote machine. Any target *after*
	// remotebuild should only be executed by the remote machine so this
	// task short circuits the normal execution of listed build targets.
	// If remote build support is removed this tasks and CancelBuildTargetExecution
	// mechanism can be removed

	/// <summary>Stops execution of next top level target (when targets are chained</summary>
	[TaskName("CancelBuildTargetsExecution", Mixed = true)]
	public class CancelBuildTargetsExecutionTask : Task
	{
		public CancelBuildTargetsExecutionTask() : base("CancelBuildTargetsExecutionTask") { }

		protected override void ExecuteTask() 
		{
			Project.CancelBuildTargetExecution = true;
		}
	}
}