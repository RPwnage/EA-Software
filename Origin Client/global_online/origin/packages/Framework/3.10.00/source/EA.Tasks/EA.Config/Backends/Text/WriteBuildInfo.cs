using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Writers;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
 

namespace EA.Eaconfig.Backends.Text
{
    [TaskName("WriteBuildInfo")]
    public class WriteBuildInfo : Task
    {
        public WriteBuildInfo() : base("WriteBuildInfo") { }

        [TaskAttribute("outputdir", Required = false)]
        public string OutputDir
        {
            get { return !String.IsNullOrEmpty(_outputdir) ? _outputdir : Path.Combine(Project.GetFullPath(Project.ExpandProperties(Properties[Project.NANT_PROPERTY_PROJECT_BUILDROOT])), "buildinfo", "txt"); }
            set { _outputdir = value; }
        }

        protected override void ExecuteTask()
        {
            var timer = new Chrono();

            foreach(var topModulesByConfig in Project.BuildGraph().TopModules.GroupBy(m=> m.Configuration.Name))
            {
                WriteOneConfig(topModulesByConfig, PathString.MakeCombinedAndNormalized(OutputDir, topModulesByConfig.Key));
            }

            Log.Status.WriteLine(LogPrefix + "  {0} buildinfo directory : {1}", timer.ToString(), OutputDir.Quote());
        }

        public void WriteOneConfig(IEnumerable<IModule> topmodules, PathString dir)
        {
            var allmodules = topmodules.Union(topmodules.SelectMany(m => m.Dependents.FlattenAll(DependencyTypes.Build), (m, d) => d.Dependent).Where(m => !(m is Module_UseDependency)));

            WriteGeneralInfo(topmodules, allmodules, dir);

            foreach (var module in allmodules)
            {
                WriteModule(module as Module, dir);
            }
        }

        private void WriteGeneralInfo(IEnumerable<IModule> topmodules, IEnumerable<IModule> allmodules, PathString dir)
        {
        }

        private void WriteModule(Module module, PathString dir)
        {

            using (var writer = new MakeWriter(writeBOM: false))
            {
                var processableModule = module as ProcessableModule;
                writer.FileName = Path.Combine(dir.Path, GetModuleFileName(module));

                var buildOptions = GetModuleOptions(module as ProcessableModule);

                writer.WriteLine("{0}-{1}  '{2}.{3}'  {4}{5}", module.Package.Name, module.Package.Version, module.BuildGroup, module.Name, module.Configuration.Name, String.IsNullOrEmpty(module.Configuration.SubSystem) ? String.Empty : " [" + module.Configuration.SubSystem + "]");
                writer.WriteLine("TYPE: {0}", ModuleTypeToString(module));
                WritePath(writer, "OutputDir", module.OutputDir);
                WritePath(writer, "IntermediateDir", module.IntermediateDir);
                WritePath(writer, "ScriptFile", module.ScriptFile);
                if (processableModule != null)
                {
                    writer.WriteLine("buildtype={0}  basebuildtype={1}", processableModule.BuildType.Name, processableModule.BuildType.BaseName);
                }
                writer.WriteLine();
                WriteProcessSteps(writer, module, buildOptions, "PREPROCESS");
                writer.WriteLine();
                WriteProcessSteps(writer, module, buildOptions, "POSTPROCESS");
                writer.WriteLine();
                writer.WriteLine();
                writer.WriteLine("DEPENDENCIES:");
                foreach (var dep in module.Dependents)
                {
                    WriteDependency(writer, dep);
                }
                writer.WriteLine();
                writer.WriteLine("TOOLS:");
                foreach (var tool in module.Tools)
                {
                    WriteTool(writer, tool);
                }

                writer.WriteLine();
                writer.WriteLine("BUILD STEPS");
                WriteBuildSteps(writer, module.BuildSteps, BuildStep.PreBuild);
                WriteBuildSteps(writer, module.BuildSteps, BuildStep.PreLink);
                WriteBuildSteps(writer, module.BuildSteps, BuildStep.PostBuild);
                // These are set if module is makestyle:
                WriteBuildSteps(writer, module.BuildSteps, BuildStep.Build);
                WriteBuildSteps(writer, module.BuildSteps, BuildStep.Clean);
                WriteBuildSteps(writer, module.BuildSteps, BuildStep.ReBuild);

                writer.WriteLine();
                writer.WriteLine("MODULE FILES:");
                WriteFiles(writer, "ExcludedFromBuild_Sources", module.ExcludedFromBuild_Sources);
                WriteFiles(writer, "ExcludedFromBuild_Files", module.ExcludedFromBuild_Files);
                WriteFiles(writer, "Assets", module.Assets);
            }
        }


        private void WriteProcessSteps(IMakeWriter writer, Module module, OptionSet buildOptions, string type)
        {
            writer.WriteLine();

            writer.WriteLine("{0} STEPS:", type);

            writer.WriteTabLine("from                   build options: {0}", buildOptions.Options[type.ToLowerInvariant()].LinesToArray().ToString("; "));
            writer.WriteTabLine("from property 'eaconfig.postprocess': {0}", module.Package.Project.Properties["eaconfig."+type.ToLowerInvariant()].LinesToArray().ToString("; "));
            writer.WriteTabLine("from property '" + module.GroupName + ".postprocess': {0}", module.Package.Project.Properties["eaconfig." + type.ToLowerInvariant()].LinesToArray().ToString("; "));
        }

        private void WriteTool(IMakeWriter writer, Tool tool, bool writefiles = true, FileItem file=null)
        {
            if (tool is BuildTool)
            {
                WriteTool(writer, tool as BuildTool, writefiles);
            }
            else if (tool is CcCompiler)
            {
                WriteTool(writer, tool as CcCompiler, writefiles, file);
            }
            else if (tool is AsmCompiler)
            {
                WriteTool(writer, tool as AsmCompiler, writefiles, file);
            }
            else if (tool is Linker)
            {
                WriteTool(writer, tool as Linker, writefiles);
            }
            else if (tool is Librarian)
            {
                WriteTool(writer, tool as Librarian, writefiles);
            }
            else if (tool is DotNetCompiler)
            {
                WriteTool(writer, tool as DotNetCompiler, writefiles);
            }
            else if (tool is PostLink)
            {
                WriteTool(writer, tool as PostLink, writefiles);
            }
            else if (tool is AppPackageTool)
            {
                WriteTool(writer, tool as AppPackageTool, writefiles);
            }
            else
            {
                WriteToolGeneric(writer, tool, writefiles);
            }

            writer.WriteLine();
        }

        private void WriteTool(IMakeWriter writer, BuildTool tool, bool writefiles = true)
        {
            writer.WriteLine("  TOOL Name='{0}' {1}", tool.ToolName, ToolTypeToString(tool));
            writer.WriteTabLine("Description='{0}'", tool.Description);
            writer.WriteTabLine("ExcludedFromBuild='{0}'", tool.IsKindOf(BuildTool.ExcludedFromBuild));
            if (tool.IsKindOf(BuildTool.Program))
            {
                WritePath(writer, "Executable", tool.Executable);
                WriteList(writer, "Options", tool.Options);
            }
            else
            {
                writer.WriteTabLine("Script='{0}'", tool.Script);
            }
            WritePath(writer, "OutputDir", tool.OutputDir);
            WritePath(writer, "IntermediateDir", tool.IntermediateDir);
            WriteFiles(writer, "Files", tool.Files);
            WriteFiles(writer, "InputDependencies", tool.InputDependencies);
            WriteFiles(writer, "OutputDependencies", tool.OutputDependencies);
            WriteList(writer, "CreateDirectories", tool.CreateDirectories);
            WritePath(writer, "WorkingDir", tool.WorkingDir);
            if (tool.Env != null)
            {
                writer.WriteTabLine("Environment:");
                foreach (var e in tool.Env)
                {
                    writer.WriteTab();
                    writer.WriteTabLine("{0}={1}", e.Key, e.Value);
                }
            }
        }

        private void WriteTool(IMakeWriter writer, CcCompiler cc, bool writefiles = true, FileItem file= null)
        {

            writer.WriteLine("  COMPILER Name='{0}' {1}", cc.ToolName, ToolTypeToString(cc));

            string optionset = file==null? null : file.OptionSetName;

            if(!String.IsNullOrEmpty(optionset))
            {
                writer.WriteTabLine("Build OptionSet='{0}'", optionset);
            }
            writer.WriteTabLine("ExcludedFromBuild='{0}'", cc.IsKindOf(BuildTool.ExcludedFromBuild));
            WritePath(writer, "Executable", cc.Executable);
            writer.WriteTabLine("PrecompiledHeaders='{0}'", cc.PrecompiledHeaders.ToString());
            WriteList(writer, "Options", cc.Options);
            WriteList(writer, "Defines", cc.Defines);
            WriteList(writer, "CompilerInternalDefines", cc.CompilerInternalDefines);
            WriteList(writer, "IncludeDirs", cc.IncludeDirs);
            WriteList(writer, "UsingDirs", cc.UsingDirs);
            
            WriteFiles(writer, "Assemblies", cc.Assemblies, nonEmptyOnly: true);
            WriteFiles(writer, "ComAssemblies", cc.ComAssemblies, nonEmptyOnly: true);

            if (writefiles)
            {
                writer.WriteTabLine("SourceFiles:");
                foreach (var fi in cc.SourceFiles.FileItems)
                {
                    writer.WriteTab();
                    writer.WriteTabLine("src: {0}", fi.Path.Path);
                    FileData filedata = fi.Data as FileData;
                    if (filedata != null)
                    {
                        if (filedata.ObjectFile != null)
                        {
                            writer.WriteTab();
                            writer.WriteTabLine("obj: {0}", filedata.ObjectFile.Path);
                        }
                        if (filedata.Tool != null)
                        {
                            WriteTool(writer, filedata.Tool, writefiles: false, file:fi);
                        }
                    }
                }
            }
        }

        private void WriteTool(IMakeWriter writer, AsmCompiler asm, bool writefiles = true, FileItem file=null)
        {
            writer.WriteLine("  COMPILER Name='{0}' {1}", asm.ToolName, ToolTypeToString(asm));
            string optionset = file == null ? null : file.OptionSetName;

            if (!String.IsNullOrEmpty(optionset))
            {
                writer.WriteTabLine("Build OptionSet='{0}'", optionset);
            }

            writer.WriteTabLine("ExcludedFromBuild='{0}'", asm.IsKindOf(BuildTool.ExcludedFromBuild));
            WritePath(writer, "Executable", asm.Executable);
            
            WriteList(writer, "Options", asm.Options);
            WriteList(writer, "Defines", asm.Defines);
            WriteList(writer, "IncludeDirs", asm.IncludeDirs);

            if (writefiles)
            {
                writer.WriteTabLine("SourceFiles:");
                foreach (var fi in asm.SourceFiles.FileItems)
                {
                    writer.WriteTab();
                    writer.WriteTabLine("src: {0}", fi.Path.Path);
                    FileData filedata = fi.Data as FileData;
                    if (filedata != null)
                    {
                        if (filedata.ObjectFile != null)
                        {
                            writer.WriteTab();
                            writer.WriteTabLine("obj: {0}", filedata.ObjectFile.Path);
                        }
                        if (filedata.Tool != null)
                        {
                            WriteTool(writer, filedata.Tool, writefiles: false, file:fi);
                        }
                    }
                }
            }
        }

        private void WriteTool(IMakeWriter writer, Linker link, bool writefiles = true)
        {
            writer.WriteLine("  Linker Name='{0}' {1}", link.ToolName, ToolTypeToString(link));
            writer.WriteTabLine("ExcludedFromBuild='{0}'", link.IsKindOf(BuildTool.ExcludedFromBuild));
            WritePath(writer, "Executable", link.Executable);

            WritePath(writer, "OutputDir", link.LinkOutputDir);
            writer.WriteTabLine("OutputName='{0}'", link.OutputName);
            writer.WriteTabLine("OutputExtension='{0}'", link.OutputExtension);
            WritePath(writer, "ImportLibrary", link.ImportLibFullPath, nonEmptyOnly : true);
            writer.WriteTabLine("UseLibraryDependencyInputs='{0}'", link.UseLibraryDependencyInputs);
            
            WriteList(writer, "Options", link.Options);

            WriteList(writer, "LibraryDirs", link.LibraryDirs);
            WriteFiles(writer, "Libraries", link.Libraries);
            WriteFiles(writer, "Assemblies", link.DynamicLibraries);
            WriteFiles(writer, "ObjectFiles (additional to compiler output obj files, aka libs)", link.ObjectFiles);
        }

        private void WriteTool(IMakeWriter writer, Librarian lib, bool writefiles = true)
        {
            writer.WriteLine("  Librarian Name='{0}' {1}", lib.ToolName, ToolTypeToString(lib));
            writer.WriteTabLine("ExcludedFromBuild='{0}'", lib.IsKindOf(BuildTool.ExcludedFromBuild));
            WritePath(writer, "Executable", lib.Executable);

            writer.WriteTabLine("OutputName='{0}'", lib.OutputName);
            writer.WriteTabLine("OutputExtension='{0}'", lib.OutputExtension);

            WriteList(writer, "Options", lib.Options);

            WriteFiles(writer, "ObjectFiles (additional to compiler output obj files)", lib.ObjectFiles);

        }

        private void WriteTool(IMakeWriter writer, DotNetCompiler dotnet, bool writefiles = true)
        {
            writer.WriteLine("  COMPILER Name='{0}' {1}", dotnet.ToolName, ToolTypeToString(dotnet));
            writer.WriteTabLine("ExcludedFromBuild='{0}'", dotnet.IsKindOf(BuildTool.ExcludedFromBuild));
            WritePath(writer, "Executable", dotnet.Executable);

            writer.WriteTabLine("Target='{0}'", dotnet.Target);
            WriteList(writer, "Options", dotnet.Options);
            WriteList(writer, "Defines", dotnet.Defines);
            WritePath(writer, "DocFile", dotnet.DocFile, nonEmptyOnly:true);
            writer.WriteTabLine("GenerateDocs='{0}'", dotnet.GenerateDocs);

            if (writefiles)
            {
                WriteFiles(writer, "SourceFiles", dotnet.SourceFiles);
                WriteFiles(writer, "Assemblies", dotnet.Assemblies);
                WriteFiles(writer, "DependentAssemblies", dotnet.DependentAssemblies);
                WriteFiles(writer, "Resources", dotnet.Resources);
                WriteFiles(writer, "NonEmbeddedResources", dotnet.NonEmbeddedResources);
                WriteFiles(writer, "ContentFiles", dotnet.ContentFiles);
                WriteFiles(writer, "Modules", dotnet.Modules);
                WriteFiles(writer, "ComAssemblies", dotnet.ComAssemblies);
            }
        }

        private void WriteTool(IMakeWriter writer, AppPackageTool apppackage, bool writefiles = true)
        {
            writer.WriteLine("  AppPackageTool : WRITETOOL NOT IMPLEMENTED");
        }

        private void WriteToolGeneric(IMakeWriter writer, Tool tool, bool writefiles = true)
        {
            writer.WriteLine("  TOOL Name='{0}' {1}", tool.ToolName, ToolTypeToString(tool));
            writer.WriteTabLine("Description='{0}'", tool.Description);
            writer.WriteTabLine("ExcludedFromBuild='{0}'", tool.IsKindOf(BuildTool.ExcludedFromBuild));
            if (tool.IsKindOf(BuildTool.Program))
            {
                WritePath(writer, "Executable", tool.Executable);
                WriteList(writer, "Options", tool.Options);
            }
            else
            {
                writer.WriteTabLine("Script='{0}'", tool.Script);
            }

            WriteFiles(writer, "InputDependencies", tool.InputDependencies);
            WriteFiles(writer, "OutputDependencies", tool.OutputDependencies);
            WriteList(writer, "CreateDirectories", tool.CreateDirectories);
            WritePath(writer, "WorkingDir", tool.WorkingDir);

            if (tool.Env != null)
            {
                writer.WriteTabLine("Environment:");
                foreach (var e in tool.Env)
                {
                    writer.WriteTab();
                    writer.WriteTabLine("{0}={1}", e.Key, e.Value);
                }
            }
        }

        private void WriteTool(IMakeWriter writer, PostLink postlink, bool writefiles = true)
        {
            WriteToolGeneric(writer, postlink, writefiles);
        }

        private void WriteBuildSteps(IMakeWriter writer, IEnumerable<BuildStep> steps, uint mask)
        {
            if (steps != null)
            {
                foreach (BuildStep step in steps)
                {
                    if (step.IsKindOf(mask))
                    {
                        WriteBuildStep(writer, step);
                    }
                }
            }
        }

        private void WriteBuildStep(IMakeWriter writer, BuildStep step)
        {
            writer.WriteLine("  BuildStep Name='{0}'  Type={1}", step.Name, BuildStepTypeToString(step));
            writer.WriteTabLine("NAnt Targets:");

            foreach (var target in step.TargetCommands)
            {
                WriteTargetCommand(writer, target);
            }

            writer.WriteTabLine("Commands:");

            foreach (var command in step.Commands)
            {
                WriteCommand(writer, command);
            }
            WriteList(writer, "InputDependencies", step.InputDependencies);
            WriteList(writer, "OutputDependencies", step.OutputDependencies);
        }

        private void WriteTargetCommand(IMakeWriter writer, TargetCommand target)
        {
            writer.WriteTab();
            writer.WriteTabLine("target '{0}'", target.Target);
        }

        private void WriteCommand(IMakeWriter writer, Command command)
        {
            if(command.IsKindOf(Command.ExcludedFromBuild))
            {
                writer.WriteTab();
                writer.WriteTabLine("Excluded from build");
            }

            writer.WriteTab();
            writer.WriteTabLine("Description: {0}", command.Description);

            if(command.IsKindOf(Command.ShellScript))
            {
                writer.WriteTab();
                writer.WriteTabLine("Type: ShellScript");
                foreach(var line in command.Script.LinesToArray())
                {
                    writer.WriteTab();
                    writer.WriteTab();
                    writer.WriteTabLine(line);
                }
            }
            if(command.IsKindOf(Command.Program))
            {
                writer.WriteTab();
                WritePath(writer, "Program", command.Executable);
                writer.WriteTab();
                writer.WriteTabLine("Options:");
                foreach(var option in command.Options)
                {
                    writer.WriteTab();
                    writer.WriteTab();
                    writer.WriteTabLine(option);
                }
            }
            WriteList(writer, "CreateDirectories", command.CreateDirectories, indent:1);
            if (command.WorkingDir != null)
            {
                writer.WriteTab();
                WritePath(writer, "WorkingDir", command.WorkingDir);
            }
            if (command.Env != null)
            {
                writer.WriteTab();
                writer.WriteTabLine("Environment:");
                foreach (var e in command.Env)
                {
                    writer.WriteTab();
                    writer.WriteTab();
                    writer.WriteTabLine("{0}={1}", e.Key, e.Value);
                }
            }
        }

        private void WriteDependency(IMakeWriter writer, Dependency<IModule> dependency)
        {
            var module = dependency.Dependent;

            writer.WriteTabLine("{0} : {1}-{2}  '{3}.{4}'  {5}{6}", DependencyTypes.ToString(dependency.Type),  module.Package.Name, module.Package.Version, module.BuildGroup, module.Name, module.Configuration.Name, String.IsNullOrEmpty(module.Configuration.SubSystem) ? String.Empty : " [" + module.Configuration.SubSystem + "]");
        }

        private void WriteList(IMakeWriter writer, string label, IEnumerable<string> values, bool nonEmptyOnly = false, int indent = 0)
        {
            if (nonEmptyOnly && (values == null || (values != null && values.Count() == 0)))
            {
                return;
            }
            for (int i = 0; i < indent; i++)
            {
                writer.WriteTab();
            }

                writer.WriteTabLine("{0}:",label);
            if(values != null)
            {
                foreach (var v in values)
                {
                    for (int i = 0; i < indent + 1; i++)
                    {
                        writer.WriteTab();
                    }
                    writer.WriteTabLine(v);
                }
            }
        }

        private void WriteList(IMakeWriter writer, string label, IEnumerable<PathString> values, bool nonEmptyOnly = false, int indent = 0)
        {
            if (nonEmptyOnly && (values == null || (values != null && values.Count() == 0)))
            {
                return;
            }

            for (int i = 0; i < indent; i++)
            {
                writer.WriteTab();
            }
            writer.WriteTabLine("{0}:", label);
            if (values != null)
            {
                foreach (var p in values)
                {
                    for (int i = 0; i < indent + 1; i++)
                    {
                        writer.WriteTab();
                    }
                    writer.WriteTabLine(p.Path);
                }
            }
        }

        private void WritePath(IMakeWriter writer, string label, PathString path, bool nonEmptyOnly = false, int indent = 0)
        {
            if (nonEmptyOnly && path == null)
            {
                return;
            }

            for (int i = 0; i < indent; i++)
            {
                writer.WriteTab();
            }
            writer.WriteTab("{0}=", label);
            if (path != null)
            {
                writer.WriteLine(path.Path.Quote());
            }
            else
            {
                writer.WriteLine("---");
            }
        }


        private void WriteFiles(IMakeWriter writer, string label, FileSet fs, bool nonEmptyOnly = false)
        {
            if (nonEmptyOnly && (fs == null || (fs != null && fs.FileItems.Count == 0)))
            {
                return;
            }
            writer.WriteTabLine("{0}:", label);
            if (fs != null)
            {

                foreach (var fi in fs.FileItems)
                {
                    writer.WriteTab();

                    string type = "";
                    FileData filedata = fi.Data as FileData;
                    if (filedata != null)
                    {
                        type = FileData.ToString(filedata.Type);
                    }
                    //************************************************************************************************************************************************
                    // There are helper extensions to test type mask that can be associated with FileItem:
                    //
                    // uint flags = (fileitem.IsKindOf(FileData.Header | FileData.Resource) || module.IsKindOf(Module.MakeStyle)) ? 0 : FileEntry.ExcludedFromBuild; 
                    //
                    //************************************************************************************************************************************************
                    if (String.IsNullOrWhiteSpace(type))
                    {
                        writer.WriteTabLine(fi.Path.Path);
                    }
                    else
                    {
                        writer.WriteTabLine("{0}: {1}", type, fi.Path.Path);
                    }
                }
            }
        }



        private string GetModuleFileName(IModule module)
        {
            return String.Format("{0}-{1}-{2}.{3}.txt", module.Package.Name, module.Package.Version, module.BuildGroup, module.Name);
        }

        private string ModuleTypeToString(Module module)
        {
            StringBuilder sb = new StringBuilder();

            test(module.Type, Module.Native, "Native", sb);
            test(module.Type, Module.DotNet, "DotNet", sb);
            test(module.Type, Module.Utility, "Utility", sb);
            test(module.Type, Module.MakeStyle, "MakeStyle", sb);

            test(module.Type, Module.Managed, "Managed", sb);
            test(module.Type, Module.CSharp, "CSharp", sb);
            test(module.Type, Module.FSharp, "FSharp", sb);
            test(module.Type, Module.EASharp, "EASharp", sb);

            test(module.Type, Module.Program, "Program", sb);
            test(module.Type, Module.Library, "Library", sb);
            test(module.Type, Module.DynamicLibrary, "DynamicLibrary", sb);

            return sb.ToString().TrimEnd('|');
        }

        private string ToolTypeToString(Tool tool)
        {
            StringBuilder sb = new StringBuilder();

            test(tool.Type, Tool.TypeBuild, "TypeBuild", sb);
            test(tool.Type, Tool.TypePreCompile, "TypePreCompile", sb);
            test(tool.Type, Tool.TypePreLink, "TypePreLink", sb);
            test(tool.Type, Tool.TypePostLink, "TypePostLink", sb);

            return sb.ToString().TrimEnd('|');
        }

        private string BuildStepTypeToString(BuildStep step)
        {
            StringBuilder sb = new StringBuilder();

            test(step.Type, BuildStep.PreBuild, "PreBuild", sb);
            test(step.Type, BuildStep.PreLink, "PreLink", sb);
            test(step.Type, BuildStep.PostBuild, "PostBuild", sb);
            test(step.Type, BuildStep.Build, "Build", sb);
            test(step.Type, BuildStep.Clean, "Clean", sb);
            test(step.Type, BuildStep.ReBuild, "ReBuild", sb);
            test(step.Type, BuildStep.ExecuteAlways, "ExecuteAlways", sb);

            return sb.ToString().TrimEnd('|');
        }

        private static void test(uint t, uint mask, string name, StringBuilder sb)
        {
            if ((mask & t) != 0) sb.AppendFormat("{0}|", name);
        }

        private OptionSet GetModuleOptions(ProcessableModule module)
        {
            OptionSet buildoptionset = null;
            if (module != null && module.Package.Project != null)
            {
                if (!module.Package.Project.NamedOptionSets.TryGetValue(module.BuildType.Name, out buildoptionset))
                {
                    buildoptionset = null;
                }
            }
            return buildoptionset;
        }


        private string _outputdir;

    }




}
