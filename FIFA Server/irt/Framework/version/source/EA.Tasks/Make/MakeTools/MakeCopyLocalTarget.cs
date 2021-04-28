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

using EA.Eaconfig.Build;
using EA.FrameworkTasks.Model;
using EA.Make.MakeItems;

namespace EA.Make.MakeTools
{
	public class MakeCopyLocalInfo
	{
		public MakeCopyLocalInfo(string from, bool useHardLinkIfPossible)
		{
			FromFile = from;
			UseHardLinkIfPossible = useHardLinkIfPossible;
		}
		public string FromFile { get; set; }
		public bool UseHardLinkIfPossible { get; set; }
	}
	
	public class MakeCopyLocalTarget
	{
		public MakeTarget CopyLocalTarget;

		protected readonly MakeModuleBase MakeModule;

		private readonly MakeFragment ToolPrograms;

		private readonly MakeFragment ToolOptions;

		private readonly IModule Module;

		private readonly string LogPrefix;

		private IDictionary<string, MakeCopyLocalInfo> _copy_map;    // key = destination file, value = (source file, use hard link if possible)

		public MakeCopyLocalTarget(IModule module, MakeModuleBase makeModule)
		{
			Module = module;
			MakeModule = makeModule;
			ToolPrograms = makeModule.Sections["tool_programs"];
			ToolOptions = makeModule.Sections["tool_options"];

			LogPrefix = "[makegen] ";
		}

		public void GenerateTargets()
		{
			if (_copy_map != null && _copy_map.Count > 0)
			{
				CopyLocalTarget = MakeModule.MakeFile.Target("copy-local");

				MakeVariableArray copylocaloutputs = ToolOptions.VariableMulti("COPY_LOCAL_OUTPUTS", _copy_map.Select(e => MakeModule.ToMakeFilePath(e.Key)));

				CopyLocalTarget.Prerequisites += copylocaloutputs.Label.ToMakeValue();

				bool useHardLink = false;
				if (Module is EA.Eaconfig.Modules.Module_Native)
				{
					useHardLink = (Module as EA.Eaconfig.Modules.Module_Native).CopyLocalUseHardLink;
				}
				else if (Module is EA.Eaconfig.Modules.Module_DotNet)
				{
					useHardLink = (Module as EA.Eaconfig.Modules.Module_DotNet).CopyLocalUseHardLink;
				}

				MakeVariableScalar copyvar = ToolPrograms.Variable("COPY", MakeModule.ToMakeFilePath(Module.Package.Project.GetPropertyValue("nant.copy")));

				foreach (KeyValuePair<string, MakeCopyLocalInfo> copyinfo in _copy_map)
				{
					string toFile = MakeModule.ToMakeFilePath(copyinfo.Key);
					string fromFile = MakeModule.ToMakeFilePath(copyinfo.Value.FromFile);
					string hardLinkSwitch = (useHardLink && copyinfo.Value.UseHardLinkIfPossible) ? "l" : "";

					MakeTarget singleCopyLocalTarget = MakeModule.MakeFile.Target(toFile);
					singleCopyLocalTarget.Prerequisites += fromFile;
					singleCopyLocalTarget.AddRuleCommentLine("[{0}] Copy Local for $<", Module.Name);
					singleCopyLocalTarget.AddRuleLine("{0} -a5oty{1} $< $@", copyvar.Label.ToMakeValue(), hardLinkSwitch);
				}

				MakeModule.AddFilesToClean(_copy_map.Keys);
			}
		}


		public void AddCopyLocalDependents(bool useHardLinkIfPossible)
		{
			// TODO make file copy local clean up
			// -dvaliant 2016/05/31
			/* currently we just ignore post build copy local files for make files so that when building shared libraries we don't
			create a make file where a file depends upon itself - what we should actually do is add a flag to the copy map that
			says "don't add this to dependency list" */
            foreach (CopyLocalInfo copyLocalInfo in Module.CopyLocalFiles.Where(copyLocalFile => !copyLocalFile.PostBuildCopyLocal))
			{
				bool useHardLink = useHardLinkIfPossible ? PathUtil.TestAllowCopyWithHardlinkUsage(Module.Package.Project, copyLocalInfo.AbsoluteSourcePath, copyLocalInfo.AbsoluteDestPath) : false;
				UpdateCopyMap(copyLocalInfo.AbsoluteSourcePath, copyLocalInfo.AbsoluteDestPath, useHardLink);
			}
		}

		// TODO make file copy local clean up
		// -dvaliant 2016/05/31
		/* this function doesn't need to exist however it does copy .map files - something no other copy local mechanism does
		we should figure out if there is any reason to do this */
		public void AddCopyLocal(FileSet files, bool copylocal, bool useHardLink)
		{
			if (files != null)
			{
				foreach (var item in files.FileItems)
				{
					if (item.AsIs == false)
					{
						// Check if assembly has optionset with copylocal flag attached.
						var itemCopyLocal = item.GetCopyLocal(Module);

						if ((copylocal || itemCopyLocal == CopyLocalType.True) && itemCopyLocal != CopyLocalType.False)
						{
							var toFile = Path.Combine(Module.OutputDir.Path, Path.GetFileName(item.Path.Path));

							UpdateCopyMap(item.Path.Path, toFile, useHardLink);

							// Copy pdb:
							var pdbFile = Path.ChangeExtension(item.Path.Path, ".pdb");
							//if (File.Exists(pdbFile))
							{

								toFile = Path.Combine(Module.OutputDir.Path, Path.GetFileName(pdbFile));

								UpdateCopyMap(pdbFile, toFile, useHardLink);
							}
							// Copy map:
							var mapbFile = Path.ChangeExtension(item.Path.Path, ".map");
							//if (File.Exists(mapbFile))
							{
								toFile = Path.Combine(Module.OutputDir.Path, Path.GetFileName(pdbFile));

								UpdateCopyMap(mapbFile, toFile, useHardLink);
							}
						}
					}
				}
			}

		}

		private void UpdateCopyMap(string from, string to, bool useHardLink)
		{
			if (_copy_map == null)
			{
				_copy_map = new Dictionary<string, MakeCopyLocalInfo>();
			}
			if(!_copy_map.ContainsKey(to))
			{
				if (!String.Equals(from, to, PathUtil.PathStringComparison))
				{
					_copy_map.Add(to, new MakeCopyLocalInfo(from,useHardLink));
				}
			}
		}

		internal class CopyFileRspData
		{
			public bool			CopyWithHardLink {  get; set; }
			public PathString	RspFile { get; set; }
			public CopyFileRspData(bool copyWithHardLink, PathString rspFile)
			{
				CopyWithHardLink = copyWithHardLink;
				RspFile = rspFile;
			}
		}

		protected void OnCopyLocalResponseFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = string.Format("{0}{1} copy local response file for '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Module.Name);
				if (e.IsUpdatingFile)
					Module.Package.Project.Log.Minimal.WriteLine(message);
				else
					Module.Package.Project.Log.Status.WriteLine(message);
			}
		}

	}
}
