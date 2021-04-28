using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using EA.CPlusPlusTasks;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Build
{
    [TaskName("nant-build-module")]
    public class DoBuildModuleTask : Task, IBuildModuleTask, IModuleProcessor
    {
        public DoBuildModuleTask() : base("nant-build-module")
        {
        }

        public ProcessableModule Module
        {
            get { return _module; }
            set { _module = value; }
        }

        // Property Not used in this context
        public Tool Tool
        {
            get { return null; }
            set { }
        }

        public OptionSet BuildTypeOptionSet 
        {
            get { return null; }
            set { }
        }

        protected override void ExecuteTask()
        {
            Directory.CreateDirectory(Module.OutputDir.Path);
            Directory.CreateDirectory(Module.IntermediateDir.Path);

            Module.Apply(this);
        }
         
        public void Process(Module_Native module)
        {
            HashSet<string> tempOptionsets = new HashSet<string>();
            try
            {

                foreach (var tool in module.Tools)
                {
                    var buildtool = tool as BuildTool;
                    if (buildtool != null && buildtool.IsKindOf(Tool.TypeBuild | Tool.TypePreCompile) && !buildtool.IsKindOf(Tool.ExcludedFromBuild))
                    {
                        Project.ExecuteBuildTool(buildtool);
                    }
                }

                ExecuteSteps(module.BuildSteps, BuildStep.PreBuild);

                EA.CPlusPlusTasks.BuildTask build = new EA.CPlusPlusTasks.BuildTask();
                build.Project = Project;

                build.OptionSetName = "__private_build.buildtype.nativenant";

                tempOptionsets.Add(build.OptionSetName);

                module.ToOptionSet(Project, build.OptionSetName);

                Project.Properties["build.outputdir.override"] = module.OutputDir.Path;

                //IMTODO: specify interrmediate and output dir!
                build.BuildDirectory = module.IntermediateDir.Normalize().Path;

                //IMTODO: Append causes another expansion of the same file set. Assign fileset instead!
                //-- CC ---
                if (module.Cc != null)
                {
                    build.IncludeDirectories.Value = module.Cc.IncludeDirs.ToNewLineString();
                    build.Defines.Value = module.Cc.Defines.ToNewLineString();
                    build.Sources.IncludeWithBaseDir(module.Cc.SourceFiles);
                    build.Dependencies.IncludeWithBaseDir(module.Cc.InputDependencies);
                    build.UsingDirectories.Value = module.Cc.UsingDirs.ToNewLineString();
                    // Set custom options for each source file:
                    foreach (var fileitem in build.Sources.FileItems)
                    {
                        if (!String.IsNullOrWhiteSpace(fileitem.OptionSetName))
                        {
                            var optionsetname = "___private.cc.build." + fileitem.OptionSetName;

                            if (!tempOptionsets.Contains(optionsetname))
                            {
                                if ((fileitem.GetTool() as CcCompiler).ToOptionSet(module, Project, optionsetname))
                                {
                                    tempOptionsets.Add(optionsetname);
                                }
                            }

                            fileitem.OptionSetName = optionsetname;
                        }
                    }
                }
                //-- AS ---
                if (module.Asm != null)
                {
                    build.AsmIncludePath.Value = module.Asm.IncludeDirs.ToNewLineString();
                    build.AsmDefines.Value = module.Asm.Defines.ToNewLineString();
                    build.AsmSources.IncludeWithBaseDir(module.Asm.SourceFiles);
                    build.Dependencies.IncludeWithBaseDir(module.Asm.InputDependencies);

                    // Set custom options for each source file:
                    foreach (var fileitem in build.AsmSources.FileItems)
                    {
                        if (!String.IsNullOrWhiteSpace(fileitem.OptionSetName))
                        {
                            var optionsetname = "___private.as.build." + fileitem.OptionSetName;

                            if (!tempOptionsets.Contains(optionsetname))
                            {
                                if ((fileitem.GetTool() as CcCompiler).ToOptionSet(module, Project, optionsetname))
                                {
                                    tempOptionsets.Add(optionsetname);
                                }
                            }
                            fileitem.OptionSetName = optionsetname;
                        }
                    }
                }

                if (module.Link != null)
                {
                    if (module.Link.ImportLibFullPath != null && !String.IsNullOrWhiteSpace(module.Link.ImportLibFullPath.Path))
                    {
                        string importlibdir = Path.GetDirectoryName(module.Link.ImportLibFullPath.Path);
                        if (!String.IsNullOrWhiteSpace(importlibdir))
                        {
                            Directory.CreateDirectory(importlibdir);
                        }
                    }

                    build.BuildName = module.Link.OutputName;
                    build.Objects.IncludeWithBaseDir(module.Link.ObjectFiles);
                    build.Libraries.IncludeWithBaseDir(module.Link.Libraries);
                    build.Dependencies.IncludeWithBaseDir(module.Link.InputDependencies);
                    build.PrimaryOutputExtension = module.Link.OutputExtension;
                }
                else if (module.Lib != null)
                {
                    build.BuildName = module.Lib.OutputName;
                    build.Dependencies.IncludeWithBaseDir(module.Lib.InputDependencies);
                    build.Objects.IncludeWithBaseDir(module.Lib.ObjectFiles);
                    build.PrimaryOutputExtension = module.Lib.OutputExtension;
                }

                if (module.Cc != null)
                {
                    // CopyLocal slim are set through optionset
                    ExecuteCopyLocal(module.Cc.Assemblies, CopyLocalType.True == module.CopyLocal, module);
                    ExecuteCopyLocal(module.Cc.ComAssemblies, CopyLocalType.True == module.CopyLocal, module);
                }

                // Copy local Dlls
                ExecuteCopyLocalDependents(module, module.CopyLocal, (item) => (item != null ? item.DynamicLibraries : null));

                build.Execute();

                ExecuteBuildCopy(module);

                ExecuteSteps(module.BuildSteps, BuildStep.PostBuild);

            }
            finally
            {
                foreach(var tempset in tempOptionsets)
                {
                    Project.NamedOptionSets.Remove(tempset);
                }
            }
        }

        public void Process(Module_DotNet module)
        {
            ExecuteSteps(module.BuildSteps, BuildStep.PreBuild);

            NAnt.DotNetTasks.CompilerBase compiler = null;

            if(module.Compiler is CscCompiler)
            {
                NAnt.DotNetTasks.CscTask csc = new  NAnt.DotNetTasks.CscTask();
                csc.Project = Project;
                csc.useDebugProperty = false;
                compiler = csc;

                csc.Win32Icon = (module.Compiler as CscCompiler).Win32icon.Path;
                csc.ArgSet.Arguments.AddRange(module.Compiler.Options);

                if (module.Compiler.GenerateDocs)
                {
                    csc.Doc = module.Compiler.DocFile.Path;
                }
                csc.ArgSet.Arguments.Add(module.Compiler.AdditionalArguments);
            }
            else if (module.Compiler is FscCompiler)
            {
                NAnt.DotNetTasks.FscTask fsc = new NAnt.DotNetTasks.FscTask();
                fsc.Project = Project;
                fsc.useDebugProperty = false;
                compiler = fsc;

                fsc.ArgSet.Arguments.AddRange(module.Compiler.Options);

                if (module.Compiler.GenerateDocs)
                {
                    fsc.Doc = module.Compiler.DocFile.Path;
                }
                fsc.ArgSet.Arguments.Add(module.Compiler.AdditionalArguments);
            }
            else
            {
                Error.Throw(Project, Location, "dobuildmodule", "Unknown compiler in [{0}] {1}.", module.Package.Name, module.Name);
            }
            
            compiler.Compiler = module.Compiler.Executable.Path;
            compiler.Resgen = Project.ExpandProperties(Project.Properties["build.resgen.program"]);

            compiler.OutputTarget = module.Compiler.Target.ToString().ToLowerInvariant();
            
            compiler.Output = Path.Combine(module.OutputDir.Path, module.Compiler.OutputName + module.Compiler.OutputExtension);
            compiler.Define = module.Compiler.Defines.ToString(" ");
            

            compiler.References.IncludeWithBaseDir(module.Compiler.DependentAssemblies);
            compiler.References.IncludeWithBaseDir(module.Compiler.Assemblies);

            compiler.Resources= module.Compiler.Resources;

            compiler.Modules = module.Compiler.Modules;
            compiler.Sources = module.Compiler.SourceFiles;

            if (module.Compiler.GenerateDocs)
            {
                Directory.CreateDirectory(Path.GetDirectoryName(module.Compiler.DocFile.Path));

            }

            ExecuteCopyLocal(module.Compiler.Assemblies, module.CopyLocal== CopyLocalType.True, module);
            ExecuteCopyLocal(module.Compiler.DependentAssemblies, (module.CopyLocal == CopyLocalType.True) || (module.CopyLocal == CopyLocalType.Slim), module);

            compiler.Execute();

            ExecuteSteps(module.BuildSteps, BuildStep.PostBuild);
        }

        public void Process(Module_Utility module)
        {
            ExecuteSteps(module.BuildSteps, BuildStep.PreBuild);

            ExecuteSteps(module.BuildSteps, BuildStep.PreLink);

            ExecuteSteps(module.BuildSteps, BuildStep.Build);

            ExecuteSteps(module.BuildSteps, BuildStep.PostBuild);

            ExecuteCopyLocalDependents(module, CopyLocalType.Undefined, (item) => (item != null ? item.DynamicLibraries : null));
            ExecuteCopyLocalDependents(module, CopyLocalType.Undefined, (item) => (item != null ? item.Assemblies : null));
        }

        public void Process(Module_MakeStyle module)
        {
            ExecuteSteps(module.BuildSteps, BuildStep.Build);

            ExecuteCopyLocalDependents(module, CopyLocalType.Undefined, (item) => (item != null ? item.DynamicLibraries : null));
            ExecuteCopyLocalDependents(module, CopyLocalType.Undefined, (item) => (item != null ? item.Assemblies : null));
        }

        public void Process(Module_Python module)
        {
            throw new BuildException("NAnt build for Module type 'PythonModule' is not yet supported");
        }

        public void Process(Module_ExternalVisualStudio module)
        {
            throw new BuildException("NAnt build for Module type 'ExternalVisualStudio' is not yet supported");
        }

        public void Process(Module_UseDependency module)
        {
            Error.Throw(Project, "ModuleProcessor", "Internal error: trying to process use dependency module");
        }

        protected void ExecuteTools(Module module, uint type)
        {
            foreach (Tool tool in module.Tools)
            {
                if (tool.IsKindOf(type))
                {
                    Project.ExecuteBuildTool(tool);
                }
            }
        }

        private void ExecuteSteps(IEnumerable<BuildStep> steps, uint mask)
        {
            if (steps != null)
            {
                foreach (BuildStep step in steps)
                {
                    if (step.IsKindOf(mask))
                    {
                        Project.ExecuteBuildStep(step);
                    }
                }
            }
        }

        private void ExecuteCopyLocalDependents(Module module, CopyLocalType copylocal, Func<IPublicData, FileSet> GetFileset)
        {
            foreach (var pair in module.Dependents.FlattenParentChildPair())
            {
                bool copy = (copylocal == CopyLocalType.True || copylocal == CopyLocalType.Slim || pair.Dependency.Dependent.IsKindOf(DependencyTypes.CopyLocal));

                var publicData = pair.Dependency.Dependent.Public(pair.ParentModule);

                var fileset = GetFileset(publicData);

                if (fileset != null)
                {
                    foreach (var item in fileset.FileItems)
                    {
                        if (item.AsIs == false)
                        {
                            // Check if assembly has optionset with copylocal flag attached.
                            var itemCopyLocal = item.GetCopyLocal(module);

                            if ((copy || itemCopyLocal == CopyLocalType.True) && itemCopyLocal != CopyLocalType.False)
                            {
                                var file = item.Path;

                                CopyTask task = new CopyTask();
                                task.Project = Project;
                                task.SourceFile = file.Path;
                                task.ToFile = Path.Combine(module.OutputDir.Path, Path.GetFileName(file.Path));

                                Log.Info.WriteLine(LogPrefix + "{0}: copylocal '{1}' to '{2}'", module.ModuleIdentityString(), task.SourceFile, task.ToFile);

                                task.Execute();
                            }
                        }
                    }
                }

                // Copy Dependent build output:
                if (copy && pair.Dependency.Dependent.IsKindOf(EA.FrameworkTasks.Model.Module.DynamicLibrary|EA.FrameworkTasks.Model.Module.Program|EA.FrameworkTasks.Model.Module.DotNet))
                {
                    var assemblyordll = pair.Dependency.Dependent.PrimaryOutput();
                    if (assemblyordll != null)
                    {
                        CopyTask task = new CopyTask();
                        task.Project = Project;
                        task.SourceFile = assemblyordll;
                        task.ToFile = Path.Combine(module.OutputDir.Path, Path.GetFileName(assemblyordll));

                        Log.Info.WriteLine(LogPrefix + "{0}: copylocal '{1}' to '{2}'", module.ModuleIdentityString(), task.SourceFile, task.ToFile);

                        task.Execute();
                    }
                }

            }
        }

        private void ExecuteCopyLocal(FileSet files, bool copylocal, IModule module)
        {
            if (files != null)
            {
                foreach (var item in files.FileItems)
                {
                    if (item.AsIs == false)
                    {
                        // Check if assembly has optionset with copylocal flag attached.
                        var itemCopyLocal = item.GetCopyLocal(module);

                        if ((copylocal || itemCopyLocal == CopyLocalType.True) && itemCopyLocal != CopyLocalType.False)
                        {
                            var file = item.Path;

                            CopyTask task = new CopyTask();
                            task.Project = Project;
                            task.SourceFile = file.Path;
                            task.ToFile = Path.Combine(module.OutputDir.Path, Path.GetFileName(file.Path));

                            Log.Info.WriteLine(LogPrefix + "{0}: copylocal '{1}' to '{2}'", module.ModuleIdentityString(), task.SourceFile, task.ToFile);

                            task.Execute();

                            // Copy pdb:
                            var pdbFile = Path.ChangeExtension(file.Path, ".pdb");
                            if (File.Exists(pdbFile))
                            {
                                task = new CopyTask();
                                task.Project = Project;
                                task.SourceFile = pdbFile;
                                task.ToFile = Path.Combine(module.OutputDir.Path, Path.GetFileName(pdbFile));

                                Log.Info.WriteLine(LogPrefix + "{0}: copylocal '{1}' to '{2}'", module.ModuleIdentityString(), task.SourceFile, task.ToFile);

                                task.Execute();
                            }
                            // Copy map:
                            var mapbFile = Path.ChangeExtension(file.Path, ".map");
                            if (File.Exists(mapbFile))
                            {
                                task = new CopyTask();
                                task.Project = Project;
                                task.SourceFile = mapbFile;
                                task.ToFile = Path.Combine(module.OutputDir.Path, Path.GetFileName(mapbFile));

                                Log.Info.WriteLine(LogPrefix + "{0}: copylocal '{1}' to '{2}'", module.ModuleIdentityString(), task.SourceFile, task.ToFile);

                                task.Execute();
                            }
                        }
                    }
                }
            }
        }


        // This is implementation of a legacy build-copy target.
        // THe default eaconfig implementation of this target is not used anymore,
        // But there may be still config specific or optionset specific implementations.
        private void ExecuteBuildCopy(Module module)
        {
            // Just for backward compatibility 
            var targetname = "build-copy-${build.buildtype}";
            if (Project.ExecuteTargetIfExists(targetname, force: true))
            {
                Log.Info.WriteLine(LogPrefix + "{0}: execute '{1}' target", module.ModuleIdentityString(), targetname);
                return;
            }

            // Execute PlatformSpecific target
            if (!module.IsKindOf(ProcessableModule.DotNet))
            {
                var targettype = "library";

                if (module.IsKindOf(ProcessableModule.Program))
                {
                    targettype = "program";
                }
                else if (module.IsKindOf(ProcessableModule.DynamicLibrary))
                {
                    targettype = "dynamiclibrary";
                }

                targetname = String.Format("build-copy-{0}-{1}", targettype, Project.Properties["config-platform-load-name"] ?? Project.Properties["config-platform"]);

                if (Project.ExecuteTargetIfExists(targetname, force: true))
                {
                    Log.Info.WriteLine(LogPrefix + "{0}: execute '{1}' target", module.ModuleIdentityString(), targetname);
                }
            }

        }

        private ProcessableModule _module;
    }
}
