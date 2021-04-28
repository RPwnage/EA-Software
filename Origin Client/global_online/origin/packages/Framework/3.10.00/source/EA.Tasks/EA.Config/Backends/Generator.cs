using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading;
using System.Threading.Tasks;


using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Threading;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;


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
                var template = module.Package.Project.GetPropertyValue(module.GroupName + ".gen.name.template") ?? module.Package.Project.GetPropertyValue(module.BuildGroup + ".gen.name.template");

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

            DuplicateNameTypes dupType = DuplicateNameTypes.NoDuplicates;

            foreach (var group in groupsWithDups)
            {
                if (group.Select(m => m.BuildGroup).Distinct().Count() > 1)
                {
                    dupType |= DuplicateNameTypes.DuplicateInGroup;
                }
                if (group.Select(m => m.Package.Name).Distinct().Count() > 1)
                {
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

        public static Generator.GeneratedLocation GetGeneratedLocation(Project project, BuildGroups buildgroup,  GeneratorTemplatesData templates, bool generateSingleConfig, bool splitByGroupNames)
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


        // Group top modules by solution name
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
            _groupnumber = 0;

            PathMode = PathModeType.Auto;

            ModuleGenerators = new Dictionary<string, ModuleGenerator>();
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

        public PathModeType PathMode
        {
            get { return _pathMode; }
            protected set { _pathMode = value; }
        } private PathModeType _pathMode;

        /// <summary>
        /// Sequential number of a group when solution files split based on output directory or file names. 
        /// </summary>
        public int GroupNumber
        {
            get
            {
                return _groupnumber;
            }
        } private int _groupnumber;

        public bool GenerateSingleConfig
        {
            get
            {
                return _generateSingleConfig;
            }
        } private bool _generateSingleConfig = false;


        /// <summary>
        /// In portable mode SDK paths are replaced with environment variables and all other paths (including paths in tools optins) are made relative whenever possible.
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


        public Portable PortableData
        {
            get { return _portableData; }

        } private Portable _portableData;

        public Configuration StartupConfiguration
        {
            get
            {
                return _startupConfiguration;
            }
        } private Configuration _startupConfiguration;

        public IEnumerable<Configuration> Configurations
        {
            get
            {
                return _configurations;
            }
        } private List<Configuration> _configurations;


        public readonly IDictionary<string, ModuleGenerator> ModuleGenerators;


        public GeneratorTemplatesData GeneratorTemplates
        {
            get { return _generatorTemplates; }
        }private GeneratorTemplatesData _generatorTemplates;


        public abstract ModuleGenerator StartupModuleGenerator { get; }

        public IDictionary<string, DuplicateNameTypes> DuplicateProjectNames;

        public abstract void Generate();

        protected abstract ModuleGenerator CreateModuleGenerator(IEnumerable<IModule> modules);

        /// <summary>
        /// Populate module generators.
        /// </summary>
        public virtual bool Initialize(Project project, IEnumerable<string> configurations, string startupconfigName, IEnumerable<IModule> topmodules, bool generateSingleConfig, GeneratorTemplatesData generatorTemplates, int groupnumber)
        {
            _generatorTemplates = generatorTemplates;
            _groupnumber = groupnumber;
            _generateSingleConfig = generateSingleConfig;

            var confignames = new HashSet<string>(configurations);

            bool includeBuildScripts = project.Properties.GetBooleanProperty("generator.includebuildscripts");

            InitPortableData(topmodules);

            var modules = topmodules.Union(topmodules.SelectMany(m => m.Dependents.FlattenAll(DependencyTypes.Build), (m, d) => d.Dependent).Where(m => !(m is Module_UseDependency)));

            _configurations = modules.Select(m => m.Configuration).Distinct().Where(c => confignames.Contains(c.Name)).OrderBy(c=> c.Name).ToList();

            if (_configurations.Count == 0)
            {
                Log.Warning.WriteLine("No buildable configurations found. Skipping solution {0} generation.", Name);
                return false;
            }

            _startupConfiguration = Configurations.SingleOrDefault(c => c.Name == startupconfigName) ?? Configurations.FirstOrDefault();


            foreach (var group in modules.GroupBy(m => ModuleGenerator.MakeKey(m), m => m))
            {
                var generator = CreateModuleGenerator(group.OrderBy(m=>m.Configuration.Name));
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

            try
            {
                // Set Dependencies for each generator
#if NANT_CONCURRENT
                Parallel.ForEach(ModuleGenerators.Values, g =>
#else
                foreach (var g in ModuleGenerators.Values)
#endif
                    {
                        var dependentgenerators = new Dictionary<string, ModuleGenerator>();

                        if (g.CollectAutoDependencies)
                        {
                            foreach (var depmodule in g.Modules.SelectMany(m => m.Dependents.Where(d => d.IsKindOf(DependencyTypes.Build | DependencyTypes.AutoBuildUse) && !(d.Dependent is Module_UseDependency)), (m, d) => d.Dependent).Distinct(Module.EqualityComparer))
                            {
                                var genkey = ModuleGenerator.MakeKey(depmodule);
                                if (!dependentgenerators.ContainsKey(genkey))
                                {
                                    ModuleGenerator depgenerator;
                                    if (ModuleGenerators.TryGetValue(genkey, out depgenerator))
                                    {
                                        dependentgenerators.Add(genkey, depgenerator);
                                    }
                                    else
                                    {
                                        Log.Warning.WriteLine("ModuleGenerator '{0}' not found", genkey);
                                    }
                                }
                            }
                        }
                        else
                        {
                            foreach (var depmodule in g.Modules.SelectMany(m => m.Dependents.Where(d => d.IsKindOf(DependencyTypes.Build) && !(d.Dependent is Module_UseDependency)), (m, d) => d.Dependent).Distinct(Module.EqualityComparer))
                            {
                                var genkey = ModuleGenerator.MakeKey(depmodule);
                                if (!dependentgenerators.ContainsKey(genkey))
                                {
                                    ModuleGenerator depgenerator;
                                    if (ModuleGenerators.TryGetValue(genkey, out depgenerator))
                                    {
                                        dependentgenerators.Add(genkey, depgenerator);
                                    }
                                    else
                                    {
                                        Log.Warning.WriteLine("ModuleGenerator '{0}' not found", genkey);
                                    }
                                }
                            }
                        }

                        g.Dependents.AddRange(dependentgenerators.Values.OrderBy(mg => mg.Key, StringComparer.InvariantCulture));
                    }
#if NANT_CONCURRENT
                );
#endif

#if NANT_CONCURRENT
                Parallel.ForEach(ModuleGenerators.Values, generator => generator.Generate());
#else
                foreach (var generator in ModuleGenerators.Values) generator.Generate();
#endif

            }
            catch (Exception ex)
            {
                ThreadUtil.RethrowThreadingException(String.Format("Error generating '{0}'", Name), ex);
            }
            return true;
        }

        private void InitPortableData(IEnumerable<IModule> topmodules)
        {
            if (IsPortable)
            {
                _portableData = new Portable();

                foreach (var project in topmodules.Select(m=>m.Package.Project).Distinct())
                {
                    _portableData.InitConfig(project);
                }
            }
        }

        private void SetConfigurationMapEntry(Configuration solutionconfig, IModule module)
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

            }
            else
            {
                Log.Warning.WriteLine("Module generator for module '{0}' does not exist", module.Key);
            }

        }
    }
}
