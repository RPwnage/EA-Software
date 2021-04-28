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
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.DotNetTasks;
using EA.FrameworkTasks.Model;
using EA.Tasks.Model;

namespace EA.Eaconfig.Modules.Tools
{
	public abstract class DotNetCompiler : Tool
	{
		public enum Targets { Exe, WinExe, Library }

		protected DotNetCompiler(string toolname, PathString toolexe, Targets target)
			: base(toolname, toolexe, Tool.TypeBuild)
		{
			SourceFiles = new FileSet();
			Defines = new SortedSet<string>();
			Resources = new ResourceFileSet();
			NonEmbeddedResources = new FileSet();
			ContentFiles = new FileSet();
			Modules = new FileSet();
			Assemblies = new FileSet();
			DependentAssemblies = new FileSet();
			ComAssemblies = new FileSet();
			Target = target;

			Assemblies.FailOnMissingFile = false;
			DependentAssemblies.FailOnMissingFile = false;
		}

		public string GetOptionValues(string name, string sep)
		{
			var val = new StringBuilder();
			if (!String.IsNullOrEmpty(name))
			{
				var cleanname = name.TrimStart(OPTION_PREFIX_CHARS).TrimEnd(':') + ':';
				foreach(var option in Options.Where(o => OPTION_PREFIXES.Any(prefix => o.StartsWith(prefix + cleanname))))
				{
						var ind = option.IndexOf(':');
						if (ind < option.Length - 1)
						{
							var oval = option.Substring(ind + 1).TrimWhiteSpace();
							if(!String.IsNullOrEmpty(oval))
							{
								if(val.Length > 0)
								{
									val.Append(sep);
								}
								val.Append(oval);
							}
						}
				}
			}
			return val.ToString();
		}

		public string GetOptionValue(string name)
		{
			string val = String.Empty;
			if (!String.IsNullOrEmpty(name))
			{
				string cleanname = name.TrimStart(OPTION_PREFIX_CHARS).TrimEnd(':') + ':';
				string option = Options.FirstOrDefault(o => OPTION_PREFIXES.Any(prefix=> o.StartsWith(prefix+cleanname)));
				if (!String.IsNullOrEmpty(option))
				{
					int ind = option.IndexOf(':');
					if (ind < option.Length-1)
					{
						val = option.Substring(ind+1);
					}
				}
			}
			return val;
		}

		// note: this handle /option:- but not /option-
		public bool GetBoolenOptionValue(string name)
		{
			return HasOption(name) && (GetOptionValue(name) != "-");
		}

		public bool HasOption(string name, bool exact = false)
		{
			string cleanname = name.TrimStart(OPTION_PREFIX_CHARS);
			return Options.Any(o => OPTION_PREFIXES.Any(prefix => exact ? o.Equals(prefix + cleanname) : o.StartsWith(prefix + cleanname)));
		}

		private static readonly char[] OPTION_PREFIX_CHARS = new char[] { '-', '/'};
		private static readonly string[] OPTION_PREFIXES = new string[] { "--", "/", "-" };

		public readonly Targets Target;
		public string OutputName;
		public string OutputExtension;
		public readonly SortedSet<string> Defines;
		public PathString DocFile;
		public bool GenerateDocs;
		public PathString ApplicationIcon;
		public string AdditionalArguments; //IMTODO: ideally I want to get rid of this and parse .csc-args into Options. But I don't have time now.
		public string TargetPlatform;

		public DotNetFrameworkFamilies TargetFrameworkFamily = DotNetFrameworkFamilies.Framework;

		/// <summary>
		/// Path to system reference assemblies.  (Corresponds to the -lib argument in csc.exe)
		/// Although the actual -lib argument list multiple directories using comma separated,
		/// you can separate them using ',' or ';' or '\n' in this task.  Note that space is a valid path
		/// character and therefore won't be used as separator characters.
		/// </summary>
		public string ReferenceAssemblyDirs;

		public readonly FileSet SourceFiles;
		public readonly FileSet Assemblies;
		public readonly FileSet DependentAssemblies;
		public readonly ResourceFileSet Resources;
		public readonly FileSet NonEmbeddedResources;
		public readonly FileSet ContentFiles;
		public readonly FileSet Modules;
		public readonly FileSet ComAssemblies;
	}
}
