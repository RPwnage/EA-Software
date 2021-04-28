using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Backends;

namespace EA.Eaconfig.Backends
{

    public abstract class ModuleGenerator
    {
        public static readonly IModuleGeneratorEqualityComparer EqualityComparer = new IModuleGeneratorEqualityComparer();

        public class SourceControlData
        {
            public string SccLocalPath;
            public string SccAuxPath;
            public string SccProjectName;
            public string SccProvider;
        }

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

            AllConfigurations = Modules.Select(mod => mod.Configuration).Distinct();
            ConfigurationMap = new Dictionary<Configuration, Configuration>();

            if(!BuildGenerator.ModuleGenerators.ContainsKey(Key))
            {
                BuildGenerator.ModuleGenerators.Add(Key, this);
            }
            else
            {
                Log.Warning.WriteLine("Duplicate generator entry {0}", Key);
            }

            IncludeBuildScripts = false;

            _projectFileNameWithoutExtension = Name;
            _outputDir = GetDefaultOutputDir();
            {
                var templates = BuildGenerator.GeneratorTemplates;
                if(templates != null)
                {
                    var outputdir = templates.ProjectDirTemplate.ReplaceTemplateParameters("gen.projectdir.template", new string[,] 
                    { 
                                {"%outputdir%",  _outputDir.Path},
                                {"%package.builddir%",  m.Package.PackageBuildDir.Path},
                                {"%package.configbuilddir%", m.Package.PackageConfigBuildDir.Path}
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
                        _projectFileNameWithoutExtension = filename;
                    }
                    else if (BuildGenerator.GroupNumber > 0)
                    {
                        _projectFileNameWithoutExtension = String.Format("{0}_{1}", _projectFileNameWithoutExtension, BuildGenerator.GroupNumber);
                    }
                }
            }

            SourceControl = CreateSourceControlData();
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

        protected string ProjectFileNameWithoutExtension
        {
            get { return _projectFileNameWithoutExtension; }
        }


        public readonly SourceControlData SourceControl;

        public bool IncludeBuildScripts;

        /// <summary>
        /// module generator output directory relative to the to solution build root:
        /// </summary>
        internal virtual string RelativeDir
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

        public readonly IEnumerable<Configuration> AllConfigurations;

        internal readonly IDictionary<Configuration, Configuration> ConfigurationMap;

        public readonly IEnumerable<IModule> Modules;

        public readonly List<ModuleGenerator> Dependents = new List<ModuleGenerator>();

        protected readonly Generator BuildGenerator;

        #region Abstract Methods

        public virtual void Generate()
        {
            WriteProject();
        }


        public abstract string ProjectFileName { get; }

        public abstract void WriteProject();



        protected abstract SourceControlData CreateSourceControlData();

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


        private string _name;
        private PathString _outputDir;
        private string _projectFileNameWithoutExtension;
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
