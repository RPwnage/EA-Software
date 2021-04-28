using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;
using NAnt.Core.PackageCore;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools; 

namespace EA.Eaconfig.Build
{
    public static class Extensions
    {
        private static readonly string EACONFIG_OUTPUTNAME_MAP_OPTIONS = "package.outputname-map-options";
        private static readonly string EACONFIG_OUTPUTNAME_PACKAGE_MAP_OPTIONS = "package.{0}.outputname-map-options";

        
        public static BuildType CreateBuildType(this Project project, string groupname)
        {
            string buildTypeName = project.Properties.GetPropertyOrDefault(groupname + ".buildtype", "Program");

            // Init common common properties required for the build:
            return GetModuleBaseType.Execute(project, buildTypeName);
        }

        public static void ExecuteProcessSteps(this Project project, ProcessableModule module, OptionSet buildTypeOptionSet, IEnumerable<string> steps, BufferedLog log)
        {
            Chrono timer = (log == null) ? null : new Chrono();
            int count = 0;

            if (log != null)
            {
                log.IndentLevel += 6;
            }

            foreach (string step in steps)
            {
                Task task = project.TaskFactory.CreateTask(step, project);
                if (task != null)
                {
                    IBuildModuleTask bmt = task as IBuildModuleTask;
                    if (bmt != null)
                    {
                        bmt.Module = module;
                        bmt.BuildTypeOptionSet = buildTypeOptionSet;
                    }

                    task.Execute();

                    if (log != null)
                    {
                        log.Info.WriteLine("task   '{0}'", step);
                        count++;
                    }

                }
                else
                {
                    if (project.ExecuteTargetIfExists(step, force: true))
                    {
                        if (log != null)
                        {
                            log.Info.WriteLine("target '{0}'", step);
                            count++;
                        }
                    }
                    else
                    {
                        project.Log.Warning.WriteLine("No task or taget that matches process step '{0}'. Package {1}-{2} ({3}) {4}", step, module.Package.Name, module.Package.Version, module.Configuration.Name, module.BuildGroup + "." + module.Name);
                    }
                }
            }

            if (log != null)
            {
                log.Info.WriteLine("Completed {0} step(s) {1}", count, timer.ToString());
                if (count > 0)
                {
                    log.Flash();
                }
            }

        }

        public static void ExecuteGlobalProjectSteps(this Project project, IEnumerable<string> steps, BufferedLog log)
        {
               
            Chrono timer = (log == null) ? null : new Chrono();
            int count = 0;

            if (log != null)
            {
                log.IndentLevel += 6;
            }

            foreach (string step in steps.OrderedDistinct())
            {
                var task = project.TaskFactory.CreateTask(step, project);
                if (task != null)
                {
                    task.Execute();

                    if (log != null)
                    {
                        log.Info.WriteLine("task   '{0}'",  step);
                        count++;
                    }
                }
                else
                {
                    if (project.ExecuteTargetIfExists(step, force: true))
                    {
                        if (log != null)
                        {
                            log.Info.WriteLine("target '{0}'", step);
                            count++;
                        }
                    }
                    else
                    {
                            project.Log.Warning.WriteLine("Task or target target '{0}' not found", step);
                    }
                }
            }

            if (log != null)
            {
                log.Info.WriteLine("Completed {0} step(s) {1}", count, timer.ToString());
                if (count > 0)
                {
                    log.Flash();
                }
            }
        }

        public static void ExecuteBuildTool(this Project project, Tool tool, bool failonerror = true)
        {
            BuildTool buildtool = tool as BuildTool;
            if (buildtool != null)
            {
                project.ExecuteCommand(buildtool, failonerror, buildtool.ToolName);
            }
            else
            {
                project.Log.Error.WriteLine("Tool [{0}] can not be executed.  Tool type is '{1}', it must be 'BuildTool' to be actionable.", tool.ToolName, tool.GetType().FullName);
            }
        }

        public static Tool GetTool(this FileItem fileitem)
        {
              FileData filedata = fileitem.Data as FileData;
              return (filedata != null) ? filedata.Tool: null;
        }

        public static CopyLocalType GetCopyLocal(this FileItem fileitem, IModule module)
        {
            var copyLocal = CopyLocalType.Undefined;

            if (fileitem != null && !String.IsNullOrEmpty(fileitem.OptionSetName))
            {
                // Check if assembly has optionset attached.
                OptionSet options;
                if (module.Package.Project != null && module.Package.Project.NamedOptionSets.TryGetValue(fileitem.OptionSetName, out options))
                {
                    var option = options.GetOptionOrDefault("copylocal", String.Empty);

                    if (option.Equals("on", StringComparison.OrdinalIgnoreCase) || option.Equals("true", StringComparison.OrdinalIgnoreCase))
                    {
                        copyLocal = CopyLocalType.True;
                    }
                    else if (option.Equals("off", StringComparison.OrdinalIgnoreCase) || option.Equals("false", StringComparison.OrdinalIgnoreCase))
                    {
                        copyLocal = CopyLocalType.False;
                    }
                }
                else if (fileitem.OptionSetName.Equals("copylocal", StringComparison.OrdinalIgnoreCase))
                {
                    copyLocal = CopyLocalType.True;
                }
            }

            return copyLocal;
        }


        public static OptionSet GetOutputMapping(this Project project, Configuration currentconfig, IPackage package)
        {
            OptionSet outputmapOptionset = null;
            // Can map only buildable packages with the same configuration
            if (package.ConfigurationName == currentconfig.Name && package.IsKindOf(EA.FrameworkTasks.Model.Package.Buildable))
            {
                return project.GetOutputMapping(package.Name);
            }
            return outputmapOptionset;
        }

        public static OptionSet GetOutputMapping(this Project project, string packagename)
        {
            OptionSet outputmapOptionset = null;
            // Can map only buildable packages
            string optionsetname = null;
            // Is mapping defined through masterconfig groups?
            if (!String.IsNullOrEmpty(packagename))
            {
                var release = PackageMap.Instance.GetMasterRelease(packagename, project);
                if (release != null)
                {
                    if (release.Buildable)
                    {
                        optionsetname = release.OutputMapOptionSet(project);

                        if (String.IsNullOrEmpty(optionsetname))
                        {
                            optionsetname = project.Properties[String.Format(EACONFIG_OUTPUTNAME_PACKAGE_MAP_OPTIONS, packagename)] ?? project.Properties[EACONFIG_OUTPUTNAME_MAP_OPTIONS];
                        }
                    }
                }
                else if (String.IsNullOrEmpty(optionsetname))
                {
                    optionsetname = project.Properties[String.Format(EACONFIG_OUTPUTNAME_PACKAGE_MAP_OPTIONS, packagename)] ?? project.Properties[EACONFIG_OUTPUTNAME_MAP_OPTIONS];
                }
            }
            else
            {
                optionsetname = project.Properties[String.Format(EACONFIG_OUTPUTNAME_PACKAGE_MAP_OPTIONS, packagename)] ?? project.Properties[EACONFIG_OUTPUTNAME_MAP_OPTIONS];
            }

            if (!String.IsNullOrEmpty(optionsetname))
            {

                    if (!project.NamedOptionSets.TryGetValue(optionsetname, out outputmapOptionset))
                    {
                        Error.Throw(project, "ModuleProcessor.GetOutputMapping()", "OutputMapping optionset '{0}' specified in masterconfig file does not exist", optionsetname);
                    }
            }

            return outputmapOptionset;
        }

        #region FileData Extensions
        // ---- FileData extensions -----------------------------------------------------

        public static FileSet SetFileDataFlags(this FileSet fs, uint flags)
        {
            if (fs != null)
            {
                foreach (var fsi in fs.Includes)
                {
                    fsi.SetFileDataFlags(flags);
                }
            }
            return fs;
        }

        public static FileSetItem SetFileDataFlags(this FileSetItem fi, uint flags)
        {
            if (fi != null)
            {
                if (fi.Data == null)
                {
                    fi.Data = new FileData(null, flags: flags);
                }
                else if (fi.Data is FileData)
                {
                    (fi.Data as FileData).SetType(flags);
                }
            }
            return fi;
        }

        public static FileItem SetFileDataFlags(this FileItem fi, uint flags)
        {
            if (fi != null)
            {
                if (fi.Data == null)
                {
                    fi.Data = new FileData(null, flags: flags);
                }
                else if (fi.Data is FileData)
                {
                    (fi.Data as FileData).SetType(flags);
                }
            }
            return fi;
        }

        public static FileSetItem ClearFileDataFlags(this FileSetItem fi, uint flags)
        {
            if (fi != null)
            {
                if (fi.Data == null)
                {
                    fi.Data = new FileData(null, flags: flags);
                }
                else if (fi.Data is FileData)
                {
                    (fi.Data as FileData).ClearType(flags);
                }
            }
            return fi;
        }

        public static FileItem ClearFileDataFlags(this FileItem fi, uint flags)
        {
            if (fi != null)
            {
                if (fi.Data == null)
                {
                    fi.Data = new FileData(null, flags: flags);
                }
                else if (fi.Data is FileData)
                {
                    (fi.Data as FileData).ClearType(flags);
                }
            }
            return fi;
        }

        internal static void SetFileData(this FileItem fi, Tool tool, PathString objectfile = null, uint flags=0)
        {
            if (fi != null)
            {
                var olddata = fi.Data as FileData;
                uint _flags = flags;
                if (olddata != null)
                {
                    _flags |= olddata.Type;
                }
                fi.Data = new FileData(tool, objectfile, _flags);
            }
        }

        public static bool IsKindOf(this FileSetItem fi, uint flags)
        {
            if (fi != null)
            {
                var mask = fi.Data as BitMask;
                if (mask != null)
                {
                    return mask.IsKindOf(flags);
                }
            }
            return false;
        }

        public static bool IsKindOf(this FileItem fi, uint flags)
        {
            if (fi != null)
            {
                var mask = fi.Data as BitMask;
                if (mask != null)
                {
                    return mask.IsKindOf(flags);
                }
            }
            return false;
        }

        public static uint Flags(this FileItem fi)
        {
            uint flags = 0;
            if (fi != null)
            {
                var mask = fi.Data as BitMask;
                if (mask != null)
                {
                    return flags = mask.Type;
                }
            }
            return flags;
        }
        #endregion

        #region Module extensions

        public static IEnumerable<PathString> ObjectFiles(this IModule imodule)
        {
            var objfiles = new List<PathString>();

            Module module = imodule as Module;

            if (module != null && module.Tools != null)
            {
                
                var asm = module.Tools.SingleOrDefault(t => t.ToolName == "asm") as AsmCompiler;

                if (asm != null)
                {
                    AddObjectFiles(asm.SourceFiles, objfiles);
                }

                var cc = module.Tools.SingleOrDefault(t => t.ToolName == "cc") as CcCompiler;

                if (cc != null)
                {
                    AddObjectFiles(cc.SourceFiles, objfiles);
                }
            }

            return objfiles;
        }

        private static void AddObjectFiles(FileSet sourcefiles, List<PathString> objfiles)
        {
            if(sourcefiles != null)
            {
                foreach (var fi in sourcefiles.FileItems)
                {
                    EA.Eaconfig.Core.FileData filedata = fi.Data as EA.Eaconfig.Core.FileData;

                    if (filedata != null && filedata.ObjectFile != null && !String.IsNullOrEmpty(filedata.ObjectFile.Path))
                    {
                        objfiles.Add(filedata.ObjectFile);
                    }
                }
            }
        }

        public static PathString DynamicLibOrAssemblyOutput(this IModule imodule)
        {
            Module module = imodule as Module;

            if (module != null && module.Tools != null)
            {
                if (module.IsKindOf(Module.DynamicLibrary))
                {
                    Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

                    if (link != null)
                    {
                        return PathString.MakeNormalized(Path.Combine(link.LinkOutputDir.Path, link.OutputName + link.OutputExtension));
                    }
                }
                if (module.IsKindOf(Module.DotNet))
                {
                    var dotnetmodule = module as Module_DotNet;
                    if (dotnetmodule != null && dotnetmodule.Compiler != null)
                    {
                        return PathString.MakeNormalized(Path.Combine(module.OutputDir.Path, dotnetmodule.Compiler.OutputName + dotnetmodule.Compiler.OutputExtension));
                    }
                }
            }
            return null;
        }


        public static PathString LibraryOutput(this IModule imodule)
        {
            Module module = imodule as Module;

            if (module != null && module.Tools != null)
            {
                Librarian lib = module.Tools.SingleOrDefault(t => t.ToolName == "lib") as Librarian;

                if (lib != null)
                {
                    return PathString.MakeNormalized(Path.Combine(module.OutputDir.Path, lib.OutputName + lib.OutputExtension));
                }

                Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

                if (link != null)
                {
                    return link.ImportLibFullPath;
                }
            }

            return null;
        }

        public static string PrimaryOutput(this IModule imodule)
        {
            Module module = imodule as Module;

            if (module != null && module.Tools != null)
            {
                Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

                if (link != null)
                {
                    return Path.Combine(link.LinkOutputDir.Path, link.OutputName + link.OutputExtension);
                }

                Librarian lib = module.Tools.SingleOrDefault(t => t.ToolName == "lib") as Librarian;

                if (lib != null)
                {
                    return Path.Combine(module.OutputDir.Path, lib.OutputName + lib.OutputExtension);
                }

                Module_DotNet dotNetMod = module as Module_DotNet;

                if (dotNetMod != null && dotNetMod.Compiler != null)
                {
                    return Path.Combine(module.OutputDir.Path, dotNetMod.Compiler.OutputName + dotNetMod.Compiler.OutputExtension);
                }
            }

            return Path.Combine(module.OutputDir.Path, module.OutputName ?? module.Name);
        }

        public static string PrimaryOutputName(this IModule imodule)
        {
            Module module = imodule as Module;

            if (module != null && module.Tools != null)
            {
                Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

                if (link != null)
                {
                    return link.OutputName;
                }

                Librarian lib = module.Tools.SingleOrDefault(t => t.ToolName == "lib") as Librarian;

                if (lib != null)
                {
                    return lib.OutputName;
                }

                Module_DotNet dotNetMod = module as Module_DotNet;

                if (dotNetMod != null && dotNetMod.Compiler != null)
                {
                    return dotNetMod.Compiler.OutputName;
                }
            }

            return module.OutputName??module.Name;
        }

        public static void PrimaryOutput(this IModule imodule, out string outputdir, out string name, out string extension)
        {
            Module module = imodule as Module;

            if (module != null && module.Tools != null)
            {
                Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

                if (link != null)
                {
                    outputdir = link.LinkOutputDir.Path;
                    name = link.OutputName;
                    extension = link.OutputExtension;
                    return;
                }

                Librarian lib = module.Tools.SingleOrDefault(t => t.ToolName == "lib") as Librarian;

                if (lib != null)
                {
                    outputdir = module.OutputDir.Path;
                    name = lib.OutputName;
                    extension = lib.OutputExtension;
                    return;
                }

                Module_DotNet dotNetMod = module as Module_DotNet;

                if (dotNetMod != null && dotNetMod.Compiler != null)
                {
                    outputdir = module.OutputDir.Path;
                    name = dotNetMod.Compiler.OutputName;
                    extension = dotNetMod.Compiler.OutputExtension;
                    return;
                }
            }
            outputdir = module.OutputDir.Path;
            name = module.OutputName ?? module.Name;
            extension = String.Empty;
        }

        public static void ReplaceTemplates(this Module_Native module, Tool tool, params string[] additional)
        {
            if (module != null && tool != null)
            {
                var outputlibdir = module.OutputDir;
                var outputname = module.Name;
                if (module.Link != null && module.Link.ImportLibOutputDir != null)
                {
                    outputlibdir = module.Link.ImportLibOutputDir;
                    outputname =  module.Link.OutputName;
                }
                else if (module.Lib != null)
                {
                    outputname = module.Lib.OutputName;
                }
                for (int i = 0; i < tool.Options.Count; i++)
                {
                    var replaced = tool.Options[i]
                        .Replace("%outputlibdir%", outputlibdir.Path)
                        .Replace("%outputbindir%", module.OutputDir.Path)
                        .Replace("%intermediatedir%", module.IntermediateDir.Path)
                        .Replace("%outputdir%", module.OutputDir.Path)
                        .Replace("%outputname%", outputname)
                        .Replace("\"%pchheaderfile%\"", module.PrecompiledHeaderFile.Quote())
                        .Replace("%pchheaderfile%", module.PrecompiledHeaderFile.Quote())
                        .Replace("%pchfile%", module.PrecompiledFile.Path);
                    if (additional != null)
                    {
                        for(int j =0; j < additional.Length; j+=2)
                        {
                            replaced = replaced.Replace(additional[j], additional[j + 1]);
                        }
                    }

                    tool.Options[i] = replaced;
                }
            }
        }


        public static string ModuleIdentityString(this IModule imodule)
        {
            return imodule != null ? String.Format("{0}/{1}.{2} ({3})", imodule.Package.Name, imodule.BuildGroup, imodule.Name, imodule.Configuration.Name) : String.Empty;
        }

        #endregion

    }
}
