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
using Model = EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{

    class VC_Common_PostprocessOptions : AbstractModuleProcessorTask
    {
        public override void Process(Module_Native module)
        {
            OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(Project, module.BuildType.Name);

            CreateMidlTool(module, buildOptionSet);

            CreateResourceTool(module);

            CreateManifestTool(module, buildOptionSet);
        }

        public override void ProcessTool(Linker linker)
        {
            UpdateLinkOptionsWithBaseAddAndDefFile(linker);

            if (linker.ImportLibFullPath != null)
            {
                linker.Options.Add(String.Format("-implib:\"{0}\"", linker.ImportLibFullPath.Path));
                linker.CreateDirectories.Add(new PathString(Path.GetDirectoryName(linker.ImportLibFullPath.Path), PathString.PathState.Directory|PathString.PathState.Normalized));
            }
        }

        public override void ProcessTool(CcCompiler cc)
        {
            ProcessDependents(cc);
        }

        private void UpdateLinkOptionsWithBaseAddAndDefFile(Linker linker)
        {
            string defFile = PropGroupValue(".definition-file." + Module.Configuration.System, null) ?? PropGroupValue(".definition-file");

            if (!String.IsNullOrEmpty(defFile))
            {
                linker.Options.Add(String.Format("-DEF:{0}", PathNormalizer.Normalize(defFile, false)));
            }

            string baseAddr = PropGroupValue(".base-address." + Module.Configuration.System, null) ?? PropGroupValue(".base-address.");

            if (!String.IsNullOrEmpty(baseAddr))
            {
                linker.Options.Add(String.Format("-BASE:{0}", baseAddr));
            }
        }

        private void ProcessDependents(CcCompiler cc)
        {
            if (Module.IsKindOf(Model.Module.Managed))
            {
                // We use this hack to disable warning 4945 because managed modules sometimes load the same assembly twice. 
                // This is introduced by project references support. We need to remove this when the double loading problem is resolved-->

                if (!Module.Package.Project.Properties.Contains(PropGroupName("enablewarning4945")))
                {
                    cc.Options.Add("-wd4945");
                }

                foreach (var assembly in cc.Assemblies.FileItems.Select(item => item.Path))
                {
                    cc.Options.Add(String.Format("-FU \"{0}\"", assembly.Path));
                    // Add assemblies as input dependencies for the dependency check tasks
                    cc.InputDependencies.Include(assembly.Path, failonmissing: false);
                }
            }
        }

        private void CreateMidlTool(Module_Native module, OptionSet buildOptionSet)
        {
            string midleoptions = null;
            if (buildOptionSet.Options.TryGetValue("midl.options", out midleoptions))
            {
                BuildTool midl = new BuildTool(Project, "midl", PathString.MakeNormalized(Project.ExpandProperties(@"???????????????")), BuildTool.TypePreCompile, 
                    outputdir: module.OutputDir, intermediatedir: module.IntermediateDir);

                foreach (var option in midleoptions.LinesToArray())
                {
                    midl.Options.Add(option);
                }
                module.SetTool(midl);
            }
        }

        // In native nant build we use postlink command to build manifest. This tool is created for Visual Studio only.
        //IMTO todo: reconsile this with manifest definition through postlinkstep. We want to use the same definitions for all types of build
        private void CreateManifestTool(Module_Native module, OptionSet buildOptionSet)
        {
            if (module.Link != null && Project.Properties.Contains("package.WindowsSDK.appdir"))
            {
                var manifest = new BuildTool(Project, "vcmanifest", PathString.MakeNormalized(Project.ExpandProperties(@"${package.WindowsSDK.appdir}\BIN\mt.exe")), BuildTool.None,
                    outputdir: module.OutputDir, intermediatedir: module.IntermediateDir, description: "compiling manifest");

                var additionalManifestFiles = AddModuleFiles(module, ".additional-manifest-files");
                var inputResourceManifests = AddModuleFiles(module, ".input-resource-manifests");
//IMTODO: Use -manifest option defined in linker:
                manifest.Files.Append(additionalManifestFiles);
                manifest.InputDependencies.Append(inputResourceManifests);
                manifest.OutputDependencies.Include(module.Link.OutputName + module.Link.OutputExtension + ".embed.manifest", failonmissing: false, baseDir: module.IntermediateDir.Path);
                manifest.OutputDependencies.Include(module.Link.OutputName + module.Link.OutputExtension + ".embed.manifest.res", failonmissing: false, baseDir: module.IntermediateDir.Path);

                module.SetTool(manifest);
            }
        }


        private void CreateResourceTool(Module_Native module)
        {
            // Add resource tool.
            if (module.Link != null)
            {
                FileSet resourcefiles = AddModuleFiles(module, ".resourcefiles", "${package.dir}/" + module.GroupSourceDir + "/**/*.ico");
                
                if (resourcefiles != null && resourcefiles.FileItems.Count > 0)
                {
                    BuildTool rc = null;
                    BuildTool resx = null;

                    var rcfiles = resourcefiles.FileItems.Where(item => Path.GetExtension(item.Path.Path) == ".rc").Select(item => item.Path);

                    if (rcfiles.Count() > 0 && Properties.Contains("eaconfig.PlatformSDK.dir"))
                    {
                        foreach (var resourcefile in rcfiles)
                        {
                            string resorcefilename = Path.GetFileNameWithoutExtension(resourcefile.Path);
                            PathString outputdir = PathString.MakeNormalized(PropGroupValue(".res.outputdir", null) ?? module.IntermediateDir.Path);
                            PathString outputfile = PathString.MakeCombinedAndNormalized(outputdir.Path, PropGroupValue(".res.name", null) ?? resorcefilename + ".res");

                            var includeDirs = new List<string> ()
                            {
                                 PathNormalizer.Normalize(Project.Properties["package.eaconfig.vcdir"] + "/atlmfc/include", false),
                                 PathNormalizer.Normalize(Project.Properties["eaconfig.PlatformSDK.dir"] + "/Include", false),
                                 PathNormalizer.Normalize(Project.Properties["package.eaconfig.vcdir"] + "/Include")
                            };

                            if (Properties.Contains("package.WindowsSDK.MajorVersion"))
                            {
                                int a;

                                Int32.TryParse(Properties["package.WindowsSDK.MajorVersion"], out a);

                                if (a == 0)
                                {
                                    Log.Warning.WriteLine("Warning: property package.WindowsSDK.MajorVersion from the WindowsSDK is not in integer format.");
                                }

                                if (a >= 8)
                                {
                                    includeDirs.Add(PathNormalizer.Normalize(Project.Properties["eaconfig.PlatformSDK.dir"] + "/Include/um"));
                                    includeDirs.Add(PathNormalizer.Normalize(Project.Properties["eaconfig.PlatformSDK.dir"] + "/Include/shared"));
                                }
                            }

                            // Add ModuleDefined include dirs:
                            includeDirs.AddRange(PropGroupValue(".res.includedirs", null).LinesToArray());

                            var rcexe = PathString.MakeNormalized(Project.ExpandProperties("${build.rc.program}"));

                            rc = new BuildTool(Project, "rc", rcexe, BuildTool.TypePreCompile, 
                                String.Format("compile resource '{0}'", Path.GetFileName(resourcefile.Path)),
                                intermediatedir: module.IntermediateDir, outputdir: outputdir);
                            rc.Files.Include(resourcefile.Path, failonmissing: false, data:new FileData(rc));
                            rc.OutputDependencies.Include(outputfile.Path, failonmissing: false);
                            rc.Options.Add("/d _DEBUG");
                            rc.Options.Add("/l 0x409"); // Specify the English codepage 
                            foreach (var dir in includeDirs)
                            {
                                rc.Options.Add("/i " + dir.Quote());
                            }
                            rc.Options.Add("/fo" + outputfile.Path.Quote());
                            rc.Options.Add( resourcefile.Path.Quote());

                            resourcefiles.Exclude(resourcefile.Path);
                            rc.CreateDirectories.Add(outputdir);

                            module.Link.ObjectFiles.Include(outputfile.Path, failonmissing: false);

                            module.AddTool(rc);
                        }

                    }
                    else
                    {
                        if (!module.IsKindOf(Model.Module.Managed))
                        {
                            Log.Error.WriteLine("package '{0}' module '{1}' resource fileset is defined, but contains no files", module.Package.Name, module.Name);
                        }

                    }

                    if (module.IsKindOf(Model.Module.Managed))
                    {
                        var resxfiles = resourcefiles.FileItems.Where(item => Path.GetExtension(item.Path.Path) == ".resx").Select(item => item.Path);

                        if (resxfiles.Count() > 0)
                        {
                            string rootnamespace = PropGroupValue(".rootnamespace", null) ?? module.Name;
                            PathString outputdir = PathString.MakeCombinedAndNormalized(module.IntermediateDir.Path, "forms");

                            resx = new BuildTool(Project, "resx", PathString.MakeNormalized(Project.ExpandProperties(Properties["build.resgen.program"])), BuildTool.TypePreCompile, "compile managed resources.",
                                intermediatedir: outputdir, outputdir: outputdir);
                            resx.Options.Add("/useSourcePath");
                            resx.Options.Add("/compile");
                            foreach (var file in resxfiles)
                            {
                                string filename = Path.GetFileNameWithoutExtension(file.Path);
                                string outresource = Path.Combine(resx.OutputDir.Path, rootnamespace + "." + filename + ".resources");

                                resx.Options.Add(String.Format("{0},{1}", file.Path.Quote(), outresource.Quote()));
                                resx.OutputDependencies.Include(outresource, failonmissing: false);

                                resourcefiles.Exclude(file.Path);
                            }

                            resx.CreateDirectories.Add(outputdir);
                            
                            module.SetTool(resx);
                        }
                        foreach (var res in resourcefiles.FileItems)
                        {
                            module.Link.Options.Add(String.Format("-ASSEMBLYRESOURCE:{0}", res.Path.Path.Quote()));
                        }
                    }

                    if (rc != null)
                    {
                        rc.InputDependencies.AppendWithBaseDir(resourcefiles);
                    }
                    if (resx != null)
                    {
                        resx.InputDependencies.AppendWithBaseDir(resourcefiles);
                    }
                }
            }

        }
    }


}
