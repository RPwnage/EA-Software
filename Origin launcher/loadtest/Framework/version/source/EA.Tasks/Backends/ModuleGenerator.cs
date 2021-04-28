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
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;
using EA.FrameworkTasks.Functions;

namespace EA.Eaconfig.Backends
{

	public abstract class ModuleGenerator : IMultiConfiguration
	{
		public static readonly IModuleGeneratorEqualityComparer EqualityComparer = new IModuleGeneratorEqualityComparer();

		// TODO: These are backwards compatibility stubs needed by older versions of XcodeProjectizer, remove them after the next compatibility break with xcode projectizer
		public class SourceControlData { }
		protected virtual SourceControlData CreateSourceControlData() { return null; }

		protected ModuleGenerator(Generator buildgenerator, IEnumerable<IModule> modules)
		{
			Log = buildgenerator.Log;
			BuildGenerator = buildgenerator;
			Modules =  new List<IModule>(modules);

			IModule m = Modules.FirstOrDefault();

			if (m == null)
			{
				throw new BuildException("ModuleGenerator internal error - module list is empty");
			}

			Key = MakeKey(m);


			var genName = Generator.GetModuleGeneratorName(m);
			Generator.DuplicateNameTypes dupType;
			if (BuildGenerator.DuplicateProjectNames != null && BuildGenerator.DuplicateProjectNames.TryGetValue(m.Key, out dupType))
			{
				if ((dupType & Generator.DuplicateNameTypes.DuplicateInGroup) == Generator.DuplicateNameTypes.DuplicateInGroup)
				{
					if (m.BuildGroup != BuildGroups.runtime)
					{
						genName = genName + "-" + Enum.GetName(typeof(BuildGroups), m.BuildGroup);
					}
				}
				if ((dupType & Generator.DuplicateNameTypes.DuplicateInPackage) == Generator.DuplicateNameTypes.DuplicateInPackage)
				{
					genName = m.Package.Name + "-" + genName;
				}
			}

			_name = genName;

			ModuleName = m.Name;
			Package = m.Package;
			BuildDir = m.Package.PackageBuildDir;

			AllConfigurations = Modules.Select(mod => mod.Configuration).Distinct().ToList();
			

			AddModuleGenerators(m);

			IncludeBuildScripts = false;

			ProjectFileNameWithoutExtension = Name;
			_outputDir = GetDefaultOutputDir();
			{
				var templates = BuildGenerator.GeneratorTemplates;
				if(templates != null)
				{
					var outputdir = templates.ProjectDirTemplate.ReplaceTemplateParameters("gen.projectdir.template", new string[,] 
					{ 
						{"%outputdir%", _outputDir.Path},
						{"%package.builddir%",  m.Package.PackageBuildDir.Path},
						{"%package.root%", PackageFunctions.GetPackageRoot(m.Package.Project, m.Package.Name)},
						{"%package.configbuilddir%", m.Package.PackageConfigBuildDir.Path},
						{"%package.name%", Package.Name},
						{"%module.name%", ModuleName},
						{"%config.name%", m.Configuration.Name}
					});

					if(!String.IsNullOrEmpty(outputdir))
					{
						_outputDir = PathString.MakeNormalized(outputdir);
					}

					var filename = templates.ProjectFileNameTemplate.ReplaceTemplateParameters("gen.projectname.template", new string[,] 
					{ 
						{"%outputname%",  Name }
					});

					if(!String.IsNullOrEmpty(filename))
					{
						ProjectFileNameWithoutExtension = filename;
					}
					else if (BuildGenerator.GroupNumber > 0)
					{
						ProjectFileNameWithoutExtension = String.Format("{0}_{1}", ProjectFileNameWithoutExtension, BuildGenerator.GroupNumber);
					}
				}
			}
		}

		protected virtual void AddModuleGenerators(IModule module)
		{
			if (!BuildGenerator.ModuleGenerators.ContainsKey(Key))
			{
				BuildGenerator.ModuleGenerators.Add(Key, this);
			}
			else
			{
				Log.Warning.WriteLine("Duplicate generator entry {0}", Key);
			}
		}

		// Return a list containing the module generator and all of its dependents (recursive)
		public IEnumerable<ModuleGenerator> Flatten()
		{
			List<ModuleGenerator> flattened = new List<ModuleGenerator>();

			// Add myself first
			flattened.Add(this);

			// Now add my dependents
			foreach (ModuleGenerator mg in Dependents)
			{
				flattened.AddRange(mg.Flatten());
			}

			return (flattened.OrderedDistinct());
		}

		public static string MakeKey(IModule m)
		{
			return m.Package.Name + "-" + m.Package.Version + ":" + m.BuildGroup + "-" + m.Name;
		}

		public string GetProjectPath(PathString path, bool addDot = false)
		{
			if (path == null)
			{
				return String.Empty;
			}

			return GetProjectPath(path.Normalize(PathString.PathParam.NormalizeOnly).Path, addDot);
		}

		public string GetProjectPath(string path, bool addDot = false)
		{
			string projectPath = path;

			switch (BuildGenerator.PathMode)
			{
				case Generator.PathModeType.Auto:
					{
						if (PathUtil.IsPathInDirectory(path, OutputDir.Path))
						{
							projectPath = PathUtil.RelativePath(path, OutputDir.Path, addDot: addDot);
							if (projectPath == String.Empty)
							{
								projectPath = "./";
							}
						}
						else
						{
							projectPath = path;
						}

						if (projectPath.Length >= 260)
						{
							Log.Warning.WriteLine("project path is longer than 260 characters which can lead to unbuildable project: {0}", projectPath);
						}
					}
					break;
				case Generator.PathModeType.Relative:
					{
						bool isSdk = false;
						if (BuildGenerator.IsPortable && !PathUtil.IsPathInDirectory(path, OutputDir.Path))
						{
							// Check if path is an SDK path
							projectPath = BuildGenerator.PortableData.FixPath(path, out isSdk);
						}
						if (path.StartsWith("%") || path.StartsWith("\"%"))
						{
							isSdk = true;
						}

						if (!isSdk)
						{
							projectPath = PathUtil.RelativePath(path, OutputDir.Path, addDot: addDot);

							// Need to check the combined length which the relative path is going to have
							// before choosing whether to relativize or not.  Before we had this check in place
							// it was possible for <nanttovcproj> to generate an unbuildable VCPROJ, because
							// the relativized path overflowed the maximum path length limit.
							string combinedFilePath = Path.Combine(OutputDir.Path, projectPath);
							if (combinedFilePath.Length >= 260)
							{
								 Log.ThrowWarning
								(
									 Log.WarningId.LongPathWarning, Log.WarnLevel.Normal,
									"Relative path combined with project path is longer than 260 characters which can lead to unbuildable project: {0}", combinedFilePath
								);
							}
						}
					}
					break;
				case Generator.PathModeType.Full:
					{
						projectPath = path;

						if (projectPath.Length >= 260)
						{
							Log.ThrowWarning
							(
								Log.WarningId.LongPathWarning, Log.WarnLevel.Normal,
								"project path is longer than 260 characters which can lead to unbuildable project: {0}", projectPath
							);
						}
					}
					break;
			}

			return projectPath.TrimQuotes();
		}

		public readonly string Key;

		public readonly Log Log;

		public virtual string Name  
		{
			get { return _name; }
		}

		public readonly IPackage Package;

		public readonly string ModuleName;

		public readonly PathString BuildDir;

		public virtual PathString OutputDir
		{
			get { return _outputDir; }
		}

		protected string ProjectFileNameWithoutExtension { get; }

		public bool IncludeBuildScripts;

		/// <summary>
		/// module generator output directory relative to the to solution build root:
		/// </summary>
		public string RelativeDir
		{
			get
			{
				if (_relativeDir == null)
				{
					_relativeDir = PathUtil.RelativePath(OutputDir.Path, BuildGenerator.OutputDir.Path);
				}
				return _relativeDir;
			}
		} private string _relativeDir;

		/// <summary>
		/// Full path to package directory
		/// </summary>
		internal virtual string PackageDir
		{
			get
			{
				// All modules in a project refer to the same package. Take first  
				String packageDir = String.Empty;
				packageDir = Package.Dir.Path;
				return packageDir;
			}
		}

		public readonly List<IModule> Modules;

		public readonly List<ModuleGenerator> Dependents = new List<ModuleGenerator>();

		public readonly List<IProject> PreGeneratedDependents = new List<IProject>();

		public readonly Generator BuildGenerator;

		// Make sure this is stored as list and not IEnumerable since it is a big performance optimization
		public List<Configuration> AllConfigurations { get; set; }

		public IDictionary<Configuration, Configuration> ConfigurationMap { get; set; } = new Dictionary<Configuration, Configuration>();

		#region Abstract Methods

		public virtual void Generate()
		{
			WriteProject();
		}

		public virtual void PostGenerate()
		{
		}

		public abstract string ProjectFileName { get; }

		public abstract void WriteProject();

		#endregion Abstract Methods

		public virtual bool CollectAutoDependencies
		{
			get
			{
				return null != Modules.FirstOrDefault(m => m.CollectAutoDependencies);
			}
		}

		#region Protected Virtual Methods

		protected virtual void Write()
		{
		}

		protected virtual PathString GetDefaultOutputDir()
		{
			return BuildGenerator.GenerateSingleConfig ? Package.PackageConfigBuildDir : Package.PackageBuildDir;
		}
		#endregion

		public virtual void AddExcludedDependency(IModule from, IModule to) { }

		private string _name;
		private PathString _outputDir;
	}

	public class IModuleGeneratorEqualityComparer : IEqualityComparer<ModuleGenerator>
	{
		public bool Equals(ModuleGenerator x, ModuleGenerator y)
		{
			return x.Key == y.Key;
		}

		public int GetHashCode(ModuleGenerator obj)
		{
			return obj.Key.GetHashCode();
		}
	}

}
