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
using System.Threading.Tasks;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Threading;
using NAnt.Core.Util;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends
{
	public abstract class Generator
	{
		//------ Static section to select modules and solution names ------------------------------------------------------------------------------------------------------

		public enum DuplicateNameTypes { NoDuplicates = 0, DuplicateInPackage = 1, DuplicateInGroup = 2 };

		public enum PathModeType { Auto = 0, Relative = 1, Full = 2 };

		public class GeneratedLocation : IEquatable<GeneratedLocation>
		{
			public GeneratedLocation(PathString dir, string name, GeneratorTemplatesData templates)
			{
				Dir = dir;
				Name = name;
				GeneratorTemplates = templates;

				_key = (dir.Path +"/" + name).ToLowerInvariant();
			}
			public readonly PathString Dir;
			public readonly string Name;
			public readonly GeneratorTemplatesData GeneratorTemplates;

			private readonly string _key;

			public bool Equals(GeneratedLocation other) 
			{
				if (other == null) 
					return false;

				return (_key == other._key);
			}

			public override bool Equals(Object obj)
			{
				if (obj == null) 
					return base.Equals(obj);

				return Equals(obj as GeneratedLocation);   
			}   

			public override int GetHashCode()
			{
				return _key.GetHashCode();
			}

			public static bool operator == (GeneratedLocation loc1, GeneratedLocation loc2)
			{
				if ((object)loc1 == null || ((object)loc2) == null)
					return Object.Equals(loc1, loc2);

				return loc1.Equals(loc2);
			}

		   public static bool operator != (GeneratedLocation loc1, GeneratedLocation loc2)
		   {
			  if (loc1 == null || loc2 == null)
				 return ! Object.Equals(loc1, loc2);

			  return ! (loc1.Equals(loc2));
		   }
		}

		public static IEnumerable<IModule> GetTopModules(Project project)
		{
			return project.BuildGraph().TopModules;
		}

		public static string GetModuleGeneratorName(IModule module)
		{
			var name = module.Name;

			if (module.Package != null && module.Package.Project != null)
			{
				var template =	module.Package.Project.GetPropertyValue(module.BuildGroup + "." + module.Package.Name + "." + module.Name + ".gen.name.template") 
								?? (module.Package.Project.GetPropertyValue(module.GroupName + ".gen.name.template") 
								?? module.Package.Project.GetPropertyValue(module.BuildGroup + ".gen.name.template"));

				var t_name = template.ReplaceTemplateParameters("gen.name.template", new string[,] 
					{ 
								{"%outputname%",  module.Name }
					});

				if (!String.IsNullOrEmpty(t_name))
				{
					name = t_name;
				}
			}
			return name;
		}

		// Group top modules by solution name
		public static IDictionary<string, DuplicateNameTypes> FindDuplicateProjectNames(IEnumerable<IModule> modules)
		{
			var duplicateProjectNames = new Dictionary<string, DuplicateNameTypes>();

			var groupsWithDups = modules.GroupBy(m => m.Configuration.Name + ":" + m.Name).Where(g=>g.Count() > 1);

			foreach (var group in groupsWithDups)
			{
				DuplicateNameTypes dupType = DuplicateNameTypes.NoDuplicates;

				if (group.Select(m => m.BuildGroup).Distinct().Count() > 1)
				{
					dupType |= DuplicateNameTypes.DuplicateInGroup;
				}
				if (group.Select(m => m.Package.Name).Distinct().Count() > 1)
				{
					var mnames = group.Select(m => GetModuleGeneratorName(m));
					if (mnames.Distinct().Count() <= 1)
						dupType |= DuplicateNameTypes.DuplicateInPackage;
				}
				if (dupType != DuplicateNameTypes.NoDuplicates)
				{
					foreach (var mod in group)
					{
						DuplicateNameTypes val;
						if (duplicateProjectNames.TryGetValue(mod.Key, out val))
						{
							val |= dupType;
						}
						else
						{
							val = dupType;
						}
						duplicateProjectNames[mod.Key] = val;
					}
				}
			}
			return duplicateProjectNames;
		}

		public static GeneratedLocation GetGeneratedLocation(Project project, BuildGroups buildgroup,  GeneratorTemplatesData templates, bool generateSingleConfig, bool splitByGroupNames)
		{
			string name = project.Properties["package.name"];
			string dir = project.Properties["package.builddir"];
			if (generateSingleConfig)
			{
				dir += "/" + project.Properties["config"];
			}

			if (splitByGroupNames && buildgroup != BuildGroups.runtime)
			{
				name = String.Format("{0}-{1}", name, buildgroup);
			}

			if (templates != null)
			{
				var templatedDir = templates.SlnDirTemplate.ReplaceTemplateParameters("gen.slndir.template", new string[,] 
								{ 
										{"%outputdir%",  dir},
										{"%package.builddir%",  project.Properties["package.builddir"]},
										{"%buildroot%",         project.Properties[Project.NANT_PROPERTY_PROJECT_BUILDROOT]}
								});

				if (!String.IsNullOrEmpty(templatedDir))
				{
					dir = templatedDir;
				}

				var templatedName = templates.SlnNameTemplate.ReplaceTemplateParameters("gen.slnname.template", new string[,] 
								{ 
									{"%outputname%",  name}
								});

				if (!String.IsNullOrEmpty(templatedName))
				{
					name = templatedName;
				}
			}

			return new Generator.GeneratedLocation(PathString.MakeNormalized(dir), name, templates);
		}


		/// <summary>Group top modules by solution name</summary>
		public static IEnumerable<IGrouping<GeneratedLocation, IModule>> GetTopModuleGroups(Project project, IEnumerable<IModule> topmodules, bool generateSingleConfig, bool splitByGroupNames)
		{
			return topmodules.GroupBy(m =>
				{
					var pm = m as ProcessableModule;

					GeneratorTemplatesData templates = (pm != null && pm.AdditionalData != null) ? pm.AdditionalData.GeneratorTemplatesData : null;
					var buildgroup = (pm != null) ? pm.BuildGroup : BuildGroups.runtime;

					return GetGeneratedLocation(project, buildgroup,  templates, generateSingleConfig, splitByGroupNames);

				});
		}

		public Generator(Log log, string name, PathString outputdir)
		{
			Log = log;
			Name = name;
			OutputDir = outputdir;
			GroupNumber = 0;

			PathMode = PathModeType.Auto;

			ModuleGenerators = new Dictionary<string, ModuleGenerator>();
			PreGeneratedModules = new Dictionary<string, IProject>();
		}

		public readonly Log Log;

		/// <summary>
		/// Generator output name. 
		/// </summary>
		public readonly string Name;

		/// <summary>
		/// Full path to the solution file without solution file name. 
		/// </summary>
		public readonly PathString OutputDir;

		/// <summary>
		/// Generated files
		/// </summary>
		public abstract IEnumerable<PathString> GeneratedFiles { get;  }

		public PathModeType PathMode { get; protected set; }

		/// <summary>
		/// Sequential number of a group when solution files split based on output directory or file names. 
		/// </summary>
		public int GroupNumber { get; private set; }

		public bool GenerateSingleConfig { get; private set; } = false;


		/// <summary>
		/// In portable mode SDK paths are replaced with environment variables and all other paths (including paths in tools options) are made relative whenever possible.
		/// The goal is to have generated files that aren't tied to particular computer
		/// </summary>
		public bool IsPortable
		{
			get
			{
				return _isPortable;
			}
			protected set
			{
				_isPortable = value;
				if (_isPortable)
				{
					PathMode = PathModeType.Relative;
				}
			}
		} private bool _isPortable = false;

		public string SlnFrameworkFilesRoot
		{
			get
			{
				if (String.IsNullOrEmpty(mSlnFrameworkFilesRoot))
				{
					if (IsPortable)
					{
						if (StartupProject.Properties.Contains("eaconfig.portable-solution.framework-files-outputdir"))
						{
							mSlnFrameworkFilesRoot = System.IO.Path.GetFullPath(StartupProject.Properties.ExpandProperties(StartupProject.Properties["eaconfig.portable-solution.framework-files-outputdir"]));
						}
						else
						{
							mSlnFrameworkFilesRoot = System.IO.Path.Combine(StartupProject.Properties["nant.project.temproot"], "MSBuild");
						}
					}
					else
					{
						mSlnFrameworkFilesRoot = NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path;
					}
				}
				return mSlnFrameworkFilesRoot;
			}
		}
		private string mSlnFrameworkFilesRoot = null;


		public Portable PortableData { get; private set; }

		public Configuration StartupConfiguration { get; private set; }

		public IEnumerable<Configuration> Configurations
		{
			get
			{
				return _configurations;
			}
		} private List<Configuration> _configurations;


		public readonly IDictionary<string, ModuleGenerator> ModuleGenerators;

		public Dictionary<string, IProject> PreGeneratedModules;

		public GeneratorTemplatesData GeneratorTemplates { get; private set; }

		public abstract ModuleGenerator StartupModuleGenerator { get; }

		public IDictionary<string, DuplicateNameTypes> DuplicateProjectNames;

		public abstract void Generate();

		protected abstract ModuleGenerator CreateModuleGenerator(IEnumerable<IModule> modules);

		/// <summary>
		/// The "project" that was used to Initialize this generator.
		/// </summary>
		public Project StartupProject { get; private set; }

		public List<IModule> IncludedModules { get; private set; }

		public HashSet<IModule> ExcludedModules;

		private void SetDependencies(ModuleGenerator g)
		{
			var dependentgenerators = new Dictionary<string, ModuleGenerator>();

			uint includedDeps = DependencyTypes.Build | (g.CollectAutoDependencies ? DependencyTypes.AutoBuildUse : 0);

			foreach (var module in g.Modules)
			{
				var handled = new HashSet<IModule>();
				foreach (var depmoduleDep in module.Dependents)
				{
					var depmodule = depmoduleDep.Dependent;

					if (!depmoduleDep.IsKindOf(includedDeps) || depmodule is Module_UseDependency)
						continue;

					if (!handled.Add(depmodule))
						continue;

					var genkey = ModuleGenerator.MakeKey(depmodule);
					if (!dependentgenerators.ContainsKey(genkey))
					{
						ModuleGenerator depgenerator;
						if (ModuleGenerators.TryGetValue(genkey, out depgenerator))
						{
							dependentgenerators.Add(genkey, depgenerator);
						}
						else if (ExcludedModules == null || !ExcludedModules.Contains(depmodule))
						{
							Log.Warning.WriteLine("ModuleGenerator '{0}' not found", genkey);
						}
						else
						{
							g.AddExcludedDependency(module, depmodule);
						}
					}
				}
			}

			g.Dependents.AddRange(dependentgenerators.Values.OrderBy(mg => mg.Key, StringComparer.InvariantCulture));
		}

		// back compatibility trampoline - overidden in xcodeprojectizer
		public virtual bool Initialize(Project project, IEnumerable<string> configurations, string startupconfigName, IEnumerable<IModule> topmodules, bool generateSingleConfig, GeneratorTemplatesData generatorTemplates, int groupnumber)
		{
			return Initialize(project, configurations, startupconfigName, topmodules, null, generateSingleConfig, generatorTemplates, groupnumber);
		}

		/// <summary>
		/// Populate module generators.
		/// </summary>
		public virtual bool Initialize(Project project, IEnumerable<string> configurations, string startupconfigName, IEnumerable<IModule> topmodules, IEnumerable<IModule> excludedRootModules, bool generateSingleConfig, GeneratorTemplatesData generatorTemplates, int groupnumber)
		{
			StartupProject = project;
			GeneratorTemplates = generatorTemplates;
			GroupNumber = groupnumber;
			GenerateSingleConfig = generateSingleConfig;

			var confignames = new HashSet<string>(configurations);
			
			InitPortableData(topmodules);

			var allModules = topmodules.Union
			(
				topmodules.SelectMany(m => m.Dependents.FlattenAll(DependencyTypes.Build),
				(m, d) => d.Dependent).Where(m => !(m is Module_UseDependency))
			);

			ExcludedModules = new HashSet<IModule>();

			// Collect all modules that should be excluded
			if (excludedRootModules != null)
			{
				foreach (var m in excludedRootModules)
				{
					foreach (var dep in m.Dependents.FlattenAll(DependencyTypes.Build))
					{
						ExcludedModules.Add(dep.Dependent);
					}
				}
			}


			IncludedModules = new List<IModule>();

			// collect all modules that should be included in the solution.
			foreach (var m in allModules)
			{
				if (ExcludedModules.Contains(m))
					continue;

				if (m.Package.Project.Properties.GetBooleanProperty("package." + m.Package.Name + "." + m.Name + ".exclude-from-solution"))
				{
					ExcludedModules.Add(m);
					continue;
				}

				IncludedModules.Add(m);
			}


			_configurations = IncludedModules.Select(m => m.Configuration).Distinct().Where(c => confignames.Contains(c.Name)).OrderBy(c=> c.Name).ToList();

			if (_configurations.Count == 0)
			{
				Log.Warning.WriteLine("No buildable configurations found. Skipping solution {0} generation.", Name);
				return false;
			}

			StartupConfiguration = Configurations.SingleOrDefault(c => c.Name == startupconfigName) ?? Configurations.FirstOrDefault();
			CreateModules(project, topmodules);
			GenerateModules();

			return true;
		}

		protected virtual void CreateModules(Project startupProject, IEnumerable<IModule> topmodules)
		{
			bool includeBuildScripts = startupProject.Properties.GetBooleanProperty("generator.includebuildscripts");

			foreach (var group in IncludedModules.GroupBy(m => ModuleGenerator.MakeKey(m), m => m))
			{
				var generator = CreateModuleGenerator(group.OrderBy(m => m.Configuration.Name));
				generator.IncludeBuildScripts = includeBuildScripts;
			}

			foreach (var module in topmodules.Where(m => Configurations.Contains(m.Configuration)))
			{
				Configuration solutionconfig = module.Configuration;

				SetConfigurationMapEntry(solutionconfig, module);

				foreach (var dep in module.Dependents.FlattenAll(DependencyTypes.Build).Where(d => !(d.Dependent is Module_UseDependency)).Distinct())
				{
					SetConfigurationMapEntry(solutionconfig, dep.Dependent);
				}
			}
		}

		protected void GenerateModules()
		{
			try
			{
				// Set Dependencies for each generator
#if NANT_CONCURRENT
				if (Project.NoParallelNant == false)
				{
					Parallel.ForEach(ModuleGenerators.Values, g =>
					{
						SetDependencies(g);
					});
				}
				else
#endif
				{
					foreach (var g in ModuleGenerators.Values)
					{
						SetDependencies(g);
					}
				}

#if NANT_CONCURRENT
				if (Project.NoParallelNant == false)
				{
					Parallel.ForEach(ModuleGenerators.Values, generator => generator.Generate());
				}
				else
#endif
				{
					foreach (var generator in ModuleGenerators.Values) generator.Generate();
				}

			}
			catch (Exception ex)
			{
				ThreadUtil.RethrowThreadingException(String.Format("Error generating '{0}'", Name), ex);
			}
		}

		public virtual void PostGenerate()
		{
			foreach (ModuleGenerator generator in ModuleGenerators.Values)
			{
				generator.PostGenerate();
			}
		}

		private void InitPortableData(IEnumerable<IModule> topmodules)
		{
			if (IsPortable)
			{
				PortableData = new Portable();

				var tm = topmodules.FirstOrDefault();
				if (tm != null && tm.Package.Project != null)
				{
					foreach (var project in tm.Package.Project.BuildGraph().Packages.Where(p=>p.Value.Project!= null).Select(p => p.Value.Project).Distinct())
					{
						PortableData.InitConfig(project);
					}
				}
			}
		}

		protected void SetConfigurationMapEntry(Configuration solutionconfig, IModule module)
		{
			ModuleGenerator modgenerator;
			if (ModuleGenerators.TryGetValue(ModuleGenerator.MakeKey(module), out modgenerator))
			{
				lock (modgenerator.ConfigurationMap)
				{
					if (!modgenerator.ConfigurationMap.ContainsKey(solutionconfig))
					{
						modgenerator.ConfigurationMap.Add(solutionconfig, module.Configuration);
					}
				}
				return;
			}

			if (ExcludedModules == null || !ExcludedModules.Contains(module))
				Log.Warning.WriteLine("Module generator for module '{0}' does not exist", module.Key);
		}
	}
}
