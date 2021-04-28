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

using NAnt.Core;
using NAnt.Core.Util;


namespace EA.FrameworkTasks.Model
{
	public class Command : BitMask
	{
		public const uint ShellScript = 16384;
		public const uint Program     = 32768;
		public const uint ExcludedFromBuild = 65536;

		public readonly string Script;
		public readonly PathString Executable;
		public readonly IEnumerable<string> Options;
		public readonly PathString WorkingDir;
		public readonly IDictionary<string, string> Env;
		public readonly IList<PathString> CreateDirectories;
		public readonly string Description;

		public Command(string script, string description = "", PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
			: base(ShellScript)
		{
			Script = script.NormalizeNewLineChars();
			Executable = GetScriptExecutable();
			Options = GetScriptOptions();
			WorkingDir = workingDir;
			Env = env;
			CreateDirectories = createdirectories == null ? new List<PathString>() : new List<PathString>(createdirectories);
			Description = description?.NormalizeNewLineChars() ?? throw new ArgumentNullException(nameof(description));
		}

		public Command(PathString executable, IEnumerable<string> options, string description = "", PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
			: base(Program)
		{
			Executable = executable;
			Options = options;
			WorkingDir = workingDir;
			Env = env;
			CreateDirectories = createdirectories == null ? new List<PathString>() : new List<PathString>(createdirectories); ;
			Description = description ?? throw new ArgumentNullException(nameof(description));
		}

		public string CommandLine
		{
			get
			{
				if (IsKindOf(Program))
				{
					return Executable.Path.Quote() + " " + Options.ToString(" ");
				}
				else
				{
					return Script;
				}
			}
		}

		private List<string> GetScriptOptions()
		{
			var options = new List<string>();
			switch(PlatformUtil.Platform)
			{
				case PlatformUtil.Windows:
					options.Add("/c");
					options.Add(Script);
					break;
				case PlatformUtil.Unix:
				case PlatformUtil.OSX:
					options.Add(Script);
					break;
				default:
					break;
			}
			return options;
		}

		private PathString GetScriptExecutable()
		{
			string exec = "undefined";
			switch (PlatformUtil.Platform)
			{
				case PlatformUtil.Windows:
					exec = "cmd";
					break;
				case PlatformUtil.Unix:
				case PlatformUtil.OSX:
					exec = "bash";
					break;
				default:
					break;
			}
			return new PathString(exec);
		}

		public virtual string Message
		{
			get { return Description ?? Executable.Path; }
		}

	}
}
