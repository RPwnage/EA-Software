using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;

using EA.nantToVSTools;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace EA.Eaconfig
{
    [TaskName("EaconfigVcproj")]
    public class EaconfigVcproj : EaconfigVisualStudioBase
    {

        public static void Execute(Project project, string groupname, string buildModule)
        {
            EaconfigVcproj task = new EaconfigVcproj(project, groupname, buildModule);
            task.Execute();
        }

        public EaconfigVcproj(Project project, string groupname, string buildModule)
            : base(project, groupname, buildModule)
        {
        }

        public EaconfigVcproj()
            : base()
        {
        }

        protected override string PreBuildTargetProperty
        {
            get { return ".vcproj.prebuildtarget"; }
        }

        protected override bool IsValidProjectType
        {
            get
            {
                BuildType bt = ComputeBaseBuildData.GetBuldType(Project, Properties[PropGroupName("buildtype")]);

                return !bt.IsCSharp && !bt.IsFSharp;
            }
        }

        protected override string BuildTypeName
        {
            get { return "build.buildtype.vcproj"; }
        }

        protected override bool Build()
        {
            TargetUtil.ExecuteTarget(Project, "eaconfig-vcproj-nanttovcproj-invoker", true);
            return true;
        }

        protected override void PostBuild()
        {
            FileSetFunctions.FileSetUndefine(Project, "__private_build.assemblies.all");
            OptionSetFunctions.OptionSetUndefine(Project, "build.buildtype.vcproj");

            string[] cleanProperties = new string[] 
            {
                "build.imgbldoptions", 
                "__private.customvcprojremoteroot", 
                "__private.dvdemulationroot", 
                "__private.deploymentfiles" };

            foreach (string name in cleanProperties)
            {
                Properties.Remove(name);
            }
            if (cleartOutputBinary)
            {
                Properties.Remove("outputbinary");
            }
        }

        protected override void SetupProjectInput()
        {
            if ((ConfigCompiler == "sn" || ConfigSystem == "ps3") && !PropertyUtil.GetBooleanProperty(Project, "platform-no-snvsi-support"))
            {
                TaskUtil.Dependent(Project, "snvsi");
            }

            if (BuildData.BuildType.Name == "Utility")
            {
                BuildData.BuildOptionSet.Options["build.tasks"] = "utility";
            }
            else if (BuildData.BuildType.Name == "MakeStyle")
            {
                BuildData.BuildOptionSet.Options["build.tasks"] = "makestyle";
            }

            //TIB
            // If this is an autobuildable program or dll, add in explicit linker input libs for build dependents that have autobuildclean set to false.
            // For programs or dlls that aren't autobuildable, we rely on the slnmaker task to add in the appropriate project dependencies resulting in the correct linker line.
            if (OptionSetUtil.IsOptionContainValue(BuildData.BuildOptionSet, "build.tasks", "link"))
            {
                bool packageIsAutobuildable = true;

                if (PropertyUtil.PropertyExists(Project, "package." + Properties["package.name"] + ".autobuildclean"))
                {
                    packageIsAutobuildable = PropertyUtil.GetBooleanProperty(Project, "package." + Properties["package.name"] + ".autobuildclean");
                }

                if (packageIsAutobuildable)
                {
                    AddBuildDependentLibs(BuildData);
                }
            }

            //Generates the build.vcproj.sourcefiles.all and build.vcproj.excludedbuildfiles.all filesets
            VcprojFileSetSetup(BuildData);

            string ccoutputpath = BuildOutput.OutputBuildDir;
            if (ccoutputpath.EndsWith(BuildProjectName))
            {
                ccoutputpath = Path.GetDirectoryName(ccoutputpath);
            }
            string outputbinary = PropertyUtil.GetPropertyOrDefault(Project, "outputbinary", BuildOutput.OutputName);
            string finalPath = (BuildData.BuildType.BaseName == "StdLibrary") ? BuildOutput.OutputLibDir : BuildOutput.OutputBinDir;


            if (ConfigSystem == "ps3")
            {
                IDictionary<string, string> taskParameters = new Dictionary<string, string>()
                {
                    {"BuildTypeName", BuildTypeName},
                    {"FileSetName", "build.vcproj.sourcefiles.all"}
                };

                Project.NamedFileSets["build.vcproj.sourcefiles.all"] = BuildData.BuildFileSets["build.vcproj.sourcefiles.all"];
                Project.NamedOptionSets[BuildTypeName] = BuildData.BuildOptionSet;
                TaskUtil.ExecuteScriptTask(Project, "ApplyConvertSystemIncludesToCCOptions", taskParameters);
            }


            //TODO: This needs to be moved to BuildData (need to resolve dependency on finalPath
            // and see if it is common with native nant)
            // --- Setup PCH options

            string pchFile = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("pch-file"), "%outputdir%\\stdPCH.pch");
            string pchHeaderFile = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("pch-header-file"), String.Empty);
            Resolve_PCH_Templates.Execute(Project, BuildData.BuildOptionSet, pchFile, pchHeaderFile, finalPath, BuildData.BuildFileSets["build.vcproj.sourcefiles.all"]);

            SetupReferences();

            SetupProperties(outputbinary, finalPath, ccoutputpath);

            //For certain platforms, required that link directories exist  (i.e. xenon)
            SetupLinkDirectories(ProjBuildData, BuildOutput.OutputBinDir, outputbinary);

            if (ConfigSystem == "xenon" && BuildData.BuildType.Name == "Utility")
            {
                string cbtool = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("vcproj.custom-build-tool"), String.Empty);
                if (!String.IsNullOrEmpty(cbtool))
                {
                    string msg = Error.Format(Project, "eaconfig-vcproj", "WARNING",
                        "Custom build steps are not supported for Utility module '{0}' on XENON configurations.\n\n" +
                        "If the module is of 'Utility' type, it's assigned the 'Xenon Utility' type in its Visual Studio project." +
                        "However VS Xenon Utility type only has pre/post build steps and it does not support custom build steps.",
                        Properties["build.module"]);

                    Log.WriteLine(msg);
                }
            }

            OptionSetUtil.CreateOptionSetInProjectIfMissing(Project, PropGroupName("vcproj.custom-build-rules-options"));
        }

        private string GetPreBuildStep(ProjBuildData BaseBuildData)
        {
            StringBuilder preBuildStep = new StringBuilder();

            /* Ideally we want to execute beforebuildtarget during the build, but because some SDKs use nant properties/optionsets , and filesets that are defined in eaconfig build caller
             * it is impossible to have gseparate nant invocation and to pass all required data.
             * For now, beforebuild target is executed during solution generation.
             */
            string targetName = Project.Properties[PropGroupName(".beforebuildsteptarget")];

            if(String.IsNullOrEmpty(targetName))
            {                
                targetName = PropGroupName(".beforebuildsteptarget");
            }

            if (!String.IsNullOrEmpty(targetName) && TargetUtil.TargetExists(Project, targetName))
            {
                preBuildStep.AppendLine(Project.ExpandProperties("${nant.location}/nant.exe " + targetName +
                    " -D:eaconfig.build.group=${eaconfig.build.group} -D:build.buildtype=${build.buildtype}" +
                    " -D:build.buildtype.base=${build.buildtype.base} -D:groupname=${groupname} -D:config=${config}" +
                    " -D:build.module=${build.module}" +
                    " -D:build.outputname=${build.outputname}" +
                    " -buildfile:${package.dir}/${package.name}.build"+
                    " -masterconfigfile:${nant.project.masterconfigfile}"+
                    " -buildroot:${nant.project.buildroot}"));
            }

            // Execute any platform specific custom target that SDK might define:

            targetName = Project.Properties["eaconfig." + ConfigPlatform + ".beforebuildsteptarget"];

            if (!String.IsNullOrEmpty(targetName) && TargetUtil.TargetExists(Project, targetName))
            {
                preBuildStep.AppendLine(Project.ExpandProperties("${nant.location}/nant.exe " + targetName + 
                    " -D:eaconfig.build.group=${eaconfig.build.group} -D:build.buildtype=${build.buildtype}"+
                    " -D:build.buildtype.base=${build.buildtype.base} -D:groupname=${groupname} -D:config=${config}"+
                    " -D:build.module=${build.module}" +
                    " -D:build.outputname=${build.outputname}" +
                    " -buildfile:${package.dir}/${package.name}.build"+
                    " -masterconfigfile:${nant.project.masterconfigfile}"+
                    " -buildroot:${nant.project.buildroot}"+
                    " -D:build.usedependencies=\""+ StringUtil.EnsureSeparator(Project.Properties["build.usedependencies"], ' ')+"\""+
                    " -D:build.builddependencies.all=\""+ StringUtil.EnsureSeparatorAndQuote(Project.Properties["build.builddependencies.all"], ' ')+"\""));
            }
            
            // --- Add user's pre-build-step if any ---
            preBuildStep.AppendLine(Properties[PropGroupName("vcproj.pre-build-step")]);

            return StringUtil.Trim(preBuildStep.ToString());
        }

        private string GetPostBuildStep(ProjBuildData BaseBuildData, string libPath, string binPath, string outputbinary, string ccoutputpath)
        {
            StringBuilder postBuildStep = new StringBuilder();

            if (BaseBuildData.BuildType.BaseName == "DynamicLibrary")
            {
                if (ConfigCompiler == "vc")
                {
                    libPath = PathFunctions.PathToWindows(Project, libPath);
                    binPath = PathFunctions.PathToWindows(Project, binPath);
                    postBuildStep.AppendFormat("if not exist {0} mkdir {0}\n", libPath);
                    postBuildStep.AppendFormat("if exist {0}\\{1}.lib %SystemRoot%\\system32\\xcopy {0}\\{1}.lib {2} /d /f /y\n", binPath, outputbinary, libPath);
                }
                if (ConfigSystem == "ps3")
                {
                    //A (hopefully) temporary solution to move any PRX stub libraries and their
                    // associated TXT files to the configlibdir.  We can't find any command-line
                    // method of controlling the output location.  They just seem to get dumped
                    // in the current directory.  There doesn't seem to any need to support
                    // controlling the stub LIB filenames via options, because there the PRX
                    // system defines a standard naming convention for them itself.  We still
                    // don't really know what the TXT file is for. -->
                    libPath = PathFunctions.PathToWindows(Project, libPath);

                    postBuildStep.AppendFormat("if exist *_stub.a %SystemRoot%\\system32\\xcopy *_stub.a {0} /d /f /y\n", libPath);
                    postBuildStep.AppendLine("if exist *_stub.a del *_stub.a");
                    postBuildStep.AppendFormat("if exist *.txt %SystemRoot%\\system32\\xcopy *.txt  {0} /d /f /y\n", libPath);
                    postBuildStep.AppendLine("if exist *.txt del *.txt");
                }

            }
            //Add a post-build step for PS3 PPU programs to generate the SELF from the ELF.
            if (ConfigSystem == "ps3" && !BaseBuildData.BuildOptionSet.Options.Contains("ps3-spu"))
            {
                if (BaseBuildData.BuildType.BaseName == "StdProgram" ||
                    BaseBuildData.BuildType.BaseName == "DynamicLibrary")
                {
                    string link_outputname = BaseBuildData.BuildOptionSet.Options["linkoutputname"];

                    string oformat_secure = String.Empty;
                    string linkoutput_secure = String.Empty;

                    if (BaseBuildData.BuildType.BaseName == "StdProgram")
                    {
                        oformat_secure = "-oformat=fself";
                        linkoutput_secure = "linkoutputnameself";
                    }
                    else // Dll
                    {
                        oformat_secure = "-oformat=fsprx";
                        linkoutput_secure = "linkoutputnamesprx";
                    }

                    string link_options = BaseBuildData.BuildOptionSet.Options["link.options"];
                    if (!link_options.Contains(oformat_secure))
                    {
                        string link_outputnamesecure = BaseBuildData.BuildOptionSet.Options[linkoutput_secure];
                        link_outputnamesecure = link_outputnamesecure.Replace("%outputdir%", binPath);
                        link_outputnamesecure = link_outputnamesecure.Replace("%outputname%", outputbinary);

                        link_outputname = link_outputname.Replace("%outputdir%", binPath);
                        link_outputname = link_outputname.Replace("%outputname%", outputbinary);

                        string ps3AppDir = Properties["package." + Properties["PlayStation3Package"] + ".appdir"];
                        postBuildStep.AppendFormat("{0}\\host-win32\\bin\\make_fself {1} {2}\n", ps3AppDir, link_outputname, link_outputnamesecure);
                    }
                }
            }

            if (BaseBuildData.BuildType.BaseName == "StdProgram")
            {
                if (ConfigSystem == "xbox")
                {
                    ccoutputpath = PathFunctions.PathToWindows(Project, ccoutputpath);
                    binPath = PathFunctions.PathToWindows(Project, binPath);
                    postBuildStep.AppendFormat("%SystemRoot%\\system32\\xcopy {0}\\{1}]\\*.xbe {2} /d /f /y\n", ccoutputpath, BuildProjectName, binPath);
                }
            }

            // --- Add user's post-build-step if any ---

            postBuildStep.AppendLine(Properties[PropGroupName("vcproj.post-build-step")]);

            return StringUtil.Trim(postBuildStep.ToString());
        }

        private void AddBuildDependentLibs(BaseBuildData basebuildData)
        {
            string buildDependencies = basebuildData.BuildProperties["build.builddependencies.all"];
            foreach (string dependent in StringUtil.ToArray(buildDependencies))
            {
                FileSet buildLibs = basebuildData.BuildFileSets["build.libs.all"];
                FileSet buildDlls = basebuildData.BuildFileSets["build.dlls.all"];

                FileSet depLibs = FileSetUtil.GetFileSet(Project, "package." + dependent + ".libs" + basebuildData.BuildType.SubSystem);
                FileSet depDlls = FileSetUtil.GetFileSet(Project, "package." + dependent + ".dlls" + basebuildData.BuildType.SubSystem);

                if (PropertyUtil.IsPropertyEqual(Project, "package." + dependent + ".autobuildclean", "false"))
                {
                    // If package exports per-module libraries - use them
                    int perModuleLibCount = 0;
                    foreach (string depModule in StringUtil.ToArray(Project.Properties[PropGroupName(dependent + "." + BuildGroup + ".buildmodules")]))
                    {
                        perModuleLibCount += FileSetUtil.FileSetAppend(buildLibs, FileSetUtil.GetFileSet(Project, "package." + dependent + "." + depModule + ".libs" + BuildData.BuildType.SubSystem));
                    }

                    // If there are no per-module libraries exported use package libs
                    if (perModuleLibCount == 0 && depLibs != null)
                    {
                        FileSetUtil.FileSetAppend(buildLibs, depLibs);
                    }
                    if (perModuleLibCount == 0 && depDlls != null)
                    {
                        FileSetUtil.FileSetAppend(buildDlls, depDlls);
                    }

                }
                else if (depLibs != null && (PropertyUtil.GetBooleanProperty(Project, "package." + dependent + ".easharp") || PropertyUtil.IsPropertyEqual(Project, "package." + dependent + ".frameworkversion", "1")))
                {
                    FileSetUtil.FileSetAppend(buildLibs, depLibs);
                }

                depLibs = FileSetUtil.GetFileSet(Project, "package." + dependent + ".libs" + basebuildData.BuildType.SubSystem + ".external");
                if (depLibs != null)
                {
                    FileSetUtil.FileSetAppend(buildLibs, depLibs);
                }
            }
        }

        private void VcprojFileSetSetup(BaseBuildData basebuildData)
        {
            if (basebuildData.BuildType.IsManaged)
            {
                FileSetUtil.FileSetAppend(basebuildData.BuildFileSets["build.sourcefiles.all"], basebuildData.BuildFileSets["build.resourcefiles.all"]);
            }

            FileSet convertedExcludeFiles = new FileSet();
            if (basebuildData.BuildType.Name == "Utility")
            {
                if (basebuildData.BuildFileSets["build.sourcefiles.all"].FileItems.Count != 0)
                {
                    string msg = Error.Format(Project, "eaconfig-vcproj", "WARNING",
                        "'Utility' project should not contain any buildable fileset.  Moving the sourcefiles to excludedbuildfiles fileset.");
                    Log.WriteLine(msg);

                    FileSetUtil.FileSetAppend(
                        convertedExcludeFiles,
                        basebuildData.BuildFileSets["build.sourcefiles.all"]);
                }
            }

            // --------- Source files -----------------
            FileSet source = new FileSet();
            if (convertedExcludeFiles.FileItems.Count == 0)
            {
                FileSetUtil.FileSetAppendWithBaseDir(source, basebuildData.BuildFileSets["build.sourcefiles.all"]);
            }
            //Pass header files into the generated VCPROJ
            FileSetUtil.FileSetAppend(source, basebuildData.BuildFileSets["build.headerfiles.all"]);
            basebuildData.BuildFileSets["build.vcproj.sourcefiles.all"] = source;

            FileSet excluded = new FileSet();
            if (convertedExcludeFiles.FileItems.Count != 0)
            {
                FileSetUtil.FileSetAppendWithBaseDir(excluded, convertedExcludeFiles);
            }
            excluded.BaseDirectory = Properties["package.dir"];
            excluded.FailOnMissingFile = false;

            FileSetUtil.FileSetAppendWithBaseDir(excluded, FileSetUtil.GetFileSet(Project, PropGroupName("vcproj.excludedbuildfiles")));
            FileSetUtil.FileSetAppendWithBaseDir(excluded, FileSetUtil.GetFileSet(Project, PropGroupName("vcproj.excludedbuildfiles." + ConfigSystem)));

            // Pass source files inside BulkBuild CPPs into the VCPROJ as 'excluded from build'
            if (BuildData.BulkBuild)
            {
                if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.manual.sourcefiles")) ||
                   FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.manual.sourcefiles." + ConfigSystem)))
                {
                    // This is the case where the user supplies pre-generated bulkbuild files.
                    FileSetUtil.FileSetAppendWithBaseDir(excluded, FileSetUtil.GetFileSet(Project, PropGroupName("sourcefiles")));
                    FileSetUtil.FileSetAppendWithBaseDir(excluded, FileSetUtil.GetFileSet(Project, PropGroupName("sourcefiles." + ConfigSystem)));
                }
                else if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.sourcefiles")))
                {
                    //This is the simple case where the user supplies a single bulkbuild fileset.
                    FileSetUtil.FileSetAppendWithBaseDir(excluded, FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles")));
                }
                else if (PropertyExists(PropGroupName("bulkbuild.filesets")))
                {
                    foreach (string filesetName in StringUtil.ToArray(Properties[PropGroupName("bulkbuild.filesets")]))
                    {
                        if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild." + filesetName + ".sourcefiles")))
                        {
                            FileSetUtil.FileSetInclude(excluded, FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild." + filesetName + ".sourcefiles")));
                        }
                    }
                }
            }

            basebuildData.BuildFileSets["build.vcproj.excludedbuildfiles.all"] = excluded;

            // --------- END Source files -----------------

            FileSet targetFileSet = new FileSet();
            FileSetUtil.FileSetAppend(targetFileSet, FileSetUtil.GetFileSet(Project, PropGroupName("vcproj.additional-manifest-files")));
            FileSetUtil.FileSetAppend(targetFileSet, FileSetUtil.GetFileSet(Project, PropGroupName("vcproj.additional-manifest-files" + "." + ConfigSystem)));
            basebuildData.BuildFileSets["__private_vcproj.additional-manifest-files.all"] = targetFileSet;

            targetFileSet = new FileSet();
            FileSetUtil.FileSetAppend(targetFileSet, FileSetUtil.GetFileSet(Project, PropGroupName("vcproj.input-resource-manifests")));
            FileSetUtil.FileSetAppend(targetFileSet, FileSetUtil.GetFileSet(Project, PropGroupName("vcproj.input-resource-manifests" + "." + ConfigSystem)));
            basebuildData.BuildFileSets["__private_vcproj.input-resource-manifests.all"] = targetFileSet;

            // Create default filesets
            FileSetUtil.CreateFileSetInProjectIfMissing(Project, PropGroupName("vcproj.custom-build-dependencies"));
            FileSetUtil.CreateFileSetInProjectIfMissing(Project, PropGroupName("vcproj.custom-build-rules"));
        }

        private void SetupLinkDirectories(ProjBuildData BaseBuildData, string binPath, string outputbinary)
        {
            if (BaseBuildData.BuildType.BaseName != "StdLibrary")
            {
                string[] OPTION_NAME_LIST = new string[] { "linkoutputpdbname", "linkoutputmapname", "imgbldoutputname", "linkoutputnameself", "linkoutputnamesprx" };

                foreach (string option in OPTION_NAME_LIST)
                {
                    string link_outputName = BaseBuildData.BuildOptionSet.Options[option];
                    if (!String.IsNullOrEmpty(link_outputName))
                    {
                        try
                        {
                            //For Xenon at least, we need to guarantee these directories exist beforehand
                            //For whatever reason, with custom option sets that override the default outpunames, their directories need to exist
                            //and the linker will not create them beforehand. These really should be removed to build targets though...
                            link_outputName = link_outputName.Replace("%outputdir%", binPath);
                            link_outputName = link_outputName.Replace("%outputname%", outputbinary);
                            string link_outputDir = Path.GetDirectoryName(link_outputName);
                            if (!Directory.Exists(link_outputDir))
                                Directory.CreateDirectory(link_outputDir);
                        }
                        catch (Exception) { }
                    }
                }
            }
        }

        private void SetupProperties(string outputbinary, string finalPath, string ccoutputpath)
        {
            BuildData.BuildProperties["__private.uselibrarydependencyinputs"] = OptionSetUtil.GetBooleanOption(BuildData.BuildOptionSet, "uselibrarydependencyinputs") ? "true" : "false";

            if (BuildData.BuildType.BaseName != "StdLibrary")
            {
                BuildData.BuildProperties["workingdir"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("workingdir"), String.Empty);
                BuildData.BuildProperties["remotemachine"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("remotemachine"), String.Empty);
                BuildData.BuildProperties["commandprogram"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("commandprogram"), String.Empty);
                BuildData.BuildProperties["commandargs"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("commandargs"), String.Empty);
            }
            else
            {
                BuildData.BuildProperties["workingdir"] = String.Empty;
                BuildData.BuildProperties["remotemachine"] = String.Empty;
                BuildData.BuildProperties["commandprogram"] = String.Empty;
                BuildData.BuildProperties["commandargs"] = String.Empty;
            }
            // keyfile is for .Net Assemblies, not for native C++ modules
            if (BuildData.BuildType.IsManaged)
            {
                BuildData.BuildProperties["keyfilename"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("keyfile"), String.Empty);
            }
            else
            {
                BuildData.BuildProperties["keyfilename"] = String.Empty;
            }

            // We use this hack to disable warning 4945 because managed modules sometimes load the same assembly twice. 
            // This is introduced by project references support. We need to remove this when the double loading problem is resolved-->

            if (BuildData.BuildType.IsManaged)
            {
                if (!PropertyExists(PropGroupName("enablewarning4945")))
                {
                    BuildData.BuildProperties["build.warningsuppression.all"] += "\n" + "-wd4945";
                }
            }

            BuildData.BuildProperties["vcproj-name"] = BuildProjectName;

            BuildData.BuildProperties["__private.customvcprojremoteroot"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("customvcprojremoteroot"), String.Empty);

            BuildData.BuildProperties["__private.dvdemulationroot"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("dvdemulationroot"), String.Empty);
            BuildData.BuildProperties["__private.deploymentfiles"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("deploymentfiles"), String.Empty);

            BuildData.BuildProperties["custom-build-tool"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("vcproj.custom-build-tool"), String.Empty);
            BuildData.BuildProperties["custom-build-outputs"] = StringUtil.EnsureSeparator(PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("vcproj.custom-build-outputs"), String.Empty), ';');
            BuildData.BuildProperties["__private.customvcprojremoteroot"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("customvcprojremoteroot"), String.Empty);

            BuildData.BuildProperties["package.perforceintegration"] = PropertyUtil.GetPropertyOrDefault(Project, "package.perforceintegration", "false");
            BuildData.BuildProperties["package.consoledeployment"] = PropertyUtil.GetPropertyOrDefault(Project, "package.consoledeployment", "false");

            BuildData.BuildProperties["post-build-step"] = GetPostBuildStep(ProjBuildData, BuildOutput.OutputLibDir, BuildOutput.OutputBinDir, outputbinary, ccoutputpath);

            BuildData.BuildProperties["pre-build-step"] = GetPreBuildStep(ProjBuildData);

            BuildData.BuildProperties["pre-link-step"] = Properties[PropGroupName("vcproj.pre-link-step")];

            BuildData.BuildProperties["outputbinary"] = outputbinary;

            BuildData.BuildProperties["finalPath"] = finalPath;
        }
    }
}
