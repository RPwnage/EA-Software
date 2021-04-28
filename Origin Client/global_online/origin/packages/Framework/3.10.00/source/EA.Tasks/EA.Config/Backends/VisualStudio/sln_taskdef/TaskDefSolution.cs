using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using PackageCore = NAnt.Core.PackageCore;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Backends;



using EA.Eaconfig.Backends.VisualStudio;

namespace EA.Eaconfig.Backends.VisualStudio.sln_taskdef
{
    internal class TaskDefSolution : VSSolutionBase
    {

        public TaskDefSolution(Log log, string name, PathString outputdir)
            : base(log, name, outputdir)
        {
             Packages = new Dictionary<string, Package>();
        }


        internal new IEnumerable<IModule> GetTopModules(Project project)
        {
            var configurations = new List<Configuration>() { new Configuration("Debug", "pc-vc", "", "vc", "pc", "debug"), new Configuration("Release", "pc-vc", "", "vc", "pc", "release") };

            return project.TaskDefModules().SelectMany(entry =>
                {
                    var modules = new List<IModule>();
                    foreach (var config in configurations)
                    {
                        modules.Add(CreateModule(entry.Value, config, project));
                    }
                    return modules;
                });
        }


        protected override ModuleGenerator CreateModuleGenerator(IEnumerable<IModule> modules)
        {
            return new TaskDefProject(this, modules);
        }


        #region Virtual Overrides for Writing Solution File

        protected override void WriteHeader(IMakeWriter writer)
        {
            writer.WriteLine("Microsoft Visual Studio Solution File, Format Version 11.00");
            writer.WriteLine("# Visual Studio 2010");
        }

        protected override string TeamTestSchemaVersion
        {
            get { return "2010"; }
        }

        #endregion Virtual Overrides for Writing Solution File

        private Module_DotNet CreateModule(TaskDefModule module_data, Configuration config, Project project)
        {
            var name = Path.GetFileNameWithoutExtension(module_data.AssemblyFileName);
            var buildtype = new BuildType("Library", "CSharp", "", isEasharp: false, isMakestype: false);

            // Find package based on the location of the script file

            var package = GetPackage(project, config, module_data);

            var module = new Module_DotNet(name, BuildGroups.runtime.ToString() + name, project.Properties["eaconfig." + BuildGroups.runtime + ".sourcedir"], config, BuildGroups.runtime, buildtype, package);
            module.OutputDir = PathString.MakeNormalized(Path.GetDirectoryName(module_data.AssemblyFileName));
            module.Compiler = new CscCompiler(new PathString("csc.exe"), DotNetCompiler.Targets.Library);
            module.Compiler.OutputName = name;
            module.Compiler.OutputExtension = Path.GetExtension(module_data.AssemblyFileName);
            module.Compiler.SourceFiles.IncludeWithBaseDir(module_data.Sources);
            foreach (var reference in module_data.References)
            {
                module.Compiler.Assemblies.Include(reference);
            }
            module.ExcludedFromBuild_Files.Include(module_data.ScriptFile);
            // ---- options -----
            module.Compiler.Options.Add("/nowarn:1607");  // Supress Assembly generation - Referenced assembly 'System.Data.dll' targets a different processor
            //module.Compiler.Defines.Add("EA_DOTNET2");
            module.Compiler.Options.Add("/platform:anycpu");

            module.Compiler.Defines.Add("FRAMEWORK3");

            return module;
        }

        private Package GetPackage(Project project, Configuration config, TaskDefModule module_data)
        {
            var package_name = module_data.Package;
            var package_version = String.Empty;
            var package_dir = new PathString(String.Empty);

            var release = PackageCore.PackageMap.Instance.Releases.FirstOrDefault(r=> module_data.ScriptFile.StartsWith(r.Path))
                            ?? PackageCore.PackageMap.Instance.GetMasterRelease(package_name, project);

            if(release != null)
            {
                package_name = release.Name;
                package_version = release.Version;
                package_dir = PathString.MakeNormalized(release.Path);
            }

            var key = Package.MakeKey(package_name, package_version, config.Name);

            Package package;
            if(!Packages.TryGetValue(key, out package))
            {
                package = new Package(package_name, package_version, package_dir, config.Name, FrameworkVersions.Framework2);
                package.Project = project;

                Packages.Add(key, package);
            }

            return package;

        }

        protected override void PopulateSolutionFolders(Project project)
        {
            foreach(var generator in ModuleGenerators.Values.Cast<TaskDefProject>())
            {
                SolutionFolders.SolutionFolder folder;

                string folder_name = generator.Package.Name + "-" + generator.Package.Version;

                if(!SolutionFolders.Folders.TryGetValue(folder_name, out folder))
                {
                    folder = new SolutionFolders.SolutionFolder(generator.Package.Name, new PathString(folder_name));
                    SolutionFolders.Folders.Add(folder_name, folder);
                }
                folder.FolderProjects.Add(generator.ProjectGuid, generator);
            }
        }


        internal override string GetTargetPlatform(Configuration configuration)
        {
            return "Any CPU";
        }

        private readonly Dictionary<string, Package> Packages;
    }
}
