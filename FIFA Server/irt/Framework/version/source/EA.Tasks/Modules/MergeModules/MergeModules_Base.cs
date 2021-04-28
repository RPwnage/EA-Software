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

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Core;


namespace EA.Eaconfig.Build.MergeModules
{
	internal abstract class MergeModules_Base<T> where T : ProcessableModule 
	{
		protected readonly Project Project;
		protected readonly IEnumerable<IModule> BuildModules;
		protected readonly Log Log;
		protected readonly string LogPrefix;

		protected readonly MergeModules_Data _merge_data;

		protected readonly T TargetModule;

		protected IPackage _targetPackage;
		protected Project _targetProject;

		private readonly HashSet<string> _srcModKeys;

		protected MergeModules_Base(MergeModules_Data data, Project project, IEnumerable<IModule> buildmodules)
		{
			Project = project;
			Log = Project.Log;
			LogPrefix = "[mergemodules] ";
			BuildModules = buildmodules;
			_merge_data = data;
			_targetPackage = GetTargetPackage();
			_targetProject = _targetPackage.Project;

			TargetModule = CreateModule() as T;

			_srcModKeys = new HashSet<string>(data.SourceModules.Select(bm => bm.Key));
		}

		private IModule TopSourceModule
		{
			get
			{
				return _merge_data.SourceModules.First();
			}
		}

		protected virtual BuildType GetTargetBuildType()
		{
				return (TopSourceModule as ProcessableModule).BuildType;
		}

		protected virtual IPackage GetTargetPackage()
		{
			return TopSourceModule.Package;
		}

		protected virtual string GetTargetGroupName()
		{
			return String.Format("{0}.{1}", _merge_data.TargetModule.Group, _merge_data.TargetModule.Name);
		}

		protected virtual string GetTargetGroupSourceDir()
		{
			return String.Empty;
		}

		private ProcessableModule CreateModule()
		{
			if (_merge_data.SourceModules.Count > 0)
			{
				// set the default value of outputname unless it has been overridden
				string propname = TargetPropGroupName("outputname");
				if (_targetProject.Properties[propname] == null)
				{
					_targetProject.Properties[propname] = _merge_data.TargetModule.Name;
				}

				var targetModule = ModuleFactory.CreateModule(_merge_data.TargetModule.Name, GetTargetGroupName(), GetTargetGroupSourceDir(), _merge_data.TargetModule.Config, _merge_data.TargetModule.Group, GetTargetBuildType(), _targetPackage);

				targetModule.IntermediateDir = PathString.MakeNormalized(ModulePath.Private.GetModuleIntermedidateDir(_targetProject, _merge_data.TargetModule.Name, _merge_data.TargetModule.Group.ToString()));
				targetModule.OutputDir = TopSourceModule.OutputDir;

				return targetModule;
			}
			return null;
		}

		protected void Merge()
		{
			if (TargetModule != null)
			{
				Log.Status.WriteLine(LogPrefix+"Created merged module {0} [{1}] from:{2}{3}", TargetModule.Name, TargetModule.Configuration.Name, Environment.NewLine, _merge_data.SourceModules.ToString(Environment.NewLine, m => "        " + m.Name));


				foreach (ProcessableModule srcmod in _merge_data.SourceModules)
				{
					MergeProcessableModule(srcmod);

					MergeSpecificModuleType(srcmod as T);

					SetDependencies(BuildModules, srcmod);
				}

				FinalizeMerge();
			}
		}

		protected abstract void MergeSpecificModuleType(T srcmod);


		protected abstract bool InitMerge();

		protected abstract bool FinalizeMerge();

		protected void MergeFiles(ref FileSet target, FileSet src)
		{
			if (src != null && (src.Includes.Count > 0 || src.Excludes.Count > 0))
			{
				if (target == null)
				{
					target = new FileSet();
				}
				target.Include(src, baseDir: src.BaseDirectory);
			}
		}

		protected void MergeFiles(FileSet target, FileSet src)
		{
			if (src != null && (src.Includes.Count > 0 || src.Excludes.Count > 0))
			{
				target.Include(src, baseDir: src.BaseDirectory);
			}
		}

		protected void MergeArray(ref string target, string src, Func<string, IEnumerable<string>> toArray, Func<IEnumerable<string>, string> toString, IEqualityComparer<string> comparer = null)
		{
			var aTarget = toArray(target);

			if (MergeArray<String>(ref aTarget, toArray(src), comparer))
			{
				target = toString(aTarget);
			}
		}

		protected bool MergeArray<U>(ref IEnumerable<U> target, IEnumerable<U> src, IEqualityComparer<U> comparer = null)
		{
			bool targetUpdated = false;
			if (src != null && !src.IsNullOrEmpty())
			{
				if (target.IsNullOrEmpty())
				{
					target = src;
					targetUpdated = true;
				}
				else
				{
					int count = target.Count();

					if (comparer == null)
						target.Union(src);
					else
						target.Union(src, comparer);

					if (target.Count() > count)
					{
						targetUpdated = true;
					}
				}
			}

			return targetUpdated;

		}

		protected void MergeSettings(ref bool target, bool src)
		{
			if (src)
			{
				target = src;
			}
		}

		protected void MergeSettings(ref PathString target, PathString src)
		{
			if (!src.IsNullOrEmpty())
			{
				target = src;
			}
		}

		protected void MergeSettings(ref String target, String src)
		{
			if (!String.IsNullOrEmpty(src))
			{
				target = src;
			}
		}

		protected void MergeAssets(Module mergemodule)
		{
			if(mergemodule.DeployAssets)
			{
				TargetModule.DeployAssets = mergemodule.DeployAssets;
			}
			MergeFiles(TargetModule.Assets, mergemodule.Assets);
		}

		protected void MergeSettings(ref OptionSet target, OptionSet src)
		{
			if (src != null)
			{
				if (target == null)
				{
					target = new OptionSet();
				}
				target.Append(src);
			}
		}

		protected string TargetPropGroupName(string name)
		{
			if (!name.StartsWith("."))
			{
				name = "." + name;
				
			}
			return String.Format("{0}.{1}{2}",_merge_data.TargetModule.Group, _merge_data.TargetModule.Name , name);
		}

		private void MergeIModule(Module mergemodule)
		{
			MergeDependencies(mergemodule);

			
			// PathString OutputDir

			// PathString ScriptFile
			
			
			MergeAssets(mergemodule);

			MergeBuildSteps(mergemodule);

			MergeCustomBuildSteps(mergemodule);

			// IPublicData Public(IModule parentModule)

			//IEnumerable<Tool> Tools

			// FileSet ExcludedFromBuild_Files;
			MergeFiles(TargetModule.ExcludedFromBuild_Files, mergemodule.ExcludedFromBuild_Files);
			// FileSet ExcludedFromBuild_Sources;
			MergeFiles(TargetModule.ExcludedFromBuild_Sources, mergemodule.ExcludedFromBuild_Sources);

		}

		private void MergeProcessableModule(ProcessableModule mergemodule)
		{
			MergeIModule(mergemodule);
		}

		private void MergeBuildSteps(Module module)
		{
			foreach (var bs in module.BuildSteps)
			{
				bool nonEmpty = false;

				if(bs.TargetCommands.Count > 0)
				{
					nonEmpty = true;
				}
				else
				{
					foreach(var command in bs.Commands)
					{
						if(!command.Executable.IsNullOrEmpty() || String.IsNullOrEmpty(command.Script))
						{
							nonEmpty = true;
							break;
						}
					}
				}
				if(nonEmpty)
				{
					foreach(var command in bs.Commands)
					{
						 command.CreateDirectories.Clear();
					}

					TargetModule.BuildSteps.Add(bs);
				}

				TargetModule.BuildSteps.AddRange(module.BuildSteps);
			}
		}

		private void MergeCustomBuildSteps(Module module)
		{
			foreach(var tool in module.Tools)
			{
				if (tool.IsKindOf(Tool.TypePreCompile))
				{
					TargetModule.AddTool(tool);
				}
			}
		}

		private void MergeDependencies(Module module)
		{
			foreach (var dep in module.Dependents)
			{
				if (!dep.Dependent.IsKindOf(Module.ExcludedFromBuild))
				{
					if (!_merge_data.SourceModules.Any(m => m.Key == dep.Dependent.Key) && TargetModule.Key != dep.Dependent.Key)
					{
						TargetModule.Dependents.Add(dep.Dependent, dep.Type);
					}
				}
			}
		}

		private void SetDependencies(IEnumerable<IModule> buildmodules, Module module)
		{
			module.SetType(Module.ExcludedFromBuild);

			foreach (var mod in buildmodules)
			{
				uint type = DependencyTypes.Undefined;
				foreach (var d in mod.Dependents.Where(dep => dep.Dependent.Key == module.Key))
				{
					type |= d.Type;

					if (d.IsKindOf(DependencyTypes.Link))  // Add Libraries
					{
						if (mod.IsKindOf(Module.DotNet | Module.Managed))
						{
							var assembly = d.Dependent.DynamicLibOrAssemblyOutput();
							if (assembly != null)
							{
								if (mod.IsKindOf(Module.DotNet))
								{
									(mod as Module_DotNet).Compiler.DependentAssemblies.Exclude(assembly.Path);
								}
								else
								{
									var native = mod as Module_Native;
									if (native != null && native.Cc != null)
									{
										native.Cc.Assemblies.Exclude(assembly.Path);
									}
								}
							}
						}
					}
				}

				if (type != DependencyTypes.Undefined && !mod.IsKindOf(Module.ExcludedFromBuild) && mod.Key != TargetModule.Key)
				{
					mod.Dependents.Add(TargetModule, type);
					// Because merging is only enabled for sln generation we do not need to add dependent assemblies explicitly;
				}
			}
		}

		protected void AdjustLibraryReferences(PathString sourceLibraryOutputPath, PathString mergedLibraryOutputPath)
		{
			// Foreach module in this package remove direct dependency on the library.
			// If module is not one of the src merge modules - add dependency on the target library.
			if (!sourceLibraryOutputPath.IsNullOrEmpty())
			{
				foreach (Module_Native mod in BuildModules.Where(m => m.IsKindOf(Module.Program | Module.Managed | Module.DynamicLibrary) && m is Module_Native))
				{
					if (!_srcModKeys.Contains(mod.Key) && mod.Link != null && mod.Link.Libraries.FileItems.Any(fi => fi.Path == sourceLibraryOutputPath))
					{
						mod.Link.Libraries.Exclude(sourceLibraryOutputPath.Path);
						mod.Link.Libraries.Include(mergedLibraryOutputPath.Path);
					}
				}
			}
		}
	}
}