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
using System.IO;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Events;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Writers;

using EA.FrameworkTasks.Model;
using EA.Make.MakeItems;

namespace EA.Make.MakeTools
{
	public class MakeFileCopyStepTarget
	{
		public MakeTarget FileCopyStepsTarget;

		protected readonly MakeModuleBase MakeModule;

		private readonly MakeFragment ToolPrograms;

		private readonly MakeFragment ToolOptions;

		private readonly IModule Module;

		private readonly string LogPrefix;

		private IDictionary<string, string> _copy_map;

		public MakeFileCopyStepTarget(IModule module, MakeModuleBase makeModule)
		{
			Module = module;
			MakeModule = makeModule;
			ToolPrograms = makeModule.Sections["tool_programs"];
			ToolOptions = makeModule.Sections["tool_options"];

			LogPrefix = "[makegen] ";
		}

		public void GenerateTargets()
		{
			if (_copy_map == null)
			{
				_copy_map = new Dictionary<string, string>();
			}
			if(_copy_map != null)
			{
				int copystepIndex = 0;
				string propName = string.Format("filecopystep{0}", copystepIndex);
				string tofile = Module.PropGroupValue(string.Format("{0}.tofile", propName), null);
				string todir = Module.PropGroupValue(string.Format("{0}.todir", propName), null);

				while (tofile != null || todir != null)
				{
					if (tofile != null)
					{
						string srcFile = Module.PropGroupValue(string.Format("{0}.file", propName), null);
						if (srcFile == null)
						{
							string modulePropName = Module.PropGroupName(string.Format("{0}", propName));
							Module.Package.Project.Log.Error.WriteLine("{0}.tofile property was defined but missing {0}.file property.", modulePropName);
							break;
						}

						if (!_copy_map.ContainsKey(tofile))
						{
							if (!String.Equals(srcFile, tofile, PathUtil.PathStringComparison))
							{
								_copy_map.Add(tofile, srcFile);
							}
						}
					}
					else if (todir != null)
					{
						FileSet srcFileset = Module.PropGroupFileSet(string.Format("{0}.fileset", propName));
						string srcFile = Module.PropGroupValue(string.Format("{0}.file", propName), null);
						if (srcFileset != null)
						{
							foreach (var item in srcFileset.FileItems)
							{
								var srcf = item.Path.NormalizedPath;
								string destFile = Path.GetFileName(srcf);
								string destFullPath = Path.Combine(todir, destFile);

								if (!_copy_map.ContainsKey(destFullPath))
								{
									if (!String.Equals(srcf, destFullPath, PathUtil.PathStringComparison))
									{
										_copy_map.Add(destFullPath, srcf);
									}
								}
							}
						}
						else if (srcFile != null)
						{
							string destFile = Path.GetFileName(srcFile);
							string destFullPath = Path.Combine(todir, destFile);

							if (!_copy_map.ContainsKey(destFullPath))
							{
								if (!String.Equals(srcFile, destFullPath, PathUtil.PathStringComparison))
								{
									_copy_map.Add(destFullPath, srcFile);
								}
							}
						}
						else
						{
							string modulePropName = Module.PropGroupName(string.Format("{0}", propName));
							Module.Package.Project.Log.Error.WriteLine("{0}.todir property was defined but missing {0}.fileset or {0}.file.", modulePropName);
							break;
						}
					}
					else
					{
						break;
					}
					copystepIndex++;
					propName = string.Format("filecopystep{0}", copystepIndex);
					tofile = Module.PropGroupValue(string.Format("{0}.tofile", propName), null);
					todir = Module.PropGroupValue(string.Format("{0}.todir", propName), null);
				}
			}

			if (_copy_map != null && _copy_map.Count > 0)
			{
				FileCopyStepsTarget = MakeModule.MakeFile.Target("file-copy-steps");

				var copylocaloutputs = ToolOptions.VariableMulti("FILE_COPY_STEPS_OUTPUTS", _copy_map.Select(e=> MakeModule.ToMakeFilePath(e.Key)));

				var copylocalinputs = ToolOptions.VariableMulti("FILE_COPY_STEPS_INPUTS", _copy_map.Select(e => MakeModule.ToMakeFilePath(e.Value)));


				FileCopyStepsTarget = MakeModule.MakeFile.Target(copylocaloutputs.Label.ToMakeValue());

				FileCopyStepsTarget.Prerequisites += copylocalinputs.Label.ToMakeValue();


				var rspfile= WriteFileCopyStepsResponseFile();

				var copyvar = ToolPrograms.Variable("FILECOPY", MakeModule.ToMakeFilePath(Module.Package.Project.GetPropertyValue("nant.copy")));

				FileCopyStepsTarget.AddRuleCommentLine("[{0}] File Copy Steps", Module.Name);
				FileCopyStepsTarget.AddRuleLine("{0} -oty @{1}", copyvar.Label.ToMakeValue(), rspfile);

				MakeModule.AddFilesToClean(_copy_map.Keys);
			}
		}


		private PathString WriteFileCopyStepsResponseFile()
		{
			var rspfile = PathString.MakeCombinedAndNormalized(Module.IntermediateDir.Path, "file_copy_steps.rsp");
			using(var writer = new MakeWriter())
			{
				writer.FileName = rspfile.Path;
				writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnFileCopyStepsResponseFileUpdate);
				foreach(var entry in _copy_map.OrderBy(e=>e.Value))
				{
					writer.WriteLine(MakeModule.ToMakeFilePath(entry.Value));
					writer.WriteLine(MakeModule.ToMakeFilePath(entry.Key));
					MakeModule.AddFileToClean(entry.Key);
				}
			}
			return rspfile;
		}

		protected void OnFileCopyStepsResponseFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = string.Format("{0}{1} file copy steps response file for '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Module.Name);
				if (e.IsUpdatingFile)
					Module.Package.Project.Log.Minimal.WriteLine(message);
				else
					Module.Package.Project.Log.Status.WriteLine(message);
			}
		}

	}
}
