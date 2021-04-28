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

using System;
using System.Collections.Generic;
using NAnt.Core.Util;

namespace EA.FrameworkTasks.Model
{
	public class Tool : CommandWithDependencies
	{
		public const uint TypeBuild = 1;
		public const uint TypePreCompile = 2;
		public const uint TypePreLink = 4;
		public const uint TypePostLink = 8;
		public const uint AppPackage = 16;
		public const uint TypeDeploy = 32;

		protected Tool(string toolname, PathString toolexe, uint type, string description = "",  PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
			: base(toolexe, new List<string>(), description, workingDir, env, createdirectories)
		{
			SetType(type);
			ToolName = toolname;            
			Templates = new Dictionary<string, string>();
		}
		
		protected Tool(string toolname, string script, uint type, string description = "", PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
			: base(script, description, workingDir, env, createdirectories)
		{
			SetType(type);
			ToolName = toolname;
			Templates = new Dictionary<string, string>();
		}
		
		public new List<string> Options // DAVE-FUTURE-REFACTOR-TODO: this is nasty - but it seems necessary to have this be writable for nant setup and postprocess steps. can we just make it writeable in base though?
		{ 
			 get {return base.Options as List<String>;}
		}

		public override string Message
		{
			get { return String.Format("      [{0}] {1}", ToolName, Description??String.Empty); }
		}

		public readonly string ToolName;
		public readonly Dictionary<string, string> Templates; // This property is for command line templates is used by build tasks.

	}
}
