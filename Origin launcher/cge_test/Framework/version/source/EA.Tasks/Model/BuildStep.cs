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
using NAnt.Core.Util;

namespace EA.FrameworkTasks.Model
{
	public class BuildStep : BitMask
	{
		public const uint PreBuild = 1;
		public const uint PreLink = 2;
		public const uint PostBuild = 4;
		public const uint Build = 8;
		public const uint Clean = 16;
		public const uint ReBuild = 32;
		public const uint ExecuteAlways = 64;

		public BuildStep(string stepname, uint type, List<TargetCommand> targets = null, List<Command> commands = null)
			: base(type)
		{
			Name = stepname;
			TargetCommands = targets ?? new List<TargetCommand>();
			Commands = commands ?? new List<Command>();
		}

		public readonly string Name;
		public readonly IList<TargetCommand> TargetCommands;
		public readonly IList<Command>  Commands;
		public List<PathString> InputDependencies;
		public List<PathString> OutputDependencies;
		
		/// <summary>
		/// Name of the target a custom build step should run before (build, link, run).
		/// Supported by native NAnt and MSBuild.
		/// </summary>
		public string Before;

		/// <summary>
		/// Name of the target a custom build step should run after (build, run).
		/// Supported by native NAnt and MSBuild.
		/// </summary>
		public string After;
	}
}
